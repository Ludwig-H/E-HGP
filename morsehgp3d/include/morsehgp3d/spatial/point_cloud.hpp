#pragma once

#include "morsehgp3d/exact/point.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace morsehgp3d::gpu {
class SpatialReferenceContext;
}

namespace morsehgp3d::spatial {

using PointId = std::uint64_t;
class ExclusionSet;
class MortonLbvhIndex;
class TopKPartition;
class ClosedBallPartition;

class CanonicalPointCloud {
 public:
  static constexpr PointId max_point_id = (PointId{1} << 53U) - PointId{1};
  static constexpr PointId max_point_count = max_point_id;

  [[nodiscard]] static CanonicalPointCloud rejecting_duplicates(
      std::span<const exact::CertifiedPoint3> input_points);

  CanonicalPointCloud(const CanonicalPointCloud&) = default;
  CanonicalPointCloud& operator=(const CanonicalPointCloud& other);
  CanonicalPointCloud(CanonicalPointCloud&& other) noexcept;
  CanonicalPointCloud& operator=(CanonicalPointCloud&& other) noexcept;

  [[nodiscard]] std::size_t size() const noexcept;

  [[nodiscard]] const exact::CertifiedPoint3& point(PointId id) const &;
  [[nodiscard]] const exact::CertifiedPoint3& point(PointId id) const && = delete;

  [[nodiscard]] std::size_t source_index(PointId id) const;

 private:
  struct IdentityToken {};

  CanonicalPointCloud(
      std::vector<exact::CertifiedPoint3> points,
      std::vector<std::size_t> source_indices);

  [[nodiscard]] std::size_t checked_index(PointId id) const;

  std::vector<exact::CertifiedPoint3> points_;
  std::vector<std::size_t> source_indices_;
  std::shared_ptr<const IdentityToken> identity_;

  friend class ExclusionSet;
  friend class MortonLbvhIndex;
  friend class TopKPartition;
  friend class ClosedBallPartition;
  friend class gpu::SpatialReferenceContext;
};

}  // namespace morsehgp3d::spatial
