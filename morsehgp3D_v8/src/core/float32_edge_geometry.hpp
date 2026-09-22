#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "spatial/float32_index.hpp"

namespace mhgp8 {

// SUM counters; checked increments can leave partial work on exception.
// Mode controls scalar predicates only: box certificates always use intervals.
struct Float32EdgeWork {
  std::uint64_t preparations{}, owner_queries{}, owner_filter_accepts{}, owner_exact_queries{};
  std::uint64_t owner_equalities{}, owner_rejections{}, owner_box_queries{}, owner_box_rejections{};
  std::uint64_t citron_box_queries{}, citron_inside{}, citron_outside{}, citron_unknown{};
  std::uint64_t citron_point_queries{}, citron_filter_accepts{}, citron_exact_queries{};
  std::uint64_t separation_queries{}, separation_filter_accepts{}, separation_exact_queries{};
  std::uint64_t interval_additions{}, interval_products{}, exact_additions{}, exact_products{}, exact_decodes{};
  bool operator==(const Float32EdgeWork&) const = default;
};

// Immutable prepared fixed edge; no index/raw-node reference or mutable cache.
// The endpoint IDs must differ. Equal endpoint coordinates are rejected.
class Float32EdgeGeometry final {
 public:
  [[nodiscard]] static Float32EdgeGeometry make(
      const Float32Point3& a, std::size_t a_id, const Float32Point3& b, std::size_t b_id,
      Float32PredicateMode mode, Float32EdgeWork& work);
  Float32EdgeGeometry(const Float32EdgeGeometry&) = default;
  Float32EdgeGeometry(Float32EdgeGeometry&&) = default;
  Float32EdgeGeometry& operator=(const Float32EdgeGeometry&) = delete;
  Float32EdgeGeometry& operator=(Float32EdgeGeometry&&) = delete;
  // Maximal length, then SMALLEST sorted original-ID pair. Does not certify
  // acuteness. An endpoint ID returns false, without changing the witness set.
  [[nodiscard]] bool owns(const Float32Point3& x, std::size_t x_id, Float32EdgeWork&) const;
  // False implies there is NO strictly acute, owned seed in X. Equality in
  // maximal-edge distance is retained (pointwise tie-breaking comes later).
  [[nodiscard]] bool may_own(const Float32Box3& x, Float32EdgeWork&) const;
  // -1: ALL Z strictly satisfy H>0 and 3H^2>Xi; +1: NONE; 0: UNKNOWN.
  // A singleton Z is decided exactly when the interval is inconclusive.
  [[nodiscard]] int citron(const Float32Box3& z, Float32EdgeWork&) const;
  // Exact boolean, including contact=false. Mode controls interval filtering.
  [[nodiscard]] bool citron(const Float32Point3& z, Float32EdgeWork&) const;
 private:
  using Interval = float32_predicate_detail::Interval;
  Float32EdgeGeometry(std::array<Float32Point3, 2> points, std::array<std::size_t, 2> ids,
                      Float32PredicateMode mode, std::array<Interval, 3> delta, Interval diameter)
      : points_(points), ids_(ids), mode_(mode), delta_(delta), diameter_(diameter) {}
  std::array<Float32Point3, 2> points_;
  std::array<std::size_t, 2> ids_;
  Float32PredicateMode mode_;
  std::array<Interval, 3> delta_;
  Interval diameter_;
};

// GENERAL product A x B x Z, never specialized to box centers. Same sign
// convention as edge.citron(box). Exact fallback only for three singletons.
[[nodiscard]] int float32_q3_citron_boxes(
    const Float32Box3& a, const Float32Box3& b, const Float32Box3& z, Float32EdgeWork&);

// Exact v8 convention gap^2 >= s^2 * max(diagA^2,diagB^2), s>=1.
// Outward filter then fixed-integer fallback; equality is separated.
[[nodiscard]] bool float32_boxes_separated(
    const Float32Box3& a, const Float32Box3& b, std::uint32_t s, Float32EdgeWork&);

}  // namespace mhgp8
