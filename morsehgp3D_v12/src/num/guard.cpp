// Boule certifiee et garde entiere (num/guard.hpp) : certificat barycentrique, pave, point entier le plus proche en
// local, budget mixte et certificat au domaine s+2. Construction neuve de la v12 ; elle remplace, pour les boules
// certifiees, LatticeSphere de la v11 (centre absolu sur 5B+6 bits, coins de boites n'importe ou dans le domaine).
// Ici : la preparation et les voies hors ligne (essai controle, repli large) ; les decisions et la puissance des voies
// native et certifiee sont en ligne dans num/guard.hpp (census a plat, T2-d-B2).
#include "num/guard.hpp"
#include "num/big.hpp"
#include "num/center_view.hpp"
#include "num/lattice_internal.hpp"
#include "num/power_checked.hpp"

#include <algorithm>

namespace mhgp12::num {
namespace {

// Plancher de N_j/D et position du reste (r = 0, 2r <= D) quand N ou D sortent de i128 (palier large, profil 32) :
// division de Big (semantique de Python, reste du signe du diviseur positif). |quotient| < 2^34 : centre certifie.
struct WideFloor {
  i64 quotient = 0;
  bool zero = false, half_or_less = false;
};
Result<WideFloor> wide_floor(const CenterInt& numerator, const CenterDen& denominator) noexcept {
  const Big n = Big::from_wide(to_wide(numerator)), d = Big::from_wide(to_wide(denominator));
  Big quotient, remainder, twice;
  MHGP12_TRY(divide(n, d, quotient, remainder));
  MHGP12_TRY(shift_left(remainder, 1, twice));
  const auto magnitude = quotient.magnitude_u128();
  if (!magnitude || *magnitude > (u128{1} << 34)) return fail(Reason::arithmetic_invariant);
  WideFloor out;
  out.quotient = quotient.negative() ? -static_cast<i64>(*magnitude) : static_cast<i64>(*magnitude);
  out.zero = remainder.is_zero();
  out.half_or_less = compare(twice, d) <= 0;
  return out;
}

}  // namespace

Result<std::optional<CertifiedBall>> CertifiedBall::certify(std::span<const Point> support) noexcept {
  if (support.empty() || support.size() > 4) return fail(Reason::parameter_out_of_range);
  Frame frame;
  for (const Point p : support) frame.add_site(p.x(), p.y(), p.z());
  const std::array<u32, 3> corner{static_cast<u32>(frame.origin()[0]), static_cast<u32>(frame.origin()[1]),
                                  static_cast<u32>(frame.origin()[2])};
  const auto certified = [&](const Sphere& sphere) {
    return std::optional<CertifiedBall>{CertifiedBall(sphere, corner, frame.span())};
  };
  if (support.size() == 1) return certified(Sphere::point(support[0]));
  if (support.size() == 2) {  // milieu du segment : toujours dans l'enveloppe
    auto made = Sphere::through(support[0], support[1]);
    if (!made.ok()) return made.outcome();
    if (!made.value()) return std::optional<CertifiedBall>{};
    return certified(*made.value());
  }
  if (support.size() == 3) {  // centre circonscrit dans le triangle ouvert ssi les trois angles sont aigus
    if (classify_triangle(support[0], support[1], support[2]) != TriangleKind::strict)
      return std::optional<CertifiedBall>{};
    auto made = Sphere::through(support[0], support[1], support[2]);
    if (!made.ok()) return made.outcome();
    if (!made.value()) return std::optional<CertifiedBall>{};
    return certified(*made.value());
  }
  // q4 : quatre poids barycentriques de la presentation strictement positifs.
  auto candidate = Q4Candidate::through(support[0], support[1], support[2], support[3]);
  if (!candidate.ok()) return candidate.outcome();
  if (!candidate.value() || !candidate.value()->q4_presentation_strictly_inside()) return std::optional<CertifiedBall>{};
  auto made = candidate.value()->materialize();
  if (!made.ok()) return made.outcome();
  return certified(made.value());
}

GuardedSphere::GuardedSphere(const CertifiedBall& ball) noexcept : ball_(ball) {
  const Sphere& sphere = ball.sphere();
  const detail::CenterView view(sphere);
  const int s = ball.span();
  const i64 m = i64{1} << s;
  for (int j = 0; j < 3; ++j) {
    anchor_[j] = sphere.anchor().coordinates()[j];
    guard_lo_[j] = i64{ball.corner()[j]} - m;
    guard_hi_[j] = i64{ball.corner()[j]} + 2 * m;
  }
  native_coefficients_ = view.native_coefficients();
  short_norm_ = s + 2 <= 30;
  // Voie uniforme : palier etroit (6s+11 <= 107), sinon certificat au domaine des requetes gardees, t = s+2.
  if (tier_of(s) == Tier::narrow) lane_ = Lane::native;
  else if (native_coefficients_ && sphere.power_domain() >= s + 2) lane_ = Lane::certified;
  else lane_ = native_coefficients_ ? Lane::checked : Lane::wide;
  if (native_coefficients_) {
    denominator_ = view.d128();
    numerator_ = view.n128();
  }
  for (int j = 0; j < 3; ++j) {
    // Point entier le plus proche en LOCAL : N_j = q D + r, 0 <= r < D ; c_j - o_j est dans ]-M, M[.
    i64 q = 0;
    bool zero = false, half_or_less = false;
    if (native_coefficients_) {
      const auto [quotient, remainder] = detail::floor_division(numerator_[j], denominator_);
      q = detail::saturate(quotient, -m, m);
      zero = remainder == 0;
      half_or_less = 2 * remainder <= denominator_;
    } else if (const auto floor = wide_floor(sphere.numerator()[j], sphere.denominator()); floor.ok()) {
      q = floor.value().quotient;
      zero = floor.value().zero;
      half_or_less = floor.value().half_or_less;
    } else {
      broken_ = true;
    }
    nearest_[j] = q + (half_or_less ? 0 : 1);  // ex aequo 2r = D : le plus petit, meme distance
    threshold_[j] = 2 * q + (zero ? 0 : half_or_less ? 1 : 2);  // ceil(2 (c_j - o_j))
  }
  inline_lane_ = !broken_ && (lane_ == Lane::native || lane_ == Lane::certified);
}

// Voies hors ligne de power_sign (num/guard.hpp) : preparation impossible, essai controle, repli large.
Outcome GuardedSphere::slow_sign(const std::array<i64, 3>& v, int& sign, GuardLedger* ledger) const noexcept {
  if (broken_) return fail(Reason::arithmetic_invariant);
  LaneCount* lanes = ledger == nullptr ? nullptr : &ledger->lanes;
  if (lane_ == Lane::checked) {
    const std::array<i128, 3> factors{-2 * i128{v[0]}, -2 * i128{v[1]}, -2 * i128{v[2]}};
    if (const auto value = detail::checked_power_sum(denominator_, numerator_, detail::dot128(v, v), factors)) {
      count_lane(lanes, Lane::checked);
      sign = detail::sign(*value);
      return {};
    }
  }
  count_lane(lanes, Lane::wide);
  // Repli large a la largeur du palier de la boule : 6s+11 <= 155 bits (moyen, trois mots), 209 bits (large, quatre).
  constexpr int medium = words_for(TierBudgets<Tier::medium>::guarded_side);
  constexpr int wide = words_for(TierBudgets<Tier::wide>::guarded_side);
  const auto made = tier_of(ball_.span()) == Tier::wide ? wide_sign<wide>(v) : wide_sign<medium>(v);
  if (!made.ok()) return made.outcome();
  sign = made.value();
  return {};
}

template <int Words>
Result<int> GuardedSphere::wide_sign(const std::array<i64, 3>& v) const noexcept {
  const Sphere& sphere = ball_.sphere();
  auto total = detail::product<Words>(sphere.denominator(), detail::dot128(v, v));
  if (!total.ok()) return total.outcome();
  for (int j = 0; j < 3; ++j) {
    auto term = detail::product<Words>(sphere.numerator()[j], -2 * i128{v[j]});
    if (!term.ok()) return term.outcome();
    total = detail::require_add(total.value(), term.value());
    if (!total.ok()) return total.outcome();
  }
  return total.value().sign();
}

}  // namespace mhgp12::num
