#include "morsehgp3d/gpu/exact_pair_block_frontier.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/rational.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::gpu::ExactPairBlockAuthority;
using morsehgp3d::gpu::ExactPairBlockFrontierAudit;
using morsehgp3d::gpu::ExactPairBlockFrontierCapacity;
using morsehgp3d::gpu::ExactPairBlockFrontierConfig;
using morsehgp3d::gpu::ExactPairBlockFrontierContext;
using morsehgp3d::gpu::ExactPairBlockFrontierStepKind;
using morsehgp3d::gpu::ExactPairBlockNodeAuthority;
using morsehgp3d::gpu::ExactPairBlockOpenKind;
using morsehgp3d::gpu::ExactPairBlockQProposal;
using morsehgp3d::gpu::ExactPairBlockQProposalState;
using morsehgp3d::gpu::ExactPairBlockWitnessBudget;
using morsehgp3d::gpu::ExactPairBlockWitnessStatus;
using morsehgp3d::gpu::exact_pair_block_frontier_maximum_pending_authority_count;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactDyadicAabb3;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

[[nodiscard]] CanonicalPointCloud make_cloud() {
  const std::array<std::array<double, 3>, 8U> coordinates{{
      {-4.0, -1.0, 0.5},
      {-2.0, 3.0, -1.0},
      {-0.5, -2.0, 2.0},
      {1.0, 0.0, -3.0},
      {2.0, 4.0, 1.5},
      {3.0, -3.0, 4.0},
      {5.0, 1.0, -2.0},
      {7.0, 5.0, 3.0},
  }};
  std::vector<CertifiedPoint3> points;
  points.reserve(coordinates.size());
  for (const auto& coordinate : coordinates) {
    points.push_back(CertifiedPoint3::from_binary64(
        coordinate[0], coordinate[1], coordinate[2]));
  }
  return CanonicalPointCloud::rejecting_duplicates(points);
}

[[nodiscard]] CanonicalPointCloud make_certifying_cloud() {
  const std::array<std::array<double, 3>, 3U> coordinates{{
      {-2.0, 0.0, 0.0},
      {0.0, 0.0, 0.0},
      {2.0, 0.0, 0.0},
  }};
  std::vector<CertifiedPoint3> points;
  points.reserve(coordinates.size());
  for (const auto& coordinate : coordinates) {
    points.push_back(CertifiedPoint3::from_binary64(
        coordinate[0], coordinate[1], coordinate[2]));
  }
  return CanonicalPointCloud::rejecting_duplicates(points);
}

[[nodiscard]] bool disjoint(
    const ExactPairBlockNodeAuthority& first,
    const ExactPairBlockNodeAuthority& second) {
  return first.leaf_end <= second.leaf_begin ||
      second.leaf_end <= first.leaf_begin;
}

struct OpenHarvest {
  std::set<std::array<PointId, 2U>> terminal_pairs;
  std::vector<ExactPairBlockNodeAuthority> singleton_nodes;
  std::optional<ExactPairBlockAuthority> first_cross_block;
  ExactPairBlockFrontierAudit audit;
};

[[nodiscard]] OpenHarvest open_entire_partition(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud) {
  auto frontier = ExactPairBlockFrontierContext::start(
      index, cloud, ExactPairBlockFrontierConfig{6U, false});
  OpenHarvest harvest;
  std::size_t iteration_count = 0U;
  while (!frontier.complete()) {
    require(
        iteration_count < 4096U,
        "the exact pair-block frontier did not progress");
    ++iteration_count;
    const auto step = frontier.advance(
        index,
        cloud,
        ExactPairBlockFrontierCapacity{
            exact_pair_block_frontier_maximum_pending_authority_count});
    if (step.kind == ExactPairBlockFrontierStepKind::complete) {
      require(
          frontier.complete(),
          "a complete step did not close the exact pair frontier");
      break;
    }
    require(
        step.kind == ExactPairBlockFrontierStepKind::cross_block &&
            step.cross_block.has_value(),
        "the unbounded host fixture unexpectedly exhausted the frontier");
    const ExactPairBlockAuthority source = *step.cross_block;
    require(
        disjoint(source.first, source.second) &&
            source.first.leaf_end <= source.second.leaf_begin &&
            source.unordered_pair_mass ==
                source.first.leaf_count() *
                    source.second.leaf_count(),
        "a cross authority lost disjoint Cartesian pair mass");
    if (!harvest.first_cross_block.has_value()) {
      harvest.first_cross_block = source;
    }

    const auto opened = frontier.open_cross_block(
        index,
        cloud,
        ExactPairBlockFrontierCapacity{
            exact_pair_block_frontier_maximum_pending_authority_count});
    require(
        opened.kind != ExactPairBlockOpenKind::inconclusive_capacity &&
            opened.kind !=
                ExactPairBlockOpenKind::inconclusive_arithmetic_overflow,
        "the fixed host stack could not open a native cross authority");
    if (opened.kind == ExactPairBlockOpenKind::terminal_pair) {
      require(
          opened.terminal_pair.has_value() &&
              opened.source.first.leaf_count() == 1U &&
              opened.source.second.leaf_count() == 1U &&
              harvest.terminal_pairs.insert(*opened.terminal_pair).second,
          "the exact pair partition emitted a duplicate terminal pair");
      harvest.singleton_nodes.push_back(opened.source.first);
      harvest.singleton_nodes.push_back(opened.source.second);
    }
  }
  harvest.audit = frontier.audit();
  return harvest;
}

void test_disjoint_triangular_partition_and_mass() {
  const CanonicalPointCloud cloud = make_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const OpenHarvest harvest = open_entire_partition(index, cloud);
  const std::size_t expected_pair_count =
      cloud.size() * (cloud.size() - 1U) / 2U;
  require(
      harvest.terminal_pairs.size() == expected_pair_count &&
          harvest.audit.total_unordered_pair_mass ==
              expected_pair_count &&
          harvest.audit.terminal_unordered_pair_mass ==
              expected_pair_count &&
          harvest.audit.certified_unordered_pair_mass == 0U &&
          harvest.audit.pending_unordered_pair_mass == 0U &&
          harvest.audit.awaiting_unordered_pair_mass == 0U &&
          harvest.audit.exact_mass_conservation_verified &&
          harvest.audit.complete,
      "T(N)=T(L) union C(L,R) union T(R) did not conserve every pair once");
  require(
      harvest.audit.no_global_pair_matrix_materialized &&
          harvest.audit.no_delaunay_cell_coface_or_incidence_materialized &&
          !harvest.audit.hierarchy_or_tree_claimed &&
          !harvest.audit.slo_claimed,
      "the host frontier overclaimed architecture, hierarchy or timing");
}

[[nodiscard]] ExactRational endpoint(
    const ExactDyadicAabb3& box,
    std::size_t axis,
    std::size_t selector) {
  return ExactRational::from_binary64_bits(
      selector == 0U ? box.lower_binary64_bits[axis]
                     : box.upper_binary64_bits[axis]);
}

[[nodiscard]] ExactRational brute_force_axis_corner_bound(
    const std::array<ExactDyadicAabb3, 3U>& boxes) {
  ExactRational total;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    std::optional<ExactRational> axis_maximum;
    for (std::size_t query_selector = 0U;
         query_selector < 2U;
         ++query_selector) {
      const ExactRational query =
          endpoint(boxes[2], axis, query_selector);
      for (std::size_t first_selector = 0U;
           first_selector < 2U;
           ++first_selector) {
        const ExactRational first =
            endpoint(boxes[0], axis, first_selector);
        for (std::size_t second_selector = 0U;
             second_selector < 2U;
             ++second_selector) {
          const ExactRational second =
              endpoint(boxes[1], axis, second_selector);
          const ExactRational value =
              (query - first) * (query - second);
          if (!axis_maximum.has_value() ||
              value > *axis_maximum) {
            axis_maximum = value;
          }
        }
      }
    }
    require(
        axis_maximum.has_value(),
        "the brute-force corner fixture visited no endpoint");
    total = total + *axis_maximum;
  }
  return total;
}

[[nodiscard]] ExactPairBlockNodeAuthority find_disjoint_witness(
    const OpenHarvest& harvest) {
  require(
      harvest.first_cross_block.has_value(),
      "the harvested partition exposed no cross block");
  for (const ExactPairBlockNodeAuthority& node :
       harvest.singleton_nodes) {
    if (disjoint(node, harvest.first_cross_block->first) &&
        disjoint(node, harvest.first_cross_block->second)) {
      return node;
    }
  }
  throw std::runtime_error(
      "the fixture exposed no support-disjoint witness node");
}

void test_exact_q_bound_matches_24_corner_products() {
  const CanonicalPointCloud cloud = make_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const OpenHarvest harvest = open_entire_partition(index, cloud);
  const ExactPairBlockNodeAuthority witness =
      find_disjoint_witness(harvest);

  auto frontier = ExactPairBlockFrontierContext::start(
      index, cloud, ExactPairBlockFrontierConfig{2U, false});
  const auto step = frontier.advance(
      index,
      cloud,
      ExactPairBlockFrontierCapacity{
          exact_pair_block_frontier_maximum_pending_authority_count});
  require(
      step.kind == ExactPairBlockFrontierStepKind::cross_block &&
          step.cross_block == harvest.first_cross_block,
      "the exact frontier replay was not deterministic");
  const auto result = frontier.try_certify_with_witness(
      index,
      cloud,
      witness.node_index,
      ExactPairBlockQProposal{
          ExactPairBlockQProposalState::usable, 0},
      ExactPairBlockWitnessBudget{1U});
  require(
      result.exact_q_replay.has_value() &&
          result.exact_q_replay->exact_axis_endpoint_product_count == 24U &&
          result.exact_q_replay->existing_exact_fallback_used,
      "the A x B x W decision did not retain its exact corner replay");
  const ExactRational brute_force =
      brute_force_axis_corner_bound(result.exact_q_replay->boxes);
  require(
      result.exact_q_replay->maximum.maximum_phi == brute_force &&
          result.exact_q_replay->maximum.maximum_phi.sign() ==
              morsehgp3d::hierarchy::
                  exact_diametral_phi_aabb_maximum_sign(
                      result.exact_q_replay->boxes[0],
                      result.exact_q_replay->boxes[1],
                      result.exact_q_replay->boxes[2]),
      "the exact dyadic Q bound differs from brute force on 8 corners per axis");
}

void test_ambiguous_and_overflow_proposals_fail_closed() {
  const CanonicalPointCloud cloud = make_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const OpenHarvest harvest = open_entire_partition(index, cloud);
  const ExactPairBlockNodeAuthority witness =
      find_disjoint_witness(harvest);

  auto frontier = ExactPairBlockFrontierContext::start(
      index, cloud, ExactPairBlockFrontierConfig{2U, false});
  const auto step = frontier.advance(
      index,
      cloud,
      ExactPairBlockFrontierCapacity{
          exact_pair_block_frontier_maximum_pending_authority_count});
  require(
      step.kind == ExactPairBlockFrontierStepKind::cross_block,
      "the proposal fixture exposed no active cross block");
  const ExactPairBlockFrontierAudit audit_before = frontier.audit();
  const auto active_before = frontier.awaiting_cross_block();

  const auto ambiguous = frontier.try_certify_with_witness(
      index,
      cloud,
      witness.node_index,
      ExactPairBlockQProposal{
          ExactPairBlockQProposalState::ambiguous, 0},
      ExactPairBlockWitnessBudget{0U});
  require(
      ambiguous.status == ExactPairBlockWitnessStatus::
          inconclusive_ambiguous_proposal &&
          !ambiguous.frontier_mutated &&
          frontier.awaiting_cross_block() == active_before &&
          frontier.audit() == audit_before,
      "an ambiguous unrecertified proposal mutated pair authority");

  const auto overflow = frontier.try_certify_with_witness(
      index,
      cloud,
      witness.node_index,
      ExactPairBlockQProposal{
          ExactPairBlockQProposalState::arithmetic_overflow, 0},
      ExactPairBlockWitnessBudget{0U});
  require(
      overflow.status == ExactPairBlockWitnessStatus::
          inconclusive_proposal_arithmetic_overflow &&
          !overflow.frontier_mutated &&
          frontier.awaiting_cross_block() == active_before &&
          frontier.audit() == audit_before,
      "an overflowing unrecertified proposal mutated pair authority");

  const auto replayed = frontier.try_certify_with_witness(
      index,
      cloud,
      witness.node_index,
      ExactPairBlockQProposal{
          ExactPairBlockQProposalState::arithmetic_overflow, 0},
      ExactPairBlockWitnessBudget{1U});
  require(
      replayed.exact_q_replay.has_value() &&
          replayed.status != ExactPairBlockWitnessStatus::
              inconclusive_proposal_arithmetic_overflow,
      "the existing exact fallback did not recover an overflowing proposal");
}

void test_exact_negative_q_commits_only_its_cross_mass() {
  const CanonicalPointCloud cloud = make_certifying_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const OpenHarvest harvest = open_entire_partition(index, cloud);
  const auto witness_found = std::find_if(
      harvest.singleton_nodes.begin(),
      harvest.singleton_nodes.end(),
      [&index](const ExactPairBlockNodeAuthority& node) {
        return index.leaves()[node.leaf_begin].point_id == 1U;
      });
  require(
      witness_found != harvest.singleton_nodes.end(),
      "the negative-Q fixture exposed no singleton witness authority");

  auto frontier = ExactPairBlockFrontierContext::start(
      index, cloud, ExactPairBlockFrontierConfig{2U, false});
  bool certified_target = false;
  std::size_t iteration_count = 0U;
  while (!frontier.complete()) {
    require(
        ++iteration_count < 128U,
        "the negative-Q fixture did not terminate");
    const auto step = frontier.advance(
        index,
        cloud,
        ExactPairBlockFrontierCapacity{
            exact_pair_block_frontier_maximum_pending_authority_count});
    if (step.kind == ExactPairBlockFrontierStepKind::complete) {
      break;
    }
    require(
        step.kind == ExactPairBlockFrontierStepKind::cross_block &&
            step.cross_block.has_value(),
        "the negative-Q fixture lost its cross authority");
    const ExactPairBlockAuthority& active = *step.cross_block;
    const bool target =
        active.first.leaf_count() == 1U &&
        active.second.leaf_count() == 1U &&
        index.leaves()[active.first.leaf_begin].point_id == 0U &&
        index.leaves()[active.second.leaf_begin].point_id == 2U;
    if (target) {
      const auto certified = frontier.try_certify_with_witness(
          index,
          cloud,
          witness_found->node_index,
          ExactPairBlockQProposal{
              ExactPairBlockQProposalState::ambiguous, 0},
          ExactPairBlockWitnessBudget{1U});
      require(
          certified.status ==
                  ExactPairBlockWitnessStatus::certified_closed &&
              certified.frontier_mutated &&
              certified.certified_unordered_pair_mass == 1U &&
              certified.exact_q_replay.has_value() &&
              certified.exact_q_replay->maximum.maximum_phi.sign() < 0,
          "an exact strict witness did not close its singleton support pair");
      certified_target = true;
      continue;
    }
    const auto opened = frontier.open_cross_block(
        index,
        cloud,
        ExactPairBlockFrontierCapacity{
            exact_pair_block_frontier_maximum_pending_authority_count});
    require(
        opened.kind != ExactPairBlockOpenKind::inconclusive_capacity &&
            opened.kind !=
                ExactPairBlockOpenKind::inconclusive_arithmetic_overflow,
        "the negative-Q fixture could not open a non-target cross block");
  }
  const auto& audit = frontier.audit();
  require(
      certified_target && audit.complete &&
          audit.total_unordered_pair_mass == 3U &&
          audit.certified_unordered_pair_mass == 1U &&
          audit.terminal_unordered_pair_mass == 2U &&
          audit.pending_unordered_pair_mass == 0U &&
          audit.awaiting_unordered_pair_mass == 0U &&
          audit.exact_mass_conservation_verified,
      "the exact negative-Q commit did not conserve the remaining pair mass");
}

void test_capacity_failures_roll_back_authority() {
  const CanonicalPointCloud cloud = make_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  auto staged_frontier = ExactPairBlockFrontierContext::start(
      index, cloud, ExactPairBlockFrontierConfig{6U, false});
  const ExactPairBlockFrontierAudit staged_root_audit =
      staged_frontier.audit();
  const auto staged_failure = staged_frontier.advance(
      index, cloud, ExactPairBlockFrontierCapacity{3U});
  require(
      staged_failure.kind ==
              ExactPairBlockFrontierStepKind::inconclusive_capacity &&
          staged_frontier.pending_authority_count() == 1U &&
          !staged_frontier.awaiting_cross_block_response() &&
          staged_frontier.audit() == staged_root_audit,
      "a late capacity failure did not roll back earlier staged splits");

  auto frontier = ExactPairBlockFrontierContext::start(
      index, cloud, ExactPairBlockFrontierConfig{6U, true});

  const ExactPairBlockFrontierAudit root_audit = frontier.audit();
  const auto blocked_advance = frontier.advance(
      index, cloud, ExactPairBlockFrontierCapacity{1U});
  require(
      blocked_advance.kind ==
              ExactPairBlockFrontierStepKind::inconclusive_capacity &&
          frontier.pending_authority_count() == 1U &&
          !frontier.awaiting_cross_block_response() &&
          frontier.audit() == root_audit,
      "a capacity-blocked diagonal split did not roll back exactly");

  const auto cross = frontier.advance(
      index,
      cloud,
      ExactPairBlockFrontierCapacity{
          exact_pair_block_frontier_maximum_pending_authority_count});
  require(
      cross.kind == ExactPairBlockFrontierStepKind::cross_block &&
          cross.cross_block.has_value() &&
          cross.cross_block->first.leaf_count() > 1U &&
          cross.cross_block->second.leaf_count() > 1U,
      "cross-first traversal did not expose a splittable root cross block");
  const ExactPairBlockFrontierAudit cross_audit = frontier.audit();
  const auto active = frontier.awaiting_cross_block();
  const auto blocked_open = frontier.open_cross_block(
      index,
      cloud,
      ExactPairBlockFrontierCapacity{
          frontier.pending_authority_count()});
  require(
      blocked_open.kind ==
              ExactPairBlockOpenKind::inconclusive_capacity &&
          frontier.awaiting_cross_block() == active &&
          frontier.audit() == cross_audit,
      "a capacity-blocked cross split did not roll back exactly");

  const auto opened = frontier.open_cross_block(
      index,
      cloud,
      ExactPairBlockFrontierCapacity{
          exact_pair_block_frontier_maximum_pending_authority_count});
  require(
      (opened.kind ==
           ExactPairBlockOpenKind::first_support_split ||
       opened.kind ==
           ExactPairBlockOpenKind::second_support_split) &&
          !frontier.awaiting_cross_block_response() &&
          frontier.audit().exact_mass_conservation_verified,
      "the same cross authority did not resume after a capacity rollback");
}

}  // namespace

int main() {
  try {
    test_disjoint_triangular_partition_and_mass();
    test_exact_q_bound_matches_24_corner_products();
    test_ambiguous_and_overflow_proposals_fail_closed();
    test_exact_negative_q_commits_only_its_cross_mass();
    test_capacity_failures_roll_back_authority();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "PASS\n";
  return 0;
}
