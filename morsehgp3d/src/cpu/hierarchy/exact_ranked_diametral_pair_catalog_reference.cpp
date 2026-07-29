#include "morsehgp3d/hierarchy/exact_ranked_diametral_pair_catalog_reference.hpp"

#include "morsehgp3d/hierarchy/anchored_pair_candidate_classifier.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>

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

[[nodiscard]] bool record_less(
    const ExactRankedDiametralPairRecord& left,
    const ExactRankedDiametralPairRecord& right) noexcept {
  return left.support_ids < right.support_ids;
}

}  // namespace

bool ExactRankedDiametralPairCatalogReferenceResult::
    locally_structurally_valid() const {
  std::size_t pair_partition_count = 0U;
  std::size_t candidate_partition_count = 0U;
  try {
    pair_partition_count = checked_add(
        counters.yao48_cutoff_pruned_pair_count,
        counters.classified_candidate_pair_count,
        "the local ranked pair mass partition overflows size_t");
    candidate_partition_count = checked_add(
        checked_add(
            counters.below_rank_candidate_count,
            counters.exact_rank_candidate_count,
            "the local ranked pair trichotomy overflows size_t"),
        counters.above_rank_candidate_count,
        "the local ranked pair trichotomy overflows size_t");
  } catch (const std::exception&) {
    return false;
  }
  if (schema_version !=
          exact_ranked_diametral_pair_catalog_reference_schema_version ||
      point_count < 2U ||
      point_count >
          exact_yao48_ranked_pair_candidate_reference_max_point_count ||
      requested_order == 0U ||
      requested_order > exact_yao48_ranked_pair_candidate_maximum_order ||
      target_closed_rank != requested_order + 1U ||
      target_closed_rank > point_count ||
      proposal.point_count != point_count ||
      proposal.requested_order != requested_order ||
      !proposal.locally_structurally_valid() ||
      counters.point_count != point_count ||
      counters.unordered_pair_count !=
          proposal.counters.unordered_pair_count ||
      counters.yao48_cutoff_pruned_pair_count !=
          proposal.counters.cutoff_pruned_pair_count ||
      counters.classified_candidate_pair_count !=
          proposal.counters.candidate_pair_count ||
      pair_partition_count != counters.unordered_pair_count ||
      candidate_partition_count !=
          counters.classified_candidate_pair_count ||
      counters.exact_rank_candidate_count != records.size() ||
      counters.output_record_count != records.size() ||
      !counters.complete_pair_mass_accounting_closed ||
      !counters.complete_candidate_trichotomy_accounting_closed ||
      !counters.bounded_quadratic_reference_only ||
      counters.higher_order_delaunay_mosaic_materialized ||
      counters.public_status_claimed ||
      !std::is_sorted(records.begin(), records.end(), record_less)) {
    return false;
  }

  std::size_t closed_reference_count = 0U;
  std::size_t proposal_index = 0U;
  for (std::size_t index = 0U; index < records.size(); ++index) {
    const ExactRankedDiametralPairRecord& record = records[index];
    while (proposal_index < proposal.candidates.size() &&
           proposal.candidates[proposal_index].support_ids <
               record.support_ids) {
      ++proposal_index;
    }
    if (!exact_ranked_diametral_pair_record_well_formed(
            record, point_count, target_closed_rank) ||
        record.closed_rank != target_closed_rank ||
        proposal_index == proposal.candidates.size() ||
        proposal.candidates[proposal_index].support_ids !=
            record.support_ids ||
        (index != 0U &&
         records[index - 1U].support_ids == record.support_ids)) {
      return false;
    }
    try {
      closed_reference_count = checked_add(
          closed_reference_count,
          record.closed_rank,
          "the ranked pair catalogue reference count overflows size_t");
    } catch (const std::exception&) {
      return false;
    }
  }
  return closed_reference_count ==
      counters.output_closed_point_reference_count;
}

ExactRankedDiametralPairCatalogReferenceResult
build_exact_ranked_diametral_pair_catalog_reference(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_order) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "the ranked pair catalogue reference requires the authentic LBVH");
  }

  ExactRankedDiametralPairCatalogReferenceResult result;
  result.point_count = cloud.size();
  result.requested_order = requested_order;
  result.target_closed_rank = checked_add(
      requested_order,
      1U,
      "the ranked pair target closed rank overflows size_t");
  result.proposal =
      build_exact_yao48_ranked_pair_candidate_reference(
          cloud, requested_order);
  result.counters.point_count = cloud.size();
  result.counters.unordered_pair_count =
      result.proposal.counters.unordered_pair_count;
  result.counters.yao48_cutoff_pruned_pair_count =
      result.proposal.counters.cutoff_pruned_pair_count;
  result.records.reserve(result.proposal.candidates.size());

  const ExactAnchoredPairCandidateClassificationBudget unlimited{
      std::numeric_limits<std::size_t>::max()};
  for (const ExactYao48RankedPairCandidate& candidate :
       result.proposal.candidates) {
    ExactAnchoredPairCandidateClassificationResult classification =
        classify_exact_ranked_diametral_pair_candidate(
            index,
            cloud,
            candidate.support_ids,
            result.target_closed_rank,
            unlimited);
    result.counters.classified_candidate_pair_count = checked_add(
        result.counters.classified_candidate_pair_count,
        1U,
        "the ranked pair classified candidate count overflows size_t");
    result.counters.exact_classifier_node_visit_count = checked_add(
        result.counters.exact_classifier_node_visit_count,
        classification.audit.node_visit_count,
        "the ranked pair exact node visit count overflows size_t");
    switch (classification.status) {
      case ExactAnchoredPairCandidateClassificationStatus::below_rank:
        ++result.counters.below_rank_candidate_count;
        break;
      case ExactAnchoredPairCandidateClassificationStatus::above_rank:
        ++result.counters.above_rank_candidate_count;
        break;
      case ExactAnchoredPairCandidateClassificationStatus::complete:
        if (!classification.ranked_pair_record.has_value() ||
            classification.event.has_value() ||
            classification.relevant_extra_shell_diagnostic.has_value()) {
          throw std::logic_error(
              "the exact ranked pair classifier returned an invalid payload");
        }
        result.counters.output_closed_point_reference_count = checked_add(
            result.counters.output_closed_point_reference_count,
            classification.ranked_pair_record->closed_rank,
            "the ranked pair output reference count overflows size_t");
        result.records.push_back(
            std::move(*classification.ranked_pair_record));
        ++result.counters.exact_rank_candidate_count;
        break;
      case ExactAnchoredPairCandidateClassificationStatus::budget_exhausted:
        throw std::logic_error(
            "an unlimited bounded ranked pair classification exhausted");
    }
  }

  std::sort(result.records.begin(), result.records.end(), record_less);
  result.counters.output_record_count = result.records.size();
  result.counters.complete_pair_mass_accounting_closed =
      result.counters.yao48_cutoff_pruned_pair_count +
          result.counters.classified_candidate_pair_count ==
      result.counters.unordered_pair_count;
  result.counters.complete_candidate_trichotomy_accounting_closed =
      result.counters.below_rank_candidate_count +
              result.counters.exact_rank_candidate_count +
              result.counters.above_rank_candidate_count ==
          result.counters.classified_candidate_pair_count;
  if (!result.locally_structurally_valid()) {
    throw std::logic_error(
        "the ranked diametral pair catalogue reference did not close");
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
