#include "morsehgp3d/gpu/k1_boruvka.hpp"

#include "morsehgp3d/exact/point.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
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
using morsehgp3d::gpu::K1HybridBoruvkaContractionStatus;
using morsehgp3d::gpu::K1HybridBoruvkaDecisionStatus;
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
  K1HybridBoruvkaResult producer;
  K1HybridBoruvkaVerification verifier;
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
    std::size_t node_count) {
  const K1BoruvkaCandidateAudit& audit = round.proposal_audit;
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
          audit.gpu_kernel_launch_count == 2U &&
          audit.gpu_synchronization_count == 2U &&
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
}

void require_case_contract(const ReplayCase& replay_case) {
  const K1HybridBoruvkaResult& result = replay_case.producer;
  const K1HybridBoruvkaVerification& verification = replay_case.verifier;
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
          result.proposal_chain_certified &&
          result.cpu_exact_decision_chain_certified &&
          result.canonical_contraction_chain_certified &&
          result.reference_cpu_witness_certified &&
          result.emst_witness_certified,
      "a full-loop replay producer certificate does not close");
  require(
      verification.index_identity_certified &&
          verification.proposal_chain_certified &&
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
              result.counters.gpu_synchronization_count,
      "a full-loop replay verifier counters disagree with the producer");
  for (std::size_t round_index = 0U;
       round_index < result.rounds.size();
       ++round_index) {
    require_round_contract(
        result.rounds[round_index],
        round_index,
        replay_case.expected_component_count_path[round_index],
        replay_case.expected_component_count_path[round_index + 1U],
        result.point_count,
        result.counters.lbvh_node_count);
  }
}

template <std::size_t PointCount, std::size_t PathSize>
[[nodiscard]] ReplayCase run_fixture(
    std::string_view fixture,
    const std::array<CertifiedPoint3, PointCount>& input,
    const std::array<std::size_t, PathSize>& expected_path) {
  const CanonicalPointCloud cloud = canonical_cloud(input);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  K1HybridBoruvkaResult producer =
      build_gpu_proposed_cpu_exact_k1_boruvka(index, cloud);
  K1HybridBoruvkaVerification verifier =
      verify_gpu_proposed_cpu_exact_k1_boruvka(index, cloud, producer);
  ReplayCase replay_case{
      fixture,
      std::vector<std::size_t>{expected_path.begin(), expected_path.end()},
      std::move(producer),
      std::move(verifier)};
  require_case_contract(replay_case);
  return replay_case;
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

void write_producer(std::ostream& output, const K1HybridBoruvkaResult& result) {
  const auto& counters = result.counters;
  output << "{\"certificates\":{\"canonical_contraction_chain_certified\":";
  write_boolean(output, result.canonical_contraction_chain_certified);
  output << ",\"cpu_exact_decision_chain_certified\":";
  write_boolean(output, result.cpu_exact_decision_chain_certified);
  output << ",\"emst_witness_certified\":";
  write_boolean(output, result.emst_witness_certified);
  output << ",\"proposal_chain_certified\":";
  write_boolean(output, result.proposal_chain_certified);
  output << ",\"reference_cpu_witness_certified\":";
  write_boolean(output, result.reference_cpu_witness_certified);
  output << "},\"counters\":{\"accepted_edge_count\":"
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
         << counters.theoretical_max_round_count << "}}";
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
         << "\"},\"proposal_status\":\""
         << proposal_status_name(round.proposal_status) << "\"}";
}

void write_verifier(
    std::ostream& output, const K1HybridBoruvkaVerification& verification) {
  output << "{\"certificates\":{\"canonical_contractions_certified\":";
  write_boolean(output, verification.canonical_contractions_certified);
  output << ",\"counters_certified\":";
  write_boolean(output, verification.counters_certified);
  output << ",\"cpu_exact_decision_chain_certified\":";
  write_boolean(output, verification.cpu_exact_decision_chain_certified);
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
  output << ",\"spanning_tree_certified\":";
  write_boolean(output, verification.spanning_tree_certified);
  output << "},\"counters\":{\"gpu_replay_kernel_launch_count\":"
         << verification.gpu_replay_kernel_launch_count
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
         << "\",\"point_count\":" << result.point_count
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
  output << "],\"status\":\"passed\",\"verifier\":";
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
    cases.reserve(3U);
    cases.push_back(run_fixture(
        "singleton_terminal",
        singleton,
        std::array<std::size_t, 1>{1U}));
    cases.push_back(run_fixture(
        "chain_three_rounds",
        chain,
        std::array<std::size_t, 4>{8U, 4U, 2U, 1U}));
    cases.push_back(run_fixture(
        "square_equal_length_ties",
        square,
        std::array<std::size_t, 2>{4U, 1U}));

    std::cout << "{\"case_count\":" << cases.size() << ",\"cases\":[";
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
        << "\",\"mode\":\"certified\",\"phase\":\"5\""
        << ",\"profile\":\"hgp_reduced\",\"proof_basis\":\""
        << K1HybridBoruvkaResult::proof_basis << "\""
        << ",\"proposal_backend\":\"cuda_g4\""
        << ",\"proposal_semantics\":\""
        << K1BoruvkaCandidateAudit::proposal_semantics << "\""
        << ",\"schema\":\"morsehgp3d.phase5.k1_boruvka_full_loop_gpu_replay.v1\""
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
