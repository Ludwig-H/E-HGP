#include "fake_gpu_k1_boruvka_launchers.hpp"

#include "morsehgp3d/gpu/k1_boruvka.hpp"
#include "morsehgp3d/hierarchy/k1_forest.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::gpu::K1BoruvkaCandidate;
using morsehgp3d::gpu::K1BoruvkaCandidateContext;
using morsehgp3d::gpu::K1BoruvkaChunkedRoundResolution;
using morsehgp3d::gpu::K1BoruvkaChunkingPolicy;
using morsehgp3d::gpu::K1BoruvkaEmissionStatus;
using morsehgp3d::gpu::K1BoruvkaRoundProposal;
using morsehgp3d::gpu::K1HybridBoruvkaResult;
using morsehgp3d::gpu::K1HybridBoruvkaVerification;
using morsehgp3d::gpu::K1HybridBoruvkaContractionStatus;
using morsehgp3d::gpu::K1HybridBoruvkaDecisionStatus;
using morsehgp3d::gpu::K1HybridBoruvkaEmissionMode;
using morsehgp3d::gpu::K1HybridHierarchyReductionStatus;
using morsehgp3d::gpu::K1HybridScientificStatus;
using morsehgp3d::gpu::build_gpu_proposed_cpu_exact_k1_boruvka;
using morsehgp3d::gpu::test_support::FakeK1BoruvkaConfiguration;
using morsehgp3d::gpu::test_support::FakeK1BoruvkaCorruption;
using morsehgp3d::gpu::test_support::configure_fake_gpu_k1_boruvka;
using morsehgp3d::gpu::test_support::
    fake_gpu_k1_boruvka_chunk_callback_count;
using morsehgp3d::gpu::test_support::
    fake_gpu_k1_boruvka_epoch_advance_count;
using morsehgp3d::gpu::test_support::
    fake_gpu_k1_boruvka_budget_enforcement_count;
using morsehgp3d::gpu::test_support::fake_gpu_k1_boruvka_last_node_count;
using morsehgp3d::gpu::test_support::fake_gpu_k1_boruvka_last_point_count;
using morsehgp3d::gpu::test_support::fake_gpu_k1_boruvka_launch_count;
using morsehgp3d::gpu::test_support::reset_fake_gpu_k1_boruvka;
using morsehgp3d::gpu::verify_gpu_proposed_cpu_exact_k1_boruvka;
using morsehgp3d::hierarchy::K1BoruvkaComponentMinimum;
using morsehgp3d::hierarchy::K1CompactForest;
using morsehgp3d::hierarchy::K1ExactBoruvkaResult;
using morsehgp3d::hierarchy::build_compact_k1_forest;
using morsehgp3d::hierarchy::build_exact_lbvh_boruvka;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

static_assert(!std::is_copy_constructible_v<K1BoruvkaCandidateContext>);
static_assert(!std::is_copy_assignable_v<K1BoruvkaCandidateContext>);
static_assert(
    std::is_nothrow_move_constructible_v<K1BoruvkaCandidateContext>);
static_assert(std::is_nothrow_move_assignable_v<K1BoruvkaCandidateContext>);

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Exception, typename Function>
void check_throws(Function&& function, const std::string& message) {
  try {
    std::forward<Function>(function)();
  } catch (const Exception&) {
    return;
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: " << message
              << " (unexpected exception: " << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

template <typename Function>
void check_throws_standard(Function&& function, const std::string& message) {
  try {
    std::forward<Function>(function)();
  } catch (const std::exception&) {
    return;
  } catch (...) {
    ++failures;
    std::cerr << "FAIL: " << message << " (non-standard exception)\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] CertifiedPoint3 point(
    double x, double y = 0.0, double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] std::size_t distinct_component_count(
    std::span<const PointId> labels) {
  std::vector<PointId> distinct{labels.begin(), labels.end()};
  std::sort(distinct.begin(), distinct.end());
  distinct.erase(std::unique(distinct.begin(), distinct.end()), distinct.end());
  return distinct.size();
}

[[nodiscard]] bool same_compact_forest(
    const K1CompactForest& left,
    const K1CompactForest& right) {
  return left.point_count == right.point_count &&
         left.levels == right.levels &&
         left.selected_edges == right.selected_edges &&
         left.merge_nodes == right.merge_nodes &&
         left.child_ids == right.child_ids &&
         left.equal_level_batches == right.equal_level_batches &&
         left.root_node_id == right.root_node_id &&
         left.total_squared_weight == right.total_squared_weight &&
         left.total_hgp_weight == right.total_hgp_weight &&
         left.counters == right.counters;
}

[[nodiscard]] std::span<const K1BoruvkaCandidate> candidate_segment(
    const K1BoruvkaRoundProposal& proposal, PointId source) {
  const std::size_t source_index = static_cast<std::size_t>(source);
  if (source_index + 1U >= proposal.candidate_offsets.size()) {
    return {};
  }
  const std::size_t begin = proposal.candidate_offsets[source_index];
  const std::size_t end = proposal.candidate_offsets[source_index + 1U];
  if (begin > end || end > proposal.candidates.size()) {
    return {};
  }
  return std::span<const K1BoruvkaCandidate>{proposal.candidates}.subspan(
      begin, end - begin);
}

[[nodiscard]] bool has_candidate(
    const K1BoruvkaRoundProposal& proposal,
    PointId source,
    PointId target) {
  const std::span<const K1BoruvkaCandidate> segment =
      candidate_segment(proposal, source);
  return std::any_of(
      segment.begin(), segment.end(),
      [source, target](const K1BoruvkaCandidate& candidate) {
        return candidate.source_point_id == source &&
               candidate.target_point_id == target;
      });
}

[[nodiscard]] std::size_t maximum_candidate_segment_size(
    const K1BoruvkaRoundProposal& proposal) {
  std::size_t maximum = 0U;
  for (std::size_t source_index = 0U;
       source_index + 1U < proposal.candidate_offsets.size();
       ++source_index) {
    maximum = std::max(
        maximum,
        proposal.candidate_offsets[source_index + 1U] -
            proposal.candidate_offsets[source_index]);
  }
  return maximum;
}

[[nodiscard]] PointId other_endpoint(
    const K1BoruvkaComponentMinimum& minimum) {
  return minimum.outgoing_edge.u == minimum.source_point_id
             ? minimum.outgoing_edge.v
             : minimum.outgoing_edge.u;
}

void check_normal_proposal(
    const K1BoruvkaRoundProposal& proposal,
    const MortonLbvhIndex& index,
    std::span<const PointId> labels,
    const std::vector<K1BoruvkaComponentMinimum>& expected_minima,
    std::uint64_t expected_epoch,
    const std::string& fixture) {
  const std::size_t point_count = labels.size();
  const std::size_t component_count = distinct_component_count(labels);
  check(
      proposal.frozen_component_labels ==
          std::vector<PointId>{labels.begin(), labels.end()},
      fixture + " preserves the certified frozen labels");
  check(
      proposal.candidate_offsets.size() == point_count + 1U &&
          proposal.candidate_offsets.front() == 0U &&
          proposal.candidate_offsets.back() == proposal.candidates.size() &&
          std::is_sorted(
              proposal.candidate_offsets.begin(),
              proposal.candidate_offsets.end()),
      fixture + " publishes one closed monotone CSR segment per source");

  for (std::size_t source_index = 0U;
       source_index < point_count;
       ++source_index) {
    for (const K1BoruvkaCandidate& candidate : candidate_segment(
             proposal, static_cast<PointId>(source_index))) {
      const std::size_t target_index =
          static_cast<std::size_t>(candidate.target_point_id);
      check(
          candidate.source_point_id == static_cast<PointId>(source_index) &&
              target_index < point_count && target_index != source_index &&
              labels[target_index] != labels[source_index],
          fixture + " emits only in-range outgoing candidates in its segment");
    }
  }

  check(
      proposal.cpu_exact_component_minima == expected_minima,
      fixture + " resolves the same exact kappa minima as the CPU anchor");
  for (const K1BoruvkaComponentMinimum& minimum : expected_minima) {
    check(
        has_candidate(
            proposal, minimum.source_point_id, other_endpoint(minimum)),
        fixture + " retains every CPU-required exact minimum in the GPU superset");
  }

  const auto& audit = proposal.audit;
  check(
      audit.resident_point_count == point_count &&
          audit.resident_node_count == index.build_counters().node_count &&
          audit.frozen_component_count == component_count &&
          audit.uniform_lbvh_node_count + audit.mixed_lbvh_node_count ==
              audit.resident_node_count,
      fixture + " closes resident, component and node-tag counts");
  check(
      audit.exact_seed_count == proposal.seeds.size() &&
          audit.gpu_candidate_count == proposal.candidates.size() &&
          audit.gpu_output_capacity == proposal.candidates.size() &&
          audit.cpu_exact_candidate_distance_evaluation_count ==
              proposal.candidates.size(),
      fixture + " closes seed, capacity and exact candidate counts");
  check(
      audit.gpu_kernel_launch_count == 2U &&
          audit.gpu_synchronization_count == 2U &&
          audit.gpu_count_pass_node_visit_count ==
              audit.gpu_emit_pass_node_visit_count &&
          audit.gpu_count_pass_node_visit_count > 0U &&
          audit.buffer_epoch == expected_epoch,
      fixture + " records deterministic count and emit passes in one epoch");
  check(
      audit.frozen_labels_certified && audit.rope_topology_certified &&
          audit.exact_capacity_certified && audit.no_truncation_certified &&
          audit.candidate_superset_certified &&
          audit.cpu_exact_resolution_complete,
      fixture + " certifies only the proposal superset and CPU resolution contract");
}

void test_terminal_singleton_without_gpu_launch() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 1> input{point(2.0, -1.0, 4.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  K1BoruvkaCandidateContext context{index, cloud};
  const std::array<PointId, 1> labels{PointId{0}};

  const K1BoruvkaRoundProposal proposal = context.propose_round(
      cloud, std::span<const PointId>{labels});
  check(
      context.point_count() == 1U && context.node_count() == 1U,
      "singleton context snapshots its terminal leaf");
  check(
      proposal.frozen_component_labels ==
              std::vector<PointId>{PointId{0}} &&
          proposal.seeds.empty() && proposal.candidates.empty() &&
          proposal.cpu_exact_component_minima.empty() &&
          proposal.candidate_offsets == std::vector<std::size_t>{0U, 0U},
      "terminal singleton returns one empty certified CSR without seeds");
  check(
      proposal.audit.frozen_component_count == 1U &&
          proposal.audit.gpu_kernel_launch_count == 0U &&
          proposal.audit.gpu_synchronization_count == 0U &&
          proposal.audit.buffer_epoch == 0U &&
          proposal.audit.candidate_superset_certified &&
          proposal.audit.cpu_exact_resolution_complete,
      "terminal singleton is resolved before any GPU work");
  check(
      fake_gpu_k1_boruvka_launch_count() == 0U,
      "terminal singleton never invokes the fake launcher");

  const K1BoruvkaChunkedRoundResolution chunked =
      context.propose_round_chunked(
          cloud,
          std::span<const PointId>{labels},
          K1BoruvkaChunkingPolicy{1U});
  check(
      chunked.cpu_exact_component_minima.empty() &&
          chunked.proposal_audit.gpu_candidate_count == 0U &&
          chunked.proposal_audit.gpu_kernel_launch_count == 0U &&
          chunked.proposal_audit.buffer_epoch == 0U &&
          chunked.proposal_audit.proposal_digest_fnv1a ==
              proposal.audit.proposal_digest_fnv1a &&
          chunked.proposal_audit.candidate_superset_certified &&
          chunked.proposal_audit.cpu_exact_resolution_complete,
      "terminal chunked singleton publishes the same vacuous proposal certificate");
  check(
      chunked.emission_audit.logical_candidate_count == 0U &&
          chunked.emission_audit.source_chunk_count == 0U &&
          chunked.emission_audit.candidate_record_budget == 1U &&
          chunked.emission_audit.candidate_record_size_bytes == 16U &&
          chunked.emission_audit.candidate_payload_peak_bytes == 0U &&
          chunked.emission_audit.complete_source_partition_certified &&
          chunked.emission_audit
              .count_emit_cardinality_and_visit_count_certified &&
          chunked.emission_audit
              .candidate_payload_physical_bound_certified &&
          chunked.emission_status ==
              K1BoruvkaEmissionStatus::
                  complete_source_ranges_candidate_payload_bound_certified &&
          fake_gpu_k1_boruvka_launch_count() == 0U,
      "terminal chunked singleton certifies an empty bounded stream without GPU work");
}

void test_hybrid_terminal_singleton_without_gpu_launch() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 1> input{point(2.0, -1.0, 4.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);

  const K1HybridBoruvkaResult result =
      build_gpu_proposed_cpu_exact_k1_boruvka(index, cloud);
  check(
      result.point_count == 1U && result.rounds.empty() &&
          result.emst_edges.empty() &&
          result.total_squared_weight == ExactLevel{} &&
          result.total_hgp_weight == ExactLevel{},
      "hybrid singleton publishes the empty exact EMST witness");
  check(
      result.counters.point_count == 1U &&
          result.counters.lbvh_node_count == 1U &&
          result.counters.round_count == 0U &&
          result.counters.theoretical_max_round_count == 0U &&
          result.counters.gpu_kernel_launch_count == 0U &&
          result.counters.gpu_synchronization_count == 0U &&
          result.counters.first_buffer_epoch == 0U &&
          result.counters.last_buffer_epoch == 0U &&
          result.counters.final_component_count == 1U,
      "hybrid singleton closes its vacuous counters without a buffer epoch");
  check(
      result.hierarchy_reduction_status ==
              K1HybridHierarchyReductionStatus::not_performed &&
          result.scientific_status ==
              K1HybridScientificStatus::local_emst_witness_only &&
          result.proposal_chain_certified &&
          result.cpu_exact_decision_chain_certified &&
          result.canonical_contraction_chain_certified &&
          result.reference_cpu_witness_certified &&
          result.emst_witness_certified,
      "hybrid singleton certifies only its local witness and keeps hierarchy reduction separate");
  check(
      fake_gpu_k1_boruvka_launch_count() == 0U,
      "hybrid singleton never invokes the fake launcher");
}

void test_hybrid_three_round_chain_and_falsification() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 8> input{
      point(0.0),
      point(1.0),
      point(10.0),
      point(12.0),
      point(100.0),
      point(104.0),
      point(120.0),
      point(125.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const K1ExactBoruvkaResult cpu_anchor =
      build_exact_lbvh_boruvka(index, cloud);

  const K1HybridBoruvkaResult result =
      build_gpu_proposed_cpu_exact_k1_boruvka(index, cloud);
  check(
      result.rounds.size() == 3U && cpu_anchor.rounds.size() == 3U,
      "hybrid separated chain closes in three Boruvka rounds");
  const std::array<std::size_t, 3> expected_pre{8U, 4U, 2U};
  const std::array<std::size_t, 3> expected_post{4U, 2U, 1U};
  const std::size_t comparable_round_count = std::min(
      {result.rounds.size(), cpu_anchor.rounds.size(), expected_pre.size()});
  for (std::size_t round_index = 0U;
       round_index < comparable_round_count;
       ++round_index) {
    const auto& hybrid_round = result.rounds[round_index];
    const auto& cpu_round = cpu_anchor.rounds[round_index];
    check(
        hybrid_round.exact_decision.round_index == cpu_round.round_index &&
            hybrid_round.exact_decision.frozen_component_count ==
                expected_pre[round_index] &&
            hybrid_round.exact_decision.component_minima ==
                cpu_round.component_minima &&
            hybrid_round.canonical_contraction.accepted_edges ==
                cpu_round.accepted_edges &&
            hybrid_round.canonical_contraction
                    .post_round_component_count == expected_post[round_index],
        "hybrid separated chain matches the CPU exact decision and 8-to-4-to-2-to-1 contraction");
    check(
        hybrid_round.proposal_audit.buffer_epoch == round_index + 1U,
        "hybrid separated chain advances one resident buffer epoch per round");
  }
  check(
      result.emst_edges == cpu_anchor.emst_edges &&
          result.total_squared_weight == cpu_anchor.total_squared_weight &&
          result.total_hgp_weight == cpu_anchor.total_hgp_weight,
      "hybrid separated chain publishes the exact CPU witness and weights");
  check(
      result.counters.round_count == 3U &&
          result.counters.accepted_edge_count == 7U &&
          result.counters.component_contraction_count == 7U &&
          result.counters.gpu_kernel_launch_count == 6U &&
          result.counters.gpu_synchronization_count == 6U &&
          result.counters.first_buffer_epoch == 1U &&
          result.counters.last_buffer_epoch == 3U &&
          result.counters.final_component_count == 1U,
      "hybrid separated chain closes its producer-only counters");
  check(
      fake_gpu_k1_boruvka_launch_count() == 6U,
      "hybrid separated chain executes three producer and three verifier proposal transactions");

  const K1CompactForest hybrid_forest = build_compact_k1_forest(
      result.point_count,
      std::span<const morsehgp3d::hierarchy::ExactEmstEdge>{
          result.emst_edges});
  const K1CompactForest cpu_forest = build_compact_k1_forest(
      cpu_anchor.point_count,
      std::span<const morsehgp3d::hierarchy::ExactEmstEdge>{
          cpu_anchor.emst_edges});
  check(
      same_compact_forest(hybrid_forest, cpu_forest),
      "hybrid separated chain induces the same canonical compact forest as the CPU witness");

  const K1HybridBoruvkaVerification verification =
      verify_gpu_proposed_cpu_exact_k1_boruvka(index, cloud, result);
  check(
      verification.proposal_chain_certified &&
          verification.cpu_exact_decision_chain_certified &&
          verification.canonical_contractions_certified &&
          verification.round_count_bound_certified &&
          verification.spanning_tree_certified &&
          verification.exact_weights_certified &&
          verification.reference_cpu_witness_certified &&
          verification.counters_certified &&
          verification.hierarchy_status_separation_certified &&
          verification.emst_witness_certified &&
          verification.reference_round_count == 3U &&
          verification.gpu_replayed_round_count == 3U &&
          verification.gpu_replay_kernel_launch_count == 6U &&
          verification.gpu_replay_synchronization_count == 6U,
      "hybrid separated chain passes the independent complete verifier");

  K1HybridBoruvkaResult falsified = result;
  falsified.rounds.at(1U).proposal_audit.no_truncation_certified = false;
  const K1HybridBoruvkaVerification rejected =
      verify_gpu_proposed_cpu_exact_k1_boruvka(index, cloud, falsified);
  check(
      !rejected.proposal_chain_certified &&
          rejected.cpu_exact_decision_chain_certified &&
          rejected.canonical_contractions_certified &&
          !rejected.emst_witness_certified,
      "hybrid verifier rejects a falsified proposal certificate without conflating exact CPU decisions");

  falsified = result;
  falsified.rounds.at(1U).decision_status =
      K1HybridBoruvkaDecisionStatus::not_certified;
  const K1HybridBoruvkaVerification rejected_decision =
      verify_gpu_proposed_cpu_exact_k1_boruvka(index, cloud, falsified);
  check(
      rejected_decision.proposal_chain_certified &&
          !rejected_decision.cpu_exact_decision_chain_certified &&
          rejected_decision.canonical_contractions_certified &&
          !rejected_decision.emst_witness_certified,
      "hybrid verifier isolates a falsified exact-decision status");

  falsified = result;
  falsified.rounds.at(1U).contraction_status =
      K1HybridBoruvkaContractionStatus::not_certified;
  const K1HybridBoruvkaVerification rejected_contraction =
      verify_gpu_proposed_cpu_exact_k1_boruvka(index, cloud, falsified);
  check(
      rejected_contraction.proposal_chain_certified &&
          rejected_contraction.cpu_exact_decision_chain_certified &&
          !rejected_contraction.canonical_contractions_certified &&
          !rejected_contraction.emst_witness_certified,
      "hybrid verifier isolates a falsified canonical-contraction status");
}

void test_chunked_hybrid_three_round_chain_and_trusted_policy() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 8> input{
      point(0.0),
      point(1.0),
      point(10.0),
      point(12.0),
      point(100.0),
      point(104.0),
      point(120.0),
      point(125.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const K1ExactBoruvkaResult cpu_anchor =
      build_exact_lbvh_boruvka(index, cloud);
  constexpr K1BoruvkaChunkingPolicy policy{7U};

  const K1HybridBoruvkaResult result =
      build_gpu_proposed_cpu_exact_k1_boruvka(index, cloud, policy);
  check(
      result.emission_mode == K1HybridBoruvkaEmissionMode::
                                  bounded_complete_source_ranges &&
          result.chunking_policy == policy &&
          result.bounded_candidate_emission_chain_certified &&
          result.proposal_chain_certified &&
          result.cpu_exact_decision_chain_certified &&
          result.canonical_contraction_chain_certified &&
          result.reference_cpu_witness_certified &&
          result.emst_witness_certified,
      "chunked hybrid chain separates and certifies bounded emission, exact decisions and contraction");
  check(
      result.rounds.size() == 3U &&
          result.emst_edges == cpu_anchor.emst_edges &&
          result.total_squared_weight == cpu_anchor.total_squared_weight &&
          result.total_hgp_weight == cpu_anchor.total_hgp_weight,
      "chunked hybrid chain preserves the three-round exact CPU witness");

  bool observed_strict_chunking = false;
  for (const auto& round : result.rounds) {
    const auto& proposal = round.proposal_audit;
    const auto& emission = round.chunked_emission_audit;
    observed_strict_chunking =
        observed_strict_chunking ||
        (emission.source_chunk_count > 1U &&
         emission.logical_candidate_count >
             emission.peak_chunk_candidate_count);
    check(
        round.chunked_emission_status ==
                K1BoruvkaEmissionStatus::
                    complete_source_ranges_candidate_payload_bound_certified &&
            emission.logical_candidate_count ==
                proposal.gpu_candidate_count &&
            emission.max_source_candidate_count > 0U &&
            emission.max_source_candidate_count <=
                emission.peak_chunk_candidate_count &&
            emission.peak_chunk_candidate_count <=
                policy.max_candidate_records_per_chunk &&
            emission.device_candidate_capacity_high_water <=
                policy.max_candidate_records_per_chunk &&
            emission.host_candidate_capacity_high_water <=
                policy.max_candidate_records_per_chunk &&
            emission.candidate_payload_peak_bytes <=
                2U * policy.max_candidate_records_per_chunk *
                    emission.candidate_record_size_bytes &&
            emission.complete_source_partition_certified &&
            emission
                .count_emit_cardinality_and_visit_count_certified &&
            emission.candidate_payload_physical_bound_certified,
        "each chunked hybrid round closes its logical, atomic-source and physical payload bounds");
  }
  check(
      observed_strict_chunking,
      "chunked hybrid chain materializes fewer candidates than its logical round volume");

  const auto& chunked = result.chunked_emission_counters;
  check(
      chunked.round_count == result.rounds.size() &&
          chunked.logical_candidate_count ==
              result.counters.gpu_candidate_count &&
          chunked.source_chunk_count > chunked.round_count &&
          chunked.max_source_candidate_count <=
              chunked.peak_chunk_candidate_count &&
          chunked.peak_chunk_candidate_count <=
              policy.max_candidate_records_per_chunk &&
          chunked.candidate_record_budget ==
              policy.max_candidate_records_per_chunk &&
          chunked.candidate_record_size_bytes == 16U &&
          chunked.count_kernel_launch_count == result.rounds.size() &&
          chunked.emit_kernel_launch_count ==
              chunked.source_chunk_count &&
          chunked.synchronization_count ==
              result.counters.gpu_synchronization_count,
      "chunked hybrid aggregate distinguishes logical volume, atomic-source maximum and physical peak");

  const K1HybridBoruvkaVerification verification =
      verify_gpu_proposed_cpu_exact_k1_boruvka(
          index, cloud, policy, result);
  check(
      verification.emission_mode_certified &&
          verification.bounded_candidate_emission_chain_certified &&
          verification.proposal_chain_certified &&
          verification.cpu_exact_decision_chain_certified &&
          verification.emst_witness_certified &&
          verification.gpu_replay_source_chunk_count ==
              chunked.source_chunk_count &&
          verification.gpu_replay_peak_chunk_candidate_count ==
              chunked.peak_chunk_candidate_count &&
          verification.gpu_replay_candidate_payload_peak_bytes ==
              chunked.candidate_payload_peak_bytes,
      "trusted chunk policy drives an independent bounded replay with identical physical counters");

  const std::size_t launches_before_untrusted_rejection =
      fake_gpu_k1_boruvka_launch_count();
  const K1HybridBoruvkaVerification missing_trusted_policy =
      verify_gpu_proposed_cpu_exact_k1_boruvka(index, cloud, result);
  check(
      !missing_trusted_policy.emission_mode_certified &&
          !missing_trusted_policy.proposal_chain_certified &&
          !missing_trusted_policy.emst_witness_certified &&
          fake_gpu_k1_boruvka_launch_count() ==
              launches_before_untrusted_rejection,
      "an untrusted serialized chunk budget cannot trigger a GPU replay allocation");

  K1HybridBoruvkaResult falsified = result;
  falsified.chunking_policy.max_candidate_records_per_chunk = 70U;
  const K1HybridBoruvkaVerification rejected_policy =
      verify_gpu_proposed_cpu_exact_k1_boruvka(
          index, cloud, policy, falsified);
  check(
      !rejected_policy.emission_mode_certified &&
          !rejected_policy.proposal_chain_certified &&
          !rejected_policy.emst_witness_certified &&
          fake_gpu_k1_boruvka_launch_count() ==
              launches_before_untrusted_rejection,
      "trusted verifier rejects a falsified result budget before GPU replay");

  falsified = result;
  falsified.rounds.at(1U)
      .chunked_emission_audit
      .candidate_payload_physical_bound_certified = false;
  const K1HybridBoruvkaVerification rejected_emission =
      verify_gpu_proposed_cpu_exact_k1_boruvka(
          index, cloud, policy, falsified);
  check(
      !rejected_emission.emission_mode_certified &&
          !rejected_emission.proposal_chain_certified &&
          !rejected_emission.emst_witness_certified,
      "chunked verifier rejects a falsified physical-emission certificate");
}

void test_square_ties_candidate_superset_and_threshold_equality() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 4> input{
      point(-1.0, -1.0),
      point(-1.0, 1.0),
      point(1.0, -1.0),
      point(1.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const K1ExactBoruvkaResult anchor =
      build_exact_lbvh_boruvka(index, cloud);
  const std::array<PointId, 4> labels{
      PointId{0}, PointId{1}, PointId{2}, PointId{3}};
  K1BoruvkaCandidateContext context{index, cloud};

  const K1BoruvkaRoundProposal proposal = context.propose_round(
      cloud, std::span<const PointId>{labels});
  check_normal_proposal(
      proposal,
      index,
      std::span<const PointId>{labels},
      anchor.rounds.front().component_minima,
      1U,
      "square singleton partition");
  check(
      proposal.seeds.size() == 4U &&
          proposal.seeds.front().source_point_id == PointId{0} &&
          proposal.seeds.front().exact_squared_cutoff ==
              ExactLevel{BigInt{4}},
      "square fixes one exact outgoing seed and cutoff per source");
  check(
      has_candidate(proposal, PointId{0}, PointId{1}) &&
          has_candidate(proposal, PointId{0}, PointId{2}),
      "strict pruning descends at equality and retains both square neighbors at the cutoff");
  check(
      fake_gpu_k1_boruvka_launch_count() == 1U &&
          fake_gpu_k1_boruvka_last_point_count() == 4U &&
          fake_gpu_k1_boruvka_last_node_count() == 7U,
      "square uses one resident seven-node fake launch");

  const K1BoruvkaRoundProposal replay = context.propose_round(
      cloud, std::span<const PointId>{labels});
  check(
      replay.seeds == proposal.seeds &&
          replay.candidate_offsets == proposal.candidate_offsets &&
          replay.candidates == proposal.candidates &&
          replay.cpu_exact_component_minima ==
              proposal.cpu_exact_component_minima &&
          replay.audit.proposal_digest_fnv1a ==
              proposal.audit.proposal_digest_fnv1a &&
          replay.audit.buffer_epoch == 2U,
      "square proposal is deterministic across resident buffer epochs");
}

void test_chunked_round_matches_monolithic_under_two_budgets() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 4> input{
      point(-1.0, -1.0),
      point(-1.0, 1.0),
      point(1.0, -1.0),
      point(1.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::array<PointId, 4> labels{
      PointId{0}, PointId{1}, PointId{2}, PointId{3}};

  K1BoruvkaCandidateContext monolithic_context{index, cloud};
  const K1BoruvkaRoundProposal monolithic =
      monolithic_context.propose_round(
          cloud, std::span<const PointId>{labels});
  const std::size_t maximum_source_count =
      maximum_candidate_segment_size(monolithic);
  check(
      maximum_source_count > 0U &&
          maximum_source_count + 1U < monolithic.candidates.size(),
      "square fixture exposes two distinct bounded chunk budgets");
  const std::array<std::size_t, 2> budgets{
      maximum_source_count, monolithic.candidates.size() - 1U};

  for (const std::size_t budget : budgets) {
    const std::size_t callbacks_before =
        fake_gpu_k1_boruvka_chunk_callback_count();
    K1BoruvkaCandidateContext chunked_context{index, cloud};
    const K1BoruvkaChunkedRoundResolution chunked =
        chunked_context.propose_round_chunked(
            cloud,
            std::span<const PointId>{labels},
            K1BoruvkaChunkingPolicy{budget});
    const auto& proposal = chunked.proposal_audit;
    const auto& emission = chunked.emission_audit;

    check(
        chunked.cpu_exact_component_minima ==
                monolithic.cpu_exact_component_minima &&
            proposal.proposal_digest_fnv1a ==
                monolithic.audit.proposal_digest_fnv1a,
        "chunked square preserves exact minima and the monolithic global digest");
    check(
        proposal.gpu_candidate_count == monolithic.candidates.size() &&
            proposal.gpu_output_capacity == monolithic.candidates.size() &&
            proposal.gpu_kernel_launch_count ==
                emission.count_kernel_launch_count +
                    emission.emit_kernel_launch_count &&
            proposal.gpu_synchronization_count ==
                emission.synchronization_count &&
            proposal.gpu_count_pass_node_visit_count ==
                proposal.gpu_emit_pass_node_visit_count &&
            proposal.buffer_epoch == 1U &&
            proposal.candidate_superset_certified &&
            proposal.cpu_exact_resolution_complete,
        "chunked square closes its logical proposal counters");
    check(
        emission.logical_candidate_count == monolithic.candidates.size() &&
            emission.source_chunk_count > 1U &&
            emission.peak_chunk_candidate_count <
                emission.logical_candidate_count &&
            emission.peak_chunk_candidate_count <= budget &&
            emission.device_candidate_capacity_high_water ==
                emission.peak_chunk_candidate_count &&
            emission.host_candidate_capacity_high_water ==
                emission.peak_chunk_candidate_count,
        "chunked square keeps physical candidate capacities below the logical output");
    check(
        emission.candidate_record_budget == budget &&
            emission.candidate_record_size_bytes == 16U &&
            emission.candidate_payload_peak_bytes ==
                (emission.device_candidate_capacity_high_water +
                 emission.host_candidate_capacity_high_water) *
                    emission.candidate_record_size_bytes &&
            emission.count_kernel_launch_count == 1U &&
            emission.emit_kernel_launch_count ==
                emission.source_chunk_count &&
            emission.synchronization_count ==
                1U + emission.source_chunk_count &&
            emission.complete_source_partition_certified &&
            emission.count_emit_cardinality_and_visit_count_certified &&
            emission.candidate_payload_physical_bound_certified &&
            chunked.emission_status ==
                K1BoruvkaEmissionStatus::
                    complete_source_ranges_candidate_payload_bound_certified,
        "chunked square publishes a closed bounded-payload certificate");
    check(
        fake_gpu_k1_boruvka_chunk_callback_count() ==
            callbacks_before + emission.source_chunk_count,
        "chunked fake consumes each complete source range synchronously");
  }
}

void test_chunked_budget_and_late_failure_fail_closed() {
  const std::array<CertifiedPoint3, 4> input{
      point(-1.0, -1.0),
      point(-1.0, 1.0),
      point(1.0, -1.0),
      point(1.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::array<PointId, 4> labels{
      PointId{0}, PointId{1}, PointId{2}, PointId{3}};

  reset_fake_gpu_k1_boruvka();
  K1BoruvkaCandidateContext oracle_context{index, cloud};
  const K1BoruvkaRoundProposal oracle = oracle_context.propose_round(
      cloud, std::span<const PointId>{labels});
  const std::size_t maximum_source_count =
      maximum_candidate_segment_size(oracle);
  check(
      maximum_source_count > 1U,
      "chunk budget rejection fixture has a multi-candidate source");

  const std::array<PointId, 4> terminal_labels{
      PointId{0}, PointId{0}, PointId{0}, PointId{0}};
  const K1BoruvkaChunkedRoundResolution bounded_terminal =
      oracle_context.propose_round_chunked(
          cloud,
          std::span<const PointId>{terminal_labels},
          K1BoruvkaChunkingPolicy{maximum_source_count});
  check(
      oracle.candidates.size() > maximum_source_count &&
          fake_gpu_k1_boruvka_budget_enforcement_count() == 1U &&
          bounded_terminal.emission_audit
                  .device_candidate_capacity_high_water == 0U &&
          bounded_terminal.emission_audit.candidate_payload_peak_bytes == 0U &&
          bounded_terminal.emission_audit
              .candidate_payload_physical_bound_certified,
      "terminal chunked round trims an inherited monolithic candidate buffer before certification");

  reset_fake_gpu_k1_boruvka();
  K1BoruvkaCandidateContext undersized{index, cloud};
  check_throws<std::invalid_argument>(
      [&undersized, &cloud, &labels, maximum_source_count]() {
        static_cast<void>(undersized.propose_round_chunked(
            cloud,
            std::span<const PointId>{labels},
            K1BoruvkaChunkingPolicy{maximum_source_count - 1U}));
      },
      "chunk budget below one complete source is rejected");
  check(
      fake_gpu_k1_boruvka_launch_count() == 1U &&
          fake_gpu_k1_boruvka_chunk_callback_count() == 0U &&
          fake_gpu_k1_boruvka_epoch_advance_count() == 0U,
      "undersized chunk budget fails after count but before callback or epoch");

  reset_fake_gpu_k1_boruvka();
  K1BoruvkaCandidateContext failed{index, cloud};
  configure_fake_gpu_k1_boruvka(FakeK1BoruvkaConfiguration{
      FakeK1BoruvkaCorruption::missing_late_chunk_candidate});
  check_throws<std::runtime_error>(
      [&failed, &cloud, &labels, maximum_source_count]() {
        static_cast<void>(failed.propose_round_chunked(
            cloud,
            std::span<const PointId>{labels},
            K1BoruvkaChunkingPolicy{maximum_source_count}));
      },
      "a missing candidate in a later chunk returns no partial resolution");
  const std::size_t callbacks_after_failure =
      fake_gpu_k1_boruvka_chunk_callback_count();
  check(
      fake_gpu_k1_boruvka_launch_count() == 1U &&
          callbacks_after_failure > 1U &&
          fake_gpu_k1_boruvka_epoch_advance_count() == 0U,
      "late chunk corruption follows a consumed chunk but never publishes an epoch");

  configure_fake_gpu_k1_boruvka(FakeK1BoruvkaConfiguration{});
  check_throws<std::runtime_error>(
      [&failed, &cloud, &labels, maximum_source_count]() {
        static_cast<void>(failed.propose_round_chunked(
            cloud,
            std::span<const PointId>{labels},
            K1BoruvkaChunkingPolicy{maximum_source_count}));
      },
      "late chunk failure poisons its resident context");
  check(
      fake_gpu_k1_boruvka_launch_count() == 1U &&
          fake_gpu_k1_boruvka_chunk_callback_count() ==
              callbacks_after_failure,
      "poisoned chunk context cannot relaunch or invoke another callback");

  K1BoruvkaCandidateContext fresh{index, cloud};
  const K1BoruvkaChunkedRoundResolution recovered =
      fresh.propose_round_chunked(
          cloud,
          std::span<const PointId>{labels},
          K1BoruvkaChunkingPolicy{maximum_source_count});
  check(
      recovered.emission_status ==
              K1BoruvkaEmissionStatus::
                  complete_source_ranges_candidate_payload_bound_certified &&
          recovered.proposal_audit.buffer_epoch == 1U &&
          fake_gpu_k1_boruvka_launch_count() == 2U &&
          fake_gpu_k1_boruvka_epoch_advance_count() == 1U,
      "late chunk poisoning remains isolated from a fresh context");
}

void test_already_contracted_two_component_partition() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 4> input{
      point(0.0), point(1.0), point(11.0), point(13.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const K1ExactBoruvkaResult anchor =
      build_exact_lbvh_boruvka(index, cloud);
  const std::array<PointId, 4> labels{
      PointId{0}, PointId{0}, PointId{2}, PointId{2}};
  K1BoruvkaCandidateContext context{index, cloud};

  const K1BoruvkaRoundProposal proposal = context.propose_round(
      cloud, std::span<const PointId>{labels});
  check_normal_proposal(
      proposal,
      index,
      std::span<const PointId>{labels},
      anchor.rounds.at(1U).component_minima,
      1U,
      "two-component contracted chain");
  check(
      proposal.cpu_exact_component_minima.size() == 2U &&
          proposal.cpu_exact_component_minima[0].outgoing_edge ==
              proposal.cpu_exact_component_minima[1].outgoing_edge &&
          proposal.cpu_exact_component_minima[0].source_point_id ==
              PointId{1} &&
          proposal.cpu_exact_component_minima[1].source_point_id ==
              PointId{2},
      "contracted chain resolves the shared exact bridge from both components");
  for (std::size_t source = 0U; source < labels.size(); ++source) {
    check(
        !candidate_segment(proposal, static_cast<PointId>(source)).empty(),
        "every point keeps at least its fixed exact outgoing seed candidate");
  }
}

void test_invalid_namespaces_labels_and_moved_from_objects() {
  reset_fake_gpu_k1_boruvka();
  const std::array<CertifiedPoint3, 4> input{
      point(0.0), point(1.0), point(11.0), point(13.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const CanonicalPointCloud other_identity = canonical_cloud(input);
  const std::array<PointId, 4> labels{
      PointId{0}, PointId{1}, PointId{2}, PointId{3}};

  check_throws<std::invalid_argument>(
      [&index, &other_identity]() {
        K1BoruvkaCandidateContext rejected{index, other_identity};
        static_cast<void>(rejected);
      },
      "context rejects an LBVH from another cloud identity");

  K1BoruvkaCandidateContext context{index, cloud};
  check_throws<std::invalid_argument>(
      [&context, &other_identity, &labels]() {
        static_cast<void>(context.propose_round(
            other_identity, std::span<const PointId>{labels}));
      },
      "context rejects a query cloud from another namespace");
  check_throws<std::invalid_argument>(
      [&context, &cloud, &labels]() {
        static_cast<void>(context.propose_round_chunked(
            cloud,
            std::span<const PointId>{labels},
            K1BoruvkaChunkingPolicy{0U}));
      },
      "chunked context rejects a zero candidate-record budget");
  const std::size_t overflowing_payload_budget =
      std::numeric_limits<std::size_t>::max() / 32U + 1U;
  check_throws<std::length_error>(
      [&context, &cloud, &labels, overflowing_payload_budget]() {
        static_cast<void>(context.propose_round_chunked(
            cloud,
            std::span<const PointId>{labels},
            K1BoruvkaChunkingPolicy{overflowing_payload_budget}));
      },
      "chunked context rejects a two-copy candidate payload byte overflow");
  check_throws<std::invalid_argument>(
      [&context, &cloud, &labels]() {
        static_cast<void>(context.propose_round(
            cloud, std::span<const PointId>{labels.data(), 3U}));
      },
      "context rejects a frozen label vector with the wrong size");
  const std::array<PointId, 4> out_of_range{
      PointId{0}, PointId{1}, PointId{2}, PointId{4}};
  check_throws_standard(
      [&context, &cloud, &out_of_range]() {
        static_cast<void>(context.propose_round(
            cloud, std::span<const PointId>{out_of_range}));
      },
      "context rejects an out-of-range frozen component label");
  const std::array<PointId, 4> noncanonical{
      PointId{1}, PointId{1}, PointId{2}, PointId{3}};
  check_throws<std::invalid_argument>(
      [&context, &cloud, &noncanonical]() {
        static_cast<void>(context.propose_round(
            cloud, std::span<const PointId>{noncanonical}));
      },
      "context rejects a component label that is not its least PointId");
  check(
      fake_gpu_k1_boruvka_launch_count() == 0U,
      "namespace and label validation fail before the GPU section");
  static_cast<void>(context.propose_round(
      cloud, std::span<const PointId>{labels}));
  check(
      fake_gpu_k1_boruvka_launch_count() == 1U,
      "pre-GPU validation errors do not poison the resident context");

  K1BoruvkaCandidateContext moved_to = std::move(context);
  check(
      context.point_count() == 0U && context.node_count() == 0U &&
          moved_to.point_count() == input.size(),
      "moved-from context exposes empty extents and moved-to state survives");
  check_throws<std::logic_error>(
      [&context, &cloud, &labels]() {
        static_cast<void>(context.propose_round(
            cloud, std::span<const PointId>{labels}));
      },
      "moved-from context rejects new proposal work");

  CanonicalPointCloud index_cloud = canonical_cloud(input);
  MortonLbvhIndex moved_index_source = MortonLbvhIndex::build(index_cloud);
  const MortonLbvhIndex retained_index = std::move(moved_index_source);
  check(retained_index.ready(), "moved-to LBVH remains ready");
  check_throws<std::invalid_argument>(
      [&moved_index_source, &index_cloud]() {
        K1BoruvkaCandidateContext rejected{
            moved_index_source, index_cloud};
        static_cast<void>(rejected);
      },
      "context rejects a moved-from LBVH index");

  CanonicalPointCloud moved_cloud_source = canonical_cloud(input);
  const MortonLbvhIndex cloud_index =
      MortonLbvhIndex::build(moved_cloud_source);
  const CanonicalPointCloud retained_cloud = std::move(moved_cloud_source);
  check(retained_cloud.size() == input.size(), "moved-to cloud remains valid");
  check_throws<std::invalid_argument>(
      [&cloud_index, &moved_cloud_source]() {
        K1BoruvkaCandidateContext rejected{
            cloud_index, moved_cloud_source};
        static_cast<void>(rejected);
      },
      "context rejects a moved-from canonical cloud");
}

void test_corrupt_batches_and_gpu_failure_poison_context() {
  const std::array<CertifiedPoint3, 4> input{
      point(-1.0, -1.0),
      point(-1.0, 1.0),
      point(1.0, -1.0),
      point(1.0, 1.0)};
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::array<PointId, 4> labels{
      PointId{0}, PointId{1}, PointId{2}, PointId{3}};
  const std::array<FakeK1BoruvkaCorruption, 4> corruptions{
      FakeK1BoruvkaCorruption::offset_count_mismatch,
      FakeK1BoruvkaCorruption::missing_outgoing_candidate,
      FakeK1BoruvkaCorruption::same_component_target,
      FakeK1BoruvkaCorruption::out_of_range_target};

  for (const FakeK1BoruvkaCorruption corruption : corruptions) {
    reset_fake_gpu_k1_boruvka();
    K1BoruvkaCandidateContext poisoned{index, cloud};
    configure_fake_gpu_k1_boruvka(FakeK1BoruvkaConfiguration{corruption});
    check_throws<std::runtime_error>(
        [&poisoned, &cloud, &labels]() {
          static_cast<void>(poisoned.propose_round(
              cloud, std::span<const PointId>{labels}));
        },
        "malformed post-GPU candidate batch fails closed");
    check(
        fake_gpu_k1_boruvka_launch_count() == 1U,
        "malformed batch is rejected only after one GPU transaction");

    configure_fake_gpu_k1_boruvka(FakeK1BoruvkaConfiguration{});
    check_throws<std::runtime_error>(
        [&poisoned, &cloud, &labels]() {
          static_cast<void>(poisoned.propose_round(
              cloud, std::span<const PointId>{labels}));
        },
        "post-GPU corruption poisons its resident context");
    check(
        fake_gpu_k1_boruvka_launch_count() == 1U,
        "poisoned context cannot relaunch after malformed output");

    K1BoruvkaCandidateContext fresh{index, cloud};
    const K1BoruvkaRoundProposal recovered = fresh.propose_round(
        cloud, std::span<const PointId>{labels});
    check(
        recovered.audit.candidate_superset_certified &&
            fake_gpu_k1_boruvka_launch_count() == 2U,
        "poisoning remains isolated from a fresh independent context");
  }

  reset_fake_gpu_k1_boruvka();
  K1BoruvkaCandidateContext failed{index, cloud};
  configure_fake_gpu_k1_boruvka(FakeK1BoruvkaConfiguration{
      FakeK1BoruvkaCorruption::simulated_gpu_failure});
  check_throws<std::runtime_error>(
      [&failed, &cloud, &labels]() {
        static_cast<void>(failed.propose_round(
            cloud, std::span<const PointId>{labels}));
      },
      "simulated GPU failure propagates without a candidate batch");
  check(
      fake_gpu_k1_boruvka_launch_count() == 0U,
      "simulated GPU failure occurs before publication counters advance");
  configure_fake_gpu_k1_boruvka(FakeK1BoruvkaConfiguration{});
  check_throws<std::runtime_error>(
      [&failed, &cloud, &labels]() {
        static_cast<void>(failed.propose_round(
            cloud, std::span<const PointId>{labels}));
      },
      "GPU failure poisons the context before any retry");
  check(
      fake_gpu_k1_boruvka_launch_count() == 0U,
      "poisoned failed context never invokes a retry launcher");

  const std::array<PointId, 4> terminal_labels{
      PointId{0}, PointId{0}, PointId{0}, PointId{0}};
  check_throws<std::runtime_error>(
      [&failed, &cloud, &terminal_labels]() {
        static_cast<void>(failed.propose_round(
            cloud, std::span<const PointId>{terminal_labels}));
      },
      "poisoned context rejects a later terminal publication too");
  check(
      fake_gpu_k1_boruvka_launch_count() == 0U,
      "poisoned terminal retry remains free of GPU launches");
}

}  // namespace

int main() {
  test_terminal_singleton_without_gpu_launch();
  test_hybrid_terminal_singleton_without_gpu_launch();
  test_hybrid_three_round_chain_and_falsification();
  test_chunked_hybrid_three_round_chain_and_trusted_policy();
  test_square_ties_candidate_superset_and_threshold_equality();
  test_chunked_round_matches_monolithic_under_two_budgets();
  test_chunked_budget_and_late_failure_fail_closed();
  test_already_contracted_two_component_partition();
  test_invalid_namespaces_labels_and_moved_from_objects();
  test_corrupt_batches_and_gpu_failure_poison_context();

  if (failures != 0) {
    std::cerr << failures << " GPU K1 Boruvka context test(s) failed\n";
    return 1;
  }
  return 0;
}
