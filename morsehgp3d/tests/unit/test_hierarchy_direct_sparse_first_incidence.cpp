#include "morsehgp3d/hierarchy/direct_sparse_first_incidence.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactCenter3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Function>
[[nodiscard]] bool throws_invalid_argument(Function&& function) {
  try {
    std::forward<Function>(function)();
  } catch (const std::invalid_argument&) {
    return true;
  }
  return false;
}

[[nodiscard]] CertifiedPoint3 point(
    double x,
    double y = 0.0,
    double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactLevel level(
    std::int64_t numerator,
    std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

[[nodiscard]] ExactCenter3 center(
    std::int64_t x,
    std::int64_t y,
    std::int64_t z,
    std::int64_t denominator = 1) {
  return ExactCenter3{
      BigInt{x}, BigInt{y}, BigInt{z}, BigInt{denominator}};
}

[[nodiscard]] PointId point_id_for_source(
    const CanonicalPointCloud& cloud,
    std::size_t source_index) {
  for (std::size_t point_index = 0U;
       point_index < cloud.size();
       ++point_index) {
    const PointId point_id = static_cast<PointId>(point_index);
    if (cloud.source_index(point_id) == source_index) {
      return point_id;
    }
  }
  throw std::logic_error("a source point is absent from its canonical cloud");
}

template <std::size_t Size>
[[nodiscard]] ExactDirectSparseFacetKey key_from_sources(
    const CanonicalPointCloud& cloud,
    const std::array<std::size_t, Size>& source_indices) {
  static_assert(Size >= 1U);
  static_assert(Size <= direct_sparse_first_incidence_maximum_source_point_count);
  std::array<PointId, Size> point_ids{};
  for (std::size_t index = 0U; index < Size; ++index) {
    point_ids[index] = point_id_for_source(cloud, source_indices[index]);
  }
  std::sort(point_ids.begin(), point_ids.end());
  ExactDirectSparseFacetKey result;
  result.point_count = Size;
  std::copy(point_ids.begin(), point_ids.end(), result.point_ids.begin());
  return result;
}

[[nodiscard]] std::vector<PointId> used_support(
    const ExactDirectSparseFirstIncidenceMinimizer& minimizer) {
  return std::vector<PointId>{
      minimizer.support_point_ids.begin(),
      minimizer.support_point_ids.begin() +
          static_cast<std::ptrdiff_t>(minimizer.support_point_count)};
}

[[nodiscard]] ExactDirectSparseFirstIncidenceBudget generous_budget() {
  return {
      1024U,
      4096U,
      4096U,
      65536U,
      4096U,
      65536U,
      262144U,
      4096U,
      64U,
  };
}

[[nodiscard]] bool fresh_verification_closes(
    const ExactDirectSparseFirstIncidenceVerification& verification) {
  return verification.trusted_inputs_certified &&
         verification.observed_storage_within_budget &&
         verification.source_miniball_freshly_replayed &&
         verification.branch_and_bound_freshly_replayed &&
         verification.all_cominimizers_freshly_replayed &&
         verification.counters_and_decision_freshly_replayed &&
         verification.no_forbidden_global_structure_materialized &&
         verification.fresh_replay_certified &&
         verification.result_certified;
}

[[nodiscard]] ExactDirectSparseFirstIncidenceResult build_and_verify(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const ExactDirectSparseFacetKey& source_key,
    const ExactDirectSparseFirstIncidenceBudget& budget,
    LbvhTraversalOrder traversal_order,
    const std::string& context) {
  const ExactDirectSparseFirstIncidenceResult result =
      build_exact_direct_sparse_first_incidence(
          index, cloud, source_key, budget, traversal_order);
  const ExactDirectSparseFirstIncidenceVerification verification =
      verify_exact_direct_sparse_first_incidence(
          index, cloud, source_key, budget, traversal_order, result);
  check(
      fresh_verification_closes(verification),
      context + " closes under a fresh exact replay");
  return result;
}

[[nodiscard]] ExactDirectSparseFirstIncidenceBudget exact_budget_for(
    const ExactDirectSparseFirstIncidenceResult& result) {
  return {
      result.audit.source_support_enumeration_count,
      result.audit.node_visit_count,
      result.audit.internal_node_expansion_count,
      result.audit.exact_aabb_bound_evaluation_count,
      result.audit.exact_point_evaluation_count,
      result.audit.coface_support_enumeration_count,
      result.audit.candidate_point_classification_count,
      result.audit.peak_frontier_entry_count,
      result.audit.peak_cominimizer_entry_count,
  };
}

struct ExhaustiveFirstIncidence {
  ExactLevel squared_level;
  std::vector<PointId> added_point_ids;
};

[[nodiscard]] ExhaustiveFirstIncidence exhaustive_all_one_point_cofaces(
    const CanonicalPointCloud& cloud,
    const ExactDirectSparseFacetKey& source) {
  const auto source_begin = source.point_ids.begin();
  const auto source_end = source_begin +
      static_cast<std::ptrdiff_t>(source.point_count);
  std::optional<ExactLevel> incumbent;
  std::vector<PointId> minimizers;
  for (std::size_t point_index = 0U;
       point_index < cloud.size();
       ++point_index) {
    const PointId added_point_id = static_cast<PointId>(point_index);
    if (std::binary_search(source_begin, source_end, added_point_id)) {
      continue;
    }
    std::vector<PointId> coface{source_begin, source_end};
    coface.push_back(added_point_id);
    std::sort(coface.begin(), coface.end());
    const ExactFacetMiniballResult miniball =
        build_exact_facet_miniball(cloud, coface);
    if (!incumbent.has_value() ||
        miniball.squared_radius < *incumbent) {
      incumbent = miniball.squared_radius;
      minimizers.assign(1U, added_point_id);
    } else if (miniball.squared_radius == *incumbent) {
      minimizers.push_back(added_point_id);
    }
  }
  if (!incumbent.has_value()) {
    throw std::logic_error(
        "an exhaustive first-incidence fixture has no eligible coface");
  }
  std::sort(minimizers.begin(), minimizers.end());
  return {*incumbent, std::move(minimizers)};
}

void test_no_coface_when_n_equals_k() {
  const std::array<CertifiedPoint3, 3U> input{
      point(-1.0), point(0.0), point(2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSparseFacetKey source =
      key_from_sources(cloud, std::array<std::size_t, 3U>{0U, 1U, 2U});
  const auto result = build_and_verify(
      index,
      cloud,
      source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the terminal n=k facet");

  check(
      result.certified_complete_no_coface() &&
          result.audit.eligible_coface_point_count == 0U &&
          result.audit.traversal_complete,
      "n=k returns the certified no-coface decision");
  check(
      result.audit.source_support_enumeration_count == 14U &&
          result.source_facet_miniball.has_value() &&
          result.source_facet_miniball_freshly_certified,
      "n=k freshly certifies its three-point source miniball in two seven-support passes");
  check(
      result.audit.node_visit_count == 0U &&
          !result.first_incidence_squared_level.has_value() &&
          result.cominimizers.empty(),
      "n=k returns +infinity semantically without any LBVH traversal");
}

void test_invalid_authorities_are_rejected_before_traversal() {
  const std::array<CertifiedPoint3, 3U> input{
      point(0.0), point(1.0), point(2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSparseFacetKey source =
      key_from_sources(cloud, std::array<std::size_t, 1U>{0U});
  const ExactDirectSparseFirstIncidenceBudget budget = generous_budget();

  ExactDirectSparseFacetKey duplicate;
  duplicate.point_count = 2U;
  duplicate.point_ids[0U] = 0U;
  duplicate.point_ids[1U] = 0U;
  ExactDirectSparseFacetKey dirty_unused = source;
  dirty_unused.point_ids[5U] = 1U;
  const CanonicalPointCloud foreign_cloud = canonical_cloud(
      std::array<CertifiedPoint3, 3U>{
          point(0.0), point(1.0), point(3.0)});

  check(
      throws_invalid_argument([&] {
        static_cast<void>(build_exact_direct_sparse_first_incidence(
            index,
            cloud,
            source,
            budget,
            static_cast<LbvhTraversalOrder>(UINT8_C(255))));
      }) &&
          throws_invalid_argument([&] {
            static_cast<void>(build_exact_direct_sparse_first_incidence(
                index,
                cloud,
                duplicate,
                budget,
                LbvhTraversalOrder::near_first));
          }) &&
          throws_invalid_argument([&] {
            static_cast<void>(build_exact_direct_sparse_first_incidence(
                index,
                cloud,
                dirty_unused,
                budget,
                LbvhTraversalOrder::near_first));
          }) &&
          throws_invalid_argument([&] {
            static_cast<void>(build_exact_direct_sparse_first_incidence(
                index,
                foreign_cloud,
                source,
                budget,
                LbvhTraversalOrder::near_first));
          }),
      "invalid traversal, malformed keys and a foreign LBVH are rejected before any scientific payload is returned");
}

void test_unique_minimizer_and_exact_positive_support() {
  const std::array<CertifiedPoint3, 3U> input{
      point(0.0), point(2.0), point(10.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const PointId source_id = point_id_for_source(cloud, 0U);
  const PointId minimizer_id = point_id_for_source(cloud, 1U);
  const ExactDirectSparseFacetKey source =
      key_from_sources(cloud, std::array<std::size_t, 1U>{0U});
  const auto result = build_and_verify(
      index,
      cloud,
      source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the unique one-dimensional first incidence");

  const std::vector<PointId> expected_support = [&] {
    std::vector<PointId> ids{source_id, minimizer_id};
    std::sort(ids.begin(), ids.end());
    return ids;
  }();
  check(
      result.certified_complete_first_incidence() &&
          result.first_incidence_squared_level == level(1) &&
          result.cominimizers.size() == 1U &&
          result.cominimizers[0U].added_point_id == minimizer_id &&
          result.cominimizers[0U].support_point_count == 2U &&
          used_support(result.cominimizers[0U]) == expected_support &&
          result.cominimizers[0U].center == center(1, 0, 0) &&
          result.cominimizers[0U].squared_level == level(1) &&
          !result.cominimizers[0U].added_point_in_source_closed_ball &&
          result.cominimizers[0U].added_point_in_selected_positive_support &&
          result.audit.eligible_coface_point_count == 2U &&
          result.audit.incumbent_improvement_count != 0U,
      "the point at 2 is the unique exact minimizer and its diameter support contains the added point");
}

void test_inside_boundary_and_selected_support_semantics() {
  const std::array<CertifiedPoint3, 6U> input{
      point(-1.0, 0.0),
      point(1.0, 0.0),
      point(0.0, -1.0),
      point(0.0, 1.0),
      point(0.0, 0.0),
      point(0.0, 4.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSparseFacetKey source =
      key_from_sources(cloud, std::array<std::size_t, 3U>{0U, 1U, 2U});
  const PointId boundary = point_id_for_source(cloud, 3U);
  const PointId interior = point_id_for_source(cloud, 4U);
  const auto result = build_and_verify(
      index,
      cloud,
      source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the inside-boundary selected-support distinction");

  std::vector<PointId> expected{boundary, interior};
  std::sort(expected.begin(), expected.end());
  std::vector<PointId> observed;
  for (const auto& minimizer : result.cominimizers) {
    observed.push_back(minimizer.added_point_id);
  }
  check(
      result.certified_complete_first_incidence() &&
          result.first_incidence_squared_level == level(1) &&
          observed == expected && result.source_facet_miniball.has_value() &&
          result.audit.inside_or_boundary_source_ball_point_count == 2U &&
          std::all_of(
              result.cominimizers.begin(),
              result.cominimizers.end(),
              [&result](
                  const ExactDirectSparseFirstIncidenceMinimizer& minimizer) {
                return minimizer.added_point_in_source_closed_ball &&
                       !minimizer
                            .added_point_in_selected_positive_support &&
                       std::equal(
                           minimizer.support_point_ids.begin(),
                           minimizer.support_point_ids.begin() +
                               static_cast<std::ptrdiff_t>(
                                   minimizer.support_point_count),
                           result.source_facet_miniball
                               ->support_point_ids.begin(),
                           result.source_facet_miniball
                               ->support_point_ids.end());
              }),
      "inside and boundary ties reuse the source-stable witness even when the boundary coface admits another positive support");
}

struct EqualityFixture {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  ExactDirectSparseFacetKey source;
  PointId negative_minimizer{};
  PointId positive_minimizer{};

  EqualityFixture()
      : cloud(canonical_cloud(std::array<CertifiedPoint3, 4U>{
            point(-2.0), point(0.0), point(2.0), point(100.0)})),
        index(MortonLbvhIndex::build(cloud)),
        source(key_from_sources(
            cloud, std::array<std::size_t, 1U>{1U})),
        negative_minimizer(point_id_for_source(cloud, 0U)),
        positive_minimizer(point_id_for_source(cloud, 2U)) {}
};

void test_equal_cominimizers_strict_pruning_and_traversal_invariance() {
  EqualityFixture fixture;
  const auto near = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the near-first equal-minimizer traversal");
  const auto far = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.source,
      generous_budget(),
      LbvhTraversalOrder::far_first,
      "the far-first equal-minimizer traversal");

  std::vector<PointId> expected{
      fixture.negative_minimizer, fixture.positive_minimizer};
  std::sort(expected.begin(), expected.end());
  std::vector<PointId> observed;
  for (const auto& minimizer : near.cominimizers) {
    observed.push_back(minimizer.added_point_id);
  }
  check(
      near.certified_complete_first_incidence() &&
          near.first_incidence_squared_level == level(1) &&
          observed == expected && near.cominimizers.size() == 2U &&
          std::all_of(
              near.cominimizers.begin(),
              near.cominimizers.end(),
              [](const ExactDirectSparseFirstIncidenceMinimizer& minimizer) {
                return minimizer.squared_level == level(1) &&
                       !minimizer.added_point_in_source_closed_ball &&
                       minimizer.added_point_in_selected_positive_support;
              }) &&
          near.audit.eligible_coface_point_count == 3U &&
          near.audit.exact_point_evaluation_count == 2U &&
          near.audit.equal_incumbent_observation_count == 1U &&
          near.audit.pruned_node_count != 0U &&
          near.equality_bounds_always_descended,
      "both level-one leaves survive equality while the distant leaf is pruned only strictly");
  check(
      far.first_incidence_squared_level == near.first_incidence_squared_level &&
          far.source_facet_miniball == near.source_facet_miniball &&
          far.cominimizers == near.cominimizers &&
          far.decision == near.decision &&
          far.every_nonexcluded_point_evaluated_or_strictly_pruned &&
          far.equality_bounds_always_descended,
      "near-first and far-first preserve the exact level and complete canonical co-minimizer set");
}

void test_new_support_uses_a_point_outside_the_old_support() {
  const std::array<CertifiedPoint3, 4U> input{
      point(-1.0, 0.0),
      point(1.0, 0.0),
      point(0.0, -0.75),
      point(0.0, 2.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSparseFacetKey source =
      key_from_sources(cloud, std::array<std::size_t, 3U>{0U, 1U, 2U});
  const PointId left = point_id_for_source(cloud, 0U);
  const PointId right = point_id_for_source(cloud, 1U);
  const PointId old_interior = point_id_for_source(cloud, 2U);
  const PointId added = point_id_for_source(cloud, 3U);
  const auto result = build_and_verify(
      index,
      cloud,
      source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the old-support exclusion counterexample");

  std::vector<PointId> expected_old_support{left, right};
  std::vector<PointId> expected_new_support{old_interior, added};
  std::sort(expected_old_support.begin(), expected_old_support.end());
  std::sort(expected_new_support.begin(), expected_new_support.end());
  check(
      result.certified_complete_first_incidence() &&
          result.source_facet_miniball.has_value() &&
          result.source_facet_miniball->center == center(0, 0, 0) &&
          result.source_facet_miniball->squared_radius == level(1) &&
          result.source_facet_miniball->support_point_ids ==
              expected_old_support &&
          result.first_incidence_squared_level == level(121, 64) &&
          result.cominimizers.size() == 1U &&
          result.cominimizers[0U].added_point_id == added &&
          result.cominimizers[0U].support_point_count == 2U &&
          used_support(result.cominimizers[0U]) == expected_new_support &&
          result.cominimizers[0U].center == center(0, 5, 0, 8) &&
          result.cominimizers[0U].squared_level == level(121, 64) &&
          result.cominimizers[0U].added_point_in_selected_positive_support &&
          result.audit.coface_support_enumeration_count == 8U,
      "the new diameter uses the old interior point, proving that all of F rather than only its old support is enumerated");
}

void test_k10_avoids_an_eleven_point_key_and_uses_176_supports() {
  const std::array<CertifiedPoint3, 11U> input{
      point(0.0),
      point(1.0),
      point(2.0),
      point(3.0),
      point(4.0),
      point(5.0),
      point(6.0),
      point(7.0),
      point(8.0),
      point(9.0),
      point(10.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSparseFacetKey source = key_from_sources(
      cloud,
      std::array<std::size_t, 10U>{
          0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U});
  const PointId left = point_id_for_source(cloud, 0U);
  const PointId added = point_id_for_source(cloud, 10U);
  const auto result = build_and_verify(
      index,
      cloud,
      source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the K=10 reduced outside-coface enumeration");

  std::vector<PointId> expected_support{left, added};
  std::sort(expected_support.begin(), expected_support.end());
  check(
      direct_sparse_first_incidence_maximum_outside_coface_support_count ==
              176U &&
          direct_sparse_first_incidence_maximum_outside_coface_classification_count ==
              1936U &&
          direct_sparse_first_incidence_maximum_source_support_enumeration_count ==
              770U &&
          direct_sparse_first_incidence_source_support_enumeration_count_per_pass ==
              385U,
      "the public K=10 bounded contract exposes 385, 770, 176 and 1936 as its exact caps");
  check(
      result.certified_complete_first_incidence() &&
          result.source_facet_key.point_count == 10U &&
          result.audit.eligible_coface_point_count == 1U &&
          result.audit.source_support_enumeration_count == 770U &&
          result.audit.coface_support_enumeration_count == 176U &&
          result.audit.candidate_point_classification_count != 0U &&
          result.audit.candidate_point_classification_count <= 1936U &&
          result.audit.exact_point_evaluation_count == 1U,
      "K=10 uses two 385-support source passes and exactly 176 x-containing candidates");
  check(
      result.first_incidence_squared_level == level(25) &&
          result.cominimizers.size() == 1U &&
          result.cominimizers[0U].added_point_id == added &&
          result.cominimizers[0U].support_point_count == 2U &&
          used_support(result.cominimizers[0U]) == expected_support &&
          result.cominimizers[0U].center == center(5, 0, 0) &&
          result.cominimizers[0U].squared_level == level(25),
      "the logical eleven-point coface returns the exact endpoint diameter without a persistent key");
}

void test_bounded_n14_differential_against_all_explicit_cofaces() {
  const std::array<CertifiedPoint3, 14U> input{
      point(0.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(0.0, -1.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(0.0, 0.0, -1.0),
      point(2.0, 2.0, 0.0),
      point(-2.0, 1.0, 1.0),
      point(3.0, -1.0, 0.0),
      point(-3.0, -1.0, 2.0),
      point(1.0, 3.0, -2.0),
      point(-2.0, -3.0, -1.0),
      point(4.0, 0.0, 1.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSparseFacetKey source =
      key_from_sources(cloud, std::array<std::size_t, 3U>{0U, 1U, 3U});
  const ExhaustiveFirstIncidence expected =
      exhaustive_all_one_point_cofaces(cloud, source);

  for (const LbvhTraversalOrder order : {
           LbvhTraversalOrder::near_first,
           LbvhTraversalOrder::far_first}) {
    const auto result = build_and_verify(
        index,
        cloud,
        source,
        generous_budget(),
        order,
        "the bounded n=14 all-cofaces differential");
    std::vector<PointId> observed;
    for (const auto& minimizer : result.cominimizers) {
      observed.push_back(minimizer.added_point_id);
    }
    check(
        result.certified_complete_first_incidence() &&
            result.first_incidence_squared_level == expected.squared_level &&
            observed == expected.added_point_ids,
        "the sparse branch-and-bound agrees with an explicit bounded oracle over every one-point coface");
  }
}

void test_ac_silent_equal_first_incidence() {
  const std::array<CertifiedPoint3, 5U> input{
      point(0.0, 0.0, 7.0),
      point(0.0, 9.0, 6.0),
      point(1.0, 4.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(4.0, 1.0, 2.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  ExactDirectSparseFacetKey source;
  source.point_count = 2U;
  source.point_ids[0U] = 1U;
  source.point_ids[1U] = 3U;
  const std::vector<PointId> expected{0U, 4U};
  const auto result = build_and_verify(
      index,
      cloud,
      source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the permanent silent AC first-incidence fixture");

  std::vector<PointId> observed;
  for (const auto& minimizer : result.cominimizers) {
    observed.push_back(minimizer.added_point_id);
  }
  check(
      result.certified_complete_first_incidence() &&
          result.first_incidence_squared_level == level(33, 2) &&
          observed == expected && result.cominimizers.size() == 2U &&
          std::all_of(
              result.cominimizers.begin(),
              result.cominimizers.end(),
              [](const ExactDirectSparseFirstIncidenceMinimizer& minimizer) {
                return minimizer.added_point_in_source_closed_ball &&
                       !minimizer.added_point_in_selected_positive_support;
              }),
      "AC retains exactly D and E at 33/2 instead of losing its silent equal incidences");
}

void test_source_boundary_point_is_an_equal_minimizer() {
  const std::array<CertifiedPoint3, 4U> input{
      point(-1.0, 0.0),
      point(1.0, 0.0),
      point(0.0, 1.0),
      point(10.0, 0.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSparseFacetKey source =
      key_from_sources(cloud, std::array<std::size_t, 2U>{0U, 1U});
  const PointId boundary = point_id_for_source(cloud, 2U);
  const auto result = build_and_verify(
      index,
      cloud,
      source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the exact source-sphere boundary fixture");

  check(
      result.certified_complete_first_incidence() &&
          result.first_incidence_squared_level == level(1) &&
          result.cominimizers.size() == 1U &&
          result.cominimizers[0U].added_point_id == boundary &&
          result.cominimizers[0U].added_point_in_source_closed_ball &&
          !result.cominimizers[0U]
               .added_point_in_selected_positive_support &&
          result.audit.inside_or_boundary_source_ball_point_count == 1U,
      "a point exactly on the closed source sphere remains an equal co-minimizer");
}

void test_provisional_overflow_is_erased_by_a_better_incumbent() {
  const std::array<CertifiedPoint3, 4U> input{
      point(0.0), point(-10.0), point(10.0), point(2.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSparseFacetKey source =
      key_from_sources(cloud, std::array<std::size_t, 1U>{0U});
  ExactDirectSparseFirstIncidenceBudget budget = generous_budget();
  budget.maximum_cominimizer_count = 1U;
  const auto result = build_and_verify(
      index,
      cloud,
      source,
      budget,
      LbvhTraversalOrder::far_first,
      "the provisional equality-shell overflow fixture");

  check(
      result.certified_complete_first_incidence() &&
          result.first_incidence_squared_level == level(1) &&
          result.cominimizers.size() == 1U &&
          result.cominimizers[0U].added_point_id ==
              point_id_for_source(cloud, 3U) &&
          result.audit.provisional_cominimizer_overflow_count == 1U &&
          result.audit.incumbent_improvement_count >= 2U,
      "a saturated provisional shell is discarded when a later strict improvement fits the output budget");
}

void test_k10_retains_two_outside_cominimizers() {
  const std::array<CertifiedPoint3, 12U> input{
      point(0.0),
      point(1.0),
      point(2.0),
      point(3.0),
      point(4.0),
      point(5.0),
      point(6.0),
      point(7.0),
      point(8.0),
      point(9.0),
      point(-1.0),
      point(10.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSparseFacetKey source = key_from_sources(
      cloud,
      std::array<std::size_t, 10U>{
          0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U});
  std::vector<PointId> expected{
      point_id_for_source(cloud, 10U), point_id_for_source(cloud, 11U)};
  std::sort(expected.begin(), expected.end());
  const auto result = build_and_verify(
      index,
      cloud,
      source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the K=10 two-sided outside equality fixture");

  std::vector<PointId> observed;
  for (const auto& minimizer : result.cominimizers) {
    observed.push_back(minimizer.added_point_id);
  }
  check(
      result.certified_complete_first_incidence() &&
          result.first_incidence_squared_level == level(25) &&
          observed == expected &&
          result.audit.coface_support_enumeration_count == 352U &&
          std::all_of(
              result.cominimizers.begin(),
              result.cominimizers.end(),
              [](const ExactDirectSparseFirstIncidenceMinimizer& minimizer) {
                return !minimizer.added_point_in_source_closed_ball &&
                       minimizer.added_point_in_selected_positive_support;
              }),
      "K=10 preserves both outside minimizers after two independent 176-support enumerations");
}

void test_small_bounded_differential_against_full_miniballs() {
  const std::array<CertifiedPoint3, 6U> input{
      point(-3.0, 1.0, 0.0),
      point(-1.0, -2.0, 1.0),
      point(0.0, 3.0, -1.0),
      point(2.0, -1.0, 2.0),
      point(4.0, 2.0, 1.0),
      point(1.0, 1.0, 5.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::uint32_t full_mask =
      (UINT32_C(1) << static_cast<std::uint32_t>(cloud.size())) - 1U;

  for (std::uint32_t mask = 1U; mask < full_mask; ++mask) {
    ExactDirectSparseFacetKey source;
    std::vector<PointId> source_ids;
    for (std::size_t point_index = 0U;
         point_index < cloud.size();
         ++point_index) {
      if ((mask &
           (UINT32_C(1) << static_cast<std::uint32_t>(point_index))) != 0U) {
        const PointId point_id = static_cast<PointId>(point_index);
        source.point_ids[source.point_count] = point_id;
        ++source.point_count;
        source_ids.push_back(point_id);
      }
    }

    std::optional<ExactLevel> expected_level;
    std::vector<PointId> expected_minimizers;
    for (std::size_t point_index = 0U;
         point_index < cloud.size();
         ++point_index) {
      const PointId added_point_id = static_cast<PointId>(point_index);
      if (std::binary_search(
              source_ids.begin(), source_ids.end(), added_point_id)) {
        continue;
      }
      std::vector<PointId> coface = source_ids;
      coface.push_back(added_point_id);
      std::sort(coface.begin(), coface.end());
      const ExactFacetMiniballResult miniball = build_exact_facet_miniball(
          cloud, std::span<const PointId>{coface});
      if (!expected_level.has_value() ||
          miniball.squared_radius < *expected_level) {
        expected_level = miniball.squared_radius;
        expected_minimizers = {added_point_id};
      } else if (miniball.squared_radius == *expected_level) {
        expected_minimizers.push_back(added_point_id);
      }
    }
    std::sort(expected_minimizers.begin(), expected_minimizers.end());

    const auto result = build_exact_direct_sparse_first_incidence(
        index,
        cloud,
        source,
        generous_budget(),
        (mask & 1U) == 0U ? LbvhTraversalOrder::near_first
                          : LbvhTraversalOrder::far_first);
    std::vector<PointId> observed_minimizers;
    for (const auto& minimizer : result.cominimizers) {
      observed_minimizers.push_back(minimizer.added_point_id);
    }
    check(
        result.certified_complete_first_incidence() &&
            result.first_incidence_squared_level == expected_level &&
            observed_minimizers == expected_minimizers,
        "the reduced support search matches full exact miniballs for mask " +
            std::to_string(mask));
  }
}

void test_all_budget_exhaustions_publish_no_partial_shell() {
  EqualityFixture fixture;
  const auto baseline = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.source,
      generous_budget(),
      LbvhTraversalOrder::near_first,
      "the exact-budget baseline");
  const ExactDirectSparseFirstIncidenceBudget exact =
      exact_budget_for(baseline);
  const auto exact_result = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.source,
      exact,
      LbvhTraversalOrder::near_first,
      "the exact nine-cap replay");
  check(
      exact_result.first_incidence_squared_level ==
              baseline.first_incidence_squared_level &&
          exact_result.cominimizers == baseline.cominimizers &&
          exact_result.audit == baseline.audit,
      "all nine exact caps reproduce the complete first-incidence result");

  check(
      exact.maximum_source_support_enumeration_count != 0U &&
          exact.maximum_node_visit_count != 0U &&
          exact.maximum_internal_node_expansion_count != 0U &&
          exact.maximum_exact_aabb_bound_evaluation_count != 0U &&
          exact.maximum_exact_point_evaluation_count != 0U &&
          exact.maximum_coface_support_enumeration_count != 0U &&
          exact.maximum_candidate_point_classification_count != 0U &&
          exact.maximum_frontier_entry_count != 0U &&
          exact.maximum_cominimizer_count == 2U,
      "the equality fixture exercises every independent first-incidence cap");

  struct BudgetCase {
    ExactDirectSparseFirstIncidenceBudget budget;
    ExactDirectSparseFirstIncidenceStopReason reason;
    std::string_view name;
  };
  std::vector<BudgetCase> cases;
  {
    auto budget = exact;
    --budget.maximum_source_support_enumeration_count;
    cases.push_back({
        budget,
        ExactDirectSparseFirstIncidenceStopReason::
            source_support_enumeration_limit,
        "source support"});
  }
  {
    auto budget = exact;
    --budget.maximum_node_visit_count;
    cases.push_back({
        budget,
        ExactDirectSparseFirstIncidenceStopReason::node_visit_limit,
        "node visit"});
  }
  {
    auto budget = exact;
    --budget.maximum_internal_node_expansion_count;
    cases.push_back({
        budget,
        ExactDirectSparseFirstIncidenceStopReason::
            internal_node_expansion_limit,
        "internal expansion"});
  }
  {
    auto budget = exact;
    --budget.maximum_exact_aabb_bound_evaluation_count;
    cases.push_back({
        budget,
        ExactDirectSparseFirstIncidenceStopReason::
            exact_aabb_bound_evaluation_limit,
        "exact AABB"});
  }
  {
    auto budget = exact;
    --budget.maximum_exact_point_evaluation_count;
    cases.push_back({
        budget,
        ExactDirectSparseFirstIncidenceStopReason::
            exact_point_evaluation_limit,
        "exact point"});
  }
  {
    auto budget = exact;
    --budget.maximum_coface_support_enumeration_count;
    cases.push_back({
        budget,
        ExactDirectSparseFirstIncidenceStopReason::
            coface_support_enumeration_limit,
        "coface support"});
  }
  {
    auto budget = exact;
    --budget.maximum_candidate_point_classification_count;
    cases.push_back({
        budget,
        ExactDirectSparseFirstIncidenceStopReason::
            candidate_point_classification_limit,
        "candidate classification"});
  }
  {
    auto budget = exact;
    --budget.maximum_frontier_entry_count;
    cases.push_back({
        budget,
        ExactDirectSparseFirstIncidenceStopReason::frontier_entry_limit,
        "frontier"});
  }
  {
    auto budget = exact;
    --budget.maximum_cominimizer_count;
    cases.push_back({
        budget,
        ExactDirectSparseFirstIncidenceStopReason::cominimizer_entry_limit,
        "co-minimizer"});
  }

  for (const BudgetCase& budget_case : cases) {
    const std::string context =
        "the one-short " + std::string{budget_case.name} + " cap";
    const auto result = build_and_verify(
        fixture.index,
        fixture.cloud,
        fixture.source,
        budget_case.budget,
        LbvhTraversalOrder::near_first,
        context);
    check(
        result.certified_budget_exhaustion() &&
            result.stop_reason == budget_case.reason &&
            result.decision == ExactDirectSparseFirstIncidenceDecision::
                                   no_first_incidence_budget_exhausted &&
            !result.first_incidence_squared_level.has_value() &&
            result.cominimizers.empty() &&
            !result.all_cominimizers_retained_atomically &&
            result.no_partial_first_incidence_payload_published,
        context + " publishes neither a provisional level nor a partial equality shell");
  }
}

void test_hostile_verifier_rejects_storage_and_scientific_mutations() {
  EqualityFixture fixture;
  const ExactDirectSparseFirstIncidenceBudget budget = generous_budget();
  const auto original = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.source,
      budget,
      LbvhTraversalOrder::near_first,
      "the hostile-verifier baseline");

  auto bad_level = original;
  bad_level.first_incidence_squared_level = level(2);
  const auto bad_level_verification = verify_exact_direct_sparse_first_incidence(
      fixture.index,
      fixture.cloud,
      fixture.source,
      budget,
      LbvhTraversalOrder::near_first,
      bad_level);
  check(
      bad_level_verification.trusted_inputs_certified &&
          bad_level_verification.observed_storage_within_budget &&
          !bad_level_verification.branch_and_bound_freshly_replayed &&
          !bad_level_verification.result_certified,
      "a forged first-incidence level fails the fresh branch-and-bound replay");

  auto missing_equal = original;
  missing_equal.cominimizers.pop_back();
  const auto missing_equal_verification =
      verify_exact_direct_sparse_first_incidence(
          fixture.index,
          fixture.cloud,
          fixture.source,
          budget,
          LbvhTraversalOrder::near_first,
          missing_equal);
  check(
      missing_equal_verification.trusted_inputs_certified &&
          missing_equal_verification.observed_storage_within_budget &&
          !missing_equal_verification.all_cominimizers_freshly_replayed &&
          !missing_equal_verification.result_certified,
      "a dropped equal minimizer is rejected rather than accepted as a valid prefix");

  auto bad_audit = original;
  ++bad_audit.audit.pruned_node_count;
  const auto bad_audit_verification = verify_exact_direct_sparse_first_incidence(
      fixture.index,
      fixture.cloud,
      fixture.source,
      budget,
      LbvhTraversalOrder::near_first,
      bad_audit);
  check(
      bad_audit_verification.trusted_inputs_certified &&
          bad_audit_verification.observed_storage_within_budget &&
          !bad_audit_verification.branch_and_bound_freshly_replayed &&
          !bad_audit_verification.result_certified,
      "a forged prune audit fails the fresh traversal replay");

  auto forbidden = original;
  forbidden.no_global_facet_or_coface_catalog_materialized = false;
  const auto forbidden_verification = verify_exact_direct_sparse_first_incidence(
      fixture.index,
      fixture.cloud,
      fixture.source,
      budget,
      LbvhTraversalOrder::near_first,
      forbidden);
  check(
      forbidden_verification.trusted_inputs_certified &&
          forbidden_verification.observed_storage_within_budget &&
          !forbidden_verification.no_forbidden_global_structure_materialized &&
          !forbidden_verification.result_certified,
      "an invented global coface catalogue invalidates the sparse scope");

  auto oversized = original;
  oversized.cominimizers.resize(budget.maximum_cominimizer_count + 1U);
  const auto oversized_verification = verify_exact_direct_sparse_first_incidence(
      fixture.index,
      fixture.cloud,
      fixture.source,
      budget,
      LbvhTraversalOrder::near_first,
      oversized);
  check(
      oversized_verification.trusted_inputs_certified &&
          !oversized_verification.observed_storage_within_budget &&
          !oversized_verification.fresh_replay_certified &&
          !oversized_verification.result_certified,
      "oversized observed storage is rejected before a scientific replay");

  auto oversized_source = original;
  oversized_source.source_facet_miniball->facet_point_ids.resize(11U);
  const auto oversized_source_verification =
      verify_exact_direct_sparse_first_incidence(
          fixture.index,
          fixture.cloud,
          fixture.source,
          budget,
          LbvhTraversalOrder::near_first,
          oversized_source);
  check(
      oversized_source_verification.trusted_inputs_certified &&
          !oversized_source_verification.observed_storage_within_budget &&
          !oversized_source_verification.fresh_replay_certified &&
          !oversized_source_verification.result_certified,
      "an oversized nested source-miniball vector is rejected before replay");

  auto bad_support = original;
  bad_support.cominimizers[0U].support_point_ids[0U] =
      fixture.positive_minimizer;
  const auto bad_support_verification =
      verify_exact_direct_sparse_first_incidence(
          fixture.index,
          fixture.cloud,
          fixture.source,
          budget,
          LbvhTraversalOrder::near_first,
          bad_support);
  check(
      bad_support_verification.trusted_inputs_certified &&
          bad_support_verification.observed_storage_within_budget &&
          !bad_support_verification.all_cominimizers_freshly_replayed &&
          !bad_support_verification.result_certified,
      "a forged selected support is rejected by the fresh complete-shell replay");

  auto bad_atomicity = original;
  bad_atomicity.no_partial_first_incidence_payload_published = false;
  const auto bad_atomicity_verification =
      verify_exact_direct_sparse_first_incidence(
          fixture.index,
          fixture.cloud,
          fixture.source,
          budget,
          LbvhTraversalOrder::near_first,
          bad_atomicity);
  check(
      bad_atomicity_verification.trusted_inputs_certified &&
          bad_atomicity_verification.observed_storage_within_budget &&
          !bad_atomicity_verification.all_cominimizers_freshly_replayed &&
          !bad_atomicity_verification.result_certified,
      "a forged no-partial-publication claim is rejected independently of completeness");

  auto oversized_peak = original;
  oversized_peak.audit.peak_cominimizer_entry_count =
      budget.maximum_cominimizer_count + 1U;
  const auto oversized_peak_verification =
      verify_exact_direct_sparse_first_incidence(
          fixture.index,
          fixture.cloud,
          fixture.source,
          budget,
          LbvhTraversalOrder::near_first,
          oversized_peak);
  check(
      oversized_peak_verification.trusted_inputs_certified &&
          !oversized_peak_verification.observed_storage_within_budget &&
          !oversized_peak_verification.fresh_replay_certified &&
          !oversized_peak_verification.result_certified,
      "an impossible observed peak is rejected before the scientific replay");
}

void test_contract_metadata() {
  check(
      ExactDirectSparseFirstIncidenceResult::backend == "reference_cpu" &&
          ExactDirectSparseFirstIncidenceResult::profile == "hgp_reduced" &&
          ExactDirectSparseFirstIncidenceResult::mode == "certified" &&
          ExactDirectSparseFirstIncidenceResult::refinement_status ==
              "partial_refinement" &&
          ExactDirectSparseFirstIncidenceResult::public_status ==
              "not_claimed" &&
          ExactDirectSparseFirstIncidenceResult::proof_basis ==
              direct_sparse_first_incidence_proof_basis,
      "the first-incidence oracle advertises only its bounded partial-refinement scope");
}

}  // namespace

int main() {
  test_contract_metadata();
  test_no_coface_when_n_equals_k();
  test_invalid_authorities_are_rejected_before_traversal();
  test_unique_minimizer_and_exact_positive_support();
  test_inside_boundary_and_selected_support_semantics();
  test_equal_cominimizers_strict_pruning_and_traversal_invariance();
  test_new_support_uses_a_point_outside_the_old_support();
  test_k10_avoids_an_eleven_point_key_and_uses_176_supports();
  test_ac_silent_equal_first_incidence();
  test_source_boundary_point_is_an_equal_minimizer();
  test_provisional_overflow_is_erased_by_a_better_incumbent();
  test_k10_retains_two_outside_cominimizers();
  test_small_bounded_differential_against_full_miniballs();
  test_bounded_n14_differential_against_all_explicit_cofaces();
  test_all_budget_exhaustions_publish_no_partial_shell();
  test_hostile_verifier_rejects_storage_and_scientific_mutations();

  if (failures != 0) {
    std::cerr << failures
              << " direct sparse first-incidence test(s) failed\n";
    return 1;
  }
  std::cout << "direct sparse first-incidence tests passed\n";
  return 0;
}
