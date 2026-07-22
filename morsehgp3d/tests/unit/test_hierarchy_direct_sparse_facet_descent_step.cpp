#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_step.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactCenter3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactDirectSparseComponentHandle;
using morsehgp3d::hierarchy::ExactDirectSparseComponentUnion;
using morsehgp3d::hierarchy::ExactDirectSparseFacetBinding;
using morsehgp3d::hierarchy::ExactDirectSparseFacetDescentStepBudget;
using morsehgp3d::hierarchy::ExactDirectSparseFacetDescentStepDecision;
using morsehgp3d::hierarchy::ExactDirectSparseFacetDescentStepDisposition;
using morsehgp3d::hierarchy::ExactDirectSparseFacetDescentStepResult;
using morsehgp3d::hierarchy::ExactDirectSparseFacetDescentStepVerification;
using morsehgp3d::hierarchy::ExactDirectSparseFacetDescentStepWitness;
using morsehgp3d::hierarchy::ExactDirectSparseFacetKey;
using morsehgp3d::hierarchy::ExactDirectSparseFacetWitness;
using morsehgp3d::hierarchy::ExactDirectSparsePositiveFacetLocator;
using morsehgp3d::hierarchy::ExactDirectSparsePositiveFacetLocatorBudget;
using morsehgp3d::hierarchy::ExactDirectSparsePositiveFacetLocatorConfig;
using morsehgp3d::hierarchy::ExactDirectSparsePositiveFacetProbeBudget;
using morsehgp3d::hierarchy::build_exact_direct_sparse_facet_descent_step;
using morsehgp3d::hierarchy::build_exact_direct_sparse_positive_facet_locator;
using morsehgp3d::hierarchy::verify_exact_direct_sparse_facet_descent_step;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactLbvhTopKAudit;
using morsehgp3d::spatial::ExactLbvhTopKBudget;
using morsehgp3d::spatial::ExactLbvhTopKStopReason;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

constexpr std::uint64_t authority_id = UINT64_C(0x105b);
int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
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

[[nodiscard]] ExactDirectSparseFacetKey key(
    std::initializer_list<PointId> point_ids) {
  ExactDirectSparseFacetKey result;
  result.point_count = point_ids.size();
  std::size_t index = 0U;
  for (const PointId point_id : point_ids) {
    result.point_ids[index] = point_id;
    ++index;
  }
  return result;
}

[[nodiscard]] ExactDirectSparseFacetWitness witness(
    std::uint64_t replay_token) {
  return {authority_id, replay_token};
}

[[nodiscard]] ExactDirectSparsePositiveFacetLocatorBudget locator_budget() {
  return {
      8U,
      16U,
      160U,
      16U,
      16U,
      16U,
      16U,
      16U,
      160U,
      33U,
      33U,
  };
}

[[nodiscard]] ExactDirectSparsePositiveFacetLocator make_locator() {
  return build_exact_direct_sparse_positive_facet_locator(
      8U,
      locator_budget(),
      ExactDirectSparsePositiveFacetLocatorConfig{
          authority_id, std::numeric_limits<std::uint64_t>::max()});
}

void seed_binding(
    ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetKey& facet_key,
    ExactDirectSparseComponentHandle handle,
    std::uint64_t replay_token) {
  const std::array<ExactDirectSparseFacetBinding, 1U> bindings{{
      {0U, facet_key, handle, witness(replay_token)},
  }};
  const auto seeded = locator.apply_batch(
      std::span<const morsehgp3d::hierarchy::ExactDirectSparseFacetQuery>{},
      std::span<const morsehgp3d::hierarchy::ExactDirectSparseComponentUnion>{},
      bindings);
  check(
      seeded.certified_committed_batch(),
      "the relative positive locator seed is committed");
}

void parent_component_handle(
    ExactDirectSparsePositiveFacetLocator& locator,
    ExactDirectSparseComponentHandle child,
    ExactDirectSparseComponentHandle parent,
    std::uint64_t replay_token) {
  const std::array<ExactDirectSparseComponentUnion, 1U> unions{{
      {0U, child, parent, witness(replay_token)},
  }};
  const auto committed = locator.apply_batch(
      std::span<const morsehgp3d::hierarchy::ExactDirectSparseFacetQuery>{},
      unions,
      std::span<const ExactDirectSparseFacetBinding>{});
  check(
      committed.certified_committed_batch() &&
          locator.component_parents()[child] == parent,
      "the locator fixture commits one visible DSU parent edge");
}

[[nodiscard]] ExactDirectSparseFacetDescentStepBudget generous_step_budget() {
  return {
      ExactDirectSparsePositiveFacetProbeBudget{33U, 8U},
      ExactLbvhTopKBudget{
          1024U, 1024U, 1024U, 1024U, 32U, 10U, 10U},
      ExactDirectSparsePositiveFacetProbeBudget{33U, 8U},
  };
}

[[nodiscard]] bool fresh_verification_closes(
    const ExactDirectSparseFacetDescentStepVerification& verification) {
  return verification.trusted_inputs_certified &&
         verification.observed_outcome_well_formed &&
         verification.source_key_freshly_reconstructed &&
         verification.bounded_top_k_freshly_replayed &&
         verification.strict_witness_freshly_replayed &&
         verification.locator_probes_freshly_replayed &&
         verification.no_partial_top_k_partition_persisted &&
         verification.no_locator_mutation_or_batch_commit &&
         verification.no_isolation_singleton_or_attachment_invented &&
         !verification.external_binding_authority_replayed &&
         verification.no_forbidden_global_structure_materialized &&
         verification.fresh_replay_certified &&
         verification.result_certified;
}

[[nodiscard]] ExactDirectSparseFacetDescentStepResult build_and_verify(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::span<const PointId> source_facet,
    const ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseFacetWitness& query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetDescentStepBudget& budget,
    LbvhTraversalOrder traversal_order,
    const std::string& context) {
  const ExactDirectSparseFacetDescentStepResult result =
      build_exact_direct_sparse_facet_descent_step(
          index,
          cloud,
          source_facet,
          closed_batch_squared_level,
          query_witness,
          locator,
          budget,
          traversal_order);
  const ExactDirectSparseFacetDescentStepVerification verification =
      verify_exact_direct_sparse_facet_descent_step(
          index,
          cloud,
          source_facet,
          closed_batch_squared_level,
          query_witness,
          locator,
          budget,
          traversal_order,
          result);
  check(
      fresh_verification_closes(verification),
      context + " closes under a fresh replay");
  return result;
}

[[nodiscard]] CanonicalPointCloud ac_de_cloud() {
  // Input labels A, B, C, D, E canonicalize to D=0, A=1, B=2, C=3, E=4.
  const std::array<CertifiedPoint3, 5U> input{
      point(0.0, 0.0, 7.0),
      point(0.0, 9.0, 6.0),
      point(1.0, 4.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(4.0, 1.0, 2.0),
  };
  return canonical_cloud(input);
}

[[nodiscard]] CanonicalPointCloud lr_lp_cloud() {
  // Canonical ids are Q=0, L=1, P=2, R=3.
  const std::array<CertifiedPoint3, 4U> input{
      point(-1.25, 0.25, 0.5),
      point(-1.0, 0.0, 0.0),
      point(0.0, 0.5, 0.0),
      point(1.0, 0.0, 0.0),
  };
  return canonical_cloud(input);
}

void check_closed_partial_scope(
    const ExactDirectSparseFacetDescentStepResult& result,
    const std::string& context) {
  check(
      !result.locator_state_mutated && !result.locator_batch_committed &&
          !result.external_binding_authority_replayed &&
          !result.missing_facet_means_isolated &&
          !result.singleton_component_created &&
          !result.global_closed_ball_materialized &&
          !result.forbidden_global_structure_materialized &&
          !result.hierarchy_attachment_published &&
          !result.public_status_claimed && result.partial_refinement_only,
      context + " remains a side-effect-free partial refinement");
}

void test_equal_level_ac_to_de_closed_segment() {
  const CanonicalPointCloud cloud = ac_de_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::array<PointId, 2U> source{1U, 3U};
  const ExactDirectSparseFacetKey successor = key({0U, 4U});
  ExactDirectSparsePositiveFacetLocator locator = make_locator();
  seed_binding(locator, successor, 3U, 101U);
  const ExactDirectSparsePositiveFacetLocator locator_before = locator;
  const auto* const slots_data_before = locator.slots().data();
  const auto* const key_data_before = locator.key_point_arena().data();
  const auto* const parents_data_before = locator.component_parents().data();
  const auto budget = generous_step_budget();
  std::optional<ExactDirectSparseFacetDescentStepWitness> first_witness;

  for (const LbvhTraversalOrder order : {
           LbvhTraversalOrder::near_first,
           LbvhTraversalOrder::far_first}) {
    const auto result = build_and_verify(
        index,
        cloud,
        source,
        level(33, 2),
        witness(1001U),
        locator,
        budget,
        order,
        "the equal-level AC-to-DE strict step");
    check(
        result.certified_relative_positive_resolution() &&
            result.disposition ==
                ExactDirectSparseFacetDescentStepDisposition::relative_positive &&
            result.decision == ExactDirectSparseFacetDescentStepDecision::
                                   complete_relative_strict_successor_positive_hit &&
            result.resolved_component_handle ==
                std::optional<ExactDirectSparseComponentHandle>{3U} &&
            result.resolved_binding_witness ==
                std::optional<ExactDirectSparseFacetWitness>{witness(101U)} &&
            result.source_locator_probe.certified_unresolved_miss() &&
            result.successor_locator_probe.has_value() &&
            result.successor_locator_probe->certified_positive_hit(),
        "AC resolves only through the pre-call DE binding");
    check(result.strict_step_witness.has_value(), "AC-to-DE retains one strict witness");
    if (!result.strict_step_witness.has_value()) {
      continue;
    }
    const auto& strict = *result.strict_step_witness;
    check(
        strict.source_facet_key == key({1U, 3U}) &&
            strict.successor_facet_key == successor &&
            strict.source_center == center(1, 4, 7, 2) &&
            strict.successor_center == center(4, 1, 3, 2) &&
            strict.source_facet_squared_level == level(33, 2) &&
            strict.top_k_cutoff_squared_level == level(31, 2) &&
            strict.successor_at_source_squared_level == level(31, 2) &&
            strict.successor_facet_squared_level == level(9, 2) &&
            strict.center_squared_displacement == level(17, 2),
        "AC-to-DE records the exact centers, cutoff, miniballs and displacement");
    check(
        strict.successor_is_complete_canonical_top_k_choice &&
            strict.successor_differs_from_source &&
            strict.successor_at_source_equals_top_k_cutoff &&
            strict.top_k_cutoff_at_most_source_level &&
            strict.successor_level_at_most_top_k_cutoff &&
            strict.strict_miniball_level_decrease &&
            strict.exact_squared_distance_chord_identity_applies &&
            strict.source_facet_at_or_below_closed_batch_level &&
            strict.source_open_target_closed_segment_strict_below_source_level &&
            strict.source_open_target_closed_segment_strict_below_closed_batch_level &&
            strict.closed_segment_strict_below_closed_batch_level,
        "AC-to-DE accepts beta(F)=a while its 31/2 cutoff closes the whole segment below 33/2");
    check(
        result.closed_batch_squared_level == level(33, 2) &&
            result.top_k_stop_reason == ExactLbvhTopKStopReason::none &&
            result.complete_top_k_query_counters.has_value() &&
            result.counters.exact_level_relation_count == 6U &&
            result.strict_half_open_segment_certified,
        "AC-to-DE exposes a complete seven-cap top-k audit and six exact relations");
    check_closed_partial_scope(result, "the AC-to-DE result");
    if (!first_witness.has_value()) {
      first_witness = strict;
    } else {
      check(
          strict == *first_witness,
          "near-first and far-first preserve the AC-to-DE scientific witness");
    }
  }

  check(
      locator == locator_before && locator.slots().data() == slots_data_before &&
          locator.key_point_arena().data() == key_data_before &&
          locator.component_parents().data() == parents_data_before,
      "both AC-to-DE traversals leave the locator storage and state unchanged");
}

void test_equal_level_lr_to_lp_source_open_segment() {
  const CanonicalPointCloud cloud = lr_lp_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::array<PointId, 2U> source{1U, 3U};
  const ExactDirectSparseFacetKey successor = key({1U, 2U});
  ExactDirectSparsePositiveFacetLocator locator = make_locator();
  seed_binding(locator, successor, 4U, 201U);
  const auto budget = generous_step_budget();
  std::optional<ExactDirectSparseFacetDescentStepWitness> first_witness;

  for (const LbvhTraversalOrder order : {
           LbvhTraversalOrder::near_first,
           LbvhTraversalOrder::far_first}) {
    const auto result = build_and_verify(
        index,
        cloud,
        source,
        level(1),
        witness(2001U),
        locator,
        budget,
        order,
        "the equal-level LR-to-LP source-open step");
    check(
        result.certified_relative_positive_resolution() &&
            result.resolved_component_handle ==
                std::optional<ExactDirectSparseComponentHandle>{4U} &&
            result.strict_step_witness.has_value(),
        "LR resolves through its strict canonical LP successor");
    if (!result.strict_step_witness.has_value()) {
      continue;
    }
    const auto& strict = *result.strict_step_witness;
    check(
        strict.source_facet_key == key({1U, 3U}) &&
            strict.successor_facet_key == successor &&
            strict.source_center == center(0, 0, 0) &&
            strict.successor_center == center(-2, 1, 0, 4) &&
            strict.source_facet_squared_level == level(1) &&
            strict.top_k_cutoff_squared_level == level(1) &&
            strict.successor_at_source_squared_level == level(1) &&
            strict.successor_facet_squared_level == level(5, 16) &&
            strict.center_squared_displacement == level(5, 16),
        "LR-to-LP records the exact equality cutoff, target miniball and displacement");
    check(
        strict.source_facet_at_or_below_closed_batch_level &&
            strict.source_open_target_closed_segment_strict_below_source_level &&
            strict.source_open_target_closed_segment_strict_below_closed_batch_level &&
            !strict.closed_segment_strict_below_closed_batch_level &&
            strict.strict_miniball_level_decrease,
        "LR-to-LP keeps only the source-open claim when cutoff=beta(F)=a");
    check_closed_partial_scope(result, "the LR-to-LP result");
    if (!first_witness.has_value()) {
      first_witness = strict;
    } else {
      check(
          strict == *first_witness,
          "near-first and far-first preserve the LR-to-LP scientific witness");
    }
  }
}

void test_source_hit_missing_target_and_above_batch() {
  const CanonicalPointCloud cloud = ac_de_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::array<PointId, 2U> source{1U, 3U};
  const auto query_witness = witness(3001U);

  ExactDirectSparsePositiveFacetLocator source_locator = make_locator();
  seed_binding(source_locator, key({1U, 3U}), 2U, 301U);
  auto source_hit_budget = generous_step_budget();
  source_hit_budget.top_k_query = ExactLbvhTopKBudget{};
  source_hit_budget.successor_locator_probe =
      ExactDirectSparsePositiveFacetProbeBudget{};
  const auto source_hit = build_and_verify(
      index,
      cloud,
      source,
      level(33, 2),
      query_witness,
      source_locator,
      source_hit_budget,
      LbvhTraversalOrder::near_first,
      "the direct source hit");
  check(
      source_hit.certified_relative_positive_resolution() &&
          source_hit.decision == ExactDirectSparseFacetDescentStepDecision::
                                     complete_relative_source_positive_hit &&
          source_hit.resolved_component_handle ==
              std::optional<ExactDirectSparseComponentHandle>{2U} &&
          source_hit.resolved_binding_witness ==
              std::optional<ExactDirectSparseFacetWitness>{witness(301U)} &&
          source_hit.counters.source_miniball_build_count == 0U &&
          source_hit.counters.top_k_query_count == 0U &&
          !source_hit.strict_step_witness.has_value() &&
          !source_hit.successor_locator_probe.has_value(),
      "a source hit short-circuits before every geometric operation");

  bool invalid_order_rejected = false;
  try {
    static_cast<void>(build_exact_direct_sparse_facet_descent_step(
        index,
        cloud,
        source,
        level(33, 2),
        query_witness,
        source_locator,
        source_hit_budget,
        static_cast<LbvhTraversalOrder>(255U)));
  } catch (const std::invalid_argument&) {
    invalid_order_rejected = true;
  }
  check(
      invalid_order_rejected,
      "an invalid traversal order is rejected before a source-hit short circuit");

  const ExactDirectSparsePositiveFacetLocator empty_locator = make_locator();
  const auto missing_target = build_and_verify(
      index,
      cloud,
      source,
      level(33, 2),
      query_witness,
      empty_locator,
      generous_step_budget(),
      LbvhTraversalOrder::near_first,
      "the absent strict target");
  check(
      missing_target.certified_unresolved_without_isolation() &&
          missing_target.decision == ExactDirectSparseFacetDescentStepDecision::
                                         complete_unresolved_strict_successor_not_bound &&
          missing_target.strict_step_witness.has_value() &&
          missing_target.successor_locator_probe.has_value() &&
          missing_target.successor_locator_probe->certified_unresolved_miss() &&
          !missing_target.resolved_component_handle.has_value() &&
          !missing_target.missing_facet_means_isolated &&
          !missing_target.singleton_component_created,
      "an absent DE binding remains unresolved without an invented root");

  const auto above_batch = build_and_verify(
      index,
      cloud,
      source,
      level(16),
      query_witness,
      empty_locator,
      generous_step_budget(),
      LbvhTraversalOrder::near_first,
      "the source above its closed batch");
  check(
      above_batch.certified_unresolved_without_isolation() &&
          above_batch.decision == ExactDirectSparseFacetDescentStepDecision::
                                      complete_unresolved_source_above_closed_batch_level &&
          above_batch.counters.source_miniball_build_count == 1U &&
          above_batch.counters.top_k_query_count == 0U &&
          !above_batch.strict_step_witness.has_value() &&
          !above_batch.successor_locator_probe.has_value(),
      "beta(AC)=33/2 is unresolved, rather than inactive by implication, above a closed level 16");
}

void test_canonical_source_and_non_strict_successor() {
  {
    const std::array<CertifiedPoint3, 2U> input{
        point(-1.0), point(1.0)};
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
    const std::array<PointId, 2U> source{0U, 1U};
    const auto result = build_and_verify(
        index,
        cloud,
        source,
        level(1),
        witness(4001U),
        make_locator(),
        generous_step_budget(),
        LbvhTraversalOrder::near_first,
        "the canonical source fixed point");
    check(
        result.certified_unresolved_without_isolation() &&
            result.decision == ExactDirectSparseFacetDescentStepDecision::
                                   complete_unresolved_source_is_canonical_top_k_choice &&
            result.counters.successor_miniball_build_count == 0U &&
            !result.strict_step_witness.has_value(),
        "G=F stops without inventing a strict arc");
  }

  {
    // Canonical ids are L=0, D=1, U=2, R=3.  Both triples contain an
    // antipodal pair, so F={L,U,R} and canonical G={L,D,U} have level one.
    const std::array<CertifiedPoint3, 4U> input{
        point(-1.0, 0.0, 0.0),
        point(0.0, -1.0, 0.0),
        point(0.0, 1.0, 0.0),
        point(1.0, 0.0, 0.0),
    };
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
    const std::array<PointId, 3U> source{0U, 2U, 3U};
    const auto result = build_and_verify(
        index,
        cloud,
        source,
        level(1),
        witness(4002U),
        make_locator(),
        generous_step_budget(),
        LbvhTraversalOrder::near_first,
        "the distinct non-strict successor");
    check(
        result.certified_unresolved_without_isolation() &&
            result.decision == ExactDirectSparseFacetDescentStepDecision::
                                   complete_unresolved_non_strict_canonical_successor &&
            result.successor_miniball_freshly_certified &&
            result.counters.successor_miniball_build_count == 1U &&
            !result.strict_step_witness.has_value() &&
            !result.successor_locator_probe.has_value(),
        "a distinct equal-level canonical successor remains unresolved");
  }
}

[[nodiscard]] ExactLbvhTopKBudget exact_top_k_budget(
    const ExactLbvhTopKAudit& audit) {
  return {
      audit.node_visit_count,
      audit.internal_node_expansion_count,
      audit.exact_aabb_bound_evaluation_count,
      audit.exact_point_distance_evaluation_count,
      audit.peak_frontier_entry_count,
      audit.peak_best_neighbor_entry_count,
      audit.peak_retained_cutoff_shell_entry_count,
  };
}

void test_all_budget_exhaustions() {
  const CanonicalPointCloud cloud = ac_de_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::array<PointId, 2U> source{1U, 3U};
  const auto query_witness = witness(5001U);
  const ExactDirectSparsePositiveFacetLocator empty_locator = make_locator();

  auto source_short_budget = generous_step_budget();
  source_short_budget.source_locator_probe.maximum_slot_visit_count = 0U;
  const auto source_short = build_and_verify(
      index,
      cloud,
      source,
      level(33, 2),
      query_witness,
      empty_locator,
      source_short_budget,
      LbvhTraversalOrder::near_first,
      "the exhausted source probe");
  check(
      source_short.certified_budget_exhaustion() &&
          source_short.decision == ExactDirectSparseFacetDescentStepDecision::
                                       no_resolution_source_locator_probe_budget_exhausted &&
          source_short.counters.source_miniball_build_count == 0U &&
          source_short.counters.top_k_query_count == 0U,
      "a zero-slot source probe exhausts before geometry");

  ExactDirectSparsePositiveFacetLocator parented_source_locator = make_locator();
  seed_binding(parented_source_locator, key({1U, 3U}), 5U, 502U);
  parent_component_handle(parented_source_locator, 5U, 1U, 503U);
  auto source_parent_hop_short_budget = generous_step_budget();
  source_parent_hop_short_budget.source_locator_probe
      .maximum_component_parent_hop_count = 0U;
  const auto source_parent_hop_short = build_and_verify(
      index,
      cloud,
      source,
      level(33, 2),
      query_witness,
      parented_source_locator,
      source_parent_hop_short_budget,
      LbvhTraversalOrder::near_first,
      "the exhausted source parent-hop probe");
  check(
      source_parent_hop_short.certified_budget_exhaustion() &&
          source_parent_hop_short.decision ==
              ExactDirectSparseFacetDescentStepDecision::
                  no_resolution_source_locator_probe_budget_exhausted &&
          source_parent_hop_short.source_locator_probe
              .component_parent_hop_budget_exhausted &&
          source_parent_hop_short.source_locator_probe.slot_search_completed &&
          !source_parent_hop_short.source_locator_probe
               .component_find_completed &&
          source_parent_hop_short.counters.source_miniball_build_count == 0U &&
          source_parent_hop_short.counters.top_k_query_count == 0U &&
          !source_parent_hop_short.resolved_component_handle.has_value(),
      "a parented source hit with zero parent hops exhausts before geometry");

  const auto generous = build_and_verify(
      index,
      cloud,
      source,
      level(33, 2),
      query_witness,
      empty_locator,
      generous_step_budget(),
      LbvhTraversalOrder::near_first,
      "the generous top-k budget baseline");
  const ExactLbvhTopKBudget exact = exact_top_k_budget(generous.top_k_audit);
  check(
      exact.maximum_node_visit_count != 0U &&
          exact.maximum_internal_node_expansion_count != 0U &&
          exact.maximum_exact_aabb_bound_evaluation_count != 0U &&
          exact.maximum_exact_point_distance_evaluation_count != 0U &&
          exact.maximum_frontier_entry_count != 0U &&
          exact.maximum_best_neighbor_entry_count != 0U &&
          exact.maximum_cutoff_shell_entry_count != 0U,
      "the AC fixture exercises all seven top-k budget dimensions");

  auto exact_step_budget = generous_step_budget();
  exact_step_budget.top_k_query = exact;
  const auto exact_result = build_and_verify(
      index,
      cloud,
      source,
      level(33, 2),
      query_witness,
      empty_locator,
      exact_step_budget,
      LbvhTraversalOrder::near_first,
      "the exact seven-boundary top-k budget");
  check(
      exact_result.certified_unresolved_without_isolation() &&
          exact_result.top_k_audit == generous.top_k_audit,
      "all seven exact caps reproduce the complete traversal");

  std::array<ExactLbvhTopKBudget, 7U> short_budgets{
      exact, exact, exact, exact, exact, exact, exact};
  --short_budgets[0U].maximum_node_visit_count;
  --short_budgets[1U].maximum_internal_node_expansion_count;
  --short_budgets[2U].maximum_exact_aabb_bound_evaluation_count;
  --short_budgets[3U].maximum_exact_point_distance_evaluation_count;
  --short_budgets[4U].maximum_frontier_entry_count;
  --short_budgets[5U].maximum_best_neighbor_entry_count;
  --short_budgets[6U].maximum_cutoff_shell_entry_count;
  const std::array<ExactLbvhTopKStopReason, 7U> stop_reasons{
      ExactLbvhTopKStopReason::node_visit_limit,
      ExactLbvhTopKStopReason::internal_node_expansion_limit,
      ExactLbvhTopKStopReason::exact_aabb_bound_evaluation_limit,
      ExactLbvhTopKStopReason::exact_point_distance_evaluation_limit,
      ExactLbvhTopKStopReason::frontier_entry_limit,
      ExactLbvhTopKStopReason::best_neighbor_entry_limit,
      ExactLbvhTopKStopReason::cutoff_shell_entry_limit,
  };
  const std::array<std::string_view, 7U> cap_names{
      "node visit",
      "internal expansion",
      "exact AABB",
      "exact point distance",
      "frontier",
      "best neighbor",
      "cutoff shell",
  };
  for (std::size_t cap_index = 0U; cap_index < short_budgets.size(); ++cap_index) {
    auto short_step_budget = generous_step_budget();
    short_step_budget.top_k_query = short_budgets[cap_index];
    const std::string context =
        "the one-short " + std::string{cap_names[cap_index]} + " top-k cap";
    const auto exhausted = build_and_verify(
        index,
        cloud,
        source,
        level(33, 2),
        query_witness,
        empty_locator,
        short_step_budget,
        LbvhTraversalOrder::near_first,
        context);
    check(
        exhausted.certified_budget_exhaustion() &&
            exhausted.decision == ExactDirectSparseFacetDescentStepDecision::
                                      no_resolution_top_k_budget_exhausted &&
            exhausted.top_k_stop_reason == stop_reasons[cap_index] &&
            !exhausted.complete_top_k_query_counters.has_value() &&
            !exhausted.strict_step_witness.has_value() &&
            !exhausted.successor_locator_probe.has_value() &&
            !exhausted.resolved_component_handle.has_value(),
        context + " exposes only an operational audit");
  }

  ExactDirectSparsePositiveFacetLocator target_locator = make_locator();
  seed_binding(target_locator, key({0U, 4U}), 5U, 501U);
  auto successor_short_budget = generous_step_budget();
  successor_short_budget.successor_locator_probe.maximum_slot_visit_count = 0U;
  const auto successor_short = build_and_verify(
      index,
      cloud,
      source,
      level(33, 2),
      query_witness,
      target_locator,
      successor_short_budget,
      LbvhTraversalOrder::near_first,
      "the exhausted successor probe");
  check(
      successor_short.certified_budget_exhaustion() &&
          successor_short.decision == ExactDirectSparseFacetDescentStepDecision::
                                          no_resolution_successor_locator_probe_budget_exhausted &&
          successor_short.strict_step_witness.has_value() &&
          successor_short.successor_locator_probe.has_value() &&
          successor_short.successor_locator_probe->certified_budget_exhaustion() &&
          !successor_short.resolved_component_handle.has_value(),
      "a zero-slot successor probe preserves the geometric witness but publishes no component");

  ExactDirectSparsePositiveFacetLocator parented_target_locator = make_locator();
  seed_binding(parented_target_locator, key({0U, 4U}), 5U, 504U);
  parent_component_handle(parented_target_locator, 5U, 1U, 505U);
  auto successor_parent_hop_short_budget = generous_step_budget();
  successor_parent_hop_short_budget.successor_locator_probe
      .maximum_component_parent_hop_count = 0U;
  const auto successor_parent_hop_short = build_and_verify(
      index,
      cloud,
      source,
      level(33, 2),
      query_witness,
      parented_target_locator,
      successor_parent_hop_short_budget,
      LbvhTraversalOrder::near_first,
      "the exhausted successor parent-hop probe");
  check(
      successor_parent_hop_short.certified_budget_exhaustion() &&
          successor_parent_hop_short.decision ==
              ExactDirectSparseFacetDescentStepDecision::
                  no_resolution_successor_locator_probe_budget_exhausted &&
          successor_parent_hop_short.strict_step_witness.has_value() &&
          successor_parent_hop_short.successor_locator_probe.has_value() &&
          successor_parent_hop_short.successor_locator_probe
              ->component_parent_hop_budget_exhausted &&
          successor_parent_hop_short.successor_locator_probe
              ->slot_search_completed &&
          !successor_parent_hop_short.successor_locator_probe
               ->component_find_completed &&
          !successor_parent_hop_short.resolved_component_handle.has_value() &&
          !successor_parent_hop_short.resolved_binding_witness.has_value(),
      "a parented strict target with zero parent hops keeps geometry but publishes no handle");
}

void test_fresh_verifier_rejects_mutations() {
  const CanonicalPointCloud cloud = ac_de_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::array<PointId, 2U> source{1U, 3U};
  ExactDirectSparsePositiveFacetLocator locator = make_locator();
  seed_binding(locator, key({0U, 4U}), 3U, 601U);
  const auto budget = generous_step_budget();
  const auto query_witness = witness(6001U);
  const auto original = build_and_verify(
      index,
      cloud,
      source,
      level(33, 2),
      query_witness,
      locator,
      budget,
      LbvhTraversalOrder::near_first,
      "the mutation baseline");

  const auto rejected = [&](
                            ExactDirectSparseFacetDescentStepResult mutated,
                            const std::string& context) {
    const auto verification = verify_exact_direct_sparse_facet_descent_step(
        index,
        cloud,
        source,
        level(33, 2),
        query_witness,
        locator,
        budget,
        LbvhTraversalOrder::near_first,
        mutated);
    check(
        !verification.fresh_replay_certified && !verification.result_certified,
        context + " is rejected by fresh replay");
  };

  auto bad_batch = original;
  bad_batch.closed_batch_squared_level = level(16);
  rejected(std::move(bad_batch), "a mutated observed closed batch");

  auto bad_source_level = original;
  bad_source_level.strict_step_witness->source_facet_squared_level = level(16);
  rejected(std::move(bad_source_level), "a mutated source miniball level");

  auto bad_successor_key = original;
  bad_successor_key.strict_step_witness->successor_facet_key = key({0U, 3U});
  rejected(std::move(bad_successor_key), "a mutated canonical successor key");

  auto bad_displacement = original;
  bad_displacement.strict_step_witness->center_squared_displacement = level(0);
  rejected(std::move(bad_displacement), "a mutated center displacement");

  auto bad_source_open = original;
  bad_source_open.strict_step_witness
      ->source_open_target_closed_segment_strict_below_closed_batch_level =
      false;
  rejected(std::move(bad_source_open), "a removed source-open batch fact");

  auto bad_closed_segment = original;
  bad_closed_segment.strict_step_witness
      ->closed_segment_strict_below_closed_batch_level = false;
  rejected(std::move(bad_closed_segment), "a mutated closed-segment fact");

  auto bad_audit = original;
  ++bad_audit.top_k_audit.node_visit_count;
  rejected(std::move(bad_audit), "a mutated top-k work audit");

  auto bad_handle = original;
  bad_handle.resolved_component_handle = 4U;
  rejected(std::move(bad_handle), "a mutated resolved component handle");

  auto bad_decision = original;
  bad_decision.decision = ExactDirectSparseFacetDescentStepDecision::
      complete_unresolved_strict_successor_not_bound;
  rejected(std::move(bad_decision), "a mutated scientific decision");

  const CanonicalPointCloud twin_cloud = ac_de_cloud();
  const auto twin_verification = verify_exact_direct_sparse_facet_descent_step(
      index,
      twin_cloud,
      source,
      level(33, 2),
      query_witness,
      locator,
      budget,
      LbvhTraversalOrder::near_first,
      original);
  check(
      !twin_verification.trusted_inputs_certified &&
          !twin_verification.result_certified,
      "the verifier rejects a coordinate-identical but foreign point namespace");
}

void test_contract_metadata() {
  check(
      ExactDirectSparseFacetDescentStepResult::backend == "reference_cpu" &&
          ExactDirectSparseFacetDescentStepResult::profile == "hgp_reduced" &&
          ExactDirectSparseFacetDescentStepResult::mode == "certified" &&
          ExactDirectSparseFacetDescentStepResult::refinement_status ==
              "partial_refinement" &&
          ExactDirectSparseFacetDescentStepResult::public_status ==
              "not_claimed",
      "the Phase-10.5b primitive advertises only its certified partial scope");
}

}  // namespace

int main() {
  test_contract_metadata();
  test_equal_level_ac_to_de_closed_segment();
  test_equal_level_lr_to_lp_source_open_segment();
  test_source_hit_missing_target_and_above_batch();
  test_canonical_source_and_non_strict_successor();
  test_all_budget_exhaustions();
  test_fresh_verifier_rejects_mutations();

  if (failures != 0) {
    std::cerr << failures
              << " direct sparse facet descent-step test(s) failed\n";
    return 1;
  }
  std::cout << "direct sparse facet descent-step tests passed\n";
  return 0;
}
