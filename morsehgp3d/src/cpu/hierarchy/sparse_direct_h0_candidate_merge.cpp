#include "morsehgp3d/hierarchy/sparse_direct_h0_candidate_merge.hpp"

#include <algorithm>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

void validate_higher_projection_manifest(
    const ExactHigherSupportCheckpointManifest& manifest);

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

[[nodiscard]] bool support_less(
    const ExactSparseDirectH0Candidate& left,
    const ExactSparseDirectH0Candidate& right) {
  const std::size_t common = std::min(
      static_cast<std::size_t>(left.support_size),
      static_cast<std::size_t>(right.support_size));
  for (std::size_t index = 0U; index < common; ++index) {
    if (left.support_ids[index] != right.support_ids[index]) {
      return left.support_ids[index] < right.support_ids[index];
    }
  }
  return left.support_size < right.support_size;
}

[[nodiscard]] std::pair<
    std::optional<std::size_t>, std::optional<std::size_t>>
expected_roles(
    std::size_t closed_rank,
    std::size_t effective_maximum_order) {
  std::optional<std::size_t> birth;
  std::optional<std::size_t> saddle;
  if (closed_rank <= effective_maximum_order) {
    birth = closed_rank;
  }
  if (closed_rank >= 2U &&
      closed_rank - 1U <= effective_maximum_order) {
    saddle = closed_rank - 1U;
  }
  return {birth, saddle};
}

[[nodiscard]] bool valid_support_and_interior_ids(
    std::uint8_t support_size,
    const std::array<spatial::PointId, 4U>& support_ids,
    const std::vector<spatial::PointId>& interior_ids,
    std::size_t point_count) {
  if (support_size < 2U || support_size > 4U ||
      static_cast<std::size_t>(support_size) > point_count) {
    return false;
  }
  for (std::size_t index = 0U; index < support_size; ++index) {
    if (support_ids[index] >= point_count ||
        (index != 0U && support_ids[index - 1U] >= support_ids[index])) {
      return false;
    }
  }
  for (std::size_t index = support_size; index < support_ids.size(); ++index) {
    if (support_ids[index] != spatial::PointId{0}) {
      return false;
    }
  }
  if (!std::is_sorted(interior_ids.begin(), interior_ids.end()) ||
      std::adjacent_find(interior_ids.begin(), interior_ids.end()) !=
          interior_ids.end()) {
    return false;
  }
  for (const spatial::PointId point_id : interior_ids) {
    if (point_id >= point_count ||
        std::binary_search(
            support_ids.begin(),
            support_ids.begin() + support_size,
            point_id)) {
      return false;
    }
  }
  return interior_ids.size() <=
      point_count - static_cast<std::size_t>(support_size);
}

void validate_candidate(
    const ExactSparseDirectH0Candidate& candidate,
    std::size_t point_count,
    std::size_t effective_maximum_order) {
  const std::size_t support_size = candidate.support_size;
  const std::size_t expected_closed_rank = checked_add(
      support_size, candidate.interior_ids.size(),
      "a sparse direct H0 closed rank overflows size_t");
  const auto [birth, saddle] = expected_roles(
      expected_closed_rank, effective_maximum_order);
  if (!valid_support_and_interior_ids(
          candidate.support_size, candidate.support_ids,
          candidate.interior_ids, point_count) ||
      candidate.squared_level.numerator() == 0 ||
      candidate.closed_rank != expected_closed_rank ||
      candidate.exterior_count != point_count - expected_closed_rank ||
      candidate.birth_order != birth || candidate.saddle_order != saddle ||
      (!birth.has_value() && !saddle.has_value())) {
    throw std::invalid_argument(
        "a sparse direct H0 candidate is locally inconsistent");
  }
}

void validate_diagnostic(
    const ExactSparseDirectH0Diagnostic& diagnostic,
    std::size_t point_count) {
  const std::size_t support_size = diagnostic.support_size;
  const std::size_t minimum_rank = checked_add(
      support_size, diagnostic.interior_ids.size(),
      "a sparse direct H0 diagnostic rank overflows size_t");
  const std::size_t observed_rank = checked_add(
      diagnostic.shell_count, diagnostic.interior_ids.size(),
      "a sparse direct H0 observed rank overflows size_t");
  if (!valid_support_and_interior_ids(
          diagnostic.support_size, diagnostic.support_ids,
          diagnostic.interior_ids, point_count) ||
      diagnostic.squared_level.numerator() == 0 ||
      diagnostic.shell_count <= support_size ||
      diagnostic.canonical_extra_shell_witness_id >= point_count ||
      std::binary_search(
          diagnostic.support_ids.begin(),
          diagnostic.support_ids.begin() + support_size,
          diagnostic.canonical_extra_shell_witness_id) ||
      std::binary_search(
          diagnostic.interior_ids.begin(), diagnostic.interior_ids.end(),
          diagnostic.canonical_extra_shell_witness_id) ||
      diagnostic.minimum_possible_closed_rank != minimum_rank ||
      diagnostic.observed_closed_rank != observed_rank ||
      observed_rank > point_count ||
      diagnostic.exterior_count != point_count - observed_rank) {
    throw std::invalid_argument(
        "a sparse direct H0 diagnostic is locally inconsistent");
  }
}

[[nodiscard]] std::size_t candidate_reference_count(
    const ExactSparseDirectH0Candidate& candidate) {
  return checked_add(
      candidate.support_size, candidate.interior_ids.size(),
      "a sparse direct H0 candidate reference count overflows size_t");
}

[[nodiscard]] std::size_t diagnostic_reference_count(
    const ExactSparseDirectH0Diagnostic& diagnostic) {
  return checked_add(
      checked_add(
          diagnostic.support_size, diagnostic.interior_ids.size(),
          "a sparse direct H0 diagnostic reference count overflows size_t"),
      1U,
      "a sparse direct H0 diagnostic reference count overflows size_t");
}

void sort_and_validate_candidates(
    ExactSparseDirectH0CandidateRun& run) {
  std::sort(
      run.candidates.begin(), run.candidates.end(),
      exact_sparse_direct_h0_candidate_less);
  for (std::size_t index = 1U; index < run.candidates.size(); ++index) {
    if (!exact_sparse_direct_h0_candidate_less(
            run.candidates[index - 1U], run.candidates[index])) {
      throw std::invalid_argument(
          "a sparse direct H0 run contains duplicate canonical candidates");
    }
  }
  run.candidates_strictly_sorted_by_terminal_facade_key = true;
}

void validate_run(const ExactSparseDirectH0CandidateRun& run) {
  if (run.schema_version != sparse_direct_h0_candidate_merge_schema_version ||
      run.point_count < 2U || run.effective_maximum_order == 0U ||
      run.effective_maximum_order > 10U ||
      run.effective_maximum_order > run.point_count ||
      run.contains_pair_candidates == run.contains_higher_candidates ||
      run.contains_higher_candidates !=
          run.higher_source_contract.has_value() ||
      !run.candidates_strictly_sorted_by_terminal_facade_key ||
      !run.diagnostics_preserved || run.global_event_indices_assigned ||
      run.hierarchy_reduction_performed || run.complete_h0_authority_claimed) {
    throw std::invalid_argument(
        "a sparse direct H0 input run has an invalid scope");
  }
  const ExactSparseDirectH0HigherSourceContract* higher_contract =
      run.higher_source_contract
          ? &*run.higher_source_contract
          : nullptr;
  std::vector<bool> higher_candidate_locator_seen;
  std::vector<bool> higher_diagnostic_locator_seen;
  if (higher_contract != nullptr) {
    validate_higher_projection_manifest(higher_contract->manifest);
    const bool valid_status =
        (higher_contract->status ==
             ExactHigherSupportStreamStatus::complete &&
         higher_contract->stop_reason ==
             ExactHigherSupportStopReason::none) ||
        (higher_contract->status ==
             ExactHigherSupportStreamStatus::budget_exhausted &&
         higher_contract->stop_reason !=
             ExactHigherSupportStopReason::none);
    const std::size_t retained_record_count = checked_add(
        higher_contract->source_event_count,
        higher_contract->source_diagnostic_count,
        "a higher H0 source-contract record count overflows size_t");
    const std::size_t emitted_record_count = checked_add(
        retained_record_count,
        higher_contract->destroyed_prune_certificate_count,
        "a higher H0 source-contract record count overflows size_t");
    if (!valid_status ||
        higher_contract->manifest.point_count != run.point_count ||
        higher_contract->manifest.effective_maximum_order !=
            run.effective_maximum_order ||
        higher_contract->source_event_count != run.candidates.size() ||
        higher_contract->source_diagnostic_count !=
            run.diagnostics.size() ||
        higher_contract->emitted_record_count != emitted_record_count ||
        higher_contract->emitted_point_id_reference_count !=
            run.source_point_id_reference_count) {
      throw std::invalid_argument(
          "a sparse direct H0 higher source contract is inconsistent");
    }
    higher_candidate_locator_seen.assign(
        higher_contract->source_event_count, false);
    higher_diagnostic_locator_seen.assign(
        higher_contract->source_diagnostic_count, false);
  }
  std::size_t references = 0U;
  for (std::size_t index = 0U; index < run.candidates.size(); ++index) {
    const bool pair_locator = std::holds_alternative<
        ExactSparseDirectH0PairSourceLocator>(
            run.candidates[index].source_locator);
    if ((pair_locator && !run.contains_pair_candidates) ||
        (!pair_locator && !run.contains_higher_candidates)) {
      throw std::invalid_argument(
          "a sparse direct H0 candidate has the wrong source lane");
    }
    if (!pair_locator) {
      const auto locator = std::get<
          ExactSparseDirectH0HigherSourceLocator>(
              run.candidates[index].source_locator);
      if (higher_contract == nullptr ||
          locator.chunk_sequence != higher_contract->chunk_sequence ||
          locator.local_kind_index >=
              higher_candidate_locator_seen.size() ||
          higher_candidate_locator_seen[locator.local_kind_index]) {
        throw std::invalid_argument(
            "a sparse direct H0 higher candidate locator is invalid");
      }
      higher_candidate_locator_seen[locator.local_kind_index] = true;
    }
    validate_candidate(
        run.candidates[index], run.point_count,
        run.effective_maximum_order);
    references = checked_add(
        references, candidate_reference_count(run.candidates[index]),
        "a sparse direct H0 run reference count overflows size_t");
    if (index != 0U &&
        !exact_sparse_direct_h0_candidate_less(
            run.candidates[index - 1U], run.candidates[index])) {
      throw std::invalid_argument(
          "a sparse direct H0 input run is not strictly sorted");
    }
  }
  for (const ExactSparseDirectH0Diagnostic& diagnostic : run.diagnostics) {
    const bool pair_locator = std::holds_alternative<
        ExactSparseDirectH0PairSourceLocator>(diagnostic.source_locator);
    if ((pair_locator && !run.contains_pair_candidates) ||
        (!pair_locator && !run.contains_higher_candidates)) {
      throw std::invalid_argument(
          "a sparse direct H0 diagnostic has the wrong source lane");
    }
    if (!pair_locator) {
      const auto locator = std::get<
          ExactSparseDirectH0HigherSourceLocator>(
              diagnostic.source_locator);
      if (higher_contract == nullptr ||
          locator.chunk_sequence != higher_contract->chunk_sequence ||
          locator.local_kind_index >=
              higher_diagnostic_locator_seen.size() ||
          higher_diagnostic_locator_seen[locator.local_kind_index]) {
        throw std::invalid_argument(
            "a sparse direct H0 higher diagnostic locator is invalid");
      }
      higher_diagnostic_locator_seen[locator.local_kind_index] = true;
    }
    validate_diagnostic(diagnostic, run.point_count);
    references = checked_add(
        references, diagnostic_reference_count(diagnostic),
        "a sparse direct H0 run reference count overflows size_t");
  }
  if (references != run.source_point_id_reference_count) {
    throw std::invalid_argument(
        "a sparse direct H0 input run has an invalid reference count");
  }
  if (std::find(
          higher_candidate_locator_seen.begin(),
          higher_candidate_locator_seen.end(), false) !=
          higher_candidate_locator_seen.end() ||
      std::find(
          higher_diagnostic_locator_seen.begin(),
          higher_diagnostic_locator_seen.end(), false) !=
          higher_diagnostic_locator_seen.end()) {
    throw std::invalid_argument(
        "a sparse direct H0 higher run omitted a source locator");
  }
}

void validate_projection_limits(
    const ExactSparseDirectH0CandidateRunLimits& limits) {
  if (limits.maximum_candidate_count == 0U ||
      limits.maximum_point_id_reference_count == 0U) {
    throw std::invalid_argument(
        "sparse direct H0 projection limits are invalid");
  }
}

[[nodiscard]] ExactSparseDirectH0Candidate pair_candidate(
    const ExactSparseAnchoredPairH0EventCandidate& source) {
  ExactSparseDirectH0Candidate result;
  result.source_locator = ExactSparseDirectH0PairSourceLocator{
      source.source_output_record_index};
  result.support_size = 2U;
  result.support_ids[0] = source.event.support_ids[0];
  result.support_ids[1] = source.event.support_ids[1];
  result.center = source.event.center;
  result.squared_level = source.event.squared_level;
  result.interior_ids = source.event.interior_ids;
  result.closed_rank = source.event.closed_rank;
  result.exterior_count = source.event.exterior_count;
  result.birth_order = source.birth_order;
  result.saddle_order = source.saddle_order;
  return result;
}

[[nodiscard]] ExactSparseDirectH0Diagnostic pair_diagnostic(
    const ExactSparseAnchoredPairH0Diagnostic& source) {
  ExactSparseDirectH0Diagnostic result;
  result.source_locator = ExactSparseDirectH0PairSourceLocator{
      source.source_output_record_index};
  result.support_size = 2U;
  result.support_ids[0] = source.diagnostic.support_ids[0];
  result.support_ids[1] = source.diagnostic.support_ids[1];
  result.center = source.diagnostic.center;
  result.squared_level = source.diagnostic.squared_level;
  result.interior_ids = source.diagnostic.interior_ids;
  result.shell_count = source.diagnostic.shell_count;
  result.canonical_extra_shell_witness_id =
      source.diagnostic.canonical_extra_shell_witness_id;
  result.minimum_possible_closed_rank =
      source.diagnostic.minimum_possible_closed_rank;
  result.observed_closed_rank = source.diagnostic.observed_closed_rank;
  result.exterior_count = source.diagnostic.exterior_count;
  return result;
}

[[nodiscard]] ExactSparseDirectH0Candidate higher_candidate(
    const ExactHigherSupportEvent& source,
    std::uint64_t chunk_sequence,
    std::size_t local_index,
    std::size_t effective_maximum_order) {
  ExactSparseDirectH0Candidate result;
  result.source_locator = ExactSparseDirectH0HigherSourceLocator{
      chunk_sequence, local_index};
  result.support_size = source.support_size;
  result.support_ids = source.support_ids;
  result.center = source.center;
  result.squared_level = source.squared_level;
  result.interior_ids = source.interior_ids;
  result.closed_rank = source.closed_rank;
  result.exterior_count = source.exterior_count;
  const auto [birth, saddle] = expected_roles(
      source.closed_rank, effective_maximum_order);
  result.birth_order = birth;
  result.saddle_order = saddle;
  return result;
}

[[nodiscard]] ExactSparseDirectH0Diagnostic higher_diagnostic(
    const ExactHigherSupportExtraShellDiagnostic& source,
    std::uint64_t chunk_sequence,
    std::size_t local_index) {
  ExactSparseDirectH0Diagnostic result;
  result.source_locator = ExactSparseDirectH0HigherSourceLocator{
      chunk_sequence, local_index};
  result.support_size = source.support_size;
  result.support_ids = source.support_ids;
  result.center = source.center;
  result.squared_level = source.squared_level;
  result.interior_ids = source.interior_ids;
  result.shell_count = source.shell_count;
  result.canonical_extra_shell_witness_id =
      source.canonical_extra_shell_witness_id;
  result.minimum_possible_closed_rank = source.minimum_possible_closed_rank;
  result.observed_closed_rank = source.observed_closed_rank;
  result.exterior_count = source.exterior_count;
  return result;
}

void validate_higher_projection_manifest(
    const ExactHigherSupportCheckpointManifest& manifest) {
  const bool order_contract_holds =
      manifest.requested_maximum_order != 0U &&
      manifest.requested_maximum_order <=
          higher_support_maximum_requested_order &&
      manifest.effective_maximum_order ==
          std::min(
              manifest.requested_maximum_order,
              manifest.point_count) &&
      manifest.maximum_relevant_closed_rank ==
          std::min(
              manifest.effective_maximum_order + 1U,
              manifest.point_count);
  if (manifest.schema_version != higher_support_checkpoint_schema_version ||
      manifest.traversal_version != higher_support_traversal_version ||
      manifest.point_count < 3U || !order_contract_holds) {
    throw std::invalid_argument(
        "a higher H0 projection manifest has an invalid local contract");
  }
}

void validate_higher_projection_segment(
    const ExactHigherSupportTerminalSegment& segment,
    const ExactSparseDirectH0CandidateRunLimits& limits) {
  const std::size_t retained_record_count = checked_add(
      segment.events.size(),
      segment.relevant_extra_shell_diagnostics.size(),
      "a higher H0 retained-record count overflows size_t");
  const std::size_t emitted_record_count = checked_add(
      retained_record_count,
      segment.destroyed_prune_certificate_count,
      "a higher H0 emitted-record count overflows size_t");
  if (segment.events.size() > limits.maximum_candidate_count ||
      segment.relevant_extra_shell_diagnostics.size() >
          limits.maximum_diagnostic_count ||
      segment.emitted_point_id_reference_count >
          limits.maximum_point_id_reference_count ||
      emitted_record_count != segment.emitted_record_count()) {
    throw std::invalid_argument(
        "a higher H0 segment exceeds its retained source contract");
  }
}

[[nodiscard]] ExactSparseDirectH0HigherSourceContract
make_higher_source_contract(
    const ExactHigherSupportCheckpointManifest& manifest,
    const ExactHigherSupportTerminalSegment& segment) {
  ExactSparseDirectH0HigherSourceContract result;
  result.manifest = manifest;
  result.chunk_sequence = segment.chunk_sequence;
  result.first_output_record_index = segment.first_output_record_index;
  result.source_checkpoint_digest = segment.source_checkpoint_digest;
  result.next_checkpoint_digest = segment.next_checkpoint_digest;
  result.previous_output_chain_digest =
      segment.previous_output_chain_digest;
  result.output_chain_digest = segment.output_chain_digest;
  result.status = segment.status;
  result.stop_reason = segment.stop_reason;
  result.source_event_count = segment.events.size();
  result.source_diagnostic_count =
      segment.relevant_extra_shell_diagnostics.size();
  result.emitted_record_count = segment.emitted_record_count();
  result.destroyed_prune_certificate_count =
      segment.destroyed_prune_certificate_count;
  result.destroyed_rank_receipt_count =
      segment.destroyed_rank_receipt_count;
  result.emitted_point_id_reference_count =
      segment.emitted_point_id_reference_count;
  return result;
}

[[nodiscard]] std::size_t validate_higher_event_source(
    const ExactHigherSupportEvent& event,
    std::size_t point_count,
    std::size_t effective_maximum_order) {
  const std::size_t support_size = event.support_size;
  const std::size_t expected_closed_rank = checked_add(
      support_size, event.interior_ids.size(),
      "a higher H0 source closed rank overflows size_t");
  const auto [birth, saddle] = expected_roles(
      expected_closed_rank, effective_maximum_order);
  if (event.support_size < 3U ||
      !valid_support_and_interior_ids(
          event.support_size, event.support_ids,
          event.interior_ids, point_count) ||
      event.squared_level.numerator() == 0 ||
      event.closed_rank != expected_closed_rank ||
      event.exterior_count != point_count - expected_closed_rank ||
      (!birth.has_value() && !saddle.has_value())) {
    throw std::invalid_argument(
        "a higher H0 source event is locally inconsistent");
  }
  return expected_closed_rank;
}

[[nodiscard]] std::size_t validate_higher_diagnostic_source(
    const ExactHigherSupportExtraShellDiagnostic& diagnostic,
    std::size_t point_count) {
  const std::size_t support_size = diagnostic.support_size;
  const std::size_t minimum_rank = checked_add(
      support_size, diagnostic.interior_ids.size(),
      "a higher H0 source diagnostic rank overflows size_t");
  const std::size_t observed_rank = checked_add(
      diagnostic.shell_count, diagnostic.interior_ids.size(),
      "a higher H0 source observed rank overflows size_t");
  if (diagnostic.support_size < 3U ||
      !valid_support_and_interior_ids(
          diagnostic.support_size, diagnostic.support_ids,
          diagnostic.interior_ids, point_count) ||
      diagnostic.squared_level.numerator() == 0 ||
      diagnostic.shell_count <= support_size ||
      diagnostic.canonical_extra_shell_witness_id >= point_count ||
      std::binary_search(
          diagnostic.support_ids.begin(),
          diagnostic.support_ids.begin() + support_size,
          diagnostic.canonical_extra_shell_witness_id) ||
      std::binary_search(
          diagnostic.interior_ids.begin(), diagnostic.interior_ids.end(),
          diagnostic.canonical_extra_shell_witness_id) ||
      diagnostic.minimum_possible_closed_rank != minimum_rank ||
      diagnostic.observed_closed_rank != observed_rank ||
      observed_rank > point_count ||
      diagnostic.exterior_count != point_count - observed_rank) {
    throw std::invalid_argument(
        "a higher H0 source diagnostic is locally inconsistent");
  }
  return checked_add(
      minimum_rank, 1U,
      "a higher H0 source diagnostic reference count overflows size_t");
}

[[nodiscard]] bool higher_support_less(
    std::uint8_t left_size,
    const std::array<spatial::PointId, 4U>& left_ids,
    std::uint8_t right_size,
    const std::array<spatial::PointId, 4U>& right_ids) {
  const std::size_t common = std::min(
      static_cast<std::size_t>(left_size),
      static_cast<std::size_t>(right_size));
  for (std::size_t index = 0U; index < common; ++index) {
    if (left_ids[index] != right_ids[index]) {
      return left_ids[index] < right_ids[index];
    }
  }
  return left_size < right_size;
}

[[nodiscard]] bool higher_event_less(
    const ExactHigherSupportEvent& left,
    const ExactHigherSupportEvent& right) {
  if (left.squared_level != right.squared_level) {
    return left.squared_level < right.squared_level;
  }
  if (left.closed_rank != right.closed_rank) {
    return left.closed_rank < right.closed_rank;
  }
  if (left.interior_ids != right.interior_ids) {
    return left.interior_ids < right.interior_ids;
  }
  if (higher_support_less(
          left.support_size, left.support_ids,
          right.support_size, right.support_ids)) {
    return true;
  }
  if (higher_support_less(
          right.support_size, right.support_ids,
          left.support_size, left.support_ids)) {
    return false;
  }
  return center_less(left.center, right.center);
}

void validate_merge_inputs(
    std::span<const ExactSparseDirectH0CandidateRun> input_runs,
    const ExactSparseDirectH0MergeLimits& limits) {
  if (input_runs.empty() || limits.maximum_input_run_count == 0U ||
      input_runs.size() > limits.maximum_input_run_count ||
      limits.maximum_output_candidate_count_per_page == 0U) {
    throw std::invalid_argument("sparse direct H0 merge limits are invalid");
  }
  const std::size_t point_count = input_runs.front().point_count;
  const std::size_t effective_maximum_order =
      input_runs.front().effective_maximum_order;
  const ExactHigherSupportCheckpointManifest* higher_manifest = nullptr;
  for (const ExactSparseDirectH0CandidateRun& run : input_runs) {
    validate_run(run);
    if (run.point_count != point_count ||
        run.effective_maximum_order != effective_maximum_order) {
      throw std::invalid_argument(
          "sparse direct H0 merge inputs have distinct contracts");
    }
    if (run.higher_source_contract) {
      const ExactHigherSupportCheckpointManifest& current =
          run.higher_source_contract->manifest;
      if (higher_manifest == nullptr) {
        higher_manifest = &current;
      } else if (*higher_manifest != current) {
        throw std::invalid_argument(
            "sparse direct H0 higher runs have distinct source manifests");
      }
    }
  }
}

void validate_cursor(
    std::span<const ExactSparseDirectH0CandidateRun> input_runs,
    const ExactSparseDirectH0MergeCursor& cursor) {
  if (cursor.next_candidate_indices.size() != input_runs.size()) {
    throw std::invalid_argument(
        "a sparse direct H0 merge cursor has the wrong fan-in");
  }
  std::size_t consumed = 0U;
  std::size_t equal_last_count = 0U;
  const ExactSparseDirectH0Candidate* last = nullptr;
  if (cursor.last_emitted_position.has_value()) {
    const ExactSparseDirectH0MergeSourcePosition position =
        *cursor.last_emitted_position;
    if (position.run_index >= input_runs.size() ||
        position.candidate_index >=
            input_runs[position.run_index].candidates.size()) {
      throw std::invalid_argument(
          "a sparse direct H0 merge cursor has an invalid last position");
    }
    last = &input_runs[position.run_index]
                .candidates[position.candidate_index];
  }
  for (std::size_t run_index = 0U; run_index < input_runs.size(); ++run_index) {
    const auto& candidates = input_runs[run_index].candidates;
    const std::size_t next = cursor.next_candidate_indices[run_index];
    if (next > candidates.size()) {
      throw std::invalid_argument(
          "a sparse direct H0 merge cursor exceeds an input run");
    }
    consumed = checked_add(
        consumed, next,
        "a sparse direct H0 merge cursor count overflows size_t");
    if (last == nullptr) {
      continue;
    }
    if (next != 0U) {
      const auto& previous = candidates[next - 1U];
      if (exact_sparse_direct_h0_candidate_less(*last, previous)) {
        throw std::invalid_argument(
            "a sparse direct H0 merge cursor skipped a larger predecessor");
      }
      if (exact_sparse_direct_h0_candidate_key_equal(previous, *last)) {
        ++equal_last_count;
      }
    }
  }
  if (consumed != cursor.emitted_candidate_count ||
      (consumed == 0U) != !cursor.last_emitted_position.has_value() ||
      (consumed != 0U && equal_last_count != 1U)) {
    throw std::invalid_argument(
        "a sparse direct H0 merge cursor has inconsistent accounting");
  }
}

}  // namespace

bool exact_sparse_direct_h0_candidate_less(
    const ExactSparseDirectH0Candidate& left,
    const ExactSparseDirectH0Candidate& right) {
  if (left.squared_level != right.squared_level) {
    return left.squared_level < right.squared_level;
  }
  if (left.closed_rank != right.closed_rank) {
    return left.closed_rank < right.closed_rank;
  }
  if (left.interior_ids != right.interior_ids) {
    return left.interior_ids < right.interior_ids;
  }
  if (support_less(left, right)) {
    return true;
  }
  if (support_less(right, left)) {
    return false;
  }
  return center_less(left.center, right.center);
}

bool exact_sparse_direct_h0_candidate_key_equal(
    const ExactSparseDirectH0Candidate& left,
    const ExactSparseDirectH0Candidate& right) {
  return !exact_sparse_direct_h0_candidate_less(left, right) &&
      !exact_sparse_direct_h0_candidate_less(right, left);
}

ExactSparseDirectH0CandidateRun
project_exact_sparse_direct_h0_pair_candidate_run(
    const ExactSparseAnchoredPairH0Run& source,
    ExactSparseDirectH0CandidateRunLimits limits) {
  validate_projection_limits(limits);
  if (source.schema_version != sparse_anchored_pair_h0_run_schema_version ||
      !source.pair_only ||
      !source.candidates_sorted_by_terminal_facade_event_key ||
      !source.diagnostics_preserved || source.global_event_indices_assigned ||
      source.supports_three_and_four_joined ||
      source.complete_h0_authority_claimed ||
      source.event_candidates.size() > limits.maximum_candidate_count ||
      source.diagnostics.size() > limits.maximum_diagnostic_count ||
      source.source_point_id_reference_count >
          limits.maximum_point_id_reference_count ||
      checked_add(
          source.event_candidates.size(), source.diagnostics.size(),
          "a pair H0 retained-record count overflows size_t") !=
          source.source_record_count) {
    throw std::invalid_argument(
        "a pair H0 source run cannot be projected into the common format");
  }

  std::vector<bool> locator_seen(source.source_record_count, false);
  const auto mark_locator = [&](std::size_t locator) {
    if (locator < source.first_source_output_record_index) {
      throw std::invalid_argument("a pair H0 source locator moved backwards");
    }
    const std::size_t local =
        locator - source.first_source_output_record_index;
    if (local >= locator_seen.size() || locator_seen[local]) {
      throw std::invalid_argument(
          "a pair H0 source locator is duplicated or out of range");
    }
    locator_seen[local] = true;
  };

  ExactSparseDirectH0CandidateRun result;
  result.point_count = source.point_count;
  result.effective_maximum_order = source.effective_maximum_order;
  result.source_point_id_reference_count =
      source.source_point_id_reference_count;
  result.candidates.reserve(source.event_candidates.size());
  result.diagnostics.reserve(source.diagnostics.size());
  for (const auto& event : source.event_candidates) {
    mark_locator(event.source_output_record_index);
    result.candidates.push_back(pair_candidate(event));
  }
  for (const auto& diagnostic : source.diagnostics) {
    mark_locator(diagnostic.source_output_record_index);
    result.diagnostics.push_back(pair_diagnostic(diagnostic));
  }
  if (std::find(locator_seen.begin(), locator_seen.end(), false) !=
      locator_seen.end()) {
    throw std::invalid_argument("a pair H0 source run omitted a locator");
  }
  result.contains_pair_candidates = true;
  result.diagnostics_preserved = true;
  sort_and_validate_candidates(result);
  validate_run(result);
  return result;
}

ExactSparseDirectH0CandidateRun
project_exact_sparse_direct_h0_higher_candidate_run(
    const ExactHigherSupportTerminalAuthority& authority,
    std::size_t segment_index,
    ExactSparseDirectH0CandidateRunLimits limits) {
  validate_projection_limits(limits);
  if (!authority.sealed_in_process_terminal_authority()) {
    throw std::invalid_argument(
        "a common higher H0 projection requires a terminal authority");
  }
  const ExactHigherSupportCheckpointManifest& manifest = authority.manifest();
  validate_higher_projection_manifest(manifest);
  if (!authority.bound_to(
          authority.index(), authority.cloud(),
          manifest.requested_maximum_order) ||
      segment_index >= authority.segments().size()) {
    throw std::invalid_argument(
        "a common higher H0 projection has an invalid source binding");
  }
  const ExactHigherSupportTerminalSegment& segment =
      authority.segments()[segment_index];
  validate_higher_projection_segment(segment, limits);
  if (segment.chunk_sequence != segment_index) {
    throw std::invalid_argument(
        "a higher H0 resident segment has an invalid sequence");
  }

  ExactSparseDirectH0CandidateRun result;
  result.point_count = manifest.point_count;
  result.effective_maximum_order = manifest.effective_maximum_order;
  result.higher_source_contract =
      make_higher_source_contract(manifest, segment);
  result.candidates.reserve(segment.events.size());
  result.diagnostics.reserve(
      segment.relevant_extra_shell_diagnostics.size());
  std::size_t retained_reference_count = 0U;
  for (std::size_t index = 0U; index < segment.events.size(); ++index) {
    ExactSparseDirectH0Candidate candidate = higher_candidate(
        segment.events[index], segment.chunk_sequence, index,
        manifest.effective_maximum_order);
    retained_reference_count = checked_add(
        retained_reference_count, candidate_reference_count(candidate),
        "a retained higher H0 reference count overflows size_t");
    result.candidates.push_back(std::move(candidate));
  }
  for (std::size_t index = 0U;
       index < segment.relevant_extra_shell_diagnostics.size(); ++index) {
    ExactSparseDirectH0Diagnostic diagnostic = higher_diagnostic(
        segment.relevant_extra_shell_diagnostics[index],
        segment.chunk_sequence, index);
    retained_reference_count = checked_add(
        retained_reference_count, diagnostic_reference_count(diagnostic),
        "a retained higher H0 reference count overflows size_t");
    result.diagnostics.push_back(std::move(diagnostic));
  }
  if (retained_reference_count !=
      segment.emitted_point_id_reference_count) {
    throw std::invalid_argument(
        "a higher H0 segment retained a different reference count than it emitted");
  }
  result.source_point_id_reference_count = retained_reference_count;
  result.contains_higher_candidates = true;
  result.diagnostics_preserved = true;
  sort_and_validate_candidates(result);
  validate_run(result);
  return result;
}

ExactSparseDirectH0CandidateRun
project_exact_sparse_direct_h0_higher_candidate_run(
    ExactHigherSupportUnsealedDrain&& source,
    ExactSparseDirectH0CandidateRunLimits limits) {
  static_assert(
      std::is_nothrow_move_assignable_v<exact::ExactCenter3> &&
          std::is_nothrow_move_assignable_v<exact::ExactLevel> &&
          std::is_nothrow_move_assignable_v<
              std::vector<spatial::PointId>>,
      "the higher H0 projection commit requires nothrow payload moves");
  static_assert(
      std::is_nothrow_move_constructible_v<
          ExactSparseDirectH0CandidateRun>,
      "the higher H0 projection return requires a nothrow run move");

  if (source.status_ !=
          ExactHigherSupportUnsealedDrainStatus::segment_ready ||
      !source.segment_available()) {
    throw std::invalid_argument(
        "a consuming higher H0 projection requires one available drained segment");
  }
  ExactHigherSupportTerminalSegment& segment = *source.segment_;
  const ExactHigherSupportCheckpointManifest& manifest = *source.manifest_;
  validate_projection_limits(limits);
  validate_higher_projection_manifest(manifest);
  validate_higher_projection_segment(segment, limits);

  // Everything that can allocate, compare exact integers or reject the local
  // contract is completed before the first payload move.  Consequently any
  // exception above or below this point leaves the opaque source retryable.
  std::size_t retained_reference_count = 0U;
  for (const ExactHigherSupportEvent& event : segment.events) {
    retained_reference_count = checked_add(
        retained_reference_count,
        validate_higher_event_source(
            event, manifest.point_count,
            manifest.effective_maximum_order),
        "a retained higher H0 reference count overflows size_t");
  }
  for (const ExactHigherSupportExtraShellDiagnostic& diagnostic :
       segment.relevant_extra_shell_diagnostics) {
    retained_reference_count = checked_add(
        retained_reference_count,
        validate_higher_diagnostic_source(
            diagnostic, manifest.point_count),
        "a retained higher H0 reference count overflows size_t");
  }
  if (retained_reference_count !=
      segment.emitted_point_id_reference_count) {
    throw std::invalid_argument(
        "a higher H0 segment retained a different reference count than it emitted");
  }

  std::vector<std::size_t> candidate_order(segment.events.size());
  std::iota(candidate_order.begin(), candidate_order.end(), 0U);
  std::sort(
      candidate_order.begin(), candidate_order.end(),
      [&segment](std::size_t left, std::size_t right) {
        return higher_event_less(
            segment.events[left], segment.events[right]);
      });
  for (std::size_t index = 1U; index < candidate_order.size(); ++index) {
    if (!higher_event_less(
            segment.events[candidate_order[index - 1U]],
            segment.events[candidate_order[index]])) {
      throw std::invalid_argument(
          "a higher H0 segment contains duplicate canonical candidates");
    }
  }

  ExactSparseDirectH0CandidateRun result;
  result.point_count = manifest.point_count;
  result.effective_maximum_order = manifest.effective_maximum_order;
  result.source_point_id_reference_count = retained_reference_count;
  result.higher_source_contract =
      make_higher_source_contract(manifest, segment);
  result.candidates.resize(segment.events.size());
  result.diagnostics.resize(
      segment.relevant_extra_shell_diagnostics.size());
  for (std::size_t output_index = 0U;
       output_index < candidate_order.size(); ++output_index) {
    const std::size_t source_index = candidate_order[output_index];
    const ExactHigherSupportEvent& event = segment.events[source_index];
    ExactSparseDirectH0Candidate& candidate =
        result.candidates[output_index];
    candidate.source_locator = ExactSparseDirectH0HigherSourceLocator{
        segment.chunk_sequence, source_index};
    candidate.support_size = event.support_size;
    candidate.support_ids = event.support_ids;
    candidate.closed_rank = event.closed_rank;
    candidate.exterior_count = event.exterior_count;
    const auto [birth, saddle] = expected_roles(
        event.closed_rank, manifest.effective_maximum_order);
    candidate.birth_order = birth;
    candidate.saddle_order = saddle;
  }
  for (std::size_t index = 0U;
       index < segment.relevant_extra_shell_diagnostics.size(); ++index) {
    const ExactHigherSupportExtraShellDiagnostic& source_diagnostic =
        segment.relevant_extra_shell_diagnostics[index];
    ExactSparseDirectH0Diagnostic& diagnostic = result.diagnostics[index];
    diagnostic.source_locator = ExactSparseDirectH0HigherSourceLocator{
        segment.chunk_sequence, index};
    diagnostic.support_size = source_diagnostic.support_size;
    diagnostic.support_ids = source_diagnostic.support_ids;
    diagnostic.shell_count = source_diagnostic.shell_count;
    diagnostic.canonical_extra_shell_witness_id =
        source_diagnostic.canonical_extra_shell_witness_id;
    diagnostic.minimum_possible_closed_rank =
        source_diagnostic.minimum_possible_closed_rank;
    diagnostic.observed_closed_rank =
        source_diagnostic.observed_closed_rank;
    diagnostic.exterior_count = source_diagnostic.exterior_count;
  }

  // Transaction commit: only statically checked nothrow moves and scalar
  // stores remain.  The token is revoked only after every payload is owned by
  // the destination run.
  for (std::size_t output_index = 0U;
       output_index < candidate_order.size(); ++output_index) {
    ExactHigherSupportEvent& event =
        segment.events[candidate_order[output_index]];
    ExactSparseDirectH0Candidate& candidate =
        result.candidates[output_index];
    candidate.center = std::move(event.center);
    candidate.squared_level = std::move(event.squared_level);
    candidate.interior_ids = std::move(event.interior_ids);
  }
  for (std::size_t index = 0U;
       index < segment.relevant_extra_shell_diagnostics.size(); ++index) {
    ExactHigherSupportExtraShellDiagnostic& source_diagnostic =
        segment.relevant_extra_shell_diagnostics[index];
    ExactSparseDirectH0Diagnostic& diagnostic = result.diagnostics[index];
    diagnostic.center = std::move(source_diagnostic.center);
    diagnostic.squared_level =
        std::move(source_diagnostic.squared_level);
    diagnostic.interior_ids =
        std::move(source_diagnostic.interior_ids);
  }
  result.contains_higher_candidates = true;
  result.candidates_strictly_sorted_by_terminal_facade_key = true;
  result.diagnostics_preserved = true;
  source.consume();
  return result;
}

ExactSparseDirectH0CandidateMergeSession::
ExactSparseDirectH0CandidateMergeSession(
    std::vector<ExactSparseDirectH0CandidateRun>&& input_runs,
    ExactSparseDirectH0MergeLimits limits)
    : input_runs_(std::move(input_runs)), limits_(limits) {
  validate_merge_inputs(input_runs_, limits_);
  audit_.input_validation_pass_count = 1U;
  audit_.validated_run_count = input_runs_.size();
  point_count_ = input_runs_.front().point_count;
  effective_maximum_order_ =
      input_runs_.front().effective_maximum_order;
  cursor_.next_candidate_indices.assign(input_runs_.size(), 0U);
  for (const ExactSparseDirectH0CandidateRun& run : input_runs_) {
    total_candidate_count_ = checked_add(
        total_candidate_count_, run.candidates.size(),
        "a sparse direct H0 merge total count overflows size_t");
    audit_.validated_candidate_count = checked_add(
        audit_.validated_candidate_count, run.candidates.size(),
        "a sparse direct H0 merge audit count overflows size_t");
    audit_.validated_diagnostic_count = checked_add(
        audit_.validated_diagnostic_count, run.diagnostics.size(),
        "a sparse direct H0 merge audit count overflows size_t");
  }
  validate_cursor(input_runs_, cursor_);
}

ExactSparseDirectH0MergedPage
ExactSparseDirectH0CandidateMergeSession::merge_next_page() & {
  if (poisoned_) {
    throw std::logic_error(
        "a poisoned sparse direct H0 merge session cannot advance");
  }
  if (complete()) {
    throw std::logic_error(
        "a complete sparse direct H0 merge session cannot advance");
  }
  validate_cursor(input_runs_, cursor_);

  struct Head {
    std::size_t run_index{};
    std::size_t candidate_index{};
  };
  const auto greater = [this](const Head& left, const Head& right) {
    const auto& left_candidate =
        input_runs_[left.run_index].candidates[left.candidate_index];
    const auto& right_candidate =
        input_runs_[right.run_index].candidates[right.candidate_index];
    if (exact_sparse_direct_h0_candidate_less(
            right_candidate, left_candidate)) {
      return true;
    }
    if (exact_sparse_direct_h0_candidate_less(
            left_candidate, right_candidate)) {
      return false;
    }
    return left.run_index > right.run_index;
  };
  std::vector<Head> heap;
  heap.reserve(input_runs_.size());
  for (std::size_t run_index = 0U; run_index < input_runs_.size(); ++run_index) {
    const std::size_t next = cursor_.next_candidate_indices[run_index];
    if (next != input_runs_[run_index].candidates.size()) {
      heap.push_back(Head{run_index, next});
    }
  }
  std::make_heap(heap.begin(), heap.end(), greater);

  ExactSparseDirectH0MergedPage page;
  page.point_count = point_count_;
  page.effective_maximum_order = effective_maximum_order_;
  ExactSparseDirectH0MergeCursor next_cursor = cursor_;
  page.candidates.reserve(
      limits_.maximum_output_candidate_count_per_page);
  while (!heap.empty() &&
         page.candidates.size() <
             limits_.maximum_output_candidate_count_per_page) {
    std::pop_heap(heap.begin(), heap.end(), greater);
    const Head head = heap.back();
    heap.pop_back();
    const ExactSparseDirectH0Candidate& candidate =
        input_runs_[head.run_index].candidates[head.candidate_index];
    const ExactSparseDirectH0Candidate* last = nullptr;
    if (next_cursor.last_emitted_position.has_value()) {
      const auto position = *next_cursor.last_emitted_position;
      last = &input_runs_[position.run_index]
                  .candidates[position.candidate_index];
    }
    if (last != nullptr &&
        !exact_sparse_direct_h0_candidate_less(
            *last, candidate)) {
      poisoned_ = true;
      throw std::invalid_argument(
          exact_sparse_direct_h0_candidate_key_equal(
              *last, candidate)
              ? "a sparse direct H0 merge found a duplicate across runs or pages"
              : "a sparse direct H0 merge lost canonical monotonicity");
    }
    page.candidates.push_back(candidate);
    next_cursor.last_emitted_position =
        ExactSparseDirectH0MergeSourcePosition{
            head.run_index, head.candidate_index};
    next_cursor.emitted_candidate_count = checked_add(
        next_cursor.emitted_candidate_count, 1U,
        "a sparse direct H0 merged count overflows size_t");
    const std::size_t next_index = head.candidate_index + 1U;
    next_cursor.next_candidate_indices[head.run_index] = next_index;
    if (next_index != input_runs_[head.run_index].candidates.size()) {
      heap.push_back(Head{head.run_index, next_index});
      std::push_heap(heap.begin(), heap.end(), greater);
    }
  }

  page.merge_complete =
      next_cursor.emitted_candidate_count == total_candidate_count_;
  page.candidates_strictly_sorted_by_terminal_facade_key = true;
  validate_cursor(input_runs_, next_cursor);
  page.successor_cursor = next_cursor;
  cursor_ = std::move(next_cursor);
  audit_.emitted_page_count = checked_add(
      audit_.emitted_page_count, 1U,
      "a sparse direct H0 merge page count overflows size_t");
  audit_.emitted_candidate_count = cursor_.emitted_candidate_count;
  return page;
}

}  // namespace morsehgp3d::hierarchy
