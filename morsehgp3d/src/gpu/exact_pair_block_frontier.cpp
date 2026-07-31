#include "morsehgp3d/gpu/exact_pair_block_frontier.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::gpu {
namespace {

[[nodiscard]] bool checked_add(
    std::size_t first,
    std::size_t second,
    std::size_t& sum) noexcept {
  constexpr std::size_t maximum = std::numeric_limits<std::size_t>::max();
  if (first > maximum - second) {
    return false;
  }
  sum = first + second;
  return true;
}

[[nodiscard]] bool checked_product(
    std::size_t first,
    std::size_t second,
    std::size_t& product) noexcept {
  constexpr std::size_t maximum = std::numeric_limits<std::size_t>::max();
  if (first != 0U && second > maximum / first) {
    return false;
  }
  product = first * second;
  return true;
}

[[nodiscard]] bool checked_unordered_pair_count(
    std::size_t point_count,
    std::size_t& pair_count) noexcept {
  if (point_count < 2U) {
    pair_count = 0U;
    return true;
  }
  const std::size_t first =
      point_count % 2U == 0U ? point_count / 2U : point_count;
  const std::size_t second =
      point_count % 2U == 0U ? point_count - 1U
                             : (point_count - 1U) / 2U;
  return checked_product(first, second, pair_count);
}

[[nodiscard]] bool ranges_are_disjoint(
    const ExactPairBlockNodeAuthority& first,
    const ExactPairBlockNodeAuthority& second) noexcept {
  return first.leaf_end <= second.leaf_begin ||
      second.leaf_end <= first.leaf_begin;
}

[[nodiscard]] std::size_t effective_capacity(
    ExactPairBlockFrontierCapacity capacity) noexcept {
  return std::min(
      capacity.maximum_retained_authority_count,
      exact_pair_block_frontier_maximum_pending_authority_count);
}

}  // namespace

ExactPairBlockFrontierContext ExactPairBlockFrontierContext::start(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    ExactPairBlockFrontierConfig config) {
  return ExactPairBlockFrontierContext(
      index, cloud, config, PrivateConstructionTag{});
}

ExactPairBlockFrontierContext::ExactPairBlockFrontierContext(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    ExactPairBlockFrontierConfig config,
    PrivateConstructionTag)
    : config_(config),
      point_count_(cloud.size()),
      lbvh_identity_(index.identity_) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "an exact pair-block frontier requires its cloud's exact LBVH");
  }
  if (config.maximum_closed_rank < 2U ||
      config.maximum_closed_rank >
          exact_pair_block_frontier_maximum_closed_rank) {
    throw std::out_of_range(
        "an exact pair-block frontier maximum closed rank must be in [2, 6]");
  }
  if (point_count_ == 0U || index.leaves_.size() != point_count_ ||
      index.root_index_ >= index.nodes_.size()) {
    throw std::logic_error(
        "an exact pair-block frontier received an incomplete LBVH");
  }
  if (index.build_counters_.maximum_depth >
      exact_pair_block_frontier_maximum_lbvh_depth) {
    throw std::logic_error(
        "an exact pair-block frontier LBVH exceeds its fixed depth bound");
  }

  std::size_t total_pair_mass = 0U;
  if (!checked_unordered_pair_count(point_count_, total_pair_mass)) {
    throw std::overflow_error(
        "an exact pair-block frontier total pair mass overflows size_t");
  }
  audit_.total_unordered_pair_mass = total_pair_mass;
  const ExactPairBlockAuthority root =
      diagonal_authority(index, index.root_index_);
  if (root.unordered_pair_mass != total_pair_mass) {
    throw std::logic_error(
        "an exact pair-block frontier root lost triangular pair mass");
  }
  push_authority(root);
  pending_unordered_pair_mass_ = total_pair_mass;
  refresh_mass_audit();
  verify_mass_conservation();
}

bool ExactPairBlockFrontierContext::validated_for(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud) const noexcept {
  return ready() && index.validated_for(cloud) &&
      lbvh_identity_.get() == index.identity_.get();
}

ExactPairBlockNodeAuthority ExactPairBlockFrontierContext::node_authority(
    const spatial::MortonLbvhIndex& index,
    std::size_t node_index) const {
  if (node_index >= index.nodes_.size()) {
    throw std::out_of_range(
        "an exact pair-block authority names a foreign LBVH node");
  }
  const auto& node = index.nodes_[node_index];
  if (node.leaf_begin >= node.leaf_end ||
      node.leaf_end > point_count_) {
    throw std::logic_error(
        "an exact pair-block authority names an empty native range");
  }
  return ExactPairBlockNodeAuthority{
      node_index, node.leaf_begin, node.leaf_end};
}

ExactPairBlockAuthority
ExactPairBlockFrontierContext::diagonal_authority(
    const spatial::MortonLbvhIndex& index,
    std::size_t node_index) const {
  const ExactPairBlockNodeAuthority node =
      node_authority(index, node_index);
  std::size_t pair_mass = 0U;
  if (!checked_unordered_pair_count(node.leaf_count(), pair_mass)) {
    throw std::overflow_error(
        "an exact diagonal pair-block mass overflows size_t");
  }
  return ExactPairBlockAuthority{
      ExactPairBlockAuthorityKind::diagonal, node, node, pair_mass};
}

ExactPairBlockAuthority ExactPairBlockFrontierContext::cross_authority(
    const spatial::MortonLbvhIndex& index,
    std::size_t first_node_index,
    std::size_t second_node_index) const {
  ExactPairBlockNodeAuthority first =
      node_authority(index, first_node_index);
  ExactPairBlockNodeAuthority second =
      node_authority(index, second_node_index);
  if (second.leaf_begin < first.leaf_begin) {
    std::swap(first, second);
  }
  if (!ranges_are_disjoint(first, second) ||
      first.leaf_end > second.leaf_begin) {
    throw std::logic_error(
        "an exact cross pair-block authority overlaps its supports");
  }
  std::size_t pair_mass = 0U;
  if (!checked_product(
          first.leaf_count(), second.leaf_count(), pair_mass)) {
    throw std::overflow_error(
        "an exact cross pair-block mass overflows size_t");
  }
  return ExactPairBlockAuthority{
      ExactPairBlockAuthorityKind::cross, first, second, pair_mass};
}

void ExactPairBlockFrontierContext::validate_authority(
    const spatial::MortonLbvhIndex& index,
    const ExactPairBlockAuthority& authority) const {
  const auto node_is_current =
      [this, &index](const ExactPairBlockNodeAuthority& value) {
        if (value.node_index >= index.nodes_.size()) {
          return false;
        }
        const auto& node = index.nodes_[value.node_index];
        return value.leaf_begin < value.leaf_end &&
            value.leaf_end <= point_count_ &&
            node.leaf_begin == value.leaf_begin &&
            node.leaf_end == value.leaf_end;
      };
  if (!node_is_current(authority.first) ||
      !node_is_current(authority.second)) {
    throw std::logic_error(
        "an exact pair-block frontier lost native LBVH authority");
  }

  std::size_t expected_mass = 0U;
  if (authority.kind == ExactPairBlockAuthorityKind::diagonal) {
    if (authority.first != authority.second ||
        !checked_unordered_pair_count(
            authority.first.leaf_count(), expected_mass)) {
      throw std::logic_error(
          "an exact diagonal pair-block authority is malformed");
    }
  } else {
    if (authority.first.leaf_end > authority.second.leaf_begin ||
        !checked_product(
            authority.first.leaf_count(),
            authority.second.leaf_count(),
            expected_mass)) {
      throw std::logic_error(
          "an exact cross pair-block authority is malformed");
    }
  }
  if (authority.unordered_pair_mass != expected_mass) {
    throw std::logic_error(
        "an exact pair-block authority lost its exact pair mass");
  }
}

void ExactPairBlockFrontierContext::push_authority(
    const ExactPairBlockAuthority& authority) {
  if (pending_authority_count_ >= pending_authorities_.size()) {
    throw std::logic_error(
        "an exact pair-block frontier exceeded its fixed stack");
  }
  pending_authorities_[pending_authority_count_] = authority;
  ++pending_authority_count_;
  const std::size_t retained =
      pending_authority_count_ +
      (awaiting_cross_block_.has_value() ? 1U : 0U);
  audit_.maximum_retained_authority_count =
      std::max(audit_.maximum_retained_authority_count, retained);
}

void ExactPairBlockFrontierContext::refresh_mass_audit() {
  audit_.pending_unordered_pair_mass = pending_unordered_pair_mass_;
  audit_.awaiting_unordered_pair_mass = awaiting_unordered_pair_mass_;
  audit_.certified_unordered_pair_mass =
      certified_unordered_pair_mass_;
  audit_.terminal_unordered_pair_mass = terminal_unordered_pair_mass_;

  std::size_t observed = 0U;
  bool valid = checked_add(
      pending_unordered_pair_mass_,
      awaiting_unordered_pair_mass_,
      observed);
  valid = valid && checked_add(
      observed, certified_unordered_pair_mass_, observed);
  valid = valid &&
      checked_add(observed, terminal_unordered_pair_mass_, observed);
  audit_.exact_mass_conservation_verified =
      valid && observed == audit_.total_unordered_pair_mass;
}

void ExactPairBlockFrontierContext::verify_mass_conservation() const {
  if (!audit_.exact_mass_conservation_verified) {
    throw std::logic_error(
        "an exact pair-block frontier lost unordered-pair mass");
  }
}

void ExactPairBlockFrontierContext::finish_if_complete() {
  if (pending_authority_count_ != 0U ||
      awaiting_cross_block_.has_value()) {
    return;
  }
  refresh_mass_audit();
  verify_mass_conservation();
  if (pending_unordered_pair_mass_ != 0U ||
      awaiting_unordered_pair_mass_ != 0U) {
    throw std::logic_error(
        "an exact pair-block frontier completed with live pair mass");
  }
  complete_ = true;
  audit_.complete = true;
}

ExactPairBlockFrontierStep ExactPairBlockFrontierContext::advance(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    ExactPairBlockFrontierCapacity capacity) & {
  if (!validated_for(index, cloud)) {
    throw std::invalid_argument(
        "an exact pair-block frontier cannot resume foreign authority");
  }
  if (awaiting_cross_block_.has_value()) {
    throw std::logic_error(
        "an exact pair-block frontier requires a cross-block response");
  }

  ExactPairBlockFrontierStep step;
  const auto finish_step = [this, &step]() {
    step.retained_authority_count =
        pending_authority_count_ +
        (awaiting_cross_block_.has_value() ? 1U : 0U);
    step.pending_unordered_pair_mass = pending_unordered_pair_mass_;
    step.awaiting_unordered_pair_mass =
        awaiting_unordered_pair_mass_;
    return step;
  };
  if (complete_) {
    step.kind = ExactPairBlockFrontierStepKind::complete;
    return finish_step();
  }

  const std::size_t retained_capacity = effective_capacity(capacity);
  const auto pending_authorities_before = pending_authorities_;
  const std::size_t pending_authority_count_before =
      pending_authority_count_;
  const std::size_t pending_mass_before =
      pending_unordered_pair_mass_;
  const ExactPairBlockFrontierAudit audit_before = audit_;
  const auto fail_transaction =
      [this,
       &step,
       &finish_step,
       &pending_authorities_before,
       pending_authority_count_before,
       pending_mass_before,
       &audit_before](ExactPairBlockFrontierStepKind kind) {
        pending_authorities_ = pending_authorities_before;
        pending_authority_count_ = pending_authority_count_before;
        pending_unordered_pair_mass_ = pending_mass_before;
        awaiting_cross_block_.reset();
        awaiting_unordered_pair_mass_ = 0U;
        audit_ = audit_before;
        step.kind = kind;
        return finish_step();
      };
  if (pending_authority_count_ > retained_capacity) {
    return fail_transaction(
        ExactPairBlockFrontierStepKind::inconclusive_capacity);
  }
  while (pending_authority_count_ != 0U) {
    const ExactPairBlockAuthority authority =
        pending_authorities_[pending_authority_count_ - 1U];
    validate_authority(index, authority);
    if (authority.kind == ExactPairBlockAuthorityKind::cross) {
      --pending_authority_count_;
      if (authority.unordered_pair_mass >
          pending_unordered_pair_mass_) {
        return fail_transaction(
            ExactPairBlockFrontierStepKind::
                inconclusive_arithmetic_overflow);
      }
      pending_unordered_pair_mass_ -= authority.unordered_pair_mass;
      awaiting_cross_block_ = authority;
      awaiting_unordered_pair_mass_ = authority.unordered_pair_mass;
      refresh_mass_audit();
      verify_mass_conservation();
      step.kind = ExactPairBlockFrontierStepKind::cross_block;
      step.cross_block = authority;
      return finish_step();
    }

    const auto& parent = index.nodes_[authority.first.node_index];
    if (parent.is_leaf()) {
      if (authority.unordered_pair_mass != 0U ||
          authority.first.leaf_count() != 1U) {
        throw std::logic_error(
            "an exact diagonal leaf retained nonzero pair mass");
      }
      --pending_authority_count_;
      continue;
    }
    if (parent.left_child >= index.nodes_.size() ||
        parent.right_child >= index.nodes_.size() ||
        parent.left_child == parent.right_child) {
      throw std::logic_error(
          "an exact diagonal authority cannot split natively");
    }

    const ExactPairBlockAuthority left =
        diagonal_authority(index, parent.left_child);
    const ExactPairBlockAuthority cross =
        cross_authority(
            index, parent.left_child, parent.right_child);
    const ExactPairBlockAuthority right =
        diagonal_authority(index, parent.right_child);
    std::size_t child_mass = 0U;
    bool mass_valid =
        checked_add(
            left.unordered_pair_mass,
            cross.unordered_pair_mass,
            child_mass);
    mass_valid = mass_valid && checked_add(
        child_mass, right.unordered_pair_mass, child_mass);
    if (!mass_valid ||
        child_mass != authority.unordered_pair_mass) {
      return fail_transaction(
          ExactPairBlockFrontierStepKind::
              inconclusive_arithmetic_overflow);
    }

    std::size_t required_retained_count = 0U;
    if (!checked_add(
            pending_authority_count_, 2U, required_retained_count)) {
      return fail_transaction(
          ExactPairBlockFrontierStepKind::
              inconclusive_arithmetic_overflow);
    }
    if (required_retained_count > retained_capacity) {
      return fail_transaction(
          ExactPairBlockFrontierStepKind::inconclusive_capacity);
    }

    --pending_authority_count_;
    if (config_.prioritize_cross_blocks) {
      push_authority(right);
      push_authority(left);
      push_authority(cross);
    } else {
      push_authority(right);
      push_authority(cross);
      push_authority(left);
    }
    ++audit_.diagonal_split_count;
    refresh_mass_audit();
    verify_mass_conservation();
  }

  finish_if_complete();
  step.kind = ExactPairBlockFrontierStepKind::complete;
  return finish_step();
}

ExactPairBlockWitnessResult
ExactPairBlockFrontierContext::try_certify_with_witness(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t witness_node_index,
    ExactPairBlockQProposal proposal,
    ExactPairBlockWitnessBudget budget) & {
  const std::array<std::size_t, 1U> witness_node_indices{
      witness_node_index};
  const std::array<ExactPairBlockQProposal, 1U> proposals{proposal};
  ExactPairBlockWitnessAntichainResult antichain =
      try_certify_with_witness_antichain_impl(
          index,
          cloud,
          witness_node_indices,
          proposals,
          budget,
          true);

  ExactPairBlockWitnessResult result;
  result.status = antichain.status;
  result.support_block = antichain.support_block;
  if (antichain.authenticated_witness_node_count != 0U) {
    result.witness = antichain.witnesses[0];
  }
  if (antichain.exact_q_replay_count != 0U) {
    result.exact_q_replay = antichain.exact_q_replays[0];
  }
  result.proposal = proposal;
  result.required_witness_point_count =
      antichain.required_witness_point_count;
  result.authenticated_witness_point_count =
      antichain.authenticated_witness_point_count;
  result.certified_unordered_pair_mass =
      antichain.certified_unordered_pair_mass;
  result.frontier_mutated = antichain.frontier_mutated;
  result.proposal_agreed_with_exact =
      antichain.proposal_agreement_count == 1U;
  return result;
}

ExactPairBlockWitnessAntichainResult
ExactPairBlockFrontierContext::try_certify_with_witness_antichain(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const std::size_t> witness_node_indices,
    std::span<const ExactPairBlockQProposal> proposals,
    ExactPairBlockWitnessBudget budget) & {
  return try_certify_with_witness_antichain_impl(
      index,
      cloud,
      witness_node_indices,
      proposals,
      budget,
      false);
}

ExactPairBlockWitnessAntichainResult
ExactPairBlockFrontierContext::
    try_certify_with_witness_antichain_impl(
        const spatial::MortonLbvhIndex& index,
        const spatial::CanonicalPointCloud& cloud,
        std::span<const std::size_t> witness_node_indices,
        std::span<const ExactPairBlockQProposal> proposals,
        ExactPairBlockWitnessBudget budget,
        bool replay_before_insufficient_mass_decision) & {
  if (!validated_for(index, cloud)) {
    throw std::invalid_argument(
        "an exact pair-block witness antichain names foreign authority");
  }
  if (!awaiting_cross_block_.has_value()) {
    throw std::logic_error(
        "an exact pair-block witness antichain has no active support block");
  }
  if (witness_node_indices.size() != proposals.size()) {
    throw std::invalid_argument(
        "an exact pair-block witness antichain requires one proposal per node");
  }
  ExactPairBlockWitnessAntichainResult result;
  result.support_block = *awaiting_cross_block_;
  result.submitted_witness_node_count =
      witness_node_indices.size();
  result.required_witness_point_count =
      config_.maximum_closed_rank - 1U;
  if (witness_node_indices.size() >
      exact_pair_block_frontier_maximum_witness_antichain_node_count) {
    result.status = ExactPairBlockWitnessStatus::
        inconclusive_witness_antichain_capacity;
    return result;
  }
  for (std::size_t proposal_index = 0U;
       proposal_index < proposals.size();
       ++proposal_index) {
    const ExactPairBlockQProposal proposal = proposals[proposal_index];
    switch (proposal.state) {
      case ExactPairBlockQProposalState::usable:
        if (proposal.proposed_maximum_sign < -1 ||
            proposal.proposed_maximum_sign > 1) {
          throw std::invalid_argument(
              "a usable pair-block Q proposal sign must be -1, 0 or 1");
        }
        break;
      case ExactPairBlockQProposalState::ambiguous:
      case ExactPairBlockQProposalState::arithmetic_overflow:
        break;
      default:
        throw std::invalid_argument(
            "an exact pair-block Q proposal has an invalid state");
    }
    result.proposals[proposal_index] = proposal;
  }

  struct WitnessAndProposal {
    ExactPairBlockNodeAuthority witness{};
    ExactPairBlockQProposal proposal{};
  };
  std::array<
      WitnessAndProposal,
      exact_pair_block_frontier_maximum_witness_antichain_node_count>
      canonical{};
  for (std::size_t witness_index = 0U;
       witness_index < witness_node_indices.size();
       ++witness_index) {
    if (witness_node_indices[witness_index] >=
        index.nodes_.size()) {
      result.status = ExactPairBlockWitnessStatus::
          inconclusive_invalid_witness_authority;
      return result;
    }
    canonical[witness_index] = WitnessAndProposal{
        node_authority(index, witness_node_indices[witness_index]),
        proposals[witness_index]};
  }
  const auto precedes =
      [](const WitnessAndProposal& first,
         const WitnessAndProposal& second) {
        if (first.witness.leaf_begin !=
            second.witness.leaf_begin) {
          return first.witness.leaf_begin <
              second.witness.leaf_begin;
        }
        if (first.witness.leaf_end != second.witness.leaf_end) {
          return first.witness.leaf_end <
              second.witness.leaf_end;
        }
        return first.witness.node_index <
            second.witness.node_index;
      };
  // The bound is five.  A direct insertion sort keeps every accessed extent
  // visible to strict compiler diagnostics and avoids a general-purpose
  // sorting implementation whose internal pivot arithmetic assumes a much
  // larger random-access range.
  for (std::size_t sorted_count = 1U;
       sorted_count < witness_node_indices.size();
       ++sorted_count) {
    const WitnessAndProposal inserted = canonical[sorted_count];
    std::size_t destination = sorted_count;
    while (destination != 0U &&
           precedes(inserted, canonical[destination - 1U])) {
      canonical[destination] = canonical[destination - 1U];
      --destination;
    }
    canonical[destination] = inserted;
  }

  std::size_t authenticated_point_count = 0U;
  for (std::size_t witness_index = 0U;
       witness_index < witness_node_indices.size();
       ++witness_index) {
    const ExactPairBlockNodeAuthority witness =
        canonical[witness_index].witness;
    result.witnesses[witness_index] = witness;
    result.proposals[witness_index] =
        canonical[witness_index].proposal;
    ++result.authenticated_witness_node_count;
    if (!checked_add(
            authenticated_point_count,
            witness.leaf_count(),
            authenticated_point_count)) {
      result.status = ExactPairBlockWitnessStatus::
          inconclusive_arithmetic_overflow;
      return result;
    }
    result.authenticated_witness_point_count =
        authenticated_point_count;
    if (!ranges_are_disjoint(
            witness, result.support_block.first) ||
        !ranges_are_disjoint(
            witness, result.support_block.second)) {
      result.status = ExactPairBlockWitnessStatus::
          inconclusive_witness_support_overlap;
      return result;
    }
    if (witness_index != 0U &&
        canonical[witness_index - 1U].witness.leaf_end >
            witness.leaf_begin) {
      result.status = ExactPairBlockWitnessStatus::
          inconclusive_witness_antichain_overlap;
      return result;
    }
  }
  if (budget.maximum_exact_q_evaluation_count <
      witness_node_indices.size()) {
    if (witness_node_indices.size() == 1U &&
        proposals[0].state ==
            ExactPairBlockQProposalState::ambiguous) {
      result.status = ExactPairBlockWitnessStatus::
          inconclusive_ambiguous_proposal;
    } else if (
        witness_node_indices.size() == 1U &&
        proposals[0].state ==
            ExactPairBlockQProposalState::arithmetic_overflow) {
      result.status = ExactPairBlockWitnessStatus::
          inconclusive_proposal_arithmetic_overflow;
    } else {
      result.status = ExactPairBlockWitnessStatus::
          inconclusive_exact_replay_limit;
    }
    return result;
  }
  const bool witness_mass_insufficient =
      authenticated_point_count <
      result.required_witness_point_count;
  if (witness_mass_insufficient &&
      !replay_before_insufficient_mass_decision) {
    result.status = ExactPairBlockWitnessStatus::
        inconclusive_insufficient_witness_mass;
    return result;
  }
  constexpr std::size_t maximum_counter =
      std::numeric_limits<std::size_t>::max();
  if (audit_.exact_q_evaluation_count >
          maximum_counter - witness_node_indices.size() ||
      audit_.exact_q_proposal_mismatch_count >
          maximum_counter - witness_node_indices.size()) {
    result.status = ExactPairBlockWitnessStatus::
        inconclusive_arithmetic_overflow;
    return result;
  }

  const auto exact_bounds =
      [&index, &cloud](std::size_t node_index) {
        const auto& node = index.nodes_[node_index];
        spatial::ExactDyadicAabb3 bounds{};
        for (std::size_t axis = 0U; axis < 3U; ++axis) {
          bounds.lower_binary64_bits[axis] =
              cloud.point(node.lower_point_ids[axis])
                  .canonical_input_bits()[axis];
          bounds.upper_binary64_bits[axis] =
              cloud.point(node.upper_point_ids[axis])
                  .canonical_input_bits()[axis];
        }
        return bounds;
      };
  const spatial::ExactDyadicAabb3 first_support_bounds =
      exact_bounds(result.support_block.first.node_index);
  const spatial::ExactDyadicAabb3 second_support_bounds =
      exact_bounds(result.support_block.second.node_index);
  for (std::size_t witness_index = 0U;
       witness_index < witness_node_indices.size();
       ++witness_index) {
    ExactPairBlockQReplay replay;
    replay.boxes = {
        first_support_bounds,
        second_support_bounds,
        exact_bounds(result.witnesses[witness_index].node_index)};
    replay.maximum =
        hierarchy::exact_diametral_phi_aabb_maximum(
            replay.boxes[0], replay.boxes[1], replay.boxes[2]);
    result.exact_q_replays[witness_index] = replay;
    ++result.exact_q_replay_count;
    ++audit_.exact_q_evaluation_count;
    const ExactPairBlockQProposal proposal =
        result.proposals[witness_index];
    if (proposal.state == ExactPairBlockQProposalState::usable) {
      if (proposal.proposed_maximum_sign ==
          replay.maximum.maximum_phi.sign()) {
        ++result.proposal_agreement_count;
      } else {
        ++result.proposal_mismatch_count;
        ++audit_.exact_q_proposal_mismatch_count;
      }
    }
    if (replay.maximum.maximum_phi.sign() >= 0) {
      result.status = ExactPairBlockWitnessStatus::
          inconclusive_nonnegative_q;
      return result;
    }
  }
  if (witness_mass_insufficient) {
    result.status = ExactPairBlockWitnessStatus::
        inconclusive_insufficient_witness_mass;
    return result;
  }

  std::size_t new_certified_mass = 0U;
  if (!checked_add(
          certified_unordered_pair_mass_,
          result.support_block.unordered_pair_mass,
          new_certified_mass)) {
    result.status = ExactPairBlockWitnessStatus::
        inconclusive_arithmetic_overflow;
    return result;
  }
  certified_unordered_pair_mass_ = new_certified_mass;
  awaiting_unordered_pair_mass_ = 0U;
  awaiting_cross_block_.reset();
  ++audit_.certified_cross_block_count;
  result.status =
      ExactPairBlockWitnessStatus::certified_closed;
  result.certified_unordered_pair_mass =
      result.support_block.unordered_pair_mass;
  result.frontier_mutated = true;
  refresh_mass_audit();
  verify_mass_conservation();
  finish_if_complete();
  return result;
}

ExactPairBlockOpenResult
ExactPairBlockFrontierContext::open_cross_block(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    ExactPairBlockFrontierCapacity capacity) & {
  if (!validated_for(index, cloud)) {
    throw std::invalid_argument(
        "an exact pair-block opening names foreign authority");
  }
  if (!awaiting_cross_block_.has_value()) {
    throw std::logic_error(
        "an exact pair-block opening has no active cross block");
  }
  const ExactPairBlockAuthority source = *awaiting_cross_block_;
  validate_authority(index, source);

  ExactPairBlockOpenResult result;
  result.source = source;
  const auto& first_node = index.nodes_[source.first.node_index];
  const auto& second_node = index.nodes_[source.second.node_index];
  if (first_node.is_leaf() && second_node.is_leaf()) {
    if (source.first.leaf_count() != 1U ||
        source.second.leaf_count() != 1U ||
        source.unordered_pair_mass != 1U) {
      throw std::logic_error(
          "an exact terminal cross block is not singleton x singleton");
    }
    std::size_t new_terminal_mass = 0U;
    if (!checked_add(
            terminal_unordered_pair_mass_,
            source.unordered_pair_mass,
            new_terminal_mass)) {
      result.kind =
          ExactPairBlockOpenKind::inconclusive_arithmetic_overflow;
      return result;
    }
    spatial::PointId first_id =
        index.leaves_[source.first.leaf_begin].point_id;
    spatial::PointId second_id =
        index.leaves_[source.second.leaf_begin].point_id;
    if (second_id < first_id) {
      std::swap(first_id, second_id);
    }
    terminal_unordered_pair_mass_ = new_terminal_mass;
    awaiting_unordered_pair_mass_ = 0U;
    awaiting_cross_block_.reset();
    ++audit_.terminal_pair_count;
    result.kind = ExactPairBlockOpenKind::terminal_pair;
    result.terminal_pair =
        std::array<spatial::PointId, 2U>{first_id, second_id};
    result.opened_unordered_pair_mass =
        source.unordered_pair_mass;
    refresh_mass_audit();
    verify_mass_conservation();
    finish_if_complete();
    return result;
  }

  const bool split_second =
      !second_node.is_leaf() &&
      (first_node.is_leaf() ||
       source.second.leaf_count() >= source.first.leaf_count());
  std::array<ExactPairBlockAuthority, 2U> children{};
  if (split_second) {
    children = {
        cross_authority(
            index,
            source.first.node_index,
            second_node.left_child),
        cross_authority(
            index,
            source.first.node_index,
            second_node.right_child)};
    result.kind = ExactPairBlockOpenKind::second_support_split;
  } else {
    if (first_node.is_leaf()) {
      throw std::logic_error(
          "an exact nonterminal cross block has no splittable support");
    }
    children = {
        cross_authority(
            index,
            first_node.left_child,
            source.second.node_index),
        cross_authority(
            index,
            first_node.right_child,
            source.second.node_index)};
    result.kind = ExactPairBlockOpenKind::first_support_split;
  }
  validate_authority(index, children[0]);
  validate_authority(index, children[1]);

  std::size_t child_mass = 0U;
  if (!checked_add(
          children[0].unordered_pair_mass,
          children[1].unordered_pair_mass,
          child_mass) ||
      child_mass != source.unordered_pair_mass) {
    result.kind =
        ExactPairBlockOpenKind::inconclusive_arithmetic_overflow;
    return result;
  }
  std::size_t required_retained_count = 0U;
  if (!checked_add(
          pending_authority_count_, 2U, required_retained_count)) {
    result.kind =
        ExactPairBlockOpenKind::inconclusive_arithmetic_overflow;
    return result;
  }
  if (required_retained_count > effective_capacity(capacity)) {
    result.kind = ExactPairBlockOpenKind::inconclusive_capacity;
    return result;
  }
  std::size_t new_pending_mass = 0U;
  if (!checked_add(
          pending_unordered_pair_mass_,
          source.unordered_pair_mass,
          new_pending_mass)) {
    result.kind =
        ExactPairBlockOpenKind::inconclusive_arithmetic_overflow;
    return result;
  }

  awaiting_unordered_pair_mass_ = 0U;
  awaiting_cross_block_.reset();
  push_authority(children[1]);
  push_authority(children[0]);
  pending_unordered_pair_mass_ = new_pending_mass;
  ++audit_.cross_split_count;
  result.opened_unordered_pair_mass =
      source.unordered_pair_mass;
  refresh_mass_audit();
  verify_mass_conservation();
  return result;
}

}  // namespace morsehgp3d::gpu
