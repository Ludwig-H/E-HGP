#pragma once

#include <array>
#include <cstdint>

#include "spatial/float32_index.hpp"

namespace mhgp8 {

// SUM operations, not candidate populations. Edge preparation is paid once
// per prepared value, block preparation once per X; one bound query pays six
// continuous parabolas / eighteen evaluations. No mutable shared counters.
struct Float32Q3OwnedBlockWork {
  std::uint64_t edge_preparations{}, edge_squared_positive{}, edge_squared_unresolved{};
  std::uint64_t block_preparations{}, projected_envelopes{}, gram_positive{}, gram_unresolved{};
  std::uint64_t xi_tightened{}, xi_intersection_fallbacks{};
  std::uint64_t center_axes_tightened{}, center_intersection_fallbacks{};
  std::uint64_t bound_queries{}, inside_certificates{}, outside_certificates{}, unknown_bounds{};
  std::uint64_t axis_parabolas{}, vertex_clamps{}, power_evaluations{};
  std::uint64_t interval_additions{}, interval_products{}, interval_divisions{}, scalar_divisions{};
  bool operator==(const Float32Q3OwnedBlockWork&) const = default;
};

// Bounds ONLY centres of strictly acute triangles whose maximal edge is ab.
// X may contain other points: they are not certified seeds, and must remain
// witnesses. This stronger contract MUST NOT replace Float32Q3Block's free
// acute-seed contract. Lexicographic tie ownership is imposed by the caller.
class Float32Q3OwnedBlock final {
 public:
  using Interval = float32_predicate_detail::Interval;
  class PreparedEdge final {
   public:
    PreparedEdge(const PreparedEdge&) = default;
    PreparedEdge(PreparedEdge&&) = default;
    PreparedEdge& operator=(const PreparedEdge&) = delete;
    PreparedEdge& operator=(PreparedEdge&&) = delete;
   private:
    friend class Float32Q3OwnedBlock;
    PreparedEdge(std::array<double, 3> a, std::array<double, 3> b,
                 std::array<Interval, 3> delta, std::array<Interval, 3> midpoint,
                 Interval diameter, std::array<std::array<Interval, 3>, 3> projection, bool ready)
        : a_(a), b_(b), delta_(delta), midpoint_(midpoint), diameter_(diameter),
          projection_(projection), ready_(ready) {}
    std::array<double, 3> a_, b_;
    std::array<Interval, 3> delta_, midpoint_;
    Interval diameter_;
    std::array<std::array<Interval, 3>, 3> projection_;
    bool ready_;
  };

  [[nodiscard]] static PreparedEdge prepare_edge(const Float32Point3& a, const Float32Point3& b,
                                                Float32Q3OwnedBlockWork& work);
  [[nodiscard]] static Float32Q3OwnedBlock make(const PreparedEdge&, const Float32Box3& x,
                                               Float32Q3OwnedBlockWork& work);
  // Convenience for standalone tests; a long-lived caller should reuse Edge.
  [[nodiscard]] static Float32Q3OwnedBlock make(const Float32Point3& a, const Float32Point3& b,
                                               const Float32Box3& x, Float32Q3OwnedBlockWork& work);
  Float32Q3OwnedBlock(const Float32Q3OwnedBlock&) = default;
  Float32Q3OwnedBlock(Float32Q3OwnedBlock&&) = default;
  Float32Q3OwnedBlock& operator=(const Float32Q3OwnedBlock&) = delete;
  Float32Q3OwnedBlock& operator=(Float32Q3OwnedBlock&&) = delete;
  [[nodiscard]] const std::array<Interval, 3>& center_bounds() const noexcept { return centers_; }
  [[nodiscard]] Interval bounds(const Float32Box3& z, Float32Q3OwnedBlockWork&) const;
  // -1 strictly inside for every valid owned seed, +1 strictly outside, 0
  // unknown/contact. No seed-existence certificate, ticket or payload.
  [[nodiscard]] int classify(const Float32Box3& z, Float32Q3OwnedBlockWork&) const;

 private:
  Float32Q3OwnedBlock(std::array<double, 3> a, std::array<Interval, 3> centers)
      : origin_(a), centers_(centers) {}
  std::array<double, 3> origin_;
  std::array<Interval, 3> centers_;
};

}  // namespace mhgp8
