#include "morsehgp3d/gpu/anchored_pair_candidate_recertifier.hpp"

#include "morsehgp3d/hierarchy/anchored_pair_witness_bank.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu {
namespace {

using Audit = ExactAnchoredPairCandidateRecertifierAudit;
using Budget = ExactAnchoredPairCandidateRecertifierBudget;
using Decision = ExactAnchoredPairCandidateRecertifierDecision;
using Query = AnchoredPairCandidateProposalQuery;
using Reason = ExactAnchoredPairCandidateRecertifierRejectionReason;
using Result = ExactAnchoredPairCandidateRecertifierResult;
using Segment = AnchoredPairCandidateProposalSegment;

constexpr std::uint64_t kFnvOffsetBasis =
    UINT64_C(14695981039346656037);
constexpr std::uint64_t kFnvPrime = UINT64_C(1099511628211);

[[nodiscard]] bool try_add(
    std::size_t left,
    std::size_t right,
    std::size_t& sum) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  sum = left + right;
  return true;
}

[[nodiscard]] bool try_subtree_node_count(
    std::size_t leaf_count,
    std::size_t& node_count) noexcept {
  if (leaf_count == 0U ||
      leaf_count > std::numeric_limits<std::size_t>::max() / 2U + 1U) {
    return false;
  }
  node_count = 2U * leaf_count - 1U;
  return true;
}

[[nodiscard]] std::uint64_t digest_word(
    std::uint64_t digest,
    std::uint64_t word) noexcept {
  for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
    digest ^= (word >> shift) & UINT64_C(0xff);
    digest *= kFnvPrime;
  }
  return digest;
}

[[nodiscard]] std::uint64_t recompute_query_digest(
    const Query& query,
    std::uint64_t source_snapshot_epoch) noexcept {
  std::uint64_t digest = kFnvOffsetBasis;
  for (const std::uint64_t word :
       {UINT64_C(0x5038645155455259),
        static_cast<std::uint64_t>(
            anchored_pair_candidate_gpu_proposal_schema_version),
        source_snapshot_epoch,
        static_cast<std::uint64_t>(query.anchor_point_id),
        static_cast<std::uint64_t>(query.maximum_closed_rank),
        static_cast<std::uint64_t>(
            query.witness_bank_point_ids.size())}) {
    digest = digest_word(digest, word);
  }
  for (const spatial::PointId witness_point_id :
       query.witness_bank_point_ids) {
    digest = digest_word(
        digest, static_cast<std::uint64_t>(witness_point_id));
  }
  return digest;
}

[[nodiscard]] bool query_shape_valid(
    const Query& query,
    std::size_t point_count) noexcept {
  if (query.anchor_point_id >= point_count ||
      query.maximum_closed_rank < 2U ||
      query.maximum_closed_rank >
          anchored_pair_candidate_gpu_maximum_closed_rank ||
      query.witness_bank_point_ids.size() >
          anchored_pair_candidate_gpu_maximum_witness_bank_size ||
      query.cursor.kind !=
          AnchoredPairCandidateProposalCursorKind::initial ||
      query.cursor.next_node_index !=
          anchored_pair_candidate_gpu_invalid_node_index ||
      query.cursor.query_digest_fnv1a != 0U ||
      query.cursor.source_snapshot_epoch != 0U) {
    return false;
  }
  for (std::size_t witness_index = 0U;
       witness_index < query.witness_bank_point_ids.size();
       ++witness_index) {
    const spatial::PointId witness_point_id =
        query.witness_bank_point_ids[witness_index];
    if (witness_point_id >= point_count ||
        witness_point_id == query.anchor_point_id) {
      return false;
    }
    for (std::size_t prior_index = 0U;
         prior_index < witness_index;
         ++prior_index) {
      if (query.witness_bank_point_ids[prior_index] ==
          witness_point_id) {
        return false;
      }
    }
  }
  return true;
}

[[nodiscard]] bool mask_is_in_bank(
    std::uint64_t mask,
    std::size_t bank_size) noexcept {
  return bank_size == 64U || (mask >> bank_size) == 0U;
}

// Repeat the public host-envelope predicate locally so this reference-CPU
// component depends only on the fixed proposal types, not on the CUDA context
// implementation or one of its launcher translation units.
[[nodiscard]] bool host_proposal_contract_valid(
    const AnchoredPairCandidateProposalBatchResult& batch) noexcept {
  return batch.schema_version ==
             anchored_pair_candidate_gpu_proposal_schema_version &&
         batch.segments.size() == batch.audit.canonical_query_count &&
         batch.audit.candidate_leaf_proposal_count <=
             batch.records.size() &&
         batch.audit.prune_proposal_count ==
             batch.records.size() -
                 batch.audit.candidate_leaf_proposal_count &&
         batch.audit.matching_immutable_point_namespace_validated &&
         batch.audit.traversal_lease_owner_retained &&
         batch.audit.traversal_lease_device_validated &&
         batch.audit.traversal_lease_extents_validated &&
         batch.audit.canonical_query_order_validated &&
         batch.audit.witness_banks_validated &&
         batch.audit.input_cursors_validated &&
         batch.audit.fixed_capacity_preflight_satisfied &&
         batch.audit.fixed_resident_capacity_bound_at_construction &&
         batch.audit.deterministic_query_order_compaction_validated &&
         batch.audit.segment_partition_validated &&
         batch.audit.record_node_domains_validated &&
         batch.audit.candidate_leaf_masks_validated &&
         batch.audit.prune_mask_cardinalities_validated &&
         batch.audit.transcript_digest_validated &&
         ((batch.audit.gpu_execution_performed &&
           batch.audit.query_witness_geometry_precomputed &&
           batch.audit
                   .gpu_witness_geometry_precomputation_kernel_launch_count ==
               1U &&
           batch.audit.precomputed_witness_geometry_byte_capacity != 0U) ||
          (!batch.audit.gpu_execution_performed &&
           !batch.audit.query_witness_geometry_precomputed &&
           batch.audit
                   .gpu_witness_geometry_precomputation_kernel_launch_count ==
               0U &&
           batch.audit.precomputed_witness_geometry_byte_capacity == 0U)) &&
         !batch.audit.candidate_leaf_status_recertified &&
         !batch.audit.strict_witness_masks_recertified &&
         !batch.audit.exact_closed_ball_partition_published &&
         !batch.audit.scientific_decision_published &&
         !batch.audit.hierarchy_reduction_or_attachment_published &&
         !batch.audit.forbidden_global_structure_materialized &&
         !batch.audit.transcript_accumulated_across_calls &&
         !batch.audit.public_status_claimed;
}

[[nodiscard]] Result rejected(Result result, Reason reason) {
  result.anchors.clear();
  result.audit.recertified_anchor_count = 0U;
  result.audit.recertified_candidate_point_id_count = 0U;
  result.audit.recertified_prune_receipt_count = 0U;
  result.decision = Decision::rejected;
  result.rejection_reason = reason;
  result.audit.batch_rejection_atomic = true;
  return result;
}

[[nodiscard]] bool non_morse_scope_honest(const Audit& audit) noexcept {
  return audit.batch_rejection_atomic &&
         !audit.exact_closed_ball_partition_published &&
         !audit.candidate_classification_performed &&
         !audit.scientific_morse_decision_published &&
         !audit.hierarchy_reduction_or_attachment_published &&
         !audit.forbidden_global_structure_materialized &&
         !audit.public_status_claimed;
}

[[nodiscard]] bool complete_output_shape(const Result& result) noexcept {
  std::size_t candidate_count = 0U;
  std::size_t receipt_count = 0U;
  if (result.anchors.size() != result.audit.recertified_anchor_count) {
    return false;
  }
  for (std::size_t anchor_index = 0U;
       anchor_index < result.anchors.size();
       ++anchor_index) {
    const auto& anchor = result.anchors[anchor_index];
    if (anchor.query_index != anchor_index ||
        !try_add(
            candidate_count,
            anchor.candidate_point_ids.size(),
            candidate_count) ||
        !try_add(
            receipt_count,
            anchor.prune_receipts.size(),
            receipt_count)) {
      return false;
    }
  }
  return candidate_count ==
             result.audit.recertified_candidate_point_id_count &&
         receipt_count ==
             result.audit.recertified_prune_receipt_count &&
         candidate_count ==
             result.audit.required_candidate_point_id_count &&
         receipt_count == result.audit.required_prune_receipt_count &&
         result.audit.exact_witness_predicate_count ==
             result.audit.required_exact_witness_predicate_count;
}

}  // namespace

bool ExactAnchoredPairCandidateRecertifierResult::
    complete_exact_candidate_stream() const noexcept {
  return schema_version ==
             exact_anchored_pair_candidate_recertifier_schema_version &&
         decision == Decision::complete &&
         rejection_reason == Reason::none &&
         audit.proposal_batch_host_contract_validated &&
         audit.index_cloud_identity_validated &&
         audit.capacity_preflight_completed &&
         audit.capacity_preflight_satisfied &&
         audit.canonical_query_order_validated &&
         audit.query_digests_recomputed &&
         audit.every_segment_initial_to_complete &&
         audit.segment_partition_validated &&
         audit.records_strictly_decreasing &&
         audit.candidate_leaf_orientation_recertified &&
         audit.strict_witness_masks_recertified &&
         audit.inverse_postorder_jump_arithmetic_validated &&
         audit.node_visit_counts_replayed &&
         audit.complete_tree_coverage_replayed &&
         non_morse_scope_honest(audit) && complete_output_shape(*this);
}

bool ExactAnchoredPairCandidateRecertifierResult::atomic_rejection()
    const noexcept {
  return schema_version ==
             exact_anchored_pair_candidate_recertifier_schema_version &&
         decision == Decision::rejected &&
         rejection_reason != Reason::none && anchors.empty() &&
         audit.recertified_anchor_count == 0U &&
         audit.recertified_candidate_point_id_count == 0U &&
         audit.recertified_prune_receipt_count == 0U &&
         non_morse_scope_honest(audit);
}

bool ExactAnchoredPairCandidateRecertifierResult::
    scientific_morse_outcome() const noexcept {
  return false;
}

class ExactAnchoredPairCandidateRecertifier final {
 public:
  [[nodiscard]] static Result run(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::span<const Query> queries,
      const AnchoredPairCandidateProposalBatchResult& batch,
      Budget budget) {
    Result result;
    result.requested_budget = budget;
    result.audit.input_query_count = queries.size();
    result.audit.input_segment_count = batch.segments.size();
    result.audit.input_transcript_record_count = batch.records.size();
    result.audit.source_snapshot_epoch = batch.audit.source_snapshot_epoch;

    if (!host_proposal_contract_valid(batch)) {
      return rejected(
          std::move(result), Reason::proposal_batch_contract);
    }
    result.audit.proposal_batch_host_contract_validated = true;

    if (!index.validated_for(cloud) || cloud.size() == 0U ||
        cloud.size() >
            std::numeric_limits<std::size_t>::max() / 2U + 1U ||
        index.nodes_.empty() ||
        index.leaves_.size() != cloud.size() ||
        index.nodes_.size() != 2U * cloud.size() - 1U ||
        index.root_index_ != index.nodes_.size() - 1U ||
        batch.audit.point_count != cloud.size() ||
        batch.audit.certified_node_count != index.nodes_.size() ||
        batch.audit.source_snapshot_epoch == 0U) {
      return rejected(std::move(result), Reason::index_cloud_identity);
    }
    result.audit.index_cloud_identity_validated = true;

    if (batch.segments.size() != queries.size() ||
        batch.audit.canonical_query_count != queries.size() ||
        batch.audit.complete_segment_count != queries.size() ||
        batch.audit.budget_exhausted_segment_count != 0U ||
        batch.audit.capacity_exhausted_segment_count != 0U ||
        batch.audit.compacted_host_transcript_record_count !=
            batch.records.size()) {
      return rejected(
          std::move(result), Reason::proposal_batch_contract);
    }

    std::size_t expected_record_begin = 0U;
    std::size_t expected_candidate_count = 0U;
    std::size_t expected_receipt_count = 0U;
    std::size_t expected_predicate_count = 0U;
    std::size_t expected_node_visit_count = 0U;
    std::size_t maximum_bank_size = 0U;
    for (std::size_t query_index = 0U;
         query_index < queries.size();
         ++query_index) {
      const Query& query = queries[query_index];
      const Segment& segment = batch.segments[query_index];
      if (!query_shape_valid(query, cloud.size()) ||
          (query_index != 0U &&
           queries[query_index - 1U].anchor_point_id >=
               query.anchor_point_id) ||
          segment.query_index != query_index ||
          segment.input_cursor != query.cursor) {
        return rejected(std::move(result), Reason::query_contract);
      }
      maximum_bank_size = std::max(
          maximum_bank_size, query.witness_bank_point_ids.size());
      const std::uint64_t expected_digest = recompute_query_digest(
          query, batch.audit.source_snapshot_epoch);
      if (segment.query_digest_fnv1a != expected_digest ||
          segment.next_cursor.query_digest_fnv1a != expected_digest ||
          segment.next_cursor.source_snapshot_epoch !=
              batch.audit.source_snapshot_epoch) {
        return rejected(std::move(result), Reason::query_digest);
      }
      if (segment.status !=
              AnchoredPairCandidateProposalSegmentStatus::complete ||
          segment.stop_reason !=
              AnchoredPairCandidateProposalStopReason::none ||
          segment.next_cursor.kind !=
              AnchoredPairCandidateProposalCursorKind::complete ||
          segment.next_cursor.next_node_index !=
              anchored_pair_candidate_gpu_invalid_node_index) {
        return rejected(std::move(result), Reason::incomplete_segment);
      }
      if (segment.record_begin != expected_record_begin ||
          segment.record_begin > batch.records.size() ||
          segment.record_count >
              batch.records.size() - segment.record_begin ||
          segment.record_count > segment.node_visit_count) {
        return rejected(std::move(result), Reason::segment_partition);
      }
      if (!try_add(
              expected_record_begin,
              segment.record_count,
              expected_record_begin) ||
          !try_add(
              expected_node_visit_count,
              segment.node_visit_count,
              expected_node_visit_count)) {
        return rejected(std::move(result), Reason::capacity_overflow);
      }

      std::uint64_t previous_node_index =
          anchored_pair_candidate_gpu_invalid_node_index;
      const std::size_t record_end =
          segment.record_begin + segment.record_count;
      for (std::size_t record_index = segment.record_begin;
           record_index < record_end;
           ++record_index) {
        const auto& record = batch.records[record_index];
        if (record.node_index >= index.nodes_.size() ||
            record.node_index >= previous_node_index) {
          return rejected(
              std::move(result), Reason::record_order_or_domain);
        }
        previous_node_index = record.node_index;
        if (record.witness_mask == 0U) {
          if (!try_add(
                  expected_candidate_count,
                  1U,
                  expected_candidate_count)) {
            return rejected(
                std::move(result), Reason::capacity_overflow);
          }
          continue;
        }
        const std::size_t required_witness_count =
            query.maximum_closed_rank - 1U;
        const std::size_t observed_witness_count =
            static_cast<std::size_t>(
                std::popcount(record.witness_mask));
        if (!mask_is_in_bank(
                record.witness_mask,
                query.witness_bank_point_ids.size()) ||
            observed_witness_count != required_witness_count) {
          return rejected(
              std::move(result), Reason::record_order_or_domain);
        }
        if (!try_add(
                expected_receipt_count,
                1U,
                expected_receipt_count) ||
            !try_add(
                expected_predicate_count,
                observed_witness_count,
                expected_predicate_count)) {
          return rejected(
              std::move(result), Reason::capacity_overflow);
        }
      }
    }
    if (expected_record_begin != batch.records.size()) {
      return rejected(std::move(result), Reason::segment_partition);
    }
    if (batch.audit.candidate_leaf_proposal_count !=
            expected_candidate_count ||
        batch.audit.prune_proposal_count != expected_receipt_count ||
        batch.audit.node_visit_count != expected_node_visit_count ||
        batch.audit.maximum_observed_witness_bank_size !=
            maximum_bank_size) {
      return rejected(
          std::move(result), Reason::proposal_batch_contract);
    }
    result.audit.maximum_observed_witness_bank_size = maximum_bank_size;
    result.audit.required_candidate_point_id_count =
        expected_candidate_count;
    result.audit.required_prune_receipt_count = expected_receipt_count;
    result.audit.required_exact_witness_predicate_count =
        expected_predicate_count;
    result.audit.canonical_query_order_validated = true;
    result.audit.query_digests_recomputed = true;
    result.audit.every_segment_initial_to_complete = true;
    result.audit.segment_partition_validated = true;
    result.audit.records_strictly_decreasing = true;
    result.audit.capacity_preflight_completed = true;

    const std::vector<ExactAnchoredPairRecertifiedAnchorCandidates>
        empty_anchor_vector;
    const std::vector<spatial::PointId> empty_candidate_vector;
    const std::vector<ExactAnchoredPairCandidatePruneReceipt>
        empty_receipt_vector;
    if (queries.size() > empty_anchor_vector.max_size() ||
        expected_candidate_count > empty_candidate_vector.max_size() ||
        expected_receipt_count > empty_receipt_vector.max_size()) {
      return rejected(std::move(result), Reason::capacity_overflow);
    }
    if (queries.size() > budget.maximum_query_count ||
        batch.records.size() >
            budget.maximum_transcript_record_count ||
        expected_candidate_count >
            budget.maximum_candidate_point_id_count ||
        expected_receipt_count >
            budget.maximum_prune_receipt_count ||
        expected_predicate_count >
            budget.maximum_exact_witness_predicate_count) {
      return rejected(std::move(result), Reason::budget_exhausted);
    }
    result.audit.capacity_preflight_satisfied = true;

    std::vector<ExactAnchoredPairRecertifiedAnchorCandidates>
        recertified_anchors;
    recertified_anchors.reserve(queries.size());
    std::size_t replayed_node_visit_count = 0U;
    std::size_t recertified_candidate_count = 0U;
    std::size_t recertified_receipt_count = 0U;
    std::size_t exact_predicate_count = 0U;

    const auto node_bounds = [&](std::size_t node_index) {
      const spatial::MortonLbvhIndex::Node& node =
          index.nodes_[node_index];
      spatial::ExactDyadicAabb3 bounds{};
      for (std::size_t axis = 0U; axis < 3U; ++axis) {
        bounds.lower_binary64_bits[axis] =
            cloud.point(node.lower_point_ids[axis])
                .canonical_input_bits()[axis];
        bounds.upper_binary64_bits[axis] =
            cloud.point(node.upper_point_ids[axis])
                .canonical_input_bits()[axis];
      }
      return bounds;
    };

    constexpr std::size_t invalid_node_index =
        std::numeric_limits<std::size_t>::max();
    for (std::size_t query_index = 0U;
         query_index < queries.size();
         ++query_index) {
      const Query& query = queries[query_index];
      const Segment& segment = batch.segments[query_index];
      ExactAnchoredPairRecertifiedAnchorCandidates anchor_output;
      anchor_output.query_index = query_index;
      anchor_output.query_digest_fnv1a =
          segment.query_digest_fnv1a;
      anchor_output.anchor_point_id = query.anchor_point_id;
      anchor_output.maximum_closed_rank =
          query.maximum_closed_rank;
      anchor_output.candidate_point_ids.reserve(segment.record_count);
      anchor_output.prune_receipts.reserve(segment.record_count);

      const std::size_t record_end =
          segment.record_begin + segment.record_count;
      std::size_t record_index = segment.record_begin;
      std::size_t current_node_index = index.root_index_;
      std::size_t segment_node_visit_count = 0U;
      while (current_node_index != invalid_node_index) {
        if (current_node_index >= index.nodes_.size() ||
            !try_add(
                segment_node_visit_count,
                1U,
                segment_node_visit_count) ||
            !try_add(
                replayed_node_visit_count,
                1U,
                replayed_node_visit_count)) {
          return rejected(std::move(result), Reason::traversal_jump);
        }
        const spatial::MortonLbvhIndex::Node& node =
            index.nodes_[current_node_index];
        if (record_index < record_end &&
            batch.records[record_index].node_index >
                current_node_index) {
          return rejected(std::move(result), Reason::traversal_jump);
        }
        const bool record_at_current =
            record_index < record_end &&
            batch.records[record_index].node_index ==
                current_node_index;

        const auto advance_by = [&](std::size_t skipped_node_count) {
          if (skipped_node_count == 0U ||
              skipped_node_count > current_node_index + 1U) {
            return false;
          }
          current_node_index =
              skipped_node_count == current_node_index + 1U
                  ? invalid_node_index
                  : current_node_index - skipped_node_count;
          return true;
        };

        if (!record_at_current) {
          if (node.is_leaf()) {
            if (node.leaf_end != node.leaf_begin + 1U ||
                node.leaf_end > index.leaves_.size()) {
              return rejected(
                  std::move(result), Reason::traversal_jump);
            }
            const spatial::PointId point_id =
                index.leaves_[node.leaf_begin].point_id;
            if (point_id > query.anchor_point_id) {
              return rejected(
                  std::move(result), Reason::missing_candidate_record);
            }
          } else if (
              current_node_index == 0U ||
              node.right_child != current_node_index - 1U) {
            return rejected(std::move(result), Reason::traversal_jump);
          }
          if (!advance_by(1U)) {
            return rejected(std::move(result), Reason::traversal_jump);
          }
          continue;
        }

        const auto& record = batch.records[record_index];
        if (record.witness_mask == 0U) {
          if (!node.is_leaf() ||
              node.leaf_end != node.leaf_begin + 1U ||
              node.leaf_end > index.leaves_.size()) {
            return rejected(
                std::move(result), Reason::false_candidate_record);
          }
          const spatial::PointId candidate_point_id =
              index.leaves_[node.leaf_begin].point_id;
          if (candidate_point_id <= query.anchor_point_id) {
            return rejected(
                std::move(result), Reason::false_candidate_record);
          }
          anchor_output.candidate_point_ids.push_back(
              candidate_point_id);
          ++recertified_candidate_count;
          ++record_index;
          if (!advance_by(1U)) {
            return rejected(std::move(result), Reason::traversal_jump);
          }
          continue;
        }

        if (node.leaf_begin >= node.leaf_end ||
            node.leaf_end > index.leaves_.size()) {
          return rejected(std::move(result), Reason::traversal_jump);
        }
        ExactAnchoredPairCandidatePruneReceipt receipt;
        receipt.lbvh_node_index = current_node_index;
        receipt.leaf_begin = node.leaf_begin;
        receipt.leaf_end = node.leaf_end;
        receipt.witness_count =
            query.maximum_closed_rank - 1U;
        const spatial::ExactDyadicAabb3 bounds =
            node_bounds(current_node_index);
        std::size_t receipt_witness_index = 0U;
        for (std::size_t witness_index = 0U;
             witness_index < query.witness_bank_point_ids.size();
             ++witness_index) {
          const std::uint64_t witness_bit =
              UINT64_C(1) << witness_index;
          if ((record.witness_mask & witness_bit) == 0U) {
            continue;
          }
          const spatial::PointId witness_point_id =
              query.witness_bank_point_ids[witness_index];
          ++exact_predicate_count;
          if (receipt_witness_index >= receipt.witness_point_ids.size() ||
              hierarchy::exact_anchored_pair_witness_aabb_minimum_sign(
                  cloud,
                  query.anchor_point_id,
                  witness_point_id,
                  bounds) <= 0) {
            return rejected(
                std::move(result), Reason::false_prune_record);
          }
          receipt.witness_point_ids[receipt_witness_index] =
              witness_point_id;
          ++receipt_witness_index;
        }
        if (receipt_witness_index != receipt.witness_count) {
          return rejected(
              std::move(result), Reason::false_prune_record);
        }
        anchor_output.prune_receipts.push_back(receipt);
        ++recertified_receipt_count;
        ++record_index;

        std::size_t subtree_node_count = 0U;
        if (!try_subtree_node_count(
                node.leaf_end - node.leaf_begin,
                subtree_node_count) ||
            !advance_by(subtree_node_count)) {
          return rejected(std::move(result), Reason::traversal_jump);
        }
      }
      if (record_index != record_end) {
        return rejected(std::move(result), Reason::traversal_jump);
      }
      if (segment_node_visit_count != segment.node_visit_count) {
        return rejected(std::move(result), Reason::node_visit_count);
      }
      recertified_anchors.push_back(std::move(anchor_output));
    }

    if (replayed_node_visit_count != expected_node_visit_count ||
        replayed_node_visit_count != batch.audit.node_visit_count) {
      return rejected(std::move(result), Reason::node_visit_count);
    }
    if (recertified_candidate_count != expected_candidate_count ||
        recertified_receipt_count != expected_receipt_count ||
        exact_predicate_count != expected_predicate_count) {
      return rejected(
          std::move(result), Reason::incomplete_tree_coverage);
    }

    result.anchors = std::move(recertified_anchors);
    result.audit.recertified_anchor_count = result.anchors.size();
    result.audit.replayed_node_visit_count =
        replayed_node_visit_count;
    result.audit.recertified_candidate_point_id_count =
        recertified_candidate_count;
    result.audit.recertified_prune_receipt_count =
        recertified_receipt_count;
    result.audit.exact_witness_predicate_count =
        exact_predicate_count;
    result.audit.candidate_leaf_orientation_recertified = true;
    result.audit.strict_witness_masks_recertified = true;
    result.audit.inverse_postorder_jump_arithmetic_validated = true;
    result.audit.node_visit_counts_replayed = true;
    result.audit.complete_tree_coverage_replayed = true;
    result.decision = Decision::complete;
    result.rejection_reason = Reason::none;
    return result;
  }
};

ExactAnchoredPairCandidateRecertifierResult
recertify_exact_anchored_pair_candidate_proposal(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const AnchoredPairCandidateProposalQuery>
        canonical_queries,
    const AnchoredPairCandidateProposalBatchResult& proposal_batch,
    ExactAnchoredPairCandidateRecertifierBudget budget) {
  return ExactAnchoredPairCandidateRecertifier::run(
      index,
      cloud,
      canonical_queries,
      proposal_batch,
      std::move(budget));
}

}  // namespace morsehgp3d::gpu
