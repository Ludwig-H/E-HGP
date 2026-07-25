#include "morsehgp3d/hierarchy/anchored_pair_witness_bank.hpp"

#include "morsehgp3d/exact/fp64_interval.hpp"
#include "morsehgp3d/hierarchy/pair_support_stream.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"
#include "morsehgp3d/spatial/query.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::CanonicalPointCloud;
using spatial::ExactDyadicAabb3;
using spatial::PointId;

struct PreparedWitness {
  PointId point_id{};
  ExactDyadicAabb3 point_bounds{};
  std::array<exact::detail::Binary64Interval, 3> direction_intervals{};
  std::array<double, 3> coordinates{};
  std::array<int, 3> direction_signs{};
};

struct TraversalEntry {
  std::size_t node_index{};
  std::uint64_t certified_witness_mask{};
};

struct MortonWindowWitnessCandidate {
  double squared_distance{};
  PointId point_id{};
};

[[nodiscard]] double ordinary_binary64_squared_distance(
    const CanonicalPointCloud& cloud,
    PointId first_point_id,
    PointId second_point_id) {
  double squared_distance = 0.0;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const double difference =
        cloud.point(first_point_id).binary64_coordinate(axis) -
        cloud.point(second_point_id).binary64_coordinate(axis);
    squared_distance += difference * difference;
  }
  return squared_distance;
}

[[nodiscard]] bool morton_window_candidate_less(
    const MortonWindowWitnessCandidate& left,
    const MortonWindowWitnessCandidate& right) {
  if (left.squared_distance != right.squared_distance) {
    return left.squared_distance < right.squared_distance;
  }
  return left.point_id < right.point_id;
}

[[nodiscard]] double binary64_value(std::uint64_t bits) {
  return std::bit_cast<double>(
      exact::canonicalize_binary64_bits(bits));
}

[[nodiscard]] ExactDyadicAabb3 point_bounds(
    const CanonicalPointCloud& cloud,
    PointId point_id) {
  const std::array<std::uint64_t, 3> words =
      cloud.point(point_id).canonical_input_bits();
  return ExactDyadicAabb3{words, words};
}

void require_exact_bounds(const ExactDyadicAabb3& bounds) {
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    if (exact::binary64_total_order_key(
            bounds.upper_binary64_bits[axis]) <
        exact::binary64_total_order_key(
            bounds.lower_binary64_bits[axis])) {
      throw std::invalid_argument(
          "an anchored pair witness AABB has reversed exact bounds");
    }
  }
}

[[nodiscard]] PreparedWitness prepare_witness(
    const CanonicalPointCloud& cloud,
    PointId anchor_point_id,
    PointId witness_point_id) {
  if (anchor_point_id == witness_point_id) {
    throw std::invalid_argument(
        "an anchored pair witness must differ from its anchor");
  }
  const exact::ExactRational3& anchor =
      cloud.point(anchor_point_id).exact();
  static_cast<void>(anchor);
  const std::array<std::uint64_t, 3> anchor_words =
      cloud.point(anchor_point_id).canonical_input_bits();
  const std::array<std::uint64_t, 3> witness_words =
      cloud.point(witness_point_id).canonical_input_bits();
  PreparedWitness prepared;
  prepared.point_id = witness_point_id;
  prepared.point_bounds =
      ExactDyadicAabb3{witness_words, witness_words};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const double anchor_coordinate =
        binary64_value(anchor_words[axis]);
    prepared.coordinates[axis] =
        binary64_value(witness_words[axis]);
    prepared.direction_intervals[axis] =
        exact::detail::subtract_binary64_intervals(
            exact::detail::point_binary64_interval(
                prepared.coordinates[axis]),
            exact::detail::point_binary64_interval(
                anchor_coordinate));
    const std::uint64_t anchor_key =
        exact::binary64_total_order_key(anchor_words[axis]);
    const std::uint64_t witness_key =
        exact::binary64_total_order_key(witness_words[axis]);
    prepared.direction_signs[axis] =
        witness_key < anchor_key ? -1 :
        (witness_key == anchor_key ? 0 : 1);
  }
  return prepared;
}

[[nodiscard]] std::optional<int>
filtered_prepared_witness_aabb_minimum_sign(
    const PreparedWitness& witness,
    const ExactDyadicAabb3& query_bounds) {
  exact::detail::Binary64Interval minimum =
      exact::detail::point_binary64_interval(0.0);
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    if (witness.direction_signs[axis] == 0) {
      continue;
    }
    const std::uint64_t extremum_bits =
        witness.direction_signs[axis] > 0
            ? query_bounds.lower_binary64_bits[axis]
            : query_bounds.upper_binary64_bits[axis];
    const exact::detail::Binary64Interval delta =
        exact::detail::subtract_binary64_intervals(
            exact::detail::point_binary64_interval(
                binary64_value(extremum_bits)),
            exact::detail::point_binary64_interval(
                witness.coordinates[axis]));
    minimum = exact::detail::add_binary64_intervals(
        minimum,
        exact::detail::multiply_binary64_intervals(
            witness.direction_intervals[axis], delta));
  }
  const exact::FilterResult filtered =
      exact::detail::sign_of_binary64_interval(minimum);
  if (!filtered.sign().has_value()) {
    return std::nullopt;
  }
  return *filtered.sign() == exact::PredicateSign::positive ? 1 : -1;
}

[[nodiscard]] int prepared_witness_aabb_minimum_sign(
    const ExactDyadicAabb3& anchor_bounds,
    const PreparedWitness& witness,
    const ExactDyadicAabb3& query_bounds) {
  // For x=witness, p=anchor and q in query_bounds,
  //
  //   (x-p).(q-p) - ||x-p||^2 = -(x-p).(x-q).
  //
  // The general exact phi maximum is therefore the negative of the desired
  // affine minimum.  Its bounded dyadic path avoids constructing or reducing
  // one rational number per axis and falls back to BigInt before any decision.
  return -exact_diametral_phi_aabb_maximum_sign(
      anchor_bounds, query_bounds, witness.point_bounds);
}

[[nodiscard]] ExactAnchoredPairWitnessBankResult stopped_result(
    ExactAnchoredPairWitnessBankResult result,
    ExactAnchoredPairCandidateStopReason reason) {
  result.status = ExactAnchoredPairCandidateStatus::budget_exhausted;
  result.stop_reason = reason;
  result.audit.traversal_complete = false;
  return result;
}

}  // namespace

int exact_anchored_pair_witness_aabb_minimum_sign(
    const CanonicalPointCloud& cloud,
    PointId anchor_point_id,
    PointId witness_point_id,
    const ExactDyadicAabb3& query_bounds) {
  require_exact_bounds(query_bounds);
  const ExactDyadicAabb3 anchor_bounds =
      point_bounds(cloud, anchor_point_id);
  const PreparedWitness witness =
      prepare_witness(cloud, anchor_point_id, witness_point_id);
  return prepared_witness_aabb_minimum_sign(
      anchor_bounds, witness, query_bounds);
}

class ExactAnchoredPairWitnessBankBuilder {
 public:
  [[nodiscard]] static ExactAnchoredPairWitnessBankResult build(
      const spatial::MortonLbvhIndex& index,
      const CanonicalPointCloud& cloud,
      PointId anchor_point_id,
      std::size_t maximum_closed_rank,
      ExactAnchoredPairWitnessBankBudget budget) {
    if (!index.validated_for(cloud)) {
      throw std::invalid_argument(
          "the anchored pair witness bank requires its cloud's exact LBVH");
    }
    static_cast<void>(cloud.point(anchor_point_id));
    if (maximum_closed_rank < 2U ||
        maximum_closed_rank >
            exact_anchored_pair_witness_bank_maximum_closed_rank) {
      throw std::out_of_range(
          "an anchored pair maximum closed rank must be in [2, 11]");
    }
    if (budget.proposed_witness_bank_size >
        exact_anchored_pair_witness_bank_maximum_size) {
      throw std::out_of_range(
          "an anchored pair witness bank cannot exceed 64 points");
    }
    switch (budget.proposal_policy) {
      case ExactAnchoredPairWitnessBankProposalPolicy::exact_top_k:
      case ExactAnchoredPairWitnessBankProposalPolicy::
          bounded_morton_window:
        break;
      default:
        throw std::invalid_argument(
            "unknown anchored pair witness-bank proposal policy");
    }

    ExactAnchoredPairWitnessBankResult result;
    result.anchor_point_id = anchor_point_id;
    result.maximum_closed_rank = maximum_closed_rank;
    result.requested_budget = budget;
    result.audit.requested_witness_bank_size =
        budget.proposed_witness_bank_size;
    result.audit.proposal_policy = budget.proposal_policy;
    result.audit.morton_window_radius = budget.morton_window_radius;

    const std::size_t anchor_index =
        static_cast<std::size_t>(anchor_point_id);
    const std::size_t candidate_upper_bound =
        cloud.size() - anchor_index - 1U;
    constexpr std::size_t initial_candidate_reserve = 256U;
    constexpr std::size_t initial_prune_record_reserve = 256U;
    result.candidate_point_ids.reserve(std::min(
        {candidate_upper_bound,
         budget.maximum_candidate_entry_count,
         initial_candidate_reserve}));
    result.prune_records.reserve(std::min(
        {index.nodes_.size(),
         budget.maximum_prune_record_count,
         initial_prune_record_reserve}));

    if (candidate_upper_bound == 0U) {
      result.audit.morton_window_complete =
          budget.proposal_policy ==
          ExactAnchoredPairWitnessBankProposalPolicy::
              bounded_morton_window;
      result.audit.witness_search_complete = true;
      result.audit.traversal_complete = true;
      result.status = ExactAnchoredPairCandidateStatus::complete;
      result.stop_reason = ExactAnchoredPairCandidateStopReason::none;
      return result;
    }

    const std::size_t eligible_witness_count = cloud.size() - 1U;
    const std::size_t effective_witness_bank_size = std::min(
        budget.proposed_witness_bank_size,
        eligible_witness_count);
    result.audit.effective_witness_bank_size =
        effective_witness_bank_size;
    if (effective_witness_bank_size == 0U) {
      result.audit.witness_search_complete = true;
      result.audit.morton_window_complete =
          budget.proposal_policy ==
          ExactAnchoredPairWitnessBankProposalPolicy::
              bounded_morton_window;
    } else {
      switch (budget.proposal_policy) {
        case ExactAnchoredPairWitnessBankProposalPolicy::exact_top_k: {
          result.audit.witness_search_attempted = true;
          const std::array<PointId, 1> excluded_ids{anchor_point_id};
          const spatial::ExclusionSet exclusions =
              spatial::ExclusionSet::from_ids(
                  excluded_ids, cloud, excluded_ids.size());
          spatial::ExactBudgetedLbvhTopKResult witness_search =
              spatial::lbvh_top_k_budgeted(
                  index,
                  cloud,
                  cloud.point(anchor_point_id).exact(),
                  effective_witness_bank_size,
                  exclusions,
                  budget.witness_search_budget,
                  spatial::LbvhTraversalOrder::near_first);
          result.audit.witness_search_status = witness_search.status();
          result.audit.witness_search_stop_reason =
              witness_search.stop_reason();
          result.audit.witness_search_audit = witness_search.audit();
          result.audit.witness_search_complete = witness_search.complete();
          if (witness_search.complete()) {
            const std::span<const PointId> chosen_ids =
                witness_search.partition().canonical_choice_ids();
            result.witness_bank_point_ids.assign(
                chosen_ids.begin(), chosen_ids.end());
          }
          break;
        }
        case ExactAnchoredPairWitnessBankProposalPolicy::
            bounded_morton_window: {
          // This proposal is intentionally heuristic.  Only the exact strict
          // witness predicates below can prune; no property of Morton order or
          // of these ordinary binary64 distances enters the certificate.
          result.audit.witness_search_complete = true;
          const std::size_t anchor_leaf_position =
              index.leaf_position_by_point_id_[anchor_index];
          if (anchor_leaf_position >= index.leaves_.size()) {
            throw std::logic_error(
                "the anchor has no valid Morton leaf position");
          }
          const std::size_t left_window_size = std::min(
              budget.morton_window_radius, anchor_leaf_position);
          const std::size_t right_window_size = std::min(
              budget.morton_window_radius,
              index.leaves_.size() - anchor_leaf_position - 1U);
          result.audit.morton_window_available_leaf_count =
              left_window_size + right_window_size;

          std::vector<MortonWindowWitnessCandidate> shortlist;
          shortlist.reserve(effective_witness_bank_size + 1U);
          const auto inspect_leaf =
              [&](std::size_t leaf_position) {
                ++result.audit.morton_window_inspection_count;
                ++result.audit.morton_window_candidate_count;
                ++result.audit.morton_window_distance_evaluation_count;
                const PointId point_id =
                    index.leaves_[leaf_position].point_id;
                if (point_id == anchor_point_id) {
                  throw std::logic_error(
                      "a Morton witness window revisited its anchor");
                }
                const MortonWindowWitnessCandidate candidate{
                    ordinary_binary64_squared_distance(
                        cloud, anchor_point_id, point_id),
                    point_id};
                const auto insertion = std::lower_bound(
                    shortlist.begin(),
                    shortlist.end(),
                    candidate,
                    morton_window_candidate_less);
                if (shortlist.size() < effective_witness_bank_size) {
                  shortlist.insert(insertion, candidate);
                } else if (insertion != shortlist.end()) {
                  shortlist.insert(insertion, candidate);
                  shortlist.pop_back();
                }
              };

          const std::size_t maximum_offset =
              std::max(left_window_size, right_window_size);
          for (std::size_t offset = 1U;
               offset <= maximum_offset &&
               result.audit.morton_window_inspection_count <
                   budget.maximum_morton_window_inspection_count;
               ++offset) {
            if (offset <= left_window_size) {
              inspect_leaf(anchor_leaf_position - offset);
            }
            if (offset <= right_window_size &&
                result.audit.morton_window_inspection_count <
                    budget.maximum_morton_window_inspection_count) {
              inspect_leaf(anchor_leaf_position + offset);
            }
          }
          result.audit.morton_window_complete =
              result.audit.morton_window_inspection_count ==
              result.audit.morton_window_available_leaf_count;
          result.audit.morton_window_inspection_budget_exhausted =
              !result.audit.morton_window_complete;
          result.witness_bank_point_ids.reserve(shortlist.size());
          for (const MortonWindowWitnessCandidate& candidate : shortlist) {
            result.witness_bank_point_ids.push_back(candidate.point_id);
          }
          break;
        }
      }
    }
    result.audit.published_witness_bank_size =
        result.witness_bank_point_ids.size();

    const std::size_t required_witness_count =
        maximum_closed_rank - 1U;
    result.audit.witness_bank_sufficient_for_rank_pruning =
        result.witness_bank_point_ids.size() >= required_witness_count;

    exact::detail::Fp64EnvironmentGuard filter_environment;
    const bool fp64_filter_enabled =
        filter_environment.supported();
    std::vector<PreparedWitness> prepared_witnesses;
    prepared_witnesses.reserve(result.witness_bank_point_ids.size());
    for (const PointId witness_point_id :
         result.witness_bank_point_ids) {
      prepared_witnesses.push_back(
          prepare_witness(cloud, anchor_point_id, witness_point_id));
    }
    const ExactDyadicAabb3 anchor_bounds =
        point_bounds(cloud, anchor_point_id);

    std::vector<TraversalEntry> traversal_stack;
    traversal_stack.reserve(std::min(
        budget.maximum_traversal_stack_entry_count,
        index.build_counters_.maximum_depth + 1U));
    if (budget.maximum_traversal_stack_entry_count == 0U) {
      return stopped_result(
          std::move(result),
          ExactAnchoredPairCandidateStopReason::
              traversal_stack_entry_limit);
    }
    traversal_stack.push_back(TraversalEntry{index.root_index_, 0U});
    result.audit.peak_traversal_stack_entry_count = 1U;

    bool predicate_pruning_enabled =
        result.audit.witness_bank_sufficient_for_rank_pruning;
    while (!traversal_stack.empty()) {
      if (result.audit.node_visit_count ==
          budget.maximum_node_visit_count) {
        return stopped_result(
            std::move(result),
            ExactAnchoredPairCandidateStopReason::node_visit_limit);
      }
      const TraversalEntry traversal_entry = traversal_stack.back();
      traversal_stack.pop_back();
      const std::size_t node_index = traversal_entry.node_index;
      ++result.audit.node_visit_count;
      const spatial::MortonLbvhIndex::Node& node =
          index.nodes_[node_index];

      bool certified_prune = false;
      std::array<PointId, 10> qualifying_witness_ids{};
      std::uint64_t certified_witness_mask =
          traversal_entry.certified_witness_mask;
      std::size_t qualifying_witness_count =
          static_cast<std::size_t>(
              std::popcount(certified_witness_mask));
      if (predicate_pruning_enabled) {
        ExactDyadicAabb3 query_bounds{};
        for (std::size_t axis = 0U; axis < 3U; ++axis) {
          query_bounds.lower_binary64_bits[axis] =
              cloud.point(node.lower_point_ids[axis])
                  .canonical_input_bits()[axis];
          query_bounds.upper_binary64_bits[axis] =
              cloud.point(node.upper_point_ids[axis])
                  .canonical_input_bits()[axis];
        }
        for (std::size_t witness_index = 0U;
             witness_index < prepared_witnesses.size();
             ++witness_index) {
          const std::uint64_t witness_bit =
              std::uint64_t{1} << witness_index;
          if ((certified_witness_mask & witness_bit) != 0U) {
            continue;
          }
          const PreparedWitness& witness =
              prepared_witnesses[witness_index];
          if (result.audit.witness_node_predicate_count ==
              budget.maximum_witness_node_predicate_count) {
            ++result.audit.predicate_budget_fail_open_count;
            predicate_pruning_enabled = false;
            qualifying_witness_count = 0U;
            break;
          }
          ++result.audit.witness_node_predicate_count;
          std::optional<int> predicate_sign;
          if (fp64_filter_enabled) {
            predicate_sign =
                filtered_prepared_witness_aabb_minimum_sign(
                    witness, query_bounds);
          }
          if (predicate_sign.has_value()) {
            if (*predicate_sign > 0) {
              ++result.audit.fp64_interval_positive_count;
            } else {
              ++result.audit.fp64_interval_negative_count;
            }
          } else {
            ++result.audit.exact_witness_node_predicate_count;
            predicate_sign =
                prepared_witness_aabb_minimum_sign(
                    anchor_bounds, witness, query_bounds);
          }
          if (*predicate_sign > 0) {
            certified_witness_mask |= witness_bit;
            ++qualifying_witness_count;
            if (qualifying_witness_count == required_witness_count) {
              certified_prune = true;
              break;
            }
          }
        }
      }

      if (certified_prune) {
        std::size_t receipt_index = 0U;
        for (std::size_t witness_index = 0U;
             witness_index < prepared_witnesses.size() &&
             receipt_index < required_witness_count;
             ++witness_index) {
          if ((certified_witness_mask &
               (std::uint64_t{1} << witness_index)) != 0U) {
            qualifying_witness_ids[receipt_index] =
                prepared_witnesses[witness_index].point_id;
            ++receipt_index;
          }
        }
        if (receipt_index != required_witness_count) {
          throw std::logic_error(
              "an anchored pair prune lost a certified witness");
        }
        if (result.prune_records.size() <
            budget.maximum_prune_record_count) {
          result.prune_records.push_back(
              ExactAnchoredPairWitnessBankPruneRecord{
                  node_index,
                  node.leaf_begin,
                  node.leaf_end,
                  qualifying_witness_ids,
                  qualifying_witness_count});
          ++result.audit.certified_pruned_subtree_count;
          result.audit.certified_pruned_leaf_upper_bound +=
              node.leaf_end - node.leaf_begin;
          continue;
        }
        ++result.audit.prune_record_capacity_fail_open_count;
      }

      if (node.is_leaf()) {
        ++result.audit.leaf_visit_count;
        const PointId candidate_point_id =
            index.leaves_[node.leaf_begin].point_id;
        if (candidate_point_id > anchor_point_id) {
          if (result.candidate_point_ids.size() ==
              budget.maximum_candidate_entry_count) {
            return stopped_result(
                std::move(result),
                ExactAnchoredPairCandidateStopReason::
                    candidate_entry_limit);
          }
          result.candidate_point_ids.push_back(candidate_point_id);
          ++result.audit.candidate_entry_count;
        }
        continue;
      }

      if (result.audit.internal_node_expansion_count ==
          budget.maximum_internal_node_expansion_count) {
        return stopped_result(
            std::move(result),
            ExactAnchoredPairCandidateStopReason::
                internal_node_expansion_limit);
      }
      if (budget.maximum_traversal_stack_entry_count < 2U ||
          traversal_stack.size() >
              budget.maximum_traversal_stack_entry_count - 2U) {
        return stopped_result(
            std::move(result),
            ExactAnchoredPairCandidateStopReason::
                traversal_stack_entry_limit);
      }
      ++result.audit.internal_node_expansion_count;
      traversal_stack.push_back(
          TraversalEntry{node.right_child, certified_witness_mask});
      traversal_stack.push_back(
          TraversalEntry{node.left_child, certified_witness_mask});
      result.audit.peak_traversal_stack_entry_count = std::max(
          result.audit.peak_traversal_stack_entry_count,
          traversal_stack.size());
    }

    result.audit.traversal_complete = true;
    result.status = ExactAnchoredPairCandidateStatus::complete;
    result.stop_reason = ExactAnchoredPairCandidateStopReason::none;
    return result;
  }
};

ExactAnchoredPairWitnessBankResult
build_exact_anchored_pair_witness_bank_candidates(
    const spatial::MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    PointId anchor_point_id,
    std::size_t maximum_closed_rank,
    ExactAnchoredPairWitnessBankBudget budget) {
  return ExactAnchoredPairWitnessBankBuilder::build(
      index,
      cloud,
      anchor_point_id,
      maximum_closed_rank,
      std::move(budget));
}

}  // namespace morsehgp3d::hierarchy
