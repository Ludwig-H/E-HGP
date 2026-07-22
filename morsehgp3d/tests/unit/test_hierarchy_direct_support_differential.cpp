#include "morsehgp3d/hierarchy/critical_catalog.hpp"
#include "morsehgp3d/hierarchy/higher_support_stream.hpp"
#include "morsehgp3d/hierarchy/pair_support_stream.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactCenter3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactCriticalCatalogBudget;
using morsehgp3d::hierarchy::ExactCriticalCatalogCandidateOutcome;
using morsehgp3d::hierarchy::ExactHigherSupportStreamBudget;
using morsehgp3d::hierarchy::ExactPairSupportStreamBudget;
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

enum class RecordKind : std::uint8_t {
  event,
  relevant_extra_shell,
};

struct RecordProjection {
  RecordKind kind{};
  ExactCenter3 center;
  ExactLevel squared_level;
  std::vector<PointId> interior_ids;
  std::size_t shell_count{};
  std::optional<PointId> canonical_extra_shell_witness;
  std::size_t minimum_possible_closed_rank{};
  std::size_t observed_closed_rank{};
  std::size_t exterior_count{};

  friend bool operator==(
      const RecordProjection&, const RecordProjection&) = default;
};

using SupportRecords = std::map<std::vector<PointId>, RecordProjection>;

[[nodiscard]] ExactPairSupportStreamBudget unlimited_pair_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return ExactPairSupportStreamBudget{
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

[[nodiscard]] ExactHigherSupportStreamBudget unlimited_higher_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return ExactHigherSupportStreamBudget{
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

[[nodiscard]] constexpr ExactCriticalCatalogBudget full_catalog_budget() {
  return ExactCriticalCatalogBudget{
      ExactCriticalCatalogBudget::maximum_supported_candidate_count,
      ExactCriticalCatalogBudget::
          maximum_supported_point_classification_count};
}

[[nodiscard]] std::vector<PointId> pair_support_ids(
    const std::array<PointId, 2U>& ids) {
  return {ids[0], ids[1]};
}

[[nodiscard]] std::vector<PointId> higher_support_ids(
    std::uint8_t support_size,
    const std::array<PointId, 4U>& ids) {
  return std::vector<PointId>{
      ids.begin(), ids.begin() + static_cast<std::ptrdiff_t>(support_size)};
}

[[nodiscard]] PointId least_extra_shell_witness(
    const std::vector<PointId>& shell,
    const std::vector<PointId>& support) {
  const auto iterator = std::find_if(
      shell.begin(), shell.end(), [&support](PointId point_id) {
        return !std::binary_search(
            support.begin(), support.end(), point_id);
      });
  if (iterator == shell.end()) {
    throw std::logic_error(
        "an extra-shell candidate omitted its additional shell point");
  }
  return *iterator;
}

[[nodiscard]] SupportRecords oracle_records(
    const morsehgp3d::hierarchy::ExactCriticalCatalogResult& catalog) {
  SupportRecords records;
  for (const auto& candidate : catalog.candidates) {
    const std::size_t support_size = candidate.support_point_ids.size();
    if (support_size < 2U || support_size > 4U) {
      continue;
    }
    if (candidate.outcome !=
            ExactCriticalCatalogCandidateOutcome::accepted_critical_event &&
        candidate.outcome != ExactCriticalCatalogCandidateOutcome::
                                 relevant_extra_shell_degeneracy) {
      continue;
    }
    if (!candidate.center.has_value() ||
        !candidate.squared_level.has_value()) {
      throw std::logic_error(
          "a rank-relevant oracle support omitted its exact sphere");
    }
    const bool diagnostic =
        candidate.outcome == ExactCriticalCatalogCandidateOutcome::
                                 relevant_extra_shell_degeneracy;
    const std::optional<PointId> witness = diagnostic
        ? std::optional<PointId>{least_extra_shell_witness(
              candidate.shell_point_ids, candidate.support_point_ids)}
        : std::nullopt;
    const auto [iterator, inserted] = records.emplace(
        candidate.support_point_ids,
        RecordProjection{
            diagnostic ? RecordKind::relevant_extra_shell
                       : RecordKind::event,
            *candidate.center,
            *candidate.squared_level,
            candidate.interior_point_ids,
            candidate.shell_point_ids.size(),
            witness,
            candidate.support_relevance_rank,
            candidate.observed_closed_rank,
            candidate.exterior_point_ids.size()});
    static_cast<void>(iterator);
    if (!inserted) {
      throw std::logic_error(
          "the exhaustive catalog repeated one support projection");
    }
  }
  return records;
}

struct DirectEvaluation {
  SupportRecords records;
  std::size_t pair_total{};
  morsehgp3d::exact::BigInt higher_total{0};
  bool certified{false};
};

[[nodiscard]] DirectEvaluation evaluate_direct(
    const CanonicalPointCloud& cloud) {
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportStreamBudget pair_budget = unlimited_pair_budget();
  const ExactHigherSupportStreamBudget higher_budget =
      unlimited_higher_budget();
  const auto pair =
      morsehgp3d::hierarchy::build_exact_pair_support_stream(
          index, cloud, 10U, pair_budget);
  const auto higher =
      morsehgp3d::hierarchy::build_exact_higher_support_stream(
          index, cloud, 10U, higher_budget);
  const auto catalog =
      morsehgp3d::hierarchy::build_exact_critical_catalog(
          cloud, 10U, full_catalog_budget());
  SupportRecords direct;
  for (const auto& event : pair.events) {
    const std::vector<PointId> support = pair_support_ids(event.support_ids);
    direct.emplace(
        support,
        RecordProjection{
            RecordKind::event,
            event.center,
            event.squared_level,
            event.interior_ids,
            support.size(),
            std::nullopt,
            event.closed_rank,
            event.closed_rank,
            event.exterior_count});
  }
  for (const auto& diagnostic : pair.relevant_extra_shell_diagnostics) {
    const std::vector<PointId> support =
        pair_support_ids(diagnostic.support_ids);
    direct.emplace(
        support,
        RecordProjection{
            RecordKind::relevant_extra_shell,
            diagnostic.center,
            diagnostic.squared_level,
            diagnostic.interior_ids,
            diagnostic.shell_count,
            diagnostic.canonical_extra_shell_witness_id,
            diagnostic.minimum_possible_closed_rank,
            diagnostic.observed_closed_rank,
            diagnostic.exterior_count});
  }
  for (const auto& event : higher.events) {
    const std::vector<PointId> support =
        higher_support_ids(event.support_size, event.support_ids);
    direct.emplace(
        support,
        RecordProjection{
            RecordKind::event,
            event.center,
            event.squared_level,
            event.interior_ids,
            support.size(),
            std::nullopt,
            event.closed_rank,
            event.closed_rank,
            event.exterior_count});
  }
  for (const auto& diagnostic :
       higher.relevant_extra_shell_diagnostics) {
    const std::vector<PointId> support = higher_support_ids(
        diagnostic.support_size, diagnostic.support_ids);
    direct.emplace(
        support,
        RecordProjection{
            RecordKind::relevant_extra_shell,
            diagnostic.center,
            diagnostic.squared_level,
            diagnostic.interior_ids,
            diagnostic.shell_count,
            diagnostic.canonical_extra_shell_witness_id,
            diagnostic.minimum_possible_closed_rank,
            diagnostic.observed_closed_rank,
            diagnostic.exterior_count});
  }

  const SupportRecords exhaustive = oracle_records(catalog);
  const bool unique_record_count =
      direct.size() == pair.events.size() +
                           pair.relevant_extra_shell_diagnostics.size() +
                           higher.events.size() +
                           higher.relevant_extra_shell_diagnostics.size();
  const bool certified =
      pair.stream_complete() && higher.stream_complete() &&
      catalog.all_support_candidates_classified &&
      catalog.all_minimal_support_global_partitions_complete &&
      catalog.accepted_events_canonical_and_indexed &&
      unique_record_count && direct == exhaustive &&
      pair.no_forbidden_global_structure_materialized &&
      higher.no_forbidden_global_structure_materialized &&
      !pair.hierarchy_reduction_performed &&
      !higher.hierarchy_reduction_performed;
  return DirectEvaluation{
      std::move(direct),
      pair.audit.total_pair_count,
      higher.audit.total_support_count,
      certified};
}

[[nodiscard]] bool same_canonical_cloud(
    const CanonicalPointCloud& left,
    const CanonicalPointCloud& right) {
  if (left.size() != right.size()) {
    return false;
  }
  for (PointId point_id = 0U; point_id < left.size(); ++point_id) {
    if (left.point(point_id).exact() != right.point(point_id).exact()) {
      return false;
    }
  }
  return true;
}

void test_every_size_through_fourteen_and_permutations() {
  std::vector<CertifiedPoint3> all_points;
  all_points.reserve(14U);
  for (std::size_t index = 0U; index < 14U; ++index) {
    const double x = static_cast<double>(index) - 7.0;
    const double y =
        static_cast<double>((index * index + 3U) % 17U) - 8.0;
    const double z = static_cast<double>(
                         (index * index * index + 5U) % 19U) -
                     9.0;
    all_points.push_back(CertifiedPoint3::from_binary64(x, y, z));
  }

  for (std::size_t point_count = 1U; point_count <= 14U; ++point_count) {
    std::vector<CertifiedPoint3> input{
        all_points.begin(),
        all_points.begin() + static_cast<std::ptrdiff_t>(point_count)};
    std::vector<CertifiedPoint3> reversed = input;
    std::reverse(reversed.begin(), reversed.end());
    const CanonicalPointCloud cloud =
        CanonicalPointCloud::rejecting_duplicates(
            std::span<const CertifiedPoint3>{input});
    const CanonicalPointCloud permuted =
        CanonicalPointCloud::rejecting_duplicates(
            std::span<const CertifiedPoint3>{reversed});
    const DirectEvaluation forward = evaluate_direct(cloud);
    std::optional<DirectEvaluation> backward;
    if (point_count == 14U) {
      backward = evaluate_direct(permuted);
    }

    const std::size_t expected_pair_count =
        point_count * (point_count - 1U) / 2U;
    const bool reversed_run_agrees =
        !backward.has_value() ||
        (backward->certified && forward.records == backward->records &&
         backward->pair_total == expected_pair_count &&
         forward.higher_total == backward->higher_total);
    check(
        same_canonical_cloud(cloud, permuted) && forward.certified &&
            forward.pair_total == expected_pair_count &&
            reversed_run_agrees,
        "the complete direct support catalog equals the exhaustive oracle "
        "for n=" +
            std::to_string(point_count) +
            " and is invariant under reversed input order");
  }
}

}  // namespace

int main() {
  test_every_size_through_fourteen_and_permutations();
  if (failures != 0) {
    std::cerr << failures << " direct-support differential checks failed\n";
    return 1;
  }
  std::cout
      << "all direct-support n<=14 and permutation differentials passed\n";
  return 0;
}
