#include "morsehgp3d/gpu/k1_boruvka.hpp"

#include "morsehgp3d/exact/point.hpp"

#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::K1BoruvkaCandidateContext;
using morsehgp3d::gpu::K1BoruvkaRoundProposal;
using morsehgp3d::hierarchy::K1ExactBoruvkaResult;
using morsehgp3d::hierarchy::build_exact_lbvh_boruvka;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

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

void require_proposal_contract(
    const K1BoruvkaRoundProposal& proposal,
    std::size_t point_count,
    std::size_t component_count) {
  if (proposal.candidate_offsets.size() != point_count + 1U ||
      proposal.candidate_offsets.front() != 0U ||
      proposal.candidate_offsets.back() != proposal.candidates.size() ||
      proposal.cpu_exact_component_minima.size() != component_count ||
      proposal.audit.gpu_kernel_launch_count != 2U ||
      proposal.audit.gpu_synchronization_count != 2U ||
      proposal.audit.gpu_count_pass_node_visit_count !=
          proposal.audit.gpu_emit_pass_node_visit_count ||
      !proposal.audit.exact_capacity_certified ||
      !proposal.audit.no_truncation_certified ||
      !proposal.audit.candidate_superset_certified ||
      !proposal.audit.cpu_exact_resolution_complete) {
    throw std::runtime_error(
        "the Phase 5 K1 Boruvka replay contract did not close");
  }
}

struct ReplaySummary {
  std::size_t case_count{};
  std::size_t point_count{};
  std::size_t candidate_count{};
  std::size_t required_candidate_count{};
  std::size_t count_pass_node_visit_count{};
  std::size_t strict_aabb_prune_count{};
  std::size_t uniform_component_prune_count{};
};

void accumulate(
    ReplaySummary& summary,
    const K1BoruvkaRoundProposal& proposal) {
  ++summary.case_count;
  summary.point_count += proposal.audit.resident_point_count;
  summary.candidate_count += proposal.audit.gpu_candidate_count;
  summary.required_candidate_count +=
      proposal.audit.cpu_required_candidate_count;
  summary.count_pass_node_visit_count +=
      proposal.audit.gpu_count_pass_node_visit_count;
  summary.strict_aabb_prune_count +=
      proposal.audit.gpu_strict_aabb_prune_count;
  summary.uniform_component_prune_count +=
      proposal.audit.gpu_uniform_component_prune_count;
}

void run_square(ReplaySummary& summary) {
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
  require_proposal_contract(proposal, cloud.size(), labels.size());
  if (anchor.rounds.empty() ||
      proposal.cpu_exact_component_minima !=
          anchor.rounds.front().component_minima) {
    throw std::runtime_error(
        "the GPU square proposal disagrees with exact CPU Boruvka");
  }
  accumulate(summary, proposal);
}

void run_contracted_chain(ReplaySummary& summary) {
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
  require_proposal_contract(proposal, cloud.size(), 2U);
  if (anchor.rounds.size() < 2U ||
      proposal.cpu_exact_component_minima !=
          anchor.rounds[1U].component_minima) {
    throw std::runtime_error(
        "the GPU contracted-chain proposal disagrees with exact CPU Boruvka");
  }
  accumulate(summary, proposal);
}

}  // namespace

int main() {
  try {
    ReplaySummary summary;
    run_square(summary);
    run_contracted_chain(summary);
    std::cout
        << "{\"candidate_count\":" << summary.candidate_count
        << ",\"case_count\":" << summary.case_count
        << ",\"count_pass_node_visit_count\":"
        << summary.count_pass_node_visit_count
        << ",\"decision_semantics\":\""
        << morsehgp3d::gpu::K1BoruvkaCandidateAudit::decision_semantics
        << "\""
        << ",\"point_count\":" << summary.point_count
        << ",\"proposal_semantics\":\""
        << morsehgp3d::gpu::K1BoruvkaCandidateAudit::proposal_semantics
        << "\",\"required_candidate_count\":"
        << summary.required_candidate_count
        << ",\"schema\":\"morsehgp3d.phase5.k1_boruvka_gpu_replay.v1\""
        << ",\"status\":\"passed\""
        << ",\"strict_aabb_prune_count\":"
        << summary.strict_aabb_prune_count
        << ",\"uniform_component_prune_count\":"
        << summary.uniform_component_prune_count << "}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "Phase 5 K1 Boruvka GPU replay failed: "
              << error.what() << '\n';
    return 1;
  }
}
