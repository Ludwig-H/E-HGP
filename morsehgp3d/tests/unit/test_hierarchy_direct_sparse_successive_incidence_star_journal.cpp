#include "morsehgp3d/hierarchy/direct_sparse_successive_incidence_star_journal.hpp"
#include "morsehgp3d/spatial/brute_force.hpp"
#include "morsehgp3d/spatial/query.hpp"

#include "fixtures/moment_curve_k3_one_step_star_incompleteness_n9.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;
namespace fixture = morsehgp3d::tests::fixtures;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::vector<CertifiedPoint3>& points) {
  return CanonicalPointCloud::rejecting_duplicates(points);
}

[[nodiscard]] ExactLevel level(
    std::int64_t numerator,
    std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

[[nodiscard]] BigInt affine_determinant(
    const fixture::MomentCurveSourcePoint& first,
    const fixture::MomentCurveSourcePoint& second,
    const fixture::MomentCurveSourcePoint& third,
    const fixture::MomentCurveSourcePoint& fourth) {
  const std::array<const fixture::MomentCurveSourcePoint*, 3U> rows{
      &second, &third, &fourth};
  std::array<std::array<BigInt, 3U>, 3U> matrix{};
  for (std::size_t row = 0U; row < matrix.size(); ++row) {
    for (std::size_t axis = 0U; axis < matrix[row].size(); ++axis) {
      matrix[row][axis] =
          BigInt{(*rows[row])[axis]} - BigInt{first[axis]};
    }
  }
  return matrix[0U][0U] *
             (matrix[1U][1U] * matrix[2U][2U] -
              matrix[1U][2U] * matrix[2U][1U]) -
         matrix[0U][1U] *
             (matrix[1U][0U] * matrix[2U][2U] -
              matrix[1U][2U] * matrix[2U][0U]) +
         matrix[0U][2U] *
             (matrix[1U][0U] * matrix[2U][1U] -
              matrix[1U][1U] * matrix[2U][0U]);
}

[[nodiscard]] PointId point_id_for_source(
    const CanonicalPointCloud& cloud,
    std::size_t source_index) {
  for (std::size_t index = 0U; index < cloud.size(); ++index) {
    const PointId point_id = static_cast<PointId>(index);
    if (cloud.source_index(point_id) == source_index) {
      return point_id;
    }
  }
  throw std::logic_error("a fixture source point is absent from the cloud");
}

template <std::size_t Size>
[[nodiscard]] std::array<PointId, Size> point_ids_for_sources(
    const CanonicalPointCloud& cloud,
    const std::array<std::size_t, Size>& source_indices) {
  std::array<PointId, Size> point_ids{};
  for (std::size_t index = 0U; index < Size; ++index) {
    point_ids[index] = point_id_for_source(cloud, source_indices[index]);
  }
  std::sort(point_ids.begin(), point_ids.end());
  return point_ids;
}

template <std::size_t Size>
[[nodiscard]] std::array<std::size_t, Size> source_indices_for_point_ids(
    const CanonicalPointCloud& cloud,
    const std::array<PointId, Size>& point_ids) {
  std::array<std::size_t, Size> source_indices{};
  for (std::size_t index = 0U; index < Size; ++index) {
    source_indices[index] = cloud.source_index(point_ids[index]);
  }
  std::sort(source_indices.begin(), source_indices.end());
  return source_indices;
}

template <std::size_t Size>
[[nodiscard]] std::vector<PointId> point_id_vector_for_sources(
    const CanonicalPointCloud& cloud,
    const std::array<std::size_t, Size>& source_indices) {
  const auto point_ids = point_ids_for_sources(cloud, source_indices);
  return {point_ids.begin(), point_ids.end()};
}

[[nodiscard]] std::vector<std::size_t> source_indices_for_point_ids(
    const CanonicalPointCloud& cloud,
    std::span<const PointId> point_ids) {
  std::vector<std::size_t> source_indices;
  source_indices.reserve(point_ids.size());
  for (const PointId point_id : point_ids) {
    source_indices.push_back(cloud.source_index(point_id));
  }
  std::sort(source_indices.begin(), source_indices.end());
  return source_indices;
}

template <std::size_t Size>
[[nodiscard]] ExactDirectSparseFacetKey facet_key_for_sources(
    const CanonicalPointCloud& cloud,
    const std::array<std::size_t, Size>& source_indices) {
  static_assert(Size >= 1U);
  static_assert(
      Size <= direct_sparse_successive_incidence_maximum_source_point_count);
  const auto point_ids = point_ids_for_sources(cloud, source_indices);
  ExactDirectSparseFacetKey key;
  key.point_count = Size;
  std::copy(point_ids.begin(), point_ids.end(), key.point_ids.begin());
  return key;
}

[[nodiscard]] ExactPairSupportStreamBudget unlimited_pair_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactHigherSupportStreamBudget unlimited_higher_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
  };
}

[[nodiscard]] ExactDirectSaddleArmSeedBudget unlimited_arm_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectClosedSaddleIncidenceBudget
unlimited_incidence_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceBudget
generous_successive_budget() {
  return {
      1024U,
      65536U,
      65536U,
      1048576U,
      65536U,
      1048576U,
      16777216U,
      65536U,
      65536U,
  };
}

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceStarJournalBudget
generous_budget() {
  return {
      1024U,
      11264U,
      11264U,
      112640U,
      131072U,
      131072U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      8388608U,
      generous_successive_budget(),
  };
}

struct DirectSources {
  ExactPairSupportStreamResult pair_stream;
  ExactHigherSupportStreamResult higher_stream;
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult event_journal;
  ExactDirectSaddleArmSeedBudget arm_budget;
  ExactDirectSaddleArmSeedJournalResult arm_journal;
  ExactDirectClosedSaddleIncidenceBudget incidence_budget;
  ExactDirectClosedSaddleIncidenceJournalResult incidence_journal;
};

[[nodiscard]] DirectSources direct_sources(
    const CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order) {
  const ExactDirectSupportTerminalBudget terminal_budget{
      unlimited_pair_budget(), unlimited_higher_budget()};
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  auto pair = build_exact_pair_support_stream(
      index, cloud, requested_maximum_order, terminal_budget.pair);
  auto higher = build_exact_higher_support_stream(
      index, cloud, requested_maximum_order, terminal_budget.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index,
      cloud,
      requested_maximum_order,
      terminal_budget,
      pair,
      higher);
  auto event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  const auto arm_budget = unlimited_arm_budget();
  auto arm_journal = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, event_journal, arm_budget);
  const auto incidence_budget = unlimited_incidence_budget();
  auto incidence_journal =
      build_exact_direct_closed_saddle_incidence_journal(
          cloud,
          facade,
          event_journal,
          arm_budget,
          arm_journal,
          incidence_budget);
  return {
      std::move(pair),
      std::move(higher),
      std::move(facade),
      std::move(event_journal),
      arm_budget,
      std::move(arm_journal),
      incidence_budget,
      std::move(incidence_journal),
  };
}

[[nodiscard]] bool complete_verification(
    const ExactDirectSparseSuccessiveIncidenceStarJournalVerification&
        verification) {
  return verification.observed_storage_within_budget &&
         verification.source_incidence_journal_freshly_replayed &&
         verification.expected_result_freshly_reconstructed &&
         verification.observed_recursively_equal &&
         verification.no_forbidden_global_structure_materialized &&
         verification.fresh_replay_certified &&
         verification.result_certified;
}

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceStarJournalResult
build_and_verify(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const DirectSources& source,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget& budget,
    LbvhTraversalOrder traversal_order,
    const std::string& context) {
  const auto result =
      build_exact_direct_sparse_successive_incidence_star_journal(
          index,
          cloud,
          source.facade,
          source.event_journal,
          source.arm_budget,
          source.arm_journal,
          source.incidence_budget,
          source.incidence_journal,
          budget,
          traversal_order);
  const auto verification =
      verify_exact_direct_sparse_successive_incidence_star_journal(
          index,
          cloud,
          source.facade,
          source.event_journal,
          source.arm_budget,
          source.arm_journal,
          source.incidence_budget,
          source.incidence_journal,
          budget,
          traversal_order,
          result);
  check(
      complete_verification(verification),
      context + " closes under a fresh recursive replay");
  return result;
}

[[nodiscard]] std::vector<PointId> reconstructed_ids(
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& result,
    std::size_t coface_index) {
  const auto key =
      reconstruct_exact_direct_sparse_successive_incidence_star_coface(
          result, coface_index);
  return {
      key.point_ids.begin(),
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(key.point_count),
  };
}

struct ScientificCoface {
  std::vector<PointId> ids;
  ExactLevel squared_level;
  ExactDirectSparseSuccessiveIncidenceStarCofaceKind kind;

  friend bool operator==(
      const ScientificCoface&, const ScientificCoface&) = default;
};

[[nodiscard]] std::vector<ScientificCoface> scientific_cofaces(
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& result) {
  std::vector<ScientificCoface> projection;
  projection.reserve(result.cofaces.size());
  for (const auto& coface : result.cofaces) {
    projection.push_back({
        reconstructed_ids(result, coface.coface_index),
        coface.squared_level,
        coface.kind});
  }
  return projection;
}

[[nodiscard]] bool scientific_arenas_empty(
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& result) {
  return result.facet_tokens.empty() && result.shells.empty() &&
         result.cofaces.empty() && result.residual_batches.empty() &&
         result.residual_batch_coface_indices.empty();
}

void observe_audit_maxima(
    const ExactDirectSparseSuccessiveIncidenceAudit& audit,
    ExactDirectSparseSuccessiveIncidenceBudget& budget) {
  budget.maximum_source_support_enumeration_count = std::max(
      budget.maximum_source_support_enumeration_count,
      audit.source_support_enumeration_count);
  budget.maximum_node_visit_count =
      std::max(budget.maximum_node_visit_count, audit.node_visit_count);
  budget.maximum_internal_node_expansion_count = std::max(
      budget.maximum_internal_node_expansion_count,
      audit.internal_node_expansion_count);
  budget.maximum_exact_aabb_bound_evaluation_count = std::max(
      budget.maximum_exact_aabb_bound_evaluation_count,
      audit.exact_aabb_bound_evaluation_count);
  budget.maximum_exact_point_evaluation_count = std::max(
      budget.maximum_exact_point_evaluation_count,
      audit.exact_point_evaluation_count);
  budget.maximum_coface_support_enumeration_count = std::max(
      budget.maximum_coface_support_enumeration_count,
      audit.coface_support_enumeration_count);
  budget.maximum_candidate_point_classification_count = std::max(
      budget.maximum_candidate_point_classification_count,
      audit.candidate_point_classification_count);
  budget.maximum_frontier_entry_count = std::max(
      budget.maximum_frontier_entry_count, audit.peak_frontier_entry_count);
  budget.maximum_cominimizer_count = std::max(
      budget.maximum_cominimizer_count,
      audit.peak_cominimizer_entry_count);
}

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceStarJournalBudget
exact_budget_for(
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& result) {
  ExactDirectSparseSuccessiveIncidenceBudget child;
  for (const auto& shell : result.shells) {
    observe_audit_maxima(shell.query_audit, child);
  }
  for (const auto& token : result.facet_tokens) {
    observe_audit_maxima(token.terminal_query_audit, child);
  }
  return {
      result.required_source_family_scan_count,
      result.required_deletion_reference_count,
      result.required_distinct_facet_count,
      result.required_facet_key_point_count,
      result.required_successive_incidence_call_count,
      result.required_incidence_shell_count,
      result.required_coface_occurrence_count,
      result.required_distinct_coface_count,
      result.required_direct_coface_count,
      result.required_residual_coface_count,
      result.required_residual_batch_count,
      result.required_residual_batch_reference_count,
      result.logical_storage_entry_count,
      child,
  };
}

void test_e5_residual_shells_and_canonical_owners() {
  const std::array<CertifiedPoint3, 5U> points{
      point(-2.0, -1.0),
      point(-2.0, 1.0),
      point(0.0, 0.0),
      point(3.0, 2.0),
      point(4.0, -1.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const DirectSources source = direct_sources(cloud, 2U);
  const auto near = build_and_verify(
      index,
      cloud,
      source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the E5 near-first successive-incidence star");
  const auto far = build_and_verify(
      index,
      cloud,
      source,
      generous_budget(),
      LbvhTraversalOrder::far_first,
      "the E5 far-first successive-incidence star");

  std::vector<std::pair<std::vector<PointId>, ExactLevel>> residual;
  for (const auto& coface : near.cofaces) {
    if (coface.kind ==
        ExactDirectSparseSuccessiveIncidenceStarCofaceKind::residual) {
      residual.emplace_back(
          reconstructed_ids(near, coface.coface_index),
          coface.squared_level);
    }
  }
  const std::vector<std::pair<std::vector<PointId>, ExactLevel>> expected{
      {{0U, 1U, 3U}, level(17, 2)},
      {{0U, 1U, 4U}, level(10)},
      {{0U, 2U, 3U}, level(17, 2)},
      {{0U, 3U, 4U}, level(85, 9)},
      {{1U, 2U, 4U}, level(10)},
      {{1U, 3U, 4U}, level(10)},
  };
  check(
      near.certified_bounded_star() &&
          near.required_distinct_facet_count == 8U &&
          near.required_distinct_coface_count == 10U &&
          near.required_direct_coface_count == 4U &&
          near.required_residual_coface_count == 6U &&
          residual == expected,
      "E5 retains exactly 013/023 at 17/2, 034 at 85/9 and 014/124/134 at 10 as residual cofaces");
  check(
      near.required_source_family_scan_count == 10U &&
          near.required_higher_order_family_count == 4U &&
          near.excluded_order_one_family_count == 6U &&
          near.order_one_families_excluded_to_preserve_boruvka_authority &&
          std::all_of(
              near.facet_tokens.begin(),
              near.facet_tokens.end(),
              [](const auto& token) {
                return token.source_facet_key.point_count >= 2U;
              }),
      "E5 scans all ten saddle families but excludes six K1 families from the higher-order star");
  check(
      near.residual_batches.size() == 3U &&
          near.residual_batches[0U].order == 2U &&
          near.residual_batches[0U].squared_level == level(17, 2) &&
          near.residual_batches[0U].residual_coface_index_count == 2U &&
          near.residual_batches[1U].squared_level == level(85, 9) &&
          near.residual_batches[1U].residual_coface_index_count == 1U &&
          near.residual_batches[2U].squared_level == level(10) &&
          near.residual_batches[2U].residual_coface_index_count == 3U,
      "E5 residual cofaces form the three canonical (order, level) batches");
  check(
      std::all_of(
          near.facet_tokens.begin(),
          near.facet_tokens.end(),
          [](const auto& token) { return token.complete_no_later_coface; }) &&
          near.required_successive_incidence_call_count ==
              near.required_incidence_shell_count +
                  near.required_distinct_facet_count &&
          near.aggregate_successive_work_entry_count <=
              near.aggregate_successive_work_entry_limit,
      "every E5 facet has one terminal no-later-coface query and bounded aggregate child work");
  check(
      scientific_cofaces(near) == scientific_cofaces(far) &&
          near.residual_batches == far.residual_batches &&
          near.residual_batch_coface_indices ==
              far.residual_batch_coface_indices,
      "near-first and far-first traversal preserve all factorized scientific cofaces and residual batches");
}

void test_moment_curve_k3_one_step_star_incompleteness_n9() {
  const auto& integer_points =
      fixture::moment_curve_k3_one_step_star_incompleteness_n9_points;
  std::size_t affine_quadruplet_count = 0U;
  std::size_t affinely_dependent_quadruplet_count = 0U;
  std::optional<BigInt> minimum_absolute_affine_determinant;
  for (std::size_t first = 0U; first < integer_points.size(); ++first) {
    for (std::size_t second = first + 1U;
         second < integer_points.size();
         ++second) {
      for (std::size_t third = second + 1U;
           third < integer_points.size();
           ++third) {
        for (std::size_t fourth = third + 1U;
             fourth < integer_points.size();
             ++fourth) {
          BigInt determinant = affine_determinant(
              integer_points[first],
              integer_points[second],
              integer_points[third],
              integer_points[fourth]);
          ++affine_quadruplet_count;
          if (determinant == 0) {
            ++affinely_dependent_quadruplet_count;
            continue;
          }
          if (determinant < 0) {
            determinant = -determinant;
          }
          if (!minimum_absolute_affine_determinant.has_value() ||
              determinant < *minimum_absolute_affine_determinant) {
            minimum_absolute_affine_determinant = std::move(determinant);
          }
        }
      }
    }
  }
  check(
      affine_quadruplet_count ==
              fixture::
                  moment_curve_k3_one_step_star_incompleteness_n9_affine_quadruplet_count &&
          affinely_dependent_quadruplet_count == 0U &&
          minimum_absolute_affine_determinant ==
              BigInt{fixture::
                         moment_curve_k3_one_step_star_incompleteness_n9_minimum_absolute_affine_determinant},
      "all 126 affine quadruplets are nondegenerate and min |det| is exactly 120000");

  std::vector<CertifiedPoint3> points;
  points.reserve(
      fixture::moment_curve_k3_one_step_star_incompleteness_n9_points.size());
  for (const auto& coordinates :
       fixture::moment_curve_k3_one_step_star_incompleteness_n9_points) {
    points.push_back(point(
        static_cast<double>(coordinates[0U]),
        static_cast<double>(coordinates[1U]),
        static_cast<double>(coordinates[2U])));
  }
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const DirectSources source = direct_sources(cloud, 3U);
  const ExactDirectSupportTerminalBudget terminal_budget{
      unlimited_pair_budget(), unlimited_higher_budget()};
  const auto higher_verification = verify_exact_higher_support_stream(
      index,
      cloud,
      3U,
      terminal_budget.higher,
      source.higher_stream);
  const auto facade_verification =
      verify_exact_direct_support_terminal_facade(
          index,
          cloud,
          3U,
          terminal_budget,
          source.pair_stream,
          source.higher_stream,
          source.facade);
  check(
      source.higher_stream.stream_complete() &&
          higher_verification.result_certified &&
          source.facade.terminal_catalog_certified() &&
          facade_verification.result_certified &&
          source.facade.relevant_extra_shell_diagnostics.empty() &&
          source.higher_stream.relevant_extra_shell_diagnostics.empty() &&
          source.event_journal.no_relevant_extra_shell_diagnostics,
      std::string{fixture::
                      moment_curve_k3_one_step_star_incompleteness_n9_name} +
          " closes the exact direct catalog without relevant shell degeneracy");

  std::vector<fixture::MomentCurveSourceCoface> direct_families;
  bool direct_family_payloads_exact = true;
  for (const auto& family : source.incidence_journal.families) {
    if (family.order != 3U) {
      continue;
    }
    const auto& event = source.facade.events[family.source_event_index];
    std::vector<PointId> carrier_ids{
        event.support_ids.begin(),
        event.support_ids.begin() +
            static_cast<std::ptrdiff_t>(event.support_size)};
    carrier_ids.insert(
        carrier_ids.end(), event.interior_ids.begin(), event.interior_ids.end());
    std::sort(carrier_ids.begin(), carrier_ids.end());
    direct_family_payloads_exact =
        direct_family_payloads_exact && carrier_ids.size() == 4U &&
        event.closed_rank == 4U && event.saddle_order == 3U &&
        family.closed_facet_count == 4U;
    if (carrier_ids.size() != 4U) {
      continue;
    }
    fixture::MomentCurveSourceCoface carrier_sources{};
    for (std::size_t local = 0U; local < carrier_ids.size(); ++local) {
      carrier_sources[local] = cloud.source_index(carrier_ids[local]);
    }
    std::sort(carrier_sources.begin(), carrier_sources.end());
    direct_families.push_back(carrier_sources);
  }
  std::sort(direct_families.begin(), direct_families.end());
  const std::vector<fixture::MomentCurveSourceCoface>
      expected_direct_families{
          fixture::moment_curve_k3_one_step_star_incompleteness_n9_direct_families
              .begin(),
          fixture::moment_curve_k3_one_step_star_incompleteness_n9_direct_families
              .end()};
  check(
      direct_family_payloads_exact &&
          direct_families == expected_direct_families,
      "the certified K=3 catalog contains exactly 0123 through 5678");

  const auto star = build_and_verify(
      index,
      cloud,
      source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the n=9 moment-curve one-step direct star");
  std::set<fixture::MomentCurveSourceTriangle> direct_deletions;
  for (const auto& token : star.facet_tokens) {
    if (token.source_facet_key.point_count != 3U) {
      continue;
    }
    std::array<PointId, 3U> point_ids{
        token.source_facet_key.point_ids[0U],
        token.source_facet_key.point_ids[1U],
        token.source_facet_key.point_ids[2U]};
    direct_deletions.insert(source_indices_for_point_ids(cloud, point_ids));
  }
  const std::set<fixture::MomentCurveSourceTriangle>
      expected_direct_deletions{
          fixture::moment_curve_k3_one_step_star_incompleteness_n9_direct_deletions
              .begin(),
          fixture::moment_curve_k3_one_step_star_incompleteness_n9_direct_deletions
              .end()};
  check(
      direct_deletions == expected_direct_deletions,
      "the six direct K=3 families expose exactly the nineteen span-at-most-three triangular deletions");

  const auto target_key = facet_key_for_sources(
      cloud,
      fixture::
          moment_curve_k3_one_step_star_incompleteness_n9_target_facet);
  const auto target_ids = point_id_vector_for_sources(
      cloud,
      fixture::
          moment_curve_k3_one_step_star_incompleteness_n9_target_facet);
  const auto target_miniball = build_exact_facet_miniball(cloud, target_ids);
  const auto target_miniball_verification =
      verify_exact_facet_miniball(cloud, target_ids, target_miniball);
  const auto support_ids = point_id_vector_for_sources(
      cloud,
      fixture::
          moment_curve_k3_one_step_star_incompleteness_n9_essential_support);
  const ExactLevel target_level =
      level(fixture::
                moment_curve_k3_one_step_star_incompleteness_n9_squared_level);
  const auto global_ball = morsehgp3d::spatial::brute_force_closed_ball(
      cloud, target_miniball.center, target_miniball.squared_radius);
  check(
      target_miniball_verification.local_exact_facet_miniball_certified &&
          target_miniball.support_point_ids == support_ids &&
          target_miniball.strictly_inside_point_ids ==
              point_id_vector_for_sources(
                  cloud, std::array<std::size_t, 1U>{4U}) &&
          target_miniball.boundary_point_ids == support_ids &&
          target_miniball.squared_radius == target_level &&
          target_miniball.counters.optimal_support_count == 1U &&
          global_ball.partition_complete() &&
          global_ball.validated_for(cloud) &&
          source_indices_for_point_ids(cloud, global_ball.interior_ids()) ==
              std::vector<std::size_t>{1U, 2U, 3U, 4U, 5U, 6U, 7U} &&
          source_indices_for_point_ids(cloud, global_ball.shell_ids()) ==
              std::vector<std::size_t>{0U, 8U} &&
          global_ball.exterior_ids().empty(),
      "facet 048 has unique essential support 08, beta=1600066560 and a nondegenerate global 08 shell");

  const auto first_cursor = build_exact_direct_sparse_successive_incidence(
      index,
      cloud,
      target_key,
      std::nullopt,
      generous_successive_budget(),
      LbvhTraversalOrder::near_first);
  const auto first_cursor_verification =
      verify_exact_direct_sparse_successive_incidence(
          index,
          cloud,
          target_key,
          std::nullopt,
          generous_successive_budget(),
          LbvhTraversalOrder::near_first,
          first_cursor);
  std::vector<std::size_t> first_cursor_added_sources;
  bool first_cursor_supports_exact = true;
  for (const auto& minimizer : first_cursor.cominimizers) {
    first_cursor_added_sources.push_back(
        cloud.source_index(minimizer.added_point_id));
    const std::vector<PointId> minimizer_support{
        minimizer.support_point_ids.begin(),
        minimizer.support_point_ids.begin() +
            static_cast<std::ptrdiff_t>(minimizer.support_point_count)};
    first_cursor_supports_exact =
        first_cursor_supports_exact && minimizer_support == support_ids &&
        minimizer.squared_level == target_level &&
        minimizer.added_point_in_source_closed_ball &&
        !minimizer.added_point_in_selected_positive_support;
  }
  std::sort(
      first_cursor_added_sources.begin(), first_cursor_added_sources.end());
  check(
      first_cursor_verification.result_certified &&
          first_cursor.certified_complete_next_incidence() &&
          first_cursor.next_incidence_squared_level == target_level &&
          first_cursor.audit.eligible_coface_point_count == 6U &&
          first_cursor.audit.inside_or_boundary_source_ball_point_count ==
              6U &&
          first_cursor.audit.outside_source_ball_point_count == 0U &&
          first_cursor_supports_exact &&
          first_cursor_added_sources ==
              std::vector<std::size_t>{1U, 2U, 3U, 5U, 6U, 7U},
      "the exact supplied-facet cursor certifies all six equal-beta silent cofaces of 048");

  std::set<fixture::MomentCurveSourceCoface> silent_cofaces;
  bool every_silent_coface_certified = true;
  bool every_silent_deletion_absent = true;
  for (const std::size_t added_source :
       fixture::
           moment_curve_k3_one_step_star_incompleteness_n9_silent_added_points) {
    fixture::MomentCurveSourceCoface coface_sources{
        0U, 4U, 8U, added_source};
    std::sort(coface_sources.begin(), coface_sources.end());
    const auto coface_ids = point_id_vector_for_sources(cloud, coface_sources);
    const auto coface_miniball =
        build_exact_facet_miniball(cloud, coface_ids);
    const auto coface_verification =
        verify_exact_facet_miniball(cloud, coface_ids, coface_miniball);
    std::array<std::size_t, 2U> expected_interior{4U, added_source};
    std::sort(expected_interior.begin(), expected_interior.end());
    every_silent_coface_certified =
        every_silent_coface_certified &&
        coface_verification.local_exact_facet_miniball_certified &&
        coface_miniball.support_point_ids == support_ids &&
        coface_miniball.strictly_inside_point_ids ==
            point_id_vector_for_sources(cloud, expected_interior) &&
        coface_miniball.boundary_point_ids == support_ids &&
        coface_miniball.squared_radius == target_level &&
        coface_miniball.counters.optimal_support_count == 1U;
    for (std::size_t removed = 0U; removed < coface_sources.size(); ++removed) {
      fixture::MomentCurveSourceTriangle deletion{};
      std::size_t write = 0U;
      for (std::size_t local = 0U; local < coface_sources.size(); ++local) {
        if (local != removed) {
          deletion[write++] = coface_sources[local];
        }
      }
      every_silent_deletion_absent =
          every_silent_deletion_absent &&
          !direct_deletions.contains(deletion);
    }
    silent_cofaces.insert(coface_sources);
  }

  std::size_t visible_silent_coface_count = 0U;
  for (const auto& coface : star.cofaces) {
    const auto coface_ids = reconstructed_ids(star, coface.coface_index);
    if (coface_ids.size() != 4U) {
      continue;
    }
    fixture::MomentCurveSourceCoface coface_sources{};
    for (std::size_t local = 0U; local < coface_ids.size(); ++local) {
      coface_sources[local] = cloud.source_index(coface_ids[local]);
    }
    std::sort(coface_sources.begin(), coface_sources.end());
    visible_silent_coface_count += silent_cofaces.contains(coface_sources);
  }
  const bool target_facet_visible = std::any_of(
      star.facet_tokens.begin(),
      star.facet_tokens.end(),
      [&target_key](const auto& token) {
        return token.source_facet_key == target_key;
      });
  check(
      every_silent_coface_certified && silent_cofaces.size() == 6U &&
          every_silent_deletion_absent && !target_facet_visible &&
          visible_silent_coface_count == 0U &&
          !star.product_sparse_silent_source_complete &&
          !star.global_gamma_completeness_claimed,
      "the six exact non-Gabriel cofaces 048x are silent and all remain invisible to the supplied one-step direct star");
}

void test_permutation_and_empty_source() {
  std::array<CertifiedPoint3, 5U> points{
      point(-2.0, -1.0),
      point(-2.0, 1.0),
      point(0.0, 0.0),
      point(3.0, 2.0),
      point(4.0, -1.0),
  };
  const CanonicalPointCloud original_cloud = canonical_cloud(points);
  std::reverse(points.begin(), points.end());
  const CanonicalPointCloud permuted_cloud = canonical_cloud(points);
  const MortonLbvhIndex original_index =
      MortonLbvhIndex::build(original_cloud);
  const MortonLbvhIndex permuted_index =
      MortonLbvhIndex::build(permuted_cloud);
  const DirectSources original_source = direct_sources(original_cloud, 2U);
  const DirectSources permuted_source = direct_sources(permuted_cloud, 2U);
  const auto original = build_and_verify(
      original_index,
      original_cloud,
      original_source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the original E5 input order");
  const auto permuted = build_and_verify(
      permuted_index,
      permuted_cloud,
      permuted_source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the reversed E5 input order");
  check(
      original == permuted,
      "reversing the raw E5 point order preserves the complete canonical journal");

  const std::array<CertifiedPoint3, 1U> singleton_points{point(0.0, 0.0)};
  const CanonicalPointCloud singleton_cloud =
      canonical_cloud(singleton_points);
  const MortonLbvhIndex singleton_index =
      MortonLbvhIndex::build(singleton_cloud);
  const DirectSources singleton_source = direct_sources(singleton_cloud, 1U);
  const auto empty = build_and_verify(
      singleton_index,
      singleton_cloud,
      singleton_source,
      ExactDirectSparseSuccessiveIncidenceStarJournalBudget{},
      LbvhTraversalOrder::near_first,
      "the empty direct-saddle star");
  check(
      empty.certified_bounded_star() && scientific_arenas_empty(empty) &&
          empty.required_successive_incidence_call_count == 0U &&
          empty.aggregate_successive_work_entry_count == 0U,
      "an empty certified saddle source completes without a geometric query");
}

void test_exact_caps_fail_atomically() {
  const std::array<CertifiedPoint3, 5U> points{
      point(-2.0, -1.0),
      point(-2.0, 1.0),
      point(0.0, 0.0),
      point(3.0, 2.0),
      point(4.0, -1.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const DirectSources source = direct_sources(cloud, 2U);
  const auto baseline = build_and_verify(
      index,
      cloud,
      source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the E5 exact-cap baseline");
  const auto exact = exact_budget_for(baseline);
  const auto exact_result = build_and_verify(
      index,
      cloud,
      source,
      exact,
      LbvhTraversalOrder::near_first,
      "the componentwise exact E5 caps");
  check(
      scientific_cofaces(exact_result) == scientific_cofaces(baseline),
      "the exact observed caps reproduce the complete E5 star");

  std::vector<ExactDirectSparseSuccessiveIncidenceStarJournalBudget> short_caps;
  const auto push_short = [&short_caps](
                              const auto& base,
                              auto member) {
    auto candidate = base;
    std::size_t& cap = candidate.*member;
    if (cap != 0U) {
      --cap;
      short_caps.push_back(std::move(candidate));
    }
  };
  push_short(exact, &ExactDirectSparseSuccessiveIncidenceStarJournalBudget::
                        maximum_source_family_scan_count);
  push_short(exact, &ExactDirectSparseSuccessiveIncidenceStarJournalBudget::
                        maximum_deletion_reference_count);
  push_short(exact, &ExactDirectSparseSuccessiveIncidenceStarJournalBudget::
                        maximum_distinct_facet_count);
  push_short(exact, &ExactDirectSparseSuccessiveIncidenceStarJournalBudget::
                        maximum_facet_key_point_count);
  push_short(exact, &ExactDirectSparseSuccessiveIncidenceStarJournalBudget::
                        maximum_successive_incidence_call_count);
  push_short(exact, &ExactDirectSparseSuccessiveIncidenceStarJournalBudget::
                        maximum_incidence_shell_count);
  push_short(exact, &ExactDirectSparseSuccessiveIncidenceStarJournalBudget::
                        maximum_coface_occurrence_count);
  push_short(exact, &ExactDirectSparseSuccessiveIncidenceStarJournalBudget::
                        maximum_distinct_coface_count);
  push_short(exact, &ExactDirectSparseSuccessiveIncidenceStarJournalBudget::
                        maximum_direct_coface_count);
  push_short(exact, &ExactDirectSparseSuccessiveIncidenceStarJournalBudget::
                        maximum_residual_coface_count);
  push_short(exact, &ExactDirectSparseSuccessiveIncidenceStarJournalBudget::
                        maximum_residual_batch_count);
  push_short(exact, &ExactDirectSparseSuccessiveIncidenceStarJournalBudget::
                        maximum_residual_batch_reference_count);
  push_short(exact, &ExactDirectSparseSuccessiveIncidenceStarJournalBudget::
                        maximum_logical_storage_entry_count);
  bool every_global_cap_fails_atomically = !short_caps.empty();
  for (const auto& short_budget : short_caps) {
    const auto rejected =
        build_exact_direct_sparse_successive_incidence_star_journal(
            index,
            cloud,
            source.facade,
            source.event_journal,
            source.arm_budget,
            source.arm_journal,
            source.incidence_budget,
            source.incidence_journal,
            short_budget,
            LbvhTraversalOrder::near_first);
    every_global_cap_fails_atomically =
        every_global_cap_fails_atomically &&
        !rejected.certified_bounded_star() &&
        scientific_arenas_empty(rejected) &&
        rejected.no_partial_scientific_payload_published;
  }
  check(
      every_global_cap_fails_atomically,
      "every one-short global cap clears all five scientific arenas atomically");

  std::vector<ExactDirectSparseSuccessiveIncidenceBudget> short_children;
  const auto push_short_child = [&short_children](
                                    const auto& base,
                                    auto member) {
    auto candidate = base;
    std::size_t& cap = candidate.*member;
    if (cap != 0U) {
      --cap;
      short_children.push_back(std::move(candidate));
    }
  };
  push_short_child(
      exact.successive_incidence_budget,
      &ExactDirectSparseSuccessiveIncidenceBudget::
          maximum_source_support_enumeration_count);
  push_short_child(
      exact.successive_incidence_budget,
      &ExactDirectSparseSuccessiveIncidenceBudget::maximum_node_visit_count);
  push_short_child(
      exact.successive_incidence_budget,
      &ExactDirectSparseSuccessiveIncidenceBudget::
          maximum_internal_node_expansion_count);
  push_short_child(
      exact.successive_incidence_budget,
      &ExactDirectSparseSuccessiveIncidenceBudget::
          maximum_exact_aabb_bound_evaluation_count);
  push_short_child(
      exact.successive_incidence_budget,
      &ExactDirectSparseSuccessiveIncidenceBudget::
          maximum_exact_point_evaluation_count);
  push_short_child(
      exact.successive_incidence_budget,
      &ExactDirectSparseSuccessiveIncidenceBudget::
          maximum_coface_support_enumeration_count);
  push_short_child(
      exact.successive_incidence_budget,
      &ExactDirectSparseSuccessiveIncidenceBudget::
          maximum_candidate_point_classification_count);
  push_short_child(
      exact.successive_incidence_budget,
      &ExactDirectSparseSuccessiveIncidenceBudget::
          maximum_frontier_entry_count);
  push_short_child(
      exact.successive_incidence_budget,
      &ExactDirectSparseSuccessiveIncidenceBudget::
          maximum_cominimizer_count);
  bool every_child_cap_fails_atomically = !short_children.empty();
  for (const auto& short_child : short_children) {
    auto short_budget = exact;
    short_budget.successive_incidence_budget = short_child;
    const auto rejected =
        build_exact_direct_sparse_successive_incidence_star_journal(
            index,
            cloud,
            source.facade,
            source.event_journal,
            source.arm_budget,
            source.arm_journal,
            source.incidence_budget,
            source.incidence_journal,
            short_budget,
            LbvhTraversalOrder::near_first);
    every_child_cap_fails_atomically =
        every_child_cap_fails_atomically &&
        rejected.decision ==
            ExactDirectSparseSuccessiveIncidenceStarJournalDecision::
                no_star_successive_incidence_budget_exhausted &&
        scientific_arenas_empty(rejected);
  }
  check(
      every_child_cap_fails_atomically,
      "every one-short nested cursor cap aborts the complete star atomically");
}

void test_mutations_and_forged_source_fail_fresh_verification() {
  const std::array<CertifiedPoint3, 5U> points{
      point(-2.0, -1.0),
      point(-2.0, 1.0),
      point(0.0, 0.0),
      point(3.0, 2.0),
      point(4.0, -1.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const DirectSources source = direct_sources(cloud, 2U);
  const auto budget = generous_budget();
  const auto baseline = build_and_verify(
      index,
      cloud,
      source,
      budget,
      LbvhTraversalOrder::near_first,
      "the E5 mutation baseline");

  std::vector<ExactDirectSparseSuccessiveIncidenceStarJournalResult> forged;
  auto wrong_enum = baseline;
  wrong_enum.cofaces.front().kind =
      ExactDirectSparseSuccessiveIncidenceStarCofaceKind::residual;
  forged.push_back(std::move(wrong_enum));
  auto wrong_owner = baseline;
  wrong_owner.cofaces.front().owner_facet_token_index =
      (wrong_owner.cofaces.front().owner_facet_token_index + 1U) %
      wrong_owner.facet_tokens.size();
  forged.push_back(std::move(wrong_owner));
  auto wrong_added = baseline;
  wrong_added.cofaces.front().added_point_id =
      static_cast<PointId>(
          (wrong_added.cofaces.front().added_point_id + 1U) % cloud.size());
  forged.push_back(std::move(wrong_added));
  auto wrong_support = baseline;
  wrong_support.cofaces.front().positive_support_point_ids[0U] =
      static_cast<PointId>(
          (wrong_support.cofaces.front().positive_support_point_ids[0U] +
           1U) %
          cloud.size());
  forged.push_back(std::move(wrong_support));
  auto wrong_level = baseline;
  wrong_level.cofaces.front().squared_level = level(999);
  forged.push_back(std::move(wrong_level));
  auto wrong_batch = baseline;
  wrong_batch.residual_batches.front().squared_level = level(999);
  forged.push_back(std::move(wrong_batch));
  auto wrong_fact = baseline;
  wrong_fact.public_status_claimed = true;
  forged.push_back(std::move(wrong_fact));

  bool every_mutation_rejected = !forged.empty();
  for (const auto& candidate : forged) {
    const auto verification =
        verify_exact_direct_sparse_successive_incidence_star_journal(
            index,
            cloud,
            source.facade,
            source.event_journal,
            source.arm_budget,
            source.arm_journal,
            source.incidence_budget,
            source.incidence_journal,
            budget,
            LbvhTraversalOrder::near_first,
            candidate);
    every_mutation_rejected =
        every_mutation_rejected && !verification.result_certified;
  }
  check(
      every_mutation_rejected,
      "fresh verification rejects owner, factorization, support, level, enum, batch and public-status mutations");

  auto forged_source = source;
  forged_source.incidence_journal.families.front().critical_squared_level =
      level(999);
  const auto rejected =
      build_exact_direct_sparse_successive_incidence_star_journal(
          index,
          cloud,
          forged_source.facade,
          forged_source.event_journal,
          forged_source.arm_budget,
          forged_source.arm_journal,
          forged_source.incidence_budget,
          forged_source.incidence_journal,
          budget,
          LbvhTraversalOrder::near_first);
  check(
      rejected.decision ==
              ExactDirectSparseSuccessiveIncidenceStarJournalDecision::
                  no_star_source_not_certified &&
          scientific_arenas_empty(rejected),
      "a forged source incidence journal is freshly rejected before geometry");
}

}  // namespace

int main() {
  test_e5_residual_shells_and_canonical_owners();
  test_moment_curve_k3_one_step_star_incompleteness_n9();
  test_permutation_and_empty_source();
  test_exact_caps_fail_atomically();
  test_mutations_and_forged_source_fail_fresh_verification();
  if (failures != 0) {
    std::cerr << failures
              << " direct sparse successive-incidence star test(s) failed\n";
    return 1;
  }
  std::cout <<
      "all direct sparse successive-incidence star tests passed\n";
  return 0;
}
