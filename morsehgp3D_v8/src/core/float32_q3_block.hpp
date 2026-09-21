#pragma once

#include <array>
#include <cstdint>

#include "core/float32_predicates.hpp"
#include "spatial/float32_index.hpp"

namespace mhgp8 {

// SUM counters, not candidate populations. A division of two intervals pays
// one interval_divisions and two scalar_divisions. Each bound query evaluates
// six continuous parabolas, three positions each (18 power_evaluations).
// interval_products counts an interval multiplication OR an interval square;
// elementary scalar operations within those outward bounds are not separate.
// Exceptions can leave partial counters; work must be exclusive to the caller.
struct Float32Q3BlockWork {
  std::uint64_t preparations{}, gram_positive{}, gram_unresolved{};
  std::uint64_t center_axes_tightened{}, center_intersection_fallbacks{};
  std::uint64_t bound_queries{}, inside_certificates{}, outside_certificates{}, unknown_bounds{};
  std::uint64_t axis_parabolas{}, vertex_clamps{}, power_evaluations{};
  std::uint64_t interval_additions{}, interval_products{}, interval_divisions{}, scalar_divisions{};
  bool operator==(const Float32Q3BlockWork&) const = default;
};

// Conditional envelope for ALL strictly acute triangles (a,b,x), x in X.
// Does NOT certify that a valid seed exists, impose ownership of ab, or remove
// invalid seed sites from the global witness population. No index/reference,
// exact-integer cache, mutable state or heap allocation is stored.
//
// Centres first lie in the box of hull(a,b,X). If interval G.low>0, intersect
// with a+W/(2G). Empty numerical intersections conservatively keep the hull;
// they NEVER expose a new seed-rejection certificate. All published endpoints
// are finite; temporary division infinities are capped before any query.
class Float32Q3Block final {
 public:
  using Interval = float32_predicate_detail::Interval;
  [[nodiscard]] static Float32Q3Block make(const Float32Point3& a, const Float32Point3& b,
                                          const Float32Box3& x, Float32Q3BlockWork& work);
  Float32Q3Block(const Float32Q3Block&) = default;
  Float32Q3Block(Float32Q3Block&&) = default;
  Float32Q3Block& operator=(const Float32Q3Block&) = delete;
  Float32Q3Block& operator=(Float32Q3Block&&) = delete;
  [[nodiscard]] const std::array<Interval, 3>& center_bounds() const noexcept { return centers_; }

  // Bounds physical power |z-c|^2-|a-c|^2 on the CONTINUOUS witness box Z,
  // for every valid seed in X. No integer-grid floor/ceil is used. Contact
  // remains UNKNOWN unless the whole interval separates zero strictly.
  [[nodiscard]] Interval bounds(const Float32Box3& z, Float32Q3BlockWork& work) const;
  // Calls bounds exactly once: -1 all strictly inside; +1 all strictly
  // outside; 0 UNKNOWN. This never supplies a payload or a census ticket.
  [[nodiscard]] int classify(const Float32Box3& z, Float32Q3BlockWork& work) const;

 private:
  Float32Q3Block(std::array<double, 3> origin, std::array<Interval, 3> centers)
      : origin_(origin), centers_(centers) {}
  std::array<double, 3> origin_;
  std::array<Interval, 3> centers_;
};

}  // namespace mhgp8
