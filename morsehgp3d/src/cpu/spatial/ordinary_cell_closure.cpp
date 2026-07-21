#include "morsehgp3d/spatial/ordinary_cell_closure.hpp"

#include "morsehgp3d/spatial/brute_force.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace morsehgp3d::spatial {
namespace {

struct ValidatedOrdinaryCellInput {
  std::vector<PointId> complete_competitor_ids;
  std::vector<PointId> canonical_requested_initial_competitor_ids;
  std::vector<PointId> canonical_effective_initial_competitor_ids;
  bool fallback_seed_injected{false};
  ExactOrdinaryCellClosureRequirements requirements;
};

struct SemanticVertexSignature {
  std::string position_key;
  std::uint8_t artificial_box_face_mask{};
  std::vector<PointId> active_competitor_ids;

  friend bool operator==(
      const SemanticVertexSignature&,
      const SemanticVertexSignature&) = default;

  friend bool operator<(
      const SemanticVertexSignature& left,
      const SemanticVertexSignature& right) {
    return std::tie(
               left.position_key,
               left.artificial_box_face_mask,
               left.active_competitor_ids) <
           std::tie(
               right.position_key,
               right.artificial_box_face_mask,
               right.active_competitor_ids);
  }
};

[[nodiscard]] bool strictly_increasing_ids(
    const std::vector<PointId>& ids) {
  return std::adjacent_find(
             ids.begin(),
             ids.end(),
             [](PointId left, PointId right) { return left >= right; }) ==
         ids.end();
}

[[nodiscard]] std::size_t choose_three(std::size_t count) {
  if (count < 3U) {
    return 0U;
  }
  return count * (count - 1U) * (count - 2U) / 6U;
}

void validate_budget(const ExactOrdinaryCellClosureBudget& budget) {
  if (budget.maximum_cell_construction_count >
          ExactOrdinaryCellClosureBudget::
              trusted_maximum_cell_construction_count ||
      budget.maximum_cumulative_plane_triple_count >
          ExactOrdinaryCellClosureBudget::
              trusted_maximum_cumulative_plane_triple_count ||
      budget.maximum_cumulative_vertex_count >
          ExactOrdinaryCellClosureBudget::
              trusted_maximum_cumulative_vertex_count ||
      budget.maximum_cumulative_incidence_count >
          ExactOrdinaryCellClosureBudget::
              trusted_maximum_cumulative_incidence_count ||
      budget.maximum_vertex_query_count >
          ExactOrdinaryCellClosureBudget::
              trusted_maximum_vertex_query_count ||
      budget.maximum_exact_distance_evaluation_count >
          ExactOrdinaryCellClosureBudget::
              trusted_maximum_exact_distance_evaluation_count ||
      budget.maximum_nearest_shell_entry_count >
          ExactOrdinaryCellClosureBudget::
              trusted_maximum_nearest_shell_entry_count ||
      budget.maximum_owner_strict_feasibility_test_count >
          ExactOrdinaryCellClosureBudget::
              trusted_maximum_owner_strict_feasibility_test_count ||
      budget.maximum_simultaneous_addition_count >
          ExactOrdinaryCellClosureBudget::
              trusted_maximum_simultaneous_addition_count) {
    throw std::invalid_argument(
        "a Phase 8 ordinary-cell closure budget exceeds its trusted cap");
  }
}

[[nodiscard]] ExactOrdinaryCellClosureRequirements requirements_for(
    std::size_t point_count,
    std::size_t requested_count,
    std::size_t effective_count) {
  const std::size_t complete_count = point_count - 1U;
  ExactOrdinaryCellClosureRequirements result;
  result.point_count = point_count;
  result.complete_competitor_count = complete_count;
  result.requested_initial_competitor_count = requested_count;
  result.effective_initial_competitor_count = effective_count;
  result.conservative_cell_construction_count =
      complete_count - effective_count + 1U;
  for (std::size_t candidate_count = effective_count;
       candidate_count <= complete_count;
       ++candidate_count) {
    const std::size_t boundary_count = 6U + candidate_count;
    const std::size_t triple_count = choose_three(boundary_count);
    result.conservative_cumulative_plane_triple_count += triple_count;
    result.conservative_cumulative_vertex_count += triple_count;
    result.conservative_cumulative_incidence_count +=
        boundary_count * triple_count;
    result.conservative_vertex_query_count += triple_count;
  }
  result.conservative_exact_distance_evaluation_count =
      point_count * result.conservative_vertex_query_count;
  result.conservative_nearest_shell_entry_count =
      result.conservative_exact_distance_evaluation_count;
  result.conservative_owner_strict_feasibility_test_count = complete_count;
  result.conservative_simultaneous_addition_count =
      complete_count - effective_count;

  if (result.conservative_cell_construction_count >
          ExactOrdinaryCellClosureBudget::
              trusted_maximum_cell_construction_count ||
      result.conservative_cumulative_plane_triple_count >
          ExactOrdinaryCellClosureBudget::
              trusted_maximum_cumulative_plane_triple_count ||
      result.conservative_cumulative_vertex_count >
          ExactOrdinaryCellClosureBudget::
              trusted_maximum_cumulative_vertex_count ||
      result.conservative_cumulative_incidence_count >
          ExactOrdinaryCellClosureBudget::
              trusted_maximum_cumulative_incidence_count ||
      result.conservative_vertex_query_count >
          ExactOrdinaryCellClosureBudget::
              trusted_maximum_vertex_query_count ||
      result.conservative_exact_distance_evaluation_count >
          ExactOrdinaryCellClosureBudget::
              trusted_maximum_exact_distance_evaluation_count ||
      result.conservative_nearest_shell_entry_count >
          ExactOrdinaryCellClosureBudget::
              trusted_maximum_nearest_shell_entry_count ||
      result.conservative_owner_strict_feasibility_test_count >
          ExactOrdinaryCellClosureBudget::
              trusted_maximum_owner_strict_feasibility_test_count ||
      result.conservative_simultaneous_addition_count >
          ExactOrdinaryCellClosureBudget::
              trusted_maximum_simultaneous_addition_count) {
    throw std::logic_error(
        "the Phase 8 ordinary-cell closure requirement exceeded its proof "
        "cap");
  }
  return result;
}

[[nodiscard]] bool budget_covers(
    const ExactOrdinaryCellClosureBudget& budget,
    const ExactOrdinaryCellClosureRequirements& requirements) {
  return budget.maximum_cell_construction_count >=
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
             requirements.conservative_owner_strict_feasibility_test_count &&
         budget.maximum_simultaneous_addition_count >=
             requirements.conservative_simultaneous_addition_count;
}

[[nodiscard]] ValidatedOrdinaryCellInput validate_input(
    const CanonicalPointCloud& cloud,
    PointId owner_id,
    std::span<const PointId> initial_competitor_ids) {
  const std::size_t point_count = cloud.size();
  if (point_count == 0U ||
      point_count >
          ExactOrdinaryCellClosureBudget::trusted_maximum_point_count) {
    throw std::invalid_argument(
        "the bounded Phase 8 ordinary-cell closure accepts one to eight "
        "canonical points");
  }
  if (!std::in_range<std::size_t>(owner_id) ||
      static_cast<std::size_t>(owner_id) >= point_count) {
    throw std::out_of_range(
        "an ordinary-cell owner is outside the canonical point cloud");
  }

  ValidatedOrdinaryCellInput result;
  result.complete_competitor_ids.reserve(point_count - 1U);
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    const PointId point_id = static_cast<PointId>(point_index);
    if (point_id != owner_id) {
      result.complete_competitor_ids.push_back(point_id);
    }
  }
  if (initial_competitor_ids.size() >
      result.complete_competitor_ids.size()) {
    throw std::invalid_argument(
        "an ordinary-cell initial competitor list exceeds the complete "
        "table");
  }
  result.canonical_requested_initial_competitor_ids.assign(
      initial_competitor_ids.begin(), initial_competitor_ids.end());
  std::sort(
      result.canonical_requested_initial_competitor_ids.begin(),
      result.canonical_requested_initial_competitor_ids.end());
  if (!strictly_increasing_ids(
          result.canonical_requested_initial_competitor_ids) &&
      !result.canonical_requested_initial_competitor_ids.empty()) {
    throw std::invalid_argument(
        "ordinary-cell initial competitor ids must be unique");
  }
  for (const PointId competitor_id :
       result.canonical_requested_initial_competitor_ids) {
    if (!std::binary_search(
            result.complete_competitor_ids.begin(),
            result.complete_competitor_ids.end(),
            competitor_id)) {
      throw std::invalid_argument(
          "every ordinary-cell initial id must name an authentic exterior "
          "competitor");
    }
  }
  result.canonical_effective_initial_competitor_ids =
      result.canonical_requested_initial_competitor_ids;
  if (result.canonical_effective_initial_competitor_ids.empty() &&
      !result.complete_competitor_ids.empty()) {
    result.canonical_effective_initial_competitor_ids.push_back(
        result.complete_competitor_ids.front());
    result.fallback_seed_injected = true;
  }
  result.requirements = requirements_for(
      point_count,
      result.canonical_requested_initial_competitor_ids.size(),
      result.canonical_effective_initial_competitor_ids.size());
  return result;
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
        "an ordinary-cell closure requires a complete freshly verified "
        "Phase 8 clipping box");
  }
}

[[nodiscard]] std::vector<Binary64WeightedSite3> ordinary_sites(
    const CanonicalPointCloud& cloud) {
  std::vector<Binary64WeightedSite3> result;
  result.reserve(cloud.size());
  for (std::size_t point_index = 0U;
       point_index < cloud.size();
       ++point_index) {
    const PointId point_id = static_cast<PointId>(point_index);
    result.push_back(Binary64WeightedSite3::from_binary64_bits(
        point_id,
        cloud.point(point_id).canonical_input_bits(),
        0U));
  }
  return result;
}

[[nodiscard]] std::vector<Binary64WeightedSite3> selected_sites(
    const std::vector<Binary64WeightedSite3>& all_sites,
    const std::vector<PointId>& ids) {
  std::vector<Binary64WeightedSite3> result;
  result.reserve(ids.size());
  for (const PointId point_id : ids) {
    result.push_back(all_sites[static_cast<std::size_t>(point_id)]);
  }
  return result;
}

[[nodiscard]] bool artificial_boundaries_are_canonical(
    const ExactPowerCellReferenceResult& cell) {
  const std::array<PowerCellBoundaryKind, 6> expected{
      PowerCellBoundaryKind::box_lower_x,
      PowerCellBoundaryKind::box_upper_x,
      PowerCellBoundaryKind::box_lower_y,
      PowerCellBoundaryKind::box_upper_y,
      PowerCellBoundaryKind::box_lower_z,
      PowerCellBoundaryKind::box_upper_z};
  if (cell.boundary_planes.size() < expected.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < expected.size(); ++index) {
    if (cell.boundary_planes[index].kind != expected[index] ||
        cell.boundary_planes[index].competitor_id.has_value()) {
      return false;
    }
  }
  for (std::size_t index = expected.size();
       index < cell.boundary_planes.size();
       ++index) {
    if (cell.boundary_planes[index].kind !=
            PowerCellBoundaryKind::power_bisector ||
        !cell.boundary_planes[index].competitor_id.has_value()) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool owner_is_strictly_feasible(
    const std::vector<Binary64WeightedSite3>& all_sites,
    PointId owner_id) {
  const Binary64WeightedSite3& owner =
      all_sites[static_cast<std::size_t>(owner_id)];
  for (const Binary64WeightedSite3& competitor : all_sites) {
    if (competitor.point_id() == owner_id) {
      continue;
    }
    const ExactPowerBisectorConstraint constraint =
        make_exact_power_bisector_constraint(owner, competitor);
    if (constraint.kind != PowerBisectorConstraintKind::proper_halfspace ||
        constraint.owner_minus_competitor
                .evaluate(owner.position())
                .sign() >= 0) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool audit_within_requirements(
    const ExactOrdinaryCellClosureAudit& audit,
    const ExactOrdinaryCellClosureRequirements& requirements) {
  return audit.exact_cell_construction_count <=
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
         audit.simultaneously_added_competitor_count <=
             requirements.conservative_simultaneous_addition_count;
}

[[nodiscard]] ExactOrdinaryCellClosureResult build_unverified(
    const CanonicalPointCloud& cloud,
    PointId owner_id,
    const StrictlyPaddedDyadicAabb3Result& clipping_box,
    std::span<const PointId> initial_competitor_ids,
    ExactOrdinaryCellClosureBudget budget) {
  validate_budget(budget);
  const ValidatedOrdinaryCellInput validated =
      validate_input(cloud, owner_id, initial_competitor_ids);
  require_complete_clipping_box(cloud, clipping_box);

  ExactOrdinaryCellClosureResult result;
  result.owner_id = owner_id;
  result.requirements = validated.requirements;
  result.clipping_box = clipping_box;
  result.complete_competitor_ids = validated.complete_competitor_ids;
  result.canonical_requested_initial_competitor_ids =
      validated.canonical_requested_initial_competitor_ids;
  result.canonical_effective_initial_competitor_ids =
      validated.canonical_effective_initial_competitor_ids;
  result.fallback_seed_injected = validated.fallback_seed_injected;
  if (!budget_covers(budget, result.requirements)) {
    return result;
  }

  const std::vector<Binary64WeightedSite3> all_sites =
      ordinary_sites(cloud);
  const Binary64WeightedSite3& owner =
      all_sites[static_cast<std::size_t>(owner_id)];
  const std::array<PointId, 0> no_exclusions{};
  const ExclusionSet exclusions = ExclusionSet::from_ids(
      std::span<const PointId>{no_exclusions}, cloud, 0U);
  std::vector<PointId> candidates =
      result.canonical_effective_initial_competitor_ids;

  for (std::size_t round_index = 0U;
       round_index < result.requirements
                         .conservative_cell_construction_count;
       ++round_index) {
    ExactOrdinaryCellClosureRound round;
    round.round_index = round_index;
    round.canonical_candidate_competitor_ids = candidates;
    const std::vector<Binary64WeightedSite3> candidate_sites =
        selected_sites(all_sites, candidates);
    round.candidate_cell = build_exact_bounded_power_cell_reference(
        owner,
        candidate_sites,
        clipping_box.certificate->omega);
    if (round.candidate_cell.decision !=
        ExactPowerCellReferenceDecision::complete_nonempty) {
      throw std::logic_error(
          "an ordinary Voronoi owner failed to retain a nonempty candidate "
          "cell");
    }

    ++result.audit.exact_cell_construction_count;
    result.audit.cumulative_plane_triple_capacity +=
        round.candidate_cell.requirements
            .conservative_plane_triple_count;
    result.audit.cumulative_vertex_capacity +=
        round.candidate_cell.requirements.conservative_vertex_count;
    result.audit.cumulative_incidence_capacity +=
        round.candidate_cell.requirements.conservative_incidence_count;
    round.vertex_queries.reserve(round.candidate_cell.vertices.size());
    std::vector<PointId> simultaneous_additions;
    for (std::size_t vertex_index = 0U;
         vertex_index < round.candidate_cell.vertices.size();
         ++vertex_index) {
      const TopKPartition nearest = brute_force_nearest(
          cloud,
          round.candidate_cell.vertices[vertex_index].position,
          exclusions);
      if (!nearest.shell_complete() ||
          nearest.distance_evaluation_count() != cloud.size()) {
        throw std::logic_error(
            "the Phase 8 vertex query did not return a complete global "
            "nearest shell");
      }
      ExactOrdinaryCellVertexQueryRecord record;
      record.candidate_vertex_index = vertex_index;
      record.nearest_squared_distance = nearest.cutoff_squared_distance();
      record.complete_nearest_shell_ids.assign(
          nearest.cutoff_shell_ids().begin(),
          nearest.cutoff_shell_ids().end());
      const bool owner_is_nearest = std::binary_search(
          record.complete_nearest_shell_ids.begin(),
          record.complete_nearest_shell_ids.end(),
          owner_id);
      for (const PointId shell_id : record.complete_nearest_shell_ids) {
        if (shell_id != owner_id &&
            !std::binary_search(candidates.begin(), candidates.end(), shell_id)) {
          record.newly_required_competitor_ids.push_back(shell_id);
          simultaneous_additions.push_back(shell_id);
        }
      }

      if (!owner_is_nearest) {
        if (record.newly_required_competitor_ids.size() !=
            record.complete_nearest_shell_ids.size()) {
          throw std::logic_error(
              "a retained ordinary-cell constraint was violated at a "
              "candidate vertex");
        }
        record.classification =
            ExactOrdinaryCellVertexClassification::
                violating_nearest_shell;
        ++result.audit.violating_vertex_count;
      } else if (record.complete_nearest_shell_ids.size() == 1U) {
        record.classification =
            ExactOrdinaryCellVertexClassification::owner_strict_nearest;
        ++result.audit.owner_strict_vertex_count;
      } else if (!record.newly_required_competitor_ids.empty()) {
        record.classification =
            ExactOrdinaryCellVertexClassification::
                missing_active_nearest_shell;
        ++result.audit.owner_tie_vertex_count;
        ++result.audit.missing_active_vertex_count;
      } else {
        record.classification =
            ExactOrdinaryCellVertexClassification::
                reconciled_active_nearest_shell;
        ++result.audit.owner_tie_vertex_count;
      }
      ++result.audit.exact_vertex_query_count;
      result.audit.exact_distance_evaluation_count +=
          nearest.distance_evaluation_count();
      result.audit.nearest_shell_entry_count +=
          record.complete_nearest_shell_ids.size();
      round.vertex_queries.push_back(std::move(record));
    }

    std::sort(simultaneous_additions.begin(), simultaneous_additions.end());
    simultaneous_additions.erase(
        std::unique(
            simultaneous_additions.begin(),
            simultaneous_additions.end()),
        simultaneous_additions.end());
    round.simultaneously_added_competitor_ids = simultaneous_additions;
    round.local_queue_empty = simultaneous_additions.empty();
    result.rounds.push_back(std::move(round));

    if (simultaneous_additions.empty()) {
      result.canonical_closed_competitor_ids = candidates;
      result.audit.owner_strict_feasibility_test_count =
          result.complete_competitor_ids.size();
      if (!owner_is_strictly_feasible(all_sites, owner_id)) {
        throw std::logic_error(
            "a canonical ordinary-cell owner is not strictly feasible");
      }
      result.local_queue_empty_certified = true;
      result.owner_strict_feasible_certified = true;
      result.full_dimensional_nonempty_certified = true;
      result.active_nearest_shells_reconciled_certified = true;
      result.artificial_box_boundaries_certified =
          artificial_boundaries_are_canonical(
              result.rounds.back().candidate_cell);
      if (!result.artificial_box_boundaries_certified ||
          !audit_within_requirements(result.audit, result.requirements)) {
        throw std::logic_error(
            "the completed Phase 8 ordinary-cell closure failed its local "
            "certificate invariants");
      }
      result.decision =
          ExactOrdinaryCellClosureDecision::complete_nonempty;
      return result;
    }

    ++result.audit.simultaneous_addition_batch_count;
    result.audit.simultaneously_added_competitor_count +=
        simultaneous_additions.size();
    result.audit.maximum_simultaneous_addition_count = std::max(
        result.audit.maximum_simultaneous_addition_count,
        simultaneous_additions.size());
    std::vector<PointId> merged;
    merged.reserve(candidates.size() + simultaneous_additions.size());
    std::set_union(
        candidates.begin(),
        candidates.end(),
        simultaneous_additions.begin(),
        simultaneous_additions.end(),
        std::back_inserter(merged));
    if (merged.size() <= candidates.size() ||
        merged.size() > result.complete_competitor_ids.size()) {
      throw std::logic_error(
          "a nonterminal ordinary-cell round did not strictly grow its "
          "candidate set");
    }
    candidates = std::move(merged);
  }
  throw std::logic_error(
      "the bounded ordinary-cell loop exhausted its proved round count");
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

[[nodiscard]] std::string position_key(
    const exact::ExactRational3& position) {
  return position.coordinate(0U).canonical_key() + ":" +
         position.coordinate(1U).canonical_key() + ":" +
         position.coordinate(2U).canonical_key();
}

[[nodiscard]] std::optional<std::vector<SemanticVertexSignature>>
semantic_vertex_signatures(const ExactPowerCellReferenceResult& cell) {
  std::vector<SemanticVertexSignature> result;
  result.reserve(cell.vertices.size());
  for (const ExactPowerCellVertex& vertex : cell.vertices) {
    SemanticVertexSignature signature;
    signature.position_key = position_key(vertex.position);
    for (const std::size_t boundary_index :
         vertex.active_boundary_plane_indices) {
      if (boundary_index >= cell.boundary_planes.size()) {
        return std::nullopt;
      }
      const ExactPowerCellBoundaryPlane& boundary =
          cell.boundary_planes[boundary_index];
      const std::optional<std::uint8_t> face_bit =
          artificial_face_bit(boundary.kind);
      if (face_bit.has_value()) {
        if (boundary.competitor_id.has_value()) {
          return std::nullopt;
        }
        signature.artificial_box_face_mask =
            static_cast<std::uint8_t>(
                signature.artificial_box_face_mask | *face_bit);
      } else {
        if (boundary.kind != PowerCellBoundaryKind::power_bisector ||
            !boundary.competitor_id.has_value()) {
          return std::nullopt;
        }
        signature.active_competitor_ids.push_back(
            *boundary.competitor_id);
      }
    }
    std::sort(
        signature.active_competitor_ids.begin(),
        signature.active_competitor_ids.end());
    if (!strictly_increasing_ids(signature.active_competitor_ids) &&
        !signature.active_competitor_ids.empty()) {
      return std::nullopt;
    }
    result.push_back(std::move(signature));
  }
  std::sort(result.begin(), result.end());
  return result;
}

[[nodiscard]] bool final_shells_match_active_incidences(
    const CanonicalPointCloud& cloud,
    PointId owner_id,
    const ExactPowerCellReferenceResult& cell) {
  const std::optional<std::vector<SemanticVertexSignature>> signatures =
      semantic_vertex_signatures(cell);
  if (!signatures.has_value() ||
      signatures->size() != cell.vertices.size()) {
    return false;
  }
  const std::array<PointId, 0> no_exclusions{};
  const ExclusionSet exclusions = ExclusionSet::from_ids(
      std::span<const PointId>{no_exclusions}, cloud, 0U);
  for (const ExactPowerCellVertex& vertex : cell.vertices) {
    const TopKPartition nearest =
        brute_force_nearest(cloud, vertex.position, exclusions);
    const std::vector<PointId> shell{
        nearest.cutoff_shell_ids().begin(),
        nearest.cutoff_shell_ids().end()};
    if (!std::binary_search(shell.begin(), shell.end(), owner_id)) {
      return false;
    }
    std::vector<PointId> expected_active_ids;
    for (const PointId shell_id : shell) {
      if (shell_id != owner_id) {
        expected_active_ids.push_back(shell_id);
      }
    }
    const std::string key = position_key(vertex.position);
    const auto signature = std::find_if(
        signatures->begin(),
        signatures->end(),
        [&key](const SemanticVertexSignature& candidate) {
          return candidate.position_key == key;
        });
    if (signature == signatures->end() ||
        signature->active_competitor_ids != expected_active_ids) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool monotone_transcript_shape(
    const ExactOrdinaryCellClosureResult& result) {
  if (result.decision ==
      ExactOrdinaryCellClosureDecision::insufficient_budget) {
    return result.rounds.empty() &&
           result.canonical_closed_competitor_ids.empty() &&
           !result.local_queue_empty_certified &&
           !result.owner_strict_feasible_certified &&
           !result.full_dimensional_nonempty_certified &&
           !result.active_nearest_shells_reconciled_certified &&
           !result.artificial_box_boundaries_certified;
  }
  if (result.rounds.empty()) {
    return false;
  }
  std::vector<PointId> current =
      result.canonical_effective_initial_competitor_ids;
  if (!current.empty() && !strictly_increasing_ids(current)) {
    return false;
  }
  for (std::size_t round_index = 0U;
       round_index < result.rounds.size();
       ++round_index) {
    const ExactOrdinaryCellClosureRound& round =
        result.rounds[round_index];
    if (round.round_index != round_index ||
        round.canonical_candidate_competitor_ids != current ||
        round.candidate_cell.decision !=
            ExactPowerCellReferenceDecision::complete_nonempty ||
        round.vertex_queries.size() !=
            round.candidate_cell.vertices.size() ||
        round.local_queue_empty !=
            round.simultaneously_added_competitor_ids.empty() ||
        (!strictly_increasing_ids(
             round.simultaneously_added_competitor_ids) &&
         !round.simultaneously_added_competitor_ids.empty())) {
      return false;
    }
    std::vector<PointId> observed_additions;
    for (std::size_t vertex_index = 0U;
         vertex_index < round.vertex_queries.size();
         ++vertex_index) {
      const ExactOrdinaryCellVertexQueryRecord& query =
          round.vertex_queries[vertex_index];
      if (query.candidate_vertex_index != vertex_index ||
          query.complete_nearest_shell_ids.empty() ||
          !strictly_increasing_ids(query.complete_nearest_shell_ids) ||
          (!query.newly_required_competitor_ids.empty() &&
           !strictly_increasing_ids(
               query.newly_required_competitor_ids))) {
        return false;
      }
      const bool owner_is_nearest = std::binary_search(
          query.complete_nearest_shell_ids.begin(),
          query.complete_nearest_shell_ids.end(),
          result.owner_id);
      std::vector<PointId> expected_new;
      for (const PointId shell_id : query.complete_nearest_shell_ids) {
        if (shell_id != result.owner_id &&
            !std::binary_search(current.begin(), current.end(), shell_id)) {
          expected_new.push_back(shell_id);
        }
      }
      ExactOrdinaryCellVertexClassification expected_classification;
      if (!owner_is_nearest) {
        expected_classification =
            ExactOrdinaryCellVertexClassification::
                violating_nearest_shell;
        if (expected_new.size() !=
            query.complete_nearest_shell_ids.size()) {
          return false;
        }
      } else if (query.complete_nearest_shell_ids.size() == 1U) {
        expected_classification =
            ExactOrdinaryCellVertexClassification::owner_strict_nearest;
      } else if (!expected_new.empty()) {
        expected_classification =
            ExactOrdinaryCellVertexClassification::
                missing_active_nearest_shell;
      } else {
        expected_classification =
            ExactOrdinaryCellVertexClassification::
                reconciled_active_nearest_shell;
      }
      if (query.newly_required_competitor_ids != expected_new ||
          query.classification != expected_classification) {
        return false;
      }
      observed_additions.insert(
          observed_additions.end(), expected_new.begin(), expected_new.end());
    }
    std::sort(observed_additions.begin(), observed_additions.end());
    observed_additions.erase(
        std::unique(observed_additions.begin(), observed_additions.end()),
        observed_additions.end());
    if (observed_additions !=
        round.simultaneously_added_competitor_ids) {
      return false;
    }
    const bool is_last = round_index + 1U == result.rounds.size();
    if (is_last != round.local_queue_empty) {
      return false;
    }
    if (!is_last) {
      std::vector<PointId> merged;
      std::set_union(
          current.begin(),
          current.end(),
          observed_additions.begin(),
          observed_additions.end(),
          std::back_inserter(merged));
      if (merged.size() <= current.size()) {
        return false;
      }
      current = std::move(merged);
    }
  }
  return current == result.canonical_closed_competitor_ids &&
         result.local_queue_empty_certified &&
         result.owner_strict_feasible_certified &&
         result.full_dimensional_nonempty_certified &&
         result.active_nearest_shells_reconciled_certified &&
         result.artificial_box_boundaries_certified;
}

}  // namespace

std::string_view to_string(
    ExactOrdinaryCellVertexClassification classification) {
  switch (classification) {
    case ExactOrdinaryCellVertexClassification::owner_strict_nearest:
      return "owner_strict_nearest";
    case ExactOrdinaryCellVertexClassification::violating_nearest_shell:
      return "violating_nearest_shell";
    case ExactOrdinaryCellVertexClassification::
        missing_active_nearest_shell:
      return "missing_active_nearest_shell";
    case ExactOrdinaryCellVertexClassification::
        reconciled_active_nearest_shell:
      return "reconciled_active_nearest_shell";
  }
  throw std::invalid_argument(
      "an ordinary-cell vertex classification is invalid");
}

std::string_view to_string(ExactOrdinaryCellClosureDecision decision) {
  switch (decision) {
    case ExactOrdinaryCellClosureDecision::complete_nonempty:
      return "complete_nonempty";
    case ExactOrdinaryCellClosureDecision::insufficient_budget:
      return "insufficient_budget";
  }
  throw std::invalid_argument(
      "an ordinary-cell closure decision is invalid");
}

ExactOrdinaryCellClosureVerification
verify_exact_bounded_ordinary_cell_closure(
    const CanonicalPointCloud& cloud,
    PointId owner_id,
    const StrictlyPaddedDyadicAabb3Result& clipping_box,
    std::span<const PointId> initial_competitor_ids,
    const ExactOrdinaryCellClosureResult& result,
    ExactOrdinaryCellClosureBudget budget) {
  const ExactOrdinaryCellClosureResult expected = build_unverified(
      cloud,
      owner_id,
      clipping_box,
      initial_competitor_ids,
      budget);
  ExactOrdinaryCellClosureVerification verification;
  verification.input_identity_certified =
      result.owner_id == expected.owner_id &&
      result.complete_competitor_ids == expected.complete_competitor_ids &&
      result.canonical_requested_initial_competitor_ids ==
          expected.canonical_requested_initial_competitor_ids &&
      result.canonical_effective_initial_competitor_ids ==
          expected.canonical_effective_initial_competitor_ids &&
      result.fallback_seed_injected == expected.fallback_seed_injected;
  verification.clipping_box_certified =
      result.clipping_box == clipping_box &&
      verify_strictly_padded_dyadic_aabb(cloud, result.clipping_box)
          .result_certified;
  verification.decision_certified = result.decision == expected.decision;
  verification.requirements_certified =
      result.requirements == expected.requirements;
  verification.audit_certified = result.audit == expected.audit;
  verification.payload_shape_certified =
      result.decision == ExactOrdinaryCellClosureDecision::complete_nonempty
          ? result.final_cell() != nullptr && !result.rounds.empty()
          : result.final_cell() == nullptr && result.rounds.empty() &&
                result.canonical_closed_competitor_ids.empty();
  verification.transcript_replay_certified = result == expected;
  if (!verification.transcript_replay_certified) {
    return verification;
  }
  verification.monotone_queue_certified = monotone_transcript_shape(result);

  if (result.decision ==
      ExactOrdinaryCellClosureDecision::insufficient_budget) {
    verification.result_certified =
        verification.input_identity_certified &&
        verification.clipping_box_certified &&
        verification.decision_certified &&
        verification.requirements_certified &&
        verification.audit_certified &&
        verification.payload_shape_certified &&
        verification.transcript_replay_certified &&
        verification.monotone_queue_certified;
    return verification;
  }

  const ExactPowerCellReferenceResult* final_cell = result.final_cell();
  if (final_cell == nullptr) {
    return verification;
  }
  const std::vector<Binary64WeightedSite3> all_sites =
      ordinary_sites(cloud);
  verification.owner_strict_feasible_certified =
      result.owner_strict_feasible_certified &&
      owner_is_strictly_feasible(all_sites, owner_id);
  verification.full_dimensional_nonempty_certified =
      result.full_dimensional_nonempty_certified &&
      final_cell->decision ==
          ExactPowerCellReferenceDecision::complete_nonempty &&
      verification.owner_strict_feasible_certified;
  verification.artificial_box_boundaries_certified =
      result.artificial_box_boundaries_certified &&
      artificial_boundaries_are_canonical(*final_cell);
  verification.active_nearest_shells_reconciled_certified =
      result.active_nearest_shells_reconciled_certified &&
      final_shells_match_active_incidences(cloud, owner_id, *final_cell);

  const std::vector<Binary64WeightedSite3> complete_competitors =
      selected_sites(all_sites, expected.complete_competitor_ids);
  const ExactPowerCellReferenceResult complete_oracle =
      build_exact_bounded_power_cell_reference(
          all_sites[static_cast<std::size_t>(owner_id)],
          complete_competitors,
          clipping_box.certificate->omega);
  const std::optional<std::vector<SemanticVertexSignature>> final_projection =
      semantic_vertex_signatures(*final_cell);
  const std::optional<std::vector<SemanticVertexSignature>> oracle_projection =
      semantic_vertex_signatures(complete_oracle);
  verification.complete_oracle_projection_certified =
      complete_oracle.decision ==
              ExactPowerCellReferenceDecision::complete_nonempty &&
      final_projection.has_value() && oracle_projection.has_value() &&
      *final_projection == *oracle_projection;
  verification.result_certified =
      verification.input_identity_certified &&
      verification.clipping_box_certified &&
      verification.decision_certified &&
      verification.requirements_certified &&
      verification.audit_certified &&
      verification.payload_shape_certified &&
      verification.transcript_replay_certified &&
      verification.monotone_queue_certified &&
      verification.owner_strict_feasible_certified &&
      verification.full_dimensional_nonempty_certified &&
      verification.active_nearest_shells_reconciled_certified &&
      verification.artificial_box_boundaries_certified &&
      verification.complete_oracle_projection_certified;
  return verification;
}

ExactOrdinaryCellClosureResult build_exact_bounded_ordinary_cell_closure(
    const CanonicalPointCloud& cloud,
    PointId owner_id,
    const StrictlyPaddedDyadicAabb3Result& clipping_box,
    std::span<const PointId> initial_competitor_ids,
    ExactOrdinaryCellClosureBudget budget) {
  ExactOrdinaryCellClosureResult result = build_unverified(
      cloud,
      owner_id,
      clipping_box,
      initial_competitor_ids,
      budget);
  const ExactOrdinaryCellClosureVerification verification =
      verify_exact_bounded_ordinary_cell_closure(
          cloud,
          owner_id,
          clipping_box,
          initial_competitor_ids,
          result,
          budget);
  if (!verification.result_certified) {
    throw std::logic_error(
        "the Phase 8 ordinary-cell closure failed its fresh verification");
  }
  return result;
}

}  // namespace morsehgp3d::spatial
