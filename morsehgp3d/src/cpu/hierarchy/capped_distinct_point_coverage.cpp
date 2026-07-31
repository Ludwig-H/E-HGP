#include "morsehgp3d/hierarchy/capped_distinct_point_coverage.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {

CappedDistinctPointCoverage::CappedDistinctPointCoverage(
    std::size_t threshold)
    : threshold_(threshold) {
  if (threshold_ == 0U) {
    throw std::invalid_argument(
        "a capped distinct-point coverage threshold must be positive");
  }
}

CappedDistinctPointCoverage::CappedDistinctPointCoverage(
    CappedDistinctPointCoverage&& other) noexcept
    : threshold_(other.threshold_),
      smallest_distinct_point_ids_(
          std::move(other.smallest_distinct_point_ids_)) {
  other.smallest_distinct_point_ids_.clear();
}

CappedDistinctPointCoverage& CappedDistinctPointCoverage::operator=(
    CappedDistinctPointCoverage&& other) noexcept {
  if (this != &other) {
    threshold_ = other.threshold_;
    smallest_distinct_point_ids_ =
        std::move(other.smallest_distinct_point_ids_);
    other.smallest_distinct_point_ids_.clear();
  }
  return *this;
}

std::optional<std::size_t>
CappedDistinctPointCoverage::exact_coverage_size_if_below_threshold()
    const noexcept {
  if (coverage_size_at_least_threshold()) {
    return std::nullopt;
  }
  return smallest_distinct_point_ids_.size();
}

bool CappedDistinctPointCoverage::insert_point_id(
    spatial::PointId point_id) {
  auto position = std::lower_bound(
      smallest_distinct_point_ids_.begin(),
      smallest_distinct_point_ids_.end(),
      point_id);
  if (position != smallest_distinct_point_ids_.end() &&
      *position == point_id) {
    return false;
  }

  if (smallest_distinct_point_ids_.size() < threshold_) {
    smallest_distinct_point_ids_.insert(position, point_id);
    return true;
  }

  if (position == smallest_distinct_point_ids_.end()) {
    return false;
  }

  // Keep the arena at exactly threshold entries even during insertion.  The
  // former largest identifier is overwritten after a right shift.
  std::move_backward(
      position,
      smallest_distinct_point_ids_.end() - 1,
      smallest_distinct_point_ids_.end());
  *position = point_id;
  return true;
}

void CappedDistinctPointCoverage::insert_facet(
    std::span<const spatial::PointId> facet_point_ids) {
  for (const spatial::PointId point_id : facet_point_ids) {
    static_cast<void>(insert_point_id(point_id));
  }
}

void CappedDistinctPointCoverage::insert_facets(
    std::span<const std::vector<spatial::PointId>> facet_point_ids) {
  for (const std::vector<spatial::PointId>& facet : facet_point_ids) {
    insert_facet(facet);
  }
}

void CappedDistinctPointCoverage::insert_coverage_delta(
    std::span<const std::vector<spatial::PointId>>
        added_facet_point_ids,
    std::span<const spatial::PointId> added_point_ids) {
  CappedDistinctPointCoverage staged = *this;
  staged.insert_facets(added_facet_point_ids);
  staged.insert_facet(added_point_ids);
  *this = std::move(staged);
}

void CappedDistinctPointCoverage::merge_from(
    const CappedDistinctPointCoverage& other) {
  if (threshold_ != other.threshold_) {
    throw std::invalid_argument(
        "capped distinct-point coverage thresholds do not match");
  }

  const std::size_t available =
      smallest_distinct_point_ids_.size() >
              std::numeric_limits<std::size_t>::max() -
                  other.smallest_distinct_point_ids_.size()
          ? std::numeric_limits<std::size_t>::max()
          : smallest_distinct_point_ids_.size() +
                other.smallest_distinct_point_ids_.size();
  std::vector<spatial::PointId> merged;
  merged.reserve(std::min(threshold_, available));

  auto first = smallest_distinct_point_ids_.begin();
  auto second = other.smallest_distinct_point_ids_.begin();
  while (merged.size() < threshold_ &&
         (first != smallest_distinct_point_ids_.end() ||
          second != other.smallest_distinct_point_ids_.end())) {
    if (second == other.smallest_distinct_point_ids_.end() ||
        (first != smallest_distinct_point_ids_.end() &&
         *first < *second)) {
      merged.push_back(*first);
      ++first;
    } else if (
        first == smallest_distinct_point_ids_.end() ||
        *second < *first) {
      merged.push_back(*second);
      ++second;
    } else {
      merged.push_back(*first);
      ++first;
      ++second;
    }
  }

  smallest_distinct_point_ids_ = std::move(merged);
}

}  // namespace morsehgp3d::hierarchy
