#include "morsehgp3d/spatial/point_cloud.hpp"

#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/level_order.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::spatial {
namespace {

static_assert(
    CanonicalPointCloud::max_point_id ==
    exact::CanonicalSupportIds::maximum_point_id);

struct CanonicalPointCandidate {
  std::array<std::uint64_t, 3> order_keys;
  std::size_t source_index;
};

[[nodiscard]] CanonicalPointCandidate canonical_candidate(
    const exact::CertifiedPoint3& point, std::size_t source_index) {
  const std::array<std::uint64_t, 3> bits = point.canonical_input_bits();
  return CanonicalPointCandidate{
      {
          exact::binary64_total_order_key(bits[0]),
          exact::binary64_total_order_key(bits[1]),
          exact::binary64_total_order_key(bits[2]),
      },
      source_index};
}

}  // namespace

CanonicalPointCloud CanonicalPointCloud::rejecting_duplicates(
    std::span<const exact::CertifiedPoint3> input_points) {
  if (input_points.empty()) {
    throw std::invalid_argument("a canonical point cloud cannot be empty");
  }
  if (!std::in_range<PointId>(input_points.size())) {
    throw std::length_error("the point count does not fit the PointId storage type");
  }
  const PointId point_count = static_cast<PointId>(input_points.size());
  if (point_count > max_point_count) {
    throw std::length_error("the point count exceeds the canonical PointId domain");
  }

  std::vector<CanonicalPointCandidate> candidates;
  candidates.reserve(input_points.size());
  for (std::size_t source_index = 0; source_index < input_points.size(); ++source_index) {
    candidates.push_back(canonical_candidate(input_points[source_index], source_index));
  }

  std::sort(
      candidates.begin(), candidates.end(),
      [](const CanonicalPointCandidate& left, const CanonicalPointCandidate& right) {
        return left.order_keys < right.order_keys;
      });

  for (std::size_t index = 1; index < candidates.size(); ++index) {
    // The total-order transform is injective on canonical finite binary64
    // words, so equal key triples are exactly duplicate geometric points.
    if (candidates[index - 1U].order_keys == candidates[index].order_keys) {
      throw std::invalid_argument(
          "a canonical point cloud cannot contain duplicate geometric points");
    }
  }

  std::vector<exact::CertifiedPoint3> points;
  std::vector<std::size_t> source_indices;
  points.reserve(candidates.size());
  source_indices.reserve(candidates.size());
  for (const CanonicalPointCandidate& candidate : candidates) {
    const exact::CertifiedPoint3& source_point =
        input_points[candidate.source_index];
    const std::array<std::uint64_t, 3> canonical_bits =
        source_point.canonical_input_bits();
    // Reuse the exact rationals already certified by the caller. Only a
    // signed-zero input needs rebuilding so that the stored words become +0.
    if (source_point.input_bits() == canonical_bits) {
      points.push_back(source_point);
    } else {
      points.push_back(
          exact::CertifiedPoint3::from_binary64_bits(canonical_bits));
    }
    source_indices.push_back(candidate.source_index);
  }

  return CanonicalPointCloud{std::move(points), std::move(source_indices)};
}

CanonicalPointCloud::CanonicalPointCloud(
    std::vector<exact::CertifiedPoint3> points,
    std::vector<std::size_t> source_indices)
    : points_(std::move(points)),
      source_indices_(std::move(source_indices)),
      identity_(std::make_shared<const IdentityToken>()) {
  if (points_.empty() || points_.size() != source_indices_.size()) {
    throw std::logic_error("invalid canonical point cloud storage");
  }
}

CanonicalPointCloud& CanonicalPointCloud::operator=(
    const CanonicalPointCloud& other) {
  if (this != &other) {
    CanonicalPointCloud copy{other};
    *this = std::move(copy);
  }
  return *this;
}

CanonicalPointCloud::CanonicalPointCloud(CanonicalPointCloud&& other) noexcept
    : points_(std::move(other.points_)),
      source_indices_(std::move(other.source_indices_)),
      identity_(std::move(other.identity_)) {}

CanonicalPointCloud& CanonicalPointCloud::operator=(
    CanonicalPointCloud&& other) noexcept {
  if (this != &other) {
    points_ = std::move(other.points_);
    source_indices_ = std::move(other.source_indices_);
    identity_ = std::move(other.identity_);
  }
  return *this;
}

std::size_t CanonicalPointCloud::size() const noexcept {
  return identity_ != nullptr ? points_.size() : 0U;
}

std::size_t CanonicalPointCloud::checked_index(PointId id) const {
  if (identity_ == nullptr) {
    throw std::logic_error("a moved-from canonical point cloud cannot be queried");
  }
  if (id > max_point_id || !std::in_range<std::size_t>(id)) {
    throw std::out_of_range("a PointId is outside the canonical point cloud domain");
  }
  const std::size_t index = static_cast<std::size_t>(id);
  if (index >= points_.size()) {
    throw std::out_of_range("a PointId is outside the canonical point cloud");
  }
  return index;
}

const exact::CertifiedPoint3& CanonicalPointCloud::point(PointId id) const & {
  return points_[checked_index(id)];
}

std::size_t CanonicalPointCloud::source_index(PointId id) const {
  return source_indices_[checked_index(id)];
}

}  // namespace morsehgp3d::spatial
