#include "morsehgp3d/gpu/k1_boruvka.hpp"

#include "morsehgp3d/exact/point.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::gpu::K1BoruvkaCandidateAudit;
using morsehgp3d::gpu::K1BoruvkaChunkedEmissionAudit;
using morsehgp3d::gpu::K1BoruvkaChunkingPolicy;
using morsehgp3d::gpu::K1BoruvkaEmissionStatus;
using morsehgp3d::gpu::K1BoruvkaMortonSeedAudit;
using morsehgp3d::gpu::K1BoruvkaMortonSeedPolicy;
using morsehgp3d::gpu::K1BoruvkaSeedMode;
using morsehgp3d::gpu::K1BoruvkaSeedStatus;
using morsehgp3d::gpu::K1HybridBoruvkaChunkedEmissionCounters;
using morsehgp3d::gpu::K1HybridBoruvkaContractionStatus;
using morsehgp3d::gpu::K1HybridBoruvkaDecisionStatus;
using morsehgp3d::gpu::K1HybridBoruvkaEmissionMode;
using morsehgp3d::gpu::K1HybridBoruvkaMortonSeedCounters;
using morsehgp3d::gpu::K1HybridBoruvkaProposalStatus;
using morsehgp3d::gpu::K1HybridBoruvkaResult;
using morsehgp3d::gpu::K1HybridBoruvkaRound;
using morsehgp3d::gpu::K1HybridBoruvkaVerification;
using morsehgp3d::gpu::K1HybridHierarchyReductionStatus;
using morsehgp3d::gpu::K1HybridScientificStatus;
using morsehgp3d::gpu::build_gpu_proposed_cpu_exact_k1_boruvka;
using morsehgp3d::gpu::verify_gpu_proposed_cpu_exact_k1_boruvka;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;

struct ReplayCase {
  std::string_view fixture;
  std::vector<std::size_t> expected_component_count_path;
  K1BoruvkaChunkingPolicy trusted_chunking_policy;
  std::optional<K1BoruvkaMortonSeedPolicy> trusted_morton_seed_policy;
  K1HybridBoruvkaResult producer;
  K1HybridBoruvkaVerification verifier;
  struct MortonSeedComparison {
    std::size_t baseline_logical_candidate_count{};
    std::size_t refined_logical_candidate_count{};
    std::size_t baseline_source_chunk_count{};
    std::size_t refined_source_chunk_count{};
    bool exact_decisions_unchanged{false};
    bool canonical_contractions_unchanged{false};
    bool emst_edges_unchanged{false};
    bool exact_weights_unchanged{false};
  };
  std::optional<MortonSeedComparison> morton_seed_comparison;
};

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

void require(bool condition, std::string_view message) {
  if (!condition) {
    throw std::runtime_error{std::string{message}};
  }
}

[[nodiscard]] const char* proposal_status_name(
    K1HybridBoruvkaProposalStatus status) {
  switch (status) {
    case K1HybridBoruvkaProposalStatus::not_certified:
      return "not_certified";
    case K1HybridBoruvkaProposalStatus::candidate_superset_certified:
      return "candidate_superset_certified";
  }
  throw std::logic_error{"unknown hybrid Boruvka proposal status"};
}

[[nodiscard]] const char* emission_mode_name(
    K1HybridBoruvkaEmissionMode mode) {
  switch (mode) {
    case K1HybridBoruvkaEmissionMode::monolithic_round_payload:
      return "monolithic_round_payload";
    case K1HybridBoruvkaEmissionMode::bounded_complete_source_ranges:
      return "bounded_complete_source_ranges";
  }
  throw std::logic_error{"unknown hybrid Boruvka emission mode"};
}

[[nodiscard]] const char* seed_mode_name(K1BoruvkaSeedMode mode) {
  switch (mode) {
    case K1BoruvkaSeedMode::canonical_external_fallback:
      return "canonical_external_fallback";
    case K1BoruvkaSeedMode::gpu_morton_window_cpu_exact_monotone:
      return "gpu_morton_window_cpu_exact_monotone";
  }
  throw std::logic_error{"unknown hybrid Boruvka seed mode"};
}

[[nodiscard]] const char* seed_status_name(K1BoruvkaSeedStatus status) {
  switch (status) {
    case K1BoruvkaSeedStatus::not_certified:
      return "not_certified";
    case K1BoruvkaSeedStatus::
        bounded_morton_window_external_exact_monotone_certified:
      return "bounded_morton_window_external_exact_monotone_certified";
  }
  throw std::logic_error{"unknown K1 Boruvka seed status"};
}

[[nodiscard]] const char* emission_status_name(
    K1BoruvkaEmissionStatus status) {
  switch (status) {
    case K1BoruvkaEmissionStatus::not_certified:
      return "not_certified";
    case K1BoruvkaEmissionStatus::
        complete_source_ranges_candidate_payload_bound_certified:
      return "complete_source_ranges_candidate_payload_bound_certified";
  }
  throw std::logic_error{"unknown K1 Boruvka emission status"};
}

[[nodiscard]] const char* decision_status_name(
    K1HybridBoruvkaDecisionStatus status) {
  switch (status) {
    case K1HybridBoruvkaDecisionStatus::not_certified:
      return "not_certified";
    case K1HybridBoruvkaDecisionStatus::
        cpu_exact_kappa_minima_certified:
      return "cpu_exact_kappa_minima_certified";
  }
  throw std::logic_error{"unknown hybrid Boruvka decision status"};
}

[[nodiscard]] const char* contraction_status_name(
    K1HybridBoruvkaContractionStatus status) {
  switch (status) {
    case K1HybridBoruvkaContractionStatus::not_certified:
      return "not_certified";
    case K1HybridBoruvkaContractionStatus::
        cpu_exact_canonical_contraction_certified:
      return "cpu_exact_canonical_contraction_certified";
  }
  throw std::logic_error{"unknown hybrid Boruvka contraction status"};
}

[[nodiscard]] const char* hierarchy_status_name(
    K1HybridHierarchyReductionStatus status) {
  switch (status) {
    case K1HybridHierarchyReductionStatus::not_performed:
      return "not_performed";
  }
  throw std::logic_error{"unknown hybrid hierarchy-reduction status"};
}

[[nodiscard]] const char* scientific_status_name(
    K1HybridScientificStatus status) {
  switch (status) {
    case K1HybridScientificStatus::local_emst_witness_only:
      return "local_emst_witness_only";
  }
  throw std::logic_error{"unknown hybrid scientific status"};
}

void require_round_contract(
    const K1HybridBoruvkaRound& round,
    std::size_t round_index,
    std::size_t expected_pre,
    std::size_t expected_post,
    std::size_t point_count,
    std::size_t node_count,
    K1BoruvkaChunkingPolicy trusted_chunking_policy,
    std::optional<K1BoruvkaMortonSeedPolicy>
        trusted_morton_seed_policy) {
  const K1BoruvkaCandidateAudit& audit = round.proposal_audit;
  const K1BoruvkaChunkedEmissionAudit& emission =
      round.chunked_emission_audit;
  const K1BoruvkaMortonSeedAudit& seed = round.morton_seed_audit;
  require(
      round.proposal_status == K1HybridBoruvkaProposalStatus::
          candidate_superset_certified,
      "a full-loop replay proposal status is not certified");
  require(
      round.decision_status == K1HybridBoruvkaDecisionStatus::
          cpu_exact_kappa_minima_certified,
      "a full-loop replay exact-decision status is not certified");
  require(
      round.contraction_status == K1HybridBoruvkaContractionStatus::
          cpu_exact_canonical_contraction_certified,
      "a full-loop replay contraction status is not certified");
  require(
      round.exact_decision.round_index == round_index &&
          round.exact_decision.frozen_component_count == expected_pre &&
          round.exact_decision.component_minima.size() == expected_pre &&
          round.canonical_contraction.post_round_component_count ==
              expected_post &&
          round.canonical_contraction.accepted_edges.size() ==
              expected_pre - expected_post,
      "a full-loop replay round does not close its exact contraction");
  require(
      audit.resident_point_count == point_count &&
          audit.resident_node_count == node_count &&
          audit.frozen_component_count == expected_pre &&
          audit.uniform_lbvh_node_count <= node_count &&
          audit.mixed_lbvh_node_count ==
              node_count - audit.uniform_lbvh_node_count &&
          audit.exact_seed_count == point_count &&
          audit.gpu_candidate_count == audit.gpu_output_capacity &&
          audit.gpu_kernel_launch_count ==
              emission.count_kernel_launch_count +
                  emission.emit_kernel_launch_count &&
          audit.gpu_synchronization_count ==
              emission.synchronization_count &&
          audit.gpu_count_pass_node_visit_count ==
              audit.gpu_emit_pass_node_visit_count &&
          audit.cpu_required_candidate_count <=
              audit.gpu_candidate_count &&
          audit.cpu_exact_candidate_distance_evaluation_count ==
              audit.gpu_candidate_count &&
          audit.buffer_epoch ==
              static_cast<std::uint64_t>(round_index) + std::uint64_t{1},
      "a full-loop replay proposal audit does not close its counters");
  require(
      audit.frozen_labels_certified && audit.rope_topology_certified &&
          audit.exact_capacity_certified &&
          audit.no_truncation_certified &&
          audit.candidate_superset_certified &&
          audit.cpu_exact_resolution_complete,
      "a full-loop replay proposal audit does not close its certificates");
  require(
      round.chunked_emission_status ==
              K1BoruvkaEmissionStatus::
                  complete_source_ranges_candidate_payload_bound_certified &&
          emission.logical_candidate_count == audit.gpu_candidate_count &&
          emission.source_chunk_count > 0U &&
          emission.peak_chunk_source_count > 0U &&
          emission.peak_chunk_source_count <= point_count &&
          emission.max_source_candidate_count > 0U &&
          emission.max_source_candidate_count <=
              emission.peak_chunk_candidate_count &&
          emission.peak_chunk_candidate_count <=
              trusted_chunking_policy.max_candidate_records_per_chunk &&
          emission.candidate_record_budget ==
              trusted_chunking_policy.max_candidate_records_per_chunk &&
          emission.device_candidate_capacity_high_water >=
              emission.peak_chunk_candidate_count &&
          emission.device_candidate_capacity_high_water <=
              trusted_chunking_policy.max_candidate_records_per_chunk &&
          emission.host_candidate_capacity_high_water >=
              emission.peak_chunk_candidate_count &&
          emission.host_candidate_capacity_high_water <=
              trusted_chunking_policy.max_candidate_records_per_chunk &&
          emission.candidate_record_size_bytes ==
              sizeof(morsehgp3d::gpu::K1BoruvkaCandidate) &&
          emission.candidate_payload_peak_bytes ==
              (emission.device_candidate_capacity_high_water +
               emission.host_candidate_capacity_high_water) *
                  emission.candidate_record_size_bytes &&
          emission.candidate_payload_peak_bytes <=
              2U * trusted_chunking_policy.max_candidate_records_per_chunk *
                  emission.candidate_record_size_bytes &&
          emission.count_kernel_launch_count == 1U &&
          emission.emit_kernel_launch_count ==
              emission.source_chunk_count &&
          emission.synchronization_count ==
              emission.count_kernel_launch_count +
                  emission.emit_kernel_launch_count &&
          emission.complete_source_partition_certified &&
          emission.count_emit_cardinality_and_visit_count_certified &&
          emission.candidate_payload_physical_bound_certified,
      "a full-loop replay chunked-emission audit does not close its bounds");
  if (!trusted_morton_seed_policy.has_value()) {
    require(
        round.seed_status == K1BoruvkaSeedStatus::not_certified &&
            seed == K1BoruvkaMortonSeedAudit{},
        "a fixed-seed replay round published a Morton-seed certificate");
    return;
  }

  const std::size_t window_radius =
      trusted_morton_seed_policy->window_radius;
  const std::size_t neighbor_budget =
      std::min(point_count - 1U, 2U * window_radius);
  std::size_t expected_inspected_neighbor_count = 0U;
  for (std::size_t source_position = 0U;
       source_position < point_count;
       ++source_position) {
    expected_inspected_neighbor_count +=
        std::min(source_position, window_radius) +
        std::min(point_count - source_position - 1U, window_radius);
  }
  require(
      round.seed_status ==
              K1BoruvkaSeedStatus::
                  bounded_morton_window_external_exact_monotone_certified &&
          seed.source_count == point_count &&
          seed.window_radius == window_radius &&
          seed.neighbor_inspection_budget_per_source == neighbor_budget &&
          seed.maximum_inspected_neighbor_count_per_source ==
              neighbor_budget &&
          seed.inspected_neighbor_count ==
              expected_inspected_neighbor_count &&
          seed.external_neighbor_count <= seed.inspected_neighbor_count &&
          seed.floating_proposal_count <= seed.external_neighbor_count &&
          seed.exact_selected_proposal_count <=
              seed.floating_proposal_count &&
          seed.exact_strict_improvement_count <=
              seed.exact_selected_proposal_count &&
          seed.exact_fallback_count +
                  seed.exact_selected_proposal_count ==
              point_count &&
          seed.exact_seed_distance_evaluation_count >= point_count &&
          seed.exact_seed_distance_evaluation_count <=
              point_count + seed.floating_proposal_count &&
          seed.gpu_kernel_launch_count == 1U &&
          seed.gpu_synchronization_count == 1U &&
          seed.complete_source_coverage_certified &&
          seed.bounded_window_certified &&
          seed.external_targets_recertified &&
          seed.exact_monotone_cutoff_certified,
      "a full-loop replay Morton-seed audit does not close its bounded exact monotone chain");
}

void require_case_contract(const ReplayCase& replay_case) {
  const K1HybridBoruvkaResult& result = replay_case.producer;
  const K1HybridBoruvkaVerification& verification = replay_case.verifier;
  const bool bounded_morton_seed =
      replay_case.trusted_morton_seed_policy.has_value();
  require(
      result.point_count > 0U &&
          replay_case.expected_component_count_path.size() ==
              result.rounds.size() + 1U &&
          replay_case.expected_component_count_path.front() ==
              result.point_count &&
          replay_case.expected_component_count_path.back() == 1U,
      "a full-loop replay fixture has an invalid component-count path");
  require(
      result.hierarchy_reduction_status ==
              K1HybridHierarchyReductionStatus::not_performed &&
          result.scientific_status ==
              K1HybridScientificStatus::local_emst_witness_only &&
          result.emission_mode == K1HybridBoruvkaEmissionMode::
                                      bounded_complete_source_ranges &&
          result.chunking_policy ==
              replay_case.trusted_chunking_policy &&
          result.seed_mode ==
              (bounded_morton_seed
                   ? K1BoruvkaSeedMode::
                         gpu_morton_window_cpu_exact_monotone
                   : K1BoruvkaSeedMode::canonical_external_fallback) &&
          result.morton_seed_policy ==
              replay_case.trusted_morton_seed_policy.value_or(
                  K1BoruvkaMortonSeedPolicy{}) &&
          result.proposal_chain_certified &&
          result.bounded_candidate_emission_chain_certified &&
          result.bounded_morton_seed_chain_certified ==
              bounded_morton_seed &&
          result.cpu_exact_decision_chain_certified &&
          result.canonical_contraction_chain_certified &&
          result.reference_cpu_witness_certified &&
          result.emst_witness_certified,
      "a full-loop replay producer certificate does not close");
  require(
      verification.index_identity_certified &&
          verification.emission_mode_certified &&
          verification.seed_mode_certified &&
          verification.proposal_chain_certified &&
          verification.bounded_candidate_emission_chain_certified &&
          verification.bounded_morton_seed_chain_certified ==
              bounded_morton_seed &&
          verification.cpu_exact_decision_chain_certified &&
          verification.canonical_contractions_certified &&
          verification.round_count_bound_certified &&
          verification.spanning_tree_certified &&
          verification.exact_weights_certified &&
          verification.reference_cpu_witness_certified &&
          verification.counters_certified &&
          verification.hierarchy_status_separation_certified &&
          verification.emst_witness_certified,
      "a full-loop replay independent verifier certificate does not close");
  require(
      result.counters.point_count == result.point_count &&
          result.counters.round_count == result.rounds.size() &&
          result.counters.accepted_edge_count == result.point_count - 1U &&
          result.counters.component_contraction_count ==
              result.point_count - 1U &&
          result.counters.final_component_count == 1U &&
          result.emst_edges.size() == result.point_count - 1U,
      "a full-loop replay producer tree counters do not close");
  const K1HybridBoruvkaChunkedEmissionCounters& chunked =
      result.chunked_emission_counters;
  require(
      chunked.round_count == result.rounds.size() &&
          chunked.logical_candidate_count ==
              result.counters.gpu_candidate_count &&
          chunked.max_source_candidate_count <=
              chunked.peak_chunk_candidate_count &&
          chunked.peak_chunk_candidate_count <=
              replay_case.trusted_chunking_policy
                  .max_candidate_records_per_chunk &&
          chunked.candidate_record_budget ==
              replay_case.trusted_chunking_policy
                  .max_candidate_records_per_chunk &&
          chunked.device_candidate_capacity_high_water >=
              chunked.peak_chunk_candidate_count &&
          chunked.device_candidate_capacity_high_water <=
              replay_case.trusted_chunking_policy
                  .max_candidate_records_per_chunk &&
          chunked.host_candidate_capacity_high_water >=
              chunked.peak_chunk_candidate_count &&
          chunked.host_candidate_capacity_high_water <=
              replay_case.trusted_chunking_policy
                  .max_candidate_records_per_chunk &&
          chunked.candidate_record_size_bytes ==
              sizeof(morsehgp3d::gpu::K1BoruvkaCandidate) &&
          chunked.candidate_payload_peak_bytes <=
              2U * replay_case.trusted_chunking_policy
                       .max_candidate_records_per_chunk *
                  chunked.candidate_record_size_bytes &&
          chunked.count_kernel_launch_count == result.rounds.size() &&
          chunked.emit_kernel_launch_count ==
              chunked.source_chunk_count &&
          chunked.synchronization_count ==
              result.counters.gpu_synchronization_count,
      "a full-loop replay aggregate chunked-emission audit does not close");
  const K1HybridBoruvkaMortonSeedCounters& seeds =
      result.morton_seed_counters;
  if (bounded_morton_seed) {
    require(
        seeds.round_count == result.rounds.size() &&
            seeds.source_count ==
                result.point_count * result.rounds.size() &&
            seeds.window_radius ==
                replay_case.trusted_morton_seed_policy->window_radius &&
            seeds.neighbor_inspection_budget_per_source ==
                std::min(
                    result.point_count - 1U,
                    2U * replay_case.trusted_morton_seed_policy
                             ->window_radius) &&
            seeds.maximum_inspected_neighbor_count_per_source ==
                seeds.neighbor_inspection_budget_per_source &&
            seeds.external_neighbor_count <=
                seeds.inspected_neighbor_count &&
            seeds.floating_proposal_count <=
                seeds.external_neighbor_count &&
            seeds.exact_selected_proposal_count <=
                seeds.floating_proposal_count &&
            seeds.exact_strict_improvement_count <=
                seeds.exact_selected_proposal_count &&
            seeds.exact_fallback_count +
                    seeds.exact_selected_proposal_count ==
                seeds.source_count &&
            seeds.gpu_kernel_launch_count == result.rounds.size() &&
            seeds.gpu_synchronization_count == result.rounds.size(),
        "a full-loop replay aggregate Morton-seed audit does not close");
  } else {
    require(
        seeds == K1HybridBoruvkaMortonSeedCounters{},
        "a fixed-seed replay published nonzero Morton-seed counters");
  }
  require(
      verification.reference_round_count == result.rounds.size() &&
          verification.gpu_replayed_round_count == result.rounds.size() &&
          verification.reference_component_minimum_count ==
              result.counters.component_minimum_count &&
          verification.gpu_replayed_component_minimum_count ==
              result.counters.component_minimum_count &&
          verification.gpu_replay_kernel_launch_count ==
              result.counters.gpu_kernel_launch_count &&
          verification.gpu_replay_synchronization_count ==
              result.counters.gpu_synchronization_count &&
          verification.gpu_replay_source_chunk_count ==
              chunked.source_chunk_count &&
          verification.gpu_replay_peak_chunk_candidate_count ==
              chunked.peak_chunk_candidate_count &&
          verification.gpu_replay_candidate_payload_peak_bytes ==
              chunked.candidate_payload_peak_bytes &&
          verification.gpu_replay_seed_inspected_neighbor_count ==
              seeds.inspected_neighbor_count &&
          verification.gpu_replay_seed_selected_proposal_count ==
              seeds.exact_selected_proposal_count &&
          verification.gpu_replay_seed_strict_improvement_count ==
              seeds.exact_strict_improvement_count &&
          verification.gpu_replay_seed_kernel_launch_count ==
              seeds.gpu_kernel_launch_count &&
          verification.gpu_replay_seed_synchronization_count ==
              seeds.gpu_synchronization_count,
      "a full-loop replay verifier counters disagree with the producer");
  bool observed_strict_chunking = false;
  for (std::size_t round_index = 0U;
       round_index < result.rounds.size();
       ++round_index) {
    const K1BoruvkaChunkedEmissionAudit& emission =
        result.rounds[round_index].chunked_emission_audit;
    observed_strict_chunking =
        observed_strict_chunking ||
        (emission.source_chunk_count > 1U &&
         emission.logical_candidate_count >
             emission.peak_chunk_candidate_count);
    require_round_contract(
        result.rounds[round_index],
        round_index,
        replay_case.expected_component_count_path[round_index],
        replay_case.expected_component_count_path[round_index + 1U],
        result.point_count,
        result.counters.lbvh_node_count,
        replay_case.trusted_chunking_policy,
        replay_case.trusted_morton_seed_policy);
  }
  require(
      result.point_count == 1U || observed_strict_chunking,
      "a nonterminal full-loop replay fixture did not force multiple chunks");
}

template <std::size_t PointCount, std::size_t PathSize>
[[nodiscard]] ReplayCase run_fixture(
    std::string_view fixture,
    const std::array<CertifiedPoint3, PointCount>& input,
    const std::array<std::size_t, PathSize>& expected_path,
    K1BoruvkaChunkingPolicy trusted_chunking_policy,
    std::optional<K1BoruvkaMortonSeedPolicy>
        trusted_morton_seed_policy = std::nullopt) {
  try {
    const CanonicalPointCloud cloud = canonical_cloud(input);
    const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
    K1HybridBoruvkaResult producer =
        trusted_morton_seed_policy.has_value()
            ? build_gpu_proposed_cpu_exact_k1_boruvka(
                  index,
                  cloud,
                  trusted_chunking_policy,
                  *trusted_morton_seed_policy)
            : build_gpu_proposed_cpu_exact_k1_boruvka(
                  index, cloud, trusted_chunking_policy);
    K1HybridBoruvkaVerification verifier =
        trusted_morton_seed_policy.has_value()
            ? verify_gpu_proposed_cpu_exact_k1_boruvka(
                  index,
                  cloud,
                  trusted_chunking_policy,
                  *trusted_morton_seed_policy,
                  producer)
            : verify_gpu_proposed_cpu_exact_k1_boruvka(
                  index, cloud, trusted_chunking_policy, producer);
    ReplayCase replay_case{
        fixture,
        std::vector<std::size_t>{expected_path.begin(), expected_path.end()},
        trusted_chunking_policy,
        trusted_morton_seed_policy,
        std::move(producer),
        std::move(verifier),
        std::nullopt};
    require_case_contract(replay_case);
    return replay_case;
  } catch (const std::exception& error) {
    throw std::runtime_error{
        std::string{fixture} + ": " + error.what()};
  }
}

void close_morton_seed_comparison(
    const ReplayCase& baseline, ReplayCase& refined) {
  const K1HybridBoruvkaResult& fixed = baseline.producer;
  const K1HybridBoruvkaResult& morton = refined.producer;
  ReplayCase::MortonSeedComparison comparison;
  comparison.baseline_logical_candidate_count =
      fixed.chunked_emission_counters.logical_candidate_count;
  comparison.refined_logical_candidate_count =
      morton.chunked_emission_counters.logical_candidate_count;
  comparison.baseline_source_chunk_count =
      fixed.chunked_emission_counters.source_chunk_count;
  comparison.refined_source_chunk_count =
      morton.chunked_emission_counters.source_chunk_count;
  comparison.exact_decisions_unchanged =
      fixed.rounds.size() == morton.rounds.size() &&
      std::equal(
          fixed.rounds.begin(),
          fixed.rounds.end(),
          morton.rounds.begin(),
          [](const K1HybridBoruvkaRound& fixed_round,
             const K1HybridBoruvkaRound& morton_round) {
            return fixed_round.exact_decision == morton_round.exact_decision;
          });
  comparison.canonical_contractions_unchanged =
      fixed.rounds.size() == morton.rounds.size() &&
      std::equal(
          fixed.rounds.begin(),
          fixed.rounds.end(),
          morton.rounds.begin(),
          [](const K1HybridBoruvkaRound& fixed_round,
             const K1HybridBoruvkaRound& morton_round) {
            return fixed_round.canonical_contraction ==
                   morton_round.canonical_contraction;
          });
  comparison.emst_edges_unchanged = fixed.emst_edges == morton.emst_edges;
  comparison.exact_weights_unchanged =
      fixed.total_squared_weight == morton.total_squared_weight &&
      fixed.total_hgp_weight == morton.total_hgp_weight;
  const K1HybridBoruvkaMortonSeedCounters& seeds =
      morton.morton_seed_counters;

  require(
      baseline.fixture == "chain_three_rounds" &&
          refined.fixture == "chain_three_rounds_morton_seed" &&
          !baseline.trusted_morton_seed_policy.has_value() &&
          refined.trusted_morton_seed_policy ==
              std::optional<K1BoruvkaMortonSeedPolicy>{
                  K1BoruvkaMortonSeedPolicy{1U}} &&
          comparison.baseline_logical_candidate_count == 86U &&
          comparison.refined_logical_candidate_count == 41U &&
          comparison.baseline_source_chunk_count == 16U &&
          comparison.refined_source_chunk_count == 9U &&
          seeds.round_count == 3U && seeds.source_count == 24U &&
          seeds.window_radius == 1U &&
          seeds.neighbor_inspection_budget_per_source == 2U &&
          seeds.maximum_inspected_neighbor_count_per_source == 2U &&
          seeds.inspected_neighbor_count == 42U &&
          seeds.external_neighbor_count == 22U &&
          seeds.floating_proposal_count == 16U &&
          seeds.exact_selected_proposal_count == 11U &&
          seeds.exact_strict_improvement_count == 11U &&
          seeds.exact_fallback_count == 13U &&
          seeds.exact_seed_distance_evaluation_count == 36U &&
          seeds.gpu_kernel_launch_count == 3U &&
          seeds.gpu_synchronization_count == 3U &&
          comparison.exact_decisions_unchanged &&
          comparison.canonical_contractions_unchanged &&
          comparison.emst_edges_unchanged &&
          comparison.exact_weights_unchanged,
      "the radius-one Morton replay does not close 86 to 41 candidates and 16 to 9 chunks with an unchanged exact witness");
  refined.morton_seed_comparison = comparison;
}

void write_boolean(std::ostream& output, bool value) {
  output << (value ? "true" : "false");
}

void write_hex_word(std::ostream& output, std::uint64_t value) {
  const auto flags = output.flags();
  const char fill = output.fill();
  output << '"' << std::hex << std::nouppercase << std::setfill('0')
         << std::setw(16) << value << '"';
  output.flags(flags);
  output.fill(fill);
}

void write_level(std::ostream& output, const ExactLevel& level) {
  output << "{\"denominator\":\"" << level.denominator_string()
         << "\",\"numerator\":\"" << level.numerator_string() << "\"}";
}

void write_size_array(
    std::ostream& output, std::span<const std::size_t> values) {
  output << '[';
  for (std::size_t index = 0U; index < values.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    output << values[index];
  }
  output << ']';
}

void write_chunking_policy(
    std::ostream& output, K1BoruvkaChunkingPolicy policy) {
  output << "{\"max_candidate_records_per_chunk\":"
         << policy.max_candidate_records_per_chunk
         << ",\"source_partition\":\"complete_contiguous_unsplit\"}";
}

void write_morton_seed_policy(
    std::ostream& output, K1BoruvkaMortonSeedPolicy policy) {
  output << "{\"window_radius\":" << policy.window_radius << '}';
}

void write_chunked_emission_counters(
    std::ostream& output,
    const K1HybridBoruvkaChunkedEmissionCounters& counters) {
  output << "{\"candidate_payload_peak_bytes\":"
         << counters.candidate_payload_peak_bytes
         << ",\"candidate_record_budget\":"
         << counters.candidate_record_budget
         << ",\"candidate_record_size_bytes\":"
         << counters.candidate_record_size_bytes
         << ",\"count_kernel_launch_count\":"
         << counters.count_kernel_launch_count
         << ",\"device_candidate_capacity_high_water\":"
         << counters.device_candidate_capacity_high_water
         << ",\"emit_kernel_launch_count\":"
         << counters.emit_kernel_launch_count
         << ",\"host_candidate_capacity_high_water\":"
         << counters.host_candidate_capacity_high_water
         << ",\"logical_candidate_count\":"
         << counters.logical_candidate_count
         << ",\"max_source_candidate_count\":"
         << counters.max_source_candidate_count
         << ",\"peak_chunk_candidate_count\":"
         << counters.peak_chunk_candidate_count
         << ",\"peak_chunk_source_count\":"
         << counters.peak_chunk_source_count
         << ",\"round_count\":" << counters.round_count
         << ",\"source_chunk_count\":"
         << counters.source_chunk_count
         << ",\"synchronization_count\":"
         << counters.synchronization_count << '}';
}

void write_morton_seed_counters(
    std::ostream& output,
    const K1HybridBoruvkaMortonSeedCounters& counters) {
  output << "{\"exact_fallback_count\":"
         << counters.exact_fallback_count
         << ",\"exact_seed_distance_evaluation_count\":"
         << counters.exact_seed_distance_evaluation_count
         << ",\"exact_selected_proposal_count\":"
         << counters.exact_selected_proposal_count
         << ",\"exact_strict_improvement_count\":"
         << counters.exact_strict_improvement_count
         << ",\"external_neighbor_count\":"
         << counters.external_neighbor_count
         << ",\"floating_proposal_count\":"
         << counters.floating_proposal_count
         << ",\"gpu_kernel_launch_count\":"
         << counters.gpu_kernel_launch_count
         << ",\"gpu_synchronization_count\":"
         << counters.gpu_synchronization_count
         << ",\"inspected_neighbor_count\":"
         << counters.inspected_neighbor_count
         << ",\"maximum_inspected_neighbor_count_per_source\":"
         << counters.maximum_inspected_neighbor_count_per_source
         << ",\"neighbor_inspection_budget_per_source\":"
         << counters.neighbor_inspection_budget_per_source
         << ",\"round_count\":" << counters.round_count
         << ",\"source_count\":" << counters.source_count
         << ",\"window_radius\":" << counters.window_radius << '}';
}

void write_producer(std::ostream& output, const K1HybridBoruvkaResult& result) {
  const auto& counters = result.counters;
  output << "{\"certificates\":{\"bounded_candidate_emission_chain_certified\":";
  write_boolean(output, result.bounded_candidate_emission_chain_certified);
  output << ",\"bounded_morton_seed_chain_certified\":";
  write_boolean(output, result.bounded_morton_seed_chain_certified);
  output << ",\"canonical_contraction_chain_certified\":";
  write_boolean(output, result.canonical_contraction_chain_certified);
  output << ",\"cpu_exact_decision_chain_certified\":";
  write_boolean(output, result.cpu_exact_decision_chain_certified);
  output << ",\"emst_witness_certified\":";
  write_boolean(output, result.emst_witness_certified);
  output << ",\"proposal_chain_certified\":";
  write_boolean(output, result.proposal_chain_certified);
  output << ",\"reference_cpu_witness_certified\":";
  write_boolean(output, result.reference_cpu_witness_certified);
  output << "},\"chunked_emission_counters\":";
  write_chunked_emission_counters(
      output, result.chunked_emission_counters);
  output << ",\"counters\":{\"accepted_edge_count\":"
         << counters.accepted_edge_count
         << ",\"component_contraction_count\":"
         << counters.component_contraction_count
         << ",\"component_minimum_count\":"
         << counters.component_minimum_count
         << ",\"cpu_exact_aabb_bound_evaluation_count\":"
         << counters.cpu_exact_aabb_bound_evaluation_count
         << ",\"cpu_exact_candidate_distance_evaluation_count\":"
         << counters.cpu_exact_candidate_distance_evaluation_count
         << ",\"cpu_required_candidate_count\":"
         << counters.cpu_required_candidate_count
         << ",\"final_component_count\":"
         << counters.final_component_count
         << ",\"first_buffer_epoch\":" << counters.first_buffer_epoch
         << ",\"frozen_component_label_count\":"
         << counters.frozen_component_label_count
         << ",\"gpu_candidate_count\":" << counters.gpu_candidate_count
         << ",\"gpu_count_pass_node_visit_count\":"
         << counters.gpu_count_pass_node_visit_count
         << ",\"gpu_emit_pass_node_visit_count\":"
         << counters.gpu_emit_pass_node_visit_count
         << ",\"gpu_invalid_bound_descent_count\":"
         << counters.gpu_invalid_bound_descent_count
         << ",\"gpu_kernel_launch_count\":"
         << counters.gpu_kernel_launch_count
         << ",\"gpu_output_capacity_sum\":"
         << counters.gpu_output_capacity_sum
         << ",\"gpu_strict_aabb_prune_count\":"
         << counters.gpu_strict_aabb_prune_count
         << ",\"gpu_synchronization_count\":"
         << counters.gpu_synchronization_count
         << ",\"gpu_uniform_component_prune_count\":"
         << counters.gpu_uniform_component_prune_count
         << ",\"last_buffer_epoch\":" << counters.last_buffer_epoch
         << ",\"lbvh_node_count\":" << counters.lbvh_node_count
         << ",\"peak_gpu_output_capacity\":"
         << counters.peak_gpu_output_capacity
         << ",\"point_count\":" << counters.point_count
         << ",\"round_count\":" << counters.round_count
         << ",\"theoretical_max_round_count\":"
         << counters.theoretical_max_round_count
         << "},\"emission_mode\":\""
         << emission_mode_name(result.emission_mode)
         << "\",\"morton_seed_counters\":";
  write_morton_seed_counters(output, result.morton_seed_counters);
  output << ",\"morton_seed_policy\":";
  write_morton_seed_policy(output, result.morton_seed_policy);
  output << ",\"seed_mode\":\"" << seed_mode_name(result.seed_mode)
         << "\"}";
}

void write_chunked_emission_audit(
    std::ostream& output,
    const K1BoruvkaChunkedEmissionAudit& emission) {
  output << "{\"candidate_payload_peak_bytes\":"
         << emission.candidate_payload_peak_bytes
         << ",\"candidate_record_budget\":"
         << emission.candidate_record_budget
         << ",\"candidate_record_size_bytes\":"
         << emission.candidate_record_size_bytes
         << ",\"certificates\":{\"candidate_payload_physical_bound_certified\":";
  write_boolean(
      output, emission.candidate_payload_physical_bound_certified);
  output << ",\"complete_source_partition_certified\":";
  write_boolean(output, emission.complete_source_partition_certified);
  output << ",\"count_emit_cardinality_and_visit_count_certified\":";
  write_boolean(
      output,
      emission.count_emit_cardinality_and_visit_count_certified);
  output << "},\"count_kernel_launch_count\":"
         << emission.count_kernel_launch_count
         << ",\"device_candidate_capacity_high_water\":"
         << emission.device_candidate_capacity_high_water
         << ",\"emit_kernel_launch_count\":"
         << emission.emit_kernel_launch_count
         << ",\"host_candidate_capacity_high_water\":"
         << emission.host_candidate_capacity_high_water
         << ",\"logical_candidate_count\":"
         << emission.logical_candidate_count
         << ",\"max_source_candidate_count\":"
         << emission.max_source_candidate_count
         << ",\"peak_chunk_candidate_count\":"
         << emission.peak_chunk_candidate_count
         << ",\"peak_chunk_source_count\":"
         << emission.peak_chunk_source_count
         << ",\"source_chunk_count\":"
         << emission.source_chunk_count
         << ",\"synchronization_count\":"
         << emission.synchronization_count << '}';
}

void write_morton_seed_audit(
    std::ostream& output, const K1BoruvkaMortonSeedAudit& seed) {
  output << "{\"certificates\":{\"bounded_window_certified\":";
  write_boolean(output, seed.bounded_window_certified);
  output << ",\"complete_source_coverage_certified\":";
  write_boolean(output, seed.complete_source_coverage_certified);
  output << ",\"exact_monotone_cutoff_certified\":";
  write_boolean(output, seed.exact_monotone_cutoff_certified);
  output << ",\"external_targets_recertified\":";
  write_boolean(output, seed.external_targets_recertified);
  output << "},\"exact_fallback_count\":" << seed.exact_fallback_count
         << ",\"exact_seed_distance_evaluation_count\":"
         << seed.exact_seed_distance_evaluation_count
         << ",\"exact_selected_proposal_count\":"
         << seed.exact_selected_proposal_count
         << ",\"exact_strict_improvement_count\":"
         << seed.exact_strict_improvement_count
         << ",\"external_neighbor_count\":"
         << seed.external_neighbor_count
         << ",\"floating_proposal_count\":"
         << seed.floating_proposal_count
         << ",\"gpu_kernel_launch_count\":"
         << seed.gpu_kernel_launch_count
         << ",\"gpu_synchronization_count\":"
         << seed.gpu_synchronization_count
         << ",\"inspected_neighbor_count\":"
         << seed.inspected_neighbor_count
         << ",\"maximum_inspected_neighbor_count_per_source\":"
         << seed.maximum_inspected_neighbor_count_per_source
         << ",\"neighbor_inspection_budget_per_source\":"
         << seed.neighbor_inspection_budget_per_source
         << ",\"source_count\":" << seed.source_count
         << ",\"window_radius\":" << seed.window_radius << '}';
}

void write_round(std::ostream& output, const K1HybridBoruvkaRound& round) {
  const K1BoruvkaCandidateAudit& audit = round.proposal_audit;
  output << "{\"audit\":{\"buffer_epoch\":" << audit.buffer_epoch
         << ",\"candidate_count\":" << audit.gpu_candidate_count
         << ",\"certificates\":{\"candidate_superset_certified\":";
  write_boolean(output, audit.candidate_superset_certified);
  output << ",\"cpu_exact_resolution_complete\":";
  write_boolean(output, audit.cpu_exact_resolution_complete);
  output << ",\"exact_capacity_certified\":";
  write_boolean(output, audit.exact_capacity_certified);
  output << ",\"frozen_labels_certified\":";
  write_boolean(output, audit.frozen_labels_certified);
  output << ",\"no_truncation_certified\":";
  write_boolean(output, audit.no_truncation_certified);
  output << ",\"rope_topology_certified\":";
  write_boolean(output, audit.rope_topology_certified);
  output << "},\"count_pass_node_visit_count\":"
         << audit.gpu_count_pass_node_visit_count
         << ",\"cpu_exact_aabb_bound_evaluation_count\":"
         << audit.cpu_exact_aabb_bound_evaluation_count
         << ",\"cpu_exact_candidate_distance_evaluation_count\":"
         << audit.cpu_exact_candidate_distance_evaluation_count
         << ",\"emit_pass_node_visit_count\":"
         << audit.gpu_emit_pass_node_visit_count
         << ",\"exact_seed_count\":" << audit.exact_seed_count
         << ",\"invalid_bound_descent_count\":"
         << audit.gpu_invalid_bound_descent_count
         << ",\"kernel_launch_count\":" << audit.gpu_kernel_launch_count
         << ",\"mixed_lbvh_node_count\":" << audit.mixed_lbvh_node_count
         << ",\"output_capacity\":" << audit.gpu_output_capacity
         << ",\"proposal_digest_fnv1a\":";
  write_hex_word(output, audit.proposal_digest_fnv1a);
  output << ",\"required_candidate_count\":"
         << audit.cpu_required_candidate_count
         << ",\"resident_node_count\":" << audit.resident_node_count
         << ",\"resident_point_count\":" << audit.resident_point_count
         << ",\"strict_aabb_prune_count\":"
         << audit.gpu_strict_aabb_prune_count
         << ",\"synchronization_count\":"
         << audit.gpu_synchronization_count
         << ",\"uniform_component_prune_count\":"
         << audit.gpu_uniform_component_prune_count
         << "},\"contraction\":{\"accepted_edge_count\":"
         << round.canonical_contraction.accepted_edges.size()
         << ",\"post_round_component_count\":"
         << round.canonical_contraction.post_round_component_count
         << ",\"status\":\""
         << contraction_status_name(round.contraction_status)
         << "\"},\"decision\":{\"component_minimum_count\":"
         << round.exact_decision.component_minima.size()
         << ",\"frozen_component_count\":"
         << round.exact_decision.frozen_component_count
         << ",\"round_index\":" << round.exact_decision.round_index
         << ",\"status\":\"" << decision_status_name(round.decision_status)
         << "\"},\"emission_audit\":";
  write_chunked_emission_audit(output, round.chunked_emission_audit);
  output << ",\"emission_status\":\""
         << emission_status_name(round.chunked_emission_status)
         << "\",\"morton_seed_audit\":";
  write_morton_seed_audit(output, round.morton_seed_audit);
  output << ",\"proposal_status\":\""
         << proposal_status_name(round.proposal_status)
         << "\",\"seed_status\":\""
         << seed_status_name(round.seed_status) << "\"}";
}

void write_verifier(
    std::ostream& output, const K1HybridBoruvkaVerification& verification) {
  output << "{\"certificates\":{\"bounded_candidate_emission_chain_certified\":";
  write_boolean(
      output,
      verification.bounded_candidate_emission_chain_certified);
  output << ",\"bounded_morton_seed_chain_certified\":";
  write_boolean(
      output,
      verification.bounded_morton_seed_chain_certified);
  output << ",\"canonical_contractions_certified\":";
  write_boolean(output, verification.canonical_contractions_certified);
  output << ",\"counters_certified\":";
  write_boolean(output, verification.counters_certified);
  output << ",\"cpu_exact_decision_chain_certified\":";
  write_boolean(output, verification.cpu_exact_decision_chain_certified);
  output << ",\"emission_mode_certified\":";
  write_boolean(output, verification.emission_mode_certified);
  output << ",\"emst_witness_certified\":";
  write_boolean(output, verification.emst_witness_certified);
  output << ",\"exact_weights_certified\":";
  write_boolean(output, verification.exact_weights_certified);
  output << ",\"hierarchy_status_separation_certified\":";
  write_boolean(output, verification.hierarchy_status_separation_certified);
  output << ",\"index_identity_certified\":";
  write_boolean(output, verification.index_identity_certified);
  output << ",\"proposal_chain_certified\":";
  write_boolean(output, verification.proposal_chain_certified);
  output << ",\"reference_cpu_witness_certified\":";
  write_boolean(output, verification.reference_cpu_witness_certified);
  output << ",\"round_count_bound_certified\":";
  write_boolean(output, verification.round_count_bound_certified);
  output << ",\"seed_mode_certified\":";
  write_boolean(output, verification.seed_mode_certified);
  output << ",\"spanning_tree_certified\":";
  write_boolean(output, verification.spanning_tree_certified);
  output << "},\"counters\":{\"gpu_replay_candidate_payload_peak_bytes\":"
         << verification.gpu_replay_candidate_payload_peak_bytes
         << ",\"gpu_replay_kernel_launch_count\":"
         << verification.gpu_replay_kernel_launch_count
         << ",\"gpu_replay_peak_chunk_candidate_count\":"
         << verification.gpu_replay_peak_chunk_candidate_count
         << ",\"gpu_replay_seed_inspected_neighbor_count\":"
         << verification.gpu_replay_seed_inspected_neighbor_count
         << ",\"gpu_replay_seed_kernel_launch_count\":"
         << verification.gpu_replay_seed_kernel_launch_count
         << ",\"gpu_replay_seed_selected_proposal_count\":"
         << verification.gpu_replay_seed_selected_proposal_count
         << ",\"gpu_replay_seed_strict_improvement_count\":"
         << verification.gpu_replay_seed_strict_improvement_count
         << ",\"gpu_replay_seed_synchronization_count\":"
         << verification.gpu_replay_seed_synchronization_count
         << ",\"gpu_replay_source_chunk_count\":"
         << verification.gpu_replay_source_chunk_count
         << ",\"gpu_replay_synchronization_count\":"
         << verification.gpu_replay_synchronization_count
         << ",\"gpu_replayed_component_minimum_count\":"
         << verification.gpu_replayed_component_minimum_count
         << ",\"gpu_replayed_round_count\":"
         << verification.gpu_replayed_round_count
         << ",\"reference_component_minimum_count\":"
         << verification.reference_component_minimum_count
         << ",\"reference_round_count\":"
         << verification.reference_round_count << "}}";
}

void write_morton_seed_comparison(
    std::ostream& output,
    const ReplayCase::MortonSeedComparison& comparison) {
  output << "{\"baseline\":{\"logical_candidate_count\":"
         << comparison.baseline_logical_candidate_count
         << ",\"seed_mode\":\"canonical_external_fallback\""
         << ",\"source_chunk_count\":"
         << comparison.baseline_source_chunk_count
         << "},\"certificates\":{\"canonical_contractions_unchanged\":";
  write_boolean(output, comparison.canonical_contractions_unchanged);
  output << ",\"emst_edges_unchanged\":";
  write_boolean(output, comparison.emst_edges_unchanged);
  output << ",\"exact_decisions_unchanged\":";
  write_boolean(output, comparison.exact_decisions_unchanged);
  output << ",\"exact_weights_unchanged\":";
  write_boolean(output, comparison.exact_weights_unchanged);
  output << "},\"refined\":{\"logical_candidate_count\":"
         << comparison.refined_logical_candidate_count
         << ",\"seed_mode\":\"gpu_morton_window_cpu_exact_monotone\""
         << ",\"source_chunk_count\":"
         << comparison.refined_source_chunk_count << "}}";
}

void write_case(std::ostream& output, const ReplayCase& replay_case) {
  const K1HybridBoruvkaResult& result = replay_case.producer;
  output << "{\"component_count_path\":";
  write_size_array(output, replay_case.expected_component_count_path);
  output << ",\"emst_edge_count\":" << result.emst_edges.size()
         << ",\"exact_weights\":{\"hgp\":";
  write_level(output, result.total_hgp_weight);
  output << ",\"squared\":";
  write_level(output, result.total_squared_weight);
  output << "},\"fixture\":\"" << replay_case.fixture
         << "\",\"morton_seed_comparison\":";
  if (replay_case.morton_seed_comparison.has_value()) {
    write_morton_seed_comparison(
        output, *replay_case.morton_seed_comparison);
  } else {
    output << "null";
  }
  output << ",\"point_count\":" << result.point_count
         << ",\"producer\":";
  write_producer(output, result);
  output << ",\"rounds\":[";
  for (std::size_t round_index = 0U;
       round_index < result.rounds.size();
       ++round_index) {
    if (round_index != 0U) {
      output << ',';
    }
    write_round(output, result.rounds[round_index]);
  }
  output << "],\"status\":\"passed\",\"trusted_chunking_policy\":";
  write_chunking_policy(output, replay_case.trusted_chunking_policy);
  output << ",\"trusted_morton_seed_policy\":";
  if (replay_case.trusted_morton_seed_policy.has_value()) {
    write_morton_seed_policy(
        output, *replay_case.trusted_morton_seed_policy);
  } else {
    output << "null";
  }
  output << ",\"verifier\":";
  write_verifier(output, replay_case.verifier);
  output << '}';
}

}  // namespace

int main() {
  try {
    const std::array<CertifiedPoint3, 1> singleton{
        point(2.0, -1.0, 4.0)};
    const std::array<CertifiedPoint3, 8> chain{
        point(0.0),
        point(1.0),
        point(10.0),
        point(12.0),
        point(100.0),
        point(104.0),
        point(120.0),
        point(125.0)};
    const std::array<CertifiedPoint3, 4> square{
        point(-1.0, -1.0),
        point(-1.0, 1.0),
        point(1.0, -1.0),
        point(1.0, 1.0)};

    std::vector<ReplayCase> cases;
    cases.reserve(4U);
    cases.push_back(run_fixture(
        "singleton_terminal",
        singleton,
        std::array<std::size_t, 1>{1U},
        K1BoruvkaChunkingPolicy{1U}));
    cases.push_back(run_fixture(
        "chain_three_rounds",
        chain,
        std::array<std::size_t, 4>{8U, 4U, 2U, 1U},
        K1BoruvkaChunkingPolicy{7U}));
    cases.push_back(run_fixture(
        "square_equal_length_ties",
        square,
        std::array<std::size_t, 2>{4U, 1U},
        K1BoruvkaChunkingPolicy{3U}));
    ReplayCase morton_seed_chain = run_fixture(
        "chain_three_rounds_morton_seed",
        chain,
        std::array<std::size_t, 4>{8U, 4U, 2U, 1U},
        K1BoruvkaChunkingPolicy{7U},
        K1BoruvkaMortonSeedPolicy{1U});
    close_morton_seed_comparison(cases.at(1U), morton_seed_chain);
    cases.push_back(std::move(morton_seed_chain));

    std::cout << "{\"bounded_emission_proof_basis\":\""
              << K1HybridBoruvkaResult::bounded_emission_proof_basis
              << "\",\"case_count\":" << cases.size()
              << ",\"cases\":[";
    for (std::size_t case_index = 0U;
         case_index < cases.size();
         ++case_index) {
      if (case_index != 0U) {
        std::cout << ',';
      }
      write_case(std::cout, cases[case_index]);
    }
    std::cout
        << "],\"decision_backend\":\"reference_cpu\""
        << ",\"decision_semantics\":\""
        << K1BoruvkaCandidateAudit::decision_semantics << "\""
        << ",\"hierarchy_reduction_status\":\""
        << hierarchy_status_name(
               K1HybridHierarchyReductionStatus::not_performed)
        << "\",\"mode\":\"certified\""
        << ",\"monotone_seed_proof_basis\":\""
        << K1HybridBoruvkaResult::monotone_seed_proof_basis
        << "\",\"phase\":\"5\""
        << ",\"profile\":\"hgp_reduced\",\"proof_basis\":\""
        << K1HybridBoruvkaResult::proof_basis << "\""
        << ",\"proposal_backend\":\"cuda_g4\""
        << ",\"proposal_semantics\":\""
        << K1BoruvkaCandidateAudit::proposal_semantics << "\""
        << ",\"schema\":\"morsehgp3d.phase5.k1_boruvka_full_loop_gpu_replay.v3\""
        << ",\"scientific_result_claimed\":false"
        << ",\"scientific_scope\":\""
        << scientific_status_name(
               K1HybridScientificStatus::local_emst_witness_only)
        << "\",\"status\":\"passed\""
        << ",\"verification_proposal_backend\":\"cuda_g4\"}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "Phase 5 full K1 Boruvka GPU replay failed: "
              << error.what() << '\n';
    return 1;
  }
}
