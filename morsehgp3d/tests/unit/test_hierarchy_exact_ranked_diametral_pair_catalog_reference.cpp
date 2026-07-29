#include "morsehgp3d/hierarchy/exact_ranked_diametral_pair_catalog_reference.hpp"

#include "morsehgp3d/exact/predicates.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::hierarchy::ExactRankedDiametralPairCatalogReferenceResult;
using morsehgp3d::hierarchy::ExactRankedDiametralPairRecord;
using morsehgp3d::hierarchy::
    build_exact_ranked_diametral_pair_catalog_reference;
using morsehgp3d::hierarchy::exact_ball_supported_gabriel_subset_count;
using morsehgp3d::hierarchy::
    exact_ranked_diametral_pair_closed_point_ids;
using morsehgp3d::hierarchy::
    exact_ranked_diametral_pair_gabriel_subset_count;
using morsehgp3d::hierarchy::
    exact_pair_supported_quadruplet_candidate_count;
using morsehgp3d::hierarchy::exact_pair_supported_triplet_candidate_count;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::vector<CertifiedPoint3>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] PointId canonical_id_from_source(
    const CanonicalPointCloud& cloud,
    std::size_t source_index) {
  for (std::size_t index = 0U; index < cloud.size(); ++index) {
    const PointId point_id = static_cast<PointId>(index);
    if (cloud.source_index(point_id) == source_index) {
      return point_id;
    }
  }
  throw std::logic_error("a source point has no canonical PointId");
}

[[nodiscard]] int phi_sign(
    const CanonicalPointCloud& cloud,
    PointId first,
    PointId second,
    PointId witness) {
  ExactRational phi;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    phi = phi +
        (cloud.point(witness).coordinate(axis) -
         cloud.point(first).coordinate(axis)) *
            (cloud.point(witness).coordinate(axis) -
             cloud.point(second).coordinate(axis));
  }
  return phi.sign();
}

struct BrutePairRecord {
  std::array<PointId, 2> support_ids{};
  ExactLevel squared_level{};
  std::vector<PointId> interior_ids;
  std::vector<PointId> extra_shell_ids;
  std::vector<PointId> closed_ids;
};

[[nodiscard]] std::vector<BrutePairRecord> brute_bucket(
    const CanonicalPointCloud& cloud,
    std::size_t target_closed_rank) {
  std::vector<BrutePairRecord> result;
  for (std::size_t first_index = 0U;
       first_index < cloud.size();
       ++first_index) {
    const PointId first = static_cast<PointId>(first_index);
    for (std::size_t second_index = first_index + 1U;
         second_index < cloud.size();
         ++second_index) {
      const PointId second = static_cast<PointId>(second_index);
      BrutePairRecord record;
      record.support_ids = {first, second};
      const ExactRational pair_squared_distance =
          morsehgp3d::exact::squared_distance(
              cloud.point(first), cloud.point(second));
      record.squared_level = ExactLevel{ExactRational{
          pair_squared_distance.numerator(),
          morsehgp3d::exact::BigInt{4} *
              pair_squared_distance.denominator()}};
      for (std::size_t witness_index = 0U;
           witness_index < cloud.size();
           ++witness_index) {
        const PointId witness = static_cast<PointId>(witness_index);
        const int sign = phi_sign(cloud, first, second, witness);
        if (sign > 0) {
          continue;
        }
        record.closed_ids.push_back(witness);
        if (sign < 0) {
          record.interior_ids.push_back(witness);
        } else if (witness != first && witness != second) {
          record.extra_shell_ids.push_back(witness);
        }
      }
      if (record.closed_ids.size() == target_closed_rank) {
        result.push_back(std::move(record));
      }
    }
  }
  return result;
}

void check_against_all_pairs(
    const CanonicalPointCloud& cloud,
    const MortonLbvhIndex& index,
    std::size_t requested_order) {
  const ExactRankedDiametralPairCatalogReferenceResult observed =
      build_exact_ranked_diametral_pair_catalog_reference(
          index, cloud, requested_order);
  const ExactRankedDiametralPairCatalogReferenceResult replay =
      build_exact_ranked_diametral_pair_catalog_reference(
          index, cloud, requested_order);
  const std::vector<BrutePairRecord> expected =
      brute_bucket(cloud, requested_order + 1U);
  check(observed == replay, "the end-to-end pair catalogue is deterministic");
  check(
      observed.locally_structurally_valid(),
      "the end-to-end pair catalogue closes its local accounting");
  check(
      observed.records.size() == expected.size(),
      "the Yao48-to-exact catalogue has the exhaustive bucket extent");
  if (observed.records.size() != expected.size()) {
    return;
  }
  for (std::size_t record_index = 0U;
       record_index < expected.size();
       ++record_index) {
    const ExactRankedDiametralPairRecord& actual =
        observed.records[record_index];
    const BrutePairRecord& brute = expected[record_index];
    check(
        actual.support_ids == brute.support_ids &&
            actual.squared_level == brute.squared_level &&
            actual.strict_interior_ids == brute.interior_ids &&
            actual.extra_shell_ids == brute.extra_shell_ids &&
            exact_ranked_diametral_pair_closed_point_ids(actual) ==
                brute.closed_ids,
        "each exact bucket record retains the complete closed ball");
  }
}

void test_every_rank_bucket_matches_all_pairs() {
  const CanonicalPointCloud cloud = canonical_cloud({
      point(-7.0, 2.0, 5.0),
      point(-4.0, -6.0, 1.0),
      point(-3.0, 7.0, -5.0),
      point(-1.0, -2.0, -8.0),
      point(0.0, 0.0, 0.0),
      point(1.0, 9.0, 3.0),
      point(2.0, -7.0, 6.0),
      point(4.0, 3.0, -6.0),
      point(6.0, -1.0, -3.0),
      point(8.0, 5.0, 2.0),
      point(9.0, -4.0, 7.0),
  });
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  std::size_t record_count_across_buckets = 0U;
  for (std::size_t requested_order = 1U;
       requested_order <= 10U;
       ++requested_order) {
    check_against_all_pairs(cloud, index, requested_order);
    record_count_across_buckets +=
        brute_bucket(cloud, requested_order + 1U).size();
  }
  check(
      record_count_across_buckets == cloud.size() * (cloud.size() - 1U) / 2U,
      "every unordered pair occurs in exactly one closed-rank bucket");
}

void test_cosphere_payload_is_complete() {
  const CanonicalPointCloud cloud = canonical_cloud({
      point(-1.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(0.0, -1.0, 0.0),
      point(0.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
  });
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const PointId first = canonical_id_from_source(cloud, 0U);
  const PointId second = canonical_id_from_source(cloud, 1U);
  const auto result = build_exact_ranked_diametral_pair_catalog_reference(
      index, cloud, 4U);
  const auto record = std::find_if(
      result.records.begin(),
      result.records.end(),
      [first, second](const ExactRankedDiametralPairRecord& candidate) {
        return candidate.support_ids ==
            std::array<PointId, 2>{std::min(first, second),
                                   std::max(first, second)};
      });
  check(
      record != result.records.end() && record->closed_rank == 5U &&
          record->strict_interior_ids.size() == 1U &&
          record->extra_shell_ids.size() == 2U,
      "a rank-five cosphere keeps its interior and complete extra shell");
  if (record != result.records.end()) {
    check(
        exact_pair_supported_triplet_candidate_count(*record) == 3U &&
            exact_pair_supported_quadruplet_candidate_count(*record) == 3U,
        "a rank-five pair record reserves exactly three triplet and three quadruplet candidates");
    check(
        exact_ranked_diametral_pair_gabriel_subset_count(*record, 2U) ==
                0U &&
            exact_ranked_diametral_pair_gabriel_subset_count(*record, 3U) ==
                1U &&
            exact_ranked_diametral_pair_gabriel_subset_count(*record, 4U) ==
                2U &&
            exact_ranked_diametral_pair_gabriel_subset_count(*record, 5U) ==
                1U &&
            exact_ranked_diametral_pair_gabriel_subset_count(*record, 6U) ==
                0U,
        "strict interiors are mandatory and extra-shell points are optional in Gabriel subsets");
  }
}

void test_generic_gabriel_subset_count() {
  check(
      exact_ball_supported_gabriel_subset_count(2U, 0U, 3U, 2U) == 1U &&
          exact_ball_supported_gabriel_subset_count(2U, 0U, 3U, 3U) == 3U &&
          exact_ball_supported_gabriel_subset_count(2U, 0U, 3U, 4U) == 3U &&
          exact_ball_supported_gabriel_subset_count(2U, 0U, 3U, 5U) == 1U,
      "a shell-only pair record contributes the expected binomial row");
  check(
      exact_ball_supported_gabriel_subset_count(3U, 2U, 4U, 4U) == 0U &&
          exact_ball_supported_gabriel_subset_count(3U, 2U, 4U, 5U) == 1U &&
          exact_ball_supported_gabriel_subset_count(3U, 2U, 4U, 7U) == 6U &&
          exact_ball_supported_gabriel_subset_count(3U, 2U, 4U, 9U) == 1U &&
          exact_ball_supported_gabriel_subset_count(3U, 2U, 4U, 10U) == 0U,
      "a generic support record includes every strict interior before choosing shell points");

  bool empty_support_rejected = false;
  try {
    static_cast<void>(
        exact_ball_supported_gabriel_subset_count(0U, 0U, 0U, 0U));
  } catch (const std::invalid_argument&) {
    empty_support_rejected = true;
  }
  check(empty_support_rejected, "an empty ball support is rejected");

  bool overflowing_count_rejected = false;
  try {
    static_cast<void>(
        exact_ball_supported_gabriel_subset_count(1U, 0U, 68U, 35U));
  } catch (const std::overflow_error&) {
    overflowing_count_rejected = true;
  }
  check(
      overflowing_count_rejected,
      "a Gabriel subset count that exceeds size_t fails explicitly");

  ExactRankedDiametralPairRecord inconsistent;
  inconsistent.support_ids = {0U, 1U};
  inconsistent.squared_level = ExactLevel{1};
  inconsistent.strict_interior_ids = {2U};
  inconsistent.closed_rank = 2U;
  bool inconsistent_record_rejected = false;
  try {
    static_cast<void>(
        exact_ranked_diametral_pair_gabriel_subset_count(inconsistent, 3U));
  } catch (const std::logic_error&) {
    inconsistent_record_rejected = true;
  }
  check(
      inconsistent_record_rejected,
      "the pair-record overload rejects inconsistent closed cardinalities");
}

void test_local_validator_rejects_forged_mass_and_non_candidate_record() {
  const CanonicalPointCloud cloud = canonical_cloud({
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(3.0, 0.0, 0.0),
  });
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto authentic = build_exact_ranked_diametral_pair_catalog_reference(
      index, cloud, 1U);
  check(
      authentic.records.size() == 2U &&
          authentic.proposal.counters.cutoff_pruned_pair_count == 1U,
      "the forged-record fixture must expose records and one pruned pair");

  auto overflowing = authentic;
  overflowing.counters.yao48_cutoff_pruned_pair_count =
      std::numeric_limits<std::size_t>::max();
  check(
      !overflowing.locally_structurally_valid(),
      "an overflowing catalogue mass partition must fail closed");

  auto overflowing_proposal = authentic;
  overflowing_proposal.proposal.counters.candidate_pair_count =
      std::numeric_limits<std::size_t>::max();
  check(
      !overflowing_proposal.proposal.locally_structurally_valid(),
      "an overflowing Yao48 pair partition must fail closed");

  if (authentic.records.empty()) {
    return;
  }
  std::array<PointId, 2> pruned_support{};
  bool found_pruned_support = false;
  for (std::size_t first_index = 0U;
       first_index < cloud.size() && !found_pruned_support;
       ++first_index) {
    for (std::size_t second_index = first_index + 1U;
         second_index < cloud.size();
         ++second_index) {
      const std::array<PointId, 2> support{
          static_cast<PointId>(first_index),
          static_cast<PointId>(second_index)};
      const auto candidate = std::find_if(
          authentic.proposal.candidates.begin(),
          authentic.proposal.candidates.end(),
          [support](const auto& entry) {
            return entry.support_ids == support;
          });
      if (candidate == authentic.proposal.candidates.end()) {
        pruned_support = support;
        found_pruned_support = true;
        break;
      }
    }
  }
  check(
      found_pruned_support,
      "the forged-record fixture must identify its pruned pair");
  if (!found_pruned_support) {
    return;
  }

  auto forged = authentic;
  forged.records.resize(1U);
  forged.records.front().support_ids = pruned_support;
  forged.counters.below_rank_candidate_count +=
      authentic.records.size() - 1U;
  forged.counters.exact_rank_candidate_count = 1U;
  forged.counters.output_record_count = 1U;
  forged.counters.output_closed_point_reference_count =
      forged.records.front().closed_rank;
  check(
      !forged.locally_structurally_valid(),
      "a structurally plausible record absent from the Yao48 superset must fail closed");
}

}  // namespace

int main() {
  test_every_rank_bucket_matches_all_pairs();
  test_cosphere_payload_is_complete();
  test_generic_gabriel_subset_count();
  test_local_validator_rejects_forged_mass_and_non_candidate_record();
  if (failures != 0) {
    std::cerr << failures
              << " exact ranked diametral pair catalogue test(s) failed\n";
    return 1;
  }
  return 0;
}
