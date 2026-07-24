#include "morsehgp3d/hierarchy/direct_morse_forest_reducer.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <new>
#include <numeric>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

[[nodiscard]] bool try_add(
    std::size_t left,
    std::size_t right,
    std::size_t& sum) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  sum = left + right;
  return true;
}

[[nodiscard]] bool try_multiply(
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

[[nodiscard]] bool append_count_within(
    std::size_t current,
    std::size_t increment,
    std::size_t maximum) noexcept {
  std::size_t sum = 0U;
  return try_add(current, increment, sum) && sum <= maximum;
}

[[nodiscard]] ExactDirectSparsePositiveFacetProbeBudget
terminal_probe_budget(
    const ExactDirectSparseFacetDescentClosureBudget& budget) noexcept {
  const auto& source = budget.step_budget.source_locator_probe;
  const auto& successor = budget.step_budget.successor_locator_probe;
  return {
      std::max(
          source.maximum_slot_visit_count,
          successor.maximum_slot_visit_count),
      std::max(
          source.maximum_component_parent_hop_count,
          successor.maximum_component_parent_hop_count),
  };
}

[[nodiscard]] bool key_less(
    const ExactDirectSparseFacetKey& left,
    const ExactDirectSparseFacetKey& right) noexcept {
  const std::size_t common = std::min(left.point_count, right.point_count);
  for (std::size_t index = 0U; index < common; ++index) {
    if (left.point_ids[index] != right.point_ids[index]) {
      return left.point_ids[index] < right.point_ids[index];
    }
  }
  return left.point_count < right.point_count;
}

[[nodiscard]] bool valid_key(
    const ExactDirectSparseFacetKey& key,
    std::size_t point_count,
    std::size_t order) noexcept {
  if (order == 0U || order > point_count ||
      order > direct_sparse_positive_facet_maximum_point_count ||
      key.point_count != order) {
    return false;
  }
  for (std::size_t index = 0U; index < order; ++index) {
    if (static_cast<std::size_t>(key.point_ids[index]) >= point_count ||
        (index != 0U &&
         key.point_ids[index - 1U] >= key.point_ids[index])) {
      return false;
    }
  }
  for (std::size_t index = order; index < key.point_ids.size(); ++index) {
    if (key.point_ids[index] != 0U) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] std::optional<std::uint64_t> replay_token(
    std::size_t index,
    std::uint64_t residue) noexcept {
  constexpr std::uint64_t modulus = 3U;
  if (residue == 0U || residue > modulus ||
      index >
          (std::numeric_limits<std::uint64_t>::max() - residue) /
              modulus) {
    return std::nullopt;
  }
  return static_cast<std::uint64_t>(index) * modulus + residue;
}

void require_traversal_order(spatial::LbvhTraversalOrder order) {
  switch (order) {
    case spatial::LbvhTraversalOrder::near_first:
    case spatial::LbvhTraversalOrder::far_first:
      return;
  }
  throw std::invalid_argument(
      "a direct Morse forest reducer traversal order is invalid");
}

[[nodiscard]] ExactDirectSparseFacetKey birth_key(
    const ExactDirectMorseEventProjection& projection,
    const ExactDirectSupportTerminalFacade& facade,
    std::size_t point_count,
    std::size_t order,
    const exact::ExactLevel& level) {
  if (projection.event_projection_index >=
          facade.events.size() + point_count ||
      projection.birth_order != std::optional<std::size_t>{order} ||
      projection.squared_level != level ||
      projection.closed_rank != order) {
    throw std::logic_error(
        "a reducer birth projection disagrees with its source batch");
  }

  ExactDirectSparseFacetKey key;
  key.point_count = order;
  if (projection.source ==
      ExactDirectMorseEventSource::canonical_singleton) {
    if (order != 1U || projection.source_index >= point_count ||
        projection.support_size != 1U ||
        projection.support_ids[0U] !=
            static_cast<PointId>(projection.source_index)) {
      throw std::logic_error(
          "a reducer singleton birth has an invalid identity");
    }
    key.point_ids[0U] = static_cast<PointId>(projection.source_index);
    return key;
  }

  if (projection.source !=
          ExactDirectMorseEventSource::direct_support_terminal_event ||
      projection.source_index >= facade.events.size()) {
    throw std::logic_error(
        "a reducer direct birth has no Phase-9 source event");
  }
  const ExactDirectSupportEvent& event =
      facade.events[projection.source_index];
  const std::size_t support_size =
      static_cast<std::size_t>(event.support_size);
  if (event.event_index != projection.source_index ||
      event.birth_order != std::optional<std::size_t>{order} ||
      event.squared_level != level || event.closed_rank != order ||
      event.support_size != projection.support_size ||
      event.support_ids != projection.support_ids ||
      support_size + event.interior_ids.size() != order) {
    throw std::logic_error(
        "a reducer direct birth changed Phase-9 identity");
  }
  std::size_t write = 0U;
  for (const PointId point_id : event.interior_ids) {
    key.point_ids[write++] = point_id;
  }
  for (std::size_t index = 0U; index < support_size; ++index) {
    key.point_ids[write++] = event.support_ids[index];
  }
  std::sort(
      key.point_ids.begin(),
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(write));
  if (write != order || !valid_key(key, point_count, order)) {
    throw std::logic_error(
        "a reducer birth did not reconstruct one canonical key");
  }
  return key;
}

[[nodiscard]] ExactDirectSparseFacetKey arm_key(
    const ExactDirectSaddleArmFacet& facet,
    std::size_t point_count,
    std::size_t order) {
  ExactDirectSparseFacetKey key;
  key.point_ids = facet.point_ids;
  key.point_count = facet.point_count;
  if (!valid_key(key, point_count, order)) {
    throw std::logic_error(
        "a reducer strict arm is not a canonical order-k key");
  }
  return key;
}

void initialize_scope(ExactDirectMorseForestJournalResult& result) noexcept {
  result.source_phase9_facade_freshly_replayed = false;
  result.conditional_on_caller_fresh_phase9_facade_replay = true;
  result.external_locator_authority_replayed = true;
  result.conditional_on_caller_external_locator_authority_replay = false;
  result.global_morse_obligation_replayed = false;
  result.conditional_on_separate_global_morse_obligation = true;
  result.equal_or_interior_facets_consumed = false;
  result.gateway_10_6_or_later_consumed = false;
  result.closure_graph_persisted = false;
  result.gamma_cells_or_global_cofaces_materialized = false;
  result.higher_order_delaunay_materialized = false;
  result.forbidden_global_structure_materialized = false;
  result.public_status_claimed = false;
  result.conditional_exact_h0_only = true;
  result.scope = ExactDirectMorseForestScope::
      all_orders_direct_minimum_carriers_strict_arms_recursive_positive_terminals_and_atomic_full_component_saddle_quotients_with_reduced_qr_only;
}

struct TemporaryArm {
  std::size_t source_seed_index{};
  ExactDirectSparseFacetKey key{};
  ExactDirectSparseComponentHandle carrier_handle{};
  std::optional<ExactDirectMorseForestNodeId> prior_reduced_root_node_id;
};

struct TemporarySaddle {
  std::size_t source_family_index{};
  std::vector<TemporaryArm> arms;
};

struct ResolvedState {
  ExactDirectSparseFacetKey key{};
  ExactDirectSparseComponentHandle carrier_handle{};
  std::optional<ExactDirectMorseForestNodeId> prior_reduced_root_node_id;
};

struct GroupPlan {
  std::vector<std::size_t> saddle_indices;
  std::vector<ExactDirectSparseComponentHandle> carrier_handles;
  std::vector<ExactDirectMorseForestNodeId> prior_reduced_root_node_ids;
  std::optional<ExactDirectMorseForestNodeId> created_node_id;
  ExactDirectMorseForestNodeId resulting_root_node_id{};
  ExactDirectMorseForestAtomicGroupKind kind{
      ExactDirectMorseForestAtomicGroupKind::reduced_birth};
  std::size_t atomic_group_index{};
};

class CarrierDsu {
 public:
  CarrierDsu(std::size_t handle_count, std::size_t maximum_order)
      : parent_(handle_count),
        rank_(handle_count, 0U),
        canonical_(handle_count),
        order_(handle_count, 0U),
        active_(handle_count, false),
        reduced_root_(handle_count),
        carrier_count_by_order_(maximum_order + 1U, 0U),
        reduced_count_by_order_(maximum_order + 1U, 0U) {
    std::iota(parent_.begin(), parent_.end(), std::size_t{0U});
    std::iota(canonical_.begin(), canonical_.end(), std::size_t{0U});
  }

  [[nodiscard]] std::size_t root(std::size_t handle) const noexcept {
    while (parent_[handle] != handle) {
      handle = parent_[handle];
    }
    return handle;
  }

  [[nodiscard]] bool active(std::size_t handle) const noexcept {
    return handle < parent_.size() && active_[root(handle)];
  }

  [[nodiscard]] std::size_t canonical(
      std::size_t handle) const noexcept {
    return canonical_[root(handle)];
  }

  [[nodiscard]] std::size_t order(std::size_t handle) const noexcept {
    return order_[root(handle)];
  }

  [[nodiscard]] std::optional<ExactDirectMorseForestNodeId> reduced_root(
      std::size_t handle) const noexcept {
    return reduced_root_[root(handle)];
  }

  [[nodiscard]] std::size_t handle_count() const noexcept {
    return parent_.size();
  }

  [[nodiscard]] std::size_t carrier_count(std::size_t order) const noexcept {
    return carrier_count_by_order_[order];
  }

  [[nodiscard]] std::size_t reduced_count(std::size_t order) const noexcept {
    return reduced_count_by_order_[order];
  }

  [[nodiscard]] bool active_root(std::size_t handle) const noexcept {
    return handle < parent_.size() && active_[handle] &&
           parent_[handle] == handle;
  }

  void commit_group(
      std::span<const ExactDirectSparseComponentHandle> carriers,
      std::size_t group_order,
      std::size_t prior_reduced_count,
      ExactDirectMorseForestNodeId resulting_root) noexcept {
    std::size_t physical_root = root_compress(carriers.front());
    for (std::size_t index = 1U; index < carriers.size(); ++index) {
      physical_root = unite(physical_root, carriers[index]);
    }
    reduced_root_[physical_root] = resulting_root;
    carrier_count_by_order_[group_order] -= carriers.size() - 1U;
    reduced_count_by_order_[group_order] -= prior_reduced_count;
    ++reduced_count_by_order_[group_order];
  }

  void activate(
      std::size_t handle,
      std::size_t birth_order,
      std::optional<ExactDirectMorseForestNodeId> root_node) noexcept {
    active_[handle] = true;
    order_[handle] = birth_order;
    reduced_root_[handle] = root_node;
    ++carrier_count_by_order_[birth_order];
    if (root_node.has_value()) {
      ++reduced_count_by_order_[birth_order];
    }
  }

  void activate_initial_canonical_singletons(
      std::size_t singleton_count) noexcept {
    for (std::size_t handle = 0U; handle < singleton_count; ++handle) {
      activate(
          handle,
          1U,
          static_cast<ExactDirectMorseForestNodeId>(handle));
    }
  }

 private:
  [[nodiscard]] std::size_t root_compress(std::size_t handle) noexcept {
    const std::size_t result = root(handle);
    while (parent_[handle] != handle) {
      const std::size_t next = parent_[handle];
      parent_[handle] = result;
      handle = next;
    }
    return result;
  }

  [[nodiscard]] std::size_t unite(
      std::size_t left,
      std::size_t right) noexcept {
    left = root_compress(left);
    right = root_compress(right);
    if (left == right) {
      return left;
    }
    if (rank_[left] < rank_[right]) {
      std::swap(left, right);
    }
    parent_[right] = left;
    if (rank_[left] == rank_[right]) {
      ++rank_[left];
    }
    canonical_[left] = std::min(canonical_[left], canonical_[right]);
    active_[left] = true;
    return left;
  }

  std::vector<std::size_t> parent_;
  std::vector<std::uint8_t> rank_;
  std::vector<std::size_t> canonical_;
  std::vector<std::size_t> order_;
  std::vector<bool> active_;
  std::vector<std::optional<ExactDirectMorseForestNodeId>> reduced_root_;
  std::vector<std::size_t> carrier_count_by_order_;
  std::vector<std::size_t> reduced_count_by_order_;
};

struct PayloadSizes {
  std::size_t birth_records{};
  std::size_t arm_root_bindings{};
  std::size_t saddle_records{};
  std::size_t atomic_groups{};
  std::size_t child_node_ids{};
  std::size_t batches{};
  std::size_t nodes{};
};

[[nodiscard]] PayloadSizes payload_sizes(
    const ExactDirectMorseForestJournalResult& result) noexcept {
  return {
      result.birth_records.size(),
      result.arm_root_bindings.size(),
      result.saddle_records.size(),
      result.atomic_groups.size(),
      result.child_node_ids.size(),
      result.batches.size(),
      result.nodes.size(),
  };
}

class PayloadRollback {
 public:
  PayloadRollback(
      ExactDirectMorseForestJournalResult& result,
      PayloadSizes sizes) noexcept
      : result_(&result), sizes_(sizes) {}

  PayloadRollback(const PayloadRollback&) = delete;
  PayloadRollback& operator=(const PayloadRollback&) = delete;

  ~PayloadRollback() {
    if (armed_) {
      result_->birth_records.resize(sizes_.birth_records);
      result_->arm_root_bindings.resize(sizes_.arm_root_bindings);
      result_->saddle_records.resize(sizes_.saddle_records);
      result_->atomic_groups.resize(sizes_.atomic_groups);
      result_->child_node_ids.resize(sizes_.child_node_ids);
      result_->batches.resize(sizes_.batches);
      result_->nodes.resize(sizes_.nodes);
    }
  }

  void release() noexcept { armed_ = false; }

 private:
  ExactDirectMorseForestJournalResult* result_{};
  PayloadSizes sizes_{};
  bool armed_{true};
};

template <typename Value>
void append_moved(std::vector<Value>& destination, std::vector<Value>& source) {
  destination.insert(
      destination.end(),
      std::make_move_iterator(source.begin()),
      std::make_move_iterator(source.end()));
}

}  // namespace

ExactDirectMorseForestReducerBatch
project_exact_direct_morse_forest_reducer_batch(
    const ExactDirectSparseFacetDescentBatchExecutionResult& source) {
  if (!source.complete_architecture_execution()) {
    throw std::invalid_argument(
        "a forest reducer projection requires one complete 14D delta");
  }
  ExactDirectMorseForestReducerBatch projected;
  projected.source_batch_index = source.source_batch_index;
  projected.source_chunk_index = source.source_chunk_index;
  projected.source_family_begin_index = source.source_family_begin_index;
  projected.source_family_end_index = source.source_family_end_index;
  projected.source_arm_seed_begin_index =
      source.source_arm_seed_begin_index;
  projected.source_arm_seed_end_index = source.source_arm_seed_end_index;
  projected.order = source.source_facet_cardinality;
  projected.squared_level = source.closed_batch_squared_level;
  projected.traversal_order = source.traversal_order;
  projected.locator_query_witness = source.locator_query_witness;
  projected.strict_pre_batch_locator_stamp =
      source.locator_snapshot_stamp;
  projected.requested_closure_budget = source.requested_closure_budget;
  projected.transient_closure_node_count =
      source.closure_summary.transient_node_count;
  projected.transient_closure_step_call_count =
      source.closure_summary.counters.evaluated_step_source_count;
  projected.shared_closure_build_count =
      source.counters.shared_closure_build_count;
  projected.resolved_keys = source.resolved_keys;
  projected.arm_joins = source.arm_joins;
  return projected;
}

bool ExactDirectMorseForestReducerFoldResult::certified_committed_batch()
    const noexcept {
  const bool staging_audit_consistent =
      canonical_singleton_bulk_count == 0U ||
      (staged_birth_record_count == 0U &&
       staged_birth_node_count == 0U &&
       staged_locator_binding_count == 0U);
  return schema_version == direct_morse_forest_reducer_schema_version &&
         staging_audit_consistent &&
         complete_batch_staged_before_mutation &&
         full_equal_level_quotient_resolved_before_mutation &&
         locator_committed_before_scientific_state &&
         scientific_state_committed && reducer_state_mutated &&
         !source_chunk_index_has_scientific_authority &&
         !forbidden_global_structure_materialized &&
         !public_status_claimed &&
         post_fold_locator_stamp.committed_batch_count ==
             pre_fold_locator_stamp.committed_batch_count + 1U &&
         decision == ExactDirectMorseForestReducerFoldDecision::
                         complete_reducer_batch_commit;
}

bool ExactDirectMorseForestReducerFoldResult::certified_atomic_rejection()
    const noexcept {
  return schema_version == direct_morse_forest_reducer_schema_version &&
         decision != ExactDirectMorseForestReducerFoldDecision::not_folded &&
         decision != ExactDirectMorseForestReducerFoldDecision::
                         complete_reducer_batch_commit &&
         !locator_committed_before_scientific_state &&
         !scientific_state_committed && !reducer_state_mutated &&
         !source_chunk_index_has_scientific_authority &&
         !forbidden_global_structure_materialized &&
         !public_status_claimed &&
         pre_fold_locator_stamp == post_fold_locator_stamp;
}

bool ExactDirectMorseForestLiveCommitResult::certified_live_commit()
    const noexcept {
  return schema_version == direct_morse_forest_live_commit_schema_version &&
         scientific_delta.has_value() &&
         source_batch_index == scientific_delta->source_batch_index &&
         pre_commit_executor_batch_index == source_batch_index &&
         post_commit_executor_batch_index == successor_batch_index &&
         ticket_was_valid_and_unconsumed &&
         shared_session_seal_matches &&
         source_epoch_and_full_cursor_match &&
         exact_scientific_delta_provenance_minted &&
         reducer_and_executor_share_locator_instance &&
         reducer_and_executor_batch_cursors_match &&
         executor_commit_capacity_preflighted &&
         all_fallible_scientific_work_precedes_irreversible_mutation &&
         reducer_fold_attempted &&
         reducer_fold.certified_committed_batch() &&
         reducer_fold.source_batch_index == source_batch_index &&
         reducer_fold.pre_fold_locator_stamp ==
             pre_commit_locator_stamp &&
         reducer_fold.post_fold_locator_stamp ==
             post_commit_locator_stamp &&
         reducer_committed_before_executor_cursor &&
         no_fallible_operation_after_reducer_commit &&
         executor_cursor_advanced &&
         scientific_delta_moved_to_result && ticket_consumed &&
         !executor_cursor_unchanged_on_rejection &&
         !locator_unchanged_on_rejection &&
         !independent_geometry_replay_performed &&
         !forbidden_global_structure_materialized &&
         !public_status_claimed &&
         decision == ExactDirectMorseForestLiveCommitDecision::
                         complete_live_reducer_then_cursor_commit;
}

bool ExactDirectMorseForestLiveCommitResult::certified_atomic_rejection()
    const noexcept {
  const bool fold_is_atomic =
      reducer_fold_attempted
          ? reducer_fold.certified_atomic_rejection()
          : reducer_fold.decision ==
                ExactDirectMorseForestReducerFoldDecision::not_folded;
  return schema_version == direct_morse_forest_live_commit_schema_version &&
         decision !=
             ExactDirectMorseForestLiveCommitDecision::not_committed &&
         decision != ExactDirectMorseForestLiveCommitDecision::
                         complete_live_reducer_then_cursor_commit &&
         fold_is_atomic &&
         !reducer_committed_before_executor_cursor &&
         !executor_cursor_advanced &&
         !scientific_delta_moved_to_result &&
         !scientific_delta.has_value() && ticket_consumed &&
         executor_cursor_unchanged_on_rejection &&
         locator_unchanged_on_rejection &&
         pre_commit_executor_batch_index ==
             post_commit_executor_batch_index &&
         pre_commit_locator_stamp == post_commit_locator_stamp &&
         !independent_geometry_replay_performed &&
         !forbidden_global_structure_materialized &&
         !public_status_claimed;
}

class ExactDirectMorseForestReducer::Impl {
 public:
  Impl(
      const spatial::CanonicalPointCloud& cloud,
      const ExactDirectSupportTerminalFacade& source_facade,
      const ExactDirectMorseEventJournalResult& source_journal,
      const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
      const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
      const ExactDirectMorseForestBudget& budget,
      const ExactDirectMorseForestConfig& config,
      spatial::LbvhTraversalOrder traversal_order)
      : cloud_(&cloud),
        source_facade_(&source_facade),
        source_journal_(&source_journal),
        source_seed_journal_(&source_seed_journal),
        budget_(budget),
        config_(config),
        traversal_order_(traversal_order),
        locator_(build_locator(
            cloud,
            source_facade,
            source_journal,
            trusted_seed_budget,
            source_seed_journal,
            budget,
            config,
            traversal_order,
            birth_count_)),
        components_(
            birth_count_,
            source_journal.effective_maximum_order) {
    result_.requested_budget = budget_;
    result_.config = config_;
    result_.traversal_order = traversal_order_;
    result_.point_count = cloud_->size();
    result_.effective_maximum_order =
        source_journal_->effective_maximum_order;
    initialize_scope(result_);
    result_.source_event_journal_freshly_replayed = true;
    result_.source_strict_arm_journal_freshly_replayed = true;
    result_.budget_preflight_certified = true;

    const std::size_t family_count = source_seed_journal_->families.size();
    const std::size_t arm_count = source_seed_journal_->arm_seeds.size();
    const std::size_t batch_count = source_journal_->batches.size();
    std::size_t node_capacity = 0U;
    if (!try_add(birth_count_, family_count, node_capacity)) {
      throw std::length_error("a reducer node capacity overflowed");
    }
    result_.birth_records.reserve(birth_count_);
    result_.arm_root_bindings.reserve(arm_count);
    result_.saddle_records.reserve(family_count);
    result_.atomic_groups.reserve(
        std::min(family_count, budget_.maximum_atomic_group_count));
    result_.child_node_ids.reserve(
        std::min(arm_count, budget_.maximum_child_reference_count));
    result_.batches.reserve(batch_count);
    result_.nodes.reserve(node_capacity);
    result_.final_roots.reserve(std::min(
        result_.effective_maximum_order,
        budget_.maximum_final_root_count));
  }

  [[nodiscard]] ExactDirectMorseForestReducerFoldResult fold(
      const ExactDirectMorseForestReducerBatch& batch) {
    ExactDirectMorseForestReducerFoldResult folded;
    folded.source_batch_index = batch.source_batch_index;
    folded.pre_fold_locator_stamp = locator_.snapshot_stamp();
    folded.post_fold_locator_stamp = folded.pre_fold_locator_stamp;

    if (finished_) {
      folded.decision =
          ExactDirectMorseForestReducerFoldDecision::no_reducer_finished;
      return folded;
    }
    if (batch.source_batch_index != next_batch_index_) {
      folded.decision = ExactDirectMorseForestReducerFoldDecision::
          no_reducer_batch_out_of_order;
      return folded;
    }

    try {
      return fold_checked(batch, std::move(folded));
    } catch (const std::length_error&) {
      folded.post_fold_locator_stamp = locator_.snapshot_stamp();
      folded.decision = ExactDirectMorseForestReducerFoldDecision::
          no_reducer_budget_exhausted;
      return folded;
    } catch (const std::bad_alloc&) {
      folded.post_fold_locator_stamp = locator_.snapshot_stamp();
      folded.decision = ExactDirectMorseForestReducerFoldDecision::
          no_reducer_operational_allocation_failed;
      return folded;
    } catch (const std::logic_error&) {
      folded.post_fold_locator_stamp = locator_.snapshot_stamp();
      folded.decision = ExactDirectMorseForestReducerFoldDecision::
          no_reducer_batch_inconsistent;
      return folded;
    }
  }

  [[nodiscard]] const ExactDirectSparsePositiveFacetLocator& locator()
      const noexcept {
    return locator_;
  }

  [[nodiscard]] std::size_t next_batch_index() const noexcept {
    return next_batch_index_;
  }

  [[nodiscard]] bool complete() const noexcept {
    return !finished_ &&
           next_batch_index_ == source_journal_->batches.size();
  }

  [[nodiscard]] ExactDirectMorseForestJournalResult finish() {
    if (finished_ || !complete() ||
        next_family_index_ != source_seed_journal_->families.size() ||
        next_arm_seed_index_ != source_seed_journal_->arm_seeds.size() ||
        result_.birth_records.size() != birth_count_ ||
        result_.saddle_records.size() !=
            source_seed_journal_->families.size() ||
        result_.arm_root_bindings.size() !=
            source_seed_journal_->arm_seeds.size()) {
      throw std::logic_error(
          "a direct Morse forest reducer cannot finish an incomplete stream");
    }

    std::vector<ExactDirectMorseForestFinalRoot> pending_final_roots;
    pending_final_roots.reserve(std::min(
        result_.effective_maximum_order,
        budget_.maximum_final_root_count));
    for (std::size_t handle = 0U;
         handle < components_.handle_count();
         ++handle) {
      if (!components_.active_root(handle)) {
        continue;
      }
      const auto reduced_root = components_.reduced_root(handle);
      if (!reduced_root.has_value()) {
        continue;
      }
      if (pending_final_roots.size() >=
          budget_.maximum_final_root_count) {
        throw std::logic_error(
            "a reducer final-root budget is exhausted");
      }
      pending_final_roots.push_back(
          {pending_final_roots.size(),
           components_.order(handle),
           components_.canonical(handle),
           *reduced_root});
    }
    std::sort(
        pending_final_roots.begin(),
        pending_final_roots.end(),
        [](const ExactDirectMorseForestFinalRoot& left,
           const ExactDirectMorseForestFinalRoot& right) {
          return left.order < right.order ||
                 (left.order == right.order &&
                  left.root_node_id < right.root_node_id);
        });
    for (std::size_t index = 0U;
         index < pending_final_roots.size();
         ++index) {
      pending_final_roots[index].final_root_index = index;
    }

    std::vector<std::size_t> root_count_by_order(
        result_.effective_maximum_order + 1U, 0U);
    for (const auto& root : pending_final_roots) {
      if (root.order == 0U ||
          root.order > result_.effective_maximum_order) {
        throw std::logic_error("a reducer final root has an invalid order");
      }
      ++root_count_by_order[root.order];
    }
    for (std::size_t order = 1U;
         order <= result_.effective_maximum_order;
         ++order) {
      const bool expected = order == 1U || order < result_.point_count;
      if (root_count_by_order[order] != (expected ? 1U : 0U)) {
        throw std::logic_error(
            "a reducer final-root partition is incomplete");
      }
    }

    std::size_t logical = 0U;
    for (const auto& birth : result_.birth_records) {
      if (!try_add(logical, birth.facet_key.point_count, logical)) {
        throw std::length_error("a reducer output count overflowed");
      }
    }
    for (const auto& arm : result_.arm_root_bindings) {
      if (!try_add(logical, arm.strict_arm_key.point_count, logical)) {
        throw std::length_error("a reducer output count overflowed");
      }
    }
    for (const std::size_t increment : {
             result_.birth_records.size(),
             result_.arm_root_bindings.size(),
             result_.saddle_records.size(),
             result_.atomic_groups.size(),
             result_.child_node_ids.size(),
             result_.batches.size(),
             pending_final_roots.size(),
             result_.nodes.size()}) {
      if (!try_add(logical, increment, logical)) {
        throw std::length_error("a reducer output count overflowed");
      }
    }
    if (logical > budget_.maximum_logical_output_entry_count) {
      throw std::logic_error(
          "a reducer logical-output budget is exhausted");
    }
    result_.final_roots.swap(pending_final_roots);
    result_.logical_output_entry_count = logical;
    result_.final_locator_stamp = locator_.snapshot_stamp();
    result_.counters.birth_record_count = result_.birth_records.size();
    result_.counters.order_one_birth_node_count =
        static_cast<std::size_t>(std::count_if(
            result_.birth_records.begin(),
            result_.birth_records.end(),
            [](const ExactDirectMorseForestBirthRecord& birth) {
              return birth.order == 1U;
            }));
    result_.counters.latent_higher_order_birth_count =
        result_.birth_records.size() -
        result_.counters.order_one_birth_node_count;
    result_.counters.arm_root_binding_count =
        result_.arm_root_bindings.size();
    result_.counters.saddle_record_count = result_.saddle_records.size();
    result_.counters.atomic_group_count = result_.atomic_groups.size();
    result_.counters.child_reference_count =
        result_.child_node_ids.size();
    result_.counters.batch_record_count = result_.batches.size();
    result_.counters.node_count = result_.nodes.size();
    result_.counters.final_root_count = result_.final_roots.size();

    result_.every_birth_key_reconstructed_from_closed_direct_event = true;
    result_.deterministic_disjoint_birth_union_and_query_tokens = true;
    result_.batches_processed_in_strict_order_level_order = true;
    result_.cardinality_isolates_orders_in_shared_locator = true;
    result_.current_level_births_hidden_from_arm_descent = true;
    result_.higher_order_direct_births_are_latent_carriers = true;
    result_.one_10_5c_call_per_nonempty_strict_arm_batch = true;
    result_.every_strict_arm_has_positive_terminal = true;
    result_.all_catalogued_saddle_families_consumed_once = true;
    result_.carrier_to_optional_reduced_root_authority_maintained = true;
    result_.every_saddle_has_positive_carrier = true;
    result_.typed_root_or_latent_carrier_hyperedges_closed_transitively = true;
    result_.q_r_counts_only_distinct_prior_reduced_roots = true;
    result_.all_equal_level_saddles_quotiented_before_mutation = true;
    result_.saddle_records_grouped_with_source_family_provenance = true;
    result_.q_zero_groups_create_one_reduced_birth = true;
    result_.q_one_continuations_create_no_node = true;
    result_.q_at_least_two_groups_create_one_multifusion = true;
    result_.current_batch_birth_nodes_never_same_batch_children = true;
    result_.all_group_carriers_attached_to_resulting_root_atomically = true;
    result_.locator_commits_unions_before_current_birth_bindings = true;
    result_.final_roots_cover_exactly_nonterminal_reduced_orders = true;
    result_.no_partial_scientific_payload_published = true;
    result_.decision = ExactDirectMorseForestDecision::
        complete_conditional_exact_direct_morse_forest;
    if (!result_.certified_conditional_h0_candidate()) {
      throw std::logic_error(
          "a direct Morse forest reducer failed the forest contract");
    }
    finished_ = true;
    return std::move(result_);
  }

 private:
  static ExactDirectSparsePositiveFacetLocator build_locator(
      const spatial::CanonicalPointCloud& cloud,
      const ExactDirectSupportTerminalFacade& source_facade,
      const ExactDirectMorseEventJournalResult& source_journal,
      const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
      const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
      const ExactDirectMorseForestBudget& budget,
      const ExactDirectMorseForestConfig& config,
      spatial::LbvhTraversalOrder traversal_order,
      std::size_t& birth_count) {
    require_traversal_order(traversal_order);
    if (config.locator_config.external_authority_id == 0U) {
      throw std::invalid_argument(
          "a reducer requires one nonzero locator authority");
    }
    if (budget.closure_budget.maximum_seed_count >
            direct_sparse_facet_descent_closure_maximum_seed_count ||
        budget.closure_budget.maximum_node_count >
            direct_sparse_facet_descent_closure_maximum_node_count ||
        budget.closure_budget.maximum_step_call_count >
            direct_sparse_facet_descent_closure_maximum_step_call_count ||
        budget.closure_budget.maximum_memo_slot_count >
            direct_sparse_facet_descent_closure_maximum_memo_slot_count) {
      throw std::invalid_argument(
          "a reducer closure budget exceeds its confidence cap");
    }
    if (source_journal.role_records.size() >
            budget.maximum_source_role_scan_count ||
        source_journal.batches.size() >
            budget.maximum_source_batch_scan_count ||
        source_seed_journal.families.size() >
            budget.maximum_source_family_scan_count ||
        source_seed_journal.arm_seeds.size() >
            budget.maximum_source_arm_seed_scan_count ||
        source_journal.batches.size() >
            budget.maximum_batch_record_count ||
        source_seed_journal.families.size() >
            budget.maximum_saddle_record_count ||
        source_seed_journal.arm_seeds.size() >
            budget.maximum_arm_root_binding_count) {
      throw std::invalid_argument(
          "a reducer source exceeds its global scan budget");
    }
    const auto verification =
        verify_exact_direct_saddle_arm_seed_journal_streaming(
            cloud,
            source_facade,
            source_journal,
            trusted_seed_budget,
            source_seed_journal);
    if (!verification.result_certified ||
        !source_journal.certified_partial_refinement() ||
        !source_seed_journal.certified_partial_refinement() ||
        source_journal.point_count != cloud.size() ||
        source_seed_journal.point_count != cloud.size()) {
      throw std::invalid_argument(
          "a reducer requires freshly verified source journals");
    }

    birth_count = static_cast<std::size_t>(std::count_if(
        source_journal.role_records.begin(),
        source_journal.role_records.end(),
        [](const ExactDirectMorseH0RoleRecord& role) {
          return role.role == ExactDirectMorseH0Role::birth;
        }));
    const std::size_t required_final_root_count =
        source_journal.effective_maximum_order == 0U
            ? 0U
            : (cloud.size() <= 1U
                   ? 1U
                   : std::min(
                         source_journal.effective_maximum_order,
                         cloud.size() - 1U));
    std::size_t birth_plus_family = 0U;
    std::size_t safe_birth_entries = 0U;
    std::size_t safe_arm_entries = 0U;
    std::size_t safe_family_entries = 0U;
    std::size_t safe_output_bound = 0U;
    if (birth_count == 0U ||
        !try_add(
            birth_count,
            source_seed_journal.families.size(),
            birth_plus_family) ||
        !try_multiply(13U, birth_count, safe_birth_entries) ||
        !try_multiply(
            12U,
            source_seed_journal.arm_seeds.size(),
            safe_arm_entries) ||
        !try_multiply(
            3U,
            source_seed_journal.families.size(),
            safe_family_entries) ||
        !try_add(
            safe_birth_entries,
            safe_arm_entries,
            safe_output_bound) ||
        !try_add(
            safe_output_bound,
            safe_family_entries,
            safe_output_bound) ||
        !try_add(
            safe_output_bound,
            source_journal.batches.size(),
            safe_output_bound)) {
      throw std::length_error("a reducer preflight capacity overflowed");
    }
    if (birth_count > budget.maximum_birth_record_count ||
        birth_plus_family > budget.maximum_node_count ||
        required_final_root_count >
            budget.maximum_final_root_count ||
        safe_output_bound >
            budget.maximum_logical_output_entry_count) {
      throw std::invalid_argument(
          "a reducer source exceeds its forest-output budget");
    }
    auto locator = build_exact_direct_sparse_positive_facet_locator(
        birth_count, budget.locator_budget, config.locator_config);
    if (!locator.certified_positive_locator()) {
      throw std::invalid_argument(
          "a reducer locator cannot satisfy its capacity budget");
    }
    return locator;
  }

  [[nodiscard]] ExactDirectMorseForestReducerFoldResult reject(
      ExactDirectMorseForestReducerFoldResult folded,
      ExactDirectMorseForestReducerFoldDecision decision) const noexcept {
    folded.post_fold_locator_stamp = locator_.snapshot_stamp();
    folded.decision = decision;
    return folded;
  }

  [[nodiscard]] ExactDirectMorseForestReducerFoldResult fold_checked(
      const ExactDirectMorseForestReducerBatch& batch,
      ExactDirectMorseForestReducerFoldResult folded) {
    if (batch.source_batch_index >= source_journal_->batches.size()) {
      return reject(
          std::move(folded),
          ExactDirectMorseForestReducerFoldDecision::
              no_reducer_batch_inconsistent);
    }
    const ExactDirectMorseH0Batch& source_batch =
        source_journal_->batches[batch.source_batch_index];
    if (batch.source_batch_index >
        std::numeric_limits<std::uint64_t>::max() / 3U - 1U) {
      return reject(
          std::move(folded),
          ExactDirectMorseForestReducerFoldDecision::
              no_reducer_batch_inconsistent);
    }
    const std::uint64_t expected_query_token =
        (static_cast<std::uint64_t>(batch.source_batch_index) + 1U) * 3U;
    if (source_batch.batch_index != batch.source_batch_index ||
        batch.order != source_batch.order ||
        batch.squared_level != source_batch.squared_level ||
        batch.traversal_order != traversal_order_ ||
        batch.requested_closure_budget != budget_.closure_budget ||
        batch.strict_pre_batch_locator_stamp !=
            folded.pre_fold_locator_stamp ||
        batch.locator_query_witness.external_authority_id !=
            config_.locator_config.external_authority_id ||
        batch.locator_query_witness.replay_token !=
            expected_query_token ||
        batch.source_family_begin_index != next_family_index_ ||
        batch.source_arm_seed_begin_index != next_arm_seed_index_ ||
        batch.source_family_end_index <
            batch.source_family_begin_index ||
        batch.source_arm_seed_end_index <
            batch.source_arm_seed_begin_index ||
        batch.source_family_end_index >
            source_seed_journal_->families.size() ||
        batch.source_arm_seed_end_index >
            source_seed_journal_->arm_seeds.size() ||
        batch.source_family_end_index -
                batch.source_family_begin_index !=
            source_batch.saddle_role_count ||
        batch.resolved_keys.size() >
            budget_.maximum_batch_distinct_arm_count ||
        batch.resolved_keys.size() >
            budget_.closure_budget.maximum_seed_count ||
        batch.shared_closure_build_count !=
            (batch.resolved_keys.empty() ? 0U : 1U)) {
      return reject(
          std::move(folded),
          ExactDirectMorseForestReducerFoldDecision::
              no_reducer_batch_inconsistent);
    }
    if (!append_count_within(
            result_.counters.aggregate_closure_node_count,
            batch.transient_closure_node_count,
            budget_.maximum_aggregate_closure_node_count) ||
        !append_count_within(
            result_.counters.aggregate_closure_step_call_count,
            batch.transient_closure_step_call_count,
            budget_.maximum_aggregate_closure_step_call_count)) {
      return reject(
          std::move(folded),
          ExactDirectMorseForestReducerFoldDecision::
              no_reducer_budget_exhausted);
    }

    const std::size_t singleton_count = cloud_->size();
    const bool canonical_singleton_bulk_shape =
        batch.source_batch_index == 0U &&
        source_batch.order == 1U &&
        source_batch.squared_level == exact::ExactLevel{} &&
        source_batch.role_record_offset == 0U &&
        source_batch.role_record_count == singleton_count &&
        source_batch.birth_role_count == singleton_count &&
        source_batch.saddle_role_count == 0U &&
        batch.source_family_begin_index == 0U &&
        batch.source_family_end_index == 0U &&
        batch.source_arm_seed_begin_index == 0U &&
        batch.source_arm_seed_end_index == 0U &&
        batch.resolved_keys.empty() && batch.arm_joins.empty() &&
        batch.transient_closure_node_count == 0U &&
        batch.transient_closure_step_call_count == 0U &&
        batch.shared_closure_build_count == 0U;
    if (canonical_singleton_bulk_shape) {
      if (singleton_count == 0U ||
          source_journal_->role_records.size() < singleton_count ||
          source_journal_->event_projections.size() < singleton_count ||
          singleton_count - 1U >
              spatial::CanonicalPointCloud::max_point_id ||
          singleton_count - 1U >
              std::numeric_limits<
                  ExactDirectMorseForestNodeId>::max()) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }
      for (std::size_t index = 0U; index < singleton_count; ++index) {
        const ExactDirectMorseH0RoleRecord expected_role{
            index,
            0U,
            index,
            ExactDirectMorseH0Role::birth};
        ExactDirectMorseEventProjection expected_projection;
        expected_projection.event_projection_index = index;
        expected_projection.source =
            ExactDirectMorseEventSource::canonical_singleton;
        expected_projection.source_index = index;
        expected_projection.support_size = 1U;
        expected_projection.support_ids[0U] =
            static_cast<PointId>(index);
        expected_projection.squared_level = exact::ExactLevel{};
        expected_projection.closed_rank = 1U;
        expected_projection.birth_order = 1U;
        if (source_journal_->role_records[index] != expected_role ||
            source_journal_->event_projections[index] !=
                expected_projection) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_batch_inconsistent);
        }
      }

      const PayloadSizes before = payload_sizes(result_);
      if (before.birth_records != 0U ||
          before.arm_root_bindings != 0U ||
          before.saddle_records != 0U ||
          before.atomic_groups != 0U ||
          before.child_node_ids != 0U ||
          before.batches != 0U || before.nodes != 0U ||
          result_.birth_records.capacity() < singleton_count ||
          result_.nodes.capacity() < singleton_count ||
          result_.batches.capacity() == 0U) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }

      PayloadRollback rollback(result_, before);
      for (std::size_t index = 0U; index < singleton_count; ++index) {
        ExactDirectSparseFacetKey key;
        key.point_count = 1U;
        key.point_ids[0U] = static_cast<PointId>(index);
        const auto token = replay_token(index, 1U);
        if (!token.has_value()) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_batch_inconsistent);
        }
        const ExactDirectSparseFacetWitness witness{
            config_.locator_config.external_authority_id, *token};
        const auto node_id =
            static_cast<ExactDirectMorseForestNodeId>(index);
        result_.birth_records.push_back(
            {index,
             index,
             0U,
             1U,
             key,
             index,
             node_id,
             witness});
        result_.nodes.push_back(
            {node_id,
             1U,
             exact::ExactLevel{},
             ExactDirectMorseForestNodeKind::order_one_birth,
             0U,
             0U,
             index,
             std::nullopt});
      }
      result_.batches.push_back(
          {0U,
           0U,
           1U,
           exact::ExactLevel{},
           0U,
           singleton_count,
           0U,
           0U,
           0U,
           0U,
           0U,
           0U,
           0U,
           0U,
           folded.pre_fold_locator_stamp,
           {},
           true,
           true,
           false});
      folded.canonical_singleton_bulk_count = singleton_count;
      folded.complete_batch_staged_before_mutation = true;
      folded.full_equal_level_quotient_resolved_before_mutation = true;

      const auto locator_commit =
          locator_.apply_canonical_singleton_identity_batch(
              singleton_count);
      if (!locator_commit.certified_committed_identity_batch()) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_locator_commit_rejected);
      }
      if (locator_commit.audit.bulk_count != singleton_count) {
        // The specialized locator transaction is already irreversible.
        // A mismatched bulk length would be an internal contract breach.
        std::terminate();
      }

      // No allocating operation follows the certified locator commit.
      components_.activate_initial_canonical_singletons(singleton_count);
      auto& committed_batch = result_.batches.back();
      committed_batch.closed_post_batch_carrier_count =
          components_.carrier_count(1U);
      committed_batch.closed_post_batch_reduced_root_count =
          components_.reduced_count(1U);
      committed_batch.committed_batch_stamp = locator_.snapshot_stamp();
      committed_batch.unions_then_births_committed_atomically = true;
      ++next_batch_index_;
      rollback.release();

      folded.locator_committed_before_scientific_state = true;
      folded.scientific_state_committed = true;
      folded.reducer_state_mutated = true;
      folded.post_fold_locator_stamp = locator_.snapshot_stamp();
      folded.decision = ExactDirectMorseForestReducerFoldDecision::
          complete_reducer_batch_commit;
      return folded;
    }

    std::vector<ResolvedState> resolved_states;
    resolved_states.reserve(batch.resolved_keys.size());
    for (std::size_t index = 0U;
         index < batch.resolved_keys.size();
         ++index) {
      const auto& resolved = batch.resolved_keys[index];
      if (resolved.resolved_key_index != index ||
          !resolved.source_projection_and_terminal_certified ||
          resolved.closure_disposition !=
              ExactDirectSparseFacetDescentClosureDisposition::
                  relative_positive ||
          !valid_key(
              resolved.source_facet_key,
              cloud_->size(),
              source_batch.order) ||
          (index != 0U &&
           !key_less(
               batch.resolved_keys[index - 1U].source_facet_key,
               resolved.source_facet_key)) ||
          resolved.resolved_binding_witness.external_authority_id !=
              config_.locator_config.external_authority_id ||
          resolved.resolved_binding_witness.replay_token == 0U ||
          resolved.resolved_binding_witness.replay_token % 3U != 1U) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }
      const std::uint64_t birth_index_u64 =
          (resolved.resolved_binding_witness.replay_token - 1U) / 3U;
      if (birth_index_u64 >
              std::numeric_limits<std::size_t>::max()) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }
      const std::size_t birth_index =
          static_cast<std::size_t>(birth_index_u64);
      if (birth_index >= result_.birth_records.size() ||
          resolved.resolved_component_handle >=
              components_.handle_count() ||
          !components_.active(resolved.resolved_component_handle) ||
          components_.order(resolved.resolved_component_handle) !=
              source_batch.order ||
          components_.canonical(birth_index) !=
              resolved.resolved_component_handle) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }
      const ExactDirectMorseForestBirthRecord& source_birth =
          result_.birth_records[birth_index];
      if (resolved.resolved_terminal_facet_key !=
              source_birth.facet_key ||
          source_birth.binding_witness !=
              resolved.resolved_binding_witness) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }
      const ExactDirectSparsePositiveFacetProbeResult terminal_probe =
          locator_.probe_positive_facet(
              resolved.resolved_terminal_facet_key,
              batch.locator_query_witness,
              terminal_probe_budget(budget_.closure_budget));
      if (!terminal_probe.certified_positive_hit() ||
          terminal_probe.component_handle !=
              resolved.resolved_component_handle ||
          terminal_probe.source_binding_witness !=
              resolved.resolved_binding_witness) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }
      resolved_states.push_back(
          {resolved.source_facet_key,
           resolved.resolved_component_handle,
           components_.reduced_root(
               resolved.resolved_component_handle)});
    }

    std::vector<TemporarySaddle> temporary_saddles;
    temporary_saddles.reserve(
        batch.source_family_end_index -
        batch.source_family_begin_index);
    std::vector<bool> resolved_seen(batch.resolved_keys.size(), false);
    std::size_t arm_cursor = batch.source_arm_seed_begin_index;
    std::size_t join_cursor = 0U;
    for (std::size_t family_index = batch.source_family_begin_index;
         family_index < batch.source_family_end_index;
         ++family_index) {
      const auto& family =
          source_seed_journal_->families[family_index];
      if (family.family_index != family_index ||
          family.journal_batch_index != batch.source_batch_index ||
          family.order != source_batch.order ||
          family.critical_squared_level != source_batch.squared_level ||
          family.arm_seed_count == 0U ||
          family.arm_seed_count > 4U ||
          family.arm_seed_offset != arm_cursor) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }
      TemporarySaddle saddle;
      saddle.source_family_index = family_index;
      saddle.arms.reserve(family.arm_seed_count);
      for (std::size_t local = 0U;
           local < family.arm_seed_count;
           ++local) {
        if (join_cursor >= batch.arm_joins.size() ||
            arm_cursor >= batch.source_arm_seed_end_index) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_batch_inconsistent);
        }
        const auto& seed =
            source_seed_journal_->arm_seeds[arm_cursor];
        const auto& join = batch.arm_joins[join_cursor];
        if (seed.arm_seed_index != arm_cursor ||
            seed.family_index != family_index ||
            join.arm_seed_index != arm_cursor ||
            join.family_index != family_index ||
            !join.arm_identity_and_full_key_joined ||
            join.resolved_key_index >= resolved_states.size()) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_batch_inconsistent);
        }
        const auto reconstructed = arm_key(
            reconstruct_exact_direct_saddle_arm_facet(
                *source_facade_,
                *source_seed_journal_,
                arm_cursor),
            cloud_->size(),
            source_batch.order);
        const auto& resolved =
            resolved_states[join.resolved_key_index];
        if (reconstructed != resolved.key) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_batch_inconsistent);
        }
        resolved_seen[join.resolved_key_index] = true;
        saddle.arms.push_back(
            {arm_cursor,
             reconstructed,
             resolved.carrier_handle,
             resolved.prior_reduced_root_node_id});
        ++arm_cursor;
        ++join_cursor;
      }
      temporary_saddles.push_back(std::move(saddle));
    }
    if (arm_cursor != batch.source_arm_seed_end_index ||
        join_cursor != batch.arm_joins.size() ||
        batch.arm_joins.size() !=
            batch.source_arm_seed_end_index -
                batch.source_arm_seed_begin_index ||
        std::find(resolved_seen.begin(), resolved_seen.end(), false) !=
            resolved_seen.end()) {
      return reject(
          std::move(folded),
          ExactDirectMorseForestReducerFoldDecision::
              no_reducer_batch_inconsistent);
    }

    std::vector<ExactFrozenRootHyperedge> hyperedges;
    std::vector<ExactFrozenRootId> carrier_references;
    hyperedges.reserve(temporary_saddles.size());
    carrier_references.reserve(batch.arm_joins.size());
    for (std::size_t saddle_index = 0U;
         saddle_index < temporary_saddles.size();
         ++saddle_index) {
      const auto& saddle = temporary_saddles[saddle_index];
      std::vector<ExactFrozenRootId> carriers;
      carriers.reserve(saddle.arms.size());
      for (const auto& arm : saddle.arms) {
        if (arm.carrier_handle >
            std::numeric_limits<ExactFrozenRootId>::max()) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_batch_inconsistent);
        }
        carriers.push_back(
            static_cast<ExactFrozenRootId>(arm.carrier_handle));
      }
      std::sort(carriers.begin(), carriers.end());
      carriers.erase(std::unique(carriers.begin(), carriers.end()),
                     carriers.end());
      if (carriers.empty()) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }
      hyperedges.push_back(
          {saddle_index, carrier_references.size(), carriers.size()});
      carrier_references.insert(
          carrier_references.end(), carriers.begin(), carriers.end());
    }

    std::optional<ExactFrozenRootQuotientResult> quotient;
    if (!hyperedges.empty()) {
      quotient.emplace(build_exact_direct_frozen_root_quotient(
          hyperedges, carrier_references, budget_.quotient_budget));
      if (!quotient->certified_frozen_root_quotient()) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_frozen_carrier_quotient_rejected);
      }
    }

    std::vector<GroupPlan> plans;
    if (quotient.has_value()) {
      plans.resize(quotient->groups.size());
      for (std::size_t group_index = 0U;
           group_index < quotient->groups.size();
           ++group_index) {
        const auto& group = quotient->groups[group_index];
        if (group.group_index != group_index || group.root_count == 0U ||
            group.root_offset > quotient->group_root_ids.size() ||
            group.root_count >
                quotient->group_root_ids.size() - group.root_offset) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_frozen_carrier_quotient_rejected);
        }
        GroupPlan& plan = plans[group_index];
        plan.atomic_group_index =
            result_.atomic_groups.size() + group_index;
        plan.carrier_handles.reserve(group.root_count);
        for (std::size_t local = 0U; local < group.root_count; ++local) {
          const ExactFrozenRootId frozen =
              quotient->group_root_ids[group.root_offset + local];
          if (frozen >
              std::numeric_limits<
                  ExactDirectSparseComponentHandle>::max()) {
            return reject(
                std::move(folded),
                ExactDirectMorseForestReducerFoldDecision::
                    no_reducer_batch_inconsistent);
          }
          const auto carrier =
              static_cast<ExactDirectSparseComponentHandle>(frozen);
          if (carrier >= components_.handle_count() ||
              !components_.active(carrier) ||
              components_.canonical(carrier) != carrier ||
              components_.order(carrier) != source_batch.order) {
            return reject(
                std::move(folded),
                ExactDirectMorseForestReducerFoldDecision::
                    no_reducer_batch_inconsistent);
          }
          plan.carrier_handles.push_back(carrier);
        }
      }
      for (const auto& binding : quotient->hyperedge_bindings) {
        if (binding.source_hyperedge_index >=
                temporary_saddles.size() ||
            binding.group_index >= plans.size()) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_frozen_carrier_quotient_rejected);
        }
        plans[binding.group_index].saddle_indices.push_back(
            binding.source_hyperedge_index);
      }

      std::size_t created_node_count = 0U;
      for (GroupPlan& plan : plans) {
        if (plan.saddle_indices.empty() ||
            plan.carrier_handles.empty()) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_batch_inconsistent);
        }
        for (const auto carrier : plan.carrier_handles) {
          const auto prior_root = components_.reduced_root(carrier);
          if (prior_root.has_value()) {
            plan.prior_reduced_root_node_ids.push_back(*prior_root);
          }
        }
        std::sort(
            plan.prior_reduced_root_node_ids.begin(),
            plan.prior_reduced_root_node_ids.end());
        if (std::adjacent_find(
                plan.prior_reduced_root_node_ids.begin(),
                plan.prior_reduced_root_node_ids.end()) !=
            plan.prior_reduced_root_node_ids.end()) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_batch_inconsistent);
        }
        if (plan.prior_reduced_root_node_ids.size() == 1U) {
          plan.kind =
              ExactDirectMorseForestAtomicGroupKind::continuation;
          plan.resulting_root_node_id =
              plan.prior_reduced_root_node_ids.front();
        } else {
          if (plan.prior_reduced_root_node_ids.empty()) {
            if (source_batch.order == 1U) {
              return reject(
                  std::move(folded),
                  ExactDirectMorseForestReducerFoldDecision::
                      no_reducer_batch_inconsistent);
            }
            plan.kind =
                ExactDirectMorseForestAtomicGroupKind::reduced_birth;
          } else {
            plan.kind =
                ExactDirectMorseForestAtomicGroupKind::multifusion;
          }
          const std::size_t node_index =
              result_.nodes.size() + created_node_count;
          if (node_index >= budget_.maximum_node_count ||
              node_index >
                  std::numeric_limits<
                      ExactDirectMorseForestNodeId>::max()) {
            return reject(
                std::move(folded),
                ExactDirectMorseForestReducerFoldDecision::
                    no_reducer_budget_exhausted);
          }
          plan.created_node_id =
              static_cast<ExactDirectMorseForestNodeId>(node_index);
          plan.resulting_root_node_id = *plan.created_node_id;
          ++created_node_count;
        }
      }
    }

    if (!append_count_within(
            result_.atomic_groups.size(),
            plans.size(),
            budget_.maximum_atomic_group_count) ||
        !append_count_within(
            result_.saddle_records.size(),
            temporary_saddles.size(),
            budget_.maximum_saddle_record_count) ||
        !append_count_within(
            result_.arm_root_bindings.size(),
            batch.arm_joins.size(),
            budget_.maximum_arm_root_binding_count)) {
      return reject(
          std::move(folded),
          ExactDirectMorseForestReducerFoldDecision::
              no_reducer_budget_exhausted);
    }

    std::vector<ExactDirectMorseForestArmRootBinding>
        pending_arm_bindings;
    std::vector<ExactDirectMorseForestSaddleRecord> pending_saddles;
    std::vector<ExactDirectMorseForestAtomicGroup> pending_groups;
    std::vector<ExactDirectMorseForestNodeId> pending_children;
    std::vector<ExactDirectMorseForestNode> pending_group_nodes;
    std::vector<ExactDirectSparseComponentUnion> locator_unions;
    pending_arm_bindings.reserve(batch.arm_joins.size());
    pending_saddles.reserve(temporary_saddles.size());
    pending_groups.reserve(plans.size());
    std::size_t reduced_birth_groups = 0U;
    std::size_t continuation_groups = 0U;
    std::size_t multifusion_groups = 0U;
    std::size_t maximum_carrier_arity = 0U;
    std::size_t maximum_merge_arity = 0U;

    for (GroupPlan& plan : plans) {
      const std::size_t group_saddle_offset =
          result_.saddle_records.size() + pending_saddles.size();
      for (const std::size_t saddle_index : plan.saddle_indices) {
        const auto& saddle = temporary_saddles[saddle_index];
        const auto& family =
            source_seed_journal_->families[
                saddle.source_family_index];
        const std::size_t arm_offset =
            result_.arm_root_bindings.size() +
            pending_arm_bindings.size();
        std::vector<ExactDirectSparseComponentHandle> saddle_carriers;
        std::vector<ExactDirectMorseForestNodeId> saddle_roots;
        saddle_carriers.reserve(saddle.arms.size());
        saddle_roots.reserve(saddle.arms.size());
        for (const auto& arm : saddle.arms) {
          pending_arm_bindings.push_back(
              {result_.arm_root_bindings.size() +
                   pending_arm_bindings.size(),
               arm.source_seed_index,
               saddle.source_family_index,
               arm.key,
               arm.carrier_handle,
               arm.prior_reduced_root_node_id});
          saddle_carriers.push_back(arm.carrier_handle);
          if (arm.prior_reduced_root_node_id.has_value()) {
            saddle_roots.push_back(*arm.prior_reduced_root_node_id);
          }
        }
        std::sort(saddle_carriers.begin(), saddle_carriers.end());
        saddle_carriers.erase(
            std::unique(saddle_carriers.begin(), saddle_carriers.end()),
            saddle_carriers.end());
        std::sort(saddle_roots.begin(), saddle_roots.end());
        saddle_roots.erase(
            std::unique(saddle_roots.begin(), saddle_roots.end()),
            saddle_roots.end());
        if (saddle_carriers.empty() ||
            saddle_roots.size() > saddle_carriers.size()) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_batch_inconsistent);
        }
        pending_saddles.push_back(
            {result_.saddle_records.size() + pending_saddles.size(),
             saddle.source_family_index,
             family.source_event_index,
             batch.source_batch_index,
             arm_offset,
             saddle.arms.size(),
             saddle_carriers.size(),
             saddle_carriers.size() - saddle_roots.size(),
             saddle_roots.size(),
             plan.atomic_group_index});
      }

      ExactDirectMorseForestAtomicGroup group_record;
      group_record.atomic_group_index = plan.atomic_group_index;
      group_record.batch_index = result_.batches.size();
      group_record.saddle_record_offset = group_saddle_offset;
      group_record.saddle_record_count =
          result_.saddle_records.size() + pending_saddles.size() -
          group_saddle_offset;
      group_record.frozen_carrier_count = plan.carrier_handles.size();
      group_record.latent_carrier_count =
          plan.carrier_handles.size() -
          plan.prior_reduced_root_node_ids.size();
      group_record.prior_reduced_root_count =
          plan.prior_reduced_root_node_ids.size();
      group_record.child_offset =
          result_.child_node_ids.size() + pending_children.size();
      group_record.created_node_id = plan.created_node_id;
      group_record.resulting_root_node_id =
          plan.resulting_root_node_id;
      group_record.kind = plan.kind;
      maximum_carrier_arity =
          std::max(maximum_carrier_arity, plan.carrier_handles.size());
      maximum_merge_arity = std::max(
          maximum_merge_arity,
          plan.prior_reduced_root_node_ids.size());
      switch (plan.kind) {
        case ExactDirectMorseForestAtomicGroupKind::reduced_birth:
          ++reduced_birth_groups;
          break;
        case ExactDirectMorseForestAtomicGroupKind::continuation:
          ++continuation_groups;
          break;
        case ExactDirectMorseForestAtomicGroupKind::multifusion:
          ++multifusion_groups;
          group_record.child_count =
              plan.prior_reduced_root_node_ids.size();
          if (!append_count_within(
                  result_.child_node_ids.size() +
                      pending_children.size(),
                  plan.prior_reduced_root_node_ids.size(),
                  budget_.maximum_child_reference_count)) {
            return reject(
                std::move(folded),
                ExactDirectMorseForestReducerFoldDecision::
                    no_reducer_budget_exhausted);
          }
          pending_children.insert(
              pending_children.end(),
              plan.prior_reduced_root_node_ids.begin(),
              plan.prior_reduced_root_node_ids.end());
          break;
      }
      if (plan.kind !=
          ExactDirectMorseForestAtomicGroupKind::continuation) {
        pending_group_nodes.push_back(
            {*plan.created_node_id,
             source_batch.order,
             source_batch.squared_level,
             plan.kind ==
                     ExactDirectMorseForestAtomicGroupKind::reduced_birth
                 ? ExactDirectMorseForestNodeKind::reduced_birth
                 : ExactDirectMorseForestNodeKind::multifusion,
             group_record.child_offset,
             group_record.child_count,
             std::nullopt,
             plan.atomic_group_index});
      }
      for (std::size_t local = 1U;
           local < plan.carrier_handles.size();
           ++local) {
        const std::size_t local_union_index = locator_unions.size();
        const std::size_t global_union_index =
            result_.counters.locator_union_count + local_union_index;
        const auto token = replay_token(global_union_index, 2U);
        if (!token.has_value()) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_batch_inconsistent);
        }
        locator_unions.push_back(
            {local_union_index,
             plan.carrier_handles.front(),
             plan.carrier_handles[local],
             {config_.locator_config.external_authority_id, *token}});
      }
      pending_groups.push_back(group_record);
    }

    std::vector<ExactDirectMorseForestBirthRecord> pending_births;
    std::vector<ExactDirectMorseForestNode> pending_birth_nodes;
    std::vector<ExactDirectSparseFacetBinding> locator_bindings;
    const std::size_t role_begin = source_batch.role_record_offset;
    if (role_begin > source_journal_->role_records.size() ||
        source_batch.role_record_count >
            source_journal_->role_records.size() - role_begin) {
      return reject(
          std::move(folded),
          ExactDirectMorseForestReducerFoldDecision::
              no_reducer_batch_inconsistent);
    }
    pending_births.reserve(source_batch.birth_role_count);
    pending_birth_nodes.reserve(source_batch.birth_role_count);
    locator_bindings.reserve(source_batch.birth_role_count);
    for (std::size_t local = 0U;
         local < source_batch.role_record_count;
         ++local) {
      const auto& role = source_journal_->role_records[role_begin + local];
      if (role.batch_index != batch.source_batch_index ||
          role.event_projection_index >=
              source_journal_->event_projections.size()) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }
      if (role.role != ExactDirectMorseH0Role::birth) {
        continue;
      }
      const auto& projection =
          source_journal_->event_projections[
              role.event_projection_index];
      const auto key = birth_key(
          projection,
          *source_facade_,
          cloud_->size(),
          source_batch.order,
          source_batch.squared_level);
      const std::size_t birth_index =
          result_.birth_records.size() + pending_births.size();
      const std::size_t node_index =
          result_.nodes.size() + pending_group_nodes.size() +
          pending_birth_nodes.size();
      if (birth_index >= birth_count_) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_budget_exhausted);
      }
      const auto token = replay_token(birth_index, 1U);
      if (!token.has_value()) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }
      const ExactDirectSparseFacetWitness witness{
          config_.locator_config.external_authority_id, *token};
      std::optional<ExactDirectMorseForestNodeId> birth_node;
      if (source_batch.order == 1U) {
        if (node_index >= budget_.maximum_node_count ||
            node_index >
                std::numeric_limits<
                    ExactDirectMorseForestNodeId>::max()) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_budget_exhausted);
        }
        birth_node =
            static_cast<ExactDirectMorseForestNodeId>(node_index);
      }
      pending_births.push_back(
          {birth_index,
           role.event_projection_index,
           batch.source_batch_index,
           source_batch.order,
           key,
           birth_index,
           birth_node,
           witness});
      if (birth_node.has_value()) {
        pending_birth_nodes.push_back(
            {*birth_node,
             source_batch.order,
             source_batch.squared_level,
             ExactDirectMorseForestNodeKind::order_one_birth,
             result_.child_node_ids.size() + pending_children.size(),
             0U,
             birth_index,
             std::nullopt});
      }
      locator_bindings.push_back(
          {locator_bindings.size(), key, birth_index, witness});
    }
    if (pending_births.size() != source_batch.birth_role_count ||
        !append_count_within(
            result_.birth_records.size(),
            pending_births.size(),
            budget_.maximum_birth_record_count) ||
        !append_count_within(
            result_.nodes.size(),
            pending_group_nodes.size() + pending_birth_nodes.size(),
            budget_.maximum_node_count)) {
      return reject(
          std::move(folded),
          ExactDirectMorseForestReducerFoldDecision::
              no_reducer_batch_inconsistent);
    }
    folded.staged_birth_record_count = pending_births.size();
    folded.staged_birth_node_count = pending_birth_nodes.size();
    folded.staged_locator_binding_count = locator_bindings.size();

    const std::size_t strict_carrier_count =
        components_.carrier_count(source_batch.order);
    const std::size_t strict_reduced_count =
        components_.reduced_count(source_batch.order);
    const PayloadSizes before = payload_sizes(result_);
    PayloadRollback rollback(result_, before);
    append_moved(result_.arm_root_bindings, pending_arm_bindings);
    append_moved(result_.saddle_records, pending_saddles);
    append_moved(result_.atomic_groups, pending_groups);
    append_moved(result_.child_node_ids, pending_children);
    append_moved(result_.nodes, pending_group_nodes);
    append_moved(result_.birth_records, pending_births);
    append_moved(result_.nodes, pending_birth_nodes);
    result_.batches.push_back(
        {before.batches,
         batch.source_batch_index,
         source_batch.order,
         source_batch.squared_level,
         before.birth_records,
         result_.birth_records.size() - before.birth_records,
         before.saddle_records,
         result_.saddle_records.size() - before.saddle_records,
         before.atomic_groups,
         result_.atomic_groups.size() - before.atomic_groups,
         strict_carrier_count,
         strict_reduced_count,
         0U,
         0U,
         folded.pre_fold_locator_stamp,
         {},
         true,
         true,
         false});
    folded.complete_batch_staged_before_mutation = true;
    folded.full_equal_level_quotient_resolved_before_mutation = true;

    const auto locator_commit = locator_.apply_batch(
        std::span<const ExactDirectSparseFacetQuery>{},
        locator_unions,
        locator_bindings);
    if (!locator_commit.certified_committed_batch()) {
      return reject(
          std::move(folded),
          ExactDirectMorseForestReducerFoldDecision::
              no_reducer_locator_commit_rejected);
    }
    if (locator_commit.counters.union_request_count !=
            locator_unions.size() ||
        locator_commit.counters.binding_request_count !=
            locator_bindings.size() ||
        locator_commit.counters.inserted_binding_count !=
            locator_bindings.size() ||
        locator_commit.counters.compatible_duplicate_binding_count !=
            0U) {
      // A certified locator commit is irreversible.  These equalities are
      // consequences of the already validated disjoint batch, so violating
      // one is an internal contract breach, never a recoverable rejection.
      std::terminate();
    }

    // No allocating operation follows this point.  Union-by-rank owns a
    // canonical-minimum attribute, so physical DSU roots never leak into the
    // hierarchy identity or the sparse locator protocol.
    for (const GroupPlan& plan : plans) {
      components_.commit_group(
          plan.carrier_handles,
          source_batch.order,
          plan.prior_reduced_root_node_ids.size(),
          plan.resulting_root_node_id);
    }
    for (std::size_t index = before.birth_records;
         index < result_.birth_records.size();
         ++index) {
      const auto& birth = result_.birth_records[index];
      components_.activate(
          birth.component_handle,
          birth.order,
          birth.order_one_birth_node_id);
    }
    auto& committed_batch = result_.batches.back();
    committed_batch.closed_post_batch_carrier_count =
        components_.carrier_count(source_batch.order);
    committed_batch.closed_post_batch_reduced_root_count =
        components_.reduced_count(source_batch.order);
    committed_batch.committed_batch_stamp = locator_.snapshot_stamp();
    committed_batch.unions_then_births_committed_atomically = true;

    result_.counters.reduced_birth_group_count += reduced_birth_groups;
    result_.counters.continuation_group_count += continuation_groups;
    result_.counters.multifusion_group_count += multifusion_groups;
    result_.counters.locator_union_count += locator_unions.size();
    result_.counters.closure_call_count +=
        batch.shared_closure_build_count;
    result_.counters.quotient_call_count +=
        hyperedges.empty() ? 0U : 1U;
    result_.counters.distinct_strict_arm_count +=
        batch.resolved_keys.size();
    result_.counters.duplicate_strict_arm_reference_count +=
        batch.arm_joins.size() - batch.resolved_keys.size();
    result_.counters.aggregate_closure_node_count +=
        batch.transient_closure_node_count;
    result_.counters.aggregate_closure_step_call_count +=
        batch.transient_closure_step_call_count;
    result_.counters.maximum_batch_arm_count = std::max(
        result_.counters.maximum_batch_arm_count,
        batch.arm_joins.size());
    result_.counters.maximum_batch_carrier_arity = std::max(
        result_.counters.maximum_batch_carrier_arity,
        maximum_carrier_arity);
    result_.counters.maximum_batch_merge_arity = std::max(
        result_.counters.maximum_batch_merge_arity,
        maximum_merge_arity);
    next_family_index_ = batch.source_family_end_index;
    next_arm_seed_index_ = batch.source_arm_seed_end_index;
    ++next_batch_index_;
    rollback.release();

    folded.locator_committed_before_scientific_state = true;
    folded.scientific_state_committed = true;
    folded.reducer_state_mutated = true;
    folded.post_fold_locator_stamp = locator_.snapshot_stamp();
    folded.decision = ExactDirectMorseForestReducerFoldDecision::
        complete_reducer_batch_commit;
    return folded;
  }

  const spatial::CanonicalPointCloud* cloud_{};
  const ExactDirectSupportTerminalFacade* source_facade_{};
  const ExactDirectMorseEventJournalResult* source_journal_{};
  const ExactDirectSaddleArmSeedJournalResult* source_seed_journal_{};
  ExactDirectMorseForestBudget budget_{};
  ExactDirectMorseForestConfig config_{};
  spatial::LbvhTraversalOrder traversal_order_{
      spatial::LbvhTraversalOrder::near_first};
  std::size_t birth_count_{};
  ExactDirectSparsePositiveFacetLocator locator_;
  CarrierDsu components_;
  ExactDirectMorseForestJournalResult result_;
  std::size_t next_batch_index_{};
  std::size_t next_family_index_{};
  std::size_t next_arm_seed_index_{};
  bool finished_{false};
};

ExactDirectMorseForestReducer::ExactDirectMorseForestReducer(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectMorseForestBudget& budget,
    const ExactDirectMorseForestConfig& config,
    spatial::LbvhTraversalOrder traversal_order)
    : impl_(std::make_unique<Impl>(
          cloud,
          source_facade,
          source_journal,
          trusted_seed_budget,
          source_seed_journal,
          budget,
          config,
          traversal_order)) {}

ExactDirectMorseForestReducer::~ExactDirectMorseForestReducer() = default;

ExactDirectMorseForestReducer::ExactDirectMorseForestReducer(
    ExactDirectMorseForestReducer&&) noexcept = default;

ExactDirectMorseForestReducer&
ExactDirectMorseForestReducer::operator=(
    ExactDirectMorseForestReducer&&) noexcept = default;

ExactDirectMorseForestReducerFoldResult
ExactDirectMorseForestReducer::fold(
    const ExactDirectMorseForestReducerBatch& batch) {
  if (!impl_) {
    throw std::logic_error("a moved forest reducer cannot fold");
  }
  return impl_->fold(batch);
}

ExactDirectMorseForestLiveCommitResult
ExactDirectMorseForestReducer::fold_prepared_ticket(
    ExactDirectSparseFacetDescentAnchoredBatchExecutor& executor,
    ExactDirectSparseFacetDescentAnchoredBatchExecutor::
        PreparedTopKProposalBatch&& prepared) {
  if (!impl_) {
    throw std::logic_error(
        "a moved forest reducer cannot commit a live batch");
  }
  static_assert(
      std::is_nothrow_move_constructible_v<
          ExactDirectSparseFacetDescentBatchExecutionResult>);
  static_assert(
      std::is_nothrow_move_constructible_v<
          ExactDirectSparseFacetDescentClosureTopKProposalConsumptionAudit>);
  static_assert(
      std::is_nothrow_move_constructible_v<
          ExactDirectMorseForestLiveCommitResult>);
  static_assert(
      std::is_nothrow_move_assignable_v<
          ExactDirectMorseForestReducerFoldResult>);

  ExactDirectMorseForestLiveCommitResult result;
  result.source_batch_index = prepared.source_batch_index_;
  result.successor_batch_index = prepared.successor_batch_index_;
  result.pre_commit_executor_batch_index =
      executor.next_source_batch_index_;
  result.post_commit_executor_batch_index =
      result.pre_commit_executor_batch_index;
  result.pre_commit_locator_stamp = impl_->locator().snapshot_stamp();
  result.post_commit_locator_stamp = result.pre_commit_locator_stamp;

  const std::size_t pre_epoch = executor.source_epoch_;
  const std::size_t pre_batch_index =
      executor.next_source_batch_index_;
  const std::size_t pre_chunk_index =
      executor.next_source_chunk_index_;
  const std::size_t pre_lane_index =
      executor.next_source_lane_index_;
  const std::size_t pre_family_index =
      executor.next_source_family_index_;
  const std::size_t pre_arm_seed_index =
      executor.next_source_arm_seed_index_;

  const auto executor_cursor_unchanged = [&]() noexcept {
    return executor.source_epoch_ == pre_epoch &&
           executor.next_source_batch_index_ == pre_batch_index &&
           executor.next_source_chunk_index_ == pre_chunk_index &&
           executor.next_source_lane_index_ == pre_lane_index &&
           executor.next_source_family_index_ == pre_family_index &&
           executor.next_source_arm_seed_index_ == pre_arm_seed_index;
  };
  const auto consume_ticket = [&]() noexcept {
    prepared.session_seal_.reset();
    prepared.exact_scientific_delta_provenance_minted_ = false;
    prepared.valid_ = false;
    prepared.consumed_ = true;
    result.ticket_consumed = true;
  };
  const auto reject =
      [&](ExactDirectMorseForestLiveCommitDecision decision,
          bool account_rejection) {
        if (account_rejection) {
          if (executor.audit_.sealed_ticket_rejected_commit_count !=
              std::numeric_limits<std::size_t>::max()) {
            ++executor.audit_.sealed_ticket_rejected_commit_count;
          } else {
            decision = ExactDirectMorseForestLiveCommitDecision::
                no_live_commit_executor_audit_capacity_exhausted;
          }
        }
        consume_ticket();
        result.post_commit_executor_batch_index =
            executor.next_source_batch_index_;
        result.post_commit_locator_stamp =
            impl_->locator().snapshot_stamp();
        result.executor_cursor_unchanged_on_rejection =
            executor_cursor_unchanged();
        result.locator_unchanged_on_rejection =
            result.post_commit_locator_stamp ==
            result.pre_commit_locator_stamp;
        result.decision = decision;
        return result;
      };

  if (executor.audit_.sealed_ticket_commit_attempt_count ==
      std::numeric_limits<std::size_t>::max()) {
    return reject(
        ExactDirectMorseForestLiveCommitDecision::
            no_live_commit_executor_audit_capacity_exhausted,
        false);
  }
  ++executor.audit_.sealed_ticket_commit_attempt_count;

  result.ticket_was_valid_and_unconsumed =
      prepared.valid_ && !prepared.consumed_;
  if (!result.ticket_was_valid_and_unconsumed) {
    return reject(
        ExactDirectMorseForestLiveCommitDecision::
            no_live_commit_invalid_moved_or_consumed_ticket,
        true);
  }

  result.shared_session_seal_matches =
      prepared.session_seal_ &&
      prepared.session_seal_ == executor.session_seal_;
  if (!result.shared_session_seal_matches) {
    return reject(
        ExactDirectMorseForestLiveCommitDecision::
            no_live_commit_foreign_session,
        true);
  }

  result.source_epoch_and_full_cursor_match =
      prepared.source_epoch_ == executor.source_epoch_ &&
      prepared.source_batch_index_ ==
          executor.next_source_batch_index_ &&
      prepared.source_chunk_index_ ==
          executor.next_source_chunk_index_ &&
      prepared.source_lane_index_ ==
          executor.next_source_lane_index_ &&
      prepared.source_family_index_ ==
          executor.next_source_family_index_ &&
      prepared.source_arm_seed_index_ ==
          executor.next_source_arm_seed_index_ &&
      !executor.complete();
  if (!result.source_epoch_and_full_cursor_match) {
    return reject(
        ExactDirectMorseForestLiveCommitDecision::
            no_live_commit_stale_epoch_or_cursor,
        true);
  }

  result.exact_scientific_delta_provenance_minted =
      prepared.exact_scientific_delta_provenance_minted_ &&
      prepared.preparation_.scientific_delta.has_value();
  result.reducer_and_executor_share_locator_instance =
      executor.locator_ == &impl_->locator();
  const bool locator_snapshot_matches =
      result.reducer_and_executor_share_locator_instance &&
      prepared.locator_snapshot_stamp_ ==
          result.pre_commit_locator_stamp &&
      executor.locator_->snapshot_stamp() ==
          result.pre_commit_locator_stamp;
  if (!result.exact_scientific_delta_provenance_minted ||
      !locator_snapshot_matches) {
    return reject(
        result.exact_scientific_delta_provenance_minted
            ? ExactDirectMorseForestLiveCommitDecision::
                  no_live_commit_distinct_locator_or_snapshot
            : ExactDirectMorseForestLiveCommitDecision::
                  no_live_commit_invalid_moved_or_consumed_ticket,
        true);
  }

  result.reducer_and_executor_batch_cursors_match =
      impl_->next_batch_index() ==
          executor.next_source_batch_index_;
  if (!result.reducer_and_executor_batch_cursors_match) {
    return reject(
        ExactDirectMorseForestLiveCommitDecision::
            no_live_commit_reducer_cursor_mismatch,
        true);
  }

  if (executor.audit_.sealed_ticket_rejected_commit_count ==
          std::numeric_limits<std::size_t>::max() ||
      executor.audit_.accepted_batch_count ==
          std::numeric_limits<std::size_t>::max() ||
      executor.audit_.sealed_ticket_accepted_commit_count ==
          std::numeric_limits<std::size_t>::max() ||
      executor.audit_.sealed_ticket_exact_replay_avoided_count ==
          std::numeric_limits<std::size_t>::max() ||
      executor.source_epoch_ ==
          std::numeric_limits<std::size_t>::max()) {
    return reject(
        ExactDirectMorseForestLiveCommitDecision::
            no_live_commit_executor_audit_capacity_exhausted,
        false);
  }
  result.executor_commit_capacity_preflighted = true;

  ExactDirectMorseForestReducerBatch projected;
  try {
    projected = project_exact_direct_morse_forest_reducer_batch(
        *prepared.preparation_.scientific_delta);
  } catch (const std::invalid_argument&) {
    return reject(
        ExactDirectMorseForestLiveCommitDecision::
            no_live_commit_invalid_moved_or_consumed_ticket,
        true);
  }
  result.all_fallible_scientific_work_precedes_irreversible_mutation = true;
  result.reducer_fold_attempted = true;
  result.reducer_fold = impl_->fold(projected);
  if (!result.reducer_fold.certified_committed_batch()) {
    if (!result.reducer_fold.certified_atomic_rejection() ||
        !executor_cursor_unchanged() ||
        impl_->locator().snapshot_stamp() !=
            result.pre_commit_locator_stamp) {
      std::terminate();
    }
    return reject(
        ExactDirectMorseForestLiveCommitDecision::
            no_live_commit_reducer_fold_rejected,
        true);
  }

  // From here to the synchronized successor cursor there is no allocation,
  // hash, probe, quotient, replay or budget decision.  Any contradiction is
  // therefore an internal fail-stop contract breach, not a recoverable
  // rejection after an irreversible locator commit.
  if (result.reducer_fold.source_batch_index !=
          result.source_batch_index ||
      result.reducer_fold.pre_fold_locator_stamp !=
          result.pre_commit_locator_stamp ||
      result.reducer_fold.post_fold_locator_stamp !=
          impl_->locator().snapshot_stamp() ||
      !executor_cursor_unchanged()) {
    std::terminate();
  }
  result.reducer_committed_before_executor_cursor = true;
  result.no_fallible_operation_after_reducer_commit = true;

  result.scientific_delta.emplace(
      std::move(*prepared.preparation_.scientific_delta));
  prepared.preparation_.scientific_delta.reset();
  if (prepared.preparation_.proposal_consumption_audit.has_value()) {
    result.operational_audit.emplace(std::move(
        *prepared.preparation_.proposal_consumption_audit));
    prepared.preparation_.proposal_consumption_audit.reset();
  }

  executor.next_source_batch_index_ =
      prepared.successor_batch_index_;
  executor.next_source_chunk_index_ =
      prepared.successor_chunk_index_;
  executor.next_source_lane_index_ =
      prepared.successor_lane_index_;
  executor.next_source_family_index_ =
      prepared.successor_family_index_;
  executor.next_source_arm_seed_index_ =
      prepared.successor_arm_seed_index_;
  ++executor.source_epoch_;
  ++executor.audit_.accepted_batch_count;
  ++executor.audit_.sealed_ticket_accepted_commit_count;
  ++executor.audit_.sealed_ticket_exact_replay_avoided_count;
  executor.audit_.sealed_ticket_or_delta_retained_by_session = false;

  result.executor_cursor_advanced = true;
  result.scientific_delta_moved_to_result = true;
  consume_ticket();
  result.post_commit_executor_batch_index =
      executor.next_source_batch_index_;
  result.post_commit_locator_stamp =
      impl_->locator().snapshot_stamp();
  result.decision = ExactDirectMorseForestLiveCommitDecision::
      complete_live_reducer_then_cursor_commit;
  if (!result.certified_live_commit()) {
    std::terminate();
  }
  return result;
}

const ExactDirectSparsePositiveFacetLocator&
ExactDirectMorseForestReducer::strict_locator() const noexcept {
  return impl_->locator();
}

std::size_t ExactDirectMorseForestReducer::next_source_batch_index()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->next_batch_index();
}

bool ExactDirectMorseForestReducer::complete() const noexcept {
  return impl_ != nullptr && impl_->complete();
}

ExactDirectMorseForestJournalResult ExactDirectMorseForestReducer::finish() {
  if (!impl_) {
    throw std::logic_error("a moved forest reducer cannot finish");
  }
  return impl_->finish();
}

}  // namespace morsehgp3d::hierarchy
