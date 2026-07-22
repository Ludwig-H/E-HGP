#include "morsehgp3d/hierarchy/direct_sparse_first_incidence.hpp"

#include "morsehgp3d/exact/support.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <optional>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

struct FirstIncidenceNodeQueueEntry {
  exact::ExactLevel coface_squared_level_lower_bound;
  std::size_t node_index{};
};

struct FirstIncidenceNodeQueueCompare {
  spatial::LbvhTraversalOrder order;

  [[nodiscard]] bool operator()(
      const FirstIncidenceNodeQueueEntry& left,
      const FirstIncidenceNodeQueueEntry& right) const {
    if (left.coface_squared_level_lower_bound !=
        right.coface_squared_level_lower_bound) {
      if (order == spatial::LbvhTraversalOrder::near_first) {
        return left.coface_squared_level_lower_bound >
               right.coface_squared_level_lower_bound;
      }
      return left.coface_squared_level_lower_bound <
             right.coface_squared_level_lower_bound;
    }
    if (order == spatial::LbvhTraversalOrder::near_first) {
      return left.node_index > right.node_index;
    }
    return left.node_index < right.node_index;
  }
};

struct OutsideCofaceCandidate {
  std::array<PointId, 4U> support_point_ids{};
  std::size_t support_point_count{};
  exact::ExactCenter3 center{};
  exact::ExactLevel squared_level{};
};

struct OutsideCofaceBuildResult {
  std::optional<OutsideCofaceCandidate> candidate;
  ExactDirectSparseFirstIncidenceStopReason stop_reason{
      ExactDirectSparseFirstIncidenceStopReason::none};
};

void require_valid_traversal_order(
    spatial::LbvhTraversalOrder traversal_order) {
  switch (traversal_order) {
    case spatial::LbvhTraversalOrder::near_first:
    case spatial::LbvhTraversalOrder::far_first:
      return;
  }
  throw std::invalid_argument("an LBVH traversal order is invalid");
}

[[nodiscard]] bool try_add_size(
    std::size_t left,
    std::size_t right,
    std::size_t& sum) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  sum = left + right;
  return true;
}

[[nodiscard]] bool try_multiply_size(
    std::size_t left,
    std::size_t right,
    std::size_t& product) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return false;
  }
  product = left * right;
  return true;
}

void checked_add_counter(std::size_t& counter, std::size_t increment) {
  std::size_t sum = 0U;
  if (!try_add_size(counter, increment, sum)) {
    throw std::overflow_error(
        "a direct sparse first-incidence counter overflows size_t");
  }
  counter = sum;
}

[[nodiscard]] std::size_t binomial(
    std::size_t point_count,
    std::size_t subset_size) {
  if (subset_size > point_count) {
    return 0U;
  }
  std::size_t result = 1U;
  for (std::size_t factor = 0U; factor < subset_size; ++factor) {
    const std::size_t numerator = point_count - factor;
    if (result > std::numeric_limits<std::size_t>::max() / numerator) {
      throw std::overflow_error(
          "a direct sparse first-incidence binomial overflows size_t");
    }
    result *= numerator;
    result /= factor + 1U;
  }
  return result;
}

[[nodiscard]] std::size_t source_support_count(
    std::size_t point_count) {
  std::size_t count = 0U;
  for (std::size_t support_size = 1U;
       support_size <= 4U;
       ++support_size) {
    checked_add_counter(count, binomial(point_count, support_size));
  }
  return count;
}

[[nodiscard]] std::size_t outside_coface_support_count(
    std::size_t source_point_count) {
  std::size_t count = 0U;
  for (std::size_t source_subset_size = 0U;
       source_subset_size <= 3U;
       ++source_subset_size) {
    checked_add_counter(
        count, binomial(source_point_count, source_subset_size));
  }
  return count;
}

static_assert(
    direct_sparse_first_incidence_maximum_source_point_count ==
    ExactDirectSparseFacetKey{}.point_ids.size());
static_assert(
    direct_sparse_first_incidence_maximum_source_point_count ==
    ExactFacetMiniballResult::maximum_facet_point_count);
static_assert(
    direct_sparse_first_incidence_source_support_enumeration_count_per_pass ==
    ExactFacetMiniballResult::maximum_enumerated_support_count);
static_assert(
    direct_sparse_first_incidence_maximum_source_support_enumeration_count ==
    2U *
        direct_sparse_first_incidence_source_support_enumeration_count_per_pass);
static_assert(
    direct_sparse_first_incidence_maximum_outside_coface_support_count ==
    176U);
static_assert(
    direct_sparse_first_incidence_maximum_outside_coface_classification_count ==
    11U *
        direct_sparse_first_incidence_maximum_outside_coface_support_count);

[[nodiscard]] exact::ExactLevel quarter_level(
    const exact::ExactLevel& level) {
  return exact::ExactLevel{
      level.rational() / exact::ExactRational{exact::BigInt{4}}};
}

[[nodiscard]] std::span<const PointId> used_point_ids(
    const ExactDirectSparseFacetKey& key) noexcept {
  return std::span<const PointId>{key.point_ids}.first(key.point_count);
}

void require_valid_source_key(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSparseFacetKey& key) {
  if (key.point_count == 0U ||
      key.point_count >
          direct_sparse_first_incidence_maximum_source_point_count ||
      key.point_count > cloud.size()) {
    throw std::invalid_argument(
        "a direct sparse first incidence requires one through ten source points");
  }
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    if (static_cast<std::size_t>(key.point_ids[index]) >= cloud.size()) {
      throw std::out_of_range(
          "a direct sparse first-incidence source PointId is outside the cloud");
    }
    if (index != 0U && key.point_ids[index - 1U] >= key.point_ids[index]) {
      throw std::invalid_argument(
          "a direct sparse first-incidence source key must be strictly increasing");
    }
  }
  for (std::size_t index = key.point_count;
       index < key.point_ids.size();
       ++index) {
    if (key.point_ids[index] != 0U) {
      throw std::invalid_argument(
          "a direct sparse first-incidence source key has nonzero unused slots");
    }
  }
}

[[nodiscard]] bool support_less(
    const OutsideCofaceCandidate& left,
    const OutsideCofaceCandidate& right) noexcept {
  if (left.support_point_count != right.support_point_count) {
    return left.support_point_count < right.support_point_count;
  }
  return std::lexicographical_compare(
      left.support_point_ids.begin(),
      left.support_point_ids.begin() +
          static_cast<std::ptrdiff_t>(left.support_point_count),
      right.support_point_ids.begin(),
      right.support_point_ids.begin() +
          static_cast<std::ptrdiff_t>(right.support_point_count));
}

[[nodiscard]] bool same_miniball(
    const ExactFacetMiniballResult& left,
    const ExactFacetMiniballResult& right) {
  return left.facet_point_ids == right.facet_point_ids &&
         left.support_point_ids == right.support_point_ids &&
         left.strictly_inside_point_ids == right.strictly_inside_point_ids &&
         left.boundary_point_ids == right.boundary_point_ids &&
         left.center == right.center &&
         left.squared_radius == right.squared_radius &&
         left.counters == right.counters && left.status == right.status &&
         left.scope == right.scope;
}

[[nodiscard]] bool same_optional_miniball(
    const std::optional<ExactFacetMiniballResult>& left,
    const std::optional<ExactFacetMiniballResult>& right) {
  return left.has_value() == right.has_value() &&
         (!left.has_value() || same_miniball(*left, *right));
}

[[nodiscard]] bool audit_within_budget(
    const ExactDirectSparseFirstIncidenceAudit& audit,
    const ExactDirectSparseFirstIncidenceBudget& budget) noexcept {
  return audit.source_support_enumeration_count <=
             budget.maximum_source_support_enumeration_count &&
         audit.node_visit_count <= budget.maximum_node_visit_count &&
         audit.internal_node_expansion_count <=
             budget.maximum_internal_node_expansion_count &&
         audit.exact_aabb_bound_evaluation_count <=
             budget.maximum_exact_aabb_bound_evaluation_count &&
         audit.exact_point_evaluation_count <=
             budget.maximum_exact_point_evaluation_count &&
         audit.coface_support_enumeration_count <=
             budget.maximum_coface_support_enumeration_count &&
         audit.candidate_point_classification_count <=
             budget.maximum_candidate_point_classification_count &&
         audit.peak_frontier_entry_count <=
             budget.maximum_frontier_entry_count &&
         audit.peak_cominimizer_entry_count <=
             budget.maximum_cominimizer_count;
}

[[nodiscard]] bool audit_covers_eligible_points(
    const ExactDirectSparseFirstIncidenceAudit& audit) noexcept {
  std::size_t classified_point_count = 0U;
  std::size_t accounted_point_count = 0U;
  return try_add_size(
             audit.inside_or_boundary_source_ball_point_count,
             audit.outside_source_ball_point_count,
             classified_point_count) &&
         classified_point_count == audit.exact_point_evaluation_count &&
         try_add_size(
             audit.exact_point_evaluation_count,
             audit.pruned_eligible_point_count,
             accounted_point_count) &&
         accounted_point_count == audit.eligible_coface_point_count;
}

[[nodiscard]] bool minimizer_shape_valid(
    const ExactDirectSparseFirstIncidenceMinimizer& minimizer,
    const ExactDirectSparseFacetKey& source_key,
    const exact::ExactLevel& common_level) noexcept {
  if (minimizer.support_point_count == 0U ||
      minimizer.support_point_count > minimizer.support_point_ids.size() ||
      minimizer.squared_level != common_level ||
      minimizer.added_point_in_source_closed_ball ==
          minimizer.added_point_in_selected_positive_support) {
    return false;
  }
  const auto source_begin = source_key.point_ids.begin();
  const auto source_end = source_begin +
      static_cast<std::ptrdiff_t>(source_key.point_count);
  if (std::binary_search(
          source_begin, source_end, minimizer.added_point_id)) {
    return false;
  }
  bool support_contains_added_point = false;
  for (std::size_t index = 0U;
       index < minimizer.support_point_count;
       ++index) {
    const PointId point_id = minimizer.support_point_ids[index];
    if (index != 0U &&
        minimizer.support_point_ids[index - 1U] >= point_id) {
      return false;
    }
    if (point_id == minimizer.added_point_id) {
      support_contains_added_point = true;
    } else if (!std::binary_search(source_begin, source_end, point_id)) {
      return false;
    }
  }
  for (std::size_t index = minimizer.support_point_count;
       index < minimizer.support_point_ids.size();
       ++index) {
    if (minimizer.support_point_ids[index] != 0U) {
      return false;
    }
  }
  return support_contains_added_point ==
         minimizer.added_point_in_selected_positive_support;
}

[[nodiscard]] bool common_result_contract(
    const ExactDirectSparseFirstIncidenceResult& result) noexcept {
  return result.schema_version == direct_sparse_first_incidence_schema_version &&
         result.trusted_authorities_certified &&
         result.aabb_lower_bounds_exact_and_valid &&
         result.equality_bounds_always_descended &&
         result.every_strict_outside_coface_support_contains_added_point &&
         result.no_partial_first_incidence_payload_published &&
         result.no_global_facet_or_coface_catalog_materialized &&
         result.no_gamma_or_higher_order_delaunay_materialized &&
         !result.public_status_claimed && result.partial_refinement_only &&
         result.scope == ExactDirectSparseFirstIncidenceScope::
             single_supplied_facet_all_one_point_cofaces_only &&
         result.audit.excluded_facet_point_count ==
             result.source_facet_key.point_count &&
         result.audit.inside_or_boundary_source_ball_point_count <=
             result.audit.exact_point_evaluation_count &&
         result.audit.outside_source_ball_point_count ==
             result.audit.exact_point_evaluation_count -
                 result.audit.inside_or_boundary_source_ball_point_count &&
         result.cominimizers.size() <=
             result.audit.peak_cominimizer_entry_count &&
         audit_within_budget(result.audit, result.requested_budget);
}

[[nodiscard]] bool observed_storage_within_trusted_bounds(
    const ExactDirectSparseFirstIncidenceResult& observed,
    std::size_t eligible_point_count,
    const ExactDirectSparseFirstIncidenceBudget& budget) {
  if (!audit_within_budget(observed.audit, budget) ||
      observed.cominimizers.size() > eligible_point_count ||
      observed.cominimizers.size() > budget.maximum_cominimizer_count) {
    return false;
  }
  if (observed.source_facet_miniball.has_value()) {
    const ExactFacetMiniballResult& source =
        *observed.source_facet_miniball;
    if (source.facet_point_ids.size() >
            direct_sparse_first_incidence_maximum_source_point_count ||
        source.support_point_ids.size() >
            ExactFacetMiniballResult::maximum_support_point_count ||
        source.strictly_inside_point_ids.size() >
            direct_sparse_first_incidence_maximum_source_point_count ||
        source.boundary_point_ids.size() >
            direct_sparse_first_incidence_maximum_source_point_count) {
      return false;
    }
  }
  for (const ExactDirectSparseFirstIncidenceMinimizer& minimizer :
       observed.cominimizers) {
    if (minimizer.support_point_count == 0U ||
        minimizer.support_point_count > minimizer.support_point_ids.size()) {
      return false;
    }
    for (std::size_t index = minimizer.support_point_count;
         index < minimizer.support_point_ids.size();
         ++index) {
      if (minimizer.support_point_ids[index] != 0U) {
        return false;
      }
    }
  }
  return true;
}

}  // namespace

class ExactDirectSparseFirstIncidenceBuilder {
 public:
  ExactDirectSparseFirstIncidenceBuilder(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      const ExactDirectSparseFacetKey& source_facet_key,
      const ExactDirectSparseFirstIncidenceBudget& budget,
      spatial::LbvhTraversalOrder traversal_order)
      : index_(index),
        cloud_(cloud),
        source_facet_key_(source_facet_key),
        source_point_ids_(used_point_ids(source_facet_key_)),
        budget_(budget),
        traversal_order_(traversal_order) {
    result_.source_facet_key = source_facet_key_;
    result_.requested_budget = budget_;
    result_.traversal_order = traversal_order_;
    result_.scope = ExactDirectSparseFirstIncidenceScope::
        single_supplied_facet_all_one_point_cofaces_only;
    result_.trusted_authorities_certified = true;
    result_.aabb_lower_bounds_exact_and_valid = true;
    result_.equality_bounds_always_descended = true;
    result_.every_strict_outside_coface_support_contains_added_point = true;
    result_.no_global_facet_or_coface_catalog_materialized = true;
    result_.no_gamma_or_higher_order_delaunay_materialized = true;
    result_.public_status_claimed = false;
    result_.partial_refinement_only = true;
    result_.audit.eligible_coface_point_count =
        cloud_.size() - source_point_ids_.size();
    result_.audit.excluded_facet_point_count = source_point_ids_.size();
  }

  [[nodiscard]] ExactDirectSparseFirstIncidenceResult run() {
    const std::size_t source_support_count_per_pass =
        source_support_count(source_point_ids_.size());
    std::size_t required_source_support_count = 0U;
    checked_add_counter(
        required_source_support_count, source_support_count_per_pass);
    checked_add_counter(
        required_source_support_count, source_support_count_per_pass);
    if (required_source_support_count >
        budget_.maximum_source_support_enumeration_count) {
      return exhausted(
          ExactDirectSparseFirstIncidenceStopReason::
              source_support_enumeration_limit);
    }

    result_.source_facet_miniball =
        build_exact_facet_miniball(cloud_, source_point_ids_);
    result_.source_facet_miniball_freshly_certified =
        result_.source_facet_miniball->status ==
            ExactFacetMiniballStatus::exact_facet_miniball_certified &&
        result_.source_facet_miniball->scope ==
            ExactFacetMiniballScope::local_facet_miniball_only &&
        result_.source_facet_miniball->counters.enumerated_support_count ==
            source_support_count_per_pass;
    result_.audit.source_support_enumeration_count =
        required_source_support_count;
    if (!result_.source_facet_miniball_freshly_certified ||
        result_.audit.source_support_enumeration_count !=
            required_source_support_count) {
      throw std::logic_error(
          "a direct sparse first incidence has an uncertified source miniball");
    }

    if (result_.audit.eligible_coface_point_count == 0U) {
      result_.audit.traversal_complete = true;
      result_.every_nonexcluded_point_evaluated_or_strictly_pruned = true;
      result_.all_cominimizers_retained_atomically = true;
      result_.no_partial_first_incidence_payload_published = true;
      result_.decision =
          ExactDirectSparseFirstIncidenceDecision::complete_no_coface;
      return std::move(result_);
    }

    if (budget_.maximum_frontier_entry_count == 0U) {
      return exhausted(
          ExactDirectSparseFirstIncidenceStopReason::frontier_entry_limit);
    }
    const std::size_t bound_evaluations_per_node =
        source_point_ids_.size() + 1U;
    if (bound_evaluations_per_node >
        budget_.maximum_exact_aabb_bound_evaluation_count) {
      return exhausted(
          ExactDirectSparseFirstIncidenceStopReason::
              exact_aabb_bound_evaluation_limit);
    }

    std::vector<FirstIncidenceNodeQueueEntry> frontier_storage;
    frontier_storage.reserve(std::min({
        budget_.maximum_frontier_entry_count,
        index_.nodes_.size(),
        std::size_t{256U}}));
    std::priority_queue<
        FirstIncidenceNodeQueueEntry,
        std::vector<FirstIncidenceNodeQueueEntry>,
        FirstIncidenceNodeQueueCompare>
        frontier{
            FirstIncidenceNodeQueueCompare{traversal_order_},
            std::move(frontier_storage)};
    result_.cominimizers.reserve(std::min({
        budget_.maximum_cominimizer_count,
        result_.audit.eligible_coface_point_count,
        std::size_t{64U}}));

    frontier.push(FirstIncidenceNodeQueueEntry{
        node_lower_bound(index_.root_index_), index_.root_index_});
    result_.audit.peak_frontier_entry_count = 1U;

    while (!frontier.empty()) {
      if (result_.audit.node_visit_count >=
          budget_.maximum_node_visit_count) {
        return exhausted(
            ExactDirectSparseFirstIncidenceStopReason::node_visit_limit);
      }
      FirstIncidenceNodeQueueEntry entry = frontier.top();
      frontier.pop();
      ++result_.audit.node_visit_count;

      if (incumbent_squared_level_.has_value() &&
          entry.coface_squared_level_lower_bound >
              *incumbent_squared_level_) {
        ++result_.audit.pruned_node_count;
        checked_add_counter(
            result_.audit.pruned_eligible_point_count,
            eligible_count_in_node(entry.node_index));
        continue;
      }

      const spatial::MortonLbvhIndex::Node& node =
          index_.nodes_[entry.node_index];
      if (node.is_leaf()) {
        const PointId added_point_id =
            index_.leaves_[node.leaf_begin].point_id;
        if (source_contains(added_point_id)) {
          continue;
        }
        if (result_.audit.exact_point_evaluation_count >=
            budget_.maximum_exact_point_evaluation_count) {
          return exhausted(
              ExactDirectSparseFirstIncidenceStopReason::
                  exact_point_evaluation_limit);
        }
        const exact::SpherePointLocation source_location =
            exact::classify_sphere_point(
                result_.source_facet_miniball->center,
                result_.source_facet_miniball->squared_radius,
                cloud_.point(added_point_id))
                .location();
        ++result_.audit.exact_point_evaluation_count;

        ExactDirectSparseFirstIncidenceMinimizer candidate;
        candidate.added_point_id = added_point_id;
        if (source_location != exact::SpherePointLocation::outside) {
          ++result_.audit.inside_or_boundary_source_ball_point_count;
          candidate.support_point_count =
              result_.source_facet_miniball->support_point_ids.size();
          std::copy(
              result_.source_facet_miniball->support_point_ids.begin(),
              result_.source_facet_miniball->support_point_ids.end(),
              candidate.support_point_ids.begin());
          candidate.center = result_.source_facet_miniball->center;
          candidate.squared_level =
              result_.source_facet_miniball->squared_radius;
          candidate.added_point_in_source_closed_ball = true;
          candidate.added_point_in_selected_positive_support = false;
        } else {
          ++result_.audit.outside_source_ball_point_count;
          OutsideCofaceBuildResult outside =
              build_outside_coface(added_point_id);
          if (outside.stop_reason !=
              ExactDirectSparseFirstIncidenceStopReason::none) {
            return exhausted(outside.stop_reason);
          }
          if (!outside.candidate.has_value()) {
            throw std::logic_error(
                "an outside one-point coface has no positive support containing its added point");
          }
          candidate.added_point_id = added_point_id;
          candidate.support_point_ids =
              outside.candidate->support_point_ids;
          candidate.support_point_count =
              outside.candidate->support_point_count;
          candidate.center = outside.candidate->center;
          candidate.squared_level = outside.candidate->squared_level;
          candidate.added_point_in_source_closed_ball = false;
          candidate.added_point_in_selected_positive_support = true;
        }
        observe_candidate(std::move(candidate));
        continue;
      }

      if (result_.audit.internal_node_expansion_count >=
          budget_.maximum_internal_node_expansion_count) {
        return exhausted(
            ExactDirectSparseFirstIncidenceStopReason::
                internal_node_expansion_limit);
      }
      if (frontier.size() > budget_.maximum_frontier_entry_count ||
          budget_.maximum_frontier_entry_count - frontier.size() < 2U) {
        return exhausted(
            ExactDirectSparseFirstIncidenceStopReason::frontier_entry_limit);
      }
      std::size_t child_bound_evaluation_count = 0U;
      checked_add_counter(
          child_bound_evaluation_count, bound_evaluations_per_node);
      checked_add_counter(
          child_bound_evaluation_count, bound_evaluations_per_node);
      if (result_.audit.exact_aabb_bound_evaluation_count >
              budget_.maximum_exact_aabb_bound_evaluation_count ||
          budget_.maximum_exact_aabb_bound_evaluation_count -
                  result_.audit.exact_aabb_bound_evaluation_count <
              child_bound_evaluation_count) {
        return exhausted(
            ExactDirectSparseFirstIncidenceStopReason::
                exact_aabb_bound_evaluation_limit);
      }
      ++result_.audit.internal_node_expansion_count;
      for (const std::size_t child : {node.left_child, node.right_child}) {
        frontier.push(FirstIncidenceNodeQueueEntry{
            node_lower_bound(child), child});
        result_.audit.peak_frontier_entry_count = std::max(
            result_.audit.peak_frontier_entry_count, frontier.size());
      }
    }

    result_.audit.traversal_complete = true;
    std::size_t classified_point_count = 0U;
    checked_add_counter(
        classified_point_count,
        result_.audit.inside_or_boundary_source_ball_point_count);
    checked_add_counter(
        classified_point_count,
        result_.audit.outside_source_ball_point_count);
    std::size_t accounted_eligible_point_count = 0U;
    checked_add_counter(
        accounted_eligible_point_count,
        result_.audit.exact_point_evaluation_count);
    checked_add_counter(
        accounted_eligible_point_count,
        result_.audit.pruned_eligible_point_count);
    result_.every_nonexcluded_point_evaluated_or_strictly_pruned =
        accounted_eligible_point_count ==
        result_.audit.eligible_coface_point_count;
    if (!result_.every_nonexcluded_point_evaluated_or_strictly_pruned ||
        classified_point_count !=
            result_.audit.exact_point_evaluation_count ||
        result_.audit.excluded_facet_point_count !=
            source_point_ids_.size() ||
        !incumbent_squared_level_.has_value()) {
      throw std::logic_error(
          "a complete direct sparse first-incidence traversal lost an eligible point");
    }
    if (cominimizer_overflowed_) {
      return exhausted(
          ExactDirectSparseFirstIncidenceStopReason::
              cominimizer_entry_limit);
    }
    std::sort(
        result_.cominimizers.begin(),
        result_.cominimizers.end(),
        [](const ExactDirectSparseFirstIncidenceMinimizer& left,
           const ExactDirectSparseFirstIncidenceMinimizer& right) {
          return left.added_point_id < right.added_point_id;
        });
    if (std::adjacent_find(
            result_.cominimizers.begin(),
            result_.cominimizers.end(),
            [](const ExactDirectSparseFirstIncidenceMinimizer& left,
               const ExactDirectSparseFirstIncidenceMinimizer& right) {
              return left.added_point_id == right.added_point_id;
            }) != result_.cominimizers.end()) {
      throw std::logic_error(
          "a direct sparse first incidence retained a duplicate co-minimizer");
    }
    result_.first_incidence_squared_level = incumbent_squared_level_;
    result_.all_cominimizers_retained_atomically = true;
    result_.no_partial_first_incidence_payload_published = true;
    result_.decision =
        ExactDirectSparseFirstIncidenceDecision::
            complete_exact_first_incidence;
    return std::move(result_);
  }

 private:
  [[nodiscard]] bool source_contains(PointId point_id) const noexcept {
    return std::binary_search(
        source_point_ids_.begin(), source_point_ids_.end(), point_id);
  }

  [[nodiscard]] std::size_t eligible_count_in_node(
      std::size_t node_index) const {
    if (node_index >= index_.nodes_.size()) {
      throw std::logic_error(
          "a first-incidence prune references an invalid LBVH node");
    }
    const spatial::MortonLbvhIndex::Node& node = index_.nodes_[node_index];
    std::size_t excluded_count = 0U;
    for (const PointId point_id : source_point_ids_) {
      const std::size_t position =
          index_.leaf_position_by_point_id_[point_id];
      if (position >= node.leaf_begin && position < node.leaf_end) {
        ++excluded_count;
      }
    }
    const std::size_t node_point_count = node.leaf_end - node.leaf_begin;
    if (excluded_count > node_point_count) {
      throw std::logic_error(
          "a first-incidence node contains too many excluded points");
    }
    return node_point_count - excluded_count;
  }

  [[nodiscard]] exact::ExactLevel coface_lower_bound(
      const exact::ExactLevel& minimum_source_center_squared_distance) const {
    const exact::ExactLevel& source_squared_radius =
        result_.source_facet_miniball->squared_radius;
    if (minimum_source_center_squared_distance <= source_squared_radius) {
      return source_squared_radius;
    }
    const exact::ExactRational sum =
        minimum_source_center_squared_distance.rational() +
        source_squared_radius.rational();
    const exact::ExactRational denominator =
        exact::ExactRational{exact::BigInt{4}} *
        minimum_source_center_squared_distance.rational();
    return exact::ExactLevel{sum * sum / denominator};
  }

  [[nodiscard]] exact::ExactLevel node_lower_bound(
      std::size_t node_index) {
    exact::ExactLevel lower = coface_lower_bound(
        index_.minimum_squared_distance_to_node(
            cloud_,
            node_index,
            result_.source_facet_miniball->center));
    ++result_.audit.exact_aabb_bound_evaluation_count;
    for (const PointId point_id : source_point_ids_) {
      const exact::ExactLevel pair_lower = quarter_level(
          index_.minimum_squared_distance_to_node(
              cloud_, node_index, cloud_.point(point_id).exact()));
      ++result_.audit.exact_aabb_bound_evaluation_count;
      if (pair_lower > lower) {
        lower = pair_lower;
      }
    }
    return lower;
  }

  template <std::size_t SourceSubsetSize>
  [[nodiscard]] std::optional<
      ExactDirectSparseFirstIncidenceStopReason>
  evaluate_outside_support(
      PointId added_point_id,
      const std::array<std::size_t, SourceSubsetSize>& source_positions,
      std::optional<OutsideCofaceCandidate>& selected) {
    static_assert(SourceSubsetSize <= 3U);
    if (result_.audit.coface_support_enumeration_count >=
        budget_.maximum_coface_support_enumeration_count) {
      return ExactDirectSparseFirstIncidenceStopReason::
          coface_support_enumeration_limit;
    }
    ++result_.audit.coface_support_enumeration_count;

    std::array<exact::ExactRational3, SourceSubsetSize + 1U> support;
    support[0U] = cloud_.point(added_point_id).exact();
    OutsideCofaceCandidate candidate;
    candidate.support_point_ids[0U] = added_point_id;
    candidate.support_point_count = SourceSubsetSize + 1U;
    for (std::size_t index = 0U; index < SourceSubsetSize; ++index) {
      if (source_positions[index] >= source_point_ids_.size()) {
        throw std::logic_error(
            "an outside coface support position is invalid");
      }
      const PointId point_id = source_point_ids_[source_positions[index]];
      support[index + 1U] = cloud_.point(point_id).exact();
      candidate.support_point_ids[index + 1U] = point_id;
    }

    const exact::CircumcenterSupportAnalysis analysis =
        exact::analyze_circumcenter_support(support);
    if (analysis.status() !=
        exact::CircumcenterSupportStatus::minimal) {
      return std::nullopt;
    }
    const exact::CircumcenterResult& sphere =
        analysis.circumcenter_result();
    if (sphere.kind() != exact::CircumcenterKind::unique ||
        !sphere.center().has_value() ||
        !sphere.squared_level().has_value()) {
      throw std::logic_error(
          "a minimal outside coface support has no exact sphere");
    }
    const std::size_t coface_point_count = source_point_ids_.size() + 1U;
    if (result_.audit.candidate_point_classification_count >
            budget_.maximum_candidate_point_classification_count ||
        budget_.maximum_candidate_point_classification_count -
                result_.audit.candidate_point_classification_count <
            coface_point_count) {
      return ExactDirectSparseFirstIncidenceStopReason::
          candidate_point_classification_limit;
    }

    bool encloses_coface = true;
    for (const PointId point_id : source_point_ids_) {
      const exact::SpherePointLocation location =
          exact::classify_sphere_point(
              *sphere.center(),
              *sphere.squared_level(),
              cloud_.point(point_id))
              .location();
      ++result_.audit.candidate_point_classification_count;
      if (location == exact::SpherePointLocation::outside) {
        encloses_coface = false;
      }
    }
    const exact::SpherePointLocation added_location =
        exact::classify_sphere_point(
            *sphere.center(),
            *sphere.squared_level(),
            cloud_.point(added_point_id))
            .location();
    ++result_.audit.candidate_point_classification_count;
    if (added_location == exact::SpherePointLocation::outside) {
      encloses_coface = false;
    }
    if (!encloses_coface) {
      return std::nullopt;
    }

    std::sort(
        candidate.support_point_ids.begin(),
        candidate.support_point_ids.begin() +
            static_cast<std::ptrdiff_t>(candidate.support_point_count));
    candidate.center = *sphere.center();
    candidate.squared_level = *sphere.squared_level();
    if (selected.has_value() &&
        candidate.squared_level == selected->squared_level &&
        candidate.center != selected->center) {
      throw std::logic_error(
          "equal outside coface miniballs disagree on their unique center");
    }
    if (!selected.has_value() ||
        candidate.squared_level < selected->squared_level ||
        (candidate.squared_level == selected->squared_level &&
         support_less(candidate, *selected))) {
      selected = std::move(candidate);
    }
    return std::nullopt;
  }

  [[nodiscard]] OutsideCofaceBuildResult build_outside_coface(
      PointId added_point_id) {
    const std::size_t support_count_before =
        result_.audit.coface_support_enumeration_count;
    const std::size_t classification_count_before =
        result_.audit.candidate_point_classification_count;
    std::optional<OutsideCofaceCandidate> selected;
    const auto evaluate =
        [this, added_point_id, &selected]<std::size_t SourceSubsetSize>(
            const std::array<std::size_t, SourceSubsetSize>& positions)
        -> std::optional<ExactDirectSparseFirstIncidenceStopReason> {
      return evaluate_outside_support(
          added_point_id, positions, selected);
    };

    if (const auto stop = evaluate(std::array<std::size_t, 0U>{});
        stop.has_value()) {
      return {std::nullopt, *stop};
    }
    for (std::size_t first = 0U;
         first < source_point_ids_.size();
         ++first) {
      if (const auto stop =
              evaluate(std::array<std::size_t, 1U>{first});
          stop.has_value()) {
        return {std::nullopt, *stop};
      }
    }
    for (std::size_t first = 0U;
         first < source_point_ids_.size();
         ++first) {
      for (std::size_t second = first + 1U;
           second < source_point_ids_.size();
           ++second) {
        if (const auto stop = evaluate(
                std::array<std::size_t, 2U>{first, second});
            stop.has_value()) {
          return {std::nullopt, *stop};
        }
      }
    }
    for (std::size_t first = 0U;
         first < source_point_ids_.size();
         ++first) {
      for (std::size_t second = first + 1U;
           second < source_point_ids_.size();
           ++second) {
        for (std::size_t third = second + 1U;
             third < source_point_ids_.size();
             ++third) {
          if (const auto stop = evaluate(
                  std::array<std::size_t, 3U>{first, second, third});
              stop.has_value()) {
            return {std::nullopt, *stop};
          }
        }
      }
    }
    const std::size_t supports_examined =
        result_.audit.coface_support_enumeration_count -
        support_count_before;
    const std::size_t classifications_per_support =
        source_point_ids_.size() + 1U;
    std::size_t maximum_local_classification_count = 0U;
    if (!try_multiply_size(
            supports_examined,
            classifications_per_support,
            maximum_local_classification_count)) {
      throw std::overflow_error(
          "an outside-coface classification bound overflows size_t");
    }
    const std::size_t local_classification_count =
        result_.audit.candidate_point_classification_count -
        classification_count_before;
    if (supports_examined !=
            outside_coface_support_count(source_point_ids_.size()) ||
        supports_examined >
            direct_sparse_first_incidence_maximum_outside_coface_support_count ||
        local_classification_count > maximum_local_classification_count ||
        local_classification_count >
            direct_sparse_first_incidence_maximum_outside_coface_classification_count ||
        !selected.has_value() ||
        selected->squared_level <=
            result_.source_facet_miniball->squared_radius ||
        !std::binary_search(
            selected->support_point_ids.begin(),
            selected->support_point_ids.begin() +
                static_cast<std::ptrdiff_t>(
                    selected->support_point_count),
            added_point_id)) {
      throw std::logic_error(
          "the positive-support reduction for an outside coface did not close");
    }
    return {std::move(selected), ExactDirectSparseFirstIncidenceStopReason::none};
  }

  void observe_candidate(
      ExactDirectSparseFirstIncidenceMinimizer candidate) {
    if (!incumbent_squared_level_.has_value() ||
        candidate.squared_level < *incumbent_squared_level_) {
      incumbent_squared_level_ = candidate.squared_level;
      result_.cominimizers.clear();
      cominimizer_overflowed_ = false;
      ++result_.audit.incumbent_improvement_count;
      observe_equal_candidate(std::move(candidate));
      return;
    }
    if (candidate.squared_level == *incumbent_squared_level_) {
      ++result_.audit.equal_incumbent_observation_count;
      observe_equal_candidate(std::move(candidate));
    }
  }

  void observe_equal_candidate(
      ExactDirectSparseFirstIncidenceMinimizer candidate) {
    if (result_.cominimizers.size() <
        budget_.maximum_cominimizer_count) {
      result_.cominimizers.push_back(std::move(candidate));
      result_.audit.peak_cominimizer_entry_count = std::max(
          result_.audit.peak_cominimizer_entry_count,
          result_.cominimizers.size());
      return;
    }
    if (!cominimizer_overflowed_) {
      cominimizer_overflowed_ = true;
      ++result_.audit.provisional_cominimizer_overflow_count;
    }
  }

  [[nodiscard]] ExactDirectSparseFirstIncidenceResult exhausted(
      ExactDirectSparseFirstIncidenceStopReason stop_reason) {
    if (stop_reason == ExactDirectSparseFirstIncidenceStopReason::none) {
      throw std::logic_error(
          "a direct sparse first-incidence exhaustion needs a stop reason");
    }
    result_.first_incidence_squared_level.reset();
    std::vector<ExactDirectSparseFirstIncidenceMinimizer>{}.swap(
        result_.cominimizers);
    result_.all_cominimizers_retained_atomically = false;
    result_.no_partial_first_incidence_payload_published = true;
    result_.stop_reason = stop_reason;
    result_.decision =
        ExactDirectSparseFirstIncidenceDecision::
            no_first_incidence_budget_exhausted;
    return std::move(result_);
  }

  const spatial::MortonLbvhIndex& index_;
  const spatial::CanonicalPointCloud& cloud_;
  ExactDirectSparseFacetKey source_facet_key_{};
  std::span<const PointId> source_point_ids_;
  ExactDirectSparseFirstIncidenceBudget budget_{};
  spatial::LbvhTraversalOrder traversal_order_;
  ExactDirectSparseFirstIncidenceResult result_{};
  std::optional<exact::ExactLevel> incumbent_squared_level_;
  bool cominimizer_overflowed_{false};
};

bool ExactDirectSparseFirstIncidenceResult::
    certified_complete_first_incidence() const noexcept {
  if (!common_result_contract(*this) ||
      !source_facet_miniball.has_value() ||
      !source_facet_miniball_freshly_certified ||
      !first_incidence_squared_level.has_value() ||
      cominimizers.empty() || !audit.traversal_complete ||
      !audit_covers_eligible_points(audit) ||
      audit.excluded_facet_point_count != source_facet_key.point_count ||
      !every_nonexcluded_point_evaluated_or_strictly_pruned ||
      !all_cominimizers_retained_atomically ||
      stop_reason != ExactDirectSparseFirstIncidenceStopReason::none ||
      decision != ExactDirectSparseFirstIncidenceDecision::
                      complete_exact_first_incidence) {
    return false;
  }
  PointId previous_point_id = 0U;
  bool first = true;
  for (const ExactDirectSparseFirstIncidenceMinimizer& minimizer :
       cominimizers) {
    if (!minimizer_shape_valid(
            minimizer,
            source_facet_key,
            *first_incidence_squared_level) ||
        (!first && minimizer.added_point_id <= previous_point_id)) {
      return false;
    }
    previous_point_id = minimizer.added_point_id;
    first = false;
  }
  return true;
}

bool ExactDirectSparseFirstIncidenceResult::certified_complete_no_coface()
    const noexcept {
  return common_result_contract(*this) &&
         source_facet_miniball.has_value() &&
         source_facet_miniball_freshly_certified &&
         audit.eligible_coface_point_count == 0U &&
         audit.traversal_complete &&
         audit_covers_eligible_points(audit) &&
         audit.excluded_facet_point_count == source_facet_key.point_count &&
         every_nonexcluded_point_evaluated_or_strictly_pruned &&
         all_cominimizers_retained_atomically &&
         !first_incidence_squared_level.has_value() &&
         cominimizers.empty() &&
         stop_reason == ExactDirectSparseFirstIncidenceStopReason::none &&
         decision ==
             ExactDirectSparseFirstIncidenceDecision::complete_no_coface;
}

bool ExactDirectSparseFirstIncidenceResult::certified_budget_exhaustion()
    const noexcept {
  return common_result_contract(*this) &&
         !first_incidence_squared_level.has_value() &&
         cominimizers.empty() && !all_cominimizers_retained_atomically &&
         stop_reason != ExactDirectSparseFirstIncidenceStopReason::none &&
         decision == ExactDirectSparseFirstIncidenceDecision::
                         no_first_incidence_budget_exhausted;
}

ExactDirectSparseFirstIncidenceResult
build_exact_direct_sparse_first_incidence(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSparseFacetKey& source_facet_key,
    const ExactDirectSparseFirstIncidenceBudget& budget,
    spatial::LbvhTraversalOrder traversal_order) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "the Morton LBVH belongs to a different canonical point namespace");
  }
  require_valid_traversal_order(traversal_order);
  require_valid_source_key(cloud, source_facet_key);
  ExactDirectSparseFirstIncidenceBuilder builder{
      index, cloud, source_facet_key, budget, traversal_order};
  ExactDirectSparseFirstIncidenceResult result = builder.run();
  if (!result.certified_complete_first_incidence() &&
      !result.certified_complete_no_coface() &&
      !result.certified_budget_exhaustion()) {
    throw std::logic_error(
        "a direct sparse first incidence produced no certified outcome");
  }
  return result;
}

ExactDirectSparseFirstIncidenceVerification
verify_exact_direct_sparse_first_incidence(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSparseFacetKey& source_facet_key,
    const ExactDirectSparseFirstIncidenceBudget& budget,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseFirstIncidenceResult& observed) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "the Morton LBVH belongs to a different canonical point namespace");
  }
  require_valid_traversal_order(traversal_order);
  require_valid_source_key(cloud, source_facet_key);

  ExactDirectSparseFirstIncidenceVerification verification;
  verification.trusted_inputs_certified = true;
  const std::size_t eligible_point_count =
      cloud.size() - source_facet_key.point_count;
  verification.observed_storage_within_budget =
      observed_storage_within_trusted_bounds(
          observed, eligible_point_count, budget);
  if (!verification.observed_storage_within_budget) {
    return verification;
  }

  const ExactDirectSparseFirstIncidenceResult expected =
      build_exact_direct_sparse_first_incidence(
          index,
          cloud,
          source_facet_key,
          budget,
          traversal_order);
  verification.source_miniball_freshly_replayed =
      same_optional_miniball(
          observed.source_facet_miniball,
          expected.source_facet_miniball) &&
      observed.source_facet_miniball_freshly_certified ==
          expected.source_facet_miniball_freshly_certified;
  verification.branch_and_bound_freshly_replayed =
      observed.audit == expected.audit &&
      observed.first_incidence_squared_level ==
          expected.first_incidence_squared_level &&
      observed.every_nonexcluded_point_evaluated_or_strictly_pruned ==
          expected.every_nonexcluded_point_evaluated_or_strictly_pruned &&
      observed.aabb_lower_bounds_exact_and_valid ==
          expected.aabb_lower_bounds_exact_and_valid &&
      observed.equality_bounds_always_descended ==
          expected.equality_bounds_always_descended;
  verification.all_cominimizers_freshly_replayed =
      observed.cominimizers == expected.cominimizers &&
      observed.all_cominimizers_retained_atomically ==
          expected.all_cominimizers_retained_atomically &&
      observed.no_partial_first_incidence_payload_published ==
          expected.no_partial_first_incidence_payload_published &&
      observed.every_strict_outside_coface_support_contains_added_point ==
          expected.every_strict_outside_coface_support_contains_added_point;
  verification.counters_and_decision_freshly_replayed =
      observed.schema_version == expected.schema_version &&
      observed.source_facet_key == expected.source_facet_key &&
      observed.requested_budget == expected.requested_budget &&
      observed.traversal_order == expected.traversal_order &&
      observed.stop_reason == expected.stop_reason &&
      observed.decision == expected.decision &&
      observed.scope == expected.scope &&
      observed.trusted_authorities_certified ==
          expected.trusted_authorities_certified &&
      observed.public_status_claimed == expected.public_status_claimed &&
      observed.partial_refinement_only == expected.partial_refinement_only;
  verification.no_forbidden_global_structure_materialized =
      observed.no_global_facet_or_coface_catalog_materialized &&
      observed.no_gamma_or_higher_order_delaunay_materialized &&
      observed.no_global_facet_or_coface_catalog_materialized ==
          expected.no_global_facet_or_coface_catalog_materialized &&
      observed.no_gamma_or_higher_order_delaunay_materialized ==
          expected.no_gamma_or_higher_order_delaunay_materialized;
  verification.fresh_replay_certified =
      verification.trusted_inputs_certified &&
      verification.observed_storage_within_budget &&
      verification.source_miniball_freshly_replayed &&
      verification.branch_and_bound_freshly_replayed &&
      verification.all_cominimizers_freshly_replayed &&
      verification.counters_and_decision_freshly_replayed &&
      verification.no_forbidden_global_structure_materialized;
  const bool expected_certified =
      expected.certified_complete_first_incidence() ||
      expected.certified_complete_no_coface() ||
      expected.certified_budget_exhaustion();
  verification.result_certified =
      verification.fresh_replay_certified && expected_certified;
  return verification;
}

}  // namespace morsehgp3d::hierarchy
