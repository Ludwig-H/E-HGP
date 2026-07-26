#include "morsehgp3d/hierarchy/sparse_anchored_pair_h0_run.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace morsehgp3d::hierarchy {
namespace {

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
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

[[nodiscard]] bool locally_valid_ids(
    const std::array<spatial::PointId, 2>& support_ids,
    const std::vector<spatial::PointId>& interior_ids,
    std::size_t point_count) {
  if (support_ids[0] >= support_ids[1] ||
      support_ids[1] >= point_count ||
      !std::is_sorted(interior_ids.begin(), interior_ids.end()) ||
      std::adjacent_find(interior_ids.begin(), interior_ids.end()) !=
          interior_ids.end()) {
    return false;
  }
  for (const spatial::PointId point_id : interior_ids) {
    if (point_id >= point_count || point_id == support_ids[0] ||
        point_id == support_ids[1]) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] std::size_t event_reference_count(
    const ExactPairSupportEvent& event) {
  return checked_add(
      2U, event.interior_ids.size(),
      "a 14W event reference count overflows size_t");
}

[[nodiscard]] std::size_t diagnostic_reference_count(
    const ExactPairSupportExtraShellDiagnostic& diagnostic) {
  return checked_add(
      3U, diagnostic.interior_ids.size(),
      "a 14W diagnostic reference count overflows size_t");
}

void validate_event(
    const ExactPairSupportEvent& event,
    std::size_t point_count) {
  if (!locally_valid_ids(
          event.support_ids, event.interior_ids, point_count) ||
      event.interior_ids.size() > point_count - 2U ||
      event.closed_rank != 2U + event.interior_ids.size() ||
      event.exterior_count != point_count - event.closed_rank ||
      event.squared_level.numerator() == 0) {
    throw std::invalid_argument("a 14W pair event is locally inconsistent");
  }
}

void validate_diagnostic(
    const ExactPairSupportExtraShellDiagnostic& diagnostic,
    std::size_t point_count) {
  if (!locally_valid_ids(
          diagnostic.support_ids, diagnostic.interior_ids, point_count) ||
      diagnostic.canonical_extra_shell_witness_id >= point_count ||
      diagnostic.canonical_extra_shell_witness_id ==
          diagnostic.support_ids[0] ||
      diagnostic.canonical_extra_shell_witness_id ==
          diagnostic.support_ids[1] ||
      std::binary_search(
          diagnostic.interior_ids.begin(), diagnostic.interior_ids.end(),
          diagnostic.canonical_extra_shell_witness_id) ||
      diagnostic.shell_count < 3U ||
      diagnostic.minimum_possible_closed_rank !=
          2U + diagnostic.interior_ids.size() ||
      diagnostic.observed_closed_rank <
          diagnostic.minimum_possible_closed_rank + 1U ||
      diagnostic.observed_closed_rank > point_count ||
      diagnostic.exterior_count !=
          point_count - diagnostic.observed_closed_rank ||
      diagnostic.squared_level.numerator() == 0) {
    throw std::invalid_argument(
        "a 14W pair diagnostic is locally inconsistent");
  }
}

}  // namespace

bool exact_sparse_anchored_pair_h0_event_less(
    const ExactPairSupportEvent& left,
    const ExactPairSupportEvent& right) {
  if (left.squared_level != right.squared_level) {
    return left.squared_level < right.squared_level;
  }
  if (left.closed_rank != right.closed_rank) {
    return left.closed_rank < right.closed_rank;
  }
  if (left.interior_ids != right.interior_ids) {
    return left.interior_ids < right.interior_ids;
  }
  if (left.support_ids != right.support_ids) {
    return left.support_ids < right.support_ids;
  }
  return center_less(left.center, right.center);
}

bool exact_sparse_anchored_pair_h0_event_candidate_less(
    const ExactSparseAnchoredPairH0EventCandidate& left,
    const ExactSparseAnchoredPairH0EventCandidate& right) {
  return exact_sparse_anchored_pair_h0_event_less(left.event, right.event);
}

ExactSparseAnchoredPairH0Run project_exact_sparse_anchored_pair_h0_run(
    const ExactSparseAnchoredPairRecertifiedChunkProjection& source,
    std::size_t point_count,
    std::size_t effective_maximum_order,
    ExactSparseAnchoredPairH0RunLimits limits) {
  const ExactSparseAnchoredPairRecordSegment& segment =
      source.record_segment;
  if (point_count < 2U || effective_maximum_order == 0U ||
      effective_maximum_order > 10U ||
      effective_maximum_order > point_count ||
      point_count != source.cumulative_audit.point_count ||
      source.successor_advance_count <= source.source_advance_count ||
      limits.maximum_source_record_count == 0U ||
      limits.maximum_event_candidate_count == 0U ||
      limits.maximum_point_id_reference_count == 0U) {
    throw std::invalid_argument("a 14W projection contract is invalid");
  }
  if (segment.record_count() > limits.maximum_source_record_count ||
      segment.event_count > limits.maximum_event_candidate_count ||
      segment.relevant_extra_shell_diagnostic_count >
          limits.maximum_diagnostic_count ||
      segment.point_id_reference_count >
          limits.maximum_point_id_reference_count) {
    throw std::length_error("a 14W source segment exceeds its caps");
  }
  if (checked_add(
          segment.event_count,
          segment.relevant_extra_shell_diagnostic_count,
          "a 14W record population overflows size_t") !=
          segment.record_count() ||
      checked_add(
          segment.first_output_record_index, segment.record_count(),
          "a 14W global record offset overflows size_t") !=
          source.cumulative_audit.emitted_record_count ||
      checked_add(
          segment.first_output_point_id_reference_index,
          segment.point_id_reference_count,
          "a 14W global reference offset overflows size_t") !=
          source.cumulative_audit.emitted_point_id_reference_count ||
      segment.event_count > source.cumulative_audit.accepted_event_count ||
      segment.relevant_extra_shell_diagnostic_count >
          source.cumulative_audit
              .relevant_extra_shell_diagnostic_count) {
    throw std::invalid_argument(
        "a 14W source segment has inconsistent counts or offsets");
  }

  ExactSparseAnchoredPairH0Run result;
  result.point_count = point_count;
  result.effective_maximum_order = effective_maximum_order;
  result.source_advance_count = source.source_advance_count;
  result.successor_advance_count = source.successor_advance_count;
  result.first_source_output_record_index =
      segment.first_output_record_index;
  result.first_source_point_id_reference_index =
      segment.first_output_point_id_reference_index;
  result.source_record_count = segment.record_count();
  result.source_point_id_reference_count =
      segment.point_id_reference_count;
  result.event_candidates.reserve(segment.event_count);
  result.diagnostics.reserve(
      segment.relevant_extra_shell_diagnostic_count);

  std::size_t observed_reference_count = 0U;
  for (std::size_t local_index = 0U;
       local_index < segment.records.size(); ++local_index) {
    const std::size_t global_index = checked_add(
        segment.first_output_record_index, local_index,
        "a 14W record locator overflows size_t");
    const ExactSparseAnchoredPairRecord& record =
        segment.records[local_index];
    if (const auto* event = std::get_if<ExactPairSupportEvent>(&record)) {
      validate_event(*event, point_count);
      observed_reference_count = checked_add(
          observed_reference_count, event_reference_count(*event),
          "a 14W event reference population overflows size_t");
      ExactSparseAnchoredPairH0EventCandidate candidate;
      candidate.source_output_record_index = global_index;
      candidate.event = *event;
      if (event->closed_rank <= effective_maximum_order) {
        candidate.birth_order = event->closed_rank;
      }
      if (event->closed_rank >= 2U &&
          event->closed_rank - 1U <= effective_maximum_order) {
        candidate.saddle_order = event->closed_rank - 1U;
      }
      if (!candidate.birth_order.has_value() &&
          !candidate.saddle_order.has_value()) {
        throw std::invalid_argument(
            "a 14W rank-relevant event has no H0 role");
      }
      result.event_candidates.push_back(std::move(candidate));
    } else {
      const auto& diagnostic =
          std::get<ExactPairSupportExtraShellDiagnostic>(record);
      validate_diagnostic(diagnostic, point_count);
      observed_reference_count = checked_add(
          observed_reference_count,
          diagnostic_reference_count(diagnostic),
          "a 14W diagnostic reference population overflows size_t");
      result.diagnostics.push_back(
          ExactSparseAnchoredPairH0Diagnostic{global_index, diagnostic});
    }
  }

  if (result.event_candidates.size() != segment.event_count ||
      result.diagnostics.size() !=
          segment.relevant_extra_shell_diagnostic_count ||
      observed_reference_count != segment.point_id_reference_count) {
    throw std::invalid_argument(
        "a 14W source record population is inconsistent");
  }

  std::sort(
      result.event_candidates.begin(), result.event_candidates.end(),
      exact_sparse_anchored_pair_h0_event_candidate_less);
  for (std::size_t index = 1U;
       index < result.event_candidates.size(); ++index) {
    if (!exact_sparse_anchored_pair_h0_event_candidate_less(
            result.event_candidates[index - 1U],
            result.event_candidates[index])) {
      throw std::invalid_argument(
          "a 14W source chunk contains duplicate canonical events");
    }
  }
  result.candidates_sorted_by_terminal_facade_event_key = true;
  result.diagnostics_preserved = true;
  return result;
}

}  // namespace morsehgp3d::hierarchy
