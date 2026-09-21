#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "core/float32_ball.hpp"

namespace mhgp8 {

// SUM throughout. The nested record pays the independent q3 eligibility
// preparation; the other arithmetic counts belong only to event predicates.
// side_queries includes the TWO side calls made by each root query, even
// when a coplanar site makes that comparison invalid. Exceptions may leave
// partial counters. There is no internal mutable state or shared work record.
struct Float32Q4EventsWork {
  Float32BallWork seed{};
  std::uint64_t preparations{}, accepted_seeds{}, rejected_seeds{}, interval_preparations{};
  std::uint64_t side_queries{}, side_filter_attempts{}, side_filter_accepts{};
  std::uint64_t side_exact_fallbacks{}, side_exact_evaluations{};
  std::uint64_t root_queries{}, root_coplanar_rejections{}, root_filter_attempts{};
  std::uint64_t root_filter_accepts{}, root_exact_fallbacks{}, root_exact_evaluations{}, root_equalities{};
  std::uint64_t interval_additions{}, interval_products{};
  std::uint64_t exact_additions{}, exact_products{}, exact_point_decodes{};
  bool operator==(const Float32Q4EventsWork&) const = default;
};

// Ordered strictly acute seed (a,b,x), normal n=(b-a) cross (x-a).
// Compact prepared intervals plus immutable original binary32 points;
// fixed-size exact temporaries are made only in the calling worker.
// No root coordinates, global ball keys, heap storage or event list.
class Float32Q4Events final {
 public:
  [[nodiscard]] static std::optional<Float32Q4Events> make(
      std::array<Float32Point3, 3> points, Float32PredicateMode mode, Float32Q4EventsWork& work);
  Float32Q4Events(const Float32Q4Events&) = default;
  Float32Q4Events(Float32Q4Events&&) = default;
  Float32Q4Events& operator=(const Float32Q4Events&) = delete;
  Float32Q4Events& operator=(Float32Q4Events&&) = delete;

  [[nodiscard]] Float32PredicateMode mode() const noexcept { return mode_; }
  [[nodiscard]] const std::array<Float32Point3, 3>& points() const noexcept { return points_; }
  // Sign of B_z=n.(z-a), including exact zero; not a positivity predicate.
  [[nodiscard]] int side(const Float32Point3& z, Float32Q4EventsWork& work) const;
  // Sign of P1/B1-P2/B2, where P=G|z-a|^2-W.(z-a), G>0 is
  // the seed Gram determinant. Throws invalid_argument if either B is zero,
  // BEFORE computing a root determinant. Equality is exact, never epsilon.
  // Uses a reduced degree-five determinant, NEVER the degree-nine P*B.
  [[nodiscard]] int compare_roots(const Float32Point3& z1, const Float32Point3& z2,
                                  Float32Q4EventsWork& work) const;

 private:
  using Interval = float32_predicate_detail::Interval;
  Float32Q4Events(std::array<Float32Point3, 3> points, Float32PredicateMode mode,
                  std::array<Interval, 6> minors, std::array<Interval, 3> normal)
      : points_(points), mode_(mode), minors_(minors), normal_(normal) {}
  std::array<Float32Point3, 3> points_;
  Float32PredicateMode mode_;
  std::array<Interval, 6> minors_;
  std::array<Interval, 3> normal_;
};

}  // namespace mhgp8
