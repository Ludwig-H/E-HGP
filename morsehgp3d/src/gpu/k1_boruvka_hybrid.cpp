#include "morsehgp3d/gpu/k1_boruvka.hpp"

#include "morsehgp3d/exact/rational.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu {
namespace {

constexpr std::size_t kChunkedCandidateRecordSizeBytes =
    2U * sizeof(std::uint64_t);
constexpr std::size_t kChunkedCandidatePayloadCopies = 2U;

[[nodiscard]] spatial::PointId checked_point_id(std::size_t index) {
  if (index > static_cast<std::size_t>(
                  spatial::CanonicalPointCloud::max_point_id)) {
    throw std::length_error(
        "a hybrid K1 Boruvka point index exceeds PointId");
  }
  return static_cast<spatial::PointId>(index);
}

[[nodiscard]] std::size_t theoretical_maximum_round_count(
    std::size_t point_count) {
  if (point_count <= 1U) {
    return 0U;
  }
  return static_cast<std::size_t>(std::bit_width(point_count - 1U));
}

[[nodiscard]] bool edge_less(
    const hierarchy::ExactEmstEdge& left,
    const hierarchy::ExactEmstEdge& right) {
  if (left.squared_length != right.squared_length) {
    return left.squared_length < right.squared_length;
  }
  if (left.u != right.u) {
    return left.u < right.u;
  }
  return left.v < right.v;
}

[[nodiscard]] bool add_without_overflow(
    std::size_t& target,
    std::size_t value) noexcept {
  if (value > std::numeric_limits<std::size_t>::max() - target) {
    return false;
  }
  target += value;
  return true;
}

[[nodiscard]] bool product_equals(
    std::size_t left,
    std::size_t right,
    std::size_t expected) noexcept {
  return left == 0U
             ? expected == 0U
             : right <= std::numeric_limits<std::size_t>::max() / left &&
                   left * right == expected;
}

[[nodiscard]] bool valid_chunking_policy(
    K1BoruvkaChunkingPolicy policy) noexcept {
  constexpr std::size_t bytes_per_budget_record =
      kChunkedCandidatePayloadCopies *
      kChunkedCandidateRecordSizeBytes;
  return policy.max_candidate_records_per_chunk != 0U &&
         policy.max_candidate_records_per_chunk <=
             std::numeric_limits<std::size_t>::max() /
                 bytes_per_budget_record;
}

[[nodiscard]] bool valid_morton_seed_policy(
    K1BoruvkaMortonSeedPolicy policy) noexcept {
  return policy.window_radius != 0U &&
         policy.window_radius <=
             std::numeric_limits<std::size_t>::max() / 2U;
}

[[nodiscard]] std::optional<std::size_t> expected_morton_inspections(
    std::size_t point_count,
    K1BoruvkaMortonSeedPolicy policy) noexcept {
  if (!valid_morton_seed_policy(policy)) {
    return std::nullopt;
  }
  if (point_count <= 1U) {
    return 0U;
  }
  const std::size_t effective_radius =
      std::min(policy.window_radius, point_count - 1U);
  std::size_t factor = point_count - 1U;
  if (!add_without_overflow(
          factor, point_count - effective_radius) ||
      (effective_radius != 0U &&
       factor > std::numeric_limits<std::size_t>::max() /
                    effective_radius)) {
    return std::nullopt;
  }
  return effective_radius * factor;
}

[[nodiscard]] bool morton_seed_audit_closes(
    const K1HybridBoruvkaRound& hybrid_round,
    std::size_t point_count,
    K1BoruvkaMortonSeedPolicy policy) noexcept {
  const K1BoruvkaMortonSeedAudit& audit =
      hybrid_round.morton_seed_audit;
  const std::optional<std::size_t> expected_inspections =
      expected_morton_inspections(point_count, policy);
  if (point_count <= 1U || !expected_inspections.has_value()) {
    return false;
  }
  std::size_t maximum_exact_seed_evaluations = point_count;
  if (!add_without_overflow(
          maximum_exact_seed_evaluations,
          audit.floating_proposal_count)) {
    return false;
  }
  const std::size_t inspection_budget_per_source = std::min(
      point_count - 1U, 2U * policy.window_radius);
  return hybrid_round.seed_status ==
             K1BoruvkaSeedStatus::
                 bounded_morton_window_external_exact_monotone_certified &&
         audit.source_count == point_count &&
         audit.window_radius == policy.window_radius &&
         audit.neighbor_inspection_budget_per_source ==
             inspection_budget_per_source &&
         audit.maximum_inspected_neighbor_count_per_source <=
             inspection_budget_per_source &&
         audit.inspected_neighbor_count == *expected_inspections &&
         audit.external_neighbor_count <= audit.inspected_neighbor_count &&
         audit.floating_proposal_count <= point_count &&
         audit.exact_selected_proposal_count <=
             audit.floating_proposal_count &&
         audit.exact_strict_improvement_count <=
             audit.exact_selected_proposal_count &&
         audit.exact_fallback_count ==
             point_count - audit.exact_selected_proposal_count &&
         audit.exact_seed_distance_evaluation_count >= point_count &&
         audit.exact_seed_distance_evaluation_count <=
             maximum_exact_seed_evaluations &&
         audit.gpu_kernel_launch_count == 1U &&
         audit.gpu_synchronization_count == 1U &&
         audit.complete_source_coverage_certified &&
         audit.bounded_window_certified &&
         audit.external_targets_recertified &&
         audit.exact_monotone_cutoff_certified;
}

[[nodiscard]] bool chunked_emission_audit_closes(
    const K1HybridBoruvkaRound& hybrid_round,
    std::size_t point_count,
    K1BoruvkaChunkingPolicy policy) noexcept {
  const K1BoruvkaCandidateAudit& proposal =
      hybrid_round.proposal_audit;
  const K1BoruvkaChunkedEmissionAudit& emission =
      hybrid_round.chunked_emission_audit;
  if (!valid_chunking_policy(policy) ||
      hybrid_round.chunked_emission_status !=
          K1BoruvkaEmissionStatus::
              complete_source_ranges_candidate_payload_bound_certified ||
      emission.logical_candidate_count == 0U ||
      emission.logical_candidate_count != proposal.gpu_candidate_count ||
      emission.source_chunk_count == 0U ||
      emission.source_chunk_count > point_count ||
      emission.peak_chunk_source_count == 0U ||
      emission.peak_chunk_source_count > point_count ||
      emission.peak_chunk_candidate_count == 0U ||
      emission.peak_chunk_candidate_count >
          policy.max_candidate_records_per_chunk ||
      emission.max_source_candidate_count == 0U ||
      emission.max_source_candidate_count >
          emission.peak_chunk_candidate_count ||
      emission.candidate_record_budget !=
          policy.max_candidate_records_per_chunk ||
      emission.device_candidate_capacity_high_water <
          emission.peak_chunk_candidate_count ||
      emission.device_candidate_capacity_high_water >
          policy.max_candidate_records_per_chunk ||
      emission.host_candidate_capacity_high_water <
          emission.peak_chunk_candidate_count ||
      emission.host_candidate_capacity_high_water >
          policy.max_candidate_records_per_chunk ||
      emission.candidate_record_size_bytes !=
          kChunkedCandidateRecordSizeBytes ||
      emission.count_kernel_launch_count != 1U ||
      emission.emit_kernel_launch_count != emission.source_chunk_count ||
      emission.synchronization_count !=
          emission.count_kernel_launch_count +
              emission.emit_kernel_launch_count ||
      proposal.gpu_kernel_launch_count !=
          emission.synchronization_count ||
      proposal.gpu_synchronization_count !=
          emission.synchronization_count ||
      !emission.complete_source_partition_certified ||
      !emission.count_emit_cardinality_and_visit_count_certified ||
      !emission.candidate_payload_physical_bound_certified) {
    return false;
  }

  std::size_t simultaneous_capacity =
      emission.device_candidate_capacity_high_water;
  if (!add_without_overflow(
          simultaneous_capacity,
          emission.host_candidate_capacity_high_water) ||
      !product_equals(
          simultaneous_capacity,
          emission.candidate_record_size_bytes,
          emission.candidate_payload_peak_bytes)) {
    return false;
  }
  const std::size_t bytes_per_budget_record =
      kChunkedCandidatePayloadCopies *
      emission.candidate_record_size_bytes;
  return emission.candidate_payload_peak_bytes <=
         policy.max_candidate_records_per_chunk *
             bytes_per_budget_record;
}

[[nodiscard]] bool proposal_audit_closes(
    const K1HybridBoruvkaRound& hybrid_round,
    std::size_t point_count,
    std::size_t node_count,
    std::uint64_t expected_epoch,
    K1HybridBoruvkaEmissionMode emission_mode,
    K1BoruvkaChunkingPolicy chunking_policy,
    K1BoruvkaSeedMode seed_mode,
    K1BoruvkaMortonSeedPolicy seed_policy) noexcept {
  const K1BoruvkaCandidateAudit& audit = hybrid_round.proposal_audit;
  const bool common_contract =
      hybrid_round.proposal_status ==
             K1HybridBoruvkaProposalStatus::candidate_superset_certified &&
         audit.resident_point_count == point_count &&
         audit.resident_node_count == node_count &&
         audit.frozen_component_count > 1U &&
         audit.frozen_component_count <= point_count &&
         audit.uniform_lbvh_node_count <= node_count &&
         audit.mixed_lbvh_node_count ==
             node_count - audit.uniform_lbvh_node_count &&
         audit.exact_seed_count == point_count &&
         audit.gpu_candidate_count == audit.gpu_output_capacity &&
         audit.gpu_count_pass_node_visit_count ==
             audit.gpu_emit_pass_node_visit_count &&
         audit.cpu_required_candidate_count <= audit.gpu_candidate_count &&
         audit.buffer_epoch == expected_epoch &&
         audit.frozen_labels_certified &&
         audit.rope_topology_certified &&
         audit.exact_capacity_certified &&
         audit.no_truncation_certified &&
         audit.candidate_superset_certified;
  if (!common_contract) {
    return false;
  }
  bool emission_closes = false;
  switch (emission_mode) {
    case K1HybridBoruvkaEmissionMode::monolithic_round_payload:
      emission_closes =
          chunking_policy.max_candidate_records_per_chunk == 0U &&
          hybrid_round.chunked_emission_status ==
              K1BoruvkaEmissionStatus::not_certified &&
          hybrid_round.chunked_emission_audit ==
              K1BoruvkaChunkedEmissionAudit{} &&
          audit.gpu_kernel_launch_count == 2U &&
          audit.gpu_synchronization_count == 2U;
      break;
    case K1HybridBoruvkaEmissionMode::bounded_complete_source_ranges:
      emission_closes = chunked_emission_audit_closes(
          hybrid_round, point_count, chunking_policy);
      break;
  }
  if (!emission_closes) {
    return false;
  }
  switch (seed_mode) {
    case K1BoruvkaSeedMode::canonical_external_fallback:
      return seed_policy.window_radius == 0U &&
             hybrid_round.seed_status ==
                 K1BoruvkaSeedStatus::not_certified &&
             hybrid_round.morton_seed_audit ==
                 K1BoruvkaMortonSeedAudit{};
    case K1BoruvkaSeedMode::gpu_morton_window_cpu_exact_monotone:
      return emission_mode == K1HybridBoruvkaEmissionMode::
                                  bounded_complete_source_ranges &&
             morton_seed_audit_closes(
                 hybrid_round, point_count, seed_policy);
  }
  return false;
}

[[nodiscard]] bool decision_audit_closes(
    const K1HybridBoruvkaRound& hybrid_round) noexcept {
  const K1BoruvkaCandidateAudit& audit = hybrid_round.proposal_audit;
  const K1HybridBoruvkaExactDecision& decision =
      hybrid_round.exact_decision;
  return hybrid_round.decision_status ==
             K1HybridBoruvkaDecisionStatus::
                 cpu_exact_kappa_minima_certified &&
         decision.frozen_component_count > 1U &&
         audit.frozen_component_count ==
             decision.frozen_component_count &&
         decision.component_minima.size() ==
             decision.frozen_component_count &&
         audit.cpu_exact_candidate_distance_evaluation_count ==
             audit.gpu_candidate_count &&
         audit.cpu_exact_resolution_complete;
}

[[nodiscard]] bool same_proposal_audit(
    const K1BoruvkaCandidateAudit& left,
    const K1BoruvkaCandidateAudit& right) noexcept {
  return left.resident_point_count == right.resident_point_count &&
         left.resident_node_count == right.resident_node_count &&
         left.frozen_component_count == right.frozen_component_count &&
         left.uniform_lbvh_node_count == right.uniform_lbvh_node_count &&
         left.mixed_lbvh_node_count == right.mixed_lbvh_node_count &&
         left.exact_seed_count == right.exact_seed_count &&
         left.gpu_candidate_count == right.gpu_candidate_count &&
         left.gpu_output_capacity == right.gpu_output_capacity &&
         left.gpu_kernel_launch_count == right.gpu_kernel_launch_count &&
         left.gpu_synchronization_count ==
             right.gpu_synchronization_count &&
         left.gpu_count_pass_node_visit_count ==
             right.gpu_count_pass_node_visit_count &&
         left.gpu_emit_pass_node_visit_count ==
             right.gpu_emit_pass_node_visit_count &&
         left.gpu_uniform_component_prune_count ==
             right.gpu_uniform_component_prune_count &&
         left.gpu_strict_aabb_prune_count ==
             right.gpu_strict_aabb_prune_count &&
         left.gpu_invalid_bound_descent_count ==
             right.gpu_invalid_bound_descent_count &&
         left.cpu_exact_aabb_bound_evaluation_count ==
             right.cpu_exact_aabb_bound_evaluation_count &&
         left.cpu_required_candidate_count ==
             right.cpu_required_candidate_count &&
         left.buffer_epoch == right.buffer_epoch &&
         left.proposal_digest_fnv1a == right.proposal_digest_fnv1a &&
         left.frozen_labels_certified == right.frozen_labels_certified &&
         left.rope_topology_certified == right.rope_topology_certified &&
         left.exact_capacity_certified == right.exact_capacity_certified &&
         left.no_truncation_certified == right.no_truncation_certified &&
         left.candidate_superset_certified ==
             right.candidate_superset_certified;
}

[[nodiscard]] bool same_decision_audit(
    const K1BoruvkaCandidateAudit& left,
    const K1BoruvkaCandidateAudit& right) noexcept {
  return left.cpu_exact_candidate_distance_evaluation_count ==
             right.cpu_exact_candidate_distance_evaluation_count &&
         left.cpu_exact_resolution_complete ==
             right.cpu_exact_resolution_complete;
}

[[nodiscard]] std::optional<K1HybridBoruvkaCounters> recompute_counters(
    const K1HybridBoruvkaResult& result,
    std::size_t node_count) noexcept {
  K1HybridBoruvkaCounters counters;
  counters.point_count = result.point_count;
  counters.lbvh_node_count = node_count;
  counters.round_count = result.rounds.size();
  counters.theoretical_max_round_count =
      theoretical_maximum_round_count(result.point_count);
  counters.final_component_count = result.point_count;

  for (std::size_t round_index = 0U;
       round_index < result.rounds.size();
       ++round_index) {
    const K1HybridBoruvkaRound& hybrid_round = result.rounds[round_index];
    const K1HybridBoruvkaExactDecision& decision =
        hybrid_round.exact_decision;
    const K1HybridBoruvkaCanonicalContraction& contraction =
        hybrid_round.canonical_contraction;
    const K1BoruvkaCandidateAudit& audit = hybrid_round.proposal_audit;
    if (!add_without_overflow(
            counters.frozen_component_label_count,
            result.point_count) ||
        !add_without_overflow(
            counters.component_minimum_count,
            decision.component_minima.size()) ||
        !add_without_overflow(
            counters.accepted_edge_count,
            contraction.accepted_edges.size()) ||
        decision.frozen_component_count <
            contraction.post_round_component_count ||
        !add_without_overflow(
            counters.component_contraction_count,
            decision.frozen_component_count -
                contraction.post_round_component_count) ||
        !add_without_overflow(
            counters.gpu_candidate_count,
            audit.gpu_candidate_count) ||
        !add_without_overflow(
            counters.gpu_output_capacity_sum,
            audit.gpu_output_capacity) ||
        !add_without_overflow(
            counters.gpu_kernel_launch_count,
            audit.gpu_kernel_launch_count) ||
        !add_without_overflow(
            counters.gpu_synchronization_count,
            audit.gpu_synchronization_count) ||
        !add_without_overflow(
            counters.gpu_count_pass_node_visit_count,
            audit.gpu_count_pass_node_visit_count) ||
        !add_without_overflow(
            counters.gpu_emit_pass_node_visit_count,
            audit.gpu_emit_pass_node_visit_count) ||
        !add_without_overflow(
            counters.gpu_uniform_component_prune_count,
            audit.gpu_uniform_component_prune_count) ||
        !add_without_overflow(
            counters.gpu_strict_aabb_prune_count,
            audit.gpu_strict_aabb_prune_count) ||
        !add_without_overflow(
            counters.gpu_invalid_bound_descent_count,
            audit.gpu_invalid_bound_descent_count) ||
        !add_without_overflow(
            counters.cpu_exact_aabb_bound_evaluation_count,
            audit.cpu_exact_aabb_bound_evaluation_count) ||
        !add_without_overflow(
            counters.cpu_required_candidate_count,
            audit.cpu_required_candidate_count) ||
        !add_without_overflow(
            counters.cpu_exact_candidate_distance_evaluation_count,
            audit.cpu_exact_candidate_distance_evaluation_count)) {
      return std::nullopt;
    }
    counters.peak_gpu_output_capacity = std::max(
        counters.peak_gpu_output_capacity,
        audit.gpu_output_capacity);
    if (round_index == 0U) {
      counters.first_buffer_epoch = audit.buffer_epoch;
    }
    counters.last_buffer_epoch = audit.buffer_epoch;
    counters.final_component_count =
        contraction.post_round_component_count;
  }
  return counters;
}

[[nodiscard]] std::optional<K1HybridBoruvkaChunkedEmissionCounters>
recompute_chunked_emission_counters(
    const K1HybridBoruvkaResult& result) noexcept {
  K1HybridBoruvkaChunkedEmissionCounters counters;
  switch (result.emission_mode) {
    case K1HybridBoruvkaEmissionMode::monolithic_round_payload:
      if (result.chunking_policy.max_candidate_records_per_chunk != 0U) {
        return std::nullopt;
      }
      for (const K1HybridBoruvkaRound& round : result.rounds) {
        if (round.chunked_emission_status !=
                K1BoruvkaEmissionStatus::not_certified ||
            round.chunked_emission_audit !=
                K1BoruvkaChunkedEmissionAudit{}) {
          return std::nullopt;
        }
      }
      return counters;
    case K1HybridBoruvkaEmissionMode::bounded_complete_source_ranges:
      break;
  }

  if (!valid_chunking_policy(result.chunking_policy)) {
    return std::nullopt;
  }
  counters.candidate_record_budget =
      result.chunking_policy.max_candidate_records_per_chunk;
  counters.candidate_record_size_bytes =
      kChunkedCandidateRecordSizeBytes;
  for (const K1HybridBoruvkaRound& round : result.rounds) {
    if (!chunked_emission_audit_closes(
            round, result.point_count, result.chunking_policy)) {
      return std::nullopt;
    }
    const K1BoruvkaChunkedEmissionAudit& emission =
        round.chunked_emission_audit;
    if (!add_without_overflow(counters.round_count, 1U) ||
        !add_without_overflow(
            counters.logical_candidate_count,
            emission.logical_candidate_count) ||
        !add_without_overflow(
            counters.source_chunk_count,
            emission.source_chunk_count) ||
        !add_without_overflow(
            counters.count_kernel_launch_count,
            emission.count_kernel_launch_count) ||
        !add_without_overflow(
            counters.emit_kernel_launch_count,
            emission.emit_kernel_launch_count) ||
        !add_without_overflow(
            counters.synchronization_count,
            emission.synchronization_count)) {
      return std::nullopt;
    }
    counters.peak_chunk_source_count = std::max(
        counters.peak_chunk_source_count,
        emission.peak_chunk_source_count);
    counters.peak_chunk_candidate_count = std::max(
        counters.peak_chunk_candidate_count,
        emission.peak_chunk_candidate_count);
    counters.max_source_candidate_count = std::max(
        counters.max_source_candidate_count,
        emission.max_source_candidate_count);
    counters.device_candidate_capacity_high_water = std::max(
        counters.device_candidate_capacity_high_water,
        emission.device_candidate_capacity_high_water);
    counters.host_candidate_capacity_high_water = std::max(
        counters.host_candidate_capacity_high_water,
        emission.host_candidate_capacity_high_water);
    counters.candidate_payload_peak_bytes = std::max(
        counters.candidate_payload_peak_bytes,
        emission.candidate_payload_peak_bytes);
  }
  return counters;
}

[[nodiscard]] std::optional<K1HybridBoruvkaMortonSeedCounters>
recompute_morton_seed_counters(
    const K1HybridBoruvkaResult& result) noexcept {
  K1HybridBoruvkaMortonSeedCounters counters;
  switch (result.seed_mode) {
    case K1BoruvkaSeedMode::canonical_external_fallback:
      if (result.morton_seed_policy.window_radius != 0U) {
        return std::nullopt;
      }
      for (const K1HybridBoruvkaRound& round : result.rounds) {
        if (round.seed_status != K1BoruvkaSeedStatus::not_certified ||
            round.morton_seed_audit != K1BoruvkaMortonSeedAudit{}) {
          return std::nullopt;
        }
      }
      return counters;
    case K1BoruvkaSeedMode::gpu_morton_window_cpu_exact_monotone:
      break;
  }

  if (!valid_morton_seed_policy(result.morton_seed_policy)) {
    return std::nullopt;
  }
  counters.window_radius = result.morton_seed_policy.window_radius;
  counters.neighbor_inspection_budget_per_source =
      result.point_count <= 1U
          ? 0U
          : std::min(
                result.point_count - 1U,
                2U * result.morton_seed_policy.window_radius);
  for (const K1HybridBoruvkaRound& round : result.rounds) {
    if (!morton_seed_audit_closes(
            round, result.point_count, result.morton_seed_policy)) {
      return std::nullopt;
    }
    const K1BoruvkaMortonSeedAudit& audit = round.morton_seed_audit;
    if (!add_without_overflow(counters.round_count, 1U) ||
        !add_without_overflow(counters.source_count, audit.source_count) ||
        !add_without_overflow(
            counters.inspected_neighbor_count,
            audit.inspected_neighbor_count) ||
        !add_without_overflow(
            counters.external_neighbor_count,
            audit.external_neighbor_count) ||
        !add_without_overflow(
            counters.floating_proposal_count,
            audit.floating_proposal_count) ||
        !add_without_overflow(
            counters.exact_selected_proposal_count,
            audit.exact_selected_proposal_count) ||
        !add_without_overflow(
            counters.exact_strict_improvement_count,
            audit.exact_strict_improvement_count) ||
        !add_without_overflow(
            counters.exact_fallback_count,
            audit.exact_fallback_count) ||
        !add_without_overflow(
            counters.exact_seed_distance_evaluation_count,
            audit.exact_seed_distance_evaluation_count) ||
        !add_without_overflow(
            counters.gpu_kernel_launch_count,
            audit.gpu_kernel_launch_count) ||
        !add_without_overflow(
            counters.gpu_synchronization_count,
            audit.gpu_synchronization_count)) {
      return std::nullopt;
    }
    counters.maximum_inspected_neighbor_count_per_source = std::max(
        counters.maximum_inspected_neighbor_count_per_source,
        audit.maximum_inspected_neighbor_count_per_source);
  }
  return counters;
}

[[nodiscard]] bool replay_canonical_contractions(
    const spatial::CanonicalPointCloud& cloud,
    const K1HybridBoruvkaResult& result) {
  if (result.point_count != cloud.size() || result.point_count == 0U) {
    return false;
  }
  std::vector<spatial::PointId> labels(result.point_count);
  for (std::size_t point_index = 0U;
       point_index < result.point_count;
       ++point_index) {
    labels[point_index] = checked_point_id(point_index);
  }
  std::size_t component_count = result.point_count;
  std::vector<hierarchy::ExactEmstEdge> replayed_edges;
  try {
    for (std::size_t round_index = 0U;
         round_index < result.rounds.size();
         ++round_index) {
      const K1HybridBoruvkaRound& round = result.rounds[round_index];
      const K1HybridBoruvkaExactDecision& decision =
          round.exact_decision;
      const K1HybridBoruvkaCanonicalContraction& observed_contraction =
          round.canonical_contraction;
      if (round.contraction_status !=
              K1HybridBoruvkaContractionStatus::
                  cpu_exact_canonical_contraction_certified ||
          decision.round_index != round_index ||
          decision.frozen_component_count != component_count) {
        return false;
      }
      hierarchy::K1BoruvkaRoundContraction contraction =
          hierarchy::contract_exact_k1_boruvka_round(
              cloud, labels, decision.component_minima);
      if (observed_contraction.accepted_edges !=
              contraction.accepted_edges ||
          observed_contraction.post_round_component_count !=
              contraction.post_round_component_count) {
        return false;
      }
      replayed_edges.insert(
          replayed_edges.end(),
          contraction.accepted_edges.begin(),
          contraction.accepted_edges.end());
      labels = std::move(contraction.post_round_component_labels);
      component_count = contraction.post_round_component_count;
    }
  } catch (const std::exception&) {
    return false;
  }
  std::sort(replayed_edges.begin(), replayed_edges.end(), edge_less);
  return component_count == 1U && replayed_edges == result.emst_edges;
}

struct K1HybridGpuReplay {
  bool emission_mode_certified{false};
  bool seed_mode_certified{false};
  bool proposal_chain_certified{false};
  bool bounded_candidate_emission_chain_certified{false};
  bool bounded_morton_seed_chain_certified{false};
  bool exact_decision_chain_certified{false};
  std::size_t round_count{};
  std::size_t component_minimum_count{};
  std::size_t kernel_launch_count{};
  std::size_t synchronization_count{};
  std::size_t source_chunk_count{};
  std::size_t peak_chunk_candidate_count{};
  std::size_t candidate_payload_peak_bytes{};
  std::size_t seed_inspected_neighbor_count{};
  std::size_t seed_selected_proposal_count{};
  std::size_t seed_strict_improvement_count{};
  std::size_t seed_kernel_launch_count{};
  std::size_t seed_synchronization_count{};
};

[[nodiscard]] K1HybridGpuReplay replay_gpu_proposal_chain(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const K1HybridBoruvkaResult& result,
    std::optional<K1BoruvkaChunkingPolicy> trusted_chunking_policy,
    std::optional<K1BoruvkaMortonSeedPolicy> trusted_seed_policy) {
  K1HybridGpuReplay replay;
  if (result.point_count != cloud.size() || result.point_count == 0U) {
    return replay;
  }
  const bool bounded_emission =
      result.emission_mode ==
      K1HybridBoruvkaEmissionMode::bounded_complete_source_ranges;
  const bool valid_emission_mode =
      (result.emission_mode ==
           K1HybridBoruvkaEmissionMode::monolithic_round_payload &&
       !trusted_chunking_policy.has_value() &&
       result.chunking_policy.max_candidate_records_per_chunk == 0U) ||
      (bounded_emission && trusted_chunking_policy.has_value() &&
       valid_chunking_policy(*trusted_chunking_policy) &&
       result.chunking_policy == *trusted_chunking_policy);
  const bool bounded_morton_seed =
      result.seed_mode ==
      K1BoruvkaSeedMode::gpu_morton_window_cpu_exact_monotone;
  const bool valid_seed_mode =
      (result.seed_mode ==
           K1BoruvkaSeedMode::canonical_external_fallback &&
       !trusted_seed_policy.has_value() &&
       result.morton_seed_policy.window_radius == 0U) ||
      (bounded_morton_seed && trusted_seed_policy.has_value() &&
       valid_morton_seed_policy(*trusted_seed_policy) &&
       result.morton_seed_policy == *trusted_seed_policy &&
       bounded_emission);
  if (!valid_emission_mode || !valid_seed_mode) {
    return replay;
  }
  if (result.point_count == 1U) {
    replay.emission_mode_certified = result.rounds.empty();
    replay.seed_mode_certified = result.rounds.empty();
    replay.proposal_chain_certified = result.rounds.empty();
    replay.bounded_candidate_emission_chain_certified =
        bounded_emission && result.rounds.empty();
    replay.bounded_morton_seed_chain_certified =
        bounded_morton_seed && result.rounds.empty();
    replay.exact_decision_chain_certified = result.rounds.empty();
    return replay;
  }

  std::vector<spatial::PointId> labels(result.point_count);
  for (std::size_t point_index = 0U;
       point_index < result.point_count;
       ++point_index) {
    labels[point_index] = checked_point_id(point_index);
  }
  std::size_t component_count = result.point_count;
  bool emission_chain = true;
  bool seed_chain = true;
  bool proposal_chain = true;
  bool decision_chain = true;
  try {
    K1BoruvkaCandidateContext context{index, cloud};
    for (const K1HybridBoruvkaRound& hybrid_round : result.rounds) {
      if (component_count <= 1U) {
        seed_chain = false;
        proposal_chain = false;
        decision_chain = false;
        break;
      }

      K1BoruvkaCandidateAudit proposal_audit;
      K1BoruvkaChunkedEmissionAudit chunked_emission_audit;
      K1BoruvkaMortonSeedAudit morton_seed_audit;
      K1BoruvkaEmissionStatus chunked_emission_status =
          K1BoruvkaEmissionStatus::not_certified;
      K1BoruvkaSeedStatus seed_status =
          K1BoruvkaSeedStatus::not_certified;
      std::vector<hierarchy::K1BoruvkaComponentMinimum> component_minima;
      if (bounded_emission) {
        K1BoruvkaChunkedRoundResolution proposal =
            bounded_morton_seed
                ? context.propose_round_chunked(
                      cloud,
                      labels,
                      *trusted_chunking_policy,
                      *trusted_seed_policy)
                : context.propose_round_chunked(
                      cloud, labels, *trusted_chunking_policy);
        proposal_audit = proposal.proposal_audit;
        chunked_emission_audit = proposal.emission_audit;
        chunked_emission_status = proposal.emission_status;
        morton_seed_audit = proposal.morton_seed_audit;
        seed_status = proposal.seed_status;
        component_minima = std::move(
            proposal.cpu_exact_component_minima);
      } else {
        K1BoruvkaRoundProposal proposal = context.propose_round(
            cloud, labels);
        if (proposal.frozen_component_labels != labels) {
          proposal_chain = false;
        }
        proposal_audit = proposal.audit;
        component_minima = std::move(
            proposal.cpu_exact_component_minima);
      }
      if (!same_proposal_audit(
              proposal_audit, hybrid_round.proposal_audit)) {
        proposal_chain = false;
      }
      if (chunked_emission_audit !=
              hybrid_round.chunked_emission_audit ||
          chunked_emission_status !=
              hybrid_round.chunked_emission_status) {
        emission_chain = false;
        proposal_chain = false;
      }
      if (morton_seed_audit != hybrid_round.morton_seed_audit ||
          seed_status != hybrid_round.seed_status) {
        seed_chain = false;
        proposal_chain = false;
      }
      if (!same_decision_audit(
              proposal_audit, hybrid_round.proposal_audit) ||
          component_minima !=
              hybrid_round.exact_decision.component_minima) {
        decision_chain = false;
      }
      ++replay.round_count;
      if (!add_without_overflow(
              replay.component_minimum_count,
              component_minima.size()) ||
          !add_without_overflow(
              replay.kernel_launch_count,
              proposal_audit.gpu_kernel_launch_count) ||
          !add_without_overflow(
              replay.synchronization_count,
              proposal_audit.gpu_synchronization_count) ||
          (bounded_emission &&
           !add_without_overflow(
               replay.source_chunk_count,
               chunked_emission_audit.source_chunk_count)) ||
          (bounded_morton_seed &&
           (!add_without_overflow(
                replay.seed_inspected_neighbor_count,
                morton_seed_audit.inspected_neighbor_count) ||
            !add_without_overflow(
                replay.seed_selected_proposal_count,
                morton_seed_audit.exact_selected_proposal_count) ||
            !add_without_overflow(
                replay.seed_strict_improvement_count,
                morton_seed_audit.exact_strict_improvement_count) ||
            !add_without_overflow(
                replay.seed_kernel_launch_count,
                morton_seed_audit.gpu_kernel_launch_count) ||
            !add_without_overflow(
                replay.seed_synchronization_count,
                morton_seed_audit.gpu_synchronization_count)))) {
        emission_chain = false;
        seed_chain = false;
        proposal_chain = false;
        decision_chain = false;
        break;
      }
      if (bounded_emission) {
        replay.peak_chunk_candidate_count = std::max(
            replay.peak_chunk_candidate_count,
            chunked_emission_audit.peak_chunk_candidate_count);
        replay.candidate_payload_peak_bytes = std::max(
            replay.candidate_payload_peak_bytes,
            chunked_emission_audit.candidate_payload_peak_bytes);
      }

      hierarchy::K1BoruvkaRoundContraction contraction =
          hierarchy::contract_exact_k1_boruvka_round(
              cloud,
              labels,
              component_minima);
      labels = std::move(contraction.post_round_component_labels);
      component_count = contraction.post_round_component_count;
      // proposal, including all candidate payloads, dies here.
    }
  } catch (const std::exception&) {
    return replay;
  }
  replay.emission_mode_certified =
      emission_chain && component_count == 1U;
  replay.seed_mode_certified =
      seed_chain && component_count == 1U;
  replay.proposal_chain_certified =
      emission_chain && seed_chain && proposal_chain &&
      component_count == 1U;
  replay.bounded_candidate_emission_chain_certified =
      bounded_emission && emission_chain && proposal_chain &&
      component_count == 1U;
  replay.bounded_morton_seed_chain_certified =
      bounded_morton_seed && seed_chain && proposal_chain &&
      component_count == 1U;
  replay.exact_decision_chain_certified =
      decision_chain && component_count == 1U;
  return replay;
}

}  // namespace

namespace {

[[nodiscard]] K1HybridBoruvkaResult build_hybrid_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::optional<K1BoruvkaChunkingPolicy> chunking_policy,
    std::optional<K1BoruvkaMortonSeedPolicy> seed_policy) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "hybrid K1 Boruvka requires a ready LBVH for the same point namespace");
  }
  const std::size_t point_count = cloud.size();
  K1HybridBoruvkaResult result;
  result.point_count = point_count;
  if (chunking_policy.has_value()) {
    if (!valid_chunking_policy(*chunking_policy)) {
      throw std::invalid_argument(
          "hybrid K1 Boruvka needs a finite nonzero candidate chunk budget");
    }
    result.emission_mode = K1HybridBoruvkaEmissionMode::
        bounded_complete_source_ranges;
    result.chunking_policy = *chunking_policy;
  }
  if (seed_policy.has_value()) {
    if (!chunking_policy.has_value() ||
        !valid_morton_seed_policy(*seed_policy)) {
      throw std::invalid_argument(
          "hybrid K1 Boruvka Morton seeds require bounded emission and a "
          "finite nonzero window radius");
    }
    result.seed_mode =
        K1BoruvkaSeedMode::gpu_morton_window_cpu_exact_monotone;
    result.morton_seed_policy = *seed_policy;
  }

  std::vector<spatial::PointId> component_labels(point_count);
  for (std::size_t point_index = 0U;
       point_index < point_count;
       ++point_index) {
    component_labels[point_index] = checked_point_id(point_index);
  }
  std::size_t component_count = point_count;
  const std::size_t maximum_round_count =
      theoretical_maximum_round_count(point_count);
  if (component_count > 1U) {
    // One context owns the immutable resident LBVH and reuses its private
    // candidate buffers for the complete producer chain.
    K1BoruvkaCandidateContext context{index, cloud};
    while (component_count > 1U) {
      const std::size_t round_index = result.rounds.size();
      if (round_index >= maximum_round_count) {
        throw std::logic_error(
            "hybrid K1 Boruvka exceeded ceil(log2(n)) rounds");
      }
      K1BoruvkaCandidateAudit proposal_audit;
      K1BoruvkaChunkedEmissionAudit chunked_emission_audit;
      K1BoruvkaMortonSeedAudit morton_seed_audit;
      K1BoruvkaEmissionStatus chunked_emission_status =
          K1BoruvkaEmissionStatus::not_certified;
      K1BoruvkaSeedStatus seed_status =
          K1BoruvkaSeedStatus::not_certified;
      std::vector<hierarchy::K1BoruvkaComponentMinimum> component_minima;
      if (chunking_policy.has_value()) {
        K1BoruvkaChunkedRoundResolution proposal =
            seed_policy.has_value()
                ? context.propose_round_chunked(
                      cloud,
                      component_labels,
                      *chunking_policy,
                      *seed_policy)
                : context.propose_round_chunked(
                      cloud, component_labels, *chunking_policy);
        proposal_audit = proposal.proposal_audit;
        chunked_emission_audit = proposal.emission_audit;
        chunked_emission_status = proposal.emission_status;
        morton_seed_audit = proposal.morton_seed_audit;
        seed_status = proposal.seed_status;
        component_minima = std::move(
            proposal.cpu_exact_component_minima);
      } else {
        K1BoruvkaRoundProposal proposal = context.propose_round(
            cloud, component_labels);
        proposal_audit = proposal.audit;
        component_minima = std::move(
            proposal.cpu_exact_component_minima);
      }
      hierarchy::K1BoruvkaRoundContraction contraction =
          hierarchy::contract_exact_k1_boruvka_round(
              cloud,
              component_labels,
              component_minima);

      result.emst_edges.insert(
          result.emst_edges.end(),
          contraction.accepted_edges.begin(),
          contraction.accepted_edges.end());
      K1HybridBoruvkaRound hybrid_round;
      hybrid_round.proposal_audit = proposal_audit;
      hybrid_round.chunked_emission_audit =
          chunked_emission_audit;
      hybrid_round.morton_seed_audit = morton_seed_audit;
      hybrid_round.chunked_emission_status =
          chunked_emission_status;
      hybrid_round.seed_status = seed_status;
      hybrid_round.exact_decision.round_index = round_index;
      hybrid_round.exact_decision.frozen_component_count =
          component_count;
      hybrid_round.exact_decision.component_minima =
          std::move(component_minima);
      hybrid_round.canonical_contraction.accepted_edges =
          std::move(contraction.accepted_edges);
      hybrid_round.canonical_contraction.post_round_component_count =
          contraction.post_round_component_count;
      hybrid_round.proposal_status =
          K1HybridBoruvkaProposalStatus::candidate_superset_certified;
      hybrid_round.decision_status =
          K1HybridBoruvkaDecisionStatus::
              cpu_exact_kappa_minima_certified;
      hybrid_round.contraction_status =
          K1HybridBoruvkaContractionStatus::
              cpu_exact_canonical_contraction_certified;
      result.rounds.push_back(std::move(hybrid_round));
      component_labels = std::move(
          contraction.post_round_component_labels);
      component_count = contraction.post_round_component_count;
      // proposal, including seeds, offsets and candidates, dies here.
    }
  }

  std::sort(result.emst_edges.begin(), result.emst_edges.end(), edge_less);
  if (result.emst_edges.size() != point_count - 1U ||
      std::adjacent_find(
          result.emst_edges.begin(), result.emst_edges.end()) !=
          result.emst_edges.end()) {
    throw std::logic_error(
        "the hybrid K1 Boruvka witness is not a canonical tree");
  }

  exact::ExactRational total_squared_weight;
  exact::ExactRational total_hgp_weight;
  for (const hierarchy::ExactEmstEdge& edge : result.emst_edges) {
    total_squared_weight =
        total_squared_weight + edge.squared_length.rational();
    total_hgp_weight =
        total_hgp_weight + edge.merge_level.rational();
  }
  result.total_squared_weight =
      exact::ExactLevel{std::move(total_squared_weight)};
  result.total_hgp_weight =
      exact::ExactLevel{std::move(total_hgp_weight)};

  const std::optional<K1HybridBoruvkaCounters> counters =
      recompute_counters(result, index.build_counters().node_count);
  if (!counters.has_value() ||
      counters->final_component_count != 1U ||
      counters->round_count > counters->theoretical_max_round_count ||
      counters->accepted_edge_count != point_count - 1U ||
      counters->component_contraction_count != point_count - 1U) {
    throw std::logic_error(
        "the hybrid K1 Boruvka counters do not close");
  }
  result.counters = *counters;
  const std::optional<K1HybridBoruvkaChunkedEmissionCounters>
      chunked_counters = recompute_chunked_emission_counters(result);
  if (!chunked_counters.has_value()) {
    throw std::logic_error(
        "the hybrid K1 Boruvka chunked-emission counters do not close");
  }
  result.chunked_emission_counters = *chunked_counters;
  const std::optional<K1HybridBoruvkaMortonSeedCounters> seed_counters =
      recompute_morton_seed_counters(result);
  if (!seed_counters.has_value()) {
    throw std::logic_error(
        "the hybrid K1 Boruvka Morton-seed counters do not close");
  }
  result.morton_seed_counters = *seed_counters;

  const K1HybridBoruvkaVerification verification =
      seed_policy.has_value()
          ? verify_gpu_proposed_cpu_exact_k1_boruvka(
                index,
                cloud,
                *chunking_policy,
                *seed_policy,
                result)
          : chunking_policy.has_value()
                ? verify_gpu_proposed_cpu_exact_k1_boruvka(
                      index, cloud, *chunking_policy, result)
                : verify_gpu_proposed_cpu_exact_k1_boruvka(
                      index, cloud, result);
  if (!verification.emst_witness_certified) {
    throw std::logic_error(
        "the hybrid K1 Boruvka witness failed its independent replay");
  }
  result.proposal_chain_certified =
      verification.proposal_chain_certified;
  result.bounded_candidate_emission_chain_certified =
      verification.bounded_candidate_emission_chain_certified;
  result.bounded_morton_seed_chain_certified =
      verification.bounded_morton_seed_chain_certified;
  result.cpu_exact_decision_chain_certified =
      verification.cpu_exact_decision_chain_certified;
  result.canonical_contraction_chain_certified =
      verification.canonical_contractions_certified;
  result.reference_cpu_witness_certified =
      verification.reference_cpu_witness_certified;
  result.emst_witness_certified = true;
  return result;
}

}  // namespace

K1HybridBoruvkaResult build_gpu_proposed_cpu_exact_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud) {
  return build_hybrid_k1_boruvka(
      index, cloud, std::nullopt, std::nullopt);
}

K1HybridBoruvkaResult build_gpu_proposed_cpu_exact_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaChunkingPolicy chunking_policy) {
  return build_hybrid_k1_boruvka(
      index, cloud, chunking_policy, std::nullopt);
}

K1HybridBoruvkaResult build_gpu_proposed_cpu_exact_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaChunkingPolicy chunking_policy,
    K1BoruvkaMortonSeedPolicy seed_policy) {
  return build_hybrid_k1_boruvka(
      index, cloud, chunking_policy, seed_policy);
}

namespace {

[[nodiscard]] K1HybridBoruvkaVerification verify_hybrid_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::optional<K1BoruvkaChunkingPolicy> trusted_chunking_policy,
    std::optional<K1BoruvkaMortonSeedPolicy> trusted_seed_policy,
    const K1HybridBoruvkaResult& result) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "hybrid K1 Boruvka verification requires a matching ready LBVH");
  }
  const hierarchy::K1ExactBoruvkaResult expected =
      hierarchy::build_exact_lbvh_boruvka(index, cloud);
  K1HybridBoruvkaVerification verification;
  verification.index_identity_certified = true;
  verification.reference_round_count = expected.rounds.size();
  verification.reference_component_minimum_count =
      expected.counters.component_minimum_count;

  const bool trusted_emission_policy_matches =
      (result.emission_mode ==
           K1HybridBoruvkaEmissionMode::monolithic_round_payload &&
       !trusted_chunking_policy.has_value() &&
       result.chunking_policy.max_candidate_records_per_chunk == 0U) ||
      (result.emission_mode ==
           K1HybridBoruvkaEmissionMode::bounded_complete_source_ranges &&
       trusted_chunking_policy.has_value() &&
       valid_chunking_policy(*trusted_chunking_policy) &&
       result.chunking_policy == *trusted_chunking_policy);
  const bool trusted_seed_policy_matches =
      (result.seed_mode ==
           K1BoruvkaSeedMode::canonical_external_fallback &&
       !trusted_seed_policy.has_value() &&
       result.morton_seed_policy.window_radius == 0U) ||
      (result.seed_mode ==
           K1BoruvkaSeedMode::gpu_morton_window_cpu_exact_monotone &&
       result.emission_mode ==
           K1HybridBoruvkaEmissionMode::bounded_complete_source_ranges &&
       trusted_seed_policy.has_value() &&
       valid_morton_seed_policy(*trusted_seed_policy) &&
       result.morton_seed_policy == *trusted_seed_policy);
  const K1BoruvkaChunkingPolicy checked_chunking_policy =
      trusted_chunking_policy.value_or(K1BoruvkaChunkingPolicy{});
  const K1BoruvkaMortonSeedPolicy checked_seed_policy =
      trusted_seed_policy.value_or(K1BoruvkaMortonSeedPolicy{});

  const std::optional<K1HybridBoruvkaChunkedEmissionCounters>
      recomputed_chunked_counters =
          recompute_chunked_emission_counters(result);
  const std::optional<K1HybridBoruvkaMortonSeedCounters>
      recomputed_seed_counters = recompute_morton_seed_counters(result);
  bool proposal_chain = trusted_emission_policy_matches &&
                        trusted_seed_policy_matches &&
                        result.point_count == cloud.size() &&
                        recomputed_chunked_counters.has_value() &&
                        recomputed_seed_counters.has_value();
  for (std::size_t round_index = 0U;
       round_index < result.rounds.size();
       ++round_index) {
    const K1HybridBoruvkaRound& round = result.rounds[round_index];
    if (!proposal_audit_closes(
            round,
            result.point_count,
            expected.counters.lbvh_node_count,
            static_cast<std::uint64_t>(round_index) + std::uint64_t{1},
            result.emission_mode,
            checked_chunking_policy,
            result.seed_mode,
            checked_seed_policy)) {
      proposal_chain = false;
    }
  }
  const K1HybridGpuReplay gpu_replay =
      replay_gpu_proposal_chain(
          index,
          cloud,
          result,
          trusted_chunking_policy,
          trusted_seed_policy);
  verification.emission_mode_certified =
      trusted_emission_policy_matches &&
      recomputed_chunked_counters.has_value() &&
      *recomputed_chunked_counters == result.chunked_emission_counters &&
      gpu_replay.emission_mode_certified;
  verification.seed_mode_certified =
      trusted_seed_policy_matches && recomputed_seed_counters.has_value() &&
      *recomputed_seed_counters == result.morton_seed_counters &&
      gpu_replay.seed_mode_certified;
  verification.bounded_candidate_emission_chain_certified =
      result.emission_mode ==
          K1HybridBoruvkaEmissionMode::bounded_complete_source_ranges &&
      verification.emission_mode_certified &&
      gpu_replay.bounded_candidate_emission_chain_certified;
  verification.bounded_morton_seed_chain_certified =
      result.seed_mode ==
          K1BoruvkaSeedMode::gpu_morton_window_cpu_exact_monotone &&
      verification.seed_mode_certified &&
      gpu_replay.bounded_morton_seed_chain_certified;
  verification.proposal_chain_certified =
      verification.emission_mode_certified &&
      verification.seed_mode_certified && proposal_chain &&
      gpu_replay.proposal_chain_certified;
  verification.gpu_replayed_round_count = gpu_replay.round_count;
  verification.gpu_replayed_component_minimum_count =
      gpu_replay.component_minimum_count;
  verification.gpu_replay_kernel_launch_count =
      gpu_replay.kernel_launch_count;
  verification.gpu_replay_synchronization_count =
      gpu_replay.synchronization_count;
  verification.gpu_replay_source_chunk_count =
      gpu_replay.source_chunk_count;
  verification.gpu_replay_peak_chunk_candidate_count =
      gpu_replay.peak_chunk_candidate_count;
  verification.gpu_replay_candidate_payload_peak_bytes =
      gpu_replay.candidate_payload_peak_bytes;
  verification.gpu_replay_seed_inspected_neighbor_count =
      gpu_replay.seed_inspected_neighbor_count;
  verification.gpu_replay_seed_selected_proposal_count =
      gpu_replay.seed_selected_proposal_count;
  verification.gpu_replay_seed_strict_improvement_count =
      gpu_replay.seed_strict_improvement_count;
  verification.gpu_replay_seed_kernel_launch_count =
      gpu_replay.seed_kernel_launch_count;
  verification.gpu_replay_seed_synchronization_count =
      gpu_replay.seed_synchronization_count;

  bool exact_decisions =
      result.point_count == expected.point_count &&
      result.rounds.size() == expected.rounds.size();
  if (exact_decisions) {
    for (std::size_t round_index = 0U;
         round_index < result.rounds.size();
         ++round_index) {
      const K1HybridBoruvkaRound& observed =
          result.rounds[round_index];
      const hierarchy::K1BoruvkaRound& reference =
          expected.rounds[round_index];
      if (!decision_audit_closes(observed) ||
          observed.exact_decision.round_index != reference.round_index ||
          observed.exact_decision.frozen_component_count !=
              reference.pre_round_component_count ||
          observed.exact_decision.component_minima !=
              reference.component_minima) {
        exact_decisions = false;
        break;
      }
    }
  }
  verification.cpu_exact_decision_chain_certified =
      exact_decisions && gpu_replay.exact_decision_chain_certified;
  verification.canonical_contractions_certified =
      replay_canonical_contractions(cloud, result);
  verification.round_count_bound_certified =
      result.point_count == cloud.size() &&
      result.rounds.size() <=
          theoretical_maximum_round_count(result.point_count);
  verification.spanning_tree_certified =
      result.point_count == expected.point_count &&
      result.emst_edges == expected.emst_edges;
  verification.exact_weights_certified =
      result.total_squared_weight == expected.total_squared_weight &&
      result.total_hgp_weight == expected.total_hgp_weight;
  verification.reference_cpu_witness_certified =
      expected.emst_witness_certified &&
      verification.cpu_exact_decision_chain_certified &&
      verification.spanning_tree_certified &&
      verification.exact_weights_certified;

  const std::optional<K1HybridBoruvkaCounters> recomputed =
      recompute_counters(result, expected.counters.lbvh_node_count);
  verification.counters_certified =
      recomputed.has_value() && *recomputed == result.counters &&
      recomputed_chunked_counters.has_value() &&
      *recomputed_chunked_counters == result.chunked_emission_counters &&
      recomputed_seed_counters.has_value() &&
      *recomputed_seed_counters == result.morton_seed_counters &&
      result.point_count != 0U &&
      recomputed->final_component_count == 1U &&
      recomputed->accepted_edge_count == result.point_count - 1U &&
      recomputed->component_contraction_count == result.point_count - 1U;
  verification.hierarchy_status_separation_certified =
      result.hierarchy_reduction_status ==
          K1HybridHierarchyReductionStatus::not_performed &&
      result.scientific_status ==
          K1HybridScientificStatus::local_emst_witness_only;
  verification.emst_witness_certified =
      verification.index_identity_certified &&
      verification.emission_mode_certified &&
      verification.seed_mode_certified &&
      verification.proposal_chain_certified &&
      verification.cpu_exact_decision_chain_certified &&
      verification.canonical_contractions_certified &&
      verification.round_count_bound_certified &&
      verification.spanning_tree_certified &&
      verification.exact_weights_certified &&
      verification.reference_cpu_witness_certified &&
      verification.counters_certified &&
      verification.hierarchy_status_separation_certified;
  return verification;
}

}  // namespace

K1HybridBoruvkaVerification
verify_gpu_proposed_cpu_exact_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const K1HybridBoruvkaResult& result) {
  return verify_hybrid_k1_boruvka(
      index, cloud, std::nullopt, std::nullopt, result);
}

K1HybridBoruvkaVerification
verify_gpu_proposed_cpu_exact_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaChunkingPolicy trusted_chunking_policy,
    const K1HybridBoruvkaResult& result) {
  return verify_hybrid_k1_boruvka(
      index,
      cloud,
      trusted_chunking_policy,
      std::nullopt,
      result);
}

K1HybridBoruvkaVerification
verify_gpu_proposed_cpu_exact_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    K1BoruvkaChunkingPolicy trusted_chunking_policy,
    K1BoruvkaMortonSeedPolicy trusted_seed_policy,
    const K1HybridBoruvkaResult& result) {
  return verify_hybrid_k1_boruvka(
      index,
      cloud,
      trusted_chunking_policy,
      trusted_seed_policy,
      result);
}

}  // namespace morsehgp3d::gpu
