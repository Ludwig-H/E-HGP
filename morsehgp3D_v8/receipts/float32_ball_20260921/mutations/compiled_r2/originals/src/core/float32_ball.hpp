#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "core/float32_predicates.hpp"

namespace mhgp8 {

// SUM counters. Additions include subtraction; negation/assignment are not
// arithmetic counters. A filter acceptance is a certified DECISION (also a
// rejected support), not necessarily an accepted geometric support.
struct Float32BallWork {
  std::uint64_t q3_preparations{}, q4_preparations{};
  std::uint64_t preparation_filter_attempts{}, preparation_filter_accepts{};
  std::uint64_t preparation_exact_fallbacks{}, preparation_exact_evaluations{};
  std::uint64_t accepted_supports{}, rejected_supports{};
  std::uint64_t power_queries{}, power_filter_attempts{}, power_filter_accepts{};
  std::uint64_t power_exact_fallbacks{}, power_exact_evaluations{};
  std::uint64_t interval_additions{}, interval_products{};
  std::uint64_t exact_additions{}, exact_products{}, exact_point_decodes{};
  bool operator==(const Float32BallWork&) const = default;
};

// A strictly positive q3/q4 support with its circumball in the support's
// affine hull. Stores original bits and four interval coefficients, NOT a
// canonical global ball key. Multiple supports may represent the same ball.
// No heap allocation or mutable lazy cache: prepared values can be shared,
// exact temporary arithmetic and counters belong to the calling worker.
class Float32Ball final {
 public:
  [[nodiscard]] static std::optional<Float32Ball> make_q3(
      std::array<Float32Point3, 3> points, Float32PredicateMode mode, Float32BallWork& work);
  [[nodiscard]] static std::optional<Float32Ball> make_q4(
      std::array<Float32Point3, 4> points, Float32PredicateMode mode, Float32BallWork& work);

  Float32Ball(const Float32Ball&) = default;
  Float32Ball(Float32Ball&&) = default;
  Float32Ball& operator=(const Float32Ball&) = delete;
  Float32Ball& operator=(Float32Ball&&) = delete;
  [[nodiscard]] std::size_t arity() const noexcept { return arity_; }
  [[nodiscard]] Float32PredicateMode mode() const noexcept { return mode_; }

  // -1 strictly inside, 0 on the ENTIRE sphere, +1 outside. No epsilon.
  // ExactOnly factory/queries use no floating arithmetic. Filtered queries
  // reuse coefficients made at preparation; ambiguous signs recompute exact
  // coefficients from the immutable bits. Exceptions leave partial work.
  [[nodiscard]] int power_sign(const Float32Point3& z, Float32BallWork& work) const;

 private:
  using Interval = float32_predicate_detail::Interval;
  Float32Ball(std::array<Float32Point3, 4> points, std::size_t arity,
              Float32PredicateMode mode, Interval scale, std::array<Interval, 3> linear)
      : points_(points), arity_(arity), mode_(mode), scale_(scale), linear_(linear) {}
  [[nodiscard]] static std::optional<Float32Ball> make(
      std::array<Float32Point3, 4> points, std::size_t arity,
      Float32PredicateMode mode, Float32BallWork& work);
  std::array<Float32Point3, 4> points_;
  std::size_t arity_;
  Float32PredicateMode mode_;
  Interval scale_;
  std::array<Interval, 3> linear_;
};

}  // namespace mhgp8
