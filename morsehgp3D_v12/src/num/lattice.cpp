// Bornes entieres du census, voie generique (num/lattice_bounds.hpp) : preparation en repere local, sans centre absolu.
// Port de LatticeSphere de la v11 (src/num/predicates.cpp, commit ac081a06f), dont le constructeur formait
// C_j = a_j D + N_j sur 5B+6 bits (CST-0109) ; ici le plancher de N_j/D se prend en local puis l'ancre s'ajoute.
#include "num/lattice_bounds.hpp"
#include "num/center_view.hpp"
#include "num/lattice_internal.hpp"

#include <algorithm>

namespace mhgp12::num {

LatticeSphere::LatticeSphere(const Sphere& sphere) noexcept : sphere_(sphere), lattice_(false) {
  const detail::CenterView view(sphere);
  // Domaine de la voie generique : boites et sites n'importe ou dans [0,2^B), etendue B autour de l'ancre.
  lane_ = detail::power_lane(view, kCoordBits);
  lattice_ = lane_ == Lane::native || lane_ == Lane::certified;
  if (!lattice_) return;
  constexpr i64 m = i64{1} << kCoordBits;
  denominator_ = view.d128();  // D>0 pour toute Sphere fabriquee
  for (int j = 0; j < 3; ++j) {
    anchor_[j] = sphere.anchor().coordinates()[j];
    numerator_[j] = view.n128()[j];
  }
  for (int j = 0; j < 3; ++j) {
    // Plancher en LOCAL : N_j = q D + r, 0 <= r < D, sans former a_j D + N_j.
    const auto [q, remainder] = detail::floor_division(numerator_[j], denominator_);
    // Saturer le plancher avant tout produit : au-dela de [-2,M+1], ni le point proche ramene dans une boite de
    // [0,M-1] ni le coin eloigne ne changent plus. 2*remainder<2D<2^102 tient dans i128.
    const i64 whole = anchor_[j] + detail::saturate(q, -2 - anchor_[j], m + 1 - anchor_[j]);  // floor(c_j) sature
    const i64 nearest = whole + (2 * remainder > denominator_ ? 1 : 0);  // ex aequo 2r=D : le plus petit
    const i64 twice = 2 * whole + (remainder == 0 ? 0 : 2 * remainder <= denominator_ ? 1 : 2);  // ceil(2c_j)
    nearest_[j] = std::clamp<i64>(nearest, 0, m - 1);
    far_threshold_[j] = std::clamp<i64>(twice, 0, 2 * m - 1);
  }
}

// Precondition : lattice_. Meme expression et meme ordre que native_power ; les points evalues sont des points du
// domaine (sommets ou points entiers d'une Box, sites d'un Point), donc ses majorants valent ici a l'identique.
i128 LatticeSphere::power_at(const std::array<i64, 3>& point) const noexcept {
  const std::array<i64, 3> v{point[0] - anchor_[0], point[1] - anchor_[1], point[2] - anchor_[2]};
  // |v_j| < 2^B : norme en i64 jusqu'au profil 30, en i128 au-dela.
  const i128 norm = kCoordBits <= 30 ? i128{detail::dot(v, v)} : detail::dot128(v, v);
  i128 power = denominator_ * norm;
  for (int j = 0; j < 3; ++j) power += numerator_[j] * (-2 * i128{v[j]});
  return power;
}

Result<PowerBoundSigns> LatticeSphere::bound_signs(const Box& box, LaneCount* lanes) const noexcept {
  if (!lattice_) return power_bound_signs(sphere_, box, lanes);
  const auto lo = box.lo().coordinates(), hi = box.hi().coordinates();  // copies : lo()/hi() rendent des valeurs
  std::array<i64, 3> near{}, far{};
  for (int j = 0; j < 3; ++j) {
    near[j] = std::clamp<i64>(nearest_[j], lo[j], hi[j]);
    far[j] = i64{lo[j]} + hi[j] >= far_threshold_[j] ? hi[j] : lo[j];
  }
  // Deux points de la boite fermee, donc du domaine : la puissance native garde ses budgets, pour tout Point.
  count_lane(lanes, lane_);
  const i128 lower = power_at(near);
  if (lower > 0) return PowerBoundSigns{.lower = 1, .upper = 1};
  count_lane(lanes, lane_);
  const i128 upper = power_at(far);
  if (lower > upper) return fail(Reason::arithmetic_invariant);
  return PowerBoundSigns{.lower = detail::sign(lower), .upper = detail::sign(upper)};
}

Result<int> LatticeSphere::side(Point point, LaneCount* lanes) const noexcept {
  if (!lattice_) return num::side(sphere_, point, lanes);
  const auto c = point.coordinates();
  count_lane(lanes, lane_);
  return detail::sign(power_at({i64{c[0]}, i64{c[1]}, i64{c[2]}}));
}

}  // namespace mhgp12::num
