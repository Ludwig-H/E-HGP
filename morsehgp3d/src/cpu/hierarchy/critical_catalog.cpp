#include "morsehgp3d/hierarchy/critical_catalog.hpp"

#include "morsehgp3d/spatial/brute_force.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    std::string_view message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(std::string{message});
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_multiply(
    std::size_t left,
    std::size_t right,
    std::string_view message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::overflow_error(std::string{message});
  }
  return left * right;
}

[[nodiscard]] std::size_t bounded_binomial(
    std::size_t point_count,
    std::size_t subset_size) {
  if (subset_size > point_count) {
    return 0U;
  }
  subset_size = std::min(subset_size, point_count - subset_size);
  std::size_t value = 1U;
  for (std::size_t factor = 1U; factor <= subset_size; ++factor) {
    value = checked_multiply(
        value,
        point_count - subset_size + factor,
        "the critical-catalog binomial coefficient overflows size_t");
    value /= factor;
  }
  return value;
}

[[nodiscard]] std::array<std::size_t, 4> candidate_counts_by_size(
    std::size_t point_count) {
  std::array<std::size_t, 4> counts{};
  for (std::size_t support_size = 1U;
       support_size <= counts.size();
       ++support_size) {
    counts[support_size - 1U] =
        bounded_binomial(point_count, support_size);
  }
  return counts;
}

[[nodiscard]] std::size_t candidate_count_sum(
    const std::array<std::size_t, 4>& counts) {
  std::size_t sum = 0U;
  for (const std::size_t count : counts) {
    sum = checked_add(
        sum,
        count,
        "the critical-catalog candidate count overflows size_t");
  }
  return sum;
}

void validate_domain(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactCriticalCatalogBudget& budget) {
  if (cloud.size() <
          ExactCriticalCatalogResult::minimum_supported_point_count ||
      cloud.size() >
          ExactCriticalCatalogResult::maximum_supported_point_count) {
    throw std::invalid_argument(
        "the exact critical catalogue requires 1<=n<=14");
  }
  if (requested_maximum_order <
          ExactCriticalCatalogResult::minimum_supported_maximum_order ||
      requested_maximum_order >
          ExactCriticalCatalogResult::maximum_supported_maximum_order) {
    throw std::invalid_argument(
        "the exact critical catalogue requires 1<=Kmax<=10");
  }
  if (budget.maximum_candidate_count >
          ExactCriticalCatalogBudget::maximum_supported_candidate_count ||
      budget.maximum_point_classification_count >
          ExactCriticalCatalogBudget::
              maximum_supported_point_classification_count) {
    throw std::invalid_argument(
        "the exact critical-catalog budget exceeds its bounded cap");
  }
}

[[nodiscard]] std::vector<PointId> closed_point_ids(
    std::span<const PointId> interior,
    std::span<const PointId> shell) {
  std::vector<PointId> closed;
  closed.reserve(interior.size() + shell.size());
  std::set_union(
      interior.begin(),
      interior.end(),
      shell.begin(),
      shell.end(),
      std::back_inserter(closed));
  if (closed.size() != interior.size() + shell.size()) {
    throw std::logic_error(
        "an exact closed-ball partition overlaps its interior and shell");
  }
  return closed;
}

[[nodiscard]] bool is_subset(
    std::span<const PointId> subset,
    std::span<const PointId> superset) {
  return std::includes(
      superset.begin(), superset.end(), subset.begin(), subset.end());
}

void copy_barycentric_witness(
    const exact::CircumcenterSupportAnalysis& analysis,
    ExactCriticalCatalogCandidate& candidate) {
  if (!analysis.barycentric().has_value()) {
    throw std::logic_error(
        "an affinely independent support omitted its barycentric witness");
  }
  const exact::BarycentricCoordinates& barycentric =
      *analysis.barycentric();
  if (barycentric.support_size() != candidate.support_point_ids.size()) {
    throw std::logic_error(
        "a critical-catalog barycentric witness has the wrong size");
  }
  candidate.support_barycentric_coordinates.reserve(
      barycentric.support_size());
  candidate.support_barycentric_signs.reserve(
      barycentric.support_size());
  for (std::size_t index = 0U;
       index < barycentric.support_size();
       ++index) {
    candidate.support_barycentric_coordinates.push_back(
        barycentric.coordinate(index));
    candidate.support_barycentric_signs.push_back(
        barycentric.sign(index));
  }
}

void copy_sphere_witness(
    const exact::CircumcenterSupportAnalysis& analysis,
    ExactCriticalCatalogCandidate& candidate) {
  const exact::CircumcenterResult& sphere =
      analysis.circumcenter_result();
  if (sphere.kind() != exact::CircumcenterKind::unique ||
      !sphere.center().has_value() ||
      !sphere.squared_level().has_value()) {
    throw std::logic_error(
        "an affinely independent support omitted its exact sphere");
  }
  candidate.center = *sphere.center();
  candidate.squared_level = *sphere.squared_level();
  copy_barycentric_witness(analysis, candidate);
}

[[nodiscard]] bool same_degeneracy_identity(
    const ExactCriticalExtraShellDegeneracy& degeneracy,
    const ExactCriticalCatalogCandidate& candidate) {
  return candidate.center.has_value() &&
         candidate.squared_level.has_value() &&
         degeneracy.center == *candidate.center &&
         degeneracy.squared_level == *candidate.squared_level &&
         degeneracy.interior_point_ids == candidate.interior_point_ids &&
         degeneracy.shell_point_ids == candidate.shell_point_ids &&
         degeneracy.closed_point_ids == candidate.closed_point_ids &&
         degeneracy.observed_closed_rank == candidate.observed_closed_rank;
}

[[nodiscard]] std::size_t append_extra_shell_degeneracy(
    ExactCriticalCatalogResult& result,
    const ExactCriticalCatalogCandidate& candidate,
    bool relevant) {
  auto position = std::find_if(
      result.extra_shell_degeneracies.begin(),
      result.extra_shell_degeneracies.end(),
      [&](const ExactCriticalExtraShellDegeneracy& degeneracy) {
        return same_degeneracy_identity(degeneracy, candidate);
      });
  if (position == result.extra_shell_degeneracies.end()) {
    if (!candidate.center.has_value() ||
        !candidate.squared_level.has_value()) {
      throw std::logic_error(
          "an extra-shell candidate omitted its sphere witness");
    }
    ExactCriticalExtraShellDegeneracy degeneracy;
    degeneracy.degeneracy_index =
        result.extra_shell_degeneracies.size();
    degeneracy.center = *candidate.center;
    degeneracy.squared_level = *candidate.squared_level;
    degeneracy.interior_point_ids = candidate.interior_point_ids;
    degeneracy.shell_point_ids = candidate.shell_point_ids;
    degeneracy.closed_point_ids = candidate.closed_point_ids;
    degeneracy.observed_closed_rank = candidate.observed_closed_rank;
    result.extra_shell_degeneracies.push_back(std::move(degeneracy));
    position = std::prev(result.extra_shell_degeneracies.end());
  }

  position->support_point_id_sets.push_back(candidate.support_point_ids);
  position->support_candidate_indices.push_back(candidate.candidate_index);
  if (relevant) {
    position->relevant_support_candidate_indices.push_back(
        candidate.candidate_index);
    position->has_relevant_support = true;
  }
  return position->degeneracy_index;
}

void append_event(
    ExactCriticalCatalogResult& result,
    ExactCriticalCatalogCandidate& candidate) {
  if (!candidate.center.has_value() ||
      !candidate.squared_level.has_value() ||
      candidate.support_point_ids != candidate.shell_point_ids ||
      candidate.observed_closed_rank == 0U ||
      candidate.observed_closed_rank >
          result.maximum_relevant_closed_rank) {
    throw std::logic_error(
        "an accepted critical event has inconsistent source witnesses");
  }
  ExactCriticalEvent event;
  event.event_index = result.events.size();
  event.source_candidate_index = candidate.candidate_index;
  event.support_point_ids = candidate.support_point_ids;
  event.interior_point_ids = candidate.interior_point_ids;
  event.shell_point_ids = candidate.shell_point_ids;
  event.closed_point_ids = candidate.closed_point_ids;
  event.center = *candidate.center;
  event.squared_level = *candidate.squared_level;
  event.support_barycentric_coordinates =
      candidate.support_barycentric_coordinates;
  event.support_barycentric_signs = candidate.support_barycentric_signs;
  event.closed_rank = candidate.observed_closed_rank;
  if (event.closed_rank <= result.effective_maximum_order) {
    event.birth_order = event.closed_rank;
  }
  if (event.closed_rank >= 2U &&
      event.closed_rank - 1U <= result.effective_maximum_order) {
    event.saddle_order = event.closed_rank - 1U;
  }
  if (!event.birth_order.has_value() &&
      !event.saddle_order.has_value()) {
    throw std::logic_error(
        "a rank-window critical event has no H0 order");
  }
  candidate.event_index = event.event_index;
  result.events.push_back(std::move(event));
}

[[nodiscard]] bool center_less(
    const exact::ExactCenter3& left,
    const exact::ExactCenter3& right) {
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    if (left.coordinate(axis) != right.coordinate(axis)) {
      return left.coordinate(axis) < right.coordinate(axis);
    }
  }
  return false;
}

[[nodiscard]] bool event_key_less(
    const ExactCriticalEvent& left,
    const ExactCriticalEvent& right) {
  if (left.squared_level != right.squared_level) {
    return left.squared_level < right.squared_level;
  }
  if (left.closed_rank != right.closed_rank) {
    return left.closed_rank < right.closed_rank;
  }
  if (left.interior_point_ids != right.interior_point_ids) {
    return left.interior_point_ids < right.interior_point_ids;
  }
  if (left.shell_point_ids != right.shell_point_ids) {
    return left.shell_point_ids < right.shell_point_ids;
  }
  if (left.support_point_ids != right.support_point_ids) {
    return left.support_point_ids < right.support_point_ids;
  }
  return center_less(left.center, right.center);
}

void canonicalize_events(ExactCriticalCatalogResult& result) {
  std::sort(result.events.begin(), result.events.end(), event_key_less);
  for (std::size_t index = 1U; index < result.events.size(); ++index) {
    if (!event_key_less(result.events[index - 1U], result.events[index])) {
      throw std::logic_error(
          "the critical catalogue produced duplicate canonical events");
    }
  }
  for (ExactCriticalCatalogCandidate& candidate : result.candidates) {
    if (candidate.outcome ==
        ExactCriticalCatalogCandidateOutcome::accepted_critical_event) {
      candidate.event_index.reset();
    }
  }
  for (std::size_t index = 0U; index < result.events.size(); ++index) {
    ExactCriticalEvent& event = result.events[index];
    if (event.source_candidate_index >= result.candidates.size()) {
      throw std::logic_error(
          "a canonical event points outside the support catalogue");
    }
    ExactCriticalCatalogCandidate& candidate =
        result.candidates[event.source_candidate_index];
    if (candidate.event_index.has_value()) {
      throw std::logic_error(
          "two canonical events point to one support candidate");
    }
    event.event_index = index;
    candidate.event_index = index;
  }
}

[[nodiscard]] bool degeneracy_key_less(
    const ExactCriticalExtraShellDegeneracy& left,
    const ExactCriticalExtraShellDegeneracy& right) {
  if (left.squared_level != right.squared_level) {
    return left.squared_level < right.squared_level;
  }
  if (left.interior_point_ids != right.interior_point_ids) {
    return left.interior_point_ids < right.interior_point_ids;
  }
  if (left.shell_point_ids != right.shell_point_ids) {
    return left.shell_point_ids < right.shell_point_ids;
  }
  if (left.support_point_id_sets != right.support_point_id_sets) {
    return left.support_point_id_sets < right.support_point_id_sets;
  }
  return center_less(left.center, right.center);
}

void canonicalize_degeneracies(ExactCriticalCatalogResult& result) {
  for (ExactCriticalExtraShellDegeneracy& degeneracy :
       result.extra_shell_degeneracies) {
    std::vector<std::size_t> positions(
        degeneracy.support_candidate_indices.size());
    for (std::size_t index = 0U; index < positions.size(); ++index) {
      positions[index] = index;
    }
    std::sort(
        positions.begin(),
        positions.end(),
        [&](std::size_t left, std::size_t right) {
          return degeneracy.support_candidate_indices[left] <
                 degeneracy.support_candidate_indices[right];
        });
    std::vector<std::vector<PointId>> support_sets;
    std::vector<std::size_t> candidate_indices;
    support_sets.reserve(positions.size());
    candidate_indices.reserve(positions.size());
    for (const std::size_t position : positions) {
      support_sets.push_back(
          degeneracy.support_point_id_sets[position]);
      candidate_indices.push_back(
          degeneracy.support_candidate_indices[position]);
    }
    degeneracy.support_point_id_sets = std::move(support_sets);
    degeneracy.support_candidate_indices = std::move(candidate_indices);
    std::sort(
        degeneracy.relevant_support_candidate_indices.begin(),
        degeneracy.relevant_support_candidate_indices.end());
  }
  std::sort(
      result.extra_shell_degeneracies.begin(),
      result.extra_shell_degeneracies.end(),
      degeneracy_key_less);
  for (std::size_t index = 1U;
       index < result.extra_shell_degeneracies.size();
       ++index) {
    if (!degeneracy_key_less(
            result.extra_shell_degeneracies[index - 1U],
            result.extra_shell_degeneracies[index])) {
      throw std::logic_error(
          "the critical catalogue produced duplicate degeneracy keys");
    }
  }
  for (std::size_t index = 0U;
       index < result.extra_shell_degeneracies.size();
       ++index) {
    ExactCriticalExtraShellDegeneracy& degeneracy =
        result.extra_shell_degeneracies[index];
    degeneracy.degeneracy_index = index;
    for (const std::size_t candidate_index :
         degeneracy.support_candidate_indices) {
      if (candidate_index >= result.candidates.size()) {
        throw std::logic_error(
            "a canonical degeneracy points outside the support catalogue");
      }
      result.candidates[candidate_index].extra_shell_degeneracy_index =
          index;
    }
  }
}

template <std::size_t SupportSize>
void classify_support(
    const spatial::CanonicalPointCloud& cloud,
    const std::array<PointId, SupportSize>& support_point_ids,
    exact::PredicateCounters& predicate_counters,
    ExactCriticalCatalogResult& result) {
  static_assert(SupportSize >= 1U && SupportSize <= 4U);
  ExactCriticalCatalogCandidate candidate;
  candidate.candidate_index = result.candidates.size();
  candidate.support_point_ids.assign(
      support_point_ids.begin(), support_point_ids.end());

  std::array<exact::ExactRational3, SupportSize> support_points{};
  for (std::size_t index = 0U; index < SupportSize; ++index) {
    support_points[index] = cloud.point(support_point_ids[index]).exact();
  }
  const exact::CircumcenterSupportAnalysis analysis =
      exact::analyze_circumcenter_support(
          support_points, &predicate_counters);
  candidate.support_status = analysis.status();
  ++result.counters.support_analysis_count;
  ++result.counters.enumerated_candidate_count_by_support_size[
      SupportSize - 1U];
  ++result.counters.enumerated_candidate_count;

  switch (analysis.status()) {
    case exact::CircumcenterSupportStatus::affinely_dependent:
      candidate.outcome = ExactCriticalCatalogCandidateOutcome::
          affinely_dependent_support;
      ++result.counters.affinely_dependent_support_count;
      result.candidates.push_back(std::move(candidate));
      return;
    case exact::CircumcenterSupportStatus::boundary_reduced:
      copy_sphere_witness(analysis, candidate);
      candidate.outcome = ExactCriticalCatalogCandidateOutcome::
          boundary_reduced_support;
      ++result.counters.boundary_reduced_support_count;
      result.candidates.push_back(std::move(candidate));
      return;
    case exact::CircumcenterSupportStatus::exterior_circumcenter:
      copy_sphere_witness(analysis, candidate);
      candidate.outcome = ExactCriticalCatalogCandidateOutcome::
          exterior_circumcenter_support;
      ++result.counters.exterior_circumcenter_support_count;
      result.candidates.push_back(std::move(candidate));
      return;
    case exact::CircumcenterSupportStatus::minimal:
      copy_sphere_witness(analysis, candidate);
      ++result.counters.minimal_support_count;
      break;
  }

  for (const exact::PredicateSign sign :
       candidate.support_barycentric_signs) {
    if (sign != exact::PredicateSign::positive) {
      throw std::logic_error(
          "a minimal critical-catalog support has a nonpositive barycentric coordinate");
    }
  }
  const spatial::ClosedBallPartition partition =
      spatial::brute_force_closed_ball(
          cloud, *candidate.center, *candidate.squared_level);
  ++result.counters.global_closed_ball_query_count;
  result.counters.global_point_classification_count = checked_add(
      result.counters.global_point_classification_count,
      partition.distance_evaluation_count(),
      "the critical-catalog point-classification count overflows size_t");
  if (!partition.partition_complete() ||
      !partition.validated_for(cloud) ||
      partition.squared_radius() != *candidate.squared_level ||
      partition.evaluation_count() != cloud.size() ||
      partition.distance_evaluation_count() != cloud.size() ||
      partition.query_counters().method !=
          spatial::SpatialQueryMethod::brute_force) {
    throw std::logic_error(
        "a critical-catalog global closed-ball query was incomplete");
  }
  candidate.interior_point_ids.assign(
      partition.interior_ids().begin(), partition.interior_ids().end());
  candidate.shell_point_ids.assign(
      partition.shell_ids().begin(), partition.shell_ids().end());
  candidate.exterior_point_ids.assign(
      partition.exterior_ids().begin(), partition.exterior_ids().end());
  candidate.closed_point_ids = closed_point_ids(
      partition.interior_ids(), partition.shell_ids());
  candidate.observed_closed_rank = partition.closed_rank();
  candidate.support_relevance_rank = checked_add(
      candidate.interior_point_ids.size(),
      candidate.support_point_ids.size(),
      "the critical-catalog support relevance rank overflows size_t");
  candidate.global_closed_ball_classified = true;
  if (candidate.observed_closed_rank != candidate.closed_point_ids.size() ||
      candidate.interior_point_ids.size() +
              candidate.shell_point_ids.size() +
              candidate.exterior_point_ids.size() !=
          cloud.size() ||
      !is_subset(
          candidate.support_point_ids, candidate.shell_point_ids)) {
    throw std::logic_error(
        "a critical-catalog global partition contradicts its support");
  }

  if (candidate.shell_point_ids != candidate.support_point_ids) {
    const bool relevant =
        candidate.support_relevance_rank <=
        result.maximum_relevant_closed_rank;
    candidate.outcome = relevant
        ? ExactCriticalCatalogCandidateOutcome::
              relevant_extra_shell_degeneracy
        : ExactCriticalCatalogCandidateOutcome::
              extra_shell_outside_relevant_window;
    candidate.extra_shell_degeneracy_index =
        append_extra_shell_degeneracy(result, candidate, relevant);
    if (relevant) {
      ++result.counters.relevant_extra_shell_candidate_count;
    } else {
      ++result.counters.outside_window_extra_shell_candidate_count;
    }
    result.candidates.push_back(std::move(candidate));
    return;
  }

  if (candidate.observed_closed_rank >
      result.maximum_relevant_closed_rank) {
    candidate.outcome = ExactCriticalCatalogCandidateOutcome::
        minimal_support_above_rank_window;
    ++result.counters.above_rank_candidate_count;
    result.candidates.push_back(std::move(candidate));
    return;
  }

  candidate.outcome =
      ExactCriticalCatalogCandidateOutcome::accepted_critical_event;
  append_event(result, candidate);
  ++result.counters.accepted_event_count;
  result.candidates.push_back(std::move(candidate));
}

template <std::size_t SupportSize>
void enumerate_supports_recursive(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t depth,
    std::size_t start,
    std::array<PointId, SupportSize>& support,
    exact::PredicateCounters& predicate_counters,
    ExactCriticalCatalogResult& result) {
  if (depth == SupportSize) {
    classify_support(
        cloud, support, predicate_counters, result);
    return;
  }
  const std::size_t remaining = SupportSize - depth;
  const std::size_t last = cloud.size() - remaining;
  for (std::size_t point_index = start;
       point_index <= last;
       ++point_index) {
    support[depth] = static_cast<PointId>(point_index);
    enumerate_supports_recursive(
        cloud,
        depth + 1U,
        point_index + 1U,
        support,
        predicate_counters,
        result);
  }
}

template <std::size_t SupportSize>
void enumerate_supports(
    const spatial::CanonicalPointCloud& cloud,
    exact::PredicateCounters& predicate_counters,
    ExactCriticalCatalogResult& result) {
  static_assert(SupportSize >= 1U && SupportSize <= 4U);
  if (SupportSize > cloud.size()) {
    return;
  }
  std::array<PointId, SupportSize> support{};
  enumerate_supports_recursive(
      cloud,
      0U,
      0U,
      support,
      predicate_counters,
      result);
}

[[nodiscard]] bool batch_key_less(
    const ExactCriticalH0Batch& left,
    const ExactCriticalH0Batch& right) {
  if (left.order != right.order) {
    return left.order < right.order;
  }
  return left.squared_level < right.squared_level;
}

ExactCriticalH0Batch& batch_for(
    std::vector<ExactCriticalH0Batch>& batches,
    std::size_t order,
    const exact::ExactLevel& squared_level) {
  const auto position = std::find_if(
      batches.begin(),
      batches.end(),
      [&](const ExactCriticalH0Batch& batch) {
        return batch.order == order &&
               batch.squared_level == squared_level;
      });
  if (position != batches.end()) {
    return *position;
  }
  batches.push_back(ExactCriticalH0Batch{order, squared_level, {}, {}});
  return batches.back();
}

void build_h0_batches(ExactCriticalCatalogResult& result) {
  for (const ExactCriticalEvent& event : result.events) {
    if (event.birth_order.has_value()) {
      batch_for(
          result.h0_batches,
          *event.birth_order,
          event.squared_level)
          .birth_event_indices.push_back(event.event_index);
      ++result.counters.birth_event_reference_count;
    }
    if (event.saddle_order.has_value()) {
      batch_for(
          result.h0_batches,
          *event.saddle_order,
          event.squared_level)
          .saddle_event_indices.push_back(event.event_index);
      ++result.counters.saddle_event_reference_count;
    }
  }
  std::sort(
      result.h0_batches.begin(),
      result.h0_batches.end(),
      batch_key_less);
  result.counters.h0_batch_count = result.h0_batches.size();
}

[[nodiscard]] bool candidates_are_canonical(
    const ExactCriticalCatalogResult& result) {
  if (result.candidates.size() != result.required_candidate_count) {
    return false;
  }
  std::size_t previous_size = 0U;
  std::vector<PointId> previous_support;
  for (std::size_t index = 0U;
       index < result.candidates.size();
       ++index) {
    const ExactCriticalCatalogCandidate& candidate =
        result.candidates[index];
    if (candidate.candidate_index != index ||
        candidate.support_point_ids.empty() ||
        candidate.support_point_ids.size() >
            ExactCriticalCatalogResult::maximum_support_point_count ||
        !std::is_sorted(
            candidate.support_point_ids.begin(),
            candidate.support_point_ids.end()) ||
        std::adjacent_find(
            candidate.support_point_ids.begin(),
            candidate.support_point_ids.end()) !=
            candidate.support_point_ids.end()) {
      return false;
    }
    if (index != 0U &&
        (candidate.support_point_ids.size() < previous_size ||
         (candidate.support_point_ids.size() == previous_size &&
          !(previous_support < candidate.support_point_ids)))) {
      return false;
    }
    previous_size = candidate.support_point_ids.size();
    previous_support = candidate.support_point_ids;

    const bool has_sphere_witness =
        candidate.center.has_value() &&
        candidate.squared_level.has_value() &&
        candidate.support_barycentric_coordinates.size() ==
            candidate.support_point_ids.size() &&
        candidate.support_barycentric_signs.size() ==
            candidate.support_point_ids.size();
    const bool has_no_partition_payload =
        !candidate.global_closed_ball_classified &&
        candidate.interior_point_ids.empty() &&
        candidate.shell_point_ids.empty() &&
        candidate.exterior_point_ids.empty() &&
        candidate.closed_point_ids.empty() &&
        candidate.observed_closed_rank == 0U &&
        candidate.support_relevance_rank == 0U;
    switch (candidate.support_status) {
      case exact::CircumcenterSupportStatus::affinely_dependent:
        if (candidate.outcome != ExactCriticalCatalogCandidateOutcome::
                affinely_dependent_support ||
            candidate.center.has_value() ||
            candidate.squared_level.has_value() ||
            !candidate.support_barycentric_coordinates.empty() ||
            !candidate.support_barycentric_signs.empty() ||
            !has_no_partition_payload) {
          return false;
        }
        break;
      case exact::CircumcenterSupportStatus::boundary_reduced:
        if (candidate.outcome != ExactCriticalCatalogCandidateOutcome::
                boundary_reduced_support ||
            !has_sphere_witness || !has_no_partition_payload) {
          return false;
        }
        break;
      case exact::CircumcenterSupportStatus::exterior_circumcenter:
        if (candidate.outcome != ExactCriticalCatalogCandidateOutcome::
                exterior_circumcenter_support ||
            !has_sphere_witness || !has_no_partition_payload) {
          return false;
        }
        break;
      case exact::CircumcenterSupportStatus::minimal: {
        if (!has_sphere_witness ||
            !candidate.global_closed_ball_classified ||
            candidate.observed_closed_rank !=
                candidate.closed_point_ids.size() ||
            candidate.support_relevance_rank !=
                candidate.interior_point_ids.size() +
                    candidate.support_point_ids.size() ||
            candidate.interior_point_ids.size() +
                    candidate.shell_point_ids.size() +
                    candidate.exterior_point_ids.size() !=
                result.point_count ||
            !is_subset(
                candidate.support_point_ids,
                candidate.shell_point_ids)) {
          return false;
        }
        const bool has_extra_shell =
            candidate.shell_point_ids != candidate.support_point_ids;
        ExactCriticalCatalogCandidateOutcome expected_outcome =
            ExactCriticalCatalogCandidateOutcome::not_classified;
        if (has_extra_shell) {
          expected_outcome = candidate.support_relevance_rank <=
                                     result.maximum_relevant_closed_rank
              ? ExactCriticalCatalogCandidateOutcome::
                    relevant_extra_shell_degeneracy
              : ExactCriticalCatalogCandidateOutcome::
                    extra_shell_outside_relevant_window;
        } else {
          expected_outcome = candidate.observed_closed_rank >
                                     result.maximum_relevant_closed_rank
              ? ExactCriticalCatalogCandidateOutcome::
                    minimal_support_above_rank_window
              : ExactCriticalCatalogCandidateOutcome::
                    accepted_critical_event;
        }
        if (candidate.outcome != expected_outcome) {
          return false;
        }
        break;
      }
    }
    const bool event = candidate.outcome ==
        ExactCriticalCatalogCandidateOutcome::accepted_critical_event;
    const bool degeneracy =
        candidate.outcome == ExactCriticalCatalogCandidateOutcome::
            relevant_extra_shell_degeneracy ||
        candidate.outcome == ExactCriticalCatalogCandidateOutcome::
            extra_shell_outside_relevant_window;
    if (candidate.event_index.has_value() != event ||
        candidate.extra_shell_degeneracy_index.has_value() != degeneracy ||
        (candidate.event_index.has_value() &&
         *candidate.event_index >= result.events.size()) ||
        (candidate.extra_shell_degeneracy_index.has_value() &&
         *candidate.extra_shell_degeneracy_index >=
             result.extra_shell_degeneracies.size())) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool events_are_canonical_and_indexed(
    const ExactCriticalCatalogResult& result) {
  for (std::size_t index = 0U; index < result.events.size(); ++index) {
    const ExactCriticalEvent& event = result.events[index];
    if (event.event_index != index ||
        event.source_candidate_index >= result.candidates.size() ||
        (index != 0U &&
         !event_key_less(result.events[index - 1U], event))) {
      return false;
    }
    const ExactCriticalCatalogCandidate& candidate =
        result.candidates[event.source_candidate_index];
    if (candidate.event_index != std::optional<std::size_t>{index} ||
        candidate.outcome != ExactCriticalCatalogCandidateOutcome::
            accepted_critical_event ||
        event.support_point_ids != candidate.support_point_ids ||
        event.interior_point_ids != candidate.interior_point_ids ||
        event.shell_point_ids != candidate.shell_point_ids ||
        event.closed_point_ids != candidate.closed_point_ids ||
        !candidate.center.has_value() ||
        event.center != *candidate.center ||
        !candidate.squared_level.has_value() ||
        event.squared_level != *candidate.squared_level ||
        event.support_barycentric_coordinates !=
            candidate.support_barycentric_coordinates ||
        event.support_barycentric_signs !=
            candidate.support_barycentric_signs ||
        event.closed_rank != candidate.observed_closed_rank) {
      return false;
    }
    const bool expects_birth =
        event.closed_rank <= result.effective_maximum_order;
    const bool expects_saddle =
        event.closed_rank >= 2U &&
        event.closed_rank - 1U <= result.effective_maximum_order;
    if (event.birth_order.has_value() != expects_birth ||
        (expects_birth && *event.birth_order != event.closed_rank) ||
        event.saddle_order.has_value() != expects_saddle ||
        (expects_saddle &&
         *event.saddle_order != event.closed_rank - 1U)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool degeneracies_are_deduplicated_and_indexed(
    const ExactCriticalCatalogResult& result) {
  std::vector<std::size_t> candidate_reference_count(
      result.candidates.size(), 0U);
  for (std::size_t index = 0U;
       index < result.extra_shell_degeneracies.size();
       ++index) {
    const ExactCriticalExtraShellDegeneracy& degeneracy =
        result.extra_shell_degeneracies[index];
    if (degeneracy.degeneracy_index != index ||
        degeneracy.support_point_id_sets.size() !=
            degeneracy.support_candidate_indices.size() ||
        degeneracy.support_candidate_indices.empty() ||
        !std::is_sorted(
            degeneracy.support_candidate_indices.begin(),
            degeneracy.support_candidate_indices.end()) ||
        std::adjacent_find(
            degeneracy.support_candidate_indices.begin(),
            degeneracy.support_candidate_indices.end()) !=
            degeneracy.support_candidate_indices.end() ||
        !std::is_sorted(
            degeneracy.relevant_support_candidate_indices.begin(),
            degeneracy.relevant_support_candidate_indices.end()) ||
        std::adjacent_find(
            degeneracy.relevant_support_candidate_indices.begin(),
            degeneracy.relevant_support_candidate_indices.end()) !=
            degeneracy.relevant_support_candidate_indices.end() ||
        degeneracy.has_relevant_support !=
            !degeneracy.relevant_support_candidate_indices.empty() ||
        (index != 0U &&
         !degeneracy_key_less(
             result.extra_shell_degeneracies[index - 1U],
             degeneracy))) {
      return false;
    }
    for (std::size_t other = 0U; other < index; ++other) {
      ExactCriticalCatalogCandidate witness;
      witness.center = degeneracy.center;
      witness.squared_level = degeneracy.squared_level;
      witness.interior_point_ids = degeneracy.interior_point_ids;
      witness.shell_point_ids = degeneracy.shell_point_ids;
      witness.closed_point_ids = degeneracy.closed_point_ids;
      witness.observed_closed_rank = degeneracy.observed_closed_rank;
      if (same_degeneracy_identity(
              result.extra_shell_degeneracies[other], witness)) {
        return false;
      }
    }
    for (std::size_t support_index = 0U;
         support_index < degeneracy.support_candidate_indices.size();
         ++support_index) {
      const std::size_t candidate_index =
          degeneracy.support_candidate_indices[support_index];
      if (candidate_index >= result.candidates.size()) {
        return false;
      }
      const ExactCriticalCatalogCandidate& candidate =
          result.candidates[candidate_index];
      if (candidate.extra_shell_degeneracy_index !=
              std::optional<std::size_t>{index} ||
          candidate.support_point_ids !=
              degeneracy.support_point_id_sets[support_index] ||
          !same_degeneracy_identity(degeneracy, candidate)) {
        return false;
      }
      ++candidate_reference_count[candidate_index];
      const bool candidate_relevant = candidate.outcome ==
          ExactCriticalCatalogCandidateOutcome::
              relevant_extra_shell_degeneracy;
      const bool listed_relevant = std::binary_search(
          degeneracy.relevant_support_candidate_indices.begin(),
          degeneracy.relevant_support_candidate_indices.end(),
          candidate_index);
      if (candidate_relevant != listed_relevant) {
        return false;
      }
    }
  }
  for (std::size_t index = 0U;
       index < result.candidates.size();
       ++index) {
    const bool is_degeneracy =
        result.candidates[index].extra_shell_degeneracy_index.has_value();
    if (candidate_reference_count[index] !=
        static_cast<std::size_t>(is_degeneracy)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] std::vector<ExactCriticalH0Batch> expected_h0_batches(
    const ExactCriticalCatalogResult& result) {
  std::vector<ExactCriticalH0Batch> expected;
  for (const ExactCriticalEvent& event : result.events) {
    if (event.birth_order.has_value()) {
      batch_for(expected, *event.birth_order, event.squared_level)
          .birth_event_indices.push_back(event.event_index);
    }
    if (event.saddle_order.has_value()) {
      batch_for(expected, *event.saddle_order, event.squared_level)
          .saddle_event_indices.push_back(event.event_index);
    }
  }
  std::sort(expected.begin(), expected.end(), batch_key_less);
  return expected;
}

[[nodiscard]] bool h0_batches_are_canonical_and_complete(
    const ExactCriticalCatalogResult& result) {
  if (!std::is_sorted(
          result.h0_batches.begin(),
          result.h0_batches.end(),
          batch_key_less) ||
      result.h0_batches != expected_h0_batches(result)) {
    return false;
  }
  for (std::size_t index = 0U;
       index < result.h0_batches.size();
       ++index) {
    const ExactCriticalH0Batch& batch = result.h0_batches[index];
    if (batch.order == 0U ||
        batch.order > result.effective_maximum_order ||
        (batch.birth_event_indices.empty() &&
         batch.saddle_event_indices.empty()) ||
        (index != 0U &&
         !batch_key_less(result.h0_batches[index - 1U], batch))) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] ExactCriticalCatalogPredicateCounters predicate_snapshot(
    const exact::PredicateCounters& counters) {
  return ExactCriticalCatalogPredicateCounters{
      counters.fp64_filtered_certified(),
      counters.expansion_certified(),
      counters.cpu_multiprecision_certified(),
      counters.exact_zeros(),
      counters.remaining_unknown()};
}

[[nodiscard]] ExactCriticalCatalogResult compute_exact_critical_catalog(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    ExactCriticalCatalogBudget budget) {
  validate_domain(cloud, requested_maximum_order, budget);
  ExactCriticalCatalogResult result;
  result.requested_budget = budget;
  result.point_count = cloud.size();
  result.requested_maximum_order = requested_maximum_order;
  result.effective_maximum_order =
      std::min(requested_maximum_order, cloud.size());
  result.maximum_relevant_closed_rank = std::min(
      checked_add(
          result.effective_maximum_order,
          1U,
          "the critical-catalog maximum rank overflows size_t"),
      cloud.size());
  const std::array<std::size_t, 4> required_by_size =
      candidate_counts_by_size(cloud.size());
  result.required_candidate_count = candidate_count_sum(required_by_size);
  result.required_point_classification_count = checked_multiply(
      result.required_candidate_count,
      cloud.size(),
      "the critical-catalog point-classification preflight overflows size_t");
  result.counters.preflight_count = 1U;
  result.candidate_space_size_certified = true;
  result.scope = ExactCriticalCatalogScope::
      bounded_n14_k10_exhaustive_supports_up_to_four_critical_catalog_h0_batches_only;

  if (budget.maximum_candidate_count <
          result.required_candidate_count ||
      budget.maximum_point_classification_count <
          result.required_point_classification_count) {
    result.decision = ExactCriticalCatalogDecision::
        no_catalog_preflight_budget_insufficient;
    return result;
  }

  result.preflight_budget_sufficient = true;
  result.geometry_started_after_successful_preflight = true;
  result.candidates.reserve(result.required_candidate_count);
  exact::PredicateCounters predicate_counters;
  enumerate_supports<1U>(cloud, predicate_counters, result);
  enumerate_supports<2U>(cloud, predicate_counters, result);
  enumerate_supports<3U>(cloud, predicate_counters, result);
  enumerate_supports<4U>(cloud, predicate_counters, result);
  result.predicate_counters = predicate_snapshot(predicate_counters);

  if (result.counters.enumerated_candidate_count !=
          result.required_candidate_count ||
      result.counters.enumerated_candidate_count_by_support_size !=
          required_by_size ||
      result.counters.support_analysis_count !=
          result.required_candidate_count) {
    throw std::logic_error(
        "the critical catalogue missed its exhaustive support preflight");
  }
  canonicalize_events(result);
  canonicalize_degeneracies(result);
  result.counters.deduplicated_extra_shell_degeneracy_count =
      result.extra_shell_degeneracies.size();
  build_h0_batches(result);

  result.all_support_candidates_classified =
      candidates_are_canonical(result);
  result.global_closed_ball_queries_restricted_to_minimal_supports =
      result.counters.global_closed_ball_query_count ==
          result.counters.minimal_support_count &&
      result.counters.global_point_classification_count ==
          checked_multiply(
              result.counters.minimal_support_count,
              cloud.size(),
              "the critical-catalog minimal point count overflows size_t");
  result.all_minimal_support_global_partitions_complete =
      result.global_closed_ball_queries_restricted_to_minimal_supports;
  result.extra_shell_degeneracies_deduplicated =
      degeneracies_are_deduplicated_and_indexed(result);
  result.accepted_events_canonical_and_indexed =
      events_are_canonical_and_indexed(result);
  result.h0_batches_canonical_and_complete =
      h0_batches_are_canonical_and_complete(result);
  result.no_relevant_extra_shell_degeneracy =
      result.counters.relevant_extra_shell_candidate_count == 0U &&
      std::none_of(
          result.extra_shell_degeneracies.begin(),
          result.extra_shell_degeneracies.end(),
          [](const ExactCriticalExtraShellDegeneracy& degeneracy) {
            return degeneracy.has_relevant_support;
          });

  if (!result.all_support_candidates_classified ||
      !result.global_closed_ball_queries_restricted_to_minimal_supports ||
      !result.all_minimal_support_global_partitions_complete ||
      !result.extra_shell_degeneracies_deduplicated ||
      !result.accepted_events_canonical_and_indexed ||
      !result.h0_batches_canonical_and_complete ||
      result.counters.accepted_event_count != result.events.size() ||
      result.counters.deduplicated_extra_shell_degeneracy_count !=
          result.extra_shell_degeneracies.size()) {
    throw std::logic_error(
        "the exact critical catalogue failed its structural closure");
  }
  result.decision = result.no_relevant_extra_shell_degeneracy
      ? ExactCriticalCatalogDecision::complete_supported_critical_catalog
      : ExactCriticalCatalogDecision::
            complete_catalog_with_relevant_extra_shell_degeneracy;
  return result;
}

}  // namespace

std::string_view to_string(
    ExactCriticalCatalogCandidateOutcome outcome) {
  switch (outcome) {
    case ExactCriticalCatalogCandidateOutcome::not_classified:
      return "not_classified";
    case ExactCriticalCatalogCandidateOutcome::
        affinely_dependent_support:
      return "affinely_dependent_support";
    case ExactCriticalCatalogCandidateOutcome::boundary_reduced_support:
      return "boundary_reduced_support";
    case ExactCriticalCatalogCandidateOutcome::
        exterior_circumcenter_support:
      return "exterior_circumcenter_support";
    case ExactCriticalCatalogCandidateOutcome::
        relevant_extra_shell_degeneracy:
      return "relevant_extra_shell_degeneracy";
    case ExactCriticalCatalogCandidateOutcome::
        extra_shell_outside_relevant_window:
      return "extra_shell_outside_relevant_window";
    case ExactCriticalCatalogCandidateOutcome::
        minimal_support_above_rank_window:
      return "minimal_support_above_rank_window";
    case ExactCriticalCatalogCandidateOutcome::accepted_critical_event:
      return "accepted_critical_event";
  }
  throw std::invalid_argument(
      "the exact critical-catalog candidate outcome is invalid");
}

ExactCriticalCatalogVerification verify_exact_critical_catalog(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    ExactCriticalCatalogBudget budget,
    const ExactCriticalCatalogResult& result) {
  const ExactCriticalCatalogResult expected =
      compute_exact_critical_catalog(
          cloud, requested_maximum_order, budget);
  ExactCriticalCatalogVerification verification;
  verification.requested_budget_certified =
      result.requested_budget == budget &&
      result.requested_budget == expected.requested_budget;
  verification.input_domain_certified =
      result.point_count == cloud.size() &&
      result.point_count == expected.point_count &&
      result.requested_maximum_order == requested_maximum_order &&
      result.requested_maximum_order ==
          expected.requested_maximum_order &&
      result.effective_maximum_order ==
          expected.effective_maximum_order &&
      result.maximum_relevant_closed_rank ==
          expected.maximum_relevant_closed_rank;
  verification.derived_sizes_certified =
      result.required_candidate_count ==
          expected.required_candidate_count &&
      result.required_point_classification_count ==
          expected.required_point_classification_count &&
      result.candidate_space_size_certified ==
          expected.candidate_space_size_certified;
  verification.candidates_certified =
      result.candidates == expected.candidates;
  verification.events_certified =
      result.events == expected.events;
  verification.extra_shell_degeneracies_certified =
      result.extra_shell_degeneracies ==
          expected.extra_shell_degeneracies;
  verification.h0_batches_certified =
      result.h0_batches == expected.h0_batches;
  verification.predicate_counters_certified =
      result.predicate_counters == expected.predicate_counters;
  verification.result_facts_certified =
      result.preflight_budget_sufficient ==
          expected.preflight_budget_sufficient &&
      result.geometry_started_after_successful_preflight ==
          expected.geometry_started_after_successful_preflight &&
      result.all_support_candidates_classified ==
          expected.all_support_candidates_classified &&
      result.global_closed_ball_queries_restricted_to_minimal_supports ==
          expected.global_closed_ball_queries_restricted_to_minimal_supports &&
      result.all_minimal_support_global_partitions_complete ==
          expected.all_minimal_support_global_partitions_complete &&
      result.extra_shell_degeneracies_deduplicated ==
          expected.extra_shell_degeneracies_deduplicated &&
      result.accepted_events_canonical_and_indexed ==
          expected.accepted_events_canonical_and_indexed &&
      result.h0_batches_canonical_and_complete ==
          expected.h0_batches_canonical_and_complete &&
      result.no_relevant_extra_shell_degeneracy ==
          expected.no_relevant_extra_shell_degeneracy;
  verification.counters_certified =
      result.counters == expected.counters;
  verification.decision_certified =
      result.decision == expected.decision;
  verification.scope_certified =
      result.scope == ExactCriticalCatalogScope::
          bounded_n14_k10_exhaustive_supports_up_to_four_critical_catalog_h0_batches_only &&
      result.scope == expected.scope;
  verification.fresh_replay_certified = result == expected;
  verification.exact_critical_catalog_decision_certified =
      verification.requested_budget_certified &&
      verification.input_domain_certified &&
      verification.derived_sizes_certified &&
      verification.candidates_certified &&
      verification.events_certified &&
      verification.extra_shell_degeneracies_certified &&
      verification.h0_batches_certified &&
      verification.predicate_counters_certified &&
      verification.result_facts_certified &&
      verification.counters_certified &&
      verification.decision_certified &&
      verification.scope_certified &&
      verification.fresh_replay_certified;
  return verification;
}

ExactCriticalCatalogResult build_exact_critical_catalog(
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    ExactCriticalCatalogBudget budget) {
  ExactCriticalCatalogResult result = compute_exact_critical_catalog(
      cloud, requested_maximum_order, budget);
  const ExactCriticalCatalogVerification verification =
      verify_exact_critical_catalog(
          cloud, requested_maximum_order, budget, result);
  if (!verification.exact_critical_catalog_decision_certified) {
    throw std::logic_error(
        "the exact critical catalogue failed its fresh replay");
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
