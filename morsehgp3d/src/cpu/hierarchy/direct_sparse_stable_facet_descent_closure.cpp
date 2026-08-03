#include "morsehgp3d/hierarchy/direct_sparse_stable_facet_descent_closure.hpp"

#include "morsehgp3d/hierarchy/facet_miniball.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <new>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

[[nodiscard]] bool checked_add(
    std::size_t left, std::size_t right, std::size_t& result) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  result = left + right;
  return true;
}

[[nodiscard]] bool checked_multiply(
    std::size_t left, std::size_t right, std::size_t& result) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return false;
  }
  result = left * right;
  return true;
}

[[nodiscard]] std::size_t capped_binomial_coefficient(
    std::size_t point_count,
    std::size_t facet_cardinality,
    std::size_t cap) noexcept {
  if (cap == 0U || facet_cardinality > point_count) {
    return 0U;
  }
  facet_cardinality =
      std::min(facet_cardinality, point_count - facet_cardinality);
  std::size_t coefficient = 1U;
  if (coefficient >= cap) {
    return cap;
  }
  for (std::size_t index = 1U; index <= facet_cardinality; ++index) {
    std::size_t numerator = point_count - facet_cardinality + index;
    std::size_t denominator = index;
    const std::size_t numerator_gcd = std::gcd(numerator, denominator);
    numerator /= numerator_gcd;
    denominator /= numerator_gcd;
    const std::size_t coefficient_gcd =
        std::gcd(coefficient, denominator);
    coefficient /= coefficient_gcd;
    denominator /= coefficient_gcd;
    if (denominator != 1U || coefficient > cap / numerator) {
      return cap;
    }
    coefficient *= numerator;
    if (coefficient >= cap) {
      return cap;
    }
  }
  return coefficient;
}

[[nodiscard]] bool valid_traversal_order(
    spatial::LbvhTraversalOrder traversal_order) noexcept {
  return traversal_order == spatial::LbvhTraversalOrder::near_first ||
         traversal_order == spatial::LbvhTraversalOrder::far_first;
}

[[nodiscard]] bool canonical_key_for_cloud(
    const ExactDirectSparseFacetKey& key,
    const spatial::CanonicalPointCloud& cloud) noexcept {
  if (key.point_count < 2U ||
      key.point_count > direct_sparse_positive_facet_maximum_point_count ||
      key.point_count > cloud.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    if (static_cast<std::size_t>(key.point_ids[index]) >= cloud.size() ||
        (index != 0U &&
         key.point_ids[index - 1U] >= key.point_ids[index])) {
      return false;
    }
  }
  return std::all_of(
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(key.point_count),
      key.point_ids.end(),
      [](PointId point_id) { return point_id == 0U; });
}

[[nodiscard]] std::span<const PointId> used_point_ids(
    const ExactDirectSparseFacetKey& key) noexcept {
  return std::span<const PointId>{key.point_ids}.first(key.point_count);
}

[[nodiscard]] ExactDirectSparseFacetKey canonical_facet_key(
    const spatial::CanonicalPointCloud& cloud,
    std::span<const PointId> point_ids) {
  ExactDirectSparseFacetKey key;
  if (point_ids.size() < 2U ||
      point_ids.size() > direct_sparse_positive_facet_maximum_point_count) {
    throw std::logic_error(
        "a complete top-k choice has an unsupported facet cardinality");
  }
  key.point_count = point_ids.size();
  for (std::size_t index = 0U; index < point_ids.size(); ++index) {
    if (static_cast<std::size_t>(point_ids[index]) >= cloud.size() ||
        (index != 0U && point_ids[index - 1U] >= point_ids[index])) {
      throw std::logic_error(
          "a complete top-k choice is not a canonical facet key");
    }
    key.point_ids[index] = point_ids[index];
  }
  return key;
}

[[nodiscard]] exact::ExactLevel exact_squared_distance(
    const exact::ExactRational3& left,
    const exact::ExactRational3& right) {
  exact::ExactRational squared_distance;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::ExactRational delta =
        left.coordinate(axis) - right.coordinate(axis);
    squared_distance = squared_distance + delta * delta;
  }
  return exact::ExactLevel{std::move(squared_distance)};
}

[[nodiscard]] bool key_matches_ids(
    const ExactDirectSparseFacetKey& key,
    std::span<const PointId> point_ids) noexcept {
  return key.point_count == point_ids.size() &&
         std::equal(
             point_ids.begin(), point_ids.end(), key.point_ids.begin());
}

[[nodiscard]] bool certified_local_miniball(
    const spatial::CanonicalPointCloud& cloud,
    const ExactFacetMiniballResult& miniball,
    const ExactDirectSparseFacetKey& key) {
  if (miniball.status !=
          ExactFacetMiniballStatus::exact_facet_miniball_certified ||
      miniball.scope != ExactFacetMiniballScope::local_facet_miniball_only ||
      !key_matches_ids(key, miniball.facet_point_ids) ||
      miniball.counters.facet_point_count != key.point_count ||
      miniball.counters.enumerated_support_count >
          ExactFacetMiniballResult::maximum_enumerated_support_count) {
    return false;
  }
  const auto ids = used_point_ids(key);
  exact::ExactLevel maximum_level = exact_squared_distance(
      miniball.center, cloud.point(ids.front()).exact());
  for (std::size_t index = 1U; index < ids.size(); ++index) {
    const exact::ExactLevel level = exact_squared_distance(
        miniball.center, cloud.point(ids[index]).exact());
    if (level > maximum_level) {
      maximum_level = level;
    }
  }
  return maximum_level == miniball.squared_radius;
}

[[nodiscard]] bool attachment_valid(
    const ExactDirectSparseStableFacetPositiveAttachment& attachment)
    noexcept {
  return attachment.component_size != 0U;
}

}  // namespace

std::uint32_t
ExactDirectSparseStableFacetDescentClosureResult::schema_version()
    const noexcept {
  return schema_version_;
}

const ExactDirectSparseStableFacetDescentClosureConfig&
ExactDirectSparseStableFacetDescentClosureResult::config() const & noexcept {
  return config_;
}

const ExactDirectSparseStableFacetDescentClosureBudget&
ExactDirectSparseStableFacetDescentClosureResult::requested_budget()
    const & noexcept {
  return requested_budget_;
}

spatial::LbvhTraversalOrder
ExactDirectSparseStableFacetDescentClosureResult::traversal_order()
    const noexcept {
  return traversal_order_;
}

const exact::ExactLevel& ExactDirectSparseStableFacetDescentClosureResult::
closed_batch_squared_level() const & noexcept {
  return closed_batch_squared_level_;
}

const ExactDirectSparseStableFacetForestStamp&
ExactDirectSparseStableFacetDescentClosureResult::forest_stamp()
    const & noexcept {
  return forest_stamp_;
}

std::size_t
ExactDirectSparseStableFacetDescentClosureResult::common_facet_cardinality()
    const noexcept {
  return common_facet_cardinality_;
}

std::size_t
ExactDirectSparseStableFacetDescentClosureResult::required_memo_slot_count()
    const noexcept {
  return required_memo_slot_count_;
}

std::span<const ExactDirectSparseStableFacetDescentNode>
ExactDirectSparseStableFacetDescentClosureResult::nodes() const & noexcept {
  return nodes_;
}

std::span<const ExactDirectSparseStableFacetDescentEdge>
ExactDirectSparseStableFacetDescentClosureResult::edges() const & noexcept {
  return edges_;
}

std::span<const ExactDirectSparseStableFacetDescentSeedProjection>
ExactDirectSparseStableFacetDescentClosureResult::seed_projections()
    const & noexcept {
  return seed_projections_;
}

const ExactDirectSparseStableFacetDescentClosureCounters&
ExactDirectSparseStableFacetDescentClosureResult::counters() const & noexcept {
  return counters_;
}

ExactDirectSparseStableFacetDescentClosureDisposition
ExactDirectSparseStableFacetDescentClosureResult::disposition()
    const noexcept {
  return disposition_;
}

ExactDirectSparseStableFacetDescentClosureDecision
ExactDirectSparseStableFacetDescentClosureResult::decision() const noexcept {
  return decision_;
}

bool ExactDirectSparseStableFacetDescentClosureResult::certified_common()
    const noexcept {
  std::size_t first_sum = 0U;
  std::size_t second_sum = 0U;
  std::size_t expected_lookup_work = 0U;
  return minted_ &&
         schema_version_ ==
             direct_sparse_stable_facet_descent_closure_schema_version &&
         forest_stamp_.session_instance_id != 0U &&
         trusted_authorities_certified_ && budget_preflight_completed_ &&
         every_memo_candidate_compared_by_full_key_ &&
         no_partial_graph_published_ &&
         no_top_k_partition_or_shell_persisted_ &&
         !forest_state_mutated_ && !global_closed_ball_materialized_ &&
         !forbidden_global_structure_materialized_ &&
         !source_exactness_claimed_ && !hierarchy_attachment_published_ &&
         !public_status_claimed_ &&
         checked_add(
             counters_.positive_key_slot_visit_count,
             counters_.full_key_comparison_count,
             first_sum) &&
         checked_add(
             counters_.handle_index_slot_visit_count,
             counters_.parent_hop_count,
             second_sum) &&
         checked_add(first_sum, second_sum, expected_lookup_work) &&
         counters_.aggregate_lookup_work_count == expected_lookup_work &&
         counters_.memo_full_key_comparison_count <=
             counters_.memo_slot_visit_count &&
         counters_.equal_fingerprint_distinct_key_count <=
             counters_.memo_full_key_comparison_count &&
         counters_.aggregate_lookup_work_count <=
             requested_budget_.maximum_lookup_work_count;
}

bool ExactDirectSparseStableFacetDescentClosureResult::certified_graph()
    const noexcept {
  if (!input_shape_certified_ || !budget_preflight_satisfied_ ||
      !common_forest_stamp_certified_ ||
      !strict_functional_graph_certified_ ||
      !exact_level_acyclicity_certified_ ||
      !edge_node_terminal_identity_certified_ ||
      !all_seed_references_processed_ ||
      nodes_.size() != counters_.interned_node_count ||
      edges_.size() != counters_.strict_edge_count ||
      seed_projections_.size() != counters_.processed_seed_reference_count ||
      nodes_.size() > requested_budget_.maximum_node_count ||
      counters_.evaluated_step_source_count >
          requested_budget_.maximum_step_call_count ||
      counters_.input_seed_reference_count >
          requested_budget_.maximum_seed_count ||
      required_memo_slot_count_ >
          requested_budget_.maximum_memo_slot_count) {
    return false;
  }

  std::size_t terminal_count = 0U;
  std::size_t positive_terminal_count = 0U;
  std::size_t unresolved_terminal_count = 0U;
  std::size_t evaluated_count = 0U;
  for (std::size_t index = 0U; index < nodes_.size(); ++index) {
    const auto& node = nodes_[index];
    if (node.node_index != index || node.terminal_node_index >= nodes_.size() ||
        !node.terminal_pointer_certified ||
        !node.full_miniball_not_persisted ||
        node.exact_center_and_level_present !=
            (node.exact_center.has_value() &&
             node.exact_squared_level.has_value())) {
      return false;
    }
    if (node.step_evaluated) {
      ++evaluated_count;
    }
    if (node.outgoing_edge_index.has_value()) {
      if (*node.outgoing_edge_index >= edges_.size() ||
          node.kind !=
              ExactDirectSparseStableFacetDescentNodeKind::evaluated_source ||
          node.stable_positive_attachment.has_value()) {
        return false;
      }
      const auto& edge = edges_[*node.outgoing_edge_index];
      if (edge.source_node_index != index ||
          edge.target_node_index >= nodes_.size() ||
          node.terminal_node_index !=
              nodes_[edge.target_node_index].terminal_node_index ||
          node.closure_disposition !=
              nodes_[edge.target_node_index].closure_disposition) {
        return false;
      }
      continue;
    }
    ++terminal_count;
    if (node.terminal_node_index != index) {
      return false;
    }
    if (node.closure_disposition ==
        ExactDirectSparseStableFacetDescentClosureDisposition::
            relative_positive) {
      if (node.kind != ExactDirectSparseStableFacetDescentNodeKind::
                           stable_positive_terminal ||
          !node.stable_positive_attachment.has_value() ||
          !attachment_valid(*node.stable_positive_attachment)) {
        return false;
      }
      ++positive_terminal_count;
    } else if (
        node.closure_disposition ==
        ExactDirectSparseStableFacetDescentClosureDisposition::unresolved) {
      if (node.kind != ExactDirectSparseStableFacetDescentNodeKind::
                           unresolved_terminal ||
          node.stable_positive_attachment.has_value()) {
        return false;
      }
      ++unresolved_terminal_count;
    } else {
      return false;
    }
  }

  for (std::size_t index = 0U; index < edges_.size(); ++index) {
    const auto& edge = edges_[index];
    if (edge.edge_index != index ||
        edge.source_node_index >= nodes_.size() ||
        edge.target_node_index >= nodes_.size() ||
        edge.source_node_index == edge.target_node_index ||
        !edge.source_and_target_keys_match_nodes ||
        !edge.target_center_and_level_match_node ||
        !edge.strict_level_decrease_certified ||
        !edge.same_closed_batch_level_certified ||
        nodes_[edge.source_node_index].facet_key !=
            edge.strict_step_witness.source_facet_key ||
        nodes_[edge.target_node_index].facet_key !=
            edge.strict_step_witness.successor_facet_key ||
        !nodes_[edge.source_node_index].exact_center.has_value() ||
        !nodes_[edge.source_node_index].exact_squared_level.has_value() ||
        !nodes_[edge.target_node_index].exact_center.has_value() ||
        !nodes_[edge.target_node_index].exact_squared_level.has_value() ||
        *nodes_[edge.source_node_index].exact_center !=
            edge.strict_step_witness.source_center ||
        *nodes_[edge.source_node_index].exact_squared_level !=
            edge.strict_step_witness.source_facet_squared_level ||
        *nodes_[edge.target_node_index].exact_center !=
            edge.strict_step_witness.successor_center ||
        *nodes_[edge.target_node_index].exact_squared_level !=
            edge.strict_step_witness.successor_facet_squared_level ||
        !edge.strict_step_witness
             .successor_is_complete_canonical_top_k_choice ||
        !edge.strict_step_witness.successor_differs_from_source ||
        !edge.strict_step_witness
             .successor_at_source_equals_top_k_cutoff ||
        !edge.strict_step_witness.top_k_cutoff_at_most_source_level ||
        !edge.strict_step_witness.successor_level_at_most_top_k_cutoff ||
        !edge.strict_step_witness.strict_miniball_level_decrease ||
        !edge.strict_step_witness
             .exact_squared_distance_chord_identity_applies ||
        !edge.strict_step_witness
             .source_facet_at_or_below_closed_batch_level ||
        !edge.strict_step_witness
             .source_open_target_closed_segment_strict_below_source_level ||
        !edge.strict_step_witness
             .source_open_target_closed_segment_strict_below_closed_batch_level ||
        edge.strict_step_witness.successor_at_source_squared_level !=
            edge.strict_step_witness.top_k_cutoff_squared_level) {
      return false;
    }
  }

  std::size_t reused_projection_count = 0U;
  for (std::size_t index = 0U; index < seed_projections_.size(); ++index) {
    const auto& projection = seed_projections_[index];
    if (projection.seed_index != index ||
        projection.root_node_index >= nodes_.size() ||
        projection.terminal_node_index >= nodes_.size() ||
        nodes_[projection.root_node_index].facet_key !=
            projection.source_facet_key ||
        nodes_[projection.root_node_index].terminal_node_index !=
            projection.terminal_node_index ||
        nodes_[projection.root_node_index].closure_disposition !=
            projection.closure_disposition) {
      return false;
    }
    if (projection.reused_existing_node &&
        !checked_add(reused_projection_count, 1U, reused_projection_count)) {
      return false;
    }
  }

  std::size_t forest_cardinality = 0U;
  std::size_t seed_reference_partition = 0U;
  std::size_t expected_probe_count = 0U;
  std::size_t expected_cached_miniball_count = 0U;
  std::size_t source_miniball_acquisition_count = 0U;
  std::size_t successor_miniball_acquisition_count = 0U;
  std::size_t maximum_suffix_reuse_count = 0U;
  return checked_add(edges_.size(), terminal_count, forest_cardinality) &&
         checked_add(
             counters_.distinct_seed_key_count,
             counters_.duplicate_seed_key_reference_count,
             seed_reference_partition) &&
         checked_add(
             counters_.evaluated_step_source_count,
             edges_.size(),
             expected_probe_count) &&
         checked_add(
             counters_.source_miniball_build_count,
             counters_.successor_miniball_build_count,
             expected_cached_miniball_count) &&
         checked_add(
             counters_.source_miniball_build_count,
             counters_.source_miniball_reuse_count,
             source_miniball_acquisition_count) &&
         checked_add(
             counters_.successor_miniball_build_count,
             counters_.successor_miniball_reuse_count,
             successor_miniball_acquisition_count) &&
         checked_add(
             counters_.distinct_seed_key_count,
             edges_.size(),
             maximum_suffix_reuse_count) &&
         forest_cardinality == nodes_.size() &&
         counters_.input_seed_reference_count ==
             counters_.processed_seed_reference_count &&
         counters_.processed_seed_reference_count ==
             seed_projections_.size() &&
         counters_.input_seed_reference_count == seed_reference_partition &&
         counters_.memoized_seed_reuse_count == reused_projection_count &&
         counters_.memoized_suffix_reuse_count <=
             maximum_suffix_reuse_count &&
         counters_.stable_forest_probe_count == expected_probe_count &&
         counters_.distinct_cached_miniball_count ==
             expected_cached_miniball_count &&
         counters_.top_k_query_count <= source_miniball_acquisition_count &&
         edges_.size() <= successor_miniball_acquisition_count &&
         successor_miniball_acquisition_count <=
             counters_.top_k_query_count &&
         terminal_count == counters_.terminal_node_count &&
         positive_terminal_count ==
             counters_.relative_positive_terminal_count &&
         unresolved_terminal_count == counters_.unresolved_terminal_count &&
         evaluated_count == counters_.evaluated_step_source_count;
}

bool ExactDirectSparseStableFacetDescentClosureResult::
certified_complete_relative_positive_closure() const noexcept {
  if (!certified_common() || !certified_graph() ||
      disposition_ !=
          ExactDirectSparseStableFacetDescentClosureDisposition::
              relative_positive ||
      (decision_ != ExactDirectSparseStableFacetDescentClosureDecision::
                        complete_empty_seed_set &&
       decision_ != ExactDirectSparseStableFacetDescentClosureDecision::
                        complete_all_seeds_relative_positive) ||
      counters_.unresolved_terminal_count != 0U) {
    return false;
  }
  return std::all_of(
      seed_projections_.begin(),
      seed_projections_.end(),
      [](const auto& projection) {
        return projection.closure_disposition ==
               ExactDirectSparseStableFacetDescentClosureDisposition::
                   relative_positive;
      });
}

bool ExactDirectSparseStableFacetDescentClosureResult::
certified_complete_with_unresolved_terminals() const noexcept {
  return certified_common() && certified_graph() &&
         disposition_ ==
             ExactDirectSparseStableFacetDescentClosureDisposition::
                 unresolved &&
         decision_ == ExactDirectSparseStableFacetDescentClosureDecision::
                          complete_all_seeds_with_unresolved_terminals &&
         counters_.unresolved_terminal_count != 0U;
}

bool ExactDirectSparseStableFacetDescentClosureResult::
certified_budget_exhaustion() const noexcept {
  const bool budget_decision =
      decision_ == ExactDirectSparseStableFacetDescentClosureDecision::
                       no_preflight_budget_exhausted ||
      decision_ == ExactDirectSparseStableFacetDescentClosureDecision::
                       no_node_budget_exhausted ||
      decision_ == ExactDirectSparseStableFacetDescentClosureDecision::
                       no_step_call_budget_exhausted ||
      decision_ == ExactDirectSparseStableFacetDescentClosureDecision::
                       no_forest_probe_budget_exhausted ||
      decision_ == ExactDirectSparseStableFacetDescentClosureDecision::
                       no_top_k_budget_exhausted ||
      decision_ == ExactDirectSparseStableFacetDescentClosureDecision::
                       no_allocation_failed;
  return certified_common() && budget_decision &&
         common_forest_stamp_certified_ &&
         disposition_ == ExactDirectSparseStableFacetDescentClosureDisposition::
                             budget_exhausted &&
         nodes_.empty() && edges_.empty() && seed_projections_.empty();
}

bool ExactDirectSparseStableFacetDescentClosureResult::
certified_fail_closed_contradiction() const noexcept {
  const bool contradiction_decision =
      decision_ == ExactDirectSparseStableFacetDescentClosureDecision::
                       contradiction_forest_probe ||
      decision_ == ExactDirectSparseStableFacetDescentClosureDecision::
                       contradiction_forest_stamp_changed ||
      decision_ == ExactDirectSparseStableFacetDescentClosureDecision::
                       contradiction_exact_geometry ||
      decision_ == ExactDirectSparseStableFacetDescentClosureDecision::
                       contradiction_cycle_or_inconsistent_shared_target;
  const bool stamp_relation_certified =
      decision_ == ExactDirectSparseStableFacetDescentClosureDecision::
                       contradiction_forest_stamp_changed
          ? !common_forest_stamp_certified_
          : common_forest_stamp_certified_;
  return certified_common() && contradiction_decision &&
         stamp_relation_certified &&
         disposition_ == ExactDirectSparseStableFacetDescentClosureDisposition::
                             contradiction &&
         nodes_.empty() && edges_.empty() && seed_projections_.empty();
}

bool ExactDirectSparseStableFacetDescentClosureResult::certified_outcome()
    const noexcept {
  return certified_complete_relative_positive_closure() ||
         certified_complete_with_unresolved_terminals() ||
         certified_budget_exhaustion() ||
         certified_fail_closed_contradiction();
}

bool ExactDirectSparseStableFacetDescentClosureResult::certified_for(
    const ExactDirectSparseStableFacetForestStamp& expected_stamp)
    const noexcept {
  return certified_outcome() && forest_stamp_ == expected_stamp;
}

bool ExactDirectSparseStableFacetDescentClosureResult::certified_for(
    const ExactDirectSparseStableFacetForest& forest) const noexcept {
  return certified_for(forest.current_stamp());
}

class ExactDirectSparseStableFacetDescentClosureBuilder {
 public:
  ExactDirectSparseStableFacetDescentClosureBuilder(
      ExactDirectSparseStableFacetDescentClosureResult& result,
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::span<const ExactDirectSparseStableFacetDescentClosureSeed> seeds,
      const ExactDirectSparseStableFacetForest& forest)
      : result_(result),
        index_(index),
        cloud_(cloud),
        input_seeds_(seeds),
        forest_(forest) {}

  void run() {
    try {
      if (!preflight()) {
        return;
      }
      if (!allocate_seed_validation_scratch()) {
        return;
      }
      if (!validate_and_order_seeds()) {
        reject_input_shape();
        return;
      }
      if (!configure_memo_after_validation()) {
        return;
      }
      if (ordered_seeds_.empty()) {
        finalize_success();
        return;
      }
      if (!preintern_seed_roots() || !allocate_output_scratch()) {
        return;
      }
      for (const auto& seed : ordered_seeds_) {
        if (!process_seed(seed)) {
          return;
        }
      }
      finalize_success();
    } catch (const std::bad_alloc&) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              no_allocation_failed,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              budget_exhausted);
    }
  }

 private:
  enum class NodeState : std::uint8_t {
    unvisited,
    visiting,
    closed,
  };

  struct MemoProbe {
    bool found{false};
    bool insertion_slot_available{false};
    bool failed{false};
    std::size_t node_index{};
    std::size_t insertion_slot{};
  };

  struct Geometry {
    exact::ExactCenter3 center{};
    exact::ExactLevel squared_level{};
  };

  struct TransientGeometryCacheSlot {
    std::uint64_t fingerprint{};
    std::size_t cache_index{};
    bool occupied{false};
  };

  struct TransientGeometryCacheEntry {
    ExactDirectSparseFacetKey facet_key{};
    Geometry geometry{};
  };

  struct TransientGeometryCacheProbe {
    bool found{false};
    bool insertion_slot_available{false};
    bool failed{false};
    std::size_t cache_index{};
    std::size_t insertion_slot{};
  };

  enum class GeometryRole : std::uint8_t {
    source,
    successor,
  };

  struct Evaluation {
    bool completed{false};
    bool has_next{false};
    std::size_t next_node_index{};
  };

  [[nodiscard]] bool preflight() {
    auto& result = result_;
    const auto& budget = result.requested_budget_;
    result.counters_.input_seed_reference_count = input_seeds_.size();
    result.budget_preflight_completed_ = true;

    if (budget.maximum_seed_count >
            direct_sparse_stable_facet_descent_closure_maximum_seed_count ||
        budget.maximum_node_count >
            direct_sparse_stable_facet_descent_closure_maximum_node_count ||
        budget.maximum_step_call_count >
            direct_sparse_stable_facet_descent_closure_maximum_step_call_count ||
        budget.maximum_memo_slot_count >
            direct_sparse_stable_facet_descent_closure_maximum_memo_slot_count) {
      throw std::invalid_argument(
          "a stable-facet descent-closure budget exceeds its confidence cap");
    }

    std::size_t first_probe_sum = 0U;
    std::size_t second_probe_sum = 0U;
    per_probe_worst_work_valid_ =
        checked_add(
            budget.positive_probe.maximum_positive_key_slot_visit_count,
            budget.positive_probe.maximum_full_key_comparison_count,
            first_probe_sum) &&
        checked_add(
            budget.positive_probe.maximum_handle_index_slot_visit_count,
            budget.positive_probe.maximum_parent_hop_count,
            second_probe_sum) &&
        checked_add(
            first_probe_sum, second_probe_sum, per_probe_worst_work_);
    if (!per_probe_worst_work_valid_) {
      fail_preflight();
      return false;
    }
    if (input_seeds_.size() > budget.maximum_seed_count) {
      fail_preflight();
      return false;
    }
    if (!input_seeds_.empty() && budget.maximum_node_count == 0U) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              no_node_budget_exhausted,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              budget_exhausted);
      return false;
    }
    if (!input_seeds_.empty() && budget.maximum_step_call_count == 0U) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              no_step_call_budget_exhausted,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              budget_exhausted);
      return false;
    }
    return true;
  }

  void fail_preflight() {
    result_.budget_preflight_satisfied_ = false;
    fail(
        ExactDirectSparseStableFacetDescentClosureDecision::
            no_preflight_budget_exhausted,
        ExactDirectSparseStableFacetDescentClosureDisposition::
            budget_exhausted);
  }

  [[nodiscard]] bool allocate_seed_validation_scratch() {
    try {
      ordered_seeds_.assign(input_seeds_.begin(), input_seeds_.end());
      return true;
    } catch (const std::bad_alloc&) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              no_allocation_failed,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              budget_exhausted);
      return false;
    }
  }

  [[nodiscard]] bool allocate_output_scratch() {
    try {
      result_.seed_projections_.reserve(input_seeds_.size());
      return true;
    } catch (const std::bad_alloc&) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              no_allocation_failed,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              budget_exhausted);
      return false;
    }
  }

  [[nodiscard]] bool ensure_node_capacity(std::size_t required_capacity) {
    if (required_capacity <= result_.nodes_.capacity() &&
        required_capacity <= result_.edges_.capacity() &&
        required_capacity <= states_.capacity() &&
        required_capacity <= seen_as_seed_.capacity() &&
        required_capacity <= path_.capacity()) {
      return true;
    }
    const std::size_t maximum_capacity =
        result_.requested_budget_.maximum_node_count;
    if (required_capacity > maximum_capacity) {
      return false;
    }
    std::size_t target_capacity = std::min(
        result_.nodes_.capacity(), maximum_capacity);
    if (target_capacity == 0U) {
      target_capacity = 1U;
    }
    while (target_capacity < required_capacity) {
      const std::size_t available = maximum_capacity - target_capacity;
      const std::size_t growth = std::min(target_capacity, available);
      if (growth == 0U) {
        target_capacity = maximum_capacity;
        break;
      }
      target_capacity += growth;
    }
    if (target_capacity < required_capacity) {
      return false;
    }
    try {
      result_.nodes_.reserve(target_capacity);
      result_.edges_.reserve(target_capacity);
      states_.reserve(target_capacity);
      seen_as_seed_.reserve(target_capacity);
      path_.reserve(target_capacity);
      return true;
    } catch (const std::bad_alloc&) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              no_allocation_failed,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              budget_exhausted);
      return false;
    }
  }

  [[nodiscard]] bool validate_and_order_seeds() {
    std::sort(
        ordered_seeds_.begin(),
        ordered_seeds_.end(),
        [](const auto& left, const auto& right) {
          return left.seed_index < right.seed_index;
        });
    std::size_t common_cardinality = 0U;
    for (std::size_t index = 0U; index < ordered_seeds_.size(); ++index) {
      const auto& seed = ordered_seeds_[index];
      if (seed.seed_index != index ||
          !canonical_key_for_cloud(seed.source_facet_key, cloud_)) {
        return false;
      }
      if (index == 0U) {
        common_cardinality = seed.source_facet_key.point_count;
      } else if (
          seed.source_facet_key.point_count != common_cardinality) {
        return false;
      }
    }
    result_.common_facet_cardinality_ = common_cardinality;
    result_.input_shape_certified_ = true;
    return true;
  }

  [[nodiscard]] bool configure_memo_after_validation() {
    std::size_t required_slots = 0U;
    std::size_t geometric_node_bound = 0U;
    if (!ordered_seeds_.empty() &&
        result_.requested_budget_.maximum_node_count != 0U) {
      geometric_node_bound = capped_binomial_coefficient(
          cloud_.size(),
          result_.common_facet_cardinality_,
          result_.requested_budget_.maximum_node_count);
      std::size_t doubled_node_bound = 0U;
      if (!checked_multiply(
              geometric_node_bound, 2U, doubled_node_bound) ||
          !checked_add(doubled_node_bound, 1U, required_slots)) {
        fail_preflight();
        return false;
      }
    }
    result_.required_memo_slot_count_ = required_slots;
    if (required_slots >
        result_.requested_budget_.maximum_memo_slot_count) {
      fail_preflight();
      return false;
    }
    try {
      memo_slots_.assign(required_slots, 0U);
      transient_geometry_cache_slots_.assign(required_slots, {});
    } catch (const std::bad_alloc&) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              no_allocation_failed,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              budget_exhausted);
      return false;
    }
    transient_geometry_cache_maximum_entry_count_ = std::min(
        geometric_node_bound,
        result_.requested_budget_.maximum_step_call_count);
    result_.budget_preflight_satisfied_ = true;
    return true;
  }

  void reject_input_shape() {
    clear_published_graph();
    result_.decision_ =
        ExactDirectSparseStableFacetDescentClosureDecision::
            no_input_shape_rejected;
    result_.disposition_ =
        ExactDirectSparseStableFacetDescentClosureDisposition::not_certified;
    result_.common_forest_stamp_certified_ =
        forest_.current_stamp() == result_.forest_stamp_;
    result_.minted_ = false;
  }

  [[nodiscard]] bool increment_memo_work_counter(
      std::size_t& counter) {
    std::size_t next = 0U;
    if (!checked_add(counter, 1U, next)) {
      fail_preflight();
      return false;
    }
    counter = next;
    return true;
  }

  [[nodiscard]] MemoProbe probe_memo(
      const ExactDirectSparseFacetKey& key) {
    MemoProbe result;
    if (memo_slots_.empty()) {
      return result;
    }
    const std::uint64_t requested_fingerprint =
        fingerprint_exact_direct_sparse_facet_key(
            key, result_.config_.memo_fingerprint_mask);
    std::size_t slot =
        static_cast<std::size_t>(requested_fingerprint) % memo_slots_.size();
    for (std::size_t probe = 0U; probe < memo_slots_.size(); ++probe) {
      if (!increment_memo_work_counter(
              result_.counters_.memo_slot_visit_count)) {
        result.failed = true;
        return result;
      }
      const std::size_t node_plus_one = memo_slots_[slot];
      if (node_plus_one == 0U) {
        result.insertion_slot_available = true;
        result.insertion_slot = slot;
        return result;
      }
      const std::size_t node_index = node_plus_one - 1U;
      if (node_index >= result_.nodes_.size()) {
        fail(
            ExactDirectSparseStableFacetDescentClosureDecision::
                contradiction_cycle_or_inconsistent_shared_target,
            ExactDirectSparseStableFacetDescentClosureDisposition::
                contradiction);
        result.failed = true;
        return result;
      }
      const auto& stored_key = result_.nodes_[node_index].facet_key;
      if (fingerprint_exact_direct_sparse_facet_key(
              stored_key, result_.config_.memo_fingerprint_mask) ==
          requested_fingerprint) {
        if (!increment_memo_work_counter(
                result_.counters_.memo_full_key_comparison_count)) {
          result.failed = true;
          return result;
        }
        if (stored_key == key) {
          result.found = true;
          result.node_index = node_index;
          return result;
        }
        if (!increment_memo_work_counter(
                result_.counters_.equal_fingerprint_distinct_key_count)) {
          result.failed = true;
          return result;
        }
      }
      slot = (slot + 1U) % memo_slots_.size();
    }
    return result;
  }

  [[nodiscard]] std::optional<std::size_t> insert_node(
      const ExactDirectSparseFacetKey& key,
      const MemoProbe& probe) {
    if (probe.failed || !probe.insertion_slot_available || probe.found ||
        result_.nodes_.size() >=
            result_.requested_budget_.maximum_node_count) {
      return std::nullopt;
    }
    const std::size_t node_index = result_.nodes_.size();
    if (!ensure_node_capacity(node_index + 1U)) {
      return std::nullopt;
    }
    ExactDirectSparseStableFacetDescentNode node;
    node.node_index = node_index;
    node.facet_key = key;
    node.terminal_node_index = node_index;
    node.full_miniball_not_persisted = true;
    result_.nodes_.push_back(std::move(node));
    states_.push_back(NodeState::unvisited);
    seen_as_seed_.push_back(false);
    memo_slots_[probe.insertion_slot] = node_index + 1U;
    result_.counters_.interned_node_count = result_.nodes_.size();
    return node_index;
  }

  [[nodiscard]] bool preintern_seed_roots() {
    for (const auto& seed : ordered_seeds_) {
      const MemoProbe probe = probe_memo(seed.source_facet_key);
      if (probe.failed) {
        return false;
      }
      if (probe.found) {
        continue;
      }
      const auto inserted = insert_node(seed.source_facet_key, probe);
      if (!inserted.has_value()) {
        if (!result_.minted_) {
          fail(
              ExactDirectSparseStableFacetDescentClosureDecision::
                  no_node_budget_exhausted,
              ExactDirectSparseStableFacetDescentClosureDisposition::
                  budget_exhausted);
        }
        return false;
      }
    }
    return true;
  }

  [[nodiscard]] bool reserve_probe_work() {
    std::size_t reserved_end = 0U;
    if (!per_probe_worst_work_valid_ ||
        !checked_add(
            result_.counters_.aggregate_lookup_work_count,
            per_probe_worst_work_,
            reserved_end) ||
        reserved_end > result_.requested_budget_.maximum_lookup_work_count) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              no_forest_probe_budget_exhausted,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              budget_exhausted);
      return false;
    }
    return true;
  }

  [[nodiscard]] bool run_forest_probe(
      const ExactDirectSparseFacetKey& key,
      ExactDirectSparseStableFacetPositiveProbeResult& output) {
    if (!reserve_probe_work()) {
      return false;
    }
    output = forest_.probe_positive_full_key(
        key, result_.requested_budget_.positive_probe);
    std::size_t next_probe_count = 0U;
    std::size_t next_positive_key_slots = 0U;
    std::size_t next_full_key_comparisons = 0U;
    std::size_t next_handle_index_slots = 0U;
    std::size_t next_parent_hops = 0U;
    std::size_t first = 0U;
    std::size_t second = 0U;
    std::size_t total = 0U;
    if (!checked_add(
            result_.counters_.stable_forest_probe_count,
            1U,
            next_probe_count) ||
        !checked_add(
            result_.counters_.positive_key_slot_visit_count,
            output.positive_key_slot_visit_count,
            next_positive_key_slots) ||
        !checked_add(
            result_.counters_.full_key_comparison_count,
            output.full_key_comparison_count,
            next_full_key_comparisons) ||
        !checked_add(
            result_.counters_.handle_index_slot_visit_count,
            output.handle_index_slot_visit_count,
            next_handle_index_slots) ||
        !checked_add(
            result_.counters_.parent_hop_count,
            output.parent_hop_count,
            next_parent_hops) ||
        !checked_add(
            next_positive_key_slots,
            next_full_key_comparisons,
            first) ||
        !checked_add(
            next_handle_index_slots,
            next_parent_hops,
            second) ||
        !checked_add(first, second, total)) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              no_forest_probe_budget_exhausted,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              budget_exhausted);
      return false;
    }
    result_.counters_.stable_forest_probe_count = next_probe_count;
    result_.counters_.positive_key_slot_visit_count =
        next_positive_key_slots;
    result_.counters_.full_key_comparison_count =
        next_full_key_comparisons;
    result_.counters_.handle_index_slot_visit_count =
        next_handle_index_slots;
    result_.counters_.parent_hop_count = next_parent_hops;
    result_.counters_.aggregate_lookup_work_count = total;
    const auto current_stamp = forest_.current_stamp();
    if (current_stamp != result_.forest_stamp_ ||
        output.forest_stamp != result_.forest_stamp_) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              contradiction_forest_stamp_changed,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              contradiction);
      return false;
    }
    if (!output.certified_for(result_.forest_stamp_)) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              contradiction_forest_probe,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              contradiction);
      return false;
    }
    return true;
  }

  [[nodiscard]] TransientGeometryCacheProbe
  probe_transient_geometry_cache(
      const ExactDirectSparseFacetKey& key) {
    TransientGeometryCacheProbe result;
    if (transient_geometry_cache_slots_.empty()) {
      return result;
    }
    const std::uint64_t requested_fingerprint =
        fingerprint_exact_direct_sparse_facet_key(
            key, result_.config_.memo_fingerprint_mask);
    std::size_t slot =
        static_cast<std::size_t>(requested_fingerprint) %
        transient_geometry_cache_slots_.size();
    for (std::size_t probe = 0U;
         probe < transient_geometry_cache_slots_.size();
         ++probe) {
      if (!increment_memo_work_counter(
              result_.counters_.memo_slot_visit_count)) {
        result.failed = true;
        return result;
      }
      const auto& cache_slot = transient_geometry_cache_slots_[slot];
      if (!cache_slot.occupied) {
        result.insertion_slot_available = true;
        result.insertion_slot = slot;
        return result;
      }
      if (cache_slot.fingerprint == requested_fingerprint) {
        if (!increment_memo_work_counter(
                result_.counters_.memo_full_key_comparison_count)) {
          result.failed = true;
          return result;
        }
        if (cache_slot.cache_index >=
            transient_geometry_cache_entries_.size()) {
          fail(
              ExactDirectSparseStableFacetDescentClosureDecision::
                  contradiction_cycle_or_inconsistent_shared_target,
              ExactDirectSparseStableFacetDescentClosureDisposition::
                  contradiction);
          result.failed = true;
          return result;
        }
        const auto& cached =
            transient_geometry_cache_entries_[cache_slot.cache_index];
        if (cached.facet_key == key) {
          result.found = true;
          result.cache_index = cache_slot.cache_index;
          return result;
        }
        if (!increment_memo_work_counter(
                result_.counters_.equal_fingerprint_distinct_key_count)) {
          result.failed = true;
          return result;
        }
      }
      slot = (slot + 1U) % transient_geometry_cache_slots_.size();
    }
    return result;
  }

  [[nodiscard]] bool ensure_transient_geometry_cache_capacity(
      std::size_t required_capacity) {
    if (required_capacity <= transient_geometry_cache_entries_.capacity()) {
      return true;
    }
    if (required_capacity >
        transient_geometry_cache_maximum_entry_count_) {
      return false;
    }
    std::size_t target_capacity = std::min(
        transient_geometry_cache_entries_.capacity(),
        transient_geometry_cache_maximum_entry_count_);
    if (target_capacity == 0U) {
      target_capacity = 1U;
    }
    while (target_capacity < required_capacity) {
      const std::size_t available =
          transient_geometry_cache_maximum_entry_count_ - target_capacity;
      const std::size_t growth = std::min(target_capacity, available);
      if (growth == 0U) {
        target_capacity =
            transient_geometry_cache_maximum_entry_count_;
        break;
      }
      target_capacity += growth;
    }
    if (target_capacity < required_capacity) {
      return false;
    }
    try {
      transient_geometry_cache_entries_.reserve(target_capacity);
      return true;
    } catch (const std::bad_alloc&) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              no_allocation_failed,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              budget_exhausted);
      return false;
    }
  }

  [[nodiscard]] bool cache_transient_successor_geometry(
      const ExactDirectSparseFacetKey& key,
      const Geometry& geometry,
      const TransientGeometryCacheProbe& probe) {
    if (probe.failed) {
      return false;
    }
    if (probe.found) {
      if (probe.cache_index >= transient_geometry_cache_entries_.size()) {
        fail(
            ExactDirectSparseStableFacetDescentClosureDecision::
                contradiction_cycle_or_inconsistent_shared_target,
            ExactDirectSparseStableFacetDescentClosureDisposition::
                contradiction);
        return false;
      }
      const auto& cached =
          transient_geometry_cache_entries_[probe.cache_index];
      if (cached.facet_key != key || cached.geometry.center != geometry.center ||
          cached.geometry.squared_level != geometry.squared_level) {
        fail(
            ExactDirectSparseStableFacetDescentClosureDecision::
                contradiction_cycle_or_inconsistent_shared_target,
            ExactDirectSparseStableFacetDescentClosureDisposition::
                contradiction);
        return false;
      }
      return true;
    }
    if (!probe.insertion_slot_available ||
        transient_geometry_cache_entries_.size() >=
            transient_geometry_cache_maximum_entry_count_) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              no_step_call_budget_exhausted,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              budget_exhausted);
      return false;
    }
    const std::size_t cache_index =
        transient_geometry_cache_entries_.size();
    if (!ensure_transient_geometry_cache_capacity(cache_index + 1U)) {
      if (!result_.minted_) {
        fail(
            ExactDirectSparseStableFacetDescentClosureDecision::
                no_step_call_budget_exhausted,
            ExactDirectSparseStableFacetDescentClosureDisposition::
                budget_exhausted);
      }
      return false;
    }
    transient_geometry_cache_entries_.push_back({key, geometry});
    transient_geometry_cache_slots_[probe.insertion_slot] =
        TransientGeometryCacheSlot{
            fingerprint_exact_direct_sparse_facet_key(
                key, result_.config_.memo_fingerprint_mask),
            cache_index,
            true};
    return true;
  }

  [[nodiscard]] std::optional<Geometry> acquire_geometry(
      const ExactDirectSparseFacetKey& key,
      std::optional<std::size_t> existing_node,
      GeometryRole role) {
    if (existing_node.has_value()) {
      if (*existing_node >= result_.nodes_.size()) {
        fail(
            ExactDirectSparseStableFacetDescentClosureDecision::
                contradiction_cycle_or_inconsistent_shared_target,
            ExactDirectSparseStableFacetDescentClosureDisposition::
                contradiction);
        return std::nullopt;
      }
      const auto& node = result_.nodes_[*existing_node];
      if (node.exact_center.has_value() &&
          node.exact_squared_level.has_value()) {
        if (role == GeometryRole::source) {
          ++result_.counters_.source_miniball_reuse_count;
        } else {
          ++result_.counters_.successor_miniball_reuse_count;
        }
        return Geometry{*node.exact_center, *node.exact_squared_level};
      }
      if (node.exact_center.has_value() !=
          node.exact_squared_level.has_value()) {
        fail_geometry();
        return std::nullopt;
      }
    }
    const TransientGeometryCacheProbe cache_probe =
        probe_transient_geometry_cache(key);
    if (cache_probe.failed) {
      return std::nullopt;
    }
    if (cache_probe.found) {
      if (cache_probe.cache_index >=
          transient_geometry_cache_entries_.size()) {
        fail(
            ExactDirectSparseStableFacetDescentClosureDecision::
                contradiction_cycle_or_inconsistent_shared_target,
            ExactDirectSparseStableFacetDescentClosureDisposition::
                contradiction);
        return std::nullopt;
      }
      if (role == GeometryRole::source) {
        ++result_.counters_.source_miniball_reuse_count;
      } else {
        ++result_.counters_.successor_miniball_reuse_count;
      }
      return transient_geometry_cache_entries_[cache_probe.cache_index]
          .geometry;
    }
    const ExactFacetMiniballResult miniball =
        build_exact_facet_miniball(cloud_, used_point_ids(key));
    if (!certified_local_miniball(cloud_, miniball, key)) {
      fail_geometry();
      return std::nullopt;
    }
    const Geometry geometry{miniball.center, miniball.squared_radius};
    if (role == GeometryRole::successor &&
        !cache_transient_successor_geometry(key, geometry, cache_probe)) {
      return std::nullopt;
    }
    if (role == GeometryRole::source) {
      ++result_.counters_.source_miniball_build_count;
    } else {
      ++result_.counters_.successor_miniball_build_count;
    }
    ++result_.counters_.distinct_cached_miniball_count;
    return geometry;
  }

  [[nodiscard]] bool assign_geometry(
      std::size_t node_index, const Geometry& geometry) {
    auto& node = result_.nodes_[node_index];
    if (node.exact_center.has_value() ||
        node.exact_squared_level.has_value()) {
      if (!node.exact_center.has_value() ||
          !node.exact_squared_level.has_value() ||
          *node.exact_center != geometry.center ||
          *node.exact_squared_level != geometry.squared_level) {
        fail(
            ExactDirectSparseStableFacetDescentClosureDecision::
                contradiction_cycle_or_inconsistent_shared_target,
            ExactDirectSparseStableFacetDescentClosureDisposition::
                contradiction);
        return false;
      }
      return true;
    }
    node.exact_center = geometry.center;
    node.exact_squared_level = geometry.squared_level;
    node.exact_center_and_level_present = true;
    return true;
  }

  void fail_geometry() {
    fail(
        ExactDirectSparseStableFacetDescentClosureDecision::
            contradiction_exact_geometry,
        ExactDirectSparseStableFacetDescentClosureDisposition::contradiction);
  }

  void mark_positive_terminal(
      std::size_t node_index,
      const ExactDirectSparseStableFacetPositiveProbeResult& probe,
      ExactDirectSparseStableFacetDescentLocalDecision local_decision,
      bool step_evaluated) {
    auto& node = result_.nodes_[node_index];
    node.outgoing_edge_index.reset();
    node.terminal_node_index = node_index;
    node.stable_positive_attachment =
        ExactDirectSparseStableFacetPositiveAttachment{
            probe.bound_handle, probe.root_handle, probe.component_size};
    node.local_decision = local_decision;
    node.closure_disposition =
        ExactDirectSparseStableFacetDescentClosureDisposition::
            relative_positive;
    node.kind = ExactDirectSparseStableFacetDescentNodeKind::
        stable_positive_terminal;
    node.step_evaluated = step_evaluated;
    node.terminal_pointer_certified = true;
    states_[node_index] = NodeState::closed;
    ++result_.counters_.terminal_node_count;
    ++result_.counters_.relative_positive_terminal_count;
  }

  void mark_unresolved_terminal(
      std::size_t node_index,
      ExactDirectSparseStableFacetDescentLocalDecision local_decision) {
    auto& node = result_.nodes_[node_index];
    node.outgoing_edge_index.reset();
    node.terminal_node_index = node_index;
    node.stable_positive_attachment.reset();
    node.local_decision = local_decision;
    node.closure_disposition =
        ExactDirectSparseStableFacetDescentClosureDisposition::unresolved;
    node.kind =
        ExactDirectSparseStableFacetDescentNodeKind::unresolved_terminal;
    node.step_evaluated = true;
    node.terminal_pointer_certified = true;
    states_[node_index] = NodeState::closed;
    ++result_.counters_.terminal_node_count;
    ++result_.counters_.unresolved_terminal_count;
  }

  [[nodiscard]] Evaluation evaluate_node(std::size_t source_index) {
    Evaluation outcome;
    if (result_.counters_.evaluated_step_source_count >=
        result_.requested_budget_.maximum_step_call_count) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              no_step_call_budget_exhausted,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              budget_exhausted);
      return outcome;
    }
    ++result_.counters_.evaluated_step_source_count;
    auto& source_node = result_.nodes_[source_index];
    source_node.step_evaluated = true;

    ExactDirectSparseStableFacetPositiveProbeResult source_probe;
    if (!run_forest_probe(source_node.facet_key, source_probe)) {
      return outcome;
    }
    if (source_probe.certified_budget_exhaustion()) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              no_forest_probe_budget_exhausted,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              budget_exhausted);
      return outcome;
    }
    if (source_probe.certified_fail_closed_contradiction()) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              contradiction_forest_probe,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              contradiction);
      return outcome;
    }
    if (source_probe.certified_observed()) {
      mark_positive_terminal(
          source_index,
          source_probe,
          ExactDirectSparseStableFacetDescentLocalDecision::
              complete_source_stable_positive_hit,
          true);
      outcome.completed = true;
      return outcome;
    }
    if (!source_probe.certified_unobserved()) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              contradiction_forest_probe,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              contradiction);
      return outcome;
    }

    const auto source_geometry = acquire_geometry(
        source_node.facet_key, source_index, GeometryRole::source);
    if (!source_geometry.has_value() ||
        !assign_geometry(source_index, *source_geometry)) {
      return outcome;
    }
    if (source_geometry->squared_level >
        result_.closed_batch_squared_level_) {
      mark_unresolved_terminal(
          source_index,
          ExactDirectSparseStableFacetDescentLocalDecision::
              complete_unresolved_source_above_closed_batch_level);
      outcome.completed = true;
      return outcome;
    }

    const spatial::ExclusionSet empty_exclusions =
        spatial::ExclusionSet::from_ids(
            std::span<const PointId>{}, cloud_, 0U);
    auto top_k_result = spatial::lbvh_top_k_budgeted(
        index_,
        cloud_,
        source_geometry->center,
        source_node.facet_key.point_count,
        empty_exclusions,
        used_point_ids(source_node.facet_key),
        std::span<const PointId>{},
        result_.requested_budget_.top_k_query,
        result_.traversal_order_);
    ++result_.counters_.top_k_query_count;
    if (!top_k_result.complete()) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              no_top_k_budget_exhausted,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              budget_exhausted);
      return outcome;
    }
    const spatial::TopKPartition& top_k = top_k_result.partition();
    if (!top_k.validated_for(cloud_) || !top_k.shell_complete() ||
        top_k.requested_rank() != source_node.facet_key.point_count ||
        top_k.eligible_point_count() != cloud_.size() ||
        top_k.query_counters().method !=
            spatial::SpatialQueryMethod::morton_lbvh ||
        top_k.query_counters().excluded_point_count != 0U ||
        top_k_result.audit().supplied_incumbent_point_count !=
            source_node.facet_key.point_count ||
        top_k_result.audit().exact_incumbent_distance_evaluation_count !=
            source_node.facet_key.point_count ||
        !top_k_result.audit().traversal_complete ||
        top_k.cutoff_squared_distance() >
            source_geometry->squared_level) {
      fail_geometry();
      return outcome;
    }

    const ExactDirectSparseFacetKey successor_key =
        canonical_facet_key(cloud_, top_k.canonical_choice_ids());
    if (successor_key == source_node.facet_key) {
      mark_unresolved_terminal(
          source_index,
          ExactDirectSparseStableFacetDescentLocalDecision::
              complete_unresolved_source_is_canonical_top_k_choice);
      outcome.completed = true;
      return outcome;
    }

    const MemoProbe successor_memo = probe_memo(successor_key);
    if (successor_memo.failed) {
      return outcome;
    }
    if (!successor_memo.found &&
        (!successor_memo.insertion_slot_available ||
         result_.nodes_.size() >=
             result_.requested_budget_.maximum_node_count)) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              no_node_budget_exhausted,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              budget_exhausted);
      return outcome;
    }
    std::optional<Geometry> successor_geometry;
    if (successor_memo.found) {
      successor_geometry = acquire_geometry(
          successor_key,
          std::optional<std::size_t>{successor_memo.node_index},
          GeometryRole::successor);
    } else {
      successor_geometry = acquire_geometry(
          successor_key, std::nullopt, GeometryRole::successor);
    }
    if (!successor_geometry.has_value()) {
      return outcome;
    }
    if (successor_geometry->squared_level >
        top_k.cutoff_squared_distance()) {
      fail_geometry();
      return outcome;
    }
    if (successor_geometry->squared_level >=
        source_geometry->squared_level) {
      mark_unresolved_terminal(
          source_index,
          ExactDirectSparseStableFacetDescentLocalDecision::
              complete_unresolved_non_strict_canonical_successor);
      outcome.completed = true;
      return outcome;
    }

    const auto successor_ids = used_point_ids(successor_key);
    exact::ExactLevel successor_at_source = exact_squared_distance(
        source_geometry->center,
        cloud_.point(successor_ids.front()).exact());
    for (std::size_t index = 1U; index < successor_ids.size(); ++index) {
      const exact::ExactLevel point_level = exact_squared_distance(
          source_geometry->center,
          cloud_.point(successor_ids[index]).exact());
      if (point_level > successor_at_source) {
        successor_at_source = point_level;
      }
    }
    if (successor_at_source != top_k.cutoff_squared_distance()) {
      fail_geometry();
      return outcome;
    }

    ExactDirectSparseFacetDescentStepWitness witness;
    witness.source_facet_key = source_node.facet_key;
    witness.successor_facet_key = successor_key;
    witness.source_center = source_geometry->center;
    witness.successor_center = successor_geometry->center;
    witness.source_facet_squared_level = source_geometry->squared_level;
    witness.top_k_cutoff_squared_level = top_k.cutoff_squared_distance();
    witness.successor_at_source_squared_level = successor_at_source;
    witness.successor_facet_squared_level =
        successor_geometry->squared_level;
    witness.center_squared_displacement = exact_squared_distance(
        source_geometry->center, successor_geometry->center);
    witness.successor_is_complete_canonical_top_k_choice = true;
    witness.successor_differs_from_source = true;
    witness.successor_at_source_equals_top_k_cutoff = true;
    witness.top_k_cutoff_at_most_source_level = true;
    witness.successor_level_at_most_top_k_cutoff = true;
    witness.strict_miniball_level_decrease = true;
    witness.exact_squared_distance_chord_identity_applies = true;
    witness.source_facet_at_or_below_closed_batch_level = true;
    witness.source_open_target_closed_segment_strict_below_source_level =
        true;
    witness.source_open_target_closed_segment_strict_below_closed_batch_level =
        true;
    witness.closed_segment_strict_below_closed_batch_level =
        top_k.cutoff_squared_distance() <
        result_.closed_batch_squared_level_;

    ExactDirectSparseStableFacetPositiveProbeResult successor_probe;
    if (!run_forest_probe(successor_key, successor_probe)) {
      return outcome;
    }
    if (successor_probe.certified_budget_exhaustion()) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              no_forest_probe_budget_exhausted,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              budget_exhausted);
      return outcome;
    }
    if (successor_probe.certified_fail_closed_contradiction()) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              contradiction_forest_probe,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              contradiction);
      return outcome;
    }

    std::size_t target_index = 0U;
    if (successor_memo.found) {
      target_index = successor_memo.node_index;
    } else {
      const auto inserted = insert_node(successor_key, successor_memo);
      if (!inserted.has_value()) {
        if (!result_.minted_) {
          fail(
              ExactDirectSparseStableFacetDescentClosureDecision::
                  no_node_budget_exhausted,
              ExactDirectSparseStableFacetDescentClosureDisposition::
                  budget_exhausted);
        }
        return outcome;
      }
      target_index = *inserted;
    }
    if (!assign_geometry(target_index, *successor_geometry)) {
      return outcome;
    }
    if (states_[target_index] == NodeState::visiting) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              contradiction_cycle_or_inconsistent_shared_target,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              contradiction);
      return outcome;
    }

    const bool successor_observed = successor_probe.certified_observed();
    if (!successor_observed && !successor_probe.certified_unobserved()) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              contradiction_forest_probe,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              contradiction);
      return outcome;
    }
    if (states_[target_index] == NodeState::closed) {
      const auto& target = result_.nodes_[target_index];
      const bool closed_positive =
          target.kind == ExactDirectSparseStableFacetDescentNodeKind::
                             stable_positive_terminal &&
          target.stable_positive_attachment ==
              std::optional<ExactDirectSparseStableFacetPositiveAttachment>{
                  ExactDirectSparseStableFacetPositiveAttachment{
                      successor_probe.bound_handle,
                      successor_probe.root_handle,
                      successor_probe.component_size}};
      if (successor_observed != closed_positive) {
        fail(
            ExactDirectSparseStableFacetDescentClosureDecision::
                contradiction_cycle_or_inconsistent_shared_target,
            ExactDirectSparseStableFacetDescentClosureDisposition::
                contradiction);
        return outcome;
      }
      ++result_.counters_.memoized_suffix_reuse_count;
    } else if (successor_observed) {
      mark_positive_terminal(
          target_index,
          successor_probe,
          ExactDirectSparseStableFacetDescentLocalDecision::not_certified,
          false);
    }

    ExactDirectSparseStableFacetDescentEdge edge;
    edge.edge_index = result_.edges_.size();
    edge.source_node_index = source_index;
    edge.target_node_index = target_index;
    edge.strict_step_witness = std::move(witness);
    edge.source_and_target_keys_match_nodes = true;
    edge.target_center_and_level_match_node = true;
    edge.strict_level_decrease_certified = true;
    edge.same_closed_batch_level_certified = true;
    result_.edges_.push_back(std::move(edge));
    auto& published_source_node = result_.nodes_[source_index];
    published_source_node.outgoing_edge_index = result_.edges_.size() - 1U;
    published_source_node.local_decision =
        successor_observed
            ? ExactDirectSparseStableFacetDescentLocalDecision::
                  complete_strict_successor_stable_positive_hit
            : ExactDirectSparseStableFacetDescentLocalDecision::
                  complete_strict_successor_unobserved;
    published_source_node.kind =
        ExactDirectSparseStableFacetDescentNodeKind::evaluated_source;
    result_.counters_.strict_edge_count = result_.edges_.size();

    outcome.completed = true;
    if (states_[target_index] != NodeState::closed) {
      outcome.has_next = true;
      outcome.next_node_index = target_index;
    }
    return outcome;
  }

  [[nodiscard]] bool close_path() {
    for (auto iterator = path_.rbegin(); iterator != path_.rend(); ++iterator) {
      const std::size_t node_index = *iterator;
      if (states_[node_index] == NodeState::closed) {
        continue;
      }
      auto& node = result_.nodes_[node_index];
      if (!node.outgoing_edge_index.has_value() ||
          *node.outgoing_edge_index >= result_.edges_.size()) {
        fail(
            ExactDirectSparseStableFacetDescentClosureDecision::
                contradiction_cycle_or_inconsistent_shared_target,
            ExactDirectSparseStableFacetDescentClosureDisposition::
                contradiction);
        return false;
      }
      const std::size_t target_index =
          result_.edges_[*node.outgoing_edge_index].target_node_index;
      if (target_index >= states_.size() ||
          states_[target_index] != NodeState::closed) {
        fail(
            ExactDirectSparseStableFacetDescentClosureDecision::
                contradiction_cycle_or_inconsistent_shared_target,
            ExactDirectSparseStableFacetDescentClosureDisposition::
                contradiction);
        return false;
      }
      const auto& target = result_.nodes_[target_index];
      node.terminal_node_index = target.terminal_node_index;
      node.closure_disposition = target.closure_disposition;
      node.terminal_pointer_certified = true;
      states_[node_index] = NodeState::closed;
    }
    return true;
  }

  [[nodiscard]] bool process_seed(
      const ExactDirectSparseStableFacetDescentClosureSeed& seed) {
    const MemoProbe root_memo = probe_memo(seed.source_facet_key);
    if (root_memo.failed) {
      return false;
    }
    if (!root_memo.found) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              contradiction_cycle_or_inconsistent_shared_target,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              contradiction);
      return false;
    }
    const std::size_t root_index = root_memo.node_index;
    const bool duplicate_seed_reference = seen_as_seed_[root_index];
    const bool reused =
        duplicate_seed_reference ||
        states_[root_index] != NodeState::unvisited;
    if (reused) {
      ++result_.counters_.memoized_seed_reuse_count;
    }
    if (duplicate_seed_reference) {
      ++result_.counters_.duplicate_seed_key_reference_count;
    } else {
      seen_as_seed_[root_index] = true;
      ++result_.counters_.distinct_seed_key_count;
    }

    if (!duplicate_seed_reference &&
        states_[root_index] == NodeState::closed) {
      ++result_.counters_.memoized_suffix_reuse_count;
    }

    path_.clear();
    std::size_t cursor = root_index;
    while (states_[cursor] != NodeState::closed) {
      if (states_[cursor] == NodeState::visiting) {
        fail(
            ExactDirectSparseStableFacetDescentClosureDecision::
                contradiction_cycle_or_inconsistent_shared_target,
            ExactDirectSparseStableFacetDescentClosureDisposition::
                contradiction);
        return false;
      }
      states_[cursor] = NodeState::visiting;
      path_.push_back(cursor);
      const Evaluation evaluation = evaluate_node(cursor);
      if (!evaluation.completed) {
        return false;
      }
      if (!evaluation.has_next) {
        break;
      }
      cursor = evaluation.next_node_index;
    }
    if (!close_path() || states_[root_index] != NodeState::closed) {
      return false;
    }

    const auto& root = result_.nodes_[root_index];
    result_.seed_projections_.push_back(
        ExactDirectSparseStableFacetDescentSeedProjection{
            seed.seed_index,
            seed.source_facet_key,
            root_index,
            root.terminal_node_index,
            root.closure_disposition,
            reused});
    ++result_.counters_.processed_seed_reference_count;
    return true;
  }

  void finalize_success() {
    if (forest_.current_stamp() != result_.forest_stamp_) {
      fail(
          ExactDirectSparseStableFacetDescentClosureDecision::
              contradiction_forest_stamp_changed,
          ExactDirectSparseStableFacetDescentClosureDisposition::
              contradiction);
      return;
    }
    result_.counters_.interned_node_count = result_.nodes_.size();
    result_.counters_.strict_edge_count = result_.edges_.size();
    result_.strict_functional_graph_certified_ = true;
    result_.exact_level_acyclicity_certified_ = true;
    result_.edge_node_terminal_identity_certified_ = true;
    result_.all_seed_references_processed_ = true;
    result_.no_partial_graph_published_ = true;
    result_.common_forest_stamp_certified_ = true;
    if (ordered_seeds_.empty()) {
      result_.disposition_ =
          ExactDirectSparseStableFacetDescentClosureDisposition::
              relative_positive;
      result_.decision_ =
          ExactDirectSparseStableFacetDescentClosureDecision::
              complete_empty_seed_set;
    } else if (result_.counters_.unresolved_terminal_count == 0U) {
      result_.disposition_ =
          ExactDirectSparseStableFacetDescentClosureDisposition::
              relative_positive;
      result_.decision_ =
          ExactDirectSparseStableFacetDescentClosureDecision::
              complete_all_seeds_relative_positive;
    } else {
      result_.disposition_ =
          ExactDirectSparseStableFacetDescentClosureDisposition::unresolved;
      result_.decision_ =
          ExactDirectSparseStableFacetDescentClosureDecision::
              complete_all_seeds_with_unresolved_terminals;
    }
    result_.minted_ = true;
  }

  void clear_published_graph() {
    result_.nodes_.clear();
    result_.edges_.clear();
    result_.seed_projections_.clear();
    std::vector<ExactDirectSparseStableFacetDescentNode>{}.swap(
        result_.nodes_);
    std::vector<ExactDirectSparseStableFacetDescentEdge>{}.swap(
        result_.edges_);
    std::vector<ExactDirectSparseStableFacetDescentSeedProjection>{}.swap(
        result_.seed_projections_);
    result_.counters_.processed_seed_reference_count = 0U;
    result_.counters_.distinct_seed_key_count = 0U;
    result_.counters_.duplicate_seed_key_reference_count = 0U;
    result_.counters_.interned_node_count = 0U;
    result_.counters_.evaluated_step_source_count = 0U;
    result_.counters_.strict_edge_count = 0U;
    result_.counters_.terminal_node_count = 0U;
    result_.counters_.relative_positive_terminal_count = 0U;
    result_.counters_.unresolved_terminal_count = 0U;
    result_.counters_.memoized_seed_reuse_count = 0U;
    result_.counters_.memoized_suffix_reuse_count = 0U;
    result_.strict_functional_graph_certified_ = false;
    result_.exact_level_acyclicity_certified_ = false;
    result_.edge_node_terminal_identity_certified_ = false;
    result_.all_seed_references_processed_ = false;
    result_.no_partial_graph_published_ = true;
  }

  void fail(
      ExactDirectSparseStableFacetDescentClosureDecision decision,
      ExactDirectSparseStableFacetDescentClosureDisposition disposition) {
    clear_published_graph();
    result_.decision_ = decision;
    result_.disposition_ = disposition;
    result_.common_forest_stamp_certified_ =
        forest_.current_stamp() == result_.forest_stamp_;
    if (!result_.common_forest_stamp_certified_) {
      result_.decision_ =
          ExactDirectSparseStableFacetDescentClosureDecision::
              contradiction_forest_stamp_changed;
      result_.disposition_ =
          ExactDirectSparseStableFacetDescentClosureDisposition::
              contradiction;
    }
    result_.minted_ = true;
  }

  ExactDirectSparseStableFacetDescentClosureResult& result_;
  const spatial::MortonLbvhIndex& index_;
  const spatial::CanonicalPointCloud& cloud_;
  std::span<const ExactDirectSparseStableFacetDescentClosureSeed> input_seeds_;
  const ExactDirectSparseStableFacetForest& forest_;
  std::vector<ExactDirectSparseStableFacetDescentClosureSeed> ordered_seeds_;
  std::vector<std::size_t> memo_slots_;
  std::vector<TransientGeometryCacheSlot>
      transient_geometry_cache_slots_;
  std::vector<TransientGeometryCacheEntry>
      transient_geometry_cache_entries_;
  std::vector<NodeState> states_;
  std::vector<bool> seen_as_seed_;
  std::vector<std::size_t> path_;
  std::size_t transient_geometry_cache_maximum_entry_count_{};
  std::size_t per_probe_worst_work_{};
  bool per_probe_worst_work_valid_{false};
};

ExactDirectSparseStableFacetDescentClosureResult
build_exact_direct_sparse_stable_facet_descent_closure(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const ExactDirectSparseStableFacetDescentClosureSeed> seeds,
    const exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseStableFacetForest& forest,
    const ExactDirectSparseStableFacetDescentClosureBudget& budget,
    const ExactDirectSparseStableFacetDescentClosureConfig& config,
    spatial::LbvhTraversalOrder traversal_order) {
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "a stable-facet descent closure requires a matching LBVH authority");
  }
  if (!forest.certified_structure_only_forest()) {
    throw std::invalid_argument(
        "a stable-facet descent closure requires a certified stable forest");
  }
  if (!valid_traversal_order(traversal_order)) {
    throw std::invalid_argument(
        "a stable-facet descent closure requires a valid traversal order");
  }

  ExactDirectSparseStableFacetDescentClosureResult result;
  result.config_ = config;
  result.requested_budget_ = budget;
  result.traversal_order_ = traversal_order;
  result.closed_batch_squared_level_ = closed_batch_squared_level;
  result.forest_stamp_ = forest.current_stamp();
  result.trusted_authorities_certified_ = true;
  result.every_memo_candidate_compared_by_full_key_ = true;
  result.no_partial_graph_published_ = true;
  result.no_top_k_partition_or_shell_persisted_ = true;
  result.forest_state_mutated_ = false;
  result.global_closed_ball_materialized_ = false;
  result.forbidden_global_structure_materialized_ = false;
  result.source_exactness_claimed_ = false;
  result.hierarchy_attachment_published_ = false;
  result.public_status_claimed_ = false;
  ExactDirectSparseStableFacetDescentClosureBuilder builder{
      result, index, cloud, seeds, forest};
  builder.run();
  return result;
}

ExactDirectSparseStableFacetDescentClosureVerification
verify_exact_direct_sparse_stable_facet_descent_closure(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::span<const ExactDirectSparseStableFacetDescentClosureSeed> seeds,
    const exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparseStableFacetForest& forest,
    const ExactDirectSparseStableFacetDescentClosureBudget& budget,
    const ExactDirectSparseStableFacetDescentClosureConfig& config,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseStableFacetDescentClosureResult& observed) {
  ExactDirectSparseStableFacetDescentClosureVerification verification;
  if (!index.validated_for(cloud)) {
    throw std::invalid_argument(
        "a stable-facet descent closure requires a matching LBVH authority");
  }
  if (!forest.certified_structure_only_forest()) {
    throw std::invalid_argument(
        "a stable-facet descent closure requires a certified stable forest");
  }
  if (!valid_traversal_order(traversal_order)) {
    throw std::invalid_argument(
        "a stable-facet descent closure requires a valid traversal order");
  }
  verification.trusted_inputs_certified = true;
  const ExactDirectSparseStableFacetForestStamp current_stamp =
      forest.current_stamp();
  verification.observed_stamp_matches_forest =
      observed.forest_stamp() == current_stamp;
  if (!verification.observed_stamp_matches_forest) {
    return verification;
  }
  const auto fresh = build_exact_direct_sparse_stable_facet_descent_closure(
      index,
      cloud,
      seeds,
      closed_batch_squared_level,
      forest,
      budget,
      config,
      traversal_order);
  verification.fresh_replay_completed = true;
  verification.fresh_replay_equal_to_observed = fresh == observed;
  verification.result_certified =
      verification.observed_stamp_matches_forest &&
      verification.fresh_replay_equal_to_observed &&
      fresh.certified_for(forest) && observed.certified_for(forest);
  return verification;
}

}  // namespace morsehgp3d::hierarchy
