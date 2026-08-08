#include "morsehgp3d/gpu/anchored_pair_candidate_recertifier.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/hierarchy/anchored_pair_witness_bank.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::AnchoredPairCandidateProposalBatchResult;
using morsehgp3d::gpu::AnchoredPairCandidateProposalCursor;
using morsehgp3d::gpu::AnchoredPairCandidateProposalCursorKind;
using morsehgp3d::gpu::AnchoredPairCandidateProposalQuery;
using morsehgp3d::gpu::AnchoredPairCandidateProposalRecord;
using morsehgp3d::gpu::AnchoredPairCandidateProposalSegment;
using morsehgp3d::gpu::AnchoredPairCandidateProposalSegmentStatus;
using morsehgp3d::gpu::AnchoredPairCandidateProposalStopReason;
using morsehgp3d::gpu::ExactAnchoredPairCandidateRecertifierBudget;
using morsehgp3d::gpu::ExactAnchoredPairCandidateRecertifierRejectionReason;
using morsehgp3d::gpu::ExactAnchoredPairCandidateRecertifierResult;
using morsehgp3d::gpu::anchored_pair_candidate_gpu_invalid_node_index;
using morsehgp3d::gpu::anchored_pair_candidate_gpu_proposal_schema_version;
using morsehgp3d::gpu::recertify_exact_anchored_pair_candidate_proposal;
using morsehgp3d::hierarchy::ExactAnchoredPairWitnessBankBudget;
using morsehgp3d::hierarchy::ExactAnchoredPairWitnessBankPruneRecord;
using morsehgp3d::hierarchy::ExactAnchoredPairWitnessBankResult;
using morsehgp3d::hierarchy::build_exact_anchored_pair_witness_bank_candidates;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactLbvhTopKBudget;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

[[nodiscard]] CanonicalPointCloud line_cloud(std::size_t point_count) {
  std::vector<CertifiedPoint3> points;
  points.reserve(point_count);
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    points.push_back(CertifiedPoint3::from_binary64(
        static_cast<double>(point_index), 0.0, 0.0));
  }
  return CanonicalPointCloud::rejecting_duplicates(points);
}

[[nodiscard]] ExactLbvhTopKBudget top_k_budget(
    std::size_t point_count,
    std::size_t rank) {
  const std::size_t roomy = 16U * point_count + 64U;
  return ExactLbvhTopKBudget{
      roomy, roomy, roomy, roomy, roomy, rank, point_count};
}

[[nodiscard]] ExactAnchoredPairWitnessBankBudget witness_budget(
    std::size_t point_count,
    std::size_t witness_count) {
  const std::size_t roomy = 32U * point_count + 128U;
  ExactAnchoredPairWitnessBankBudget budget;
  budget.proposed_witness_bank_size = witness_count;
  budget.witness_search_budget =
      top_k_budget(point_count, witness_count);
  budget.maximum_node_visit_count = roomy;
  budget.maximum_internal_node_expansion_count = roomy;
  budget.maximum_traversal_stack_entry_count = roomy;
  budget.maximum_witness_node_predicate_count =
      roomy * (witness_count + 1U);
  budget.maximum_candidate_entry_count = point_count;
  budget.maximum_prune_record_count = 2U * point_count;
  return budget;
}

constexpr std::uint64_t kFnvOffsetBasis =
    UINT64_C(14695981039346656037);
constexpr std::uint64_t kFnvPrime = UINT64_C(1099511628211);

[[nodiscard]] std::uint64_t digest_word(
    std::uint64_t digest,
    std::uint64_t word) {
  for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
    digest ^= (word >> shift) & UINT64_C(0xff);
    digest *= kFnvPrime;
  }
  return digest;
}

[[nodiscard]] std::uint64_t query_digest(
    const AnchoredPairCandidateProposalQuery& query,
    std::uint64_t source_snapshot_epoch) {
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
  for (const PointId witness_point_id :
       query.witness_bank_point_ids) {
    digest = digest_word(
        digest, static_cast<std::uint64_t>(witness_point_id));
  }
  return digest;
}

[[nodiscard]] AnchoredPairCandidateProposalBatchResult valid_host_batch(
    const AnchoredPairCandidateProposalQuery& query,
    std::size_t point_count,
    std::size_t node_count,
    std::vector<AnchoredPairCandidateProposalRecord> records,
    std::size_t node_visit_count) {
  constexpr std::uint64_t source_snapshot_epoch = UINT64_C(17);
  AnchoredPairCandidateProposalBatchResult batch;
  batch.records = std::move(records);
  const std::uint64_t digest = query_digest(
      query, source_snapshot_epoch);
  batch.segments.push_back(AnchoredPairCandidateProposalSegment{
      0U,
      digest,
      query.cursor,
      AnchoredPairCandidateProposalCursor{
          AnchoredPairCandidateProposalCursorKind::complete,
          anchored_pair_candidate_gpu_invalid_node_index,
          digest,
          source_snapshot_epoch},
      AnchoredPairCandidateProposalSegmentStatus::complete,
      AnchoredPairCandidateProposalStopReason::none,
      node_visit_count,
      0U,
      batch.records.size()});

  auto& audit = batch.audit;
  audit.maximum_query_count = 1U;
  audit.physical_transcript_record_capacity =
      std::max<std::size_t>(1U, batch.records.size());
  audit.active_total_transcript_record_limit =
      audit.physical_transcript_record_capacity;
  audit.compacted_host_transcript_record_count = batch.records.size();
  audit.point_count = point_count;
  audit.certified_node_count = node_count;
  audit.retained_coordinate_word_capacity = 3U * point_count;
  audit.retained_morton_point_id_capacity = point_count;
  audit.retained_node_capacity = node_count;
  audit.source_snapshot_epoch = source_snapshot_epoch;
  audit.canonical_query_count = 1U;
  audit.witness_point_id_reference_count =
      query.witness_bank_point_ids.size();
  audit.maximum_observed_witness_bank_size =
      query.witness_bank_point_ids.size();
  audit.node_visit_count = node_visit_count;
  audit.complete_segment_count = 1U;
  for (const auto& record : batch.records) {
    if (record.witness_mask == 0U) {
      ++audit.candidate_leaf_proposal_count;
    } else {
      ++audit.prune_proposal_count;
    }
  }
  audit.matching_immutable_point_namespace_validated = true;
  audit.traversal_lease_owner_retained = true;
  audit.traversal_lease_device_validated = true;
  audit.traversal_lease_extents_validated = true;
  audit.canonical_query_order_validated = true;
  audit.witness_banks_validated = true;
  audit.input_cursors_validated = true;
  audit.fixed_capacity_preflight_satisfied = true;
  audit.fixed_resident_capacity_bound_at_construction = true;
  audit.deterministic_query_order_compaction_validated = true;
  audit.segment_partition_validated = true;
  audit.record_node_domains_validated = true;
  audit.candidate_leaf_masks_validated = true;
  audit.prune_mask_cardinalities_validated = true;
  audit.transcript_digest_validated = true;
  return batch;
}

[[nodiscard]] ExactAnchoredPairCandidateRecertifierBudget roomy_budget() {
  return ExactAnchoredPairCandidateRecertifierBudget{
      8U, 256U, 256U, 256U, 4096U};
}

[[nodiscard]] std::uint64_t receipt_mask(
    const ExactAnchoredPairWitnessBankPruneRecord& receipt,
    std::span<const PointId> witness_bank) {
  std::uint64_t mask = 0U;
  for (std::size_t receipt_index = 0U;
       receipt_index < receipt.witness_count;
       ++receipt_index) {
    const auto found = std::find(
        witness_bank.begin(),
        witness_bank.end(),
        receipt.witness_point_ids[receipt_index]);
    require(
        found != witness_bank.end(),
        "the CPU P8b receipt must name an in-bank witness");
    const std::size_t witness_index =
        static_cast<std::size_t>(found - witness_bank.begin());
    mask |= UINT64_C(1) << witness_index;
  }
  return mask;
}

[[nodiscard]] std::vector<std::size_t> leaf_node_indices(
    std::span<const morsehgp3d::spatial::MortonLeafRecord> leaves) {
  std::vector<std::size_t> result(leaves.size());
  std::size_t next_node_index = 0U;
  const auto build_range = [&](auto&& self,
                               std::size_t begin,
                               std::size_t end) -> void {
    if (end - begin == 1U) {
      result[begin] = next_node_index;
      ++next_node_index;
      return;
    }
    std::size_t split = begin + (end - begin) / 2U;
    const std::uint64_t difference =
        leaves[begin].morton_code ^ leaves[end - 1U].morton_code;
    if (difference != 0U) {
      const int width = static_cast<int>(std::bit_width(difference));
      const unsigned int highest_bit = static_cast<unsigned int>(
          width - 1);
      const std::uint64_t mask = UINT64_C(1) << highest_bit;
      std::size_t low = begin + 1U;
      std::size_t high = end;
      while (low < high) {
        const std::size_t middle = low + (high - low) / 2U;
        if ((leaves[middle].morton_code & mask) == 0U) {
          low = middle + 1U;
        } else {
          high = middle;
        }
      }
      split = low;
    }
    require(
        split > begin && split < end,
        "the independent test topology must find a strict split");
    self(self, begin, split);
    self(self, split, end);
    ++next_node_index;
  };
  build_range(build_range, 0U, leaves.size());
  require(
      next_node_index == 2U * leaves.size() - 1U,
      "the independent test topology must close a full binary tree");
  return result;
}

struct PruneFixture {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  ExactAnchoredPairWitnessBankResult cpu;
  std::vector<PointId> witnesses;
  std::vector<AnchoredPairCandidateProposalRecord> records;
  std::size_t internal_record_offset{};
  PointId in_node_witness{};
};

[[nodiscard]] PruneFixture make_prune_fixture() {
  CanonicalPointCloud cloud = line_cloud(20U);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  ExactAnchoredPairWitnessBankResult cpu =
      build_exact_anchored_pair_witness_bank_candidates(
          index,
          cloud,
          static_cast<PointId>(0U),
          11U,
          witness_budget(cloud.size(), 11U));
  require(cpu.complete(), "the short CPU P8b fixture must complete");
  require(
      !cpu.prune_records.empty(),
      "the short collinear P8b fixture must produce a prune");

  std::vector<PointId> witnesses = cpu.witness_bank_point_ids;
  const auto internal_receipt = std::find_if(
      cpu.prune_records.begin(),
      cpu.prune_records.end(),
      [](const ExactAnchoredPairWitnessBankPruneRecord& receipt) {
        return receipt.leaf_end - receipt.leaf_begin > 1U;
      });
  require(
      internal_receipt != cpu.prune_records.end(),
      "the collinear fixture must expose an internal subtree jump");
  const PointId in_node_witness =
      index.leaves()[internal_receipt->leaf_begin].point_id;
  if (std::find(
          witnesses.begin(), witnesses.end(), in_node_witness) ==
      witnesses.end()) {
    witnesses.push_back(in_node_witness);
  }

  std::vector<AnchoredPairCandidateProposalRecord> records;
  records.reserve(
      cpu.prune_records.size() + cpu.candidate_point_ids.size());
  for (const auto& receipt : cpu.prune_records) {
    records.push_back(AnchoredPairCandidateProposalRecord{
        receipt.lbvh_node_index,
        receipt_mask(receipt, witnesses)});
  }
  const std::vector<std::size_t> leaf_nodes =
      leaf_node_indices(index.leaves());
  for (const PointId candidate_point_id : cpu.candidate_point_ids) {
    const auto leaf = std::find_if(
        index.leaves().begin(),
        index.leaves().end(),
        [candidate_point_id](const auto& candidate) {
          return candidate.point_id == candidate_point_id;
        });
    require(
        leaf != index.leaves().end(),
        "every CPU P8b candidate must own a Morton leaf");
    const std::size_t leaf_position =
        static_cast<std::size_t>(leaf - index.leaves().begin());
    records.push_back(AnchoredPairCandidateProposalRecord{
        leaf_nodes[leaf_position], 0U});
  }
  std::sort(
      records.begin(),
      records.end(),
      [](const auto& left, const auto& right) {
        return left.node_index > right.node_index;
      });
  const auto record = std::find_if(
      records.begin(),
      records.end(),
      [&](const AnchoredPairCandidateProposalRecord& candidate) {
        return candidate.node_index == internal_receipt->lbvh_node_index;
      });
  require(record != records.end(), "the internal receipt lost its record");
  const std::size_t internal_record_offset =
      static_cast<std::size_t>(record - records.begin());
  return PruneFixture{
      std::move(cloud),
      std::move(index),
      std::move(cpu),
      std::move(witnesses),
      std::move(records),
      internal_record_offset,
      in_node_witness};
}

void test_two_point_candidate_stream() {
  const CanonicalPointCloud cloud = line_cloud(2U);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::array<PointId, 1U> witnesses{1U};
  const AnchoredPairCandidateProposalQuery query{
      0U, 2U, std::span<const PointId>{witnesses}, {}};
  const auto target_leaf = std::find_if(
      index.leaves().begin(),
      index.leaves().end(),
      [](const auto& leaf) { return leaf.point_id == 1U; });
  require(
      target_leaf != index.leaves().end(),
      "the two-point target must have a Morton leaf");
  const std::size_t target_node_index =
      static_cast<std::size_t>(target_leaf - index.leaves().begin());
  const auto batch = valid_host_batch(
      query,
      cloud.size(),
      2U * cloud.size() - 1U,
      {AnchoredPairCandidateProposalRecord{target_node_index, 0U}},
      3U);
  const std::array<AnchoredPairCandidateProposalQuery, 1U> queries{query};
  const ExactAnchoredPairCandidateRecertifierResult result =
      recertify_exact_anchored_pair_candidate_proposal(
          index, cloud, queries, batch, roomy_budget());
  require(
      result.complete_exact_candidate_stream() &&
          !result.scientific_morse_outcome(),
      "the complete two-point transcript must recertify without a Morse claim");
  require(
          result.anchors.size() == 1U &&
          result.anchors.front().candidate_point_ids ==
              std::vector<PointId>{1U} &&
          result.anchors.front().prune_receipts.empty(),
      "the recertified candidate stream must match CPU P8b");
}

void test_prune_receipts_match_cpu_p8b_and_false_mask_rejects() {
  PruneFixture fixture = make_prune_fixture();
  const AnchoredPairCandidateProposalQuery query{
      0U, 11U, std::span<const PointId>{fixture.witnesses}, {}};
  const std::array<AnchoredPairCandidateProposalQuery, 1U> queries{query};
  const auto valid_batch = valid_host_batch(
      query,
      fixture.cloud.size(),
      2U * fixture.cloud.size() - 1U,
      fixture.records,
      fixture.cpu.audit.node_visit_count);
  const auto valid = recertify_exact_anchored_pair_candidate_proposal(
      fixture.index,
      fixture.cloud,
      queries,
      valid_batch,
      roomy_budget());
  require(
      valid.complete_exact_candidate_stream() &&
          valid.anchors.front().prune_receipts.size() ==
              fixture.cpu.prune_records.size(),
      "the inverse-postorder receipts must close the CPU P8b candidate stream");
  std::vector<PointId> recertified_candidates =
      valid.anchors.front().candidate_point_ids;
  std::vector<PointId> cpu_candidates = fixture.cpu.candidate_point_ids;
  std::sort(recertified_candidates.begin(), recertified_candidates.end());
  std::sort(cpu_candidates.begin(), cpu_candidates.end());
  require(
      recertified_candidates == cpu_candidates,
      "the recertified candidates must equal the CPU P8b candidate set");
  for (const auto& receipt : valid.anchors.front().prune_receipts) {
    const auto expected = std::find_if(
        fixture.cpu.prune_records.begin(),
        fixture.cpu.prune_records.end(),
        [&](const ExactAnchoredPairWitnessBankPruneRecord& candidate) {
          return candidate.lbvh_node_index == receipt.lbvh_node_index;
        });
    require(
        expected != fixture.cpu.prune_records.end() &&
            expected->leaf_begin == receipt.leaf_begin &&
            expected->leaf_end == receipt.leaf_end &&
            expected->witness_count == receipt.witness_count &&
            expected->witness_point_ids == receipt.witness_point_ids,
        "each recertified receipt must equal its CPU P8b receipt");
  }

  auto false_records = fixture.records;
  auto& false_record =
      false_records[fixture.internal_record_offset];
  const auto invalid_witness = std::find(
      fixture.witnesses.begin(),
      fixture.witnesses.end(),
      fixture.in_node_witness);
  require(
      invalid_witness != fixture.witnesses.end(),
      "the false-mask point must belong to the extended bank");
  const std::size_t invalid_bit = static_cast<std::size_t>(
      invalid_witness - fixture.witnesses.begin());
  require(
      (false_record.witness_mask & (UINT64_C(1) << invalid_bit)) == 0U,
      "a point covered by the pruned node cannot certify that node");
  const unsigned int replaced_bit =
      static_cast<unsigned int>(
          std::countr_zero(false_record.witness_mask));
  false_record.witness_mask &= ~(UINT64_C(1) << replaced_bit);
  false_record.witness_mask |= UINT64_C(1) << invalid_bit;
  require(
      std::popcount(false_record.witness_mask) == 10,
      "the forged mask must preserve its host-validated cardinality");
  const auto false_batch = valid_host_batch(
      query,
      fixture.cloud.size(),
      2U * fixture.cloud.size() - 1U,
      std::move(false_records),
      fixture.cpu.audit.node_visit_count);
  const auto rejected = recertify_exact_anchored_pair_candidate_proposal(
      fixture.index,
      fixture.cloud,
      queries,
      false_batch,
      roomy_budget());
  require(
      rejected.atomic_rejection() &&
          rejected.rejection_reason ==
              ExactAnchoredPairCandidateRecertifierRejectionReason::
                  false_prune_record,
      "a cardinality-valid but geometrically false witness mask must reject atomically");
}

void test_missing_and_extra_candidate_reject_atomically() {
  const CanonicalPointCloud cloud = line_cloud(2U);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::array<PointId, 1U> witnesses{1U};
  const AnchoredPairCandidateProposalQuery query{
      0U, 2U, std::span<const PointId>{witnesses}, {}};
  const std::array<AnchoredPairCandidateProposalQuery, 1U> queries{query};
  const auto missing_batch = valid_host_batch(
      query, cloud.size(), 3U, {}, 3U);
  const auto missing = recertify_exact_anchored_pair_candidate_proposal(
      index, cloud, queries, missing_batch, roomy_budget());
  require(
      missing.atomic_rejection() &&
          missing.rejection_reason ==
              ExactAnchoredPairCandidateRecertifierRejectionReason::
                  missing_candidate_record,
      "an omitted oriented leaf must reject the whole batch");

  std::vector<AnchoredPairCandidateProposalRecord> extra_records{
      {1U, 0U}, {0U, 0U}};
  const auto extra_batch = valid_host_batch(
      query,
      cloud.size(),
      3U,
      std::move(extra_records),
      3U);
  const auto extra = recertify_exact_anchored_pair_candidate_proposal(
      index, cloud, queries, extra_batch, roomy_budget());
  require(
      extra.atomic_rejection() &&
          extra.rejection_reason ==
              ExactAnchoredPairCandidateRecertifierRejectionReason::
                  false_candidate_record,
      "a mask-zero record for q<=anchor must reject the whole batch");
}

void test_forged_jump_and_node_visit_count_reject_atomically() {
  PruneFixture fixture = make_prune_fixture();
  const AnchoredPairCandidateProposalQuery query{
      0U, 11U, std::span<const PointId>{fixture.witnesses}, {}};
  const std::array<AnchoredPairCandidateProposalQuery, 1U> queries{query};

  auto jump_records = fixture.records;
  const auto internal_record =
      jump_records[fixture.internal_record_offset];
  require(
      internal_record.node_index > 0U,
      "an internal prune must own a lower node in its subtree");
  jump_records.push_back(AnchoredPairCandidateProposalRecord{
      internal_record.node_index - 1U,
      internal_record.witness_mask});
  std::sort(
      jump_records.begin(),
      jump_records.end(),
      [](const auto& left, const auto& right) {
        return left.node_index > right.node_index;
      });
  const auto jump_batch = valid_host_batch(
      query,
      fixture.cloud.size(),
      2U * fixture.cloud.size() - 1U,
      std::move(jump_records),
      fixture.cpu.audit.node_visit_count + 1U);
  const auto jump = recertify_exact_anchored_pair_candidate_proposal(
      fixture.index,
      fixture.cloud,
      queries,
      jump_batch,
      roomy_budget());
  require(
      jump.atomic_rejection() &&
          jump.rejection_reason ==
              ExactAnchoredPairCandidateRecertifierRejectionReason::
                  traversal_jump,
      "a record hidden inside a skipped subtree must reject atomically");

  const CanonicalPointCloud small_cloud = line_cloud(2U);
  const MortonLbvhIndex small_index =
      MortonLbvhIndex::build(small_cloud);
  const std::array<PointId, 1U> small_witnesses{1U};
  const AnchoredPairCandidateProposalQuery small_query{
      0U, 2U, std::span<const PointId>{small_witnesses}, {}};
  const std::array<AnchoredPairCandidateProposalQuery, 1U>
      small_queries{small_query};
  const auto forged_count_batch = valid_host_batch(
      small_query,
      small_cloud.size(),
      3U,
      {AnchoredPairCandidateProposalRecord{1U, 0U}},
      2U);
  const auto forged_count =
      recertify_exact_anchored_pair_candidate_proposal(
          small_index,
          small_cloud,
          small_queries,
          forged_count_batch,
          roomy_budget());
  require(
      forged_count.atomic_rejection() &&
          forged_count.rejection_reason ==
              ExactAnchoredPairCandidateRecertifierRejectionReason::
                  node_visit_count,
      "a forged visit count must reject atomically after full replay");
}

}  // namespace

int main() {
  try {
    test_two_point_candidate_stream();
    test_prune_receipts_match_cpu_p8b_and_false_mask_rejects();
    test_missing_and_extra_candidate_reject_atomically();
    test_forged_jump_and_node_visit_count_reject_atomically();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
