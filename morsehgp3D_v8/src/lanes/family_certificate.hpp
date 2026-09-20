#pragma once

#include "lanes/q4_family.hpp"

#include <optional>

namespace mhgp8 {

struct Q34FamilyWitness {
  bool q3{};  // Strictly inside the face's circumball.
  bool q4{};  // Strictly inside EVERY positive q4 ball with maximal edge ab.
  bool operator==(const Q34FamilyWitness&) const = default;
};

// Immutable geometric certificate for a strictly acute triangle abx with ab
// a maximal-length edge. Tied lengths are allowed: original-ID ownership is
// deliberately NOT part of this primitive. It owns a copy of the family;
// copies of the certificate are independent values, not borrowed aliases.
//
// D=|b-a|^2, E=|x-a|^2, X=|x-b|^2, G=Gram(a,b,x), J=D*(3G-2EX).
// Every positive q4 ball of maximal edge ab satisfies 2*mu^2<=J. The integer
// U=ceil(sqrt(ceil(J/2))) encloses that parameter interval. This is the family
// parameter mu, NOT a bound on the Euclidean radius of an arbitrary ball.
class Q34FamilyCertificate final {
 public:
  // A repeated/collinear/right/obtuse triangle or a nonmaximal ab returns
  // nullopt. There is no floating-point proposal or approximate decision.
  [[nodiscard]] static std::optional<Q34FamilyCertificate> make(Point3 a, Point3 b, Point3 x);

  [[nodiscard]] i64 radius_parameter_bound() const noexcept { return parameter_bound_; }
  [[nodiscard]] i128 j_bound() const noexcept { return j_bound_; }
  [[nodiscard]] u64 sqrt_iterations() const noexcept { return sqrt_iterations_; }
  [[nodiscard]] const Q4FamilySeed& family() const noexcept { return family_; }

  // Computes P and B once. q3 iff P<0; q4 iff P+U*|B|<0. Equality is NOT a
  // witness. q4 implies q3, but lane thresholds differ and must stay separate.
  // False does not prove exteriority or absence of a different certificate.
  // Distinct witness IDs/counting and lane rejection belong to the caller.
  [[nodiscard]] Q34FamilyWitness witness(Point3 z) const noexcept;

 private:
  Q34FamilyCertificate(const Q4FamilySeed& family, i128 j_bound, i64 parameter_bound,
                       u64 sqrt_iterations) noexcept;
  const Q4FamilySeed family_;
  const i128 j_bound_;
  const i64 parameter_bound_;
  const u64 sqrt_iterations_;
};

}  // namespace mhgp8
