#include "morsehgp3d/spatial/brute_force.hpp"

#include "morsehgp3d/exact/rational.hpp"

#include <algorithm>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::spatial {
namespace {

[[nodiscard]] exact::ExactRational3 validated_query(
    const exact::ExactRational3& query) {
  exact::ExactRational3 validated{
      query.numerator(0U),
      query.numerator(1U),
      query.numerator(2U),
      query.denominator()};
  if (validated != query) {
    throw std::invalid_argument("an exact spatial query must be canonical");
  }
  return validated;
}

[[nodiscard]] exact::ExactLevel validated_squared_radius(
    const exact::ExactLevel& squared_radius) {
  exact::ExactLevel validated{
      squared_radius.numerator(), squared_radius.denominator()};
  if (validated != squared_radius) {
    throw std::invalid_argument("an exact squared radius must be canonical");
  }
  return validated;
}

[[nodiscard]] exact::ExactLevel exact_squared_distance(
    const exact::ExactRational3& query,
    const exact::CertifiedPoint3& point) {
  const exact::ExactRational3& exact_point = point.exact();
  const exact::BigInt common_denominator =
      query.denominator() * exact_point.denominator();
  exact::BigInt squared_numerator = 0;
  for (std::size_t axis = 0; axis < 3U; ++axis) {
    const exact::BigInt difference_numerator =
        exact_point.numerator(axis) * query.denominator() -
        query.numerator(axis) * exact_point.denominator();
    squared_numerator += difference_numerator * difference_numerator;
  }
  return exact::ExactLevel{
      std::move(squared_numerator), common_denominator * common_denominator};
}

[[nodiscard]] bool exact_neighbor_less(
    const ExactNeighbor& left,
    const ExactNeighbor& right) {
  if (left.squared_distance != right.squared_distance) {
    return left.squared_distance < right.squared_distance;
  }
  return left.point_id < right.point_id;
}

[[nodiscard]] bool strictly_increasing_ids(std::span<const PointId> ids) {
  return std::adjacent_find(
             ids.begin(), ids.end(),
             [](PointId left, PointId right) { return left >= right; }) == ids.end();
}

void require_unique_partition_ids(
    std::span<const ExactNeighbor> strict_below,
    std::span<const PointId> shell_ids) {
  std::vector<PointId> partition_ids;
  partition_ids.reserve(strict_below.size() + shell_ids.size());
  for (const ExactNeighbor& neighbor : strict_below) {
    partition_ids.push_back(neighbor.point_id);
  }
  partition_ids.insert(partition_ids.end(), shell_ids.begin(), shell_ids.end());
  std::sort(partition_ids.begin(), partition_ids.end());
  if (std::adjacent_find(partition_ids.begin(), partition_ids.end()) !=
      partition_ids.end()) {
    throw std::logic_error("a top-k partition cannot repeat a PointId");
  }
}

void require_unique_partition_ids(
    std::span<const PointId> interior_ids,
    std::span<const PointId> shell_ids,
    std::span<const PointId> exterior_ids) {
  std::vector<PointId> partition_ids;
  partition_ids.reserve(
      interior_ids.size() + shell_ids.size() + exterior_ids.size());
  partition_ids.insert(partition_ids.end(), interior_ids.begin(), interior_ids.end());
  partition_ids.insert(partition_ids.end(), shell_ids.begin(), shell_ids.end());
  partition_ids.insert(partition_ids.end(), exterior_ids.begin(), exterior_ids.end());
  std::sort(partition_ids.begin(), partition_ids.end());
  if (std::adjacent_find(partition_ids.begin(), partition_ids.end()) !=
      partition_ids.end()) {
    throw std::logic_error("a closed-ball partition cannot repeat a PointId");
  }
}

}  // namespace

ExclusionSet ExclusionSet::from_ids(
    std::span<const PointId> ids,
    const CanonicalPointCloud& cloud,
    std::size_t run_m_star) {
  if (run_m_star > max_size) {
    throw std::invalid_argument("run_m_star must be between zero and nine");
  }
  const std::size_t point_count = cloud.size();
  if (point_count == 0U) {
    throw std::invalid_argument("an exclusion set requires a nonempty point cloud");
  }
  if (!std::in_range<PointId>(point_count) ||
      static_cast<PointId>(point_count) > CanonicalPointCloud::max_point_count) {
    throw std::length_error("the exclusion point count exceeds the PointId domain");
  }
  const std::size_t point_count_bound =
      point_count > 1U ? std::min(max_size, point_count - 2U) : 0U;
  if (run_m_star > point_count_bound) {
    throw std::invalid_argument(
        "run_m_star exceeds the refinement depth possible for this point count");
  }
  if (ids.size() > run_m_star) {
    throw std::invalid_argument("the exclusion set exceeds run_m_star");
  }

  std::vector<PointId> canonical_ids{ids.begin(), ids.end()};
  std::sort(canonical_ids.begin(), canonical_ids.end());
  if (std::adjacent_find(canonical_ids.begin(), canonical_ids.end()) !=
      canonical_ids.end()) {
    throw std::invalid_argument("an exclusion set cannot repeat a PointId");
  }
  for (const PointId id : canonical_ids) {
    if (!std::in_range<std::size_t>(id) ||
        static_cast<std::size_t>(id) >= point_count) {
      throw std::out_of_range("an excluded PointId is outside the point cloud");
    }
  }
  return ExclusionSet{
      cloud.identity_, point_count, run_m_star, std::move(canonical_ids)};
}

ExclusionSet::ExclusionSet(
    std::shared_ptr<const CanonicalPointCloud::IdentityToken> cloud_identity,
    std::size_t point_count,
    std::size_t run_m_star,
    std::vector<PointId> ids)
    : cloud_identity_(std::move(cloud_identity)),
      point_count_(point_count),
      run_m_star_(run_m_star),
      ids_(std::move(ids)) {
  const std::size_t point_count_bound =
      point_count_ > 1U ? std::min(max_size, point_count_ - 2U) : 0U;
  if (cloud_identity_ == nullptr || point_count_ == 0U ||
      run_m_star_ > point_count_bound ||
      ids_.size() > run_m_star_ ||
      !strictly_increasing_ids(ids_)) {
    throw std::logic_error("invalid canonical exclusion-set storage");
  }
}

ExclusionSet& ExclusionSet::operator=(const ExclusionSet& other) {
  if (this != &other) {
    ExclusionSet copy{other};
    *this = std::move(copy);
  }
  return *this;
}

ExclusionSet::ExclusionSet(ExclusionSet&& other) noexcept
    : cloud_identity_(std::move(other.cloud_identity_)),
      point_count_(std::exchange(other.point_count_, 0U)),
      run_m_star_(std::exchange(other.run_m_star_, 0U)),
      ids_(std::move(other.ids_)) {}

ExclusionSet& ExclusionSet::operator=(ExclusionSet&& other) noexcept {
  if (this != &other) {
    cloud_identity_ = std::move(other.cloud_identity_);
    point_count_ = std::exchange(other.point_count_, 0U);
    run_m_star_ = std::exchange(other.run_m_star_, 0U);
    ids_ = std::move(other.ids_);
  }
  return *this;
}

TopKPartition::TopKPartition(
    std::size_t requested_rank,
    exact::ExactLevel cutoff_squared_distance,
    std::vector<ExactNeighbor> strict_below,
    std::vector<PointId> cutoff_shell_ids,
    std::vector<PointId> canonical_choice_ids,
    std::size_t eligible_point_count,
    std::size_t distance_evaluation_count)
    : complete_(true),
      requested_rank_(requested_rank),
      cutoff_squared_distance_(std::move(cutoff_squared_distance)),
      strict_below_(std::move(strict_below)),
      cutoff_shell_ids_(std::move(cutoff_shell_ids)),
      canonical_choice_ids_(std::move(canonical_choice_ids)),
      eligible_point_count_(eligible_point_count),
      distance_evaluation_count_(distance_evaluation_count) {
  if (requested_rank_ == 0U || requested_rank_ > eligible_point_count_) {
    throw std::logic_error("a top-k rank must address an eligible point");
  }
  if (distance_evaluation_count_ != eligible_point_count_) {
    throw std::logic_error("the brute-force top-k oracle must evaluate every eligible point");
  }
  if (!std::is_sorted(
          strict_below_.begin(), strict_below_.end(), exact_neighbor_less)) {
    throw std::logic_error(
        "strict top-k neighbors must be ordered by exact distance then PointId");
  }
  for (const ExactNeighbor& neighbor : strict_below_) {
    if (!(neighbor.squared_distance < cutoff_squared_distance_)) {
      throw std::logic_error("a strict top-k neighbor must be below the cutoff");
    }
  }
  if (!strictly_increasing_ids(cutoff_shell_ids_) || cutoff_shell_ids_.empty()) {
    throw std::logic_error("the complete cutoff shell must be nonempty and ID-sorted");
  }
  if (strict_below_.size() >= requested_rank_ ||
      cutoff_shell_ids_.size() < requested_rank_ - strict_below_.size()) {
    throw std::logic_error("the strict/shell split does not straddle the requested rank");
  }
  if (strict_below_.size() > eligible_point_count_ ||
      cutoff_shell_ids_.size() > eligible_point_count_ - strict_below_.size()) {
    throw std::logic_error("a top-k partition exceeds the eligible point count");
  }
  require_unique_partition_ids(strict_below_, cutoff_shell_ids_);

  if (canonical_choice_ids_.size() != requested_rank_ ||
      !strictly_increasing_ids(canonical_choice_ids_)) {
    throw std::logic_error("the canonical top-k choice must contain k sorted unique IDs");
  }
  std::vector<PointId> expected_choice;
  expected_choice.reserve(requested_rank_);
  for (const ExactNeighbor& neighbor : strict_below_) {
    expected_choice.push_back(neighbor.point_id);
  }
  const std::size_t shell_choice_count = requested_rank_ - strict_below_.size();
  for (std::size_t index = 0; index < shell_choice_count; ++index) {
    expected_choice.push_back(cutoff_shell_ids_[index]);
  }
  std::sort(expected_choice.begin(), expected_choice.end());
  if (canonical_choice_ids_ != expected_choice) {
    throw std::logic_error(
        "the canonical top-k choice must use the smallest required shell IDs");
  }
}

TopKPartition& TopKPartition::operator=(const TopKPartition& other) {
  if (this != &other) {
    TopKPartition copy{other};
    *this = std::move(copy);
  }
  return *this;
}

TopKPartition::TopKPartition(TopKPartition&& other) noexcept
    : complete_(std::exchange(other.complete_, false)),
      requested_rank_(other.requested_rank_),
      cutoff_squared_distance_(std::move(other.cutoff_squared_distance_)),
      strict_below_(std::move(other.strict_below_)),
      cutoff_shell_ids_(std::move(other.cutoff_shell_ids_)),
      canonical_choice_ids_(std::move(other.canonical_choice_ids_)),
      eligible_point_count_(other.eligible_point_count_),
      distance_evaluation_count_(other.distance_evaluation_count_) {
  other.requested_rank_ = 0U;
  other.eligible_point_count_ = 0U;
  other.distance_evaluation_count_ = 0U;
}

TopKPartition& TopKPartition::operator=(TopKPartition&& other) noexcept {
  if (this != &other) {
    const bool incoming_complete = std::exchange(other.complete_, false);
    complete_ = false;
    requested_rank_ = other.requested_rank_;
    cutoff_squared_distance_ = std::move(other.cutoff_squared_distance_);
    strict_below_ = std::move(other.strict_below_);
    cutoff_shell_ids_ = std::move(other.cutoff_shell_ids_);
    canonical_choice_ids_ = std::move(other.canonical_choice_ids_);
    eligible_point_count_ = other.eligible_point_count_;
    distance_evaluation_count_ = other.distance_evaluation_count_;
    complete_ = incoming_complete;
    other.requested_rank_ = 0U;
    other.eligible_point_count_ = 0U;
    other.distance_evaluation_count_ = 0U;
  }
  return *this;
}

ClosedBallPartition::ClosedBallPartition(
    exact::ExactLevel squared_radius,
    std::vector<PointId> interior_ids,
    std::vector<PointId> shell_ids,
    std::vector<PointId> exterior_ids,
    std::size_t evaluation_count)
    : complete_(true),
      squared_radius_(std::move(squared_radius)),
      interior_ids_(std::move(interior_ids)),
      shell_ids_(std::move(shell_ids)),
      exterior_ids_(std::move(exterior_ids)),
      closed_rank_(interior_ids_.size() + shell_ids_.size()),
      evaluation_count_(evaluation_count) {
  if (!strictly_increasing_ids(interior_ids_) ||
      !strictly_increasing_ids(shell_ids_) ||
      !strictly_increasing_ids(exterior_ids_)) {
    throw std::logic_error("closed-ball partition classes must be ID-sorted and unique");
  }
  if (interior_ids_.size() > evaluation_count_ ||
      shell_ids_.size() > evaluation_count_ - interior_ids_.size() ||
      exterior_ids_.size() !=
          evaluation_count_ - interior_ids_.size() - shell_ids_.size()) {
    throw std::logic_error("the closed-ball classes must partition every evaluated point");
  }
  require_unique_partition_ids(interior_ids_, shell_ids_, exterior_ids_);
}

ClosedBallPartition& ClosedBallPartition::operator=(
    const ClosedBallPartition& other) {
  if (this != &other) {
    ClosedBallPartition copy{other};
    *this = std::move(copy);
  }
  return *this;
}

ClosedBallPartition::ClosedBallPartition(ClosedBallPartition&& other) noexcept
    : complete_(std::exchange(other.complete_, false)),
      squared_radius_(std::move(other.squared_radius_)),
      interior_ids_(std::move(other.interior_ids_)),
      shell_ids_(std::move(other.shell_ids_)),
      exterior_ids_(std::move(other.exterior_ids_)),
      closed_rank_(other.closed_rank_),
      evaluation_count_(other.evaluation_count_) {
  other.closed_rank_ = 0U;
  other.evaluation_count_ = 0U;
}

ClosedBallPartition& ClosedBallPartition::operator=(
    ClosedBallPartition&& other) noexcept {
  if (this != &other) {
    const bool incoming_complete = std::exchange(other.complete_, false);
    complete_ = false;
    squared_radius_ = std::move(other.squared_radius_);
    interior_ids_ = std::move(other.interior_ids_);
    shell_ids_ = std::move(other.shell_ids_);
    exterior_ids_ = std::move(other.exterior_ids_);
    closed_rank_ = other.closed_rank_;
    evaluation_count_ = other.evaluation_count_;
    complete_ = incoming_complete;
    other.closed_rank_ = 0U;
    other.evaluation_count_ = 0U;
  }
  return *this;
}

TopKPartition brute_force_top_k(
    const CanonicalPointCloud& cloud,
    const exact::ExactRational3& query,
    std::size_t requested_rank,
    const ExclusionSet& exclusions) {
  if (cloud.size() == 0U) {
    throw std::invalid_argument("a moved-from canonical point cloud is not queryable");
  }
  if (!exclusions.validated_for(cloud)) {
    throw std::invalid_argument(
        "the exclusion set belongs to a different canonical point namespace");
  }
  const std::size_t eligible_point_count = cloud.size() - exclusions.ids().size();
  if (requested_rank == 0U || requested_rank > eligible_point_count) {
    throw std::out_of_range("the requested rank is outside the eligible point set");
  }
  const exact::ExactRational3 canonical_query = validated_query(query);

  std::vector<ExactNeighbor> neighbors;
  neighbors.reserve(eligible_point_count);
  for (std::size_t index = 0; index < cloud.size(); ++index) {
    const PointId id = static_cast<PointId>(index);
    if (!exclusions.contains(id)) {
      neighbors.push_back(
          ExactNeighbor{id, exact_squared_distance(canonical_query, cloud.point(id))});
    }
  }
  using NeighborDifference = std::vector<ExactNeighbor>::difference_type;
  if (!std::in_range<NeighborDifference>(requested_rank - 1U)) {
    throw std::length_error("the requested rank exceeds the iterator domain");
  }
  const auto cutoff_position =
      neighbors.begin() + static_cast<NeighborDifference>(requested_rank - 1U);
  std::nth_element(
      neighbors.begin(), cutoff_position, neighbors.end(), exact_neighbor_less);
  const exact::ExactLevel cutoff = cutoff_position->squared_distance;
  std::vector<ExactNeighbor> strict_below;
  std::vector<PointId> cutoff_shell_ids;
  strict_below.reserve(requested_rank - 1U);
  for (ExactNeighbor& neighbor : neighbors) {
    if (neighbor.squared_distance < cutoff) {
      strict_below.push_back(std::move(neighbor));
    } else if (neighbor.squared_distance == cutoff) {
      cutoff_shell_ids.push_back(neighbor.point_id);
    }
  }
  std::sort(strict_below.begin(), strict_below.end(), exact_neighbor_less);
  std::sort(cutoff_shell_ids.begin(), cutoff_shell_ids.end());

  std::vector<PointId> canonical_choice_ids;
  canonical_choice_ids.reserve(requested_rank);
  for (const ExactNeighbor& neighbor : strict_below) {
    canonical_choice_ids.push_back(neighbor.point_id);
  }
  const std::size_t shell_choice_count = requested_rank - strict_below.size();
  for (std::size_t index = 0; index < shell_choice_count; ++index) {
    canonical_choice_ids.push_back(cutoff_shell_ids[index]);
  }
  std::sort(canonical_choice_ids.begin(), canonical_choice_ids.end());

  return TopKPartition{
      requested_rank,
      cutoff,
      std::move(strict_below),
      std::move(cutoff_shell_ids),
      std::move(canonical_choice_ids),
      eligible_point_count,
      neighbors.size()};
}

TopKPartition brute_force_nearest(
    const CanonicalPointCloud& cloud,
    const exact::ExactRational3& query,
    const ExclusionSet& exclusions) {
  return brute_force_top_k(cloud, query, 1U, exclusions);
}

ClosedBallPartition brute_force_closed_ball(
    const CanonicalPointCloud& cloud,
    const exact::ExactRational3& query,
    const exact::ExactLevel& squared_radius) {
  if (cloud.size() == 0U) {
    throw std::invalid_argument("a moved-from canonical point cloud is not queryable");
  }
  const exact::ExactRational3 canonical_query = validated_query(query);
  const exact::ExactLevel canonical_squared_radius =
      validated_squared_radius(squared_radius);
  std::vector<PointId> interior_ids;
  std::vector<PointId> shell_ids;
  std::vector<PointId> exterior_ids;
  for (std::size_t index = 0; index < cloud.size(); ++index) {
    const PointId id = static_cast<PointId>(index);
    const exact::ExactLevel distance =
        exact_squared_distance(canonical_query, cloud.point(id));
    if (distance < canonical_squared_radius) {
      interior_ids.push_back(id);
    } else if (distance == canonical_squared_radius) {
      shell_ids.push_back(id);
    } else {
      exterior_ids.push_back(id);
    }
  }
  return ClosedBallPartition{
      canonical_squared_radius,
      std::move(interior_ids),
      std::move(shell_ids),
      std::move(exterior_ids),
      cloud.size()};
}

}  // namespace morsehgp3d::spatial
