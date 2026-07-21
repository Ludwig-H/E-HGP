#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/spatial/ordinary_diagram_closure.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"
#include "morsehgp3d/spatial/point_cloud_aabb.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactOrdinaryCellClosureDecision;
using morsehgp3d::spatial::ExactOrdinaryCellClosureResult;
using morsehgp3d::spatial::ExactOrdinaryCellClosureRound;
using morsehgp3d::spatial::ExactOrdinaryCellVertexQueryRecord;
using morsehgp3d::spatial::ExactOrdinaryDiagramClosureDecision;
using morsehgp3d::spatial::ExactOrdinaryDiagramClosureResult;
using morsehgp3d::spatial::ExactOrdinaryDiagramContact;
using morsehgp3d::spatial::ExactOrdinaryDiagramVertex;
using morsehgp3d::spatial::ExactPowerCellReferenceDecision;
using morsehgp3d::spatial::ExactPowerCellReferenceResult;
using morsehgp3d::spatial::ExactPowerCellVertex;
using morsehgp3d::spatial::PointId;
using morsehgp3d::spatial::PowerCellBoundaryKind;
using morsehgp3d::spatial::StrictDyadicPaddingDecision;
using morsehgp3d::spatial::StrictlyPaddedDyadicAabb3Result;

constexpr std::string_view input_protocol_header =
    "morsehgp3d-phase8-ordinary-diagram-input-v1";
constexpr std::string_view output_protocol_header =
    "morsehgp3d-phase8-ordinary-diagram-output-v1";

struct InputCase {
  std::string name;
  std::vector<CertifiedPoint3> input_points;
};

struct FinalCellView {
  const ExactOrdinaryCellClosureResult* closure{};
  const ExactPowerCellReferenceResult* geometry{};
  const ExactOrdinaryCellClosureRound* final_round{};
  std::vector<const ExactOrdinaryCellVertexQueryRecord*> query_by_vertex;
};

[[nodiscard]] bool is_case_character(char character) noexcept {
  return (character >= 'A' && character <= 'Z') ||
         (character >= 'a' && character <= 'z') ||
         (character >= '0' && character <= '9') || character == '_' ||
         character == '-';
}

void require_case_name(std::string_view name) {
  if (name.empty() ||
      !std::all_of(name.begin(), name.end(), is_case_character)) {
    throw std::invalid_argument(
        "case must match the non-empty pattern [A-Za-z0-9_-]+");
  }
}

[[nodiscard]] CertifiedPoint3 parse_point(std::string_view payload) {
  const std::size_t first_comma = payload.find(',');
  if (first_comma == std::string_view::npos) {
    throw std::invalid_argument(
        "each point must contain three comma-separated binary64 words");
  }
  const std::size_t second_comma = payload.find(',', first_comma + 1U);
  if (second_comma == std::string_view::npos ||
      payload.find(',', second_comma + 1U) != std::string_view::npos) {
    throw std::invalid_argument(
        "each point must contain exactly three binary64 words");
  }

  const std::array<std::string_view, 3> words{
      payload.substr(0U, first_comma),
      payload.substr(first_comma + 1U, second_comma - first_comma - 1U),
      payload.substr(second_comma + 1U)};
  std::array<std::uint64_t, 3> bits{};
  for (std::size_t axis = 0U; axis < bits.size(); ++axis) {
    bits[axis] = morsehgp3d::exact::parse_binary64_hex(words[axis], false);
  }
  return CertifiedPoint3::from_binary64_bits(bits);
}

[[nodiscard]] InputCase parse_case(std::string_view line) {
  const std::size_t separator = line.find('|');
  if (separator == std::string_view::npos ||
      line.find('|', separator + 1U) != std::string_view::npos) {
    throw std::invalid_argument(
        "a case line must contain exactly one '|' separator");
  }

  InputCase result;
  result.name = std::string{line.substr(0U, separator)};
  require_case_name(result.name);

  const std::string_view point_payload = line.substr(separator + 1U);
  if (point_payload.empty()) {
    throw std::invalid_argument("a case must contain at least one point");
  }

  std::size_t begin = 0U;
  while (begin < point_payload.size()) {
    const std::size_t end = point_payload.find(';', begin);
    const std::size_t length =
        end == std::string_view::npos ? point_payload.size() - begin
                                      : end - begin;
    if (length == 0U) {
      throw std::invalid_argument("empty point records are forbidden");
    }
    result.input_points.push_back(
        parse_point(point_payload.substr(begin, length)));
    if (result.input_points.size() >
        morsehgp3d::spatial::ExactOrdinaryDiagramClosureBudget::
            trusted_maximum_point_count) {
      throw std::invalid_argument(
          "an ordinary-diagram dump case may contain at most eight points");
    }
    if (end == std::string_view::npos) {
      break;
    }
    begin = end + 1U;
    if (begin == point_payload.size()) {
      throw std::invalid_argument("a trailing point separator is forbidden");
    }
  }
  return result;
}

[[nodiscard]] bool valid_point_id(
    PointId point_id, std::size_t point_count) noexcept {
  return point_id < static_cast<PointId>(point_count);
}

void require_strict_point_ids(
    std::span<const PointId> point_ids,
    std::size_t point_count,
    std::string_view field_name) {
  for (std::size_t index = 0U; index < point_ids.size(); ++index) {
    if (!valid_point_id(point_ids[index], point_count)) {
      throw std::logic_error(
          std::string{field_name} + " contains an invalid PointId");
    }
    if (index != 0U && point_ids[index - 1U] >= point_ids[index]) {
      throw std::logic_error(
          std::string{field_name} + " is not strictly increasing");
    }
  }
}

void require_strict_indices(
    std::span<const std::size_t> indices,
    std::size_t upper_bound,
    std::string_view field_name) {
  for (std::size_t index = 0U; index < indices.size(); ++index) {
    if (indices[index] >= upper_bound) {
      throw std::logic_error(
          std::string{field_name} + " contains an invalid index");
    }
    if (index != 0U && indices[index - 1U] >= indices[index]) {
      throw std::logic_error(
          std::string{field_name} + " is not strictly increasing");
    }
  }
}

void require_full_box_verification(
    const CanonicalPointCloud& cloud,
    const StrictlyPaddedDyadicAabb3Result& clipping_box) {
  const auto verification =
      morsehgp3d::spatial::verify_strictly_padded_dyadic_aabb(
          cloud, clipping_box);
  if (!verification.audit_certified || !verification.decision_certified ||
      !verification.failure_masks_certified ||
      !verification.payload_shape_certified ||
      !verification.exact_extrema_certified ||
      !verification.extremum_witnesses_certified ||
      !verification.finite_adjacent_padding_certified ||
      !verification.exact_positive_padding_certified ||
      !verification.all_sites_strictly_inside_certified ||
      !verification.convex_hull_strictly_inside_certified ||
      !verification.result_certified ||
      clipping_box.decision != StrictDyadicPaddingDecision::complete ||
      !clipping_box.certificate.has_value()) {
    throw std::logic_error(
        "the freshly constructed Phase 8.1 clipping box is not fully "
        "certified");
  }
}

void require_full_diagram_verification(
    const CanonicalPointCloud& cloud,
    const StrictlyPaddedDyadicAabb3Result& clipping_box,
    const ExactOrdinaryDiagramClosureResult& result) {
  const auto verification =
      morsehgp3d::spatial::verify_exact_bounded_ordinary_diagram_closure(
          cloud, clipping_box, result);
  if (!verification.input_identity_certified ||
      !verification.clipping_box_certified ||
      !verification.decision_certified ||
      !verification.requirements_certified || !verification.audit_certified ||
      !verification.payload_shape_certified ||
      !verification.transcript_replay_certified ||
      !verification.all_cells_freshly_verified_certified ||
      !verification.all_local_queues_empty_certified ||
      !verification.all_cells_full_dimensional_nonempty_certified ||
      !verification.global_vertex_occurrence_bijection_certified ||
      !verification.natural_incidences_reconciled_certified ||
      !verification.artificial_box_boundaries_certified ||
      !verification.result_certified) {
    throw std::logic_error(
        "the Phase 8.3 ordinary diagram did not pass its full fresh "
        "verification");
  }
}

[[nodiscard]] std::uint8_t box_face_bit(PowerCellBoundaryKind kind) {
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
      return UINT8_C(0);
  }
  throw std::logic_error("an unknown cell boundary kind was dumped");
}

[[nodiscard]] std::uint8_t artificial_box_mask(
    const ExactPowerCellReferenceResult& geometry,
    const ExactPowerCellVertex& vertex,
    std::size_t point_count) {
  require_strict_indices(
      vertex.active_boundary_plane_indices,
      geometry.boundary_planes.size(),
      "cell vertex active boundary indices");
  std::uint8_t mask = UINT8_C(0);
  for (const std::size_t boundary_index :
       vertex.active_boundary_plane_indices) {
    const auto& boundary = geometry.boundary_planes[boundary_index];
    const std::uint8_t bit = box_face_bit(boundary.kind);
    if (bit != UINT8_C(0)) {
      if (boundary.competitor_id.has_value()) {
        throw std::logic_error(
            "an artificial boundary unexpectedly names a competitor");
      }
      mask = static_cast<std::uint8_t>(mask | bit);
      continue;
    }
    if (boundary.kind != PowerCellBoundaryKind::power_bisector ||
        !boundary.competitor_id.has_value() ||
        !valid_point_id(*boundary.competitor_id, point_count)) {
      throw std::logic_error(
          "a natural boundary does not name a valid competitor");
    }
  }
  return mask;
}

[[nodiscard]] std::vector<FinalCellView> validated_final_cells(
    const CanonicalPointCloud& cloud,
    const ExactOrdinaryDiagramClosureResult& result) {
  if (result.cells.size() != cloud.size()) {
    throw std::logic_error(
        "a complete ordinary diagram must contain one cell per owner");
  }

  std::vector<FinalCellView> views(cloud.size());
  std::vector<std::uint8_t> owner_seen(cloud.size(), UINT8_C(0));
  for (const ExactOrdinaryCellClosureResult& closure : result.cells) {
    if (!valid_point_id(closure.owner_id, cloud.size())) {
      throw std::logic_error("a final cell has an invalid owner id");
    }
    const std::size_t owner_index =
        static_cast<std::size_t>(closure.owner_id);
    if (owner_seen[owner_index] != UINT8_C(0)) {
      throw std::logic_error("a final cell owner is duplicated");
    }
    owner_seen[owner_index] = UINT8_C(1);

    const ExactPowerCellReferenceResult* const geometry =
        closure.final_cell();
    if (closure.decision !=
            ExactOrdinaryCellClosureDecision::complete_nonempty ||
        geometry == nullptr || closure.rounds.empty() ||
        geometry->decision !=
            ExactPowerCellReferenceDecision::complete_nonempty) {
      throw std::logic_error("a final ordinary cell is incomplete");
    }
    const ExactOrdinaryCellClosureRound& final_round =
        closure.rounds.back();
    if (final_round.vertex_queries.size() != geometry->vertices.size()) {
      throw std::logic_error(
          "a final ordinary cell has a malformed vertex-query table");
    }

    FinalCellView view;
    view.closure = &closure;
    view.geometry = geometry;
    view.final_round = &final_round;
    view.query_by_vertex.resize(geometry->vertices.size(), nullptr);
    for (const ExactOrdinaryCellVertexQueryRecord& query :
         final_round.vertex_queries) {
      if (query.candidate_vertex_index >= geometry->vertices.size() ||
          view.query_by_vertex[query.candidate_vertex_index] != nullptr) {
        throw std::logic_error(
            "a final ordinary-cell query references an invalid or duplicate "
            "vertex");
      }
      require_strict_point_ids(
          query.complete_nearest_shell_ids,
          cloud.size(),
          "final cell nearest shell");
      view.query_by_vertex[query.candidate_vertex_index] = &query;
    }
    if (std::any_of(
            view.query_by_vertex.begin(),
            view.query_by_vertex.end(),
            [](const ExactOrdinaryCellVertexQueryRecord* query) {
              return query == nullptr;
            })) {
      throw std::logic_error(
          "a final ordinary-cell vertex has no nearest-shell query");
    }
    for (const ExactPowerCellVertex& vertex : geometry->vertices) {
      static_cast<void>(artificial_box_mask(
          *geometry, vertex, cloud.size()));
    }
    views[owner_index] = std::move(view);
  }
  return views;
}

[[nodiscard]] std::vector<std::vector<PointId>>
validated_global_owner_ids(
    const CanonicalPointCloud& cloud,
    const std::vector<FinalCellView>& cells,
    const ExactOrdinaryDiagramClosureResult& result) {
  std::vector<std::vector<std::uint8_t>> local_occurrence_seen;
  local_occurrence_seen.reserve(cells.size());
  for (const FinalCellView& cell : cells) {
    if (cell.geometry == nullptr) {
      throw std::logic_error("an owner has no validated final cell");
    }
    local_occurrence_seen.emplace_back(
        cell.geometry->vertices.size(), UINT8_C(0));
  }

  std::vector<std::vector<PointId>> owner_ids;
  owner_ids.reserve(result.global_vertices.size());
  for (const ExactOrdinaryDiagramVertex& vertex : result.global_vertices) {
    require_strict_point_ids(
        vertex.complete_nearest_shell_ids,
        cloud.size(),
        "global vertex nearest shell");
    if ((vertex.artificial_box_face_mask & UINT8_C(0xc0)) != UINT8_C(0)) {
      throw std::logic_error("a global vertex has an invalid box mask");
    }

    std::vector<PointId> vertex_owner_ids;
    vertex_owner_ids.reserve(vertex.cell_occurrences.size());
    for (const auto& occurrence : vertex.cell_occurrences) {
      if (!valid_point_id(occurrence.owner_id, cloud.size())) {
        throw std::logic_error(
            "a global vertex occurrence has an invalid owner id");
      }
      const std::size_t owner_index =
          static_cast<std::size_t>(occurrence.owner_id);
      const FinalCellView& cell = cells[owner_index];
      if (cell.geometry == nullptr ||
          occurrence.cell_vertex_index >= cell.geometry->vertices.size()) {
        throw std::logic_error(
            "a global vertex occurrence has an invalid local vertex index");
      }
      if (local_occurrence_seen[owner_index][occurrence.cell_vertex_index] !=
          UINT8_C(0)) {
        throw std::logic_error(
            "a local vertex occurs more than once in the global table");
      }
      local_occurrence_seen[owner_index][occurrence.cell_vertex_index] =
          UINT8_C(1);

      const ExactPowerCellVertex& local_vertex =
          cell.geometry->vertices[occurrence.cell_vertex_index];
      const ExactOrdinaryCellVertexQueryRecord* const query =
          cell.query_by_vertex[occurrence.cell_vertex_index];
      if (query == nullptr || local_vertex.position != vertex.position ||
          query->nearest_squared_distance != vertex.nearest_squared_distance ||
          query->complete_nearest_shell_ids !=
              vertex.complete_nearest_shell_ids ||
          artificial_box_mask(
              *cell.geometry, local_vertex, cloud.size()) !=
              vertex.artificial_box_face_mask) {
        throw std::logic_error(
            "a global vertex disagrees with a referenced local occurrence");
      }
      vertex_owner_ids.push_back(occurrence.owner_id);
    }
    require_strict_point_ids(
        vertex_owner_ids, cloud.size(), "global vertex owner ids");
    if (vertex_owner_ids != vertex.complete_nearest_shell_ids) {
      throw std::logic_error(
          "global vertex owners disagree with its complete nearest shell");
    }
    owner_ids.push_back(std::move(vertex_owner_ids));
  }

  for (const auto& owner_occurrences : local_occurrence_seen) {
    if (std::any_of(
            owner_occurrences.begin(),
            owner_occurrences.end(),
            [](std::uint8_t count) { return count != UINT8_C(1); })) {
      throw std::logic_error(
          "a final local vertex has no unique global occurrence");
    }
  }
  return owner_ids;
}

void validate_contacts(
    const CanonicalPointCloud& cloud,
    const ExactOrdinaryDiagramClosureResult& result) {
  for (const ExactOrdinaryDiagramContact& contact : result.contacts) {
    require_strict_point_ids(
        contact.query_ids, cloud.size(), "contact query ids");
    require_strict_point_ids(
        contact.carrier_shell_ids, cloud.size(), "contact carrier ids");
    require_strict_indices(
        contact.global_vertex_indices,
        result.global_vertices.size(),
        "contact global vertex indices");
    if (contact.query_ids.size() < 2U ||
        contact.carrier_shell_ids.size() < contact.query_ids.size() ||
        contact.global_vertex_indices.empty() ||
        contact.affine_dimension > 3U || contact.site_affine_rank > 3U ||
        (contact.common_artificial_box_face_mask & UINT8_C(0xc0)) !=
            UINT8_C(0) ||
        !std::includes(
            contact.carrier_shell_ids.begin(),
            contact.carrier_shell_ids.end(),
            contact.query_ids.begin(),
            contact.query_ids.end())) {
      throw std::logic_error("an ordinary-diagram contact is malformed");
    }
  }
}

void write_ratio(std::ostream& output, const ExactRational& value) {
  output << "{\"denominator\":\""
         << morsehgp3d::exact::canonical_integer_string(value.denominator())
         << "\",\"numerator\":\""
         << morsehgp3d::exact::canonical_integer_string(value.numerator())
         << "\"}";
}

void write_level(std::ostream& output, const ExactLevel& level) {
  output << "{\"denominator\":\"" << level.denominator_string()
         << "\",\"numerator\":\"" << level.numerator_string() << "\"}";
}

void write_position(std::ostream& output, const ExactRational3& position) {
  output << "{\"x\":";
  write_ratio(output, position.coordinate(0U));
  output << ",\"y\":";
  write_ratio(output, position.coordinate(1U));
  output << ",\"z\":";
  write_ratio(output, position.coordinate(2U));
  output << '}';
}

void write_point_ids(
    std::ostream& output, std::span<const PointId> point_ids) {
  output << '[';
  for (std::size_t index = 0U; index < point_ids.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    output << point_ids[index];
  }
  output << ']';
}

void write_canonical_point_bits(
    std::ostream& output,
    const CanonicalPointCloud& cloud,
    const ExactOrdinaryDiagramClosureResult& result) {
  if (result.canonical_point_bits.size() != cloud.size()) {
    throw std::logic_error(
        "the diagram point-bit manifest has the wrong size");
  }
  output << '[';
  for (std::size_t index = 0U; index < cloud.size(); ++index) {
    const PointId point_id = static_cast<PointId>(index);
    const auto expected = cloud.point(point_id).canonical_input_bits();
    const auto& observed = result.canonical_point_bits[index];
    if (observed != expected) {
      throw std::logic_error(
          "the diagram point-bit manifest disagrees with the canonical cloud");
    }
    if (index != 0U) {
      output << ',';
    }
    output << "[\"" << morsehgp3d::exact::binary64_hex(observed[0U])
           << "\",\"" << morsehgp3d::exact::binary64_hex(observed[1U])
           << "\",\"" << morsehgp3d::exact::binary64_hex(observed[2U])
           << "\"]";
  }
  output << ']';
}

void write_cells(
    std::ostream& output,
    const CanonicalPointCloud& cloud,
    const std::vector<FinalCellView>& cells) {
  output << '[';
  for (std::size_t owner_index = 0U; owner_index < cells.size();
       ++owner_index) {
    if (owner_index != 0U) {
      output << ',';
    }
    const FinalCellView& cell = cells[owner_index];
    if (cell.closure == nullptr || cell.geometry == nullptr ||
        cell.final_round == nullptr ||
        cell.closure->owner_id != static_cast<PointId>(owner_index)) {
      throw std::logic_error("a validated final-cell view is malformed");
    }
    output << "{\"owner_id\":" << cell.closure->owner_id
           << ",\"vertices\":[";
    for (std::size_t vertex_index = 0U;
         vertex_index < cell.geometry->vertices.size(); ++vertex_index) {
      if (vertex_index != 0U) {
        output << ',';
      }
      const ExactPowerCellVertex& vertex =
          cell.geometry->vertices[vertex_index];
      const ExactOrdinaryCellVertexQueryRecord* const query =
          cell.query_by_vertex[vertex_index];
      if (query == nullptr) {
        throw std::logic_error(
            "a final cell vertex lost its validated nearest-shell query");
      }
      output << "{\"box_mask\":"
             << static_cast<unsigned int>(artificial_box_mask(
                    *cell.geometry, vertex, cloud.size()))
             << ",\"position\":";
      write_position(output, vertex.position);
      output << ",\"shell_ids\":";
      write_point_ids(output, query->complete_nearest_shell_ids);
      output << '}';
    }
    output << "]}";
  }
  output << ']';
}

void write_claims(
    std::ostream& output,
    const ExactOrdinaryDiagramClosureResult& result) {
  const auto write_bool = [&output](bool value) {
    output << (value ? "true" : "false");
  };
  output << "{\"all_cells_full_dimensional_nonempty_certified\":";
  write_bool(result.all_cells_full_dimensional_nonempty_certified);
  output << ",\"all_local_queues_empty_certified\":";
  write_bool(result.all_local_queues_empty_certified);
  output << ",\"artificial_box_boundaries_certified\":";
  write_bool(result.artificial_box_boundaries_certified);
  output << ",\"global_vertex_occurrence_bijection_certified\":";
  write_bool(result.global_vertex_occurrence_bijection_certified);
  output << ",\"natural_incidences_reconciled_certified\":";
  write_bool(result.natural_incidences_reconciled_certified);
  output << '}';
}

void write_global_vertices(
    std::ostream& output,
    const ExactOrdinaryDiagramClosureResult& result,
    const std::vector<std::vector<PointId>>& owner_ids) {
  if (owner_ids.size() != result.global_vertices.size()) {
    throw std::logic_error("the global owner-id projection has the wrong size");
  }
  output << '[';
  for (std::size_t index = 0U; index < result.global_vertices.size();
       ++index) {
    if (index != 0U) {
      output << ',';
    }
    const ExactOrdinaryDiagramVertex& vertex = result.global_vertices[index];
    output << "{\"box_mask\":"
           << static_cast<unsigned int>(vertex.artificial_box_face_mask)
           << ",\"distance\":";
    write_level(output, vertex.nearest_squared_distance);
    output << ",\"owner_ids\":";
    write_point_ids(output, owner_ids[index]);
    output << ",\"position\":";
    write_position(output, vertex.position);
    output << ",\"shell_ids\":";
    write_point_ids(output, vertex.complete_nearest_shell_ids);
    output << '}';
  }
  output << ']';
}

void write_contacts(
    std::ostream& output,
    const ExactOrdinaryDiagramClosureResult& result) {
  output << '[';
  for (std::size_t contact_index = 0U;
       contact_index < result.contacts.size(); ++contact_index) {
    if (contact_index != 0U) {
      output << ',';
    }
    const ExactOrdinaryDiagramContact& contact =
        result.contacts[contact_index];
    output << "{\"box_mask\":"
           << static_cast<unsigned int>(
                  contact.common_artificial_box_face_mask)
           << ",\"carrier\":";
    write_point_ids(output, contact.carrier_shell_ids);
    output << ",\"dimension\":" << contact.affine_dimension
           << ",\"kind\":\""
           << morsehgp3d::spatial::to_string(contact.kind)
           << "\",\"query\":";
    write_point_ids(output, contact.query_ids);
    output << ",\"site_rank\":" << contact.site_affine_rank
           << ",\"vertex_positions\":[";
    for (std::size_t vertex_offset = 0U;
         vertex_offset < contact.global_vertex_indices.size();
         ++vertex_offset) {
      if (vertex_offset != 0U) {
        output << ',';
      }
      const std::size_t vertex_index =
          contact.global_vertex_indices[vertex_offset];
      if (vertex_index >= result.global_vertices.size()) {
        throw std::logic_error(
            "a contact lost its validated global vertex reference");
      }
      write_position(output, result.global_vertices[vertex_index].position);
    }
    output << "],\"witness\":";
    write_position(output, contact.relative_interior_witness);
    output << ",\"witness_distance\":";
    write_level(output, contact.witness_nearest_squared_distance);
    output << '}';
  }
  output << ']';
}

void write_omega(
    std::ostream& output,
    const StrictlyPaddedDyadicAabb3Result& clipping_box) {
  if (!clipping_box.certificate.has_value()) {
    throw std::logic_error("a complete clipping box has no certificate");
  }
  const auto& omega = clipping_box.certificate->omega;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    if (morsehgp3d::exact::canonicalize_binary64_bits(
            omega.lower_binary64_bits[axis]) !=
            omega.lower_binary64_bits[axis] ||
        morsehgp3d::exact::canonicalize_binary64_bits(
            omega.upper_binary64_bits[axis]) !=
            omega.upper_binary64_bits[axis]) {
      throw std::logic_error("the clipping box contains a noncanonical word");
    }
  }
  output << "{\"lower\":[";
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    if (axis != 0U) {
      output << ',';
    }
    output << '\"'
           << morsehgp3d::exact::binary64_hex(
                  omega.lower_binary64_bits[axis])
           << '\"';
  }
  output << "],\"upper\":[";
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    if (axis != 0U) {
      output << ',';
    }
    output << '\"'
           << morsehgp3d::exact::binary64_hex(
                  omega.upper_binary64_bits[axis])
           << '\"';
  }
  output << "]}";
}

[[nodiscard]] std::string process_case(const InputCase& input_case) {
  const CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(input_case.input_points);
  const StrictlyPaddedDyadicAabb3Result clipping_box =
      morsehgp3d::spatial::build_strictly_padded_dyadic_aabb(cloud);
  require_full_box_verification(cloud, clipping_box);

  const ExactOrdinaryDiagramClosureResult result =
      morsehgp3d::spatial::build_exact_bounded_ordinary_diagram_closure(
          cloud, clipping_box);
  require_full_diagram_verification(cloud, clipping_box, result);
  if (result.decision != ExactOrdinaryDiagramClosureDecision::complete ||
      result.clipping_box != clipping_box ||
      !result.all_local_queues_empty_certified ||
      !result.all_cells_full_dimensional_nonempty_certified ||
      !result.global_vertex_occurrence_bijection_certified ||
      !result.natural_incidences_reconciled_certified ||
      !result.artificial_box_boundaries_certified) {
    throw std::logic_error(
        "a fully verified ordinary diagram lacks its complete claims");
  }

  const std::vector<FinalCellView> cells =
      validated_final_cells(cloud, result);
  const std::vector<std::vector<PointId>> global_owner_ids =
      validated_global_owner_ids(cloud, cells, result);
  validate_contacts(cloud, result);

  std::ostringstream output;
  output << "{\"canonical_point_bits\":";
  write_canonical_point_bits(output, cloud, result);
  output << ",\"case\":\"" << input_case.name << "\",\"cells\":";
  write_cells(output, cloud, cells);
  output << ",\"claims\":";
  write_claims(output, result);
  output << ",\"contacts\":";
  write_contacts(output, result);
  output << ",\"global_vertices\":";
  write_global_vertices(output, result, global_owner_ids);
  output << ",\"omega\":";
  write_omega(output, clipping_box);
  output << ",\"schema\":\"" << ExactOrdinaryDiagramClosureResult::schema
         << "\"}";
  return output.str();
}

}  // namespace

int main() {
  try {
    std::string line;
    if (!std::getline(std::cin, line) || line != input_protocol_header) {
      throw std::invalid_argument(
          "missing ordinary-diagram input protocol header");
    }

    std::cout << output_protocol_header << '\n';
    while (std::getline(std::cin, line)) {
      if (line.empty()) {
        throw std::invalid_argument("blank protocol lines are forbidden");
      }
      const InputCase input_case = parse_case(line);
      std::cout << process_case(input_case) << '\n';
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "ordinary_diagram_closure_dump: " << error.what() << '\n';
    return 2;
  }
}
