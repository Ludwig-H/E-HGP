#include "morsehgp3d/hierarchy/exact_low_order_gabriel_skeleton.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

[[nodiscard]] std::uint64_t checked_u64(
    std::size_t value,
    const char* message) {
  if constexpr (sizeof(std::size_t) > sizeof(std::uint64_t)) {
    if (value > static_cast<std::size_t>(
                    std::numeric_limits<std::uint64_t>::max())) {
      throw std::overflow_error(message);
    }
  }
  return static_cast<std::uint64_t>(value);
}

class DigestWriter {
 public:
  explicit DigestWriter(std::string_view domain) { text(domain); }

  void byte(std::uint8_t value) {
    builder_.update(std::span<const std::uint8_t>{&value, 1U});
  }

  void boolean(bool value) { byte(value ? std::uint8_t{1U} : 0U); }

  void u64(std::uint64_t value) {
    std::array<std::uint8_t, 8U> bytes{};
    for (std::size_t index = 0U; index < bytes.size(); ++index) {
      const std::size_t shift = (bytes.size() - 1U - index) * 8U;
      bytes[index] = static_cast<std::uint8_t>(value >> shift);
    }
    builder_.update(bytes);
  }

  void size(std::size_t value) {
    u64(checked_u64(
        value,
        "a low-order Gabriel skeleton size does not fit uint64"));
  }

  void point_id(spatial::PointId value) { u64(value); }

  void text(std::string_view value) {
    size(value.size());
    builder_.update(value);
  }

  void big_int(const exact::BigInt& value) {
    text(exact::canonical_integer_string(value));
  }

  void level(const exact::ExactLevel& value) {
    text(value.canonical_key());
  }

  [[nodiscard]] contract::CanonicalId finalize() {
    return builder_.finalize();
  }

 private:
  contract::CanonicalSha256Builder builder_;
};

void write_requirements(
    DigestWriter& writer,
    std::size_t point_count,
    std::size_t requested_maximum_order,
    std::size_t effective_maximum_order,
    std::size_t maximum_relevant_closed_rank) {
  writer.size(point_count);
  writer.size(requested_maximum_order);
  writer.size(effective_maximum_order);
  writer.size(maximum_relevant_closed_rank);
}

void write_pair_event(
    DigestWriter& writer,
    const ExactPairSupportEvent& event) {
  writer.point_id(event.support_ids[0]);
  writer.point_id(event.support_ids[1]);
  writer.level(event.squared_level);
  writer.size(event.interior_ids.size());
  for (const spatial::PointId point_id : event.interior_ids) {
    writer.point_id(point_id);
  }
  writer.size(event.closed_rank);
  writer.size(event.exterior_count);
}

void write_pair_diagnostic(
    DigestWriter& writer,
    const ExactPairSupportExtraShellDiagnostic& diagnostic) {
  writer.point_id(diagnostic.support_ids[0]);
  writer.point_id(diagnostic.support_ids[1]);
  writer.level(diagnostic.squared_level);
  writer.size(diagnostic.interior_ids.size());
  for (const spatial::PointId point_id : diagnostic.interior_ids) {
    writer.point_id(point_id);
  }
  writer.size(diagnostic.shell_count);
  writer.point_id(diagnostic.canonical_extra_shell_witness_id);
  writer.size(diagnostic.minimum_possible_closed_rank);
  writer.size(diagnostic.observed_closed_rank);
  writer.size(diagnostic.exterior_count);
}

[[nodiscard]] contract::CanonicalId pair_projection_payload_digest(
    const ExactPairSupportStreamResult& source) {
  DigestWriter writer{
      "MorseHGP3D/exact_low_order_gabriel_skeleton/pair/v1"};
  write_requirements(
      writer,
      source.requirements.point_count,
      source.requirements.requested_maximum_order,
      source.requirements.effective_maximum_order,
      source.requirements.maximum_relevant_closed_rank);
  writer.byte(static_cast<std::underlying_type_t<
                  ExactPairSupportStreamStatus>>(source.status));
  writer.byte(static_cast<std::underlying_type_t<
                  ExactPairSupportStopReason>>(source.stop_reason));
  writer.size(source.events.size());
  for (const ExactPairSupportEvent& event : source.events) {
    write_pair_event(writer, event);
  }
  writer.size(source.relevant_extra_shell_diagnostics.size());
  for (const ExactPairSupportExtraShellDiagnostic& diagnostic :
       source.relevant_extra_shell_diagnostics) {
    write_pair_diagnostic(writer, diagnostic);
  }
  writer.size(source.remaining_frontier.size());
  writer.size(source.audit.total_pair_count);
  writer.size(source.audit.resolved_pair_count);
  writer.size(source.audit.remaining_frontier_pair_count);
  writer.size(source.audit.accepted_event_count);
  writer.size(source.audit.relevant_extra_shell_diagnostic_count);
  writer.boolean(source.audit.pair_partition_accounting_certified);
  writer.boolean(source.self_product_partition_certified);
  writer.boolean(source.witness_antichains_certified);
  writer.boolean(source.all_rank_prunes_recertified);
  writer.boolean(source.all_rank_relevant_shells_complete);
  writer.boolean(source.frontier_exhausted);
  writer.boolean(source.no_forbidden_global_structure_materialized);
  writer.boolean(source.hierarchy_reduction_performed);
  return writer.finalize();
}

void write_higher_event(
    DigestWriter& writer,
    const ExactHigherSupportEvent& event) {
  writer.byte(event.support_size);
  for (const spatial::PointId point_id : event.support_ids) {
    writer.point_id(point_id);
  }
  writer.level(event.squared_level);
  writer.size(event.interior_ids.size());
  for (const spatial::PointId point_id : event.interior_ids) {
    writer.point_id(point_id);
  }
  writer.size(event.closed_rank);
  writer.size(event.exterior_count);
}

void write_higher_diagnostic(
    DigestWriter& writer,
    const ExactHigherSupportExtraShellDiagnostic& diagnostic) {
  writer.byte(diagnostic.support_size);
  for (const spatial::PointId point_id : diagnostic.support_ids) {
    writer.point_id(point_id);
  }
  writer.level(diagnostic.squared_level);
  writer.size(diagnostic.interior_ids.size());
  for (const spatial::PointId point_id : diagnostic.interior_ids) {
    writer.point_id(point_id);
  }
  writer.size(diagnostic.shell_count);
  writer.point_id(diagnostic.canonical_extra_shell_witness_id);
  writer.size(diagnostic.minimum_possible_closed_rank);
  writer.size(diagnostic.observed_closed_rank);
  writer.size(diagnostic.exterior_count);
}

[[nodiscard]] contract::CanonicalId higher_projection_payload_digest(
    const ExactHigherSupportStreamResult& source) {
  DigestWriter writer{
      "MorseHGP3D/exact_low_order_gabriel_skeleton/higher/v1"};
  write_requirements(
      writer,
      source.requirements.point_count,
      source.requirements.requested_maximum_order,
      source.requirements.effective_maximum_order,
      source.requirements.maximum_relevant_closed_rank);
  writer.byte(static_cast<std::underlying_type_t<
                  ExactHigherSupportStreamStatus>>(source.status));
  writer.byte(static_cast<std::underlying_type_t<
                  ExactHigherSupportStopReason>>(source.stop_reason));
  writer.size(source.events.size());
  for (const ExactHigherSupportEvent& event : source.events) {
    write_higher_event(writer, event);
  }
  writer.size(source.relevant_extra_shell_diagnostics.size());
  for (const ExactHigherSupportExtraShellDiagnostic& diagnostic :
       source.relevant_extra_shell_diagnostics) {
    write_higher_diagnostic(writer, diagnostic);
  }
  writer.size(source.remaining_frontier.size());
  writer.big_int(source.audit.total_support_count);
  writer.big_int(source.audit.resolved_support_count);
  writer.big_int(source.audit.remaining_frontier_support_count);
  writer.size(source.audit.accepted_event_count);
  writer.size(source.audit.relevant_extra_shell_diagnostic_count);
  writer.boolean(source.audit.exact_bigint_universe_certified);
  writer.boolean(source.audit.grouped_partition_accounting_certified);
  writer.boolean(source.grouped_frontier_partition_certified);
  writer.boolean(source.all_prunes_replayable);
  writer.boolean(source.all_rank_relevant_shells_complete);
  writer.boolean(source.frontier_exhausted);
  writer.boolean(source.no_forbidden_global_structure_materialized);
  writer.boolean(source.hierarchy_reduction_performed);
  return writer.finalize();
}

[[nodiscard]] bool pair_frontier_empty(
    const ExactPairSupportStreamResult& source) noexcept {
  return source.frontier_exhausted && source.remaining_frontier.empty() &&
         source.audit.remaining_frontier_pair_count == 0U;
}

[[nodiscard]] bool higher_frontier_empty(
    const ExactHigherSupportStreamResult& source) {
  return source.frontier_exhausted && source.remaining_frontier.empty() &&
         source.audit.remaining_frontier_support_count == 0;
}

[[nodiscard]] bool requirements_match_and_cover_low_orders(
    const ExactPairSupportRequirements& pair,
    const ExactHigherSupportRequirements& higher) noexcept {
  if (pair.point_count != higher.point_count ||
      pair.requested_maximum_order != higher.requested_maximum_order ||
      pair.effective_maximum_order != higher.effective_maximum_order ||
      pair.maximum_relevant_closed_rank !=
          higher.maximum_relevant_closed_rank ||
      pair.point_count == 0U || pair.requested_maximum_order == 0U ||
      pair.requested_maximum_order >
          pair_support_maximum_requested_order ||
      higher.requested_maximum_order >
          higher_support_maximum_requested_order) {
    return false;
  }
  const std::size_t expected_effective_order =
      std::min(pair.requested_maximum_order, pair.point_count);
  if (pair.effective_maximum_order != expected_effective_order ||
      pair.effective_maximum_order == 0U) {
    return false;
  }
  const std::size_t expected_maximum_rank =
      pair.effective_maximum_order < pair.point_count
          ? pair.effective_maximum_order + 1U
          : pair.point_count;
  return pair.maximum_relevant_closed_rank == expected_maximum_rank;
}

[[nodiscard]] bool valid_point_id(
    spatial::PointId point_id,
    std::size_t point_count) noexcept {
  if constexpr (sizeof(std::size_t) > sizeof(spatial::PointId)) {
    if (point_count > static_cast<std::size_t>(
                          std::numeric_limits<spatial::PointId>::max())) {
      return false;
    }
  }
  return point_id < static_cast<spatial::PointId>(point_count);
}

[[nodiscard]] bool strictly_increasing_valid_ids(
    std::span<const spatial::PointId> point_ids,
    std::size_t point_count) noexcept {
  for (std::size_t index = 0U; index < point_ids.size(); ++index) {
    if (!valid_point_id(point_ids[index], point_count) ||
        (index != 0U && point_ids[index - 1U] >= point_ids[index])) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool support_disjoint_from_interior(
    std::span<const spatial::PointId> support_ids,
    std::span<const spatial::PointId> interior_ids) noexcept {
  for (const spatial::PointId support_id : support_ids) {
    if (std::binary_search(
            interior_ids.begin(), interior_ids.end(), support_id)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool rank_and_exterior_counts_valid(
    std::size_t support_size,
    std::size_t interior_size,
    std::size_t closed_rank,
    std::size_t exterior_count,
    std::size_t point_count) noexcept {
  if (interior_size >
      std::numeric_limits<std::size_t>::max() - support_size) {
    return false;
  }
  const std::size_t expected_rank = support_size + interior_size;
  return closed_rank == expected_rank && closed_rank <= point_count &&
         exterior_count == point_count - closed_rank;
}

[[nodiscard]] bool pair_event_valid(
    const ExactPairSupportEvent& event,
    std::size_t point_count) noexcept {
  const std::span<const spatial::PointId> support_ids{event.support_ids};
  const std::span<const spatial::PointId> interior_ids{event.interior_ids};
  return strictly_increasing_valid_ids(support_ids, point_count) &&
         strictly_increasing_valid_ids(interior_ids, point_count) &&
         support_disjoint_from_interior(support_ids, interior_ids) &&
         rank_and_exterior_counts_valid(
             support_ids.size(),
             interior_ids.size(),
             event.closed_rank,
             event.exterior_count,
             point_count);
}

[[nodiscard]] bool higher_event_valid(
    const ExactHigherSupportEvent& event,
    std::size_t point_count) noexcept {
  if (event.support_size < 3U || event.support_size > 4U) {
    return false;
  }
  const std::size_t support_size =
      static_cast<std::size_t>(event.support_size);
  const std::span<const spatial::PointId> support_ids{
      event.support_ids.data(), support_size};
  const std::span<const spatial::PointId> interior_ids{event.interior_ids};
  for (std::size_t index = support_size;
       index < event.support_ids.size();
       ++index) {
    if (event.support_ids[index] != spatial::PointId{0U}) {
      return false;
    }
  }
  return strictly_increasing_valid_ids(support_ids, point_count) &&
         strictly_increasing_valid_ids(interior_ids, point_count) &&
         support_disjoint_from_interior(support_ids, interior_ids) &&
         rank_and_exterior_counts_valid(
             support_size,
             interior_ids.size(),
             event.closed_rank,
             event.exterior_count,
             point_count);
}

[[nodiscard]] bool edge_identity_less(
    const ExactLowOrderGabrielEdgeRecord& left,
    const ExactLowOrderGabrielEdgeRecord& right) {
  if (left.point_ids != right.point_ids) {
    return left.point_ids < right.point_ids;
  }
  return left.squared_level < right.squared_level;
}

[[nodiscard]] bool edge_output_less(
    const ExactLowOrderGabrielEdgeRecord& left,
    const ExactLowOrderGabrielEdgeRecord& right) {
  if (left.squared_level != right.squared_level) {
    return left.squared_level < right.squared_level;
  }
  return left.point_ids < right.point_ids;
}

[[nodiscard]] bool triangle_identity_less(
    const ExactLowOrderGabrielTriangleRecord& left,
    const ExactLowOrderGabrielTriangleRecord& right) {
  if (left.point_ids != right.point_ids) {
    return left.point_ids < right.point_ids;
  }
  if (left.squared_level != right.squared_level) {
    return left.squared_level < right.squared_level;
  }
  return static_cast<std::underlying_type_t<
             ExactLowOrderGabrielTriangleProvenance>>(left.provenance) <
         static_cast<std::underlying_type_t<
             ExactLowOrderGabrielTriangleProvenance>>(right.provenance);
}

[[nodiscard]] bool triangle_output_less(
    const ExactLowOrderGabrielTriangleRecord& left,
    const ExactLowOrderGabrielTriangleRecord& right) {
  if (left.squared_level != right.squared_level) {
    return left.squared_level < right.squared_level;
  }
  return left.point_ids < right.point_ids;
}

[[nodiscard]] ExactLowOrderGabrielTriangleProvenance combine_provenance(
    ExactLowOrderGabrielTriangleProvenance left,
    ExactLowOrderGabrielTriangleProvenance right) noexcept {
  using Underlying = std::underlying_type_t<
      ExactLowOrderGabrielTriangleProvenance>;
  return static_cast<ExactLowOrderGabrielTriangleProvenance>(
      static_cast<Underlying>(left) | static_cast<Underlying>(right));
}

[[nodiscard]] ExactLowOrderGabrielSkeletonResult rejected_result(
    ExactLowOrderGabrielSkeletonResult result,
    ExactLowOrderGabrielSkeletonDecision decision) {
  result.decision = decision;
  result.rank_two_edges.clear();
  result.rank_three_triangles.clear();
  result.audit.rank_two_edges_canonical_sorted_unique = false;
  result.audit.rank_three_triangles_canonical_sorted_unique = false;
  return result;
}

}  // namespace

ExactLowOrderGabrielEventSourceReceipt
make_exact_low_order_gabriel_pair_source_receipt(
    const ExactPairSupportStreamResult& source) {
  return ExactLowOrderGabrielEventSourceReceipt{
      exact_low_order_gabriel_skeleton_receipt_schema_version,
      ExactLowOrderGabrielEventSource::pair,
      source.requirements.point_count,
      source.requirements.requested_maximum_order,
      source.requirements.effective_maximum_order,
      source.requirements.maximum_relevant_closed_rank,
      source.events.size(),
      source.relevant_extra_shell_diagnostics.size(),
      pair_projection_payload_digest(source),
      source.stream_complete(),
      pair_frontier_empty(source),
      source.absence_of_additional_pair_supports_certified(),
      source.relevant_extra_shell_diagnostics.empty() &&
          source.audit.relevant_extra_shell_diagnostic_count == 0U};
}

ExactLowOrderGabrielEventSourceReceipt
make_exact_low_order_gabriel_higher_source_receipt(
    const ExactHigherSupportStreamResult& source) {
  return ExactLowOrderGabrielEventSourceReceipt{
      exact_low_order_gabriel_skeleton_receipt_schema_version,
      ExactLowOrderGabrielEventSource::higher,
      source.requirements.point_count,
      source.requirements.requested_maximum_order,
      source.requirements.effective_maximum_order,
      source.requirements.maximum_relevant_closed_rank,
      source.events.size(),
      source.relevant_extra_shell_diagnostics.size(),
      higher_projection_payload_digest(source),
      source.stream_complete(),
      higher_frontier_empty(source),
      source.absence_of_additional_higher_supports_certified(),
      source.relevant_extra_shell_diagnostics.empty() &&
          source.audit.relevant_extra_shell_diagnostic_count == 0U};
}

ExactLowOrderGabrielSkeletonResult
project_exact_low_order_gabriel_skeleton_from_events(
    const ExactPairSupportStreamResult& pair_source,
    const ExactLowOrderGabrielEventSourceReceipt& pair_receipt,
    const ExactHigherSupportStreamResult& higher_source,
    const ExactLowOrderGabrielEventSourceReceipt& higher_receipt) {
  ExactLowOrderGabrielSkeletonResult result;
  result.audit.pair_event_count = pair_source.events.size();
  result.audit.higher_event_count = higher_source.events.size();
  result.audit.source_requirements_match =
      requirements_match_and_cover_low_orders(
          pair_source.requirements, higher_source.requirements);
  if (!result.audit.source_requirements_match) {
    return rejected_result(
        std::move(result),
        ExactLowOrderGabrielSkeletonDecision::
            invalid_source_requirements);
  }

  const bool pair_complete = pair_source.stream_complete() &&
                             pair_frontier_empty(pair_source) &&
                             pair_receipt.stream_complete &&
                             pair_receipt.frontier_empty &&
                             pair_receipt.absence_of_additional_supports_certified;
  if (!pair_complete) {
    return rejected_result(
        std::move(result),
        ExactLowOrderGabrielSkeletonDecision::pair_source_not_complete);
  }
  const bool higher_complete = higher_source.stream_complete() &&
                               higher_frontier_empty(higher_source) &&
                               higher_receipt.stream_complete &&
                               higher_receipt.frontier_empty &&
                               higher_receipt
                                   .absence_of_additional_supports_certified;
  if (!higher_complete) {
    return rejected_result(
        std::move(result),
        ExactLowOrderGabrielSkeletonDecision::higher_source_not_complete);
  }

  result.audit.relevant_extra_shell_diagnostics_absent =
      pair_source.relevant_extra_shell_diagnostics.empty() &&
      higher_source.relevant_extra_shell_diagnostics.empty() &&
      pair_source.audit.relevant_extra_shell_diagnostic_count == 0U &&
      higher_source.audit.relevant_extra_shell_diagnostic_count == 0U &&
      pair_receipt.relevant_extra_shell_diagnostics_absent &&
      higher_receipt.relevant_extra_shell_diagnostics_absent &&
      pair_receipt.relevant_extra_shell_diagnostic_count == 0U &&
      higher_receipt.relevant_extra_shell_diagnostic_count == 0U;
  if (!result.audit.relevant_extra_shell_diagnostics_absent) {
    return rejected_result(
        std::move(result),
        ExactLowOrderGabrielSkeletonDecision::
            relevant_extra_shell_diagnostics_present);
  }

  const ExactLowOrderGabrielEventSourceReceipt expected_pair_receipt =
      make_exact_low_order_gabriel_pair_source_receipt(pair_source);
  const ExactLowOrderGabrielEventSourceReceipt expected_higher_receipt =
      make_exact_low_order_gabriel_higher_source_receipt(higher_source);
  result.audit.pair_receipt_verified = pair_receipt == expected_pair_receipt;
  result.audit.higher_receipt_verified =
      higher_receipt == expected_higher_receipt;
  if (!result.audit.pair_receipt_verified ||
      !result.audit.higher_receipt_verified) {
    return rejected_result(
        std::move(result),
        ExactLowOrderGabrielSkeletonDecision::receipt_mismatch);
  }

  const std::size_t point_count = pair_source.requirements.point_count;
  if (pair_source.audit.accepted_event_count != pair_source.events.size() ||
      pair_source.audit.relevant_extra_shell_diagnostic_count !=
          pair_source.relevant_extra_shell_diagnostics.size()) {
    return rejected_result(
        std::move(result),
        ExactLowOrderGabrielSkeletonDecision::invalid_pair_event);
  }
  if (higher_source.audit.accepted_event_count !=
          higher_source.events.size() ||
      higher_source.audit.relevant_extra_shell_diagnostic_count !=
          higher_source.relevant_extra_shell_diagnostics.size()) {
    return rejected_result(
        std::move(result),
        ExactLowOrderGabrielSkeletonDecision::invalid_higher_event);
  }

  if (higher_source.events.size() >
      std::numeric_limits<std::size_t>::max() - pair_source.events.size()) {
    throw std::length_error(
        "the low-order Gabriel skeleton input count overflows size_t");
  }
  result.rank_two_edges.reserve(pair_source.events.size());
  result.rank_three_triangles.reserve(
      pair_source.events.size() + higher_source.events.size());
  for (const ExactPairSupportEvent& event : pair_source.events) {
    if (!pair_event_valid(event, point_count) ||
        event.closed_rank >
            pair_source.requirements.maximum_relevant_closed_rank) {
      return rejected_result(
          std::move(result),
          ExactLowOrderGabrielSkeletonDecision::invalid_pair_event);
    }
    if (event.closed_rank == 2U) {
      result.rank_two_edges.push_back(
          {event.support_ids, event.squared_level});
      ++result.audit.rank_two_edge_occurrence_count;
    } else if (event.closed_rank == 3U) {
      ExactLowOrderGabrielTriangleIds triangle{
          event.support_ids[0],
          event.support_ids[1],
          event.interior_ids[0]};
      std::sort(triangle.begin(), triangle.end());
      result.rank_three_triangles.push_back(
          {triangle,
           event.squared_level,
           ExactLowOrderGabrielTriangleProvenance::pair});
      ++result.audit.pair_triangle_occurrence_count;
    }
  }
  for (const ExactHigherSupportEvent& event : higher_source.events) {
    if (!higher_event_valid(event, point_count) ||
        event.closed_rank >
            higher_source.requirements.maximum_relevant_closed_rank) {
      return rejected_result(
          std::move(result),
          ExactLowOrderGabrielSkeletonDecision::invalid_higher_event);
    }
    if (event.support_size == 3U && event.closed_rank == 3U) {
      ExactLowOrderGabrielTriangleIds triangle{
          event.support_ids[0],
          event.support_ids[1],
          event.support_ids[2]};
      std::sort(triangle.begin(), triangle.end());
      result.rank_three_triangles.push_back(
          {triangle,
           event.squared_level,
           ExactLowOrderGabrielTriangleProvenance::higher});
      ++result.audit.higher_triangle_occurrence_count;
    }
  }

  std::sort(
      result.rank_two_edges.begin(),
      result.rank_two_edges.end(),
      edge_identity_less);
  std::vector<ExactLowOrderGabrielEdgeRecord> unique_edges;
  unique_edges.reserve(result.rank_two_edges.size());
  for (const ExactLowOrderGabrielEdgeRecord& edge :
       result.rank_two_edges) {
    if (!unique_edges.empty() &&
        unique_edges.back().point_ids == edge.point_ids) {
      if (unique_edges.back().squared_level != edge.squared_level) {
        ++result.audit.contradictory_rank_two_edge_level_count;
        return rejected_result(
            std::move(result),
            ExactLowOrderGabrielSkeletonDecision::
                contradictory_rank_two_edge_levels);
      }
      ++result.audit.duplicate_rank_two_edge_occurrence_count;
      continue;
    }
    unique_edges.push_back(edge);
  }
  result.rank_two_edges = std::move(unique_edges);
  std::sort(
      result.rank_two_edges.begin(),
      result.rank_two_edges.end(),
      edge_output_less);
  result.audit.rank_two_edges_canonical_sorted_unique =
      std::is_sorted(
          result.rank_two_edges.begin(),
          result.rank_two_edges.end(),
          edge_output_less);

  std::sort(
      result.rank_three_triangles.begin(),
      result.rank_three_triangles.end(),
      triangle_identity_less);
  std::vector<ExactLowOrderGabrielTriangleRecord> unique_triangles;
  unique_triangles.reserve(result.rank_three_triangles.size());
  for (const ExactLowOrderGabrielTriangleRecord& triangle :
       result.rank_three_triangles) {
    if (!unique_triangles.empty() &&
        unique_triangles.back().point_ids == triangle.point_ids) {
      if (unique_triangles.back().squared_level !=
          triangle.squared_level) {
        ++result.audit.contradictory_rank_three_triangle_level_count;
        return rejected_result(
            std::move(result),
            ExactLowOrderGabrielSkeletonDecision::
                contradictory_rank_three_triangle_levels);
      }
      unique_triangles.back().provenance = combine_provenance(
          unique_triangles.back().provenance, triangle.provenance);
      ++result.audit.duplicate_triangle_occurrence_count;
      continue;
    }
    unique_triangles.push_back(triangle);
  }
  result.rank_three_triangles = std::move(unique_triangles);
  std::sort(
      result.rank_three_triangles.begin(),
      result.rank_three_triangles.end(),
      triangle_output_less);
  result.audit.rank_three_triangles_canonical_sorted_unique =
      std::is_sorted(
          result.rank_three_triangles.begin(),
          result.rank_three_triangles.end(),
          triangle_output_less);
  result.decision = ExactLowOrderGabrielSkeletonDecision::
      locally_verified_regular_low_order_skeleton_projection;
  return result;
}

}  // namespace morsehgp3d::hierarchy
