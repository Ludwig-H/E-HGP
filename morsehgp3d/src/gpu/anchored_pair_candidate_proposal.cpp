#include "morsehgp3d/gpu/anchored_pair_candidate_proposal.hpp"

#include "../cuda/phase14_anchored_pair_candidate_proposal_internal.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::detail {

class Phase14AnchoredPairCandidateProposalHostState final {
 public:
  std::shared_ptr<const void> cloud_identity;
  std::shared_ptr<void> adopted_owner;
  const std::uint64_t* device_coordinate_bits{};
  const std::uint64_t* device_morton_point_ids{};
  const void* device_nodes{};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t retained_coordinate_word_capacity{};
  std::size_t retained_morton_point_id_capacity{};
  std::size_t retained_node_capacity{};
  std::size_t persistent_device_byte_capacity{};
  std::uint64_t source_snapshot_epoch{};
  int cuda_device{-1};
  bool cuda_resident{false};
  bool host_fake{false};

  [[nodiscard]] Phase14AnchoredPairCandidateAdoptedTraversal
  adopted_traversal() const {
    if (!adopted_owner || !cloud_identity) {
      throw std::logic_error(
          "the Phase 14 anchored-pair context has no traversal owner");
    }
    return Phase14AnchoredPairCandidateAdoptedTraversal{
        adopted_owner,
        device_coordinate_bits,
        device_morton_point_ids,
        device_nodes,
        retained_coordinate_word_capacity,
        retained_morton_point_id_capacity,
        retained_node_capacity,
        point_count,
        certified_node_count,
        source_snapshot_epoch,
        cuda_device,
        cuda_resident,
        host_fake};
  }
};

}  // namespace morsehgp3d::gpu::detail

namespace morsehgp3d::gpu {
namespace {

using DeviceBatch =
    detail::Phase14AnchoredPairCandidateDeviceBatch;
using DeviceRecord =
    detail::Phase14AnchoredPairCandidateTranscriptRecord;
using DeviceSegment =
    detail::Phase14AnchoredPairCandidateSegmentRecord;
using InputRecord =
    detail::Phase14AnchoredPairCandidateQueryInputRecord;

constexpr std::uint64_t kFnvOffsetBasis =
    UINT64_C(14695981039346656037);

[[nodiscard]] std::size_t checked_sum(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_product(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::length_error(message);
  }
  return left * right;
}

[[nodiscard]] std::size_t checked_node_count(
    std::size_t point_count) {
  if (point_count == 0U ||
      point_count >
          std::numeric_limits<std::size_t>::max() / 2U + 1U) {
    throw std::length_error(
        "the anchored-pair proposal node count overflows size_t");
  }
  return 2U * point_count - 1U;
}

[[nodiscard]] std::uint64_t checked_u64(
    std::size_t value,
    const char* message) {
  if (!std::in_range<std::uint64_t>(value)) {
    throw std::length_error(message);
  }
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::size_t checked_size(
    std::uint64_t value,
    const char* message) {
  if (!std::in_range<std::size_t>(value)) {
    throw std::runtime_error(message);
  }
  return static_cast<std::size_t>(value);
}

[[nodiscard]] std::uint64_t query_digest(
    spatial::PointId anchor_point_id,
    std::size_t maximum_closed_rank,
    std::span<const spatial::PointId> witness_bank_point_ids,
    std::uint64_t source_snapshot_epoch) {
  std::uint64_t digest = kFnvOffsetBasis;
  for (const std::uint64_t word :
       {UINT64_C(0x5038645155455259),
        static_cast<std::uint64_t>(
            anchored_pair_candidate_gpu_proposal_schema_version),
        source_snapshot_epoch,
        static_cast<std::uint64_t>(anchor_point_id),
        checked_u64(
            maximum_closed_rank,
            "an anchored-pair rank does not fit uint64"),
        checked_u64(
            witness_bank_point_ids.size(),
            "an anchored-pair bank size does not fit uint64")}) {
    digest = detail::phase14_anchored_pair_candidate_digest_word(
        digest, word);
  }
  for (const spatial::PointId witness : witness_bank_point_ids) {
    digest = detail::phase14_anchored_pair_candidate_digest_word(
        digest, static_cast<std::uint64_t>(witness));
  }
  return digest;
}

[[nodiscard]] std::uint64_t encode_cursor_kind(
    AnchoredPairCandidateProposalCursorKind kind) {
  switch (kind) {
    case AnchoredPairCandidateProposalCursorKind::initial:
      return static_cast<std::uint64_t>(
          detail::Phase14AnchoredPairCandidateCursorKind::initial);
    case AnchoredPairCandidateProposalCursorKind::active:
      return static_cast<std::uint64_t>(
          detail::Phase14AnchoredPairCandidateCursorKind::active);
    case AnchoredPairCandidateProposalCursorKind::complete:
      return static_cast<std::uint64_t>(
          detail::Phase14AnchoredPairCandidateCursorKind::complete);
  }
  throw std::invalid_argument(
      "an anchored-pair proposal cursor has an invalid kind");
}

[[nodiscard]] AnchoredPairCandidateProposalCursorKind decode_cursor_kind(
    std::uint64_t kind) {
  if (kind == static_cast<std::uint64_t>(
                  detail::Phase14AnchoredPairCandidateCursorKind::initial)) {
    return AnchoredPairCandidateProposalCursorKind::initial;
  }
  if (kind == static_cast<std::uint64_t>(
                  detail::Phase14AnchoredPairCandidateCursorKind::active)) {
    return AnchoredPairCandidateProposalCursorKind::active;
  }
  if (kind == static_cast<std::uint64_t>(
                  detail::Phase14AnchoredPairCandidateCursorKind::complete)) {
    return AnchoredPairCandidateProposalCursorKind::complete;
  }
  throw std::runtime_error(
      "the anchored-pair device transcript has an invalid cursor kind");
}

[[nodiscard]] AnchoredPairCandidateProposalSegmentStatus decode_status(
    std::uint64_t status) {
  if (status == static_cast<std::uint64_t>(
                    detail::Phase14AnchoredPairCandidateSegmentStatus::
                        complete)) {
    return AnchoredPairCandidateProposalSegmentStatus::complete;
  }
  if (status == static_cast<std::uint64_t>(
                    detail::Phase14AnchoredPairCandidateSegmentStatus::
                        budget_exhausted)) {
    return AnchoredPairCandidateProposalSegmentStatus::budget_exhausted;
  }
  if (status == static_cast<std::uint64_t>(
                    detail::Phase14AnchoredPairCandidateSegmentStatus::
                        capacity_exhausted)) {
    return AnchoredPairCandidateProposalSegmentStatus::capacity_exhausted;
  }
  throw std::runtime_error(
      "the anchored-pair device transcript has an invalid segment status");
}

[[nodiscard]] AnchoredPairCandidateProposalStopReason decode_stop_reason(
    std::uint64_t reason) {
  if (reason == static_cast<std::uint64_t>(
                    detail::Phase14AnchoredPairCandidateStopReason::none)) {
    return AnchoredPairCandidateProposalStopReason::none;
  }
  if (reason == static_cast<std::uint64_t>(
                    detail::Phase14AnchoredPairCandidateStopReason::
                        node_visit_limit)) {
    return AnchoredPairCandidateProposalStopReason::node_visit_limit;
  }
  if (reason == static_cast<std::uint64_t>(
                    detail::Phase14AnchoredPairCandidateStopReason::
                        transcript_record_limit)) {
    return AnchoredPairCandidateProposalStopReason::transcript_record_limit;
  }
  throw std::runtime_error(
      "the anchored-pair device transcript has an invalid stop reason");
}

struct PackedQueries {
  std::vector<InputRecord> records;
  std::vector<std::uint64_t> query_digests;
  std::size_t witness_reference_count{};
  std::size_t maximum_witness_bank_size{};
};

void validate_budget(
    AnchoredPairCandidateProposalBudget budget,
    std::size_t maximum_transcript_record_count) {
  if (budget.maximum_node_visit_count_per_query == 0U) {
    throw std::invalid_argument(
        "an anchored-pair proposal requires a nonzero node-visit budget");
  }
  if (budget.maximum_transcript_record_count_per_query == 0U ||
      budget.maximum_total_transcript_record_count == 0U ||
      budget.maximum_total_transcript_record_count >
          maximum_transcript_record_count ||
      budget.maximum_transcript_record_count_per_query >
          maximum_transcript_record_count) {
    throw std::invalid_argument(
        "an anchored-pair proposal transcript budget exceeds its fixed "
        "capacity");
  }
  static_cast<void>(checked_u64(
      budget.maximum_node_visit_count_per_query,
      "the anchored-pair node budget does not fit uint64"));
  static_cast<void>(checked_u64(
      budget.maximum_transcript_record_count_per_query,
      "the anchored-pair record budget does not fit uint64"));
  static_cast<void>(checked_u64(
      budget.maximum_total_transcript_record_count,
      "the anchored-pair total record budget does not fit uint64"));
}

[[nodiscard]] PackedQueries validate_and_pack_queries(
    const detail::Phase14AnchoredPairCandidateProposalHostState& host,
    std::span<const AnchoredPairCandidateProposalQuery> queries,
    std::size_t maximum_query_count,
    AnchoredPairCandidateProposalBudget budget) {
  if (queries.size() > maximum_query_count) {
    throw std::invalid_argument(
        "an anchored-pair proposal batch exceeds its fixed query capacity");
  }
  PackedQueries packed;
  packed.records.reserve(queries.size());
  packed.query_digests.reserve(queries.size());
  for (std::size_t query_index = 0U;
       query_index < queries.size();
       ++query_index) {
    const AnchoredPairCandidateProposalQuery& query =
        queries[query_index];
    if (query.anchor_point_id >= host.point_count ||
        (query_index != 0U &&
         queries[query_index - 1U].anchor_point_id >=
             query.anchor_point_id)) {
      throw std::invalid_argument(
          "anchored-pair proposal anchors must be strictly increasing and "
          "in domain");
    }
    if (query.maximum_closed_rank < 2U ||
        query.maximum_closed_rank >
            anchored_pair_candidate_gpu_maximum_closed_rank) {
      throw std::out_of_range(
          "an anchored-pair proposal closed-rank bound must be in [2, 11]");
    }
    if (query.witness_bank_point_ids.size() >
        anchored_pair_candidate_gpu_maximum_witness_bank_size) {
      throw std::length_error(
          "an anchored-pair proposal witness bank exceeds 64 entries");
    }
    for (std::size_t witness_index = 0U;
         witness_index < query.witness_bank_point_ids.size();
         ++witness_index) {
      const spatial::PointId witness =
          query.witness_bank_point_ids[witness_index];
      if (witness >= host.point_count ||
          witness == query.anchor_point_id) {
        throw std::invalid_argument(
            "an anchored-pair proposal witness bank is not in domain or "
            "contains its anchor");
      }
      for (std::size_t prior_index = 0U;
           prior_index < witness_index;
           ++prior_index) {
        if (query.witness_bank_point_ids[prior_index] == witness) {
          throw std::invalid_argument(
              "an anchored-pair proposal witness bank contains duplicate "
              "PointIds");
        }
      }
    }
    packed.witness_reference_count = checked_sum(
        packed.witness_reference_count,
        query.witness_bank_point_ids.size(),
        "the anchored-pair witness-reference count overflows size_t");
    packed.maximum_witness_bank_size = std::max(
        packed.maximum_witness_bank_size,
        query.witness_bank_point_ids.size());

    const std::uint64_t digest = query_digest(
        query.anchor_point_id,
        query.maximum_closed_rank,
        query.witness_bank_point_ids,
        host.source_snapshot_epoch);
    const AnchoredPairCandidateProposalCursor& cursor = query.cursor;
    switch (cursor.kind) {
      case AnchoredPairCandidateProposalCursorKind::initial:
        if (cursor.next_node_index !=
                anchored_pair_candidate_gpu_invalid_node_index ||
            cursor.query_digest_fnv1a != 0U ||
            cursor.source_snapshot_epoch != 0U) {
          throw std::invalid_argument(
              "an initial anchored-pair cursor has noninitial state");
        }
        break;
      case AnchoredPairCandidateProposalCursorKind::active:
        if (cursor.next_node_index >= host.certified_node_count ||
            cursor.query_digest_fnv1a != digest ||
            cursor.source_snapshot_epoch !=
                host.source_snapshot_epoch) {
          throw std::invalid_argument(
              "an active anchored-pair cursor is stale, foreign or out of "
              "range");
        }
        break;
      case AnchoredPairCandidateProposalCursorKind::complete:
        throw std::invalid_argument(
            "a complete anchored-pair cursor cannot be resubmitted");
    }

    InputRecord input;
    input.query_index = checked_u64(
        query_index,
        "an anchored-pair query index does not fit uint64");
    input.query_digest_fnv1a = digest;
    input.anchor_point_id =
        static_cast<std::uint64_t>(query.anchor_point_id);
    input.maximum_closed_rank = checked_u64(
        query.maximum_closed_rank,
        "an anchored-pair rank does not fit uint64");
    input.witness_count = checked_u64(
        query.witness_bank_point_ids.size(),
        "an anchored-pair bank size does not fit uint64");
    input.cursor_kind = encode_cursor_kind(cursor.kind);
    input.cursor_node_index = cursor.next_node_index;
    input.cursor_query_digest_fnv1a = cursor.query_digest_fnv1a;
    input.cursor_source_snapshot_epoch = cursor.source_snapshot_epoch;
    input.maximum_node_visit_count = checked_u64(
        budget.maximum_node_visit_count_per_query,
        "an anchored-pair node budget does not fit uint64");
    input.maximum_transcript_record_count = checked_u64(
        budget.maximum_transcript_record_count_per_query,
        "an anchored-pair record budget does not fit uint64");
    std::fill(
        std::begin(input.witnesses),
        std::end(input.witnesses),
        detail::phase14_anchored_pair_candidate_sentinel);
    std::copy(
        query.witness_bank_point_ids.begin(),
        query.witness_bank_point_ids.end(),
        std::begin(input.witnesses));
    packed.records.push_back(input);
    packed.query_digests.push_back(digest);
  }
  return packed;
}

void initialize_common_audit(
    AnchoredPairCandidateProposalAudit& audit,
    const detail::Phase14AnchoredPairCandidateProposalHostState& host,
    std::size_t maximum_query_count,
    std::size_t maximum_transcript_record_count,
    std::size_t active_total_transcript_record_limit,
    const PackedQueries& packed) {
  audit.maximum_query_count = maximum_query_count;
  audit.physical_transcript_record_capacity =
      maximum_transcript_record_count;
  audit.active_total_transcript_record_limit =
      active_total_transcript_record_limit;
  audit.point_count = host.point_count;
  audit.certified_node_count = host.certified_node_count;
  audit.retained_coordinate_word_capacity =
      host.retained_coordinate_word_capacity;
  audit.retained_morton_point_id_capacity =
      host.retained_morton_point_id_capacity;
  audit.retained_node_capacity = host.retained_node_capacity;
  audit.persistent_device_byte_capacity =
      host.persistent_device_byte_capacity;
  audit.source_snapshot_epoch = host.source_snapshot_epoch;
  audit.canonical_query_count = packed.records.size();
  audit.witness_point_id_reference_count =
      packed.witness_reference_count;
  audit.maximum_observed_witness_bank_size =
      packed.maximum_witness_bank_size;
  audit.cuda_device = host.cuda_device;
  audit.matching_immutable_point_namespace_validated = true;
  audit.traversal_lease_owner_retained =
      host.adopted_owner != nullptr;
  audit.traversal_lease_device_validated =
      host.cuda_resident != host.host_fake;
  audit.traversal_lease_extents_validated = true;
  audit.canonical_query_order_validated = true;
  audit.witness_banks_validated = true;
  audit.input_cursors_validated = true;
  audit.fixed_capacity_preflight_satisfied = true;
  audit.fixed_resident_capacity_bound_at_construction = true;
}

void validate_execution_metadata(
    const DeviceBatch& batch,
    const detail::Phase14AnchoredPairCandidateProposalHostState& host) {
  switch (batch.execution_kind) {
    case detail::Phase14AnchoredPairCandidateExecutionKind::host_fake:
      if (!host.host_fake || host.cuda_resident ||
          batch.cuda_path_qualified || batch.cuda_device != -1 ||
          batch.kernel_launch_count != 0U ||
          batch.compaction_kernel_launch_count != 0U ||
          batch.synchronization_count != 0U ||
          batch.compaction_kind !=
              detail::Phase14AnchoredPairCandidateCompactionKind::
                  host_fake) {
        throw std::runtime_error(
            "the anchored-pair host-fake launcher returned CUDA metadata");
      }
      return;
    case detail::Phase14AnchoredPairCandidateExecutionKind::cuda:
      if (!host.cuda_resident || host.host_fake ||
          !batch.cuda_path_qualified ||
          batch.cuda_device != host.cuda_device ||
          batch.kernel_launch_count < 2U ||
          batch.kernel_launch_count >
              detail::
                  phase14_anchored_pair_candidate_maximum_cuda_kernel_launch_count ||
          batch.compaction_kernel_launch_count == 0U ||
          batch.compaction_kernel_launch_count >=
              batch.kernel_launch_count ||
          batch.synchronization_count != 1U ||
          batch.compaction_kind !=
              detail::Phase14AnchoredPairCandidateCompactionKind::
                  device_serial_prefix_clamp) {
        throw std::runtime_error(
            "the anchored-pair CUDA launcher returned invalid execution "
            "metadata");
      }
      return;
  }
  throw std::runtime_error(
      "the anchored-pair launcher returned an invalid execution kind");
}

[[nodiscard]] AnchoredPairCandidateProposalCursor make_output_cursor(
    AnchoredPairCandidateProposalCursorKind kind,
    std::uint64_t node_index,
    std::uint64_t query_digest_fnv1a,
    std::uint64_t source_snapshot_epoch) {
  return AnchoredPairCandidateProposalCursor{
      kind,
      node_index,
      query_digest_fnv1a,
      source_snapshot_epoch};
}

[[nodiscard]] AnchoredPairCandidateProposalBatchResult validate_gpu_batch(
    const DeviceBatch& batch,
    const detail::Phase14AnchoredPairCandidateProposalHostState& host,
    std::span<const AnchoredPairCandidateProposalQuery> queries,
    const PackedQueries& packed,
    AnchoredPairCandidateProposalBudget budget,
    std::size_t maximum_query_count,
    std::size_t maximum_transcript_record_count,
    std::uint64_t previous_epoch) {
  if (previous_epoch == std::numeric_limits<std::uint64_t>::max() ||
      batch.buffer_epoch != previous_epoch + UINT64_C(1) ||
      batch.source_snapshot_epoch != host.source_snapshot_epoch ||
      batch.segment_count != queries.size() ||
      batch.segments.size() != queries.size() ||
      batch.record_count != batch.records.size() ||
      batch.physical_query_capacity != maximum_query_count ||
      batch.physical_transcript_record_capacity !=
          maximum_transcript_record_count ||
      batch.active_total_transcript_record_limit !=
          budget.maximum_total_transcript_record_count ||
      batch.records.size() > budget.maximum_total_transcript_record_count ||
      batch.records.size() > maximum_transcript_record_count) {
    throw std::runtime_error(
        "the anchored-pair launcher returned invalid extents or epoch");
  }
  const std::size_t expected_query_bytes = checked_product(
      queries.size(),
      sizeof(InputRecord),
      "the anchored-pair query transfer extent overflows size_t");
  const std::size_t expected_segment_bytes = checked_product(
      queries.size(),
      sizeof(DeviceSegment),
      "the anchored-pair segment transfer extent overflows size_t");
  const std::size_t returned_record_bytes = checked_product(
      batch.records.size(),
      sizeof(DeviceRecord),
      "the anchored-pair transcript transfer extent overflows size_t");
  const std::size_t expected_active_transcript_bytes = checked_product(
      budget.maximum_total_transcript_record_count,
      sizeof(DeviceRecord),
      "the anchored-pair active transcript extent overflows size_t");
  if (batch.host_to_device_query_byte_count != expected_query_bytes ||
      batch.initialized_segment_byte_count != expected_segment_bytes ||
      batch.initialized_transcript_byte_count !=
          expected_active_transcript_bytes ||
      batch.device_to_host_segment_byte_count != expected_segment_bytes ||
      batch.device_to_host_transcript_byte_count !=
          expected_active_transcript_bytes ||
      batch.device_to_host_transcript_byte_count < returned_record_bytes) {
    throw std::runtime_error(
        "the anchored-pair launcher returned invalid active transfer "
        "extents");
  }
  validate_execution_metadata(batch, host);
  if (batch.proposal_digest_fnv1a !=
      detail::phase14_anchored_pair_candidate_batch_digest(batch)) {
    throw std::runtime_error(
        "the anchored-pair launcher returned a forged transcript digest");
  }

  AnchoredPairCandidateProposalBatchResult result;
  result.segments.reserve(batch.segments.size());
  result.records.reserve(batch.records.size());
  initialize_common_audit(
      result.audit,
      host,
      maximum_query_count,
      maximum_transcript_record_count,
      budget.maximum_total_transcript_record_count,
      packed);
  result.audit.active_host_to_device_query_byte_count =
      batch.host_to_device_query_byte_count;
  result.audit.initialized_device_segment_byte_count =
      batch.initialized_segment_byte_count;
  result.audit.initialized_device_transcript_byte_count =
      batch.initialized_transcript_byte_count;
  result.audit.copied_device_to_host_segment_byte_count =
      batch.device_to_host_segment_byte_count;
  result.audit.copied_device_to_host_transcript_byte_count =
      batch.device_to_host_transcript_byte_count;
  result.audit.gpu_kernel_launch_count = batch.kernel_launch_count;
  result.audit.gpu_compaction_kernel_launch_count =
      batch.compaction_kernel_launch_count;
  result.audit.compacted_host_transcript_record_count =
      batch.records.size();
  result.audit.gpu_synchronization_count = batch.synchronization_count;
  result.audit.buffer_epoch = batch.buffer_epoch;
  result.audit.proposal_digest_fnv1a = batch.proposal_digest_fnv1a;
  result.audit.gpu_execution_performed =
      batch.execution_kind ==
      detail::Phase14AnchoredPairCandidateExecutionKind::cuda;
  result.audit.device_serial_prefix_clamp_performed =
      batch.compaction_kind ==
      detail::Phase14AnchoredPairCandidateCompactionKind::
          device_serial_prefix_clamp;

  std::size_t expected_record_begin = 0U;
  for (std::size_t query_index = 0U;
       query_index < queries.size();
       ++query_index) {
    const DeviceSegment& segment = batch.segments[query_index];
    const InputRecord& input = packed.records[query_index];
    if (segment.query_index != input.query_index ||
        segment.query_digest_fnv1a != input.query_digest_fnv1a ||
        segment.buffer_epoch != batch.buffer_epoch ||
        segment.source_snapshot_epoch != host.source_snapshot_epoch ||
        segment.input_cursor_kind != input.cursor_kind ||
        segment.input_cursor_node_index != input.cursor_node_index ||
        segment.failure_code != static_cast<std::uint64_t>(
            detail::Phase14AnchoredPairCandidateFailureCode::none)) {
      throw std::runtime_error(
          "the anchored-pair transcript segment does not match its query");
    }
    const std::size_t node_visit_count = checked_size(
        segment.node_visit_count,
        "an anchored-pair node count does not fit size_t");
    const std::size_t record_begin = checked_size(
        segment.record_begin,
        "an anchored-pair record begin does not fit size_t");
    const std::size_t record_count = checked_size(
        segment.record_count,
        "an anchored-pair record count does not fit size_t");
    if (node_visit_count > budget.maximum_node_visit_count_per_query ||
        record_count >
            budget.maximum_transcript_record_count_per_query ||
        record_begin != expected_record_begin ||
        record_count > batch.records.size() - record_begin ||
        record_count > node_visit_count) {
      throw std::runtime_error(
          "the anchored-pair transcript segment exceeds its budget or "
          "breaks the record partition");
    }
    expected_record_begin = checked_sum(
        expected_record_begin,
        record_count,
        "the anchored-pair record partition overflows size_t");

    const AnchoredPairCandidateProposalSegmentStatus status =
        decode_status(segment.status);
    const AnchoredPairCandidateProposalStopReason stop_reason =
        decode_stop_reason(segment.stop_reason);
    const AnchoredPairCandidateProposalCursorKind next_kind =
        decode_cursor_kind(segment.next_cursor_kind);
    if (status == AnchoredPairCandidateProposalSegmentStatus::complete) {
      if (stop_reason != AnchoredPairCandidateProposalStopReason::none ||
          next_kind != AnchoredPairCandidateProposalCursorKind::complete ||
          segment.next_cursor_node_index !=
              anchored_pair_candidate_gpu_invalid_node_index) {
        throw std::runtime_error(
            "a complete anchored-pair segment has a nonterminal cursor");
      }
      ++result.audit.complete_segment_count;
    } else {
      if (next_kind != AnchoredPairCandidateProposalCursorKind::active ||
          segment.next_cursor_node_index >= host.certified_node_count) {
        throw std::runtime_error(
            "an exhausted anchored-pair segment has an invalid resume "
            "cursor");
      }
      if (status ==
          AnchoredPairCandidateProposalSegmentStatus::budget_exhausted) {
        if (stop_reason !=
                AnchoredPairCandidateProposalStopReason::node_visit_limit ||
            node_visit_count !=
                budget.maximum_node_visit_count_per_query) {
          throw std::runtime_error(
              "an anchored-pair budget exhaustion lacks its exact cap");
        }
        ++result.audit.budget_exhausted_segment_count;
      } else {
        if (stop_reason !=
                AnchoredPairCandidateProposalStopReason::
                    transcript_record_limit ||
            (record_count !=
                 budget.maximum_transcript_record_count_per_query &&
             batch.records.size() !=
                 budget.maximum_total_transcript_record_count)) {
          throw std::runtime_error(
              "an anchored-pair capacity exhaustion lacks its exact cap");
        }
        ++result.audit.capacity_exhausted_segment_count;
      }
    }

    const std::size_t required_witness_count =
        queries[query_index].maximum_closed_rank - 1U;
    const std::size_t witness_count =
        queries[query_index].witness_bank_point_ids.size();
    std::uint64_t previous_node_index =
        anchored_pair_candidate_gpu_invalid_node_index;
    for (std::size_t record_offset = 0U;
         record_offset < record_count;
         ++record_offset) {
      const DeviceRecord& record =
          batch.records[record_begin + record_offset];
      if (record.node_index >= host.certified_node_count ||
          record.node_index >= previous_node_index) {
        throw std::runtime_error(
            "an anchored-pair transcript leaves its node domain or is not "
            "strictly decreasing in inverse-postorder traversal order");
      }
      previous_node_index = record.node_index;
      if (record.witness_mask == 0U) {
        ++result.audit.candidate_leaf_proposal_count;
      } else {
        const bool mask_in_bank =
            witness_count == 64U ||
            (record.witness_mask >> witness_count) == 0U;
        if (!mask_in_bank ||
            std::popcount(record.witness_mask) !=
                static_cast<int>(required_witness_count)) {
          throw std::runtime_error(
              "an anchored-pair prune mask has the wrong domain or "
              "cardinality");
        }
        ++result.audit.prune_proposal_count;
      }
      result.records.push_back(
          AnchoredPairCandidateProposalRecord{
              record.node_index, record.witness_mask});
    }
    result.audit.node_visit_count = checked_sum(
        result.audit.node_visit_count,
        node_visit_count,
        "the anchored-pair aggregate visit count overflows size_t");
    result.segments.push_back(AnchoredPairCandidateProposalSegment{
        query_index,
        packed.query_digests[query_index],
        queries[query_index].cursor,
        make_output_cursor(
            next_kind,
            segment.next_cursor_node_index,
            packed.query_digests[query_index],
            host.source_snapshot_epoch),
        status,
        stop_reason,
        node_visit_count,
        record_begin,
        record_count});
  }
  if (expected_record_begin != batch.records.size()) {
    throw std::runtime_error(
        "the anchored-pair transcript has an unowned record suffix");
  }
  result.audit.segment_partition_validated = true;
  result.audit.deterministic_query_order_compaction_validated = true;
  result.audit.record_node_domains_validated = true;
  result.audit.candidate_leaf_masks_validated = true;
  result.audit.prune_mask_cardinalities_validated = true;
  result.audit.transcript_digest_validated = true;
  return result;
}

[[nodiscard]] AnchoredPairCandidateProposalBatchResult empty_result(
    const detail::Phase14AnchoredPairCandidateProposalHostState& host,
    std::size_t maximum_query_count,
    std::size_t maximum_transcript_record_count,
    std::size_t active_total_transcript_record_limit,
    const PackedQueries& packed) {
  AnchoredPairCandidateProposalBatchResult result;
  initialize_common_audit(
      result.audit,
      host,
      maximum_query_count,
      maximum_transcript_record_count,
      active_total_transcript_record_limit,
      packed);
  result.audit.segment_partition_validated = true;
  result.audit.deterministic_query_order_compaction_validated = true;
  result.audit.record_node_domains_validated = true;
  result.audit.candidate_leaf_masks_validated = true;
  result.audit.prune_mask_cardinalities_validated = true;
  result.audit.transcript_digest_validated = true;
  result.audit.proposal_digest_fnv1a = kFnvOffsetBasis;
  return result;
}

}  // namespace

bool AnchoredPairCandidateProposalBatchResult::validated_proposal_batch()
    const noexcept {
  return schema_version ==
             anchored_pair_candidate_gpu_proposal_schema_version &&
         segments.size() == audit.canonical_query_count &&
         records.size() == audit.candidate_leaf_proposal_count +
                               audit.prune_proposal_count &&
         audit.matching_immutable_point_namespace_validated &&
         audit.traversal_lease_owner_retained &&
         audit.traversal_lease_device_validated &&
         audit.traversal_lease_extents_validated &&
         audit.canonical_query_order_validated &&
         audit.witness_banks_validated &&
         audit.input_cursors_validated &&
         audit.fixed_capacity_preflight_satisfied &&
         audit.fixed_resident_capacity_bound_at_construction &&
         audit.deterministic_query_order_compaction_validated &&
         audit.segment_partition_validated &&
         audit.record_node_domains_validated &&
         audit.candidate_leaf_masks_validated &&
         audit.prune_mask_cardinalities_validated &&
         audit.transcript_digest_validated &&
         !audit.candidate_leaf_status_recertified &&
         !audit.strict_witness_masks_recertified &&
         !audit.exact_closed_ball_partition_published &&
         !audit.scientific_decision_published &&
         !audit.hierarchy_reduction_or_attachment_published &&
         !audit.forbidden_global_structure_materialized &&
         !audit.transcript_accumulated_across_calls &&
         !audit.public_status_claimed;
}

bool AnchoredPairCandidateProposalBatchResult::scientific_outcome()
    const noexcept {
  return false;
}

AnchoredPairCandidateProposalContext::
    AnchoredPairCandidateProposalContext(
        const spatial::CanonicalPointCloud& cloud,
        MortonLbvhDeviceTraversalLease&& traversal_lease,
        std::size_t maximum_query_count,
        std::size_t maximum_transcript_record_count)
    : state_(
          std::make_shared<
              detail::Phase14AnchoredPairCandidateProposalContextState>(
                  maximum_query_count,
                  maximum_transcript_record_count)),
      host_(
          std::make_unique<
              detail::Phase14AnchoredPairCandidateProposalHostState>()),
      maximum_query_count_(maximum_query_count),
      maximum_transcript_record_count_(
          maximum_transcript_record_count) {
  if (maximum_query_count == 0U ||
      maximum_transcript_record_count == 0U) {
    throw std::invalid_argument(
        "an anchored-pair proposal context requires nonzero fixed "
        "capacities");
  }
  static_cast<void>(checked_product(
      maximum_query_count,
      sizeof(InputRecord),
      "the anchored-pair fixed query capacity overflows size_t"));
  static_cast<void>(checked_product(
      maximum_query_count,
      sizeof(DeviceSegment),
      "the anchored-pair fixed segment capacity overflows size_t"));
  static_cast<void>(checked_product(
      maximum_transcript_record_count,
      sizeof(DeviceRecord),
      "the anchored-pair fixed transcript capacity overflows size_t"));
  if (!traversal_lease.ready()) {
    throw std::invalid_argument(
        "an anchored-pair proposal context requires a ready traversal "
        "lease");
  }
  const MortonLbvhDeviceTraversalLeaseAudit& lease_audit =
      traversal_lease.audit();
  if (cloud.identity_ == nullptr ||
      traversal_lease.source_cloud_identity_ == nullptr ||
      cloud.identity_.get() !=
          traversal_lease.source_cloud_identity_.get()) {
    throw std::invalid_argument(
        "an anchored-pair traversal lease belongs to another immutable "
        "PointId namespace");
  }
  const std::size_t point_count = cloud.size();
  const std::size_t node_count = checked_node_count(point_count);
  if (lease_audit.point_count != point_count ||
      lease_audit.certified_node_count != node_count ||
      lease_audit.retained_coordinate_word_capacity < 3U * point_count ||
      lease_audit.retained_morton_point_id_capacity < point_count ||
      lease_audit.retained_node_capacity < node_count ||
      lease_audit.source_snapshot_epoch == 0U ||
      lease_audit.retained_host_snapshot_byte_count != 0U ||
      lease_audit.second_host_snapshot_retained ||
      lease_audit.higher_order_delaunay_mosaic_materialized ||
      lease_audit.global_cell_or_coface_arena_materialized ||
      lease_audit.public_status_claimed) {
    throw std::invalid_argument(
        "an anchored-pair traversal lease has invalid certified extents or "
        "forbidden retained structures");
  }
  const bool cuda_resident = traversal_lease.cuda_resident();
  const bool host_fake =
      lease_audit.host_fake_lifecycle_exercised;
  if (cuda_resident == host_fake ||
      (cuda_resident &&
       (traversal_lease.device_coordinate_bits_ == nullptr ||
        traversal_lease.device_morton_point_ids_ == nullptr ||
        traversal_lease.device_nodes_ == nullptr ||
        traversal_lease.cuda_device_ < 0)) ||
      (host_fake &&
       (traversal_lease.device_coordinate_bits_ != nullptr ||
        traversal_lease.device_morton_point_ids_ != nullptr ||
        traversal_lease.device_nodes_ != nullptr ||
        traversal_lease.cuda_device_ != -1))) {
    throw std::invalid_argument(
        "an anchored-pair traversal lease has ambiguous device ownership");
  }

  // Transfer the move-only owner only after every potentially throwing
  // validation.  Explicitly clear the source views so ready()==false even
  // though its immutable audit remains available for diagnostics.
  host_->cloud_identity = traversal_lease.source_cloud_identity_;
  host_->point_count = point_count;
  host_->certified_node_count = node_count;
  host_->retained_coordinate_word_capacity =
      lease_audit.retained_coordinate_word_capacity;
  host_->retained_morton_point_id_capacity =
      lease_audit.retained_morton_point_id_capacity;
  host_->retained_node_capacity = lease_audit.retained_node_capacity;
  host_->persistent_device_byte_capacity =
      lease_audit.persistent_device_byte_capacity;
  host_->source_snapshot_epoch = lease_audit.source_snapshot_epoch;
  host_->device_coordinate_bits =
      traversal_lease.device_coordinate_bits_;
  host_->device_morton_point_ids =
      traversal_lease.device_morton_point_ids_;
  host_->device_nodes = traversal_lease.device_nodes_;
  host_->cuda_device = traversal_lease.cuda_device_;
  host_->cuda_resident = cuda_resident;
  host_->host_fake = host_fake;
  host_->adopted_owner =
      std::move(traversal_lease.retained_resources_);
  traversal_lease.source_cloud_identity_.reset();
  traversal_lease.device_coordinate_bits_ = nullptr;
  traversal_lease.device_morton_point_ids_ = nullptr;
  traversal_lease.device_nodes_ = nullptr;
  traversal_lease.cuda_device_ = -1;
}

AnchoredPairCandidateProposalContext::
    ~AnchoredPairCandidateProposalContext() noexcept = default;

AnchoredPairCandidateProposalContext::
    AnchoredPairCandidateProposalContext(
        AnchoredPairCandidateProposalContext&&) noexcept = default;

AnchoredPairCandidateProposalContext&
AnchoredPairCandidateProposalContext::operator=(
    AnchoredPairCandidateProposalContext&&) noexcept = default;

void AnchoredPairCandidateProposalContext::require_matching_cloud(
    const spatial::CanonicalPointCloud& cloud) const {
  if (!state_ || !host_) {
    throw std::logic_error(
        "a moved-from anchored-pair proposal context cannot be used");
  }
  if (cloud.identity_ == nullptr ||
      host_->cloud_identity == nullptr ||
      cloud.identity_.get() != host_->cloud_identity.get() ||
      cloud.size() != host_->point_count) {
    throw std::invalid_argument(
        "an anchored-pair proposal context was called with another "
        "immutable PointId namespace");
  }
}

AnchoredPairCandidateProposalBatchResult
AnchoredPairCandidateProposalContext::build(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const AnchoredPairCandidateProposalQuery> canonical_queries,
    AnchoredPairCandidateProposalBudget budget) {
  require_matching_cloud(cloud);
  validate_budget(budget, maximum_transcript_record_count_);
  const PackedQueries packed = validate_and_pack_queries(
      *host_, canonical_queries, maximum_query_count_, budget);
  return state_->with_gpu_section([&]() {
    if (packed.records.empty()) {
      return empty_result(
          *host_,
          maximum_query_count_,
          maximum_transcript_record_count_,
          budget.maximum_total_transcript_record_count,
          packed);
    }
    const detail::Phase14AnchoredPairCandidateAdoptedTraversal traversal =
        host_->adopted_traversal();
    const DeviceBatch batch =
        detail::propose_phase14_anchored_pair_candidates_on_gpu(
            *state_,
            traversal,
            packed.records,
            maximum_query_count_,
            maximum_transcript_record_count_,
            budget.maximum_total_transcript_record_count);
    AnchoredPairCandidateProposalBatchResult result = validate_gpu_batch(
        batch,
        *host_,
        canonical_queries,
        packed,
        budget,
        maximum_query_count_,
        maximum_transcript_record_count_,
        last_buffer_epoch_);
    last_buffer_epoch_ = batch.buffer_epoch;
    if (!result.validated_proposal_batch() ||
        result.scientific_outcome()) {
      throw std::logic_error(
          "the anchored-pair host contract did not close its non-scientific "
          "proposal result");
    }
    return result;
  });
}

std::size_t AnchoredPairCandidateProposalContext::point_count()
    const noexcept {
  return host_ == nullptr ? 0U : host_->point_count;
}

std::size_t AnchoredPairCandidateProposalContext::certified_node_count()
    const noexcept {
  return host_ == nullptr ? 0U : host_->certified_node_count;
}

std::size_t AnchoredPairCandidateProposalContext::maximum_query_count()
    const noexcept {
  return state_ == nullptr ? 0U : maximum_query_count_;
}

std::size_t
AnchoredPairCandidateProposalContext::maximum_transcript_record_count()
    const noexcept {
  return state_ == nullptr ? 0U : maximum_transcript_record_count_;
}

}  // namespace morsehgp3d::gpu
