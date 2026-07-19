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

[[nodiscard]] bool proposal_audit_closes(
    const K1HybridBoruvkaRound& hybrid_round,
    std::size_t point_count,
    std::size_t node_count,
    std::uint64_t expected_epoch) noexcept {
  const K1BoruvkaCandidateAudit& audit = hybrid_round.proposal_audit;
  return hybrid_round.proposal_status ==
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
         audit.gpu_kernel_launch_count == 2U &&
         audit.gpu_synchronization_count == 2U &&
         audit.gpu_count_pass_node_visit_count ==
             audit.gpu_emit_pass_node_visit_count &&
         audit.cpu_required_candidate_count <= audit.gpu_candidate_count &&
         audit.buffer_epoch == expected_epoch &&
         audit.frozen_labels_certified &&
         audit.rope_topology_certified &&
         audit.exact_capacity_certified &&
         audit.no_truncation_certified &&
         audit.candidate_superset_certified;
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
  bool proposal_chain_certified{false};
  bool exact_decision_chain_certified{false};
  std::size_t round_count{};
  std::size_t component_minimum_count{};
  std::size_t kernel_launch_count{};
  std::size_t synchronization_count{};
};

[[nodiscard]] K1HybridGpuReplay replay_gpu_proposal_chain(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const K1HybridBoruvkaResult& result) {
  K1HybridGpuReplay replay;
  if (result.point_count != cloud.size() || result.point_count == 0U) {
    return replay;
  }
  if (result.point_count == 1U) {
    replay.proposal_chain_certified = result.rounds.empty();
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
  bool proposal_chain = true;
  bool decision_chain = true;
  try {
    K1BoruvkaCandidateContext context{index, cloud};
    for (const K1HybridBoruvkaRound& hybrid_round : result.rounds) {
      if (component_count <= 1U) {
        proposal_chain = false;
        decision_chain = false;
        break;
      }

      K1BoruvkaRoundProposal proposal = context.propose_round(
          cloud, labels);
      if (proposal.frozen_component_labels != labels ||
          !same_proposal_audit(
              proposal.audit, hybrid_round.proposal_audit)) {
        proposal_chain = false;
      }
      if (!same_decision_audit(
              proposal.audit, hybrid_round.proposal_audit) ||
          proposal.cpu_exact_component_minima !=
              hybrid_round.exact_decision.component_minima) {
        decision_chain = false;
      }
      ++replay.round_count;
      if (!add_without_overflow(
              replay.component_minimum_count,
              proposal.cpu_exact_component_minima.size()) ||
          !add_without_overflow(
              replay.kernel_launch_count,
              proposal.audit.gpu_kernel_launch_count) ||
          !add_without_overflow(
              replay.synchronization_count,
              proposal.audit.gpu_synchronization_count)) {
        proposal_chain = false;
        decision_chain = false;
        break;
      }

      hierarchy::K1BoruvkaRoundContraction contraction =
          hierarchy::contract_exact_k1_boruvka_round(
              cloud,
              labels,
              proposal.cpu_exact_component_minima);
      labels = std::move(contraction.post_round_component_labels);
      component_count = contraction.post_round_component_count;
      // proposal, including all candidate payloads, dies here.
    }
  } catch (const std::exception&) {
    return replay;
  }
  replay.proposal_chain_certified =
      proposal_chain && component_count == 1U;
  replay.exact_decision_chain_certified =
      decision_chain && component_count == 1U;
  return replay;
}

}  // namespace

K1HybridBoruvkaResult build_gpu_proposed_cpu_exact_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "hybrid K1 Boruvka requires a ready LBVH for the same point namespace");
  }
  const std::size_t point_count = cloud.size();
  K1HybridBoruvkaResult result;
  result.point_count = point_count;

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
      K1BoruvkaRoundProposal proposal = context.propose_round(
          cloud, component_labels);
      hierarchy::K1BoruvkaRoundContraction contraction =
          hierarchy::contract_exact_k1_boruvka_round(
              cloud,
              component_labels,
              proposal.cpu_exact_component_minima);

      result.emst_edges.insert(
          result.emst_edges.end(),
          contraction.accepted_edges.begin(),
          contraction.accepted_edges.end());
      K1HybridBoruvkaRound hybrid_round;
      hybrid_round.proposal_audit = proposal.audit;
      hybrid_round.exact_decision.round_index = round_index;
      hybrid_round.exact_decision.frozen_component_count =
          component_count;
      hybrid_round.exact_decision.component_minima = std::move(
          proposal.cpu_exact_component_minima);
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

  const K1HybridBoruvkaVerification verification =
      verify_gpu_proposed_cpu_exact_k1_boruvka(index, cloud, result);
  if (!verification.emst_witness_certified) {
    throw std::logic_error(
        "the hybrid K1 Boruvka witness failed its independent replay");
  }
  result.proposal_chain_certified =
      verification.proposal_chain_certified;
  result.cpu_exact_decision_chain_certified =
      verification.cpu_exact_decision_chain_certified;
  result.canonical_contraction_chain_certified =
      verification.canonical_contractions_certified;
  result.reference_cpu_witness_certified =
      verification.reference_cpu_witness_certified;
  result.emst_witness_certified = true;
  return result;
}

K1HybridBoruvkaVerification
verify_gpu_proposed_cpu_exact_k1_boruvka(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
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

  bool proposal_chain = result.point_count == cloud.size();
  for (std::size_t round_index = 0U;
       round_index < result.rounds.size();
       ++round_index) {
    const K1HybridBoruvkaRound& round = result.rounds[round_index];
    if (!proposal_audit_closes(
            round,
            result.point_count,
            expected.counters.lbvh_node_count,
            static_cast<std::uint64_t>(round_index) + std::uint64_t{1})) {
      proposal_chain = false;
    }
  }
  const K1HybridGpuReplay gpu_replay =
      replay_gpu_proposal_chain(index, cloud, result);
  verification.proposal_chain_certified =
      proposal_chain && gpu_replay.proposal_chain_certified;
  verification.gpu_replayed_round_count = gpu_replay.round_count;
  verification.gpu_replayed_component_minimum_count =
      gpu_replay.component_minimum_count;
  verification.gpu_replay_kernel_launch_count =
      gpu_replay.kernel_launch_count;
  verification.gpu_replay_synchronization_count =
      gpu_replay.synchronization_count;

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

}  // namespace morsehgp3d::gpu
