#include "morsehgp3d/hierarchy/anchored_pair_candidate_classifier.hpp"

#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::CanonicalPointCloud;
using spatial::ExactDyadicAabb3;
using spatial::MortonLbvhIndex;
using spatial::PointId;

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(message);
  }
  return left + right;
}

[[nodiscard]] ExactDyadicAabb3 point_bounds(
    const CanonicalPointCloud& cloud,
    PointId point_id) {
  const std::array<std::uint64_t, 3> words =
      cloud.point(point_id).canonical_input_bits();
  return ExactDyadicAabb3{words, words};
}

[[nodiscard]] double binary64_value(std::uint64_t bits) {
  return std::bit_cast<double>(exact::canonicalize_binary64_bits(bits));
}

[[nodiscard]] bool filter_partition_closes(
    const ExactAnchoredPairCandidateClassificationAudit& audit) {
  return audit.node_visit_count ==
      checked_add(
          checked_add(
              audit.interval_exterior_node_count,
              audit.interval_interior_node_count,
              "the anchored pair interval partition overflows size_t"),
          audit.exact_node_fallback_count,
          "the anchored pair interval partition overflows size_t");
}

void require_filter_partition(
    const ExactAnchoredPairCandidateClassificationAudit& audit) {
  if (!filter_partition_closes(audit)) {
    throw std::logic_error(
        "the anchored pair interval audit does not partition node visits");
  }
}

}  // namespace

ExactAnchoredPairCandidateClassificationContext
ExactAnchoredPairCandidateClassificationContext::start(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::array<PointId, 2> support_ids,
    std::size_t maximum_closed_rank) {
  return ExactAnchoredPairCandidateClassificationContext(
      index,
      cloud,
      support_ids,
      maximum_closed_rank,
      PrivateConstructionTag{});
}

ExactAnchoredPairCandidateClassificationContext::
    ExactAnchoredPairCandidateClassificationContext(
        const MortonLbvhIndex& index,
        const CanonicalPointCloud& cloud,
        std::array<PointId, 2> support_ids,
        std::size_t maximum_closed_rank,
        PrivateConstructionTag) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "an anchored pair classifier requires its cloud's exact LBVH");
  }
  static_cast<void>(cloud.point(support_ids[0]));
  static_cast<void>(cloud.point(support_ids[1]));
  if (support_ids[0] == support_ids[1]) {
    throw std::invalid_argument(
        "an anchored pair classifier requires two distinct points");
  }
  if (support_ids[1] < support_ids[0]) {
    std::swap(support_ids[0], support_ids[1]);
  }
  if (maximum_closed_rank < 2U ||
      maximum_closed_rank >
          exact_anchored_pair_candidate_maximum_closed_rank) {
    throw std::out_of_range(
        "an anchored pair maximum closed rank must be in [2, 11]");
  }
  if (index.build_counters_.maximum_depth >=
      exact_anchored_pair_candidate_maximum_frontier_entry_count) {
    throw std::length_error(
        "the certified LBVH depth exceeds the fixed anchored-pair frontier");
  }

  support_ids_ = support_ids;
  maximum_closed_rank_ = maximum_closed_rank;
  first_support_bounds_ = point_bounds(cloud, support_ids_[0]);
  second_support_bounds_ = point_bounds(cloud, support_ids_[1]);
  exact::detail::Fp64EnvironmentGuard filter_environment;
  audit_.fp64_interval_filter_enabled = filter_environment.supported();
  interval_anchor_ = prepare_interval_anchor(cloud, support_ids_);
  frontier_[0] = index.root_index_;
  frontier_entry_count_ = 1U;
  audit_.maximum_frontier_entry_count = 1U;
  cloud_identity_ = cloud.identity_;
  lbvh_identity_ = index.identity_;
}

bool ExactAnchoredPairCandidateClassificationContext::validated_for(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud) const noexcept {
  return index.validated_for(cloud) &&
      cloud_identity_.get() == cloud.identity_.get() &&
      lbvh_identity_.get() == index.identity_.get();
}

ExactAnchoredPairCandidateClassificationContext::PreparedPhiIntervalAnchor
ExactAnchoredPairCandidateClassificationContext::prepare_interval_anchor(
    const CanonicalPointCloud& cloud,
    const std::array<PointId, 2>& support_ids) {
  const std::array<std::uint64_t, 3> first_words =
      cloud.point(support_ids[0]).canonical_input_bits();
  const std::array<std::uint64_t, 3> second_words =
      cloud.point(support_ids[1]).canonical_input_bits();
  PreparedPhiIntervalAnchor anchor;
  const exact::detail::Binary64Interval half =
      exact::detail::point_binary64_interval(0.5);
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::detail::Binary64Interval first =
        exact::detail::point_binary64_interval(
            binary64_value(first_words[axis]));
    const exact::detail::Binary64Interval second =
        exact::detail::point_binary64_interval(
            binary64_value(second_words[axis]));
    anchor.midpoint[axis] = exact::detail::multiply_binary64_intervals(
        exact::detail::add_binary64_intervals(first, second), half);
    anchor.half_difference[axis] =
        exact::detail::multiply_binary64_intervals(
            exact::detail::subtract_binary64_intervals(first, second),
            half);
    anchor.squared_half_difference[axis] =
        exact::detail::square_binary64_interval(
            anchor.half_difference[axis]);
  }
  return anchor;
}

// Centered outward interval extension of
//
//   phi(q) = sum_i ((q_i-m_i)^2-d_i^2),
//   m_i = (a_i+b_i)/2, d_i = (a_i-b_i)/2.
//
// The outward midpoint and half-difference intervals contain their exact
// dyadic values.  Subsequent outward subtraction, square and sum therefore
// contain phi over the whole AABB.  Strict exclusion of zero is authoritative;
// every interval containing zero remains deliberately inconclusive.
std::optional<int>
ExactAnchoredPairCandidateClassificationContext::filtered_phi_aabb_sign(
    const PreparedPhiIntervalAnchor& anchor,
    const ExactDyadicAabb3& query_bounds) {
  exact::detail::Binary64Interval sum =
      exact::detail::point_binary64_interval(0.0);
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const double lower =
        binary64_value(query_bounds.lower_binary64_bits[axis]);
    const double upper =
        binary64_value(query_bounds.upper_binary64_bits[axis]);
    if (lower > upper) {
      throw std::logic_error(
          "an anchored pair interval filter received reversed bounds");
    }
    const exact::detail::Binary64Interval query{lower, upper, true};
    const exact::detail::Binary64Interval centered_query =
        exact::detail::subtract_binary64_intervals(
            query, anchor.midpoint[axis]);
    sum = exact::detail::add_binary64_intervals(
        sum,
        exact::detail::subtract_binary64_intervals(
            exact::detail::square_binary64_interval(centered_query),
            anchor.squared_half_difference[axis]));
  }
  const exact::FilterResult sign =
      exact::detail::sign_of_binary64_interval(sum);
  if (!sign.sign().has_value()) {
    return std::nullopt;
  }
  return *sign.sign() == exact::PredicateSign::positive ? 1 : -1;
}

ExactDyadicAabb3
ExactAnchoredPairCandidateClassificationContext::node_bounds(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t node_index) const {
  const auto& node = index.nodes_[node_index];
  ExactDyadicAabb3 bounds{};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    bounds.lower_binary64_bits[axis] =
        cloud.point(node.lower_point_ids[axis]).canonical_input_bits()[axis];
    bounds.upper_binary64_bits[axis] =
        cloud.point(node.upper_point_ids[axis]).canonical_input_bits()[axis];
  }
  return bounds;
}

ExactAnchoredPairCandidateClassificationStep
ExactAnchoredPairCandidateClassificationContext::make_step(
    ExactAnchoredPairCandidateClassificationStepKind kind,
    ExactAnchoredPairCandidateClassificationStopReason stop_reason,
    ExactAnchoredPairCandidateClassificationBudget budget,
    ExactAnchoredPairCandidateClassificationStepWork work) const {
  return ExactAnchoredPairCandidateClassificationStep{
      kind, stop_reason, budget, work, terminal()};
}

ExactAnchoredPairCandidateClassificationResult
ExactAnchoredPairCandidateClassificationContext::make_result(
    ExactAnchoredPairCandidateClassificationStatus status,
    ExactAnchoredPairCandidateClassificationStopReason stop_reason,
    ExactAnchoredPairCandidateClassificationBudget budget) const {
  ExactAnchoredPairCandidateClassificationResult result;
  result.status = status;
  result.stop_reason = stop_reason;
  result.support_ids = support_ids_;
  result.maximum_closed_rank = maximum_closed_rank_;
  result.requested_budget = budget;
  result.audit = audit_;
  return result;
}

ExactAnchoredPairCandidateClassificationStep
ExactAnchoredPairCandidateClassificationContext::advance(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    ExactAnchoredPairCandidateClassificationBudget budget) & {
  if (!validated_for(index, cloud)) {
    throw std::invalid_argument(
        "an anchored pair classification advance requires its authentic cloud and LBVH");
  }
  ExactAnchoredPairCandidateClassificationStepWork work;

  if (complete()) {
    audit_.advance_call_count = checked_add(
        audit_.advance_call_count,
        1U,
        "the anchored pair advance-call count overflows size_t");
    return make_step(
        ExactAnchoredPairCandidateClassificationStepKind::complete,
        ExactAnchoredPairCandidateClassificationStopReason::none,
        budget,
        work);
  }
  if (record_ready()) {
    audit_.advance_call_count = checked_add(
        audit_.advance_call_count,
        1U,
        "the anchored pair advance-call count overflows size_t");
    return make_step(
        ExactAnchoredPairCandidateClassificationStepKind::record_ready,
        ExactAnchoredPairCandidateClassificationStopReason::none,
        budget,
        work);
  }

  exact::detail::Fp64EnvironmentGuard filter_environment;
  if (audit_.fp64_interval_filter_enabled &&
      !filter_environment.supported()) {
    throw std::runtime_error(
        "the anchored pair interval environment changed during a resumable classification");
  }
  audit_.advance_call_count = checked_add(
      audit_.advance_call_count,
      1U,
      "the anchored pair advance-call count overflows size_t");

  const std::size_t interior_cap = maximum_closed_rank_ - 2U;
  while (frontier_entry_count_ != 0U) {
    if (work.node_visit_count >= budget.maximum_node_visit_count) {
      audit_.budget_exhaustion_count = checked_add(
          audit_.budget_exhaustion_count,
          1U,
          "the anchored pair budget-exhaustion count overflows size_t");
      require_filter_partition(audit_);
      return make_step(
          ExactAnchoredPairCandidateClassificationStepKind::budget_exhausted,
          ExactAnchoredPairCandidateClassificationStopReason::node_visit_limit,
          budget,
          work);
    }
    work.node_visit_count = checked_add(
        work.node_visit_count,
        1U,
        "the anchored pair step node-visit count overflows size_t");
    audit_.node_visit_count = checked_add(
        audit_.node_visit_count,
        1U,
        "the anchored pair node-visit count overflows size_t");

    const std::size_t node_index = frontier_[frontier_entry_count_ - 1U];
    --frontier_entry_count_;
    if (node_index >= index.nodes_.size()) {
      throw std::logic_error(
          "an anchored pair traversal reached an invalid LBVH node");
    }
    const auto& node = index.nodes_[node_index];
    const std::size_t subtree_size = node.leaf_end - node.leaf_begin;
    const ExactDyadicAabb3 query_bounds =
        node_bounds(index, cloud, node_index);

    std::optional<int> interval_sign;
    if (audit_.fp64_interval_filter_enabled) {
      interval_sign = filtered_phi_aabb_sign(interval_anchor_, query_bounds);
    }

    bool certified_exterior = false;
    bool certified_interior = false;
    int exact_minimum_phi_sign = 0;
    int exact_maximum_phi_sign = 0;
    if (interval_sign.has_value() && *interval_sign > 0) {
      audit_.interval_exterior_node_count = checked_add(
          audit_.interval_exterior_node_count,
          1U,
          "the anchored pair interval-exterior count overflows size_t");
      certified_exterior = true;
    } else if (interval_sign.has_value() && *interval_sign < 0) {
      audit_.interval_interior_node_count = checked_add(
          audit_.interval_interior_node_count,
          1U,
          "the anchored pair interval-interior count overflows size_t");
      certified_interior = true;
    } else {
      audit_.exact_node_fallback_count = checked_add(
          audit_.exact_node_fallback_count,
          1U,
          "the anchored pair exact-fallback count overflows size_t");
      exact_minimum_phi_sign =
          exact_diametral_anchor_phi_aabb_minimum_sign(
              first_support_bounds_,
              second_support_bounds_,
              query_bounds);
      audit_.exact_minimum_phi_aabb_bound_count = checked_add(
          audit_.exact_minimum_phi_aabb_bound_count,
          1U,
          "the anchored pair minimum-bound count overflows size_t");
      if (exact_minimum_phi_sign > 0) {
        certified_exterior = true;
      } else {
        exact_maximum_phi_sign =
            exact_diametral_phi_aabb_maximum_sign(
                first_support_bounds_,
                second_support_bounds_,
                query_bounds);
        audit_.exact_maximum_phi_aabb_bound_count = checked_add(
            audit_.exact_maximum_phi_aabb_bound_count,
            1U,
            "the anchored pair maximum-bound count overflows size_t");
        certified_interior = exact_maximum_phi_sign < 0;
      }
    }

    if (certified_exterior) {
      exterior_count_ = checked_add(
          exterior_count_,
          subtree_size,
          "the anchored pair exterior count overflows size_t");
      audit_.bulk_exterior_subtree_count = checked_add(
          audit_.bulk_exterior_subtree_count,
          1U,
          "the anchored pair exterior-subtree count overflows size_t");
      audit_.bulk_exterior_point_count = checked_add(
          audit_.bulk_exterior_point_count,
          subtree_size,
          "the anchored pair exterior-point count overflows size_t");
      audit_.classified_point_count = checked_add(
          audit_.classified_point_count,
          subtree_size,
          "the anchored pair classified-point count overflows size_t");
      continue;
    }
    if (certified_interior) {
      audit_.bulk_interior_subtree_count = checked_add(
          audit_.bulk_interior_subtree_count,
          1U,
          "the anchored pair interior-subtree count overflows size_t");
      audit_.bulk_interior_point_count = checked_add(
          audit_.bulk_interior_point_count,
          subtree_size,
          "the anchored pair interior-point count overflows size_t");
      audit_.classified_point_count = checked_add(
          audit_.classified_point_count,
          subtree_size,
          "the anchored pair classified-point count overflows size_t");
      if (interior_count_ > interior_cap) {
        throw std::logic_error(
            "an anchored pair interior cursor exceeds its rank cap");
      }
      const std::size_t remaining_interior_capacity =
          interior_cap - interior_count_;
      if (subtree_size > remaining_interior_capacity) {
        terminal_status_ =
            ExactAnchoredPairCandidateClassificationStatus::above_rank;
        terminal_requested_budget_ = budget;
        terminal_consumed_ = true;
        audit_.early_above_rank_certificate = true;
        require_filter_partition(audit_);
        return make_step(
            ExactAnchoredPairCandidateClassificationStepKind::above_rank,
            ExactAnchoredPairCandidateClassificationStopReason::none,
            budget,
            work);
      }
      for (std::size_t position = node.leaf_begin;
           position < node.leaf_end;
           ++position) {
        interior_ids_[interior_count_] =
            index.leaves_[position].point_id;
        ++interior_count_;
      }
      continue;
    }

    if (node.is_leaf()) {
      audit_.leaf_visit_count = checked_add(
          audit_.leaf_visit_count,
          1U,
          "the anchored pair leaf-visit count overflows size_t");
      audit_.exact_point_classification_count = checked_add(
          audit_.exact_point_classification_count,
          1U,
          "the anchored pair exact-point count overflows size_t");
      audit_.classified_point_count = checked_add(
          audit_.classified_point_count,
          1U,
          "the anchored pair classified-point count overflows size_t");
      if (exact_minimum_phi_sign != exact_maximum_phi_sign) {
        throw std::logic_error(
            "a degenerate exact LBVH box has inconsistent phi extrema");
      }
      const PointId point_id = index.leaves_[node.leaf_begin].point_id;
      if (exact_maximum_phi_sign < 0) {
        if (interior_count_ == interior_cap) {
          terminal_status_ =
              ExactAnchoredPairCandidateClassificationStatus::above_rank;
          terminal_requested_budget_ = budget;
          terminal_consumed_ = true;
          audit_.early_above_rank_certificate = true;
          require_filter_partition(audit_);
          return make_step(
              ExactAnchoredPairCandidateClassificationStepKind::above_rank,
              ExactAnchoredPairCandidateClassificationStopReason::none,
              budget,
              work);
        }
        interior_ids_[interior_count_] = point_id;
        ++interior_count_;
      } else if (exact_maximum_phi_sign == 0) {
        shell_count_ = checked_add(
            shell_count_,
            1U,
            "the anchored pair shell count overflows size_t");
        if (point_id == support_ids_[0]) {
          support_seen_mask_ =
              static_cast<std::uint8_t>(support_seen_mask_ | 1U);
        } else if (point_id == support_ids_[1]) {
          support_seen_mask_ =
              static_cast<std::uint8_t>(support_seen_mask_ | 2U);
        } else if (!canonical_extra_shell_witness_id_.has_value() ||
                   point_id < *canonical_extra_shell_witness_id_) {
          canonical_extra_shell_witness_id_ = point_id;
        }
      } else {
        exterior_count_ = checked_add(
            exterior_count_,
            1U,
            "the anchored pair exterior count overflows size_t");
      }
      continue;
    }

    audit_.internal_node_expansion_count = checked_add(
        audit_.internal_node_expansion_count,
        1U,
        "the anchored pair expansion count overflows size_t");
    if (frontier_entry_count_ >
        exact_anchored_pair_candidate_maximum_frontier_entry_count - 2U) {
      throw std::logic_error(
          "the anchored pair DFS exceeded its certified fixed frontier");
    }
    frontier_[frontier_entry_count_] = node.right_child;
    ++frontier_entry_count_;
    frontier_[frontier_entry_count_] = node.left_child;
    ++frontier_entry_count_;
    audit_.maximum_frontier_entry_count = std::max(
        audit_.maximum_frontier_entry_count,
        frontier_entry_count_);
  }

  const std::size_t classified_count = checked_add(
      checked_add(
          interior_count_,
          shell_count_,
          "the anchored pair partition count overflows size_t"),
      exterior_count_,
      "the anchored pair partition count overflows size_t");
  if (classified_count != cloud.size() || support_seen_mask_ != 3U ||
      shell_count_ < 2U ||
      (shell_count_ == 2U) !=
          !canonical_extra_shell_witness_id_.has_value()) {
    throw std::logic_error(
        "an anchored pair traversal did not close its exact partition");
  }
  if (audit_.classified_point_count != cloud.size()) {
    throw std::logic_error(
        "an anchored pair classification audit did not close");
  }
  require_filter_partition(audit_);

  std::sort(
      interior_ids_.begin(),
      interior_ids_.begin() + static_cast<std::ptrdiff_t>(interior_count_));
  terminal_status_ =
      ExactAnchoredPairCandidateClassificationStatus::complete;
  terminal_requested_budget_ = budget;
  audit_.complete_partition_certified = true;
  return make_step(
      ExactAnchoredPairCandidateClassificationStepKind::record_ready,
      ExactAnchoredPairCandidateClassificationStopReason::none,
      budget,
      work);
}

ExactAnchoredPairCandidateClassificationResult
ExactAnchoredPairCandidateClassificationContext::take_result(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud) & {
  if (!validated_for(index, cloud)) {
    throw std::invalid_argument(
        "taking an anchored pair result requires its authentic cloud and LBVH");
  }
  if (!record_ready()) {
    throw std::logic_error(
        "an anchored pair result is not ready for one-time emission");
  }

  const exact::CircumcenterResult sphere = exact::circumcenter(
      cloud.point(support_ids_[0]), cloud.point(support_ids_[1]));
  if (sphere.kind() != exact::CircumcenterKind::unique ||
      !sphere.center().has_value() ||
      !sphere.squared_level().has_value()) {
    throw std::logic_error(
        "two canonical distinct points did not define a unique sphere");
  }
  std::vector<PointId> interior_ids(
      interior_ids_.begin(),
      interior_ids_.begin() + static_cast<std::ptrdiff_t>(interior_count_));
  ExactAnchoredPairCandidateClassificationResult result = make_result(
      ExactAnchoredPairCandidateClassificationStatus::complete,
      ExactAnchoredPairCandidateClassificationStopReason::none,
      terminal_requested_budget_);
  result.audit.center_and_level_constructed = true;

  const std::size_t observed_closed_rank = checked_add(
      interior_count_,
      shell_count_,
      "the anchored pair observed rank overflows size_t");
  const std::size_t minimum_possible_closed_rank = checked_add(
      interior_count_,
      2U,
      "the anchored pair minimum rank overflows size_t");
  if (minimum_possible_closed_rank > maximum_closed_rank_) {
    throw std::logic_error(
        "an anchored pair escaped its exact interior-rank cap");
  }

  if (shell_count_ == 2U) {
    result.event = ExactPairSupportEvent{
        support_ids_,
        *sphere.center(),
        *sphere.squared_level(),
        std::move(interior_ids),
        observed_closed_rank,
        exterior_count_};
  } else {
    if (!canonical_extra_shell_witness_id_.has_value()) {
      throw std::logic_error(
          "an anchored extra shell omitted its canonical witness");
    }
    result.relevant_extra_shell_diagnostic =
        ExactPairSupportExtraShellDiagnostic{
            support_ids_,
            *sphere.center(),
            *sphere.squared_level(),
            std::move(interior_ids),
            shell_count_,
            *canonical_extra_shell_witness_id_,
            minimum_possible_closed_rank,
            observed_closed_rank,
            exterior_count_};
  }

  audit_.center_and_level_constructed = true;
  terminal_consumed_ = true;
  return result;
}

ExactAnchoredPairCandidateClassificationResult
ExactAnchoredPairCandidateClassifier::classify(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::array<PointId, 2> support_ids,
    std::size_t maximum_closed_rank,
    ExactAnchoredPairCandidateClassificationBudget budget) {
  ExactAnchoredPairCandidateClassificationContext context =
      ExactAnchoredPairCandidateClassificationContext::start(
          index, cloud, support_ids, maximum_closed_rank);
  const ExactAnchoredPairCandidateClassificationStep step =
      context.advance(index, cloud, budget);
  switch (step.kind) {
    case ExactAnchoredPairCandidateClassificationStepKind::record_ready:
      return context.take_result(index, cloud);
    case ExactAnchoredPairCandidateClassificationStepKind::above_rank:
      return context.make_result(
          ExactAnchoredPairCandidateClassificationStatus::above_rank,
          ExactAnchoredPairCandidateClassificationStopReason::none,
          budget);
    case ExactAnchoredPairCandidateClassificationStepKind::budget_exhausted:
      return context.make_result(
          ExactAnchoredPairCandidateClassificationStatus::budget_exhausted,
          step.stop_reason,
          budget);
    case ExactAnchoredPairCandidateClassificationStepKind::complete:
      throw std::logic_error(
          "a fresh anchored pair classification completed without a terminal");
  }
  throw std::logic_error(
      "an anchored pair classification returned an unknown step kind");
}

ExactAnchoredPairCandidateClassificationResult
classify_exact_anchored_pair_candidate(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::array<PointId, 2> support_ids,
    std::size_t maximum_closed_rank,
    ExactAnchoredPairCandidateClassificationBudget budget) {
  return ExactAnchoredPairCandidateClassifier::classify(
      index,
      cloud,
      support_ids,
      maximum_closed_rank,
      budget);
}

}  // namespace morsehgp3d::hierarchy
