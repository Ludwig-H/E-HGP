#pragma once

#include "core/types.hpp"

#include <array>
#include <optional>

namespace mhgp8 {

// A canonical geometric ball, NOT a support/incidence or an HGP level.
// Its primitive integer power is A*|z|^2 + B.z + C, with A>0 and the
// common gcd of all five coefficients equal to one. The sign is negative
// strictly inside, zero on the complete shell and positive outside.
// Equality identifies the same centre AND radius across different arities;
// equal radii alone do not identify a ball. No q_min is attached to this key.
//
// Only the checked factories can create a value; immutable private integer
// coefficients neither borrow points nor admit a forged coefficient array.
// Factories reject any coordinate outside [0,coordinate_limit] before
// arithmetic. This is not an inherited v7 certificate.
class ExactBall final {
 public:
  // Distinct endpoints of a diameter; duplicates return nullopt.
  [[nodiscard]] static std::optional<ExactBall> make_q2(Point3 a, Point3 b);
  // Strictly acute nondegenerate triangle; all other inputs return nullopt.
  [[nodiscard]] static std::optional<ExactBall> make_q3(std::array<Point3, 3> points);
  // Affinely independent tetrahedron whose circumcentre is STRICTLY inside
  // it. Flat/repeated inputs and centres on/outside a face return nullopt.
  [[nodiscard]] static std::optional<ExactBall> make_q4(std::array<Point3, 4> points);

  // Coefficient order: {A, Bx, By, Bz, C}. The reference lasts as long as
  // this immutable value; copying the value copies no coordinate owner.
  [[nodiscard]] const std::array<i128, 5>& coefficients() const noexcept { return coefficients_; }
  // Precondition: z is in the certified coordinate domain (e.g. a point
  // from PreparedCloud). No per-census-point range scan is performed here.
  [[nodiscard]] i128 power(Point3 z) const noexcept;
  [[nodiscard]] bool operator==(const ExactBall& other) const noexcept {
    return coefficients_ == other.coefficients_;
  }

 private:
  explicit ExactBall(std::array<i128, 5> coefficients) noexcept : coefficients_(coefficients) {}
  const std::array<i128, 5> coefficients_;
};

}  // namespace mhgp8
