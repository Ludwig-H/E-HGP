#include "morsehgp3d/hierarchy/direct_sparse_facet_descent_closure.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactLbvhTopKBudget;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

using ProposalConsumptionDecision =
    ExactDirectSparseFacetDescentClosureTopKProposalConsumptionDecision;
using ProposalConsumptionResult =
    ExactDirectSparseFacetDescentClosureTopKProposalConsumptionResult;
using ProposalRecord = ExactDirectSparseFacetTopKProposalRecord;
using ProposalTranscriptBudget =
    ExactDirectSparseFacetTopKProposalTranscriptBudget;
using ProposalTranscriptMetadata =
    ExactDirectSparseFacetTopKProposalTranscriptMetadata;
using ProposalTranscriptResult =
    ExactDirectSparseFacetTopKProposalTranscriptResult;

constexpr std::uint64_t authority_id = UINT64_C(0x105c);
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
      32U,
      320U,
      32U,
      32U,
      32U,
      32U,
      32U,
      640U,
      65U,
      65U,
  };
}

[[nodiscard]] ExactDirectSparsePositiveFacetLocator make_locator(
    std::uint64_t external_authority_id = authority_id) {
  return build_exact_direct_sparse_positive_facet_locator(
      8U,
      locator_budget(),
      ExactDirectSparsePositiveFacetLocatorConfig{
          external_authority_id,
          std::numeric_limits<std::uint64_t>::max()});
}

void seed_binding(
    ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetKey& facet_key,
    ExactDirectSparseComponentHandle handle,
    std::uint64_t replay_token) {
  const std::array<ExactDirectSparseFacetBinding, 1U> bindings{{
      {0U, facet_key, handle, witness(replay_token)},
  }};
  const auto committed = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      bindings);
  check(
      committed.certified_committed_batch(),
      "the closure fixture commits its relative positive binding");
}

[[nodiscard]] ExactDirectSparseFacetDescentStepBudget generous_step_budget() {
  return {
      ExactDirectSparsePositiveFacetProbeBudget{65U, 8U},
      ExactLbvhTopKBudget{
          1024U, 1024U, 1024U, 1024U, 64U, 10U, 10U},
      ExactDirectSparsePositiveFacetProbeBudget{65U, 8U},
  };
}

[[nodiscard]] ExactDirectSparseFacetDescentClosureBudget
generous_closure_budget() {
  return {
      16U,
      16U,
      16U,
      33U,
      generous_step_budget(),
  };
}

[[nodiscard]] ProposalRecord proposal_record(
    const ExactDirectSparseFacetKey& source_facet_key,
    std::initializer_list<PointId> candidate_point_ids) {
  ProposalRecord result;
  result.source_facet_key = source_facet_key;
  result.candidate_point_count = candidate_point_ids.size();
  std::copy(
      candidate_point_ids.begin(),
      candidate_point_ids.end(),
      result.candidate_point_ids.begin());
  return result;
}

[[nodiscard]] ProposalTranscriptBudget exact_proposal_transcript_budget(
    std::span<const ProposalRecord> records) {
  std::size_t facet_key_point_reference_count = 0U;
  std::size_t candidate_point_reference_count = 0U;
  for (const ProposalRecord& record : records) {
    facet_key_point_reference_count +=
        record.source_facet_key.point_count;
    candidate_point_reference_count += record.candidate_point_count;
  }
  return {
      records.size(),
      facet_key_point_reference_count,
      candidate_point_reference_count,
      records.size() * sizeof(ProposalRecord),
      records.size() + facet_key_point_reference_count +
          candidate_point_reference_count,
  };
}

[[nodiscard]] ProposalTranscriptResult proposal_transcript(
    std::size_t source_batch_index,
    const ExactLevel& closed_batch_squared_level,
    const ExactDirectSparsePositiveFacetLocator& locator,
    std::span<const ProposalRecord> records) {
  // Keep this target isolated behind the closure public library.  The
  // transcript target tests its builder separately; these fixtures materialize
  // a complete payload so that the closure must independently revalidate it.
  ProposalTranscriptResult result;
  result.metadata = ProposalTranscriptMetadata{
      source_batch_index,
      closed_batch_squared_level,
      locator.snapshot_stamp(),
  };
  result.requested_budget = exact_proposal_transcript_budget(records);
  result.input_proposal_record_count = records.size();
  result.required_facet_key_point_reference_count =
      result.requested_budget.maximum_facet_key_point_reference_count;
  result.required_candidate_point_reference_count =
      result.requested_budget.maximum_candidate_point_reference_count;
  result.required_payload_byte_count =
      result.requested_budget.maximum_payload_byte_count;
  result.required_logical_storage_entry_count =
      result.requested_budget.maximum_logical_storage_entry_count;
  result.facet_cardinality =
      records.empty() ? 0U : records.front().source_facet_key.point_count;
  result.published_payload_byte_count =
      result.required_payload_byte_count;
  result.published_logical_storage_entry_count =
      result.required_logical_storage_entry_count;
  result.proposal_records.assign(records.begin(), records.end());
  result.metadata_shape_validated = true;
  result.budget_preflight_completed = true;
  result.budget_preflight_satisfied = true;
  result.every_full_key_validated = true;
  result.homogeneous_facet_cardinality_validated = true;
  result.records_strictly_sorted_by_full_key = true;
  result.full_keys_unique = true;
  result.candidate_counts_within_k = true;
  result.candidate_point_ids_distinct = true;
  result.unused_candidate_slots_zero = true;
  result.input_validation_atomic = true;
  result.payload_published_only_after_full_validation = true;
  result.no_partial_proposal_payload_published = true;
  result.proposal_only = true;
  result.decision =
      records.empty()
          ? ExactDirectSparseFacetTopKProposalTranscriptDecision::
                complete_empty_proposal_transcript
          : ExactDirectSparseFacetTopKProposalTranscriptDecision::
                complete_validated_proposal_transcript;
  result.scope = ExactDirectSparseFacetTopKProposalTranscriptScope::
      bounded_structurally_validated_incumbent_proposals_by_full_facet_key_only;
  return result;
}

[[nodiscard]] ProposalConsumptionResult build_proposal_closure(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::span<const ExactDirectSparseFacetKey> canonical_distinct_keys,
    std::size_t source_batch_index,
    const ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseFacetWitness& query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ProposalTranscriptResult& transcript,
    const ExactDirectSparseFacetDescentClosureBudget& budget) {
  return build_exact_direct_sparse_facet_descent_closure_from_canonical_distinct_keys_with_top_k_proposal_transcript(
      index,
      cloud,
      canonical_distinct_keys,
      source_batch_index,
      closed_batch_squared_level,
      query_witness,
      locator,
      transcript,
      budget);
}

[[nodiscard]] bool same_scientific_closure_graph(
    const ExactDirectSparseFacetDescentClosureResult& left,
    const ExactDirectSparseFacetDescentClosureResult& right) {
  return left.closed_batch_squared_level ==
             right.closed_batch_squared_level &&
         left.locator_query_witness == right.locator_query_witness &&
         left.locator_snapshot_stamp == right.locator_snapshot_stamp &&
         left.common_facet_cardinality == right.common_facet_cardinality &&
         left.nodes == right.nodes && left.edges == right.edges &&
         left.seed_projections == right.seed_projections &&
         left.contradiction_witness == right.contradiction_witness &&
         left.disposition == right.disposition &&
         left.decision == right.decision && left.scope == right.scope;
}

void check_atomic_proposal_rejection(
    const ProposalConsumptionResult& result,
    ProposalConsumptionDecision expected_decision,
    const ExactDirectSparsePositiveFacetLocatorSnapshotStamp&
        expected_locator_stamp,
    const std::string& context) {
  check(
      result.decision == expected_decision &&
          result.certified_atomic_rejection() &&
          result.certified_outcome() &&
          !result.scientific_closure.has_value() &&
          result.validation_completed_before_closure &&
          result.no_closure_constructed_on_rejection &&
          result.no_transcript_or_top_k_payload_persisted_in_closure &&
          result.consumption_audit.closure_build_count == 0U &&
          result.consumption_audit.locator_snapshot_stamp ==
              expected_locator_stamp &&
          !result.consumption_audit.transcript_payload_persisted &&
          !result.consumption_audit.top_k_partition_or_shell_persisted &&
          !result.consumption_audit.scientific_decision_taken_from_proposal &&
          !result.consumption_audit.locator_state_mutated,
      context);
}

[[nodiscard]] bool fresh_verification_closes(
    const ExactDirectSparseFacetDescentClosureVerification& verification) {
  return verification.trusted_inputs_certified &&
         verification.observed_storage_within_budget &&
         verification.locator_snapshot_matches_observed_build &&
         verification.observed_outcome_well_formed &&
         verification.seeds_freshly_canonicalized &&
         verification.memoized_graph_freshly_replayed &&
         verification.local_step_projections_freshly_replayed &&
         verification.strict_edges_and_seams_freshly_replayed &&
         verification.functional_forest_cardinality_certified &&
         verification.terminal_dispositions_freshly_propagated &&
         verification.no_duplicate_top_k_or_miniball_work_certified &&
         verification.no_locator_mutation_or_batch_commit &&
         !verification.external_binding_authority_replayed &&
         verification.no_isolation_singleton_or_attachment_invented &&
         verification.no_forbidden_global_structure_materialized &&
         verification.fresh_replay_certified &&
         verification.result_certified;
}

[[nodiscard]] ExactDirectSparseFacetDescentClosureResult build_and_verify(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::span<const ExactDirectSparseFacetDescentClosureSeed> seeds,
    const ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseFacetWitness& query_witness,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetDescentClosureBudget& budget,
    const ExactDirectSparseFacetDescentClosureConfig& config,
    LbvhTraversalOrder traversal_order,
    const std::string& context) {
  const ExactDirectSparseFacetDescentClosureResult result =
      build_exact_direct_sparse_facet_descent_closure(
          index,
          cloud,
          seeds,
          closed_batch_squared_level,
          query_witness,
          locator,
          budget,
          config,
          traversal_order);
  const ExactDirectSparseFacetDescentClosureVerification verification =
      verify_exact_direct_sparse_facet_descent_closure(
          index,
          cloud,
          seeds,
          closed_batch_squared_level,
          query_witness,
          locator,
          budget,
          config,
          traversal_order,
          result);
  check(
      fresh_verification_closes(verification),
      context + " closes under a fresh reconstruction");
  return result;
}

void check_functional_forest_shape(
    const ExactDirectSparseFacetDescentClosureResult& result,
    const std::string& context) {
  std::size_t terminal_count = 0U;
  for (const ExactDirectSparseFacetDescentNode& node : result.nodes) {
    if (!node.outgoing_edge_index.has_value()) {
      ++terminal_count;
    } else {
      check(
          *node.outgoing_edge_index < result.edges.size() &&
              result.edges[*node.outgoing_edge_index].source_node_index ==
                  node.node_index,
          context + " gives every nonterminal exactly one owned edge");
    }
  }
  check(
      terminal_count == result.counters.terminal_node_count &&
          result.edges.size() + terminal_count == result.nodes.size() &&
          (result.nodes.empty() || result.edges.size() < result.nodes.size()) &&
          result.no_half_edge_published,
      context + " satisfies E=V-T and publishes no half-edge");
}

[[nodiscard]] const ExactDirectSparseFacetDescentNode* find_node(
    const ExactDirectSparseFacetDescentClosureResult& result,
    const ExactDirectSparseFacetKey& facet_key) {
  for (const ExactDirectSparseFacetDescentNode& node : result.nodes) {
    if (node.facet_key == facet_key) {
      return &node;
    }
  }
  return nullptr;
}

[[nodiscard]] const ExactDirectSparseFacetDescentSeedProjection*
find_projection(
    const ExactDirectSparseFacetDescentClosureResult& result,
    std::size_t seed_index) {
  for (const ExactDirectSparseFacetDescentSeedProjection& projection :
       result.seed_projections) {
    if (projection.seed_index == seed_index) {
      return &projection;
    }
  }
  return nullptr;
}

[[nodiscard]] CanonicalPointCloud chain_cloud() {
  const std::array<CertifiedPoint3, 6U> input{
      point(-5.0, -6.0),
      point(-5.0, 5.0),
      point(-3.0, 3.0),
      point(3.0, 1.0),
      point(3.0, 6.0),
      point(4.0, 5.0),
  };
  return canonical_cloud(input);
}

[[nodiscard]] CanonicalPointCloud ac_de_cloud() {
  const std::array<CertifiedPoint3, 5U> input{
      point(0.0, 0.0, 7.0),
      point(0.0, 9.0, 6.0),
      point(1.0, 4.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(4.0, 1.0, 2.0),
  };
  return canonical_cloud(input);
}

struct ChainFixture {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  ExactDirectSparseFacetKey f0;
  ExactDirectSparseFacetKey f1;
  ExactDirectSparseFacetKey f2;
  ExactDirectSparsePositiveFacetLocator locator;
  ExactDirectSparseFacetDescentClosureBudget budget;
  ExactDirectSparseFacetWitness query_witness;
  std::array<ExactDirectSparseFacetDescentClosureSeed, 2U> seeds;

  ChainFixture()
      : cloud(chain_cloud()),
        index(MortonLbvhIndex::build(cloud)),
        f0(key({0U, 1U, 2U, 4U})),
        f1(key({1U, 2U, 3U, 5U})),
        f2(key({1U, 2U, 3U, 4U})),
        locator(make_locator()),
        budget(generous_closure_budget()),
        query_witness(witness(9001U)),
        seeds{{{0U, f0}, {1U, f1}}} {
    seed_binding(locator, f2, 3U, 101U);
    bool canonical_ids_preserved = cloud.size() == 6U;
    for (std::size_t point_index = 0U;
         point_index < cloud.size();
         ++point_index) {
      canonical_ids_preserved =
          canonical_ids_preserved &&
          cloud.source_index(static_cast<PointId>(point_index)) == point_index;
    }
    check(
        canonical_ids_preserved,
        "the six sorted fixture points preserve canonical ids 0 through 5");
  }
};

void test_top_k_proposal_chain_consumption_and_fallbacks() {
  ChainFixture fixture;
  constexpr std::size_t source_batch_index = 17U;
  const ExactLevel closed_level = level(52);
  const std::array<ExactDirectSparseFacetKey, 1U> canonical_keys{
      fixture.f0,
  };
  const std::array<ProposalRecord, 0U> empty_records{};
  const std::array<ProposalRecord, 1U> useful_records{
      proposal_record(fixture.f0, {3U, 5U}),
  };
  const std::array<ProposalRecord, 1U> adversarial_records{
      proposal_record(fixture.f0, {0U, 4U}),
  };

  const ProposalTranscriptResult empty_transcript = proposal_transcript(
      source_batch_index,
      closed_level,
      fixture.locator,
      empty_records);
  const ProposalTranscriptResult useful_transcript = proposal_transcript(
      source_batch_index,
      closed_level,
      fixture.locator,
      useful_records);
  const ProposalTranscriptResult adversarial_transcript =
      proposal_transcript(
          source_batch_index,
          closed_level,
          fixture.locator,
          adversarial_records);
  const ProposalConsumptionResult baseline = build_proposal_closure(
      fixture.index,
      fixture.cloud,
      canonical_keys,
      source_batch_index,
      closed_level,
      fixture.query_witness,
      fixture.locator,
      empty_transcript,
      fixture.budget);
  const ProposalConsumptionResult useful = build_proposal_closure(
      fixture.index,
      fixture.cloud,
      canonical_keys,
      source_batch_index,
      closed_level,
      fixture.query_witness,
      fixture.locator,
      useful_transcript,
      fixture.budget);
  const ProposalConsumptionResult adversarial = build_proposal_closure(
      fixture.index,
      fixture.cloud,
      canonical_keys,
      source_batch_index,
      closed_level,
      fixture.query_witness,
      fixture.locator,
      adversarial_transcript,
      fixture.budget);

  check(
      baseline.certified_exact_consumption_outcome() &&
          useful.certified_exact_consumption_outcome() &&
          adversarial.certified_exact_consumption_outcome() &&
          baseline.scientific_closure.has_value() &&
          useful.scientific_closure.has_value() &&
          adversarial.scientific_closure.has_value(),
      "complete empty, useful and adversarial 14F transcripts all produce certified exact closures");
  if (!baseline.scientific_closure.has_value() ||
      !useful.scientific_closure.has_value() ||
      !adversarial.scientific_closure.has_value()) {
    return;
  }

  check(
      same_scientific_closure_graph(
          *baseline.scientific_closure,
          *useful.scientific_closure) &&
          same_scientific_closure_graph(
              *baseline.scientific_closure,
              *adversarial.scientific_closure) &&
          baseline.scientific_closure->nodes.size() == 3U &&
          baseline.scientific_closure->edges.size() == 2U &&
          baseline.scientific_closure
              ->certified_complete_relative_positive_closure(),
      "proposal quality changes only operational work, never the exact F0-to-F1-to-F2 scientific graph");

  const auto& empty_audit = baseline.consumption_audit;
  const auto& useful_audit = useful.consumption_audit;
  const auto& adversarial_audit = adversarial.consumption_audit;
  check(
      empty_audit.canonical_seed_key_count == 1U &&
          empty_audit.transcript_record_count == 0U &&
          empty_audit.top_k_query_count == 2U &&
          empty_audit.nonempty_proposal_hit_query_count == 0U &&
          empty_audit.missing_initial_record_fallback_query_count == 1U &&
          empty_audit.dynamic_successor_fallback_query_count == 1U &&
          empty_audit.baseline_facet_point_reference_count == 8U &&
          empty_audit.proposal_point_reference_count == 0U &&
          empty_audit.union_point_reference_count == 8U &&
          empty_audit.deduplicated_point_reference_count == 0U,
      "the empty baseline accounts for two exact four-point facets and both missing-root and dynamic-successor fallbacks");
  check(
      useful_audit.transcript_record_count == 1U &&
          useful_audit.transcript_candidate_point_reference_count == 2U &&
          useful_audit.top_k_query_count == 2U &&
          useful_audit.nonempty_proposal_hit_query_count == 1U &&
          useful_audit.missing_initial_record_fallback_query_count == 0U &&
          useful_audit.explicit_empty_record_fallback_query_count == 0U &&
          useful_audit.dynamic_successor_fallback_query_count == 1U &&
          useful_audit.baseline_facet_point_reference_count == 8U &&
          useful_audit.proposal_point_reference_count == 2U &&
          useful_audit.union_point_reference_count == 10U &&
          useful_audit.deduplicated_point_reference_count == 0U,
      "the sparse useful transcript feeds only the initial F0 query while the discovered F1 successor falls back to its exact facet");
  check(
      adversarial_audit.transcript_record_count == 1U &&
          adversarial_audit.top_k_query_count == 2U &&
          adversarial_audit.nonempty_proposal_hit_query_count == 1U &&
          adversarial_audit.dynamic_successor_fallback_query_count == 1U &&
          adversarial_audit.baseline_facet_point_reference_count == 8U &&
          adversarial_audit.proposal_point_reference_count == 2U &&
          adversarial_audit.union_point_reference_count == 8U &&
          adversarial_audit.deduplicated_point_reference_count == 2U,
      "the adversarial source-only proposal is fully deduplicated from F without gaining scientific authority");

  const auto audit_has_closed_separation =
      [](const ProposalConsumptionResult& result) {
        const auto& audit = result.consumption_audit;
        return result.no_transcript_or_top_k_payload_persisted_in_closure &&
               result.scientific_closure.has_value() &&
               result.scientific_closure
                   ->no_top_k_partition_or_shell_persisted &&
               audit.transcript_complete_revalidated &&
               audit.metadata_matches_requested_batch_level_and_live_locator &&
               audit.canonical_seed_keys_revalidated &&
               audit.record_keys_are_canonical_seed_subset &&
               audit.candidate_point_domains_revalidated &&
               audit.locator_snapshot_stable_during_atomic_validation &&
               audit.every_top_k_query_used_exact_source_facet_baseline &&
               audit.nonempty_records_passed_only_as_proposals &&
               audit
                   .empty_missing_and_dynamic_fallbacks_used_empty_proposal_pool &&
               audit.exact_pool_and_spatial_work_accounted &&
               audit.exact_seed_distance_evaluation_count <=
                   audit.union_point_reference_count &&
               !audit.transcript_payload_persisted &&
               !audit.top_k_partition_or_shell_persisted &&
               !audit.scientific_decision_taken_from_proposal &&
               !audit.locator_state_mutated;
      };
  check(
      audit_has_closed_separation(baseline) &&
          audit_has_closed_separation(useful) &&
          audit_has_closed_separation(adversarial),
      "every 14F outcome separates the exact scientific closure from proposal, partition and shell payloads");
}

void test_top_k_proposal_preinterned_seed_reached_as_dynamic_successor() {
  ChainFixture fixture;
  constexpr std::size_t source_batch_index = 19U;
  const ExactLevel closed_level = level(52);
  const std::array<ExactDirectSparseFacetKey, 2U> canonical_keys{
      fixture.f0,
      fixture.f1,
  };
  const std::array<ProposalRecord, 1U> root_only_records{
      proposal_record(fixture.f0, {3U, 5U}),
  };
  const std::array<ProposalRecord, 2U> root_and_f1_records{
      proposal_record(fixture.f0, {3U, 5U}),
      proposal_record(fixture.f1, {0U, 4U}),
  };
  const ProposalTranscriptResult root_only_transcript =
      proposal_transcript(
          source_batch_index,
          closed_level,
          fixture.locator,
          root_only_records);
  const ProposalTranscriptResult root_and_f1_transcript =
      proposal_transcript(
          source_batch_index,
          closed_level,
          fixture.locator,
          root_and_f1_records);
  const ProposalConsumptionResult baseline = build_proposal_closure(
      fixture.index,
      fixture.cloud,
      canonical_keys,
      source_batch_index,
      closed_level,
      fixture.query_witness,
      fixture.locator,
      root_only_transcript,
      fixture.budget);
  const ProposalConsumptionResult with_f1_record =
      build_proposal_closure(
          fixture.index,
          fixture.cloud,
          canonical_keys,
          source_batch_index,
          closed_level,
          fixture.query_witness,
          fixture.locator,
          root_and_f1_transcript,
          fixture.budget);

  check(
      baseline.certified_exact_consumption_outcome() &&
          with_f1_record.certified_exact_consumption_outcome() &&
          baseline.scientific_closure.has_value() &&
          with_f1_record.scientific_closure.has_value(),
      "the preinterned F1 dynamic-successor regression produces two certified exact closures");
  if (!baseline.scientific_closure.has_value() ||
      !with_f1_record.scientific_closure.has_value()) {
    return;
  }

  const auto& audit = with_f1_record.consumption_audit;
  check(
      audit.transcript_record_count == 2U &&
          audit.transcript_candidate_point_reference_count == 4U &&
          audit.top_k_query_count == 2U &&
          audit.nonempty_proposal_hit_query_count == 1U &&
          audit.missing_initial_record_fallback_query_count == 0U &&
          audit.explicit_empty_record_fallback_query_count == 0U &&
          audit.dynamic_successor_fallback_query_count == 1U &&
          audit.baseline_facet_point_reference_count == 8U &&
          audit.proposal_point_reference_count == 2U &&
          audit.union_point_reference_count == 10U &&
          audit.deduplicated_point_reference_count == 0U,
      "F1 ignores its retained nonempty record when first reached dynamically from F0");
  check(
      same_scientific_closure_graph(
          *baseline.scientific_closure,
          *with_f1_record.scientific_closure) &&
          baseline.scientific_closure->nodes.size() == 3U &&
          baseline.scientific_closure->edges.size() == 2U &&
          baseline.consumption_audit
                  .dynamic_successor_fallback_query_count ==
              1U &&
          baseline.consumption_audit.proposal_point_reference_count ==
              audit.proposal_point_reference_count &&
          with_f1_record
              .no_transcript_or_top_k_payload_persisted_in_closure &&
          !audit.transcript_payload_persisted &&
          !audit.top_k_partition_or_shell_persisted &&
          !audit.scientific_decision_taken_from_proposal,
      "the unused P(F1) changes neither the exact graph nor the persisted closure payload");
}

void test_top_k_proposal_atomic_rejections() {
  ChainFixture fixture;
  constexpr std::size_t source_batch_index = 23U;
  const ExactLevel closed_level = level(52);
  const auto entry_stamp = fixture.locator.snapshot_stamp();
  const std::array<ExactDirectSparseFacetKey, 1U> canonical_keys{
      fixture.f0,
  };
  const std::array<ProposalRecord, 1U> valid_records{
      proposal_record(fixture.f0, {3U, 5U}),
  };
  const ProposalTranscriptResult valid_transcript = proposal_transcript(
      source_batch_index,
      closed_level,
      fixture.locator,
      valid_records);
  const ProposalConsumptionResult stale_batch = build_proposal_closure(
      fixture.index,
      fixture.cloud,
      canonical_keys,
      source_batch_index + 1U,
      closed_level,
      fixture.query_witness,
      fixture.locator,
      valid_transcript,
      fixture.budget);
  check_atomic_proposal_rejection(
      stale_batch,
      ProposalConsumptionDecision::no_closure_metadata_mismatch,
      entry_stamp,
      "a stale transcript batch rejects atomically before constructing a closure");

  const ProposalConsumptionResult stale_level = build_proposal_closure(
      fixture.index,
      fixture.cloud,
      canonical_keys,
      source_batch_index,
      level(51),
      fixture.query_witness,
      fixture.locator,
      valid_transcript,
      fixture.budget);
  check_atomic_proposal_rejection(
      stale_level,
      ProposalConsumptionDecision::no_closure_metadata_mismatch,
      entry_stamp,
      "a stale transcript level rejects atomically before constructing a closure");

  ProposalTranscriptResult stale_stamp_transcript = valid_transcript;
  ++stale_stamp_transcript.metadata.locator_snapshot_stamp
        .committed_batch_count;
  const ProposalConsumptionResult stale_stamp = build_proposal_closure(
      fixture.index,
      fixture.cloud,
      canonical_keys,
      source_batch_index,
      closed_level,
      fixture.query_witness,
      fixture.locator,
      stale_stamp_transcript,
      fixture.budget);
  check_atomic_proposal_rejection(
      stale_stamp,
      ProposalConsumptionDecision::no_closure_metadata_mismatch,
      entry_stamp,
      "a stale locator stamp rejects atomically before constructing a closure");
  check(
      stale_batch.consumption_audit.transcript_complete_revalidated &&
          stale_level.consumption_audit.transcript_complete_revalidated &&
          stale_stamp.consumption_audit.transcript_complete_revalidated &&
          !stale_batch.consumption_audit
               .metadata_matches_requested_batch_level_and_live_locator &&
          !stale_level.consumption_audit
               .metadata_matches_requested_batch_level_and_live_locator &&
          !stale_stamp.consumption_audit
               .metadata_matches_requested_batch_level_and_live_locator,
      "batch, level and locator-stamp freshness are checked only after full transcript revalidation");

  const std::array<ProposalRecord, 1U> outside_seed_records{
      proposal_record(fixture.f1, {0U}),
  };
  const ProposalTranscriptResult outside_seed_transcript =
      proposal_transcript(
          source_batch_index,
          closed_level,
          fixture.locator,
          outside_seed_records);
  const ProposalConsumptionResult outside_seed = build_proposal_closure(
      fixture.index,
      fixture.cloud,
      canonical_keys,
      source_batch_index,
      closed_level,
      fixture.query_witness,
      fixture.locator,
      outside_seed_transcript,
      fixture.budget);
  check_atomic_proposal_rejection(
      outside_seed,
      ProposalConsumptionDecision::
          no_closure_record_key_not_in_seed_domain,
      entry_stamp,
      "a proposal record outside the canonical initial seed domain rejects atomically");
  check(
      outside_seed.consumption_audit.canonical_seed_keys_revalidated &&
          !outside_seed.consumption_audit
               .record_keys_are_canonical_seed_subset,
      "the outside-record rejection identifies the exact full-key subset boundary");

  const PointId outside_cloud =
      static_cast<PointId>(fixture.cloud.size());
  const std::array<ProposalRecord, 1U> outside_domain_records{
      proposal_record(fixture.f0, {outside_cloud}),
  };
  const ProposalTranscriptResult outside_domain_transcript =
      proposal_transcript(
          source_batch_index,
          closed_level,
          fixture.locator,
          outside_domain_records);
  const ProposalConsumptionResult outside_domain = build_proposal_closure(
      fixture.index,
      fixture.cloud,
      canonical_keys,
      source_batch_index,
      closed_level,
      fixture.query_witness,
      fixture.locator,
      outside_domain_transcript,
      fixture.budget);
  check_atomic_proposal_rejection(
      outside_domain,
      ProposalConsumptionDecision::
          no_closure_candidate_point_domain_rejected,
      entry_stamp,
      "an out-of-cloud proposed candidate rejects atomically");
  check(
      outside_domain.consumption_audit
              .record_keys_are_canonical_seed_subset &&
          !outside_domain.consumption_audit
               .candidate_point_domains_revalidated,
      "candidate-domain rejection occurs after exact record-key membership");

  ProposalTranscriptResult incomplete_transcript = valid_transcript;
  incomplete_transcript.proposal_records.clear();
  incomplete_transcript.published_payload_byte_count = 0U;
  incomplete_transcript.published_logical_storage_entry_count = 0U;
  incomplete_transcript.payload_published_only_after_full_validation = false;
  incomplete_transcript.decision =
      ExactDirectSparseFacetTopKProposalTranscriptDecision::
          no_transcript_budget_exhausted;
  const ProposalConsumptionResult incomplete = build_proposal_closure(
      fixture.index,
      fixture.cloud,
      canonical_keys,
      source_batch_index,
      closed_level,
      fixture.query_witness,
      fixture.locator,
      incomplete_transcript,
      fixture.budget);
  check_atomic_proposal_rejection(
      incomplete,
      ProposalConsumptionDecision::no_closure_transcript_not_complete,
      entry_stamp,
      "an incomplete bounded transcript rejects atomically");

  ProposalTranscriptResult mutated_transcript = valid_transcript;
  --mutated_transcript.published_payload_byte_count;
  const ProposalConsumptionResult mutated = build_proposal_closure(
      fixture.index,
      fixture.cloud,
      canonical_keys,
      source_batch_index,
      closed_level,
      fixture.query_witness,
      fixture.locator,
      mutated_transcript,
      fixture.budget);
  check_atomic_proposal_rejection(
      mutated,
      ProposalConsumptionDecision::no_closure_transcript_not_complete,
      entry_stamp,
      "a post-validation transcript payload mutation rejects atomically");

  ProposalTranscriptResult narrowed_record_cap = valid_transcript;
  narrowed_record_cap.requested_budget.maximum_proposal_record_count = 0U;
  const ProposalConsumptionResult narrowed_cap = build_proposal_closure(
      fixture.index,
      fixture.cloud,
      canonical_keys,
      source_batch_index,
      closed_level,
      fixture.query_witness,
      fixture.locator,
      narrowed_record_cap,
      fixture.budget);
  check_atomic_proposal_rejection(
      narrowed_cap,
      ProposalConsumptionDecision::no_closure_transcript_not_complete,
      entry_stamp,
      "a narrowed retained record cap rejects before transcript replay");
  check(
      !incomplete.consumption_audit.transcript_complete_revalidated &&
          !mutated.consumption_audit.transcript_complete_revalidated &&
          !narrowed_cap.consumption_audit
               .transcript_complete_revalidated &&
          fixture.locator.snapshot_stamp() == entry_stamp,
      "incomplete, mutated and narrowed-cap transcripts publish no graph "
      "and leave the locator frozen");
}

void test_chain_and_shared_seed_suffix() {
  ChainFixture fixture;
  const auto result = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.seeds,
      level(52),
      fixture.query_witness,
      fixture.locator,
      fixture.budget,
      {},
      LbvhTraversalOrder::near_first,
      "the F0-to-F1-to-F2 closure");
  const std::array<ExactDirectSparseFacetKey, 2U> canonical_distinct_keys{
      fixture.f0,
      fixture.f1,
  };
  const auto canonical_view_result =
      build_exact_direct_sparse_facet_descent_closure_from_canonical_distinct_keys(
          fixture.index,
          fixture.cloud,
          canonical_distinct_keys,
          level(52),
          fixture.query_witness,
          fixture.locator,
          fixture.budget,
          {},
          LbvhTraversalOrder::near_first);
  check(
      canonical_view_result == result &&
          canonical_view_result.counters.strict_edge_count == 2U &&
          canonical_view_result.edges.size() == 2U,
      "the allocation-lean canonical distinct-key view exactly matches the general seed API on a nonzero strict chain");

  const auto canonical_view_rejected =
      [&fixture](std::span<const ExactDirectSparseFacetKey> keys) {
        try {
          static_cast<void>(
              build_exact_direct_sparse_facet_descent_closure_from_canonical_distinct_keys(
                  fixture.index,
                  fixture.cloud,
                  keys,
                  level(52),
                  fixture.query_witness,
                  fixture.locator,
                  fixture.budget));
        } catch (const std::invalid_argument&) {
          return true;
        }
        return false;
      };
  const std::array<ExactDirectSparseFacetKey, 2U> reversed_keys{
      fixture.f1,
      fixture.f0,
  };
  const std::array<ExactDirectSparseFacetKey, 2U> duplicate_keys{
      fixture.f0,
      fixture.f0,
  };
  const std::array<ExactDirectSparseFacetKey, 2U> mixed_cardinality_keys{
      key({0U, 1U}),
      fixture.f0,
  };
  ExactDirectSparseFacetKey invalid_key = fixture.f0;
  invalid_key.point_ids.back() = 7U;
  const std::array<ExactDirectSparseFacetKey, 1U> invalid_keys{invalid_key};
  check(
      canonical_view_rejected(reversed_keys) &&
          canonical_view_rejected(duplicate_keys) &&
          canonical_view_rejected(mixed_cardinality_keys) &&
          canonical_view_rejected(invalid_keys),
      "the canonical distinct-key entry rejects nonincreasing, duplicate, mixed-cardinality and invalid views before closure execution");

  const auto general_far = build_exact_direct_sparse_facet_descent_closure(
      fixture.index,
      fixture.cloud,
      fixture.seeds,
      level(52),
      fixture.query_witness,
      fixture.locator,
      fixture.budget,
      {},
      LbvhTraversalOrder::far_first);
  const auto canonical_view_far =
      build_exact_direct_sparse_facet_descent_closure_from_canonical_distinct_keys(
          fixture.index,
          fixture.cloud,
          canonical_distinct_keys,
          level(52),
          fixture.query_witness,
          fixture.locator,
          fixture.budget,
          {},
          LbvhTraversalOrder::far_first);
  check(
      canonical_view_far == general_far &&
          canonical_view_far.counters.strict_edge_count == 2U,
      "the canonical distinct-key view also exactly matches the general API under far-first traversal");
  const ExactDirectSparseFacetDescentClosureConfig collision_config{0U};
  const auto general_collision =
      build_exact_direct_sparse_facet_descent_closure(
          fixture.index,
          fixture.cloud,
          fixture.seeds,
          level(52),
          fixture.query_witness,
          fixture.locator,
          fixture.budget,
          collision_config);
  const auto canonical_view_collision =
      build_exact_direct_sparse_facet_descent_closure_from_canonical_distinct_keys(
          fixture.index,
          fixture.cloud,
          canonical_distinct_keys,
          level(52),
          fixture.query_witness,
          fixture.locator,
          fixture.budget,
          collision_config);
  check(
      canonical_view_collision == general_collision &&
          canonical_view_collision.counters
                  .equal_fingerprint_distinct_key_count !=
              0U,
      "the canonical distinct-key view preserves full-key authority under forced fingerprint collisions");

  check(
      result.certified_complete_relative_positive_closure() &&
          result.decision == ExactDirectSparseFacetDescentClosureDecision::
                                 complete_all_seeds_relative_positive &&
          result.common_locator_snapshot_certified &&
          result.edge_node_terminal_identity_certified &&
          result.counters.interned_node_count == 3U &&
          result.counters.strict_edge_count == 2U &&
          result.counters.terminal_node_count == 1U &&
          result.counters.evaluated_step_source_count == 2U &&
          result.counters.aggregate_step_counters.top_k_query_count == 2U,
      "the canonical chain has V=3, E=2, T=1, B=2 and Q=2");
  check(
      result.counters.source_miniball_build_count == 1U &&
          result.counters.source_miniball_reuse_count == 1U &&
          result.counters.successor_miniball_build_count == 2U &&
          result.counters.successor_miniball_reuse_count == 0U &&
          result.counters.source_miniball_build_count +
                  result.counters.successor_miniball_build_count ==
              3U &&
          result.cached_miniballs_reused_at_exact_seams,
      "the chain constructs only F0, F1 and F2 miniballs and reuses F1 as a source");
  check(
      result.counters.source_positive_hit_count == 0U &&
          result.counters.successor_positive_hit_count == 1U &&
          result.counters.relative_positive_terminal_count == 1U &&
          result.counters.memoized_seed_reuse_count == 1U,
      "only the interned F2 sink is positive and the F1 seed reuses its existing suffix");
  check_functional_forest_shape(result, "the canonical chain");

  const auto* const node_f0 = find_node(result, fixture.f0);
  const auto* const node_f1 = find_node(result, fixture.f1);
  const auto* const node_f2 = find_node(result, fixture.f2);
  check(
      node_f0 != nullptr && node_f1 != nullptr && node_f2 != nullptr,
      "all three complete facet keys are interned");
  if (node_f0 == nullptr || node_f1 == nullptr || node_f2 == nullptr) {
    return;
  }

  check(
      node_f0->exact_squared_level == level(52) &&
          node_f1->exact_squared_level == level(85, 4) &&
          node_f2->exact_squared_level == level(325, 16),
      "the interned F0, F1 and F2 levels are exactly 52, 85/4 and 325/16");
  check(
      node_f0->outgoing_edge_index.has_value() &&
          node_f1->outgoing_edge_index.has_value() &&
          !node_f2->outgoing_edge_index.has_value() &&
          result.edges[*node_f0->outgoing_edge_index].target_node_index ==
              node_f1->node_index &&
          result.edges[*node_f1->outgoing_edge_index].target_node_index ==
              node_f2->node_index,
      "the only published arcs are F0-to-F1 and F1-to-F2");
  check(
      node_f0->local_step_decision ==
              ExactDirectSparseFacetDescentStepDecision::
                  complete_unresolved_strict_successor_not_bound &&
          node_f1->local_step_decision ==
              ExactDirectSparseFacetDescentStepDecision::
                  complete_relative_strict_successor_positive_hit &&
          !node_f2->step_evaluated &&
          node_f2->kind ==
              ExactDirectSparseFacetDescentNodeKind::positive_locator_terminal &&
          node_f2->resolved_component_handle ==
              std::optional<ExactDirectSparseComponentHandle>{3U},
      "only the two complete strict 10.5b decisions create arcs and the positive target is not reevaluated");
  check(
      node_f0->terminal_node_index == node_f2->node_index &&
          node_f1->terminal_node_index == node_f2->node_index &&
          node_f2->terminal_node_index == node_f2->node_index,
      "both seeds share the unique positive F2 terminal");

  const auto* const seed_f0 = find_projection(result, 0U);
  const auto* const seed_f1 = find_projection(result, 1U);
  const bool shared_suffix =
      seed_f0 != nullptr && seed_f1 != nullptr &&
      seed_f0->terminal_node_index == seed_f1->terminal_node_index;
  check(
      seed_f0 != nullptr && seed_f1 != nullptr &&
          seed_f0->root_node_index == node_f0->node_index &&
          seed_f1->root_node_index == node_f1->node_index &&
          seed_f0->terminal_node_index == node_f2->node_index &&
          seed_f1->terminal_node_index == node_f2->node_index && shared_suffix &&
          !seed_f0->reused_existing_node && seed_f1->reused_existing_node,
      "the F1 seed projection shares the suffix already built from F0");
}

void test_five_point_ac_to_de_relative_positive_closure() {
  const CanonicalPointCloud cloud = ac_de_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSparseFacetKey ac = key({1U, 3U});
  const ExactDirectSparseFacetKey de = key({0U, 4U});
  ExactDirectSparsePositiveFacetLocator locator = make_locator();
  seed_binding(locator, de, 3U, 101U);
  const std::array<ExactDirectSparseFacetDescentClosureSeed, 1U> seeds{{
      {0U, ac},
  }};
  const auto result = build_and_verify(
      index,
      cloud,
      seeds,
      level(33, 2),
      witness(9201U),
      locator,
      generous_closure_budget(),
      {},
      LbvhTraversalOrder::near_first,
      "the five-point AC-to-DE relative-positive closure");

  check(
      result.certified_complete_relative_positive_closure() &&
          result.nodes.size() == 2U && result.edges.size() == 1U &&
          result.counters.terminal_node_count == 1U &&
          result.counters.evaluated_step_source_count == 1U &&
          result.counters.source_positive_hit_count == 0U &&
          result.counters.successor_positive_hit_count == 1U &&
          result.counters.aggregate_step_counters.top_k_query_count == 1U &&
          result.counters.source_miniball_build_count == 1U &&
          result.counters.successor_miniball_build_count == 1U,
      "AC reaches the pre-bound DE sink in one strict local step and no terminal reevaluation");
  check_functional_forest_shape(result, "the five-point AC-to-DE closure");

  const auto* const ac_node = find_node(result, ac);
  const auto* const de_node = find_node(result, de);
  const auto* const projection = find_projection(result, 0U);
  check(
      ac_node != nullptr && de_node != nullptr && projection != nullptr,
      "AC, DE and the unique seed projection are all interned");
  if (ac_node == nullptr || de_node == nullptr || projection == nullptr) {
    return;
  }

  check(
      ac_node->step_evaluated && ac_node->exact_squared_level == level(33, 2) &&
          ac_node->local_step_decision ==
              ExactDirectSparseFacetDescentStepDecision::
                  complete_relative_strict_successor_positive_hit &&
          ac_node->outgoing_edge_index.has_value() &&
          !de_node->step_evaluated &&
          de_node->kind ==
              ExactDirectSparseFacetDescentNodeKind::positive_locator_terminal &&
          de_node->exact_squared_level == level(9, 2) &&
          de_node->resolved_component_handle ==
              std::optional<ExactDirectSparseComponentHandle>{3U} &&
          de_node->resolved_binding_witness ==
              std::optional<ExactDirectSparseFacetWitness>{witness(101U)} &&
          projection->root_node_index == ac_node->node_index &&
          projection->terminal_node_index == de_node->node_index,
      "the exact AC level, DE level and caller-owned positive binding survive closure");

  if (!ac_node->outgoing_edge_index.has_value()) {
    return;
  }
  const ExactDirectSparseFacetDescentEdge& edge =
      result.edges[*ac_node->outgoing_edge_index];
  check(
      edge.source_node_index == ac_node->node_index &&
          edge.target_node_index == de_node->node_index &&
          edge.source_and_target_keys_match_nodes &&
          edge.target_center_and_level_match_node &&
          edge.strict_level_decrease_certified &&
          edge.same_closed_batch_level_certified &&
          edge.strict_step_witness.source_facet_squared_level ==
              level(33, 2) &&
          edge.strict_step_witness.top_k_cutoff_squared_level ==
              level(31, 2) &&
          edge.strict_step_witness.successor_facet_squared_level ==
              level(9, 2),
      "the AC-to-DE edge retains the exact 33/2, 31/2 and 9/2 descent seam");
}

void test_top_k_proposal_ac_to_de_exact_carrier_invariance() {
  const CanonicalPointCloud cloud = ac_de_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSparseFacetKey ac = key({1U, 3U});
  const ExactDirectSparseFacetKey de = key({0U, 4U});
  ExactDirectSparsePositiveFacetLocator locator = make_locator();
  seed_binding(locator, de, 3U, 101U);
  constexpr std::size_t source_batch_index = 29U;
  const ExactLevel closed_level = level(33, 2);
  const ExactDirectSparseFacetWitness query_witness = witness(9251U);
  const std::array<ExactDirectSparseFacetKey, 1U> canonical_keys{ac};
  const std::array<ProposalRecord, 1U> exact_de_records{
      proposal_record(ac, {0U, 4U}),
  };
  const std::array<ProposalRecord, 1U> adversarial_records{
      proposal_record(ac, {2U}),
  };
  const ProposalTranscriptResult exact_de_transcript =
      proposal_transcript(
          source_batch_index,
          closed_level,
          locator,
          exact_de_records);
  const ProposalTranscriptResult adversarial_transcript =
      proposal_transcript(
          source_batch_index,
          closed_level,
          locator,
          adversarial_records);
  const ProposalConsumptionResult exact_de = build_proposal_closure(
      index,
      cloud,
      canonical_keys,
      source_batch_index,
      closed_level,
      query_witness,
      locator,
      exact_de_transcript,
      generous_closure_budget());
  const ProposalConsumptionResult adversarial = build_proposal_closure(
      index,
      cloud,
      canonical_keys,
      source_batch_index,
      closed_level,
      query_witness,
      locator,
      adversarial_transcript,
      generous_closure_budget());

  check(
      exact_de.certified_exact_consumption_outcome() &&
          adversarial.certified_exact_consumption_outcome() &&
          exact_de.scientific_closure.has_value() &&
          adversarial.scientific_closure.has_value(),
      "both the exact DE proposal and a valid adversarial proposal produce certified AC closures");
  if (!exact_de.scientific_closure.has_value() ||
      !adversarial.scientific_closure.has_value()) {
    return;
  }
  check(
      same_scientific_closure_graph(
          *exact_de.scientific_closure,
          *adversarial.scientific_closure) &&
          exact_de.scientific_closure->nodes.size() == 2U &&
          exact_de.scientific_closure->edges.size() == 1U &&
          exact_de.scientific_closure
              ->certified_complete_relative_positive_closure(),
      "good P={DE} and adversarial P retain the identical exact AC-to-DE scientific edge");

  const auto* const exact_de_carrier =
      find_node(*exact_de.scientific_closure, de);
  const auto* const adversarial_carrier =
      find_node(*adversarial.scientific_closure, de);
  check(
      exact_de_carrier != nullptr &&
          adversarial_carrier != nullptr &&
          exact_de_carrier->kind ==
              ExactDirectSparseFacetDescentNodeKind::
                  positive_locator_terminal &&
          exact_de_carrier->resolved_component_handle ==
              std::optional<ExactDirectSparseComponentHandle>{3U} &&
          exact_de_carrier->resolved_binding_witness ==
              std::optional<ExactDirectSparseFacetWitness>{witness(101U)} &&
          *exact_de_carrier == *adversarial_carrier,
      "proposal quality cannot change the exact DE carrier or its caller-owned positive binding");

  const auto& exact_de_audit = exact_de.consumption_audit;
  const auto& adversarial_audit = adversarial.consumption_audit;
  check(
      exact_de_audit.top_k_query_count == 1U &&
          exact_de_audit.nonempty_proposal_hit_query_count == 1U &&
          exact_de_audit.baseline_facet_point_reference_count == 2U &&
          exact_de_audit.proposal_point_reference_count == 2U &&
          exact_de_audit.union_point_reference_count == 4U &&
          exact_de_audit.deduplicated_point_reference_count == 0U &&
          adversarial_audit.top_k_query_count == 1U &&
          adversarial_audit.nonempty_proposal_hit_query_count == 1U &&
          adversarial_audit.baseline_facet_point_reference_count == 2U &&
          adversarial_audit.proposal_point_reference_count == 1U &&
          adversarial_audit.union_point_reference_count == 3U &&
          adversarial_audit.deduplicated_point_reference_count == 0U,
      "AC audits exact F union P work separately for the good DE and wrong-point proposals");
  check(
      exact_de.no_transcript_or_top_k_payload_persisted_in_closure &&
          adversarial.no_transcript_or_top_k_payload_persisted_in_closure &&
          !exact_de_audit.transcript_payload_persisted &&
          !adversarial_audit.transcript_payload_persisted &&
          !exact_de_audit.top_k_partition_or_shell_persisted &&
          !adversarial_audit.top_k_partition_or_shell_persisted &&
          !exact_de_audit.scientific_decision_taken_from_proposal &&
          !adversarial_audit.scientific_decision_taken_from_proposal,
      "neither AC proposal survives nor becomes a scientific decision");
}

void test_order_traversal_collisions_and_duplicates() {
  ChainFixture fixture;
  const auto near = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.seeds,
      level(52),
      fixture.query_witness,
      fixture.locator,
      fixture.budget,
      {},
      LbvhTraversalOrder::near_first,
      "the near-first canonical baseline");

  const std::array<ExactDirectSparseFacetDescentClosureSeed, 2U> permuted{{
      {1U, fixture.f1},
      {0U, fixture.f0},
  }};
  const auto permuted_result = build_and_verify(
      fixture.index,
      fixture.cloud,
      permuted,
      level(52),
      fixture.query_witness,
      fixture.locator,
      fixture.budget,
      {},
      LbvhTraversalOrder::near_first,
      "the permuted seed records");
  check(
      permuted_result == near,
      "durable seed indices make the complete result invariant to record order");

  const auto far = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.seeds,
      level(52),
      fixture.query_witness,
      fixture.locator,
      fixture.budget,
      {},
      LbvhTraversalOrder::far_first,
      "the far-first traversal");
  check(
      far.nodes == near.nodes && far.edges == near.edges &&
          far.seed_projections == near.seed_projections &&
          far.disposition == near.disposition &&
          far.decision == near.decision &&
          far.counters.interned_node_count == 3U &&
          far.counters.strict_edge_count == 2U &&
          far.counters.terminal_node_count == 1U &&
          far.counters.evaluated_step_source_count == 2U &&
          far.counters.aggregate_step_counters.top_k_query_count == 2U,
      "near-first and far-first preserve the canonical scientific forest");

  ExactDirectSparseFacetDescentClosureConfig collision_config;
  collision_config.memo_fingerprint_mask = 0U;
  const auto collisions = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.seeds,
      level(52),
      fixture.query_witness,
      fixture.locator,
      fixture.budget,
      collision_config,
      LbvhTraversalOrder::near_first,
      "the zero-mask fingerprint collision run");
  check(
      collisions.nodes == near.nodes && collisions.edges == near.edges &&
          collisions.seed_projections == near.seed_projections &&
          collisions.counters.equal_fingerprint_distinct_key_count != 0U &&
          collisions.counters.memo_full_key_comparison_count != 0U &&
          collisions.every_memo_fingerprint_candidate_compared_by_full_key,
      "a zero fingerprint mask forces collisions without changing full-key identity");

  const std::array<ExactDirectSparseFacetDescentClosureSeed, 3U> duplicates{{
      {0U, fixture.f0},
      {1U, fixture.f1},
      {2U, fixture.f1},
  }};
  const auto duplicate_result = build_and_verify(
      fixture.index,
      fixture.cloud,
      duplicates,
      level(52),
      fixture.query_witness,
      fixture.locator,
      fixture.budget,
      {},
      LbvhTraversalOrder::near_first,
      "the duplicate F1 seed references");
  const auto* const first_f1 = find_projection(duplicate_result, 1U);
  const auto* const second_f1 = find_projection(duplicate_result, 2U);
  check(
      duplicate_result.counters.input_seed_reference_count == 3U &&
          duplicate_result.counters.processed_seed_reference_count == 3U &&
          duplicate_result.counters.distinct_seed_key_count == 2U &&
          duplicate_result.counters.duplicate_seed_key_reference_count == 1U &&
          duplicate_result.counters.interned_node_count == 3U &&
          duplicate_result.counters.evaluated_step_source_count == 2U &&
          duplicate_result.counters.memoized_seed_reuse_count == 2U &&
          first_f1 != nullptr && second_f1 != nullptr &&
          first_f1->root_node_index == second_f1->root_node_index &&
          first_f1->terminal_node_index == second_f1->terminal_node_index &&
          first_f1->reused_existing_node &&
          second_f1->reused_existing_node,
      "duplicate seed references remain distinct while sharing one F1 node and suffix");
}

void test_source_positive_and_non_strict_terminals() {
  {
    ChainFixture fixture;
    ExactDirectSparsePositiveFacetLocator source_locator = make_locator();
    seed_binding(source_locator, fixture.f0, 4U, 201U);
    auto source_hit_budget = fixture.budget;
    source_hit_budget.step_budget.top_k_query = ExactLbvhTopKBudget{};
    source_hit_budget.step_budget.successor_locator_probe =
        ExactDirectSparsePositiveFacetProbeBudget{};
    const std::array<ExactDirectSparseFacetDescentClosureSeed, 1U> seeds{{
        {0U, fixture.f0},
    }};
    const auto result = build_and_verify(
        fixture.index,
        fixture.cloud,
        seeds,
        level(52),
        fixture.query_witness,
        source_locator,
        source_hit_budget,
        {},
        LbvhTraversalOrder::near_first,
        "the direct positive source terminal");
    check(
        result.certified_complete_relative_positive_closure() &&
            result.nodes.size() == 1U && result.edges.empty() &&
            result.counters.terminal_node_count == 1U &&
            result.counters.evaluated_step_source_count == 1U &&
            result.counters.source_positive_hit_count == 1U &&
            result.counters.aggregate_step_counters.top_k_query_count == 0U &&
            result.counters.source_miniball_build_count == 0U &&
            result.nodes[0U].local_step_decision ==
                ExactDirectSparseFacetDescentStepDecision::
                    complete_relative_source_positive_hit &&
            result.nodes[0U].resolved_component_handle ==
                std::optional<ExactDirectSparseComponentHandle>{4U},
        "a positive source performs one locator probe and no geometry or arc");
    check_functional_forest_shape(result, "the positive source closure");
  }

  {
    const std::array<CertifiedPoint3, 2U> input{
        point(-1.0), point(1.0)};
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
    const ExactDirectSparsePositiveFacetLocator locator = make_locator();
    const std::array<ExactDirectSparseFacetDescentClosureSeed, 1U> seeds{{
        {0U, key({0U, 1U})},
    }};
    const auto result = build_and_verify(
        index,
        cloud,
        seeds,
        level(1),
        witness(9101U),
        locator,
        generous_closure_budget(),
        {},
        LbvhTraversalOrder::near_first,
        "the canonical fixed-point plateau terminal");
    check(
        result.certified_complete_with_unresolved_terminals() &&
            result.nodes.size() == 1U && result.edges.empty() &&
            result.counters.terminal_node_count == 1U &&
            result.nodes[0U].local_step_decision ==
                ExactDirectSparseFacetDescentStepDecision::
                    complete_unresolved_source_is_canonical_top_k_choice &&
            !result.nodes[0U].diagnostic_strict_step_witness.has_value(),
        "G=F closes as unresolved without inventing a plateau edge");
    check_functional_forest_shape(result, "the fixed-point plateau closure");
  }

  {
    const std::array<CertifiedPoint3, 4U> input{
        point(-1.0, 0.0, 0.0),
        point(0.0, -1.0, 0.0),
        point(0.0, 1.0, 0.0),
        point(1.0, 0.0, 0.0),
    };
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
    const ExactDirectSparsePositiveFacetLocator locator = make_locator();
    const std::array<ExactDirectSparseFacetDescentClosureSeed, 2U> seeds{{
        {0U, key({0U, 2U, 3U})},
        {1U, key({1U, 2U, 3U})},
    }};
    const auto result = build_and_verify(
        index,
        cloud,
        seeds,
        level(1),
        witness(9102U),
        locator,
        generous_closure_budget(),
        {},
        LbvhTraversalOrder::near_first,
        "two distinct equal-level successor terminals");
    check(
        result.certified_complete_with_unresolved_terminals() &&
            result.nodes.size() == 2U && result.edges.empty() &&
            result.counters.terminal_node_count == 2U &&
            result.counters.evaluated_step_source_count == 2U &&
            result.counters.source_miniball_build_count == 2U &&
            result.counters.successor_miniball_build_count == 1U &&
            result.counters.successor_miniball_reuse_count == 1U &&
            result.counters.distinct_cached_miniball_count == 3U &&
            std::all_of(
                result.nodes.begin(),
                result.nodes.end(),
                [](const ExactDirectSparseFacetDescentNode& node) {
                  return node.local_step_decision ==
                             ExactDirectSparseFacetDescentStepDecision::
                                 complete_unresolved_non_strict_canonical_successor &&
                         !node.diagnostic_strict_step_witness.has_value();
                }),
        "two equal-level facets reuse one uninterned canonical successor miniball and create no edge");
    check_functional_forest_shape(result, "the non-strict successor closure");
  }
}

void test_source_probe_top_k_budget_and_above_batch_terminals() {
  const CanonicalPointCloud cloud = ac_de_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSparsePositiveFacetLocator locator = make_locator();
  const ExactDirectSparseFacetKey ac = key({1U, 3U});
  const std::array<ExactDirectSparseFacetDescentClosureSeed, 1U> seeds{{
      {0U, ac},
  }};

  {
    auto source_probe_budget = generous_closure_budget();
    source_probe_budget.step_budget.source_locator_probe
        .maximum_slot_visit_count = 0U;
    const auto result = build_and_verify(
        index,
        cloud,
        seeds,
        level(33, 2),
        witness(9202U),
        locator,
        source_probe_budget,
        {},
        LbvhTraversalOrder::near_first,
        "the closure-level exhausted AC source probe");
    const auto* const ac_node = find_node(result, ac);
    check(
        result.certified_budget_exhaustion() &&
            result.decision == ExactDirectSparseFacetDescentClosureDecision::
                                   certified_prefix_step_budget_exhausted &&
            result.nodes.size() == 1U && result.edges.empty() &&
            result.seed_projections.size() == 1U &&
            result.counters.terminal_node_count == 1U &&
            result.counters.budget_terminal_count == 1U &&
            result.counters.evaluated_step_source_count == 1U &&
            result.counters.aggregate_step_counters
                    .source_locator_probe_count == 1U &&
            result.counters.aggregate_step_counters.top_k_query_count == 0U &&
            result.counters.source_miniball_build_count == 0U &&
            result.counters.successor_miniball_build_count == 0U &&
            result.counters.diagnostic_strict_witness_without_edge_count ==
                0U &&
            ac_node != nullptr && ac_node->step_evaluated &&
            ac_node->local_step_decision ==
                ExactDirectSparseFacetDescentStepDecision::
                    no_resolution_source_locator_probe_budget_exhausted &&
            !ac_node->diagnostic_strict_step_witness.has_value(),
        "a zero-slot source probe publishes one budget terminal, no geometry and no half-edge");
    check_functional_forest_shape(result, "the exhausted source-probe closure");
  }

  {
    auto top_k_budget = generous_closure_budget();
    top_k_budget.step_budget.top_k_query.maximum_node_visit_count = 0U;
    const auto result = build_and_verify(
        index,
        cloud,
        seeds,
        level(33, 2),
        witness(9203U),
        locator,
        top_k_budget,
        {},
        LbvhTraversalOrder::near_first,
        "the closure-level exhausted AC top-k query");
    const auto* const ac_node = find_node(result, ac);
    check(
        result.certified_budget_exhaustion() &&
            result.decision == ExactDirectSparseFacetDescentClosureDecision::
                                   certified_prefix_step_budget_exhausted &&
            result.nodes.size() == 1U && result.edges.empty() &&
            result.seed_projections.size() == 1U &&
            result.counters.terminal_node_count == 1U &&
            result.counters.budget_terminal_count == 1U &&
            result.counters.evaluated_step_source_count == 1U &&
            result.counters.aggregate_step_counters
                    .source_locator_probe_count == 1U &&
            result.counters.aggregate_step_counters.top_k_query_count == 1U &&
            result.counters.aggregate_step_counters
                    .successor_locator_probe_count == 0U &&
            result.counters.source_miniball_build_count == 1U &&
            result.counters.successor_miniball_build_count == 0U &&
            result.counters.diagnostic_strict_witness_without_edge_count ==
                0U &&
            ac_node != nullptr && ac_node->step_evaluated &&
            ac_node->exact_squared_level == level(33, 2) &&
            ac_node->local_step_decision ==
                ExactDirectSparseFacetDescentStepDecision::
                    no_resolution_top_k_budget_exhausted &&
            !ac_node->diagnostic_strict_step_witness.has_value(),
        "a zero-node top-k cap retains only the exact AC source miniball and no half-edge");
    check_functional_forest_shape(result, "the exhausted top-k closure");
  }

  {
    const auto result = build_and_verify(
        index,
        cloud,
        seeds,
        level(16),
        witness(9204U),
        locator,
        generous_closure_budget(),
        {},
        LbvhTraversalOrder::near_first,
        "the AC source above its closed batch");
    const auto* const ac_node = find_node(result, ac);
    check(
        result.certified_complete_with_unresolved_terminals() &&
            result.nodes.size() == 1U && result.edges.empty() &&
            result.counters.terminal_node_count == 1U &&
            result.counters.unresolved_terminal_count == 1U &&
            result.counters.evaluated_step_source_count == 1U &&
            result.counters.aggregate_step_counters.top_k_query_count == 0U &&
            result.counters.aggregate_step_counters
                    .successor_locator_probe_count == 0U &&
            result.counters.source_miniball_build_count == 1U &&
            result.counters.successor_miniball_build_count == 0U &&
            ac_node != nullptr && ac_node->step_evaluated &&
            ac_node->exact_squared_level == level(33, 2) &&
            ac_node->local_step_decision ==
                ExactDirectSparseFacetDescentStepDecision::
                    complete_unresolved_source_above_closed_batch_level &&
            !ac_node->diagnostic_strict_step_witness.has_value(),
        "beta(AC)=33/2 remains unresolved above the closed level 16 without a top-k query");
    check_functional_forest_shape(result, "the above-batch AC closure");
  }
}

void test_k10_boundary_without_global_facet_materialization() {
  const std::array<CertifiedPoint3, 10U> input{
      point(-5.0),
      point(-4.0),
      point(-3.0),
      point(-2.0),
      point(-1.0),
      point(0.0),
      point(1.0),
      point(2.0),
      point(3.0),
      point(4.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSparsePositiveFacetLocator locator = make_locator();
  const std::array<ExactDirectSparseFacetDescentClosureSeed, 1U> seeds{{
      {0U, key({0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U})},
  }};
  const auto result = build_and_verify(
      index,
      cloud,
      seeds,
      level(81, 4),
      witness(9103U),
      locator,
      generous_closure_budget(),
      {},
      LbvhTraversalOrder::near_first,
      "the K=10 full-key boundary");
  check(
      result.certified_complete_with_unresolved_terminals() &&
          result.nodes.size() == 1U && result.edges.empty() &&
          result.nodes[0U].facet_key.point_count == 10U &&
          result.nodes[0U].local_step_decision ==
              ExactDirectSparseFacetDescentStepDecision::
                  complete_unresolved_source_is_canonical_top_k_choice &&
          result.counters.evaluated_step_source_count == 1U &&
          result.counters.aggregate_step_counters.top_k_query_count == 1U &&
          result.counters.source_miniball_build_count == 1U &&
          result.counters.successor_miniball_build_count == 0U,
      "one ten-id facet remains local scratch and closes without a global facet arena");
}

void test_k10_positive_sources_with_forced_memo_collisions() {
  const std::array<CertifiedPoint3, 11U> input{
      point(-5.0),
      point(-4.0),
      point(-3.0),
      point(-2.0),
      point(-1.0),
      point(0.0),
      point(1.0),
      point(2.0),
      point(3.0),
      point(4.0),
      point(5.0),
  };
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSparseFacetKey left =
      key({0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U});
  const ExactDirectSparseFacetKey right =
      key({1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U});
  ExactDirectSparsePositiveFacetLocator locator = make_locator();
  seed_binding(locator, left, 4U, 301U);
  seed_binding(locator, right, 5U, 302U);
  const std::array<ExactDirectSparseFacetDescentClosureSeed, 2U> seeds{{
      {0U, left},
      {1U, right},
  }};
  auto budget = generous_closure_budget();
  budget.step_budget.top_k_query = ExactLbvhTopKBudget{};
  budget.step_budget.successor_locator_probe =
      ExactDirectSparsePositiveFacetProbeBudget{};
  ExactDirectSparseFacetDescentClosureConfig collision_config;
  collision_config.memo_fingerprint_mask = 0U;
  const auto result = build_and_verify(
      index,
      cloud,
      seeds,
      level(25),
      witness(9205U),
      locator,
      budget,
      collision_config,
      LbvhTraversalOrder::near_first,
      "the colliding K=10 positive-source boundary");

  const auto* const left_node = find_node(result, left);
  const auto* const right_node = find_node(result, right);
  check(
      result.certified_complete_relative_positive_closure() &&
          result.nodes.size() == 2U && result.edges.empty() &&
          result.seed_projections.size() == 2U &&
          result.counters.terminal_node_count == 2U &&
          result.counters.relative_positive_terminal_count == 2U &&
          result.counters.evaluated_step_source_count == 2U &&
          result.counters.source_positive_hit_count == 2U &&
          result.counters.aggregate_step_counters.top_k_query_count == 0U &&
          result.counters.source_miniball_build_count == 0U &&
          result.counters.successor_miniball_build_count == 0U &&
          result.counters.equal_fingerprint_distinct_key_count != 0U &&
          result.counters.memo_full_key_comparison_count != 0U &&
          result.every_memo_fingerprint_candidate_compared_by_full_key &&
          left_node != nullptr && right_node != nullptr &&
          left_node->facet_key.point_count == 10U &&
          right_node->facet_key.point_count == 10U &&
          left_node->resolved_component_handle ==
              std::optional<ExactDirectSparseComponentHandle>{4U} &&
          right_node->resolved_component_handle ==
              std::optional<ExactDirectSparseComponentHandle>{5U} &&
          left_node->resolved_binding_witness ==
              std::optional<ExactDirectSparseFacetWitness>{witness(301U)} &&
          right_node->resolved_binding_witness ==
              std::optional<ExactDirectSparseFacetWitness>{witness(302U)},
      "two colliding ten-id keys remain distinct and resolve through their own full-key bindings");
  check_functional_forest_shape(result, "the colliding K=10 closure");
}

void test_all_closure_budget_boundaries_and_invalid_traversal() {
  ChainFixture fixture;

  auto preflight_budget = fixture.budget;
  preflight_budget.maximum_seed_count = 1U;
  const auto preflight = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.seeds,
      level(52),
      fixture.query_witness,
      fixture.locator,
      preflight_budget,
      {},
      LbvhTraversalOrder::near_first,
      "the seed-count preflight exhaustion");
  check(
      preflight.certified_budget_exhaustion() &&
          preflight.decision ==
              ExactDirectSparseFacetDescentClosureDecision::
                  no_closure_preflight_budget_exhausted &&
          !preflight.budget_preflight_satisfied && preflight.nodes.empty() &&
          preflight.edges.empty() && preflight.seed_projections.empty() &&
          preflight.counters.evaluated_step_source_count == 0U &&
          preflight.no_half_edge_published,
      "preflight exhaustion allocates no scientific graph and runs no local step");

  auto memo_preflight_budget = fixture.budget;
  --memo_preflight_budget.maximum_memo_slot_count;
  const auto memo_preflight = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.seeds,
      level(52),
      fixture.query_witness,
      fixture.locator,
      memo_preflight_budget,
      {},
      LbvhTraversalOrder::near_first,
      "the one-short memo-table preflight exhaustion");
  check(
      memo_preflight.certified_budget_exhaustion() &&
          memo_preflight.decision ==
              ExactDirectSparseFacetDescentClosureDecision::
                  no_closure_preflight_budget_exhausted &&
          memo_preflight.required_memo_slot_count == 33U &&
          memo_preflight.requested_budget.maximum_memo_slot_count == 32U &&
          memo_preflight.nodes.empty() && memo_preflight.edges.empty() &&
          memo_preflight.no_half_edge_published,
      "a one-short memo-table cap fails before allocating the scientific graph");

  auto seed_root_preflight_budget = fixture.budget;
  seed_root_preflight_budget.maximum_node_count = 1U;
  const auto seed_root_preflight = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.seeds,
      level(52),
      fixture.query_witness,
      fixture.locator,
      seed_root_preflight_budget,
      {},
      LbvhTraversalOrder::near_first,
      "the one-short distinct-seed-root preflight");
  check(
      seed_root_preflight.certified_budget_exhaustion() &&
          seed_root_preflight.decision ==
              ExactDirectSparseFacetDescentClosureDecision::
                  no_closure_preflight_budget_exhausted &&
          seed_root_preflight.input_shape_certified &&
          !seed_root_preflight.budget_preflight_satisfied &&
          seed_root_preflight.counters.distinct_seed_key_count == 2U &&
          seed_root_preflight.nodes.empty() &&
          seed_root_preflight.edges.empty() &&
          seed_root_preflight.counters.evaluated_step_source_count == 0U,
      "all distinct seed roots must fit before any geometry is evaluated");

  auto node_budget = fixture.budget;
  node_budget.maximum_node_count = 2U;
  const auto node_short = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.seeds,
      level(52),
      fixture.query_witness,
      fixture.locator,
      node_budget,
      {},
      LbvhTraversalOrder::near_first,
      "the one-short F2 node budget");
  check(
      node_short.certified_budget_exhaustion() &&
          node_short.decision ==
              ExactDirectSparseFacetDescentClosureDecision::
                  certified_prefix_node_budget_exhausted &&
          node_short.nodes.size() == 2U && node_short.edges.size() == 1U &&
          node_short.counters.terminal_node_count == 1U &&
          node_short.counters.evaluated_step_source_count == 2U &&
          node_short.counters.aggregate_step_counters.top_k_query_count == 2U &&
          node_short.counters.diagnostic_strict_witness_without_edge_count ==
              1U &&
          node_short.seed_projections.size() == 2U,
      "the node cap keeps F0-to-F1 and a diagnostic F1-to-F2 witness without a half-edge");
  check_functional_forest_shape(node_short, "the node-budget prefix");

  auto step_call_budget = fixture.budget;
  step_call_budget.maximum_step_call_count = 1U;
  const auto step_call_short = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.seeds,
      level(52),
      fixture.query_witness,
      fixture.locator,
      step_call_budget,
      {},
      LbvhTraversalOrder::near_first,
      "the one-call graph budget");
  check(
      step_call_short.certified_budget_exhaustion() &&
          step_call_short.decision ==
              ExactDirectSparseFacetDescentClosureDecision::
                  certified_prefix_step_call_budget_exhausted &&
          step_call_short.nodes.size() == 2U &&
          step_call_short.edges.size() == 1U &&
          step_call_short.counters.terminal_node_count == 1U &&
          step_call_short.counters.evaluated_step_source_count == 1U &&
          step_call_short.counters.aggregate_step_counters.top_k_query_count ==
              1U &&
          step_call_short.counters.budget_terminal_count == 1U,
      "the step-call cap publishes the complete F0-to-F1 edge and leaves F1 as a graph-budget terminal");
  check_functional_forest_shape(step_call_short, "the step-call prefix");

  auto local_step_budget = fixture.budget;
  local_step_budget.step_budget.successor_locator_probe
      .maximum_slot_visit_count = 0U;
  const auto local_step_short = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.seeds,
      level(52),
      fixture.query_witness,
      fixture.locator,
      local_step_budget,
      {},
      LbvhTraversalOrder::near_first,
      "the exhausted successor locator probe");
  check(
      local_step_short.certified_budget_exhaustion() &&
          local_step_short.decision ==
              ExactDirectSparseFacetDescentClosureDecision::
                  certified_prefix_step_budget_exhausted &&
          local_step_short.nodes.size() == 2U &&
          local_step_short.edges.empty() &&
          local_step_short.counters.terminal_node_count == 2U &&
          local_step_short.counters.evaluated_step_source_count == 1U &&
          local_step_short.counters.aggregate_step_counters.top_k_query_count ==
              1U &&
          local_step_short.counters.diagnostic_strict_witness_without_edge_count ==
              1U &&
          local_step_short.seed_projections.size() == 2U &&
          std::any_of(
              local_step_short.nodes.begin(),
              local_step_short.nodes.end(),
              [](const ExactDirectSparseFacetDescentNode& node) {
                return node.diagnostic_strict_step_witness.has_value();
              }),
      "a local successor-probe exhaustion keeps its strict witness diagnostic, all seed roots and no edge");
  check_functional_forest_shape(local_step_short, "the local-step prefix");

  bool invalid_traversal_rejected = false;
  try {
    static_cast<void>(build_exact_direct_sparse_facet_descent_closure(
        fixture.index,
        fixture.cloud,
        fixture.seeds,
        level(52),
        fixture.query_witness,
        fixture.locator,
        fixture.budget,
        {},
        static_cast<LbvhTraversalOrder>(255U)));
  } catch (const std::invalid_argument&) {
    invalid_traversal_rejected = true;
  }
  check(
      invalid_traversal_rejected,
      "an invalid LBVH traversal order is rejected before closure work");
}

void test_snapshot_authority_and_fresh_verifier_rejections() {
  ChainFixture fixture;
  const auto original = build_and_verify(
      fixture.index,
      fixture.cloud,
      fixture.seeds,
      level(52),
      fixture.query_witness,
      fixture.locator,
      fixture.budget,
      {},
      LbvhTraversalOrder::near_first,
      "the verifier mutation baseline");

  const auto rejected = [&](
                            ExactDirectSparseFacetDescentClosureResult mutated,
                            const std::string& context) {
    const auto verification = verify_exact_direct_sparse_facet_descent_closure(
        fixture.index,
        fixture.cloud,
        fixture.seeds,
        level(52),
        fixture.query_witness,
        fixture.locator,
        fixture.budget,
        {},
        LbvhTraversalOrder::near_first,
        mutated);
    check(
        verification.trusted_inputs_certified &&
            verification.locator_snapshot_matches_observed_build &&
            !verification.fresh_replay_certified &&
            !verification.result_certified,
        context + " is rejected by the fresh closure replay");
  };

  auto bad_edge_target = original;
  bad_edge_target.edges[0U].target_node_index =
      bad_edge_target.edges[0U].source_node_index;
  rejected(std::move(bad_edge_target), "a self-loop substituted for F0-to-F1");

  auto bad_terminal = original;
  bad_terminal.nodes[0U].terminal_node_index = 0U;
  rejected(std::move(bad_terminal), "a forged F0 terminal pointer");

  auto bad_level = original;
  bad_level.edges[0U].strict_step_witness.successor_facet_squared_level =
      level(52);
  rejected(std::move(bad_level), "a nondecreasing strict-edge witness");

  auto bad_counter = original;
  ++bad_counter.counters.aggregate_step_counters.top_k_query_count;
  rejected(std::move(bad_counter), "a forged aggregate top-k count");

  auto bad_miniball_counter = original;
  ++bad_miniball_counter.counters.source_miniball_build_count;
  check(
      !bad_miniball_counter.certified_partial_refinement_outcome(),
      "a top-level miniball count inconsistent with the aggregate fails autonomously");
  rejected(
      std::move(bad_miniball_counter),
      "a forged top-level miniball build count");

  auto bad_projection = original;
  bad_projection.seed_projections[1U].terminal_node_index =
      bad_projection.seed_projections[1U].root_node_index;
  rejected(std::move(bad_projection), "a forged shared-suffix seed projection");

  auto bad_scope = original;
  bad_scope.hierarchy_attachment_published = true;
  rejected(std::move(bad_scope), "an invented hierarchy attachment");

  auto bad_snapshot = original;
  ++bad_snapshot.locator_snapshot_stamp.committed_batch_count;
  const auto bad_snapshot_verification =
      verify_exact_direct_sparse_facet_descent_closure(
          fixture.index,
          fixture.cloud,
          fixture.seeds,
          level(52),
          fixture.query_witness,
          fixture.locator,
          fixture.budget,
          {},
          LbvhTraversalOrder::near_first,
          bad_snapshot);
  check(
      bad_snapshot_verification.trusted_inputs_certified &&
          bad_snapshot_verification.observed_storage_within_budget &&
          !bad_snapshot_verification.locator_snapshot_matches_observed_build &&
          !bad_snapshot_verification.result_certified,
      "a forged locator snapshot stamp is rejected before fresh geometry");

  auto oversized_observed = original;
  oversized_observed.edges.resize(
      fixture.budget.maximum_node_count + 1U);
  const auto oversized_verification =
      verify_exact_direct_sparse_facet_descent_closure(
          fixture.index,
          fixture.cloud,
          fixture.seeds,
          level(52),
          fixture.query_witness,
          fixture.locator,
          fixture.budget,
          {},
          LbvhTraversalOrder::near_first,
          oversized_observed);
  check(
      oversized_verification.trusted_inputs_certified &&
          !oversized_verification.observed_storage_within_budget &&
          !oversized_verification.locator_snapshot_matches_observed_build &&
          !oversized_verification.result_certified,
      "the verifier rejects over-budget observed storage before traversing it");

  const ExactDirectSparseFacetWitness foreign_witness{
      authority_id + 1U, 9001U};
  bool foreign_build_rejected = false;
  try {
    static_cast<void>(build_exact_direct_sparse_facet_descent_closure(
        fixture.index,
        fixture.cloud,
        fixture.seeds,
        level(52),
        foreign_witness,
        fixture.locator,
        fixture.budget));
  } catch (const std::invalid_argument&) {
    foreign_build_rejected = true;
  }
  const auto foreign_verification =
      verify_exact_direct_sparse_facet_descent_closure(
          fixture.index,
          fixture.cloud,
          fixture.seeds,
          level(52),
          foreign_witness,
          fixture.locator,
          fixture.budget,
          {},
          LbvhTraversalOrder::near_first,
          original);
  check(
      foreign_build_rejected &&
          !foreign_verification.trusted_inputs_certified &&
          !foreign_verification.result_certified,
      "both builder and verifier reject a mismatched external authority");

  const std::array<CertifiedPoint3, 6U> twin_input{
      point(-5.0, -6.0, 1.0),
      point(-5.0, 5.0, 1.0),
      point(-3.0, 3.0, 1.0),
      point(3.0, 1.0, 1.0),
      point(3.0, 6.0, 1.0),
      point(4.0, 5.0, 1.0),
  };
  const CanonicalPointCloud twin_cloud = canonical_cloud(twin_input);
  bool twin_build_rejected = false;
  try {
    static_cast<void>(build_exact_direct_sparse_facet_descent_closure(
        fixture.index,
        twin_cloud,
        fixture.seeds,
        level(52),
        fixture.query_witness,
        fixture.locator,
        fixture.budget));
  } catch (const std::invalid_argument&) {
    twin_build_rejected = true;
  }
  const auto twin_verification =
      verify_exact_direct_sparse_facet_descent_closure(
          fixture.index,
          twin_cloud,
          fixture.seeds,
          level(52),
          fixture.query_witness,
          fixture.locator,
          fixture.budget,
          {},
          LbvhTraversalOrder::near_first,
          original);
  check(
      twin_build_rejected &&
          !twin_verification.trusted_inputs_certified &&
          !twin_verification.result_certified,
      "a same-cardinality twin cloud cannot reuse the original LBVH authority");

  const auto empty_commit = fixture.locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      std::span<const ExactDirectSparseFacetBinding>{});
  check(
      empty_commit.certified_committed_batch() &&
          fixture.locator.snapshot_stamp() != original.locator_snapshot_stamp,
      "the snapshot rejection fixture advances the locator commit clock");
  const auto stale_snapshot_verification =
      verify_exact_direct_sparse_facet_descent_closure(
          fixture.index,
          fixture.cloud,
          fixture.seeds,
          level(52),
          fixture.query_witness,
          fixture.locator,
          fixture.budget,
          {},
          LbvhTraversalOrder::near_first,
          original);
  check(
      stale_snapshot_verification.trusted_inputs_certified &&
          !stale_snapshot_verification.locator_snapshot_matches_observed_build &&
          !stale_snapshot_verification.fresh_replay_certified &&
          !stale_snapshot_verification.result_certified,
      "a post-build locator commit invalidates the observed common snapshot");
}

void test_contract_metadata() {
  check(
      ExactDirectSparseFacetDescentClosureResult::backend == "reference_cpu" &&
          ExactDirectSparseFacetDescentClosureResult::profile ==
              "hgp_reduced" &&
          ExactDirectSparseFacetDescentClosureResult::mode == "certified" &&
          ExactDirectSparseFacetDescentClosureResult::refinement_status ==
              "partial_refinement" &&
          ExactDirectSparseFacetDescentClosureResult::public_status ==
              "not_claimed" &&
          ExactDirectSparseFacetDescentClosureResult::proof_basis ==
              direct_sparse_facet_descent_closure_proof_basis,
      "Phase 10.5c advertises only its certified partial-refinement scope");
}

}  // namespace

int main() {
  test_contract_metadata();
  test_top_k_proposal_chain_consumption_and_fallbacks();
  test_top_k_proposal_preinterned_seed_reached_as_dynamic_successor();
  test_top_k_proposal_atomic_rejections();
  test_chain_and_shared_seed_suffix();
  test_five_point_ac_to_de_relative_positive_closure();
  test_top_k_proposal_ac_to_de_exact_carrier_invariance();
  test_order_traversal_collisions_and_duplicates();
  test_source_positive_and_non_strict_terminals();
  test_source_probe_top_k_budget_and_above_batch_terminals();
  test_k10_boundary_without_global_facet_materialization();
  test_k10_positive_sources_with_forced_memo_collisions();
  test_all_closure_budget_boundaries_and_invalid_traversal();
  test_snapshot_authority_and_fresh_verifier_rejections();

  if (failures != 0) {
    std::cerr << failures
              << " direct sparse facet descent-closure test(s) failed\n";
    return 1;
  }
  std::cout << "direct sparse facet descent-closure tests passed\n";
  return 0;
}
