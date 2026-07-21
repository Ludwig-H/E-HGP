#include "morsehgp3d/spatial/ordinary_diagram_closure.hpp"

#include "morsehgp3d/spatial/brute_force.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <map>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace morsehgp3d::spatial {
namespace {

using ExactVector3 = std::array<exact::ExactRational, 3>;

struct ReconciledDiagramProjection {
  std::vector<ExactOrdinaryDiagramVertex> global_vertices;
  std::vector<ExactOrdinaryDiagramContact> contacts;
  ExactOrdinaryDiagramClosureAudit audit;
};

struct BoundaryProjection {
  std::uint8_t artificial_box_face_mask{};
  std::vector<PointId> active_competitor_ids;
};

[[nodiscard]] bool strictly_increasing_ids(
    const std::vector<PointId>& ids) {
  return std::adjacent_find(
             ids.begin(),
             ids.end(),
             [](PointId left, PointId right) { return left >= right; }) ==
         ids.end();
}

[[nodiscard]] bool strictly_increasing_indices(
    const std::vector<std::size_t>& indices) {
  return std::adjacent_find(
             indices.begin(),
             indices.end(),
             [](std::size_t left, std::size_t right) {
               return left >= right;
             }) == indices.end();
}

[[nodiscard]] std::size_t choose_three(std::size_t count) {
  if (count < 3U) {
    return 0U;
  }
  return count * (count - 1U) * (count - 2U) / 6U;
}

void validate_budget(const ExactOrdinaryDiagramClosureBudget& budget) {
  if (budget.maximum_cell_count >
          ExactOrdinaryDiagramClosureBudget::trusted_maximum_cell_count ||
      budget.maximum_cell_construction_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_cell_construction_count ||
      budget.maximum_cumulative_plane_triple_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_cumulative_plane_triple_count ||
      budget.maximum_cumulative_vertex_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_cumulative_vertex_count ||
      budget.maximum_cumulative_incidence_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_cumulative_incidence_count ||
      budget.maximum_vertex_query_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_vertex_query_count ||
      budget.maximum_exact_distance_evaluation_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_exact_distance_evaluation_count ||
      budget.maximum_nearest_shell_entry_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_nearest_shell_entry_count ||
      budget.maximum_owner_strict_feasibility_test_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_owner_strict_feasibility_test_count ||
      budget.maximum_simultaneous_addition_batch_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_simultaneous_addition_batch_count ||
      budget.maximum_total_simultaneous_addition_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_total_simultaneous_addition_count ||
      budget.maximum_simultaneous_batch_size >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_simultaneous_batch_size ||
      budget.maximum_final_cell_vertex_occurrence_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_final_cell_vertex_occurrence_count ||
      budget.maximum_global_vertex_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_global_vertex_count ||
      budget.maximum_global_nearest_shell_entry_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_global_nearest_shell_entry_count ||
      budget.maximum_contact_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_contact_count ||
      budget.maximum_contact_query_id_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_contact_query_id_count ||
      budget.maximum_contact_carrier_shell_id_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_contact_carrier_shell_id_count ||
      budget.maximum_contact_vertex_reference_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_contact_vertex_reference_count ||
      budget.maximum_stratum_witness_query_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_stratum_witness_query_count ||
      budget.maximum_stratum_witness_exact_distance_evaluation_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_stratum_witness_exact_distance_evaluation_count) {
    throw std::invalid_argument(
        "a Phase 8 ordinary-diagram budget exceeds its trusted cap");
  }
}

[[nodiscard]] ExactOrdinaryDiagramClosureRequirements requirements_for(
    std::size_t point_count) {
  const std::size_t complete_competitor_count = point_count - 1U;
  const std::size_t effective_seed_count = point_count == 1U ? 0U : 1U;
  std::size_t local_triple_count = 0U;
  std::size_t local_incidence_count = 0U;
  for (std::size_t candidate_count = effective_seed_count;
       candidate_count <= complete_competitor_count;
       ++candidate_count) {
    const std::size_t boundary_count = 6U + candidate_count;
    const std::size_t triple_count = choose_three(boundary_count);
    local_triple_count += triple_count;
    local_incidence_count += boundary_count * triple_count;
  }

  ExactOrdinaryDiagramClosureRequirements result;
  result.point_count = point_count;
  result.conservative_cell_count = point_count;
  result.conservative_cell_construction_count =
      point_count *
      (complete_competitor_count - effective_seed_count + 1U);
  result.conservative_cumulative_plane_triple_count =
      point_count * local_triple_count;
  result.conservative_cumulative_vertex_count =
      result.conservative_cumulative_plane_triple_count;
  result.conservative_cumulative_incidence_count =
      point_count * local_incidence_count;
  result.conservative_vertex_query_count =
      result.conservative_cumulative_vertex_count;
  result.conservative_exact_distance_evaluation_count =
      point_count * result.conservative_vertex_query_count;
  result.conservative_nearest_shell_entry_count =
      result.conservative_exact_distance_evaluation_count;
  result.conservative_owner_strict_feasibility_test_count =
      point_count * complete_competitor_count;
  result.conservative_simultaneous_addition_batch_count =
      point_count * (complete_competitor_count - effective_seed_count);
  result.conservative_total_simultaneous_addition_count =
      result.conservative_simultaneous_addition_batch_count;
  result.conservative_maximum_simultaneous_batch_size =
      complete_competitor_count - effective_seed_count;

  const std::size_t final_boundary_count = 6U + complete_competitor_count;
  const std::size_t final_vertex_capacity =
      choose_three(final_boundary_count);
  result.conservative_final_cell_vertex_occurrence_count =
      point_count * final_vertex_capacity;
  result.conservative_global_vertex_count =
      result.conservative_final_cell_vertex_occurrence_count;
  result.conservative_global_nearest_shell_entry_count =
      point_count * result.conservative_global_vertex_count;

  const std::size_t subset_count = std::size_t{1U} << point_count;
  result.conservative_contact_count =
      point_count >= 2U ? subset_count - point_count - 1U : 0U;
  result.conservative_contact_query_id_count =
      point_count >= 2U
          ? point_count * ((std::size_t{1U} << (point_count - 1U)) - 1U)
          : 0U;
  result.conservative_contact_carrier_shell_id_count =
      point_count * result.conservative_contact_count;
  result.conservative_contact_vertex_reference_count =
      result.conservative_global_vertex_count *
      result.conservative_contact_count;
  result.conservative_stratum_witness_query_count =
      result.conservative_contact_count;
  result.conservative_stratum_witness_exact_distance_evaluation_count =
      point_count * result.conservative_contact_count;

  if (result.conservative_cell_count >
          ExactOrdinaryDiagramClosureBudget::trusted_maximum_cell_count ||
      result.conservative_cell_construction_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_cell_construction_count ||
      result.conservative_cumulative_plane_triple_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_cumulative_plane_triple_count ||
      result.conservative_cumulative_vertex_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_cumulative_vertex_count ||
      result.conservative_cumulative_incidence_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_cumulative_incidence_count ||
      result.conservative_vertex_query_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_vertex_query_count ||
      result.conservative_exact_distance_evaluation_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_exact_distance_evaluation_count ||
      result.conservative_nearest_shell_entry_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_nearest_shell_entry_count ||
      result.conservative_owner_strict_feasibility_test_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_owner_strict_feasibility_test_count ||
      result.conservative_simultaneous_addition_batch_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_simultaneous_addition_batch_count ||
      result.conservative_total_simultaneous_addition_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_total_simultaneous_addition_count ||
      result.conservative_maximum_simultaneous_batch_size >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_simultaneous_batch_size ||
      result.conservative_final_cell_vertex_occurrence_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_final_cell_vertex_occurrence_count ||
      result.conservative_global_vertex_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_global_vertex_count ||
      result.conservative_global_nearest_shell_entry_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_global_nearest_shell_entry_count ||
      result.conservative_contact_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_contact_count ||
      result.conservative_contact_query_id_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_contact_query_id_count ||
      result.conservative_contact_carrier_shell_id_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_contact_carrier_shell_id_count ||
      result.conservative_contact_vertex_reference_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_contact_vertex_reference_count ||
      result.conservative_stratum_witness_query_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_stratum_witness_query_count ||
      result.conservative_stratum_witness_exact_distance_evaluation_count >
          ExactOrdinaryDiagramClosureBudget::
              trusted_maximum_stratum_witness_exact_distance_evaluation_count) {
    throw std::logic_error(
        "an ordinary-diagram requirement exceeded its proof cap");
  }
  return result;
}

[[nodiscard]] bool budget_covers(
    const ExactOrdinaryDiagramClosureBudget& budget,
    const ExactOrdinaryDiagramClosureRequirements& requirements) {
  return budget.maximum_cell_count >= requirements.conservative_cell_count &&
         budget.maximum_cell_construction_count >=
             requirements.conservative_cell_construction_count &&
         budget.maximum_cumulative_plane_triple_count >=
             requirements.conservative_cumulative_plane_triple_count &&
         budget.maximum_cumulative_vertex_count >=
             requirements.conservative_cumulative_vertex_count &&
         budget.maximum_cumulative_incidence_count >=
             requirements.conservative_cumulative_incidence_count &&
         budget.maximum_vertex_query_count >=
             requirements.conservative_vertex_query_count &&
         budget.maximum_exact_distance_evaluation_count >=
             requirements.conservative_exact_distance_evaluation_count &&
         budget.maximum_nearest_shell_entry_count >=
             requirements.conservative_nearest_shell_entry_count &&
         budget.maximum_owner_strict_feasibility_test_count >=
             requirements
                 .conservative_owner_strict_feasibility_test_count &&
         budget.maximum_simultaneous_addition_batch_count >=
             requirements.conservative_simultaneous_addition_batch_count &&
         budget.maximum_total_simultaneous_addition_count >=
             requirements.conservative_total_simultaneous_addition_count &&
         budget.maximum_simultaneous_batch_size >=
             requirements.conservative_maximum_simultaneous_batch_size &&
         budget.maximum_final_cell_vertex_occurrence_count >=
             requirements
                 .conservative_final_cell_vertex_occurrence_count &&
         budget.maximum_global_vertex_count >=
             requirements.conservative_global_vertex_count &&
         budget.maximum_global_nearest_shell_entry_count >=
             requirements.conservative_global_nearest_shell_entry_count &&
         budget.maximum_contact_count >=
             requirements.conservative_contact_count &&
         budget.maximum_contact_query_id_count >=
             requirements.conservative_contact_query_id_count &&
         budget.maximum_contact_carrier_shell_id_count >=
             requirements.conservative_contact_carrier_shell_id_count &&
         budget.maximum_contact_vertex_reference_count >=
             requirements.conservative_contact_vertex_reference_count &&
         budget.maximum_stratum_witness_query_count >=
             requirements.conservative_stratum_witness_query_count &&
         budget.maximum_stratum_witness_exact_distance_evaluation_count >=
             requirements
                 .conservative_stratum_witness_exact_distance_evaluation_count;
}

void require_complete_clipping_box(
    const CanonicalPointCloud& cloud,
    const StrictlyPaddedDyadicAabb3Result& clipping_box) {
  const StrictlyPaddedDyadicAabb3Verification verification =
      verify_strictly_padded_dyadic_aabb(cloud, clipping_box);
  if (!verification.result_certified ||
      clipping_box.decision != StrictDyadicPaddingDecision::complete ||
      !clipping_box.certificate.has_value()) {
    throw std::invalid_argument(
        "an ordinary diagram requires a complete freshly verified Phase 8 "
        "clipping box");
  }
}

[[nodiscard]] std::optional<std::uint8_t> artificial_face_bit(
    PowerCellBoundaryKind kind) {
  switch (kind) {
    case PowerCellBoundaryKind::box_lower_x:
      return UINT8_C(1) << 0U;
    case PowerCellBoundaryKind::box_upper_x:
      return UINT8_C(1) << 1U;
    case PowerCellBoundaryKind::box_lower_y:
      return UINT8_C(1) << 2U;
    case PowerCellBoundaryKind::box_upper_y:
      return UINT8_C(1) << 3U;
    case PowerCellBoundaryKind::box_lower_z:
      return UINT8_C(1) << 4U;
    case PowerCellBoundaryKind::box_upper_z:
      return UINT8_C(1) << 5U;
    case PowerCellBoundaryKind::power_bisector:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] BoundaryProjection boundary_projection(
    const ExactPowerCellReferenceResult& cell,
    const ExactPowerCellVertex& vertex) {
  BoundaryProjection result;
  for (const std::size_t boundary_index :
       vertex.active_boundary_plane_indices) {
    if (boundary_index >= cell.boundary_planes.size()) {
      throw std::logic_error(
          "an ordinary-diagram cell vertex references an invalid boundary");
    }
    const ExactPowerCellBoundaryPlane& boundary =
        cell.boundary_planes[boundary_index];
    const std::optional<std::uint8_t> face_bit =
        artificial_face_bit(boundary.kind);
    if (face_bit.has_value()) {
      if (boundary.competitor_id.has_value()) {
        throw std::logic_error(
            "an artificial ordinary-diagram boundary names a competitor");
      }
      result.artificial_box_face_mask = static_cast<std::uint8_t>(
          result.artificial_box_face_mask | *face_bit);
      continue;
    }
    if (boundary.kind != PowerCellBoundaryKind::power_bisector ||
        !boundary.competitor_id.has_value()) {
      throw std::logic_error(
          "a natural ordinary-diagram boundary lacks a competitor");
    }
    result.active_competitor_ids.push_back(*boundary.competitor_id);
  }
  std::sort(
      result.active_competitor_ids.begin(),
      result.active_competitor_ids.end());
  if (!result.active_competitor_ids.empty() &&
      !strictly_increasing_ids(result.active_competitor_ids)) {
    throw std::logic_error(
        "an ordinary-diagram vertex repeats an active competitor");
  }
  return result;
}

[[nodiscard]] std::string position_key(
    const exact::ExactRational3& position) {
  return position.coordinate(0U).canonical_key() + ":" +
         position.coordinate(1U).canonical_key() + ":" +
         position.coordinate(2U).canonical_key();
}

[[nodiscard]] ExactVector3 subtract(
    const exact::ExactRational3& left,
    const exact::ExactRational3& right) {
  return {
      left.coordinate(0U) - right.coordinate(0U),
      left.coordinate(1U) - right.coordinate(1U),
      left.coordinate(2U) - right.coordinate(2U)};
}

[[nodiscard]] ExactVector3 cross(
    const ExactVector3& left,
    const ExactVector3& right) {
  return {
      left[1U] * right[2U] - left[2U] * right[1U],
      left[2U] * right[0U] - left[0U] * right[2U],
      left[0U] * right[1U] - left[1U] * right[0U]};
}

[[nodiscard]] exact::ExactRational dot(
    const ExactVector3& left,
    const ExactVector3& right) {
  return left[0U] * right[0U] + left[1U] * right[1U] +
         left[2U] * right[2U];
}

[[nodiscard]] bool is_zero(const ExactVector3& vector) {
  return vector[0U].is_zero() && vector[1U].is_zero() &&
         vector[2U].is_zero();
}

[[nodiscard]] std::size_t affine_dimension(
    std::span<const exact::ExactRational3> points) {
  if (points.empty()) {
    throw std::invalid_argument(
        "an empty exact point set has no affine dimension");
  }
  const exact::ExactRational3& base = points.front();
  std::optional<ExactVector3> first_direction;
  for (std::size_t index = 1U; index < points.size(); ++index) {
    ExactVector3 direction = subtract(points[index], base);
    if (!is_zero(direction)) {
      first_direction = std::move(direction);
      break;
    }
  }
  if (!first_direction.has_value()) {
    return 0U;
  }
  std::optional<ExactVector3> spanning_normal;
  for (std::size_t index = 1U; index < points.size(); ++index) {
    ExactVector3 normal =
        cross(*first_direction, subtract(points[index], base));
    if (!is_zero(normal)) {
      spanning_normal = std::move(normal);
      break;
    }
  }
  if (!spanning_normal.has_value()) {
    return 1U;
  }
  for (std::size_t index = 1U; index < points.size(); ++index) {
    if (!dot(*spanning_normal, subtract(points[index], base)).is_zero()) {
      return 3U;
    }
  }
  return 2U;
}

[[nodiscard]] exact::ExactRational3 average_position(
    std::span<const exact::ExactRational3> points) {
  if (points.empty()) {
    throw std::invalid_argument("an empty point set has no average");
  }
  std::array<exact::ExactRational, 3> sums{};
  for (const exact::ExactRational3& point : points) {
    for (std::size_t axis = 0U; axis < sums.size(); ++axis) {
      sums[axis] = sums[axis] + point.coordinate(axis);
    }
  }
  const exact::ExactRational divisor{
      exact::BigInt{points.size()}};
  for (exact::ExactRational& sum : sums) {
    sum = sum / divisor;
  }
  return exact::ExactRational3{sums};
}

[[nodiscard]] std::vector<PointId> shell_intersection(
    const std::vector<ExactOrdinaryDiagramVertex>& vertices,
    std::span<const std::size_t> vertex_indices) {
  if (vertex_indices.empty()) {
    throw std::invalid_argument("an empty contact has no carrier shell");
  }
  std::vector<PointId> result =
      vertices[vertex_indices.front()].complete_nearest_shell_ids;
  for (std::size_t offset = 1U; offset < vertex_indices.size(); ++offset) {
    std::vector<PointId> intersection;
    const std::vector<PointId>& shell =
        vertices[vertex_indices[offset]].complete_nearest_shell_ids;
    std::set_intersection(
        result.begin(),
        result.end(),
        shell.begin(),
        shell.end(),
        std::back_inserter(intersection));
    result = std::move(intersection);
  }
  return result;
}

[[nodiscard]] std::uint8_t common_artificial_mask(
    const std::vector<ExactOrdinaryDiagramVertex>& vertices,
    std::span<const std::size_t> vertex_indices) {
  if (vertex_indices.empty()) {
    throw std::invalid_argument("an empty contact has no artificial mask");
  }
  std::uint8_t result =
      vertices[vertex_indices.front()].artificial_box_face_mask;
  for (std::size_t offset = 1U; offset < vertex_indices.size(); ++offset) {
    result = static_cast<std::uint8_t>(
        result &
        vertices[vertex_indices[offset]].artificial_box_face_mask);
  }
  return result;
}

[[nodiscard]] ExactOrdinaryDiagramContactKind classify_contact(
    bool canonical,
    std::uint8_t common_box_mask,
    std::size_t site_rank,
    std::size_t contact_dimension) {
  if (!canonical) {
    return ExactOrdinaryDiagramContactKind::noncanonical_quotient_contact;
  }
  if (common_box_mask != 0U) {
    return ExactOrdinaryDiagramContactKind::box_supported_contact;
  }
  if (site_rank + contact_dimension != 3U) {
    throw std::logic_error(
        "an interior canonical ordinary-diagram stratum has inconsistent "
        "exact ranks");
  }
  switch (site_rank) {
    case 1U:
      return ExactOrdinaryDiagramContactKind::natural_face;
    case 2U:
      return ExactOrdinaryDiagramContactKind::natural_edge;
    case 3U:
      return ExactOrdinaryDiagramContactKind::natural_vertex;
    default:
      throw std::logic_error(
          "a canonical ordinary-diagram stratum has invalid site rank");
  }
}

[[nodiscard]] ReconciledDiagramProjection reconcile_cells(
    const CanonicalPointCloud& cloud,
    const std::vector<ExactOrdinaryCellClosureResult>& cells) {
  std::map<std::string, ExactOrdinaryDiagramVertex> grouped_vertices;
  ReconciledDiagramProjection result;

  for (std::size_t owner_index = 0U;
       owner_index < cells.size();
       ++owner_index) {
    const PointId owner_id = static_cast<PointId>(owner_index);
    const ExactOrdinaryCellClosureResult& cell = cells[owner_index];
    const ExactPowerCellReferenceResult* final_cell = cell.final_cell();
    if (cell.owner_id != owner_id || final_cell == nullptr ||
        cell.rounds.empty() ||
        cell.rounds.back().vertex_queries.size() !=
            final_cell->vertices.size()) {
      throw std::logic_error(
          "an ordinary diagram received a malformed completed local cell");
    }
    const ExactOrdinaryCellClosureRound& final_round = cell.rounds.back();
    for (std::size_t vertex_index = 0U;
         vertex_index < final_cell->vertices.size();
         ++vertex_index) {
      const ExactPowerCellVertex& local_vertex =
          final_cell->vertices[vertex_index];
      const ExactOrdinaryCellVertexQueryRecord& query =
          final_round.vertex_queries[vertex_index];
      const BoundaryProjection projection =
          boundary_projection(*final_cell, local_vertex);
      std::vector<PointId> expected_active;
      for (const PointId shell_id : query.complete_nearest_shell_ids) {
        if (shell_id != owner_id) {
          expected_active.push_back(shell_id);
        }
      }
      if (projection.active_competitor_ids != expected_active ||
          !std::binary_search(
              query.complete_nearest_shell_ids.begin(),
              query.complete_nearest_shell_ids.end(),
              owner_id)) {
        throw std::logic_error(
            "a final local vertex disagrees with its complete nearest shell");
      }

      const std::string key = position_key(local_vertex.position);
      auto [iterator, inserted] = grouped_vertices.try_emplace(key);
      ExactOrdinaryDiagramVertex& global_vertex = iterator->second;
      if (inserted) {
        global_vertex.position = local_vertex.position;
        global_vertex.nearest_squared_distance =
            query.nearest_squared_distance;
        global_vertex.complete_nearest_shell_ids =
            query.complete_nearest_shell_ids;
        global_vertex.artificial_box_face_mask =
            projection.artificial_box_face_mask;
      } else if (
          global_vertex.position != local_vertex.position ||
          global_vertex.nearest_squared_distance !=
              query.nearest_squared_distance ||
          global_vertex.complete_nearest_shell_ids !=
              query.complete_nearest_shell_ids ||
          global_vertex.artificial_box_face_mask !=
              projection.artificial_box_face_mask) {
        throw std::logic_error(
            "reciprocal ordinary cells disagree at an exact global vertex");
      }
      global_vertex.cell_occurrences.push_back(
          ExactOrdinaryDiagramVertexOccurrence{owner_id, vertex_index});
    }
  }

  result.global_vertices.reserve(grouped_vertices.size());
  for (auto& [key, vertex] : grouped_vertices) {
    static_cast<void>(key);
    std::vector<PointId> occurrence_owners;
    occurrence_owners.reserve(vertex.cell_occurrences.size());
    for (const ExactOrdinaryDiagramVertexOccurrence& occurrence :
         vertex.cell_occurrences) {
      occurrence_owners.push_back(occurrence.owner_id);
    }
    if (occurrence_owners.empty() ||
        !strictly_increasing_ids(occurrence_owners) ||
        occurrence_owners != vertex.complete_nearest_shell_ids) {
      throw std::logic_error(
          "global nearest-shell membership is not a reciprocal occurrence "
          "bijection");
    }
    result.audit.final_cell_vertex_occurrence_count +=
        vertex.cell_occurrences.size();
    result.audit.global_nearest_shell_entry_count +=
        vertex.complete_nearest_shell_ids.size();
    result.global_vertices.push_back(std::move(vertex));
  }
  result.audit.global_vertex_count = result.global_vertices.size();

  std::map<std::vector<PointId>, std::vector<std::size_t>> contact_vertices;
  for (std::size_t vertex_index = 0U;
       vertex_index < result.global_vertices.size();
       ++vertex_index) {
    const std::vector<PointId>& shell =
        result.global_vertices[vertex_index].complete_nearest_shell_ids;
    if (shell.size() >
        ExactOrdinaryDiagramClosureBudget::trusted_maximum_point_count) {
      throw std::logic_error(
          "a global ordinary-diagram shell exceeds the trusted point cap");
    }
    const std::size_t subset_count = std::size_t{1U} << shell.size();
    for (std::size_t mask = 0U; mask < subset_count; ++mask) {
      if (std::popcount(mask) < 2) {
        continue;
      }
      std::vector<PointId> query_ids;
      query_ids.reserve(shell.size());
      for (std::size_t shell_index = 0U;
           shell_index < shell.size();
           ++shell_index) {
        if ((mask & (std::size_t{1U} << shell_index)) != 0U) {
          query_ids.push_back(shell[shell_index]);
        }
      }
      contact_vertices[query_ids].push_back(vertex_index);
    }
  }

  const std::array<PointId, 0> no_exclusions{};
  const ExclusionSet exclusions = ExclusionSet::from_ids(
      std::span<const PointId>{no_exclusions}, cloud, 0U);
  result.contacts.reserve(contact_vertices.size());
  for (const auto& [query_ids, vertex_indices] : contact_vertices) {
    if (query_ids.size() < 2U || !strictly_increasing_ids(query_ids) ||
        vertex_indices.empty() ||
        !strictly_increasing_indices(vertex_indices)) {
      throw std::logic_error(
          "an ordinary-diagram contact is not canonical");
    }
    ExactOrdinaryDiagramContact contact;
    contact.query_ids = query_ids;
    contact.global_vertex_indices = vertex_indices;
    contact.carrier_shell_ids = shell_intersection(
        result.global_vertices, contact.global_vertex_indices);
    contact.common_artificial_box_face_mask = common_artificial_mask(
        result.global_vertices, contact.global_vertex_indices);

    std::vector<exact::ExactRational3> contact_positions;
    contact_positions.reserve(contact.global_vertex_indices.size());
    for (const std::size_t vertex_index :
         contact.global_vertex_indices) {
      contact_positions.push_back(
          result.global_vertices[vertex_index].position);
    }
    contact.affine_dimension = affine_dimension(contact_positions);
    contact.relative_interior_witness = average_position(contact_positions);

    std::vector<exact::ExactRational3> carrier_positions;
    carrier_positions.reserve(contact.carrier_shell_ids.size());
    for (const PointId carrier_id : contact.carrier_shell_ids) {
      if (static_cast<std::size_t>(carrier_id) >= cloud.size()) {
        throw std::logic_error(
            "an ordinary-diagram carrier contains an invalid site id");
      }
      carrier_positions.push_back(cloud.point(carrier_id).exact());
    }
    contact.site_affine_rank = affine_dimension(carrier_positions);

    const TopKPartition witness_nearest = brute_force_nearest(
        cloud, contact.relative_interior_witness, exclusions);
    const std::vector<PointId> witness_shell{
        witness_nearest.cutoff_shell_ids().begin(),
        witness_nearest.cutoff_shell_ids().end()};
    if (!witness_nearest.shell_complete() ||
        witness_nearest.distance_evaluation_count() != cloud.size() ||
        witness_shell != contact.carrier_shell_ids) {
      throw std::logic_error(
          "a contact barycenter disagrees with its exact carrier shell");
    }
    contact.witness_nearest_squared_distance =
        witness_nearest.cutoff_squared_distance();
    contact.kind = classify_contact(
        contact.query_ids == contact.carrier_shell_ids,
        contact.common_artificial_box_face_mask,
        contact.site_affine_rank,
        contact.affine_dimension);

    ++result.audit.exact_stratum_witness_query_count;
    result.audit.exact_stratum_witness_distance_evaluation_count +=
        witness_nearest.distance_evaluation_count();
    result.audit.contact_query_id_count += contact.query_ids.size();
    result.audit.contact_carrier_shell_id_count +=
        contact.carrier_shell_ids.size();
    result.audit.contact_vertex_reference_count +=
        contact.global_vertex_indices.size();
    switch (contact.kind) {
      case ExactOrdinaryDiagramContactKind::noncanonical_quotient_contact:
        ++result.audit.noncanonical_quotient_contact_count;
        break;
      case ExactOrdinaryDiagramContactKind::natural_face:
        ++result.audit.natural_face_count;
        break;
      case ExactOrdinaryDiagramContactKind::natural_edge:
        ++result.audit.natural_edge_count;
        break;
      case ExactOrdinaryDiagramContactKind::natural_vertex:
        ++result.audit.natural_vertex_count;
        break;
      case ExactOrdinaryDiagramContactKind::box_supported_contact:
        ++result.audit.box_supported_contact_count;
        break;
    }
    result.contacts.push_back(std::move(contact));
  }
  result.audit.contact_count = result.contacts.size();
  return result;
}

void accumulate_cell_audit(
    const ExactOrdinaryCellClosureAudit& cell,
    ExactOrdinaryDiagramClosureAudit& diagram) {
  ++diagram.completed_cell_count;
  diagram.exact_cell_construction_count +=
      cell.exact_cell_construction_count;
  diagram.cumulative_plane_triple_capacity +=
      cell.cumulative_plane_triple_capacity;
  diagram.cumulative_vertex_capacity += cell.cumulative_vertex_capacity;
  diagram.cumulative_incidence_capacity +=
      cell.cumulative_incidence_capacity;
  diagram.exact_vertex_query_count += cell.exact_vertex_query_count;
  diagram.exact_distance_evaluation_count +=
      cell.exact_distance_evaluation_count;
  diagram.nearest_shell_entry_count += cell.nearest_shell_entry_count;
  diagram.owner_strict_feasibility_test_count +=
      cell.owner_strict_feasibility_test_count;
  diagram.simultaneous_addition_batch_count +=
      cell.simultaneous_addition_batch_count;
  diagram.simultaneously_added_competitor_count +=
      cell.simultaneously_added_competitor_count;
  diagram.maximum_simultaneous_batch_size = std::max(
      diagram.maximum_simultaneous_batch_size,
      cell.maximum_simultaneous_addition_count);
}

void accumulate_projection_audit(
    const ExactOrdinaryDiagramClosureAudit& projection,
    ExactOrdinaryDiagramClosureAudit& diagram) {
  diagram.final_cell_vertex_occurrence_count =
      projection.final_cell_vertex_occurrence_count;
  diagram.global_vertex_count = projection.global_vertex_count;
  diagram.global_nearest_shell_entry_count =
      projection.global_nearest_shell_entry_count;
  diagram.contact_count = projection.contact_count;
  diagram.contact_query_id_count = projection.contact_query_id_count;
  diagram.contact_carrier_shell_id_count =
      projection.contact_carrier_shell_id_count;
  diagram.contact_vertex_reference_count =
      projection.contact_vertex_reference_count;
  diagram.noncanonical_quotient_contact_count =
      projection.noncanonical_quotient_contact_count;
  diagram.natural_face_count = projection.natural_face_count;
  diagram.natural_edge_count = projection.natural_edge_count;
  diagram.natural_vertex_count = projection.natural_vertex_count;
  diagram.box_supported_contact_count =
      projection.box_supported_contact_count;
  diagram.exact_stratum_witness_query_count =
      projection.exact_stratum_witness_query_count;
  diagram.exact_stratum_witness_distance_evaluation_count =
      projection.exact_stratum_witness_distance_evaluation_count;
}

[[nodiscard]] bool audit_within_requirements(
    const ExactOrdinaryDiagramClosureAudit& audit,
    const ExactOrdinaryDiagramClosureRequirements& requirements) {
  return audit.completed_cell_count <= requirements.conservative_cell_count &&
         audit.exact_cell_construction_count <=
             requirements.conservative_cell_construction_count &&
         audit.cumulative_plane_triple_capacity <=
             requirements.conservative_cumulative_plane_triple_count &&
         audit.cumulative_vertex_capacity <=
             requirements.conservative_cumulative_vertex_count &&
         audit.cumulative_incidence_capacity <=
             requirements.conservative_cumulative_incidence_count &&
         audit.exact_vertex_query_count <=
             requirements.conservative_vertex_query_count &&
         audit.exact_distance_evaluation_count <=
             requirements.conservative_exact_distance_evaluation_count &&
         audit.nearest_shell_entry_count <=
             requirements.conservative_nearest_shell_entry_count &&
         audit.owner_strict_feasibility_test_count <=
             requirements
                 .conservative_owner_strict_feasibility_test_count &&
         audit.simultaneous_addition_batch_count <=
             requirements.conservative_simultaneous_addition_batch_count &&
         audit.simultaneously_added_competitor_count <=
             requirements.conservative_total_simultaneous_addition_count &&
         audit.maximum_simultaneous_batch_size <=
             requirements.conservative_maximum_simultaneous_batch_size &&
         audit.final_cell_vertex_occurrence_count <=
             requirements
                 .conservative_final_cell_vertex_occurrence_count &&
         audit.global_vertex_count <=
             requirements.conservative_global_vertex_count &&
         audit.global_nearest_shell_entry_count <=
             requirements.conservative_global_nearest_shell_entry_count &&
         audit.contact_count <= requirements.conservative_contact_count &&
         audit.contact_query_id_count <=
             requirements.conservative_contact_query_id_count &&
         audit.contact_carrier_shell_id_count <=
             requirements.conservative_contact_carrier_shell_id_count &&
         audit.contact_vertex_reference_count <=
             requirements.conservative_contact_vertex_reference_count &&
         audit.exact_stratum_witness_query_count <=
             requirements.conservative_stratum_witness_query_count &&
         audit.exact_stratum_witness_distance_evaluation_count <=
             requirements
                 .conservative_stratum_witness_exact_distance_evaluation_count;
}

[[nodiscard]] ExactOrdinaryDiagramClosureResult build_unverified(
    const CanonicalPointCloud& cloud,
    const StrictlyPaddedDyadicAabb3Result& clipping_box,
    ExactOrdinaryDiagramClosureBudget budget) {
  validate_budget(budget);
  if (cloud.size() == 0U ||
      cloud.size() >
          ExactOrdinaryDiagramClosureBudget::trusted_maximum_point_count) {
    throw std::invalid_argument(
        "the bounded ordinary diagram accepts one to eight canonical sites");
  }
  const ExactOrdinaryDiagramClosureRequirements requirements =
      requirements_for(cloud.size());
  require_complete_clipping_box(cloud, clipping_box);

  ExactOrdinaryDiagramClosureResult result;
  result.canonical_point_bits.reserve(cloud.size());
  for (std::size_t point_index = 0U;
       point_index < cloud.size();
       ++point_index) {
    result.canonical_point_bits.push_back(
        cloud.point(static_cast<PointId>(point_index)).canonical_input_bits());
  }
  result.requirements = requirements;
  result.clipping_box = clipping_box;
  if (!budget_covers(budget, requirements)) {
    return result;
  }

  const std::array<PointId, 0> empty_seed{};
  result.cells.reserve(cloud.size());
  for (std::size_t owner_index = 0U;
       owner_index < cloud.size();
       ++owner_index) {
    ExactOrdinaryCellClosureResult cell =
        build_exact_bounded_ordinary_cell_closure(
            cloud,
            static_cast<PointId>(owner_index),
            clipping_box,
            empty_seed);
    if (cell.decision !=
            ExactOrdinaryCellClosureDecision::complete_nonempty ||
        !cell.local_queue_empty_certified ||
        !cell.full_dimensional_nonempty_certified ||
        !cell.active_nearest_shells_reconciled_certified ||
        !cell.artificial_box_boundaries_certified) {
      throw std::logic_error(
          "an ordinary diagram contains an uncertified local cell");
    }
    accumulate_cell_audit(cell.audit, result.audit);
    result.cells.push_back(std::move(cell));
  }

  ReconciledDiagramProjection projection =
      reconcile_cells(cloud, result.cells);
  result.global_vertices = std::move(projection.global_vertices);
  result.contacts = std::move(projection.contacts);
  accumulate_projection_audit(projection.audit, result.audit);
  if (!audit_within_requirements(result.audit, result.requirements)) {
    throw std::logic_error(
        "the completed ordinary diagram exceeded its proved requirements");
  }

  result.all_local_queues_empty_certified = true;
  result.all_cells_full_dimensional_nonempty_certified = true;
  result.global_vertex_occurrence_bijection_certified = true;
  result.natural_incidences_reconciled_certified = true;
  result.artificial_box_boundaries_certified = true;
  result.decision = ExactOrdinaryDiagramClosureDecision::complete;
  return result;
}

[[nodiscard]] bool insufficient_shape(
    const ExactOrdinaryDiagramClosureResult& result) {
  return result.cells.empty() && result.global_vertices.empty() &&
         result.contacts.empty() &&
         result.audit == ExactOrdinaryDiagramClosureAudit{} &&
         !result.all_local_queues_empty_certified &&
         !result.all_cells_full_dimensional_nonempty_certified &&
         !result.global_vertex_occurrence_bijection_certified &&
         !result.natural_incidences_reconciled_certified &&
         !result.artificial_box_boundaries_certified;
}

}  // namespace

std::string_view to_string(ExactOrdinaryDiagramContactKind kind) {
  switch (kind) {
    case ExactOrdinaryDiagramContactKind::noncanonical_quotient_contact:
      return "noncanonical_quotient_contact";
    case ExactOrdinaryDiagramContactKind::natural_face:
      return "natural_face";
    case ExactOrdinaryDiagramContactKind::natural_edge:
      return "natural_edge";
    case ExactOrdinaryDiagramContactKind::natural_vertex:
      return "natural_vertex";
    case ExactOrdinaryDiagramContactKind::box_supported_contact:
      return "box_supported_contact";
  }
  throw std::invalid_argument("an ordinary-diagram contact kind is invalid");
}

std::string_view to_string(
    ExactOrdinaryDiagramClosureDecision decision) {
  switch (decision) {
    case ExactOrdinaryDiagramClosureDecision::complete:
      return "complete";
    case ExactOrdinaryDiagramClosureDecision::insufficient_budget:
      return "insufficient_budget";
  }
  throw std::invalid_argument(
      "an ordinary-diagram closure decision is invalid");
}

ExactOrdinaryDiagramClosureVerification
verify_exact_bounded_ordinary_diagram_closure(
    const CanonicalPointCloud& cloud,
    const StrictlyPaddedDyadicAabb3Result& clipping_box,
    const ExactOrdinaryDiagramClosureResult& result,
    ExactOrdinaryDiagramClosureBudget budget) {
  const ExactOrdinaryDiagramClosureResult expected =
      build_unverified(cloud, clipping_box, budget);
  ExactOrdinaryDiagramClosureVerification verification;
  verification.input_identity_certified =
      result.canonical_point_bits == expected.canonical_point_bits &&
      result.requirements.point_count == cloud.size();
  verification.clipping_box_certified =
      result.clipping_box == clipping_box &&
      verify_strictly_padded_dyadic_aabb(cloud, result.clipping_box)
          .result_certified;
  verification.decision_certified = result.decision == expected.decision;
  verification.requirements_certified =
      result.requirements == expected.requirements;
  verification.audit_certified = result.audit == expected.audit;
  verification.payload_shape_certified =
      result.decision == ExactOrdinaryDiagramClosureDecision::complete
          ? result.cells.size() == cloud.size() &&
                !result.global_vertices.empty()
          : insufficient_shape(result);
  verification.transcript_replay_certified = result == expected;
  if (!verification.transcript_replay_certified) {
    return verification;
  }

  if (result.decision ==
      ExactOrdinaryDiagramClosureDecision::insufficient_budget) {
    verification.result_certified =
        verification.input_identity_certified &&
        verification.clipping_box_certified &&
        verification.decision_certified &&
        verification.requirements_certified &&
        verification.audit_certified &&
        verification.payload_shape_certified &&
        verification.transcript_replay_certified;
    return verification;
  }

  const std::array<PointId, 0> empty_seed{};
  verification.all_cells_freshly_verified_certified = true;
  for (std::size_t owner_index = 0U;
       owner_index < result.cells.size();
       ++owner_index) {
    const ExactOrdinaryCellClosureVerification cell_verification =
        verify_exact_bounded_ordinary_cell_closure(
            cloud,
            static_cast<PointId>(owner_index),
            clipping_box,
            empty_seed,
            result.cells[owner_index]);
    verification.all_cells_freshly_verified_certified =
        verification.all_cells_freshly_verified_certified &&
        cell_verification.result_certified;
  }

  const ReconciledDiagramProjection projection =
      reconcile_cells(cloud, result.cells);
  verification.all_local_queues_empty_certified =
      result.all_local_queues_empty_certified &&
      std::all_of(
          result.cells.begin(),
          result.cells.end(),
          [](const ExactOrdinaryCellClosureResult& cell) {
            return cell.local_queue_empty_certified;
          });
  verification.all_cells_full_dimensional_nonempty_certified =
      result.all_cells_full_dimensional_nonempty_certified &&
      std::all_of(
          result.cells.begin(),
          result.cells.end(),
          [](const ExactOrdinaryCellClosureResult& cell) {
            return cell.full_dimensional_nonempty_certified;
          });
  verification.global_vertex_occurrence_bijection_certified =
      result.global_vertex_occurrence_bijection_certified &&
      projection.global_vertices == result.global_vertices;
  verification.natural_incidences_reconciled_certified =
      result.natural_incidences_reconciled_certified &&
      projection.contacts == result.contacts;
  verification.artificial_box_boundaries_certified =
      result.artificial_box_boundaries_certified &&
      std::all_of(
          result.cells.begin(),
          result.cells.end(),
          [](const ExactOrdinaryCellClosureResult& cell) {
            return cell.artificial_box_boundaries_certified;
          });
  verification.result_certified =
      verification.input_identity_certified &&
      verification.clipping_box_certified &&
      verification.decision_certified &&
      verification.requirements_certified &&
      verification.audit_certified &&
      verification.payload_shape_certified &&
      verification.transcript_replay_certified &&
      verification.all_cells_freshly_verified_certified &&
      verification.all_local_queues_empty_certified &&
      verification.all_cells_full_dimensional_nonempty_certified &&
      verification.global_vertex_occurrence_bijection_certified &&
      verification.natural_incidences_reconciled_certified &&
      verification.artificial_box_boundaries_certified;
  return verification;
}

ExactOrdinaryDiagramClosureResult
build_exact_bounded_ordinary_diagram_closure(
    const CanonicalPointCloud& cloud,
    const StrictlyPaddedDyadicAabb3Result& clipping_box,
    ExactOrdinaryDiagramClosureBudget budget) {
  ExactOrdinaryDiagramClosureResult result =
      build_unverified(cloud, clipping_box, budget);
  const ExactOrdinaryDiagramClosureVerification verification =
      verify_exact_bounded_ordinary_diagram_closure(
          cloud, clipping_box, result, budget);
  if (!verification.result_certified) {
    throw std::logic_error(
        "the Phase 8 ordinary diagram failed its fresh verification");
  }
  return result;
}

}  // namespace morsehgp3d::spatial
