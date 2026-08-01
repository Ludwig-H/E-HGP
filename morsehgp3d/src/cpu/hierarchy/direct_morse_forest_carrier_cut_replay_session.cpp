#include "morsehgp3d/hierarchy/direct_morse_forest_carrier_cut_replay_session.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <new>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include <boost/multiprecision/integer.hpp>

namespace morsehgp3d::hierarchy {
namespace {

enum class ReplayFailure : std::uint8_t {
  none,
  capacity_overflow,
  allocation_failed,
  budget_exhausted,
  source_forest_contradiction,
  locator_commit_rejected,
  locator_stamp_contradiction,
};

[[nodiscard]] bool checked_add(
    std::size_t left,
    std::size_t right,
    std::size_t& result) noexcept {
  if (left > std::numeric_limits<std::size_t>::max() - right) {
    return false;
  }
  result = left + right;
  return true;
}

[[nodiscard]] std::optional<std::uint64_t> replay_token(
    std::size_t index,
    std::uint64_t residue) noexcept {
  constexpr std::uint64_t stride = 3U;
  if (residue == 0U || residue > stride ||
      index >
          (std::numeric_limits<std::uint64_t>::max() - residue) /
              stride) {
    return std::nullopt;
  }
  return static_cast<std::uint64_t>(index) * stride + residue;
}

[[nodiscard]] bool valid_nonnegative_level(
    const exact::ExactLevel& level) noexcept {
  return level.numerator() >= 0 && level.denominator() > 0;
}

[[nodiscard]] std::size_t positive_integer_bit_count(
    const exact::BigInt& value) {
  if (value == 0) {
    return 1U;
  }
  return static_cast<std::size_t>(boost::multiprecision::msb(value)) + 1U;
}

[[nodiscard]] bool canonical_key_shape(
    const ExactDirectSparseFacetKey& key,
    std::size_t point_count,
    std::size_t expected_order) noexcept {
  if (key.point_count != expected_order || key.point_count == 0U ||
      key.point_count > direct_sparse_positive_facet_maximum_point_count) {
    return false;
  }
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    if (key.point_ids[index] >= point_count ||
        (index != 0U &&
         key.point_ids[index - 1U] >= key.point_ids[index])) {
      return false;
    }
  }
  return std::all_of(
      key.point_ids.begin() +
          static_cast<std::ptrdiff_t>(key.point_count),
      key.point_ids.end(),
      [](spatial::PointId point_id) { return point_id == 0U; });
}

struct ComponentState {
  std::size_t parent{};
  std::size_t order{};
  std::optional<ExactDirectMorseForestNodeId> reduced_root_node_id;
  std::size_t last_group_marker{};
  bool active{false};
};

struct NodeState {
  std::optional<ExactDirectSparseComponentHandle> owner_component_handle;
  std::size_t last_group_marker{};
};

struct BatchGroupPlan {
  std::size_t group_index{};
  std::size_t carrier_offset{};
  std::size_t carrier_count{};
  std::size_t prior_root_offset{};
  std::size_t prior_root_count{};
};

struct AdvanceScratchRequirements {
  std::size_t maximum_group_plan_count{};
  std::size_t maximum_group_carrier_reference_count{};
  std::size_t maximum_group_prior_root_reference_count{};
  std::size_t maximum_locator_union_count{};
  std::size_t maximum_locator_binding_count{};
};

[[nodiscard]] ExactDirectMorseForestCarrierCutReplayAdvanceDecision
advance_decision(ReplayFailure failure) noexcept {
  switch (failure) {
    case ReplayFailure::capacity_overflow:
      return ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
          no_replay_capacity_overflow;
    case ReplayFailure::allocation_failed:
      return ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
          no_replay_allocation_failed;
    case ReplayFailure::budget_exhausted:
      return ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
          no_replay_budget_exhausted;
    case ReplayFailure::source_forest_contradiction:
      return ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
          no_replay_source_forest_contradiction;
    case ReplayFailure::locator_commit_rejected:
      return ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
          no_replay_locator_commit_rejected;
    case ReplayFailure::locator_stamp_contradiction:
      return ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
          no_replay_locator_stamp_contradiction;
    case ReplayFailure::none:
      break;
  }
  return ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
      not_certified;
}

[[nodiscard]] ExactDirectMorseForestCarrierCutReplaySessionInitialization
initialization_failure(
    ExactDirectMorseForestCarrierCutReplaySessionInitialization result,
    ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision
        decision) noexcept {
  result.session.reset();
  result.decision = decision;
  result.scope = ExactDirectMorseForestCarrierCutReplayScope::unspecified;
  return result;
}

[[nodiscard]] bool initialization_non_scope_honest(
    const ExactDirectMorseForestCarrierCutReplaySessionInitialization&
        result) noexcept {
  return result.source_forest_must_remain_immutable &&
         result.forest_relative_only &&
         !result.gamma_cells_or_global_cofaces_materialized &&
         !result.higher_order_delaunay_materialized &&
         !result.public_status_claimed;
}

[[nodiscard]] bool advance_non_scope_honest(
    const ExactDirectMorseForestCarrierCutReplayAdvanceResult& result)
    noexcept {
  return result.forest_relative_only &&
         !result.gamma_cells_or_global_cofaces_materialized &&
         !result.higher_order_delaunay_materialized &&
         !result.public_status_claimed;
}

}  // namespace

struct ExactDirectMorseForestCarrierCutReplaySession::Impl {
  const ExactDirectMorseForestJournalResult* source_forest{};
  ExactDirectMorseForestCarrierCutReplaySessionBudget budget{};
  ExactDirectSparsePositiveFacetLocator locator{};
  std::size_t target_order{};
  std::size_t birth_record_count{};
  std::size_t node_count{};
  std::size_t next_global_batch_index{};
  std::size_t active_locator_union_count{};
  std::size_t active_locator_binding_count{};
  std::optional<exact::ExactLevel> current_closed_squared_level;
  std::vector<ComponentState> components;
  std::vector<NodeState> node_states;
  std::vector<std::size_t> active_carrier_root_counts_by_order;
  std::vector<std::size_t> active_reduced_root_counts_by_order;
  std::vector<ExactDirectSparseComponentHandle> target_handles;
  std::vector<BatchGroupPlan> scratch_group_plans;
  std::vector<ExactDirectSparseComponentHandle>
      scratch_group_carriers;
  std::vector<ExactDirectMorseForestNodeId> scratch_group_prior_roots;
  std::vector<ExactDirectSparseComponentUnion> scratch_locator_unions;
  std::vector<ExactDirectSparseFacetBinding> scratch_locator_bindings;
  ExactDirectMorseForestCarrierCutReplaySessionCounters counters{};
  std::uint64_t visit_epoch{};
  bool ready{false};
  bool poisoned{false};
  bool view_live{false};
};

namespace {

[[nodiscard]] bool measure_level(
    ExactDirectMorseForestCarrierCutReplaySession::Impl&,
    const exact::ExactLevel&);
[[nodiscard]] std::optional<int> compare_levels(
    ExactDirectMorseForestCarrierCutReplaySession::Impl&,
    const exact::ExactLevel&,
    const exact::ExactLevel&);
[[nodiscard]] bool singleton_batch_shape(
    const ExactDirectMorseForestCarrierCutReplaySession::Impl&,
    const ExactDirectMorseForestBatch&) noexcept;
[[nodiscard]] bool preflight_advance_scratch(
    ExactDirectMorseForestCarrierCutReplaySession::Impl&,
    std::size_t,
    std::size_t,
    AdvanceScratchRequirements&,
    ReplayFailure&);
void reserve_advance_scratch(
    ExactDirectMorseForestCarrierCutReplaySession::Impl&,
    const AdvanceScratchRequirements&);
[[nodiscard]] ReplayFailure replay_batch(
    ExactDirectMorseForestCarrierCutReplaySession::Impl&,
    std::size_t);
[[nodiscard]] std::optional<std::size_t> find_component_root_const(
    const ExactDirectMorseForestCarrierCutReplaySession::Impl&,
    ExactDirectSparseComponentHandle) noexcept;

}  // namespace

ExactDirectMorseForestCarrierCutReplayAdvanceResult
ExactDirectMorseForestCarrierCutReplaySession::
    advance_to_closed_cut_erased(
        const exact::ExactLevel& closed_squared_level,
        void* erased_consumer,
        ErasedConsumer consumer) & {
  ExactDirectMorseForestCarrierCutReplayAdvanceResult result;
  result.target_order = target_order();
  try {
    result.requested_closed_squared_level = closed_squared_level;
  } catch (const std::bad_alloc&) {
    result.decision =
        ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
            no_replay_allocation_failed;
    return result;
  } catch (const std::length_error&) {
    result.decision =
        ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
            no_replay_capacity_overflow;
    return result;
  }

  if (impl_ == nullptr || !impl_->ready || impl_->poisoned ||
      impl_->view_live || erased_consumer == nullptr || consumer == nullptr) {
    result.session_poisoned = impl_ != nullptr && impl_->poisoned;
    result.decision =
        ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
            no_replay_session_not_ready;
    return result;
  }
  result.cumulative_counters = impl_->counters;
  result.committed_global_batch_prefix_count =
      impl_->next_global_batch_index;
  result.frozen_locator_stamp = impl_->locator.snapshot_stamp();
  if (!valid_nonnegative_level(closed_squared_level)) {
    result.decision =
        ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
            no_replay_invalid_closed_squared_level;
    return result;
  }
  if (!measure_level(*impl_, closed_squared_level)) {
    result.cumulative_counters = impl_->counters;
    result.decision =
        ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
            no_replay_budget_exhausted;
    return result;
  }
  if (impl_->visit_epoch == std::numeric_limits<std::uint64_t>::max()) {
    result.cumulative_counters = impl_->counters;
    result.decision =
        ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
            no_replay_capacity_overflow;
    return result;
  }

  const std::size_t prefix_at_advance_entry =
      impl_->next_global_batch_index;
  bool consumer_started = false;
  try {
    if (impl_->current_closed_squared_level.has_value()) {
      const auto relation = compare_levels(
          *impl_,
          closed_squared_level,
          *impl_->current_closed_squared_level);
      if (!relation.has_value()) {
        result.cumulative_counters = impl_->counters;
        result.decision =
            ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
                no_replay_budget_exhausted;
        return result;
      }
      if (*relation < 0) {
        result.cumulative_counters = impl_->counters;
        result.decision =
            ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
                no_replay_nonmonotone_closed_cut;
        return result;
      }
    }

    exact::ExactLevel prepared_closed_squared_level = closed_squared_level;
    std::size_t end_batch = impl_->next_global_batch_index;
    while (end_batch < impl_->source_forest->batches.size()) {
      const auto& batch = impl_->source_forest->batches[end_batch];
      if (batch.order < impl_->target_order) {
        ++end_batch;
        continue;
      }
      if (batch.order > impl_->target_order) {
        break;
      }
      const auto relation = compare_levels(
          *impl_, batch.squared_level, closed_squared_level);
      if (!relation.has_value()) {
        result.cumulative_counters = impl_->counters;
        result.decision =
            ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
                no_replay_budget_exhausted;
        return result;
      }
      if (*relation > 0) {
        break;
      }
      ++end_batch;
    }

    AdvanceScratchRequirements requirements;
    ReplayFailure preflight_failure = ReplayFailure::none;
    if (!preflight_advance_scratch(
            *impl_,
            impl_->next_global_batch_index,
            end_batch,
            requirements,
            preflight_failure)) {
      if (preflight_failure == ReplayFailure::source_forest_contradiction) {
        impl_->poisoned = true;
      }
      result.cumulative_counters = impl_->counters;
      result.session_poisoned = impl_->poisoned;
      result.decision = advance_decision(preflight_failure);
      return result;
    }
    reserve_advance_scratch(*impl_, requirements);

    const std::size_t prefix_before = impl_->next_global_batch_index;
    ReplayFailure replay_failure = ReplayFailure::none;
    try {
      while (impl_->next_global_batch_index < end_batch) {
        replay_failure =
            replay_batch(*impl_, impl_->next_global_batch_index);
        if (replay_failure != ReplayFailure::none) {
          break;
        }
      }
    } catch (const std::bad_alloc&) {
      replay_failure = ReplayFailure::allocation_failed;
    } catch (const std::length_error&) {
      replay_failure = ReplayFailure::capacity_overflow;
    } catch (const std::exception&) {
      replay_failure = ReplayFailure::source_forest_contradiction;
    }
    if (replay_failure != ReplayFailure::none) {
      result.locator_mutated_during_advance =
          impl_->locator.snapshot_stamp().committed_batch_count !=
          prefix_before;
      impl_->poisoned = true;
      result.session_poisoned = true;
      result.committed_global_batch_prefix_count =
          impl_->locator.snapshot_stamp().committed_batch_count;
      result.frozen_locator_stamp = impl_->locator.snapshot_stamp();
      result.cumulative_counters = impl_->counters;
      result.decision = advance_decision(replay_failure);
      return result;
    }

    if (impl_->next_global_batch_index ==
        impl_->source_forest->batches.size()) {
      ++impl_->counters.locator_stamp_equality_check_count;
      if (impl_->locator.snapshot_stamp() !=
          impl_->source_forest->final_locator_stamp) {
        impl_->poisoned = true;
        result.locator_mutated_during_advance =
            impl_->next_global_batch_index != prefix_before;
        result.session_poisoned = true;
        result.committed_global_batch_prefix_count =
            impl_->next_global_batch_index;
        result.frozen_locator_stamp = impl_->locator.snapshot_stamp();
        result.cumulative_counters = impl_->counters;
        result.decision =
            ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
                no_replay_locator_stamp_contradiction;
        return result;
      }
    }

    impl_->current_closed_squared_level.emplace(
        std::move(prepared_closed_squared_level));
    const auto frozen_stamp = impl_->locator.snapshot_stamp();
    result.committed_global_batch_prefix_count =
        impl_->next_global_batch_index;
    result.frozen_locator_stamp = frozen_stamp;
    result.source_forest_prefix_freshly_replayed = true;
    result.every_strict_pre_and_committed_stamp_matched = true;
    result.groups_before_same_level_births_replayed = true;
    result.locator_unions_then_bindings_committed_atomically = true;
    result.carrier_state_matches_locator_canonical_parents = true;
    result.locator_mutated_during_advance =
        impl_->next_global_batch_index != prefix_before;

    ++impl_->visit_epoch;
    impl_->view_live = true;
    const ExactDirectMorseForestCarrierCutReplayView view{
        *this, impl_->visit_epoch};
    consumer_started = true;
    result.synchronous_view_invoked = true;
    try {
      consumer(erased_consumer, view);
    } catch (...) {
      impl_->view_live = false;
      throw;
    }
    impl_->view_live = false;
    if (impl_->locator.snapshot_stamp() != frozen_stamp) {
      impl_->poisoned = true;
      result.session_poisoned = true;
      result.decision =
          ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
              no_replay_locator_stamp_contradiction;
      return result;
    }
    if (impl_->poisoned) {
      result.session_poisoned = true;
      result.cumulative_counters = impl_->counters;
      result.decision =
          ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
              no_replay_source_forest_contradiction;
      return result;
    }
    ++impl_->counters.synchronous_view_visit_count;
    result.cumulative_counters = impl_->counters;
    result.scope = ExactDirectMorseForestCarrierCutReplayScope::
        one_target_order_live_locator_and_carrier_to_optional_reduced_root_view_at_monotone_closed_exact_cuts_only;
    result.decision =
        ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
            complete_certified_forest_relative_closed_cut;
    return result;
  } catch (const std::bad_alloc&) {
    if (consumer_started) {
      throw;
    }
    result.locator_mutated_during_advance =
        impl_->locator.snapshot_stamp().committed_batch_count !=
        prefix_at_advance_entry;
    if (result.locator_mutated_during_advance) {
      impl_->poisoned = true;
    }
    result.cumulative_counters = impl_->counters;
    result.session_poisoned = impl_->poisoned;
    result.decision =
        ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
            no_replay_allocation_failed;
    return result;
  } catch (const std::length_error&) {
    if (consumer_started) {
      throw;
    }
    result.locator_mutated_during_advance =
        impl_->locator.snapshot_stamp().committed_batch_count !=
        prefix_at_advance_entry;
    if (result.locator_mutated_during_advance) {
      impl_->poisoned = true;
    }
    result.cumulative_counters = impl_->counters;
    result.session_poisoned = impl_->poisoned;
    result.decision =
        ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
            no_replay_capacity_overflow;
    return result;
  }
}

ExactDirectMorseForestCarrierCutReplaySession::FrozenLocatorViewDecision
ExactDirectMorseForestCarrierCutReplaySession::view_with_frozen_locator(
    std::uint64_t epoch,
    void* erased_consumer,
    FrozenLocatorViewConsumer consumer) const {
  if (impl_ == nullptr || !impl_->ready || erased_consumer == nullptr ||
      consumer == nullptr) {
    return FrozenLocatorViewDecision::session_not_ready;
  }
  if (impl_->poisoned) {
    return FrozenLocatorViewDecision::session_poisoned;
  }
  if (!impl_->view_live || impl_->visit_epoch != epoch ||
      !impl_->current_closed_squared_level.has_value()) {
    return FrozenLocatorViewDecision::stale_view_epoch;
  }
  const auto entry_stamp = impl_->locator.snapshot_stamp();
  bool consumer_certified_coupling = false;
  try {
    consumer_certified_coupling = consumer(
        erased_consumer,
        impl_->source_forest->point_count,
        impl_->target_order,
        *impl_->current_closed_squared_level,
        entry_stamp,
        impl_->locator);
  } catch (...) {
    if (impl_->locator.snapshot_stamp() != entry_stamp) {
      impl_->poisoned = true;
    }
    throw;
  }
  if (impl_->poisoned) {
    return FrozenLocatorViewDecision::session_poisoned;
  }
  if (!impl_->ready || !impl_->view_live || impl_->visit_epoch != epoch ||
      !impl_->current_closed_squared_level.has_value()) {
    return FrozenLocatorViewDecision::stale_view_epoch;
  }
  if (impl_->locator.snapshot_stamp() != entry_stamp) {
    impl_->poisoned = true;
    return FrozenLocatorViewDecision::locator_snapshot_changed;
  }
  if (!consumer_certified_coupling) {
    impl_->poisoned = true;
    return FrozenLocatorViewDecision::session_poisoned;
  }
  return FrozenLocatorViewDecision::complete_stable_snapshot;
}

bool ExactDirectMorseForestCarrierCutReplaySession::view_epoch_is_live(
    std::uint64_t epoch) const noexcept {
  return impl_ != nullptr && impl_->ready && !impl_->poisoned &&
         impl_->view_live && impl_->visit_epoch == epoch &&
         impl_->current_closed_squared_level.has_value();
}

bool ExactDirectMorseForestCarrierCutReplaySession::
    view_borrows_source_forest(
        std::uint64_t epoch,
        const ExactDirectMorseForestJournalResult* source_forest) const
    noexcept {
  return view_epoch_is_live(epoch) && source_forest != nullptr &&
         impl_->source_forest == source_forest;
}

exact::ExactLevel
ExactDirectMorseForestCarrierCutReplaySession::view_closed_squared_level(
    std::uint64_t epoch) const {
  if (!view_epoch_is_live(epoch)) {
    std::terminate();
  }
  return *impl_->current_closed_squared_level;
}

std::size_t ExactDirectMorseForestCarrierCutReplaySession::
    view_committed_batch_prefix_count(std::uint64_t epoch) const noexcept {
  if (!view_epoch_is_live(epoch)) {
    std::terminate();
  }
  return impl_->next_global_batch_index;
}

std::size_t ExactDirectMorseForestCarrierCutReplaySession::
    view_target_carrier_count(std::uint64_t epoch) const noexcept {
  if (!view_epoch_is_live(epoch)) {
    std::terminate();
  }
  return impl_->target_handles.size();
}

ExactDirectSparsePositiveFacetLocatorSnapshotStamp
ExactDirectMorseForestCarrierCutReplaySession::view_locator_snapshot_stamp(
    std::uint64_t epoch) const noexcept {
  if (!view_epoch_is_live(epoch)) {
    std::terminate();
  }
  return impl_->locator.snapshot_stamp();
}

ExactDirectSparsePositiveFacetProbeResult
ExactDirectMorseForestCarrierCutReplaySession::view_probe_positive_facet(
    std::uint64_t epoch,
    const ExactDirectSparseFacetKey& key,
    const ExactDirectSparseFacetWitness& witness,
    const ExactDirectSparsePositiveFacetProbeBudget& budget) const noexcept {
  if (!view_epoch_is_live(epoch)) {
    std::terminate();
  }
  return impl_->locator.probe_positive_facet(key, witness, budget);
}

ExactDirectMorseForestCarrierCutEntry
ExactDirectMorseForestCarrierCutReplaySession::view_entry_at(
    std::uint64_t epoch,
    std::size_t target_entry_index) const {
  if (!view_epoch_is_live(epoch) ||
      target_entry_index >= impl_->target_handles.size()) {
    throw std::out_of_range("a carrier-cut replay view entry is unavailable");
  }
  const ExactDirectSparseComponentHandle handle =
      impl_->target_handles[target_entry_index];
  ExactDirectMorseForestCarrierCutEntry entry;
  entry.entry_index = target_entry_index;
  entry.component_handle = handle;
  entry.birth_record_index = handle;
  if (!impl_->components[handle].active) {
    entry.disposition = ExactDirectMorseForestCarrierCutDisposition::
        inactive_at_closed_cut;
    return entry;
  }
  const auto root = find_component_root_const(*impl_, handle);
  if (!root.has_value() || *root >= impl_->components.size()) {
    throw std::logic_error(
        "a carrier-cut replay view found an invalid component parent");
  }
  const auto reduced_root =
      impl_->components[*root].reduced_root_node_id;
  if (!reduced_root.has_value()) {
    entry.disposition = ExactDirectMorseForestCarrierCutDisposition::
        active_latent_without_reduced_root;
    return entry;
  }
  if (*reduced_root >= impl_->node_states.size() ||
      impl_->node_states[*reduced_root].owner_component_handle !=
          std::optional<ExactDirectSparseComponentHandle>{*root}) {
    throw std::logic_error(
        "a carrier-cut replay view found an invalid reduced root owner");
  }
  const ExactDirectMorseForestJournalView forest_view{
      *impl_->source_forest};
  const auto node = forest_view.node_at(*reduced_root);
  if (node.order != impl_->target_order ||
      node.squared_level > *impl_->current_closed_squared_level) {
    throw std::logic_error(
        "a carrier-cut replay view found a future or wrong-order root");
  }
  entry.reduced_root_node_id = reduced_root;
  entry.disposition = ExactDirectMorseForestCarrierCutDisposition::
      resolved_reduced_root;
  return entry;
}

std::optional<ExactDirectMorseForestCarrierCutEntry>
ExactDirectMorseForestCarrierCutReplaySession::view_find_entry(
    std::uint64_t epoch,
    ExactDirectSparseComponentHandle component_handle) const {
  if (!view_epoch_is_live(epoch)) {
    throw std::logic_error("a carrier-cut replay view is no longer live");
  }
  const auto found = std::lower_bound(
      impl_->target_handles.begin(),
      impl_->target_handles.end(),
      component_handle);
  if (found == impl_->target_handles.end() ||
      *found != component_handle) {
    return std::nullopt;
  }
  return view_entry_at(
      epoch,
      static_cast<std::size_t>(found - impl_->target_handles.begin()));
}

ExactDirectMorseForestCarrierCutReplayView::
    ExactDirectMorseForestCarrierCutReplayView(
        const ExactDirectMorseForestCarrierCutReplaySession& session,
        std::uint64_t visit_epoch) noexcept
    : session_(&session), visit_epoch_(visit_epoch) {}

std::size_t ExactDirectMorseForestCarrierCutReplayView::target_order() const
    noexcept {
  if (session_ == nullptr ||
      !session_->view_epoch_is_live(visit_epoch_)) {
    std::terminate();
  }
  return session_->target_order();
}

exact::ExactLevel
ExactDirectMorseForestCarrierCutReplayView::closed_squared_level() const {
  return session_->view_closed_squared_level(visit_epoch_);
}

std::size_t ExactDirectMorseForestCarrierCutReplayView::
    committed_global_batch_prefix_count() const noexcept {
  return session_->view_committed_batch_prefix_count(visit_epoch_);
}

std::size_t ExactDirectMorseForestCarrierCutReplayView::
    target_carrier_count() const noexcept {
  return session_->view_target_carrier_count(visit_epoch_);
}

ExactDirectSparsePositiveFacetLocatorSnapshotStamp
ExactDirectMorseForestCarrierCutReplayView::locator_snapshot_stamp() const
    noexcept {
  return session_->view_locator_snapshot_stamp(visit_epoch_);
}

ExactDirectSparsePositiveFacetProbeResult
ExactDirectMorseForestCarrierCutReplayView::probe_positive_facet(
    const ExactDirectSparseFacetKey& key,
    const ExactDirectSparseFacetWitness& witness,
    const ExactDirectSparsePositiveFacetProbeBudget& budget) const noexcept {
  if (key.point_count != target_order()) {
    ExactDirectSparsePositiveFacetProbeResult rejected;
    rejected.budget = budget;
    rejected.query_key = key;
    rejected.query_witness = witness;
    rejected.decision = ExactDirectSparsePositiveFacetProbeDecision::
        no_positive_locator_input_shape_rejected;
    return rejected;
  }
  return session_->view_probe_positive_facet(
      visit_epoch_, key, witness, budget);
}

ExactDirectMorseForestCarrierCutEntry
ExactDirectMorseForestCarrierCutReplayView::entry_at(
    std::size_t target_entry_index) const {
  return session_->view_entry_at(visit_epoch_, target_entry_index);
}

std::optional<ExactDirectMorseForestCarrierCutEntry>
ExactDirectMorseForestCarrierCutReplayView::find_entry(
    ExactDirectSparseComponentHandle component_handle) const {
  return session_->view_find_entry(visit_epoch_, component_handle);
}

bool ExactDirectMorseForestCarrierCutReplayAdvanceResult::
    certified_forest_relative_closed_cut() const noexcept {
  return schema_version ==
             direct_morse_forest_carrier_cut_replay_session_schema_version &&
         target_order != 0U &&
         valid_nonnegative_level(requested_closed_squared_level) &&
         decision == ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
                         complete_certified_forest_relative_closed_cut &&
         source_forest_prefix_freshly_replayed &&
         every_strict_pre_and_committed_stamp_matched &&
         groups_before_same_level_births_replayed &&
         locator_unions_then_bindings_committed_atomically &&
         carrier_state_matches_locator_canonical_parents &&
         synchronous_view_invoked && !session_poisoned &&
         frozen_locator_stamp.schema_version ==
             direct_sparse_positive_facet_locator_schema_version &&
         frozen_locator_stamp.external_authority_id != 0U &&
         frozen_locator_stamp.committed_batch_count ==
             committed_global_batch_prefix_count &&
         scope == ExactDirectMorseForestCarrierCutReplayScope::
                      one_target_order_live_locator_and_carrier_to_optional_reduced_root_view_at_monotone_closed_exact_cuts_only &&
         advance_non_scope_honest(*this);
}

bool ExactDirectMorseForestCarrierCutReplayAdvanceResult::
    certified_nonmutating_rejection() const noexcept {
  const bool nonmutating_decision =
      decision == ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
                      no_replay_session_not_ready ||
      decision == ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
                      no_replay_invalid_closed_squared_level ||
      decision == ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
                      no_replay_nonmonotone_closed_cut ||
      decision == ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
                      no_replay_allocation_failed ||
      decision == ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
                      no_replay_budget_exhausted;
  return schema_version ==
             direct_morse_forest_carrier_cut_replay_session_schema_version &&
         nonmutating_decision && !locator_mutated_during_advance &&
         !synchronous_view_invoked &&
         scope == ExactDirectMorseForestCarrierCutReplayScope::unspecified &&
         advance_non_scope_honest(*this);
}

bool ExactDirectMorseForestCarrierCutReplayAdvanceResult::
    certified_atomic_failure() const noexcept {
  return schema_version ==
             direct_morse_forest_carrier_cut_replay_session_schema_version &&
         decision != ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
                         not_certified &&
         decision != ExactDirectMorseForestCarrierCutReplayAdvanceDecision::
                         complete_certified_forest_relative_closed_cut &&
         !synchronous_view_invoked &&
         scope == ExactDirectMorseForestCarrierCutReplayScope::unspecified &&
         advance_non_scope_honest(*this);
}

bool ExactDirectMorseForestCarrierCutReplaySessionInitialization::
    certified_ready_session() const noexcept {
  return schema_version ==
             direct_morse_forest_carrier_cut_replay_session_schema_version &&
         decision ==
             ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
                 complete_certified_replay_session_ready &&
         target_order != 0U && target_order <= effective_maximum_order &&
         source_forest_certified_outcome_accepted &&
         budget_preflight_certified && global_batches_strictly_ordered &&
         dense_component_and_node_state_initialized &&
         target_carrier_partition_reconstructed &&
         locator_initialized_from_trusted_recorded_budget &&
         initial_locator_stamp_matched && session != nullptr &&
         session->ready() && !session->poisoned() &&
         scope == ExactDirectMorseForestCarrierCutReplayScope::
                      one_target_order_live_locator_and_carrier_to_optional_reduced_root_view_at_monotone_closed_exact_cuts_only &&
         initialization_non_scope_honest(*this);
}

bool ExactDirectMorseForestCarrierCutReplaySessionInitialization::
    certified_atomic_failure() const noexcept {
  return schema_version ==
             direct_morse_forest_carrier_cut_replay_session_schema_version &&
         decision !=
             ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
                 not_certified &&
         decision !=
             ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
                 complete_certified_replay_session_ready &&
         session == nullptr &&
         scope == ExactDirectMorseForestCarrierCutReplayScope::unspecified &&
         initialization_non_scope_honest(*this);
}

ExactDirectMorseForestCarrierCutReplaySession::
    ExactDirectMorseForestCarrierCutReplaySession(
        std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

ExactDirectMorseForestCarrierCutReplaySession::
    ~ExactDirectMorseForestCarrierCutReplaySession() = default;

bool ExactDirectMorseForestCarrierCutReplaySession::ready() const noexcept {
  return impl_ != nullptr && impl_->ready && !impl_->poisoned;
}

bool ExactDirectMorseForestCarrierCutReplaySession::poisoned() const
    noexcept {
  return impl_ != nullptr && impl_->poisoned;
}

bool ExactDirectMorseForestCarrierCutReplaySession::has_frozen_cut() const
    noexcept {
  return ready() && impl_->current_closed_squared_level.has_value();
}

std::size_t ExactDirectMorseForestCarrierCutReplaySession::target_order()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->target_order;
}

ExactDirectMorseForestCarrierCutReplaySessionInitialization
build_exact_direct_morse_forest_carrier_cut_replay_session(
    const ExactDirectMorseForestJournalResult& source_forest,
    std::size_t target_order,
    const ExactDirectMorseForestCarrierCutReplaySessionBudget& budget) {
  ExactDirectMorseForestCarrierCutReplaySessionInitialization result;
  result.requested_budget = budget;
  result.point_count = source_forest.point_count;
  result.effective_maximum_order =
      source_forest.effective_maximum_order;
  result.target_order = target_order;

  if (target_order == 0U ||
      target_order > source_forest.effective_maximum_order) {
    return initialization_failure(
        std::move(result),
        ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
            no_replay_session_invalid_target_order);
  }
  if (!source_forest.certified_conditional_h0_candidate()) {
    return initialization_failure(
        std::move(result),
        ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
            no_replay_session_source_forest_rejected);
  }
  result.source_forest_certified_outcome_accepted = true;
  if (budget.locator_budget !=
      source_forest.requested_budget.locator_budget) {
    return initialization_failure(
        std::move(result),
        ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
            no_replay_session_locator_budget_mismatch);
  }

  std::size_t birth_record_count = 0U;
  std::size_t node_count = 0U;
  if (!checked_add(
          source_forest.implicit_order_one_prefix_count,
          source_forest.birth_records.size(),
          birth_record_count) ||
      !checked_add(
          source_forest.implicit_order_one_prefix_count,
          source_forest.nodes.size(),
          node_count)) {
    return initialization_failure(
        std::move(result),
        ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
            no_replay_session_capacity_overflow);
  }
  const auto& state_budget = budget.carrier_state_budget;
  const std::size_t replayable_global_batch_count =
      static_cast<std::size_t>(std::count_if(
          source_forest.batches.begin(),
          source_forest.batches.end(),
          [target_order](const ExactDirectMorseForestBatch& batch) {
            return batch.order <= target_order;
          }));
  if (birth_record_count >
          state_budget.maximum_forest_birth_record_scan_count ||
      node_count > state_budget.maximum_forest_node_scan_count ||
      source_forest.batches.size() >
          state_budget.maximum_forest_batch_scan_count ||
      source_forest.atomic_groups.size() >
          state_budget.maximum_forest_atomic_group_scan_count ||
      source_forest.saddle_records.size() >
          state_budget.maximum_forest_saddle_scan_count ||
      source_forest.arm_root_bindings.size() >
          state_budget.maximum_forest_arm_binding_scan_count ||
      source_forest.child_node_ids.size() >
          state_budget.maximum_forest_child_reference_scan_count ||
      source_forest.final_roots.size() >
          state_budget.maximum_forest_final_root_scan_count ||
      birth_record_count > state_budget.maximum_component_state_count ||
      node_count > state_budget.maximum_node_marker_state_count ||
      replayable_global_batch_count >
          budget.maximum_replayed_global_batch_count ||
      birth_record_count > budget.locator_budget.maximum_component_handle_count) {
    return initialization_failure(
        std::move(result),
        ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
            no_replay_session_budget_exhausted);
  }
  result.budget_preflight_certified = true;

  try {
    auto impl = std::make_unique<
        ExactDirectMorseForestCarrierCutReplaySession::Impl>();
    impl->source_forest = &source_forest;
    impl->budget = budget;
    impl->target_order = target_order;
    impl->birth_record_count = birth_record_count;
    impl->node_count = node_count;
    impl->components.resize(birth_record_count);
    impl->node_states.resize(node_count);
    impl->active_carrier_root_counts_by_order.resize(
        source_forest.effective_maximum_order + 1U);
    impl->active_reduced_root_counts_by_order.resize(
        source_forest.effective_maximum_order + 1U);
    impl->target_handles.reserve(std::min(
        birth_record_count, state_budget.maximum_index_entry_count));

    const ExactDirectMorseForestJournalView forest_view{source_forest};
    for (std::size_t birth_index = 0U;
         birth_index < birth_record_count;
         ++birth_index) {
      const auto birth = forest_view.birth_record_at(birth_index);
      ++impl->counters.forest_birth_record_scan_count;
      if (birth.birth_record_index != birth_index ||
          birth.component_handle != birth_index || birth.order == 0U ||
          birth.order > source_forest.effective_maximum_order ||
          birth.source_journal_batch_index >= source_forest.batches.size() ||
          source_forest.batches[birth.source_journal_batch_index].order !=
              birth.order ||
          !canonical_key_shape(
              birth.facet_key, source_forest.point_count, birth.order)) {
        return initialization_failure(
            std::move(result),
            ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
                no_replay_session_source_forest_contradiction);
      }
      const auto token = replay_token(birth_index, 1U);
      if (!token.has_value()) {
        return initialization_failure(
            std::move(result),
            ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
                no_replay_session_capacity_overflow);
      }
      if (birth.binding_witness != ExactDirectSparseFacetWitness{
              source_forest.config.locator_config.external_authority_id,
              *token} ||
          (birth.order == 1U) !=
              birth.order_one_birth_node_id.has_value()) {
        return initialization_failure(
            std::move(result),
            ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
                no_replay_session_source_forest_contradiction);
      }
      impl->components[birth_index].parent = birth_index;
      impl->components[birth_index].order = birth.order;
      if (birth.order == target_order) {
        if (impl->target_handles.size() >=
                state_budget.maximum_index_entry_count ||
            impl->target_handles.size() >=
                state_budget.maximum_logical_output_entry_count) {
          return initialization_failure(
              std::move(result),
              ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
                  no_replay_session_budget_exhausted);
        }
        impl->target_handles.push_back(birth_index);
      }
    }
    impl->counters.target_carrier_count = impl->target_handles.size();

    for (std::size_t node_index = 0U; node_index < node_count;
         ++node_index) {
      const auto node = forest_view.node_at(
          static_cast<ExactDirectMorseForestNodeId>(node_index));
      ++impl->counters.forest_node_scan_count;
      if (node.node_id !=
              static_cast<ExactDirectMorseForestNodeId>(node_index) ||
          node.order == 0U ||
          node.order > source_forest.effective_maximum_order ||
          !valid_nonnegative_level(node.squared_level)) {
        return initialization_failure(
            std::move(result),
            ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
                no_replay_session_source_forest_contradiction);
      }
      if (!measure_level(*impl, node.squared_level)) {
        return initialization_failure(
            std::move(result),
            ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
                no_replay_session_budget_exhausted);
      }
    }

    for (std::size_t batch_index = 0U;
         batch_index < source_forest.batches.size();
         ++batch_index) {
      const auto& batch = source_forest.batches[batch_index];
      ++impl->counters.forest_batch_preflight_scan_count;
      if (batch.batch_index != batch_index ||
          batch.source_journal_batch_index != batch_index ||
          batch.order == 0U ||
          batch.order > source_forest.effective_maximum_order ||
          !valid_nonnegative_level(batch.squared_level) ||
          batch.strict_pre_batch_stamp.schema_version !=
              direct_sparse_positive_facet_locator_schema_version ||
          batch.committed_batch_stamp.schema_version !=
              direct_sparse_positive_facet_locator_schema_version ||
          batch.strict_pre_batch_stamp.external_authority_id !=
              source_forest.config.locator_config.external_authority_id ||
          batch.committed_batch_stamp.external_authority_id !=
              source_forest.config.locator_config.external_authority_id ||
          batch.strict_pre_batch_stamp.committed_batch_count != batch_index ||
          batch.committed_batch_stamp.committed_batch_count !=
              batch_index + 1U ||
          !batch.strict_arms_resolved_before_mutation ||
          !batch.quotient_resolved_before_mutation ||
          !batch.unions_then_births_committed_atomically) {
        return initialization_failure(
            std::move(result),
            ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
                no_replay_session_source_forest_contradiction);
      }
      if (!measure_level(*impl, batch.squared_level)) {
        return initialization_failure(
            std::move(result),
            ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
                no_replay_session_budget_exhausted);
      }
      if (batch_index != 0U) {
        const auto& prior = source_forest.batches[batch_index - 1U];
        if (prior.order > batch.order) {
          return initialization_failure(
              std::move(result),
              ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
                  no_replay_session_source_forest_contradiction);
        }
        if (prior.order == batch.order) {
          const auto relation = compare_levels(
              *impl, prior.squared_level, batch.squared_level);
          if (!relation.has_value()) {
            return initialization_failure(
                std::move(result),
                ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
                    no_replay_session_budget_exhausted);
          }
          if (*relation >= 0) {
            return initialization_failure(
                std::move(result),
                ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
                    no_replay_session_source_forest_contradiction);
          }
        }
      }
    }
    if (source_forest.batches.empty() ||
        !singleton_batch_shape(*impl, source_forest.batches.front())) {
      return initialization_failure(
          std::move(result),
          ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
              no_replay_session_source_forest_contradiction);
    }
    result.global_batches_strictly_ordered = true;
    result.dense_component_and_node_state_initialized = true;
    result.target_carrier_partition_reconstructed = true;

    impl->locator = build_exact_direct_sparse_positive_facet_locator(
        birth_record_count,
        budget.locator_budget,
        source_forest.config.locator_config);
    if (!impl->locator.certified_positive_locator()) {
      return initialization_failure(
          std::move(result),
          ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
              no_replay_session_locator_initialization_rejected);
    }
    result.locator_initialized_from_trusted_recorded_budget = true;
    ++impl->counters.locator_stamp_equality_check_count;
    if (impl->locator.snapshot_stamp() !=
        source_forest.batches.front().strict_pre_batch_stamp) {
      return initialization_failure(
          std::move(result),
          ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
              no_replay_session_initial_locator_stamp_contradiction);
    }
    result.initial_locator_stamp_matched = true;
    impl->ready = true;
    result.counters = impl->counters;
    result.scope = ExactDirectMorseForestCarrierCutReplayScope::
        one_target_order_live_locator_and_carrier_to_optional_reduced_root_view_at_monotone_closed_exact_cuts_only;
    result.decision =
        ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
            complete_certified_replay_session_ready;
    result.session = std::unique_ptr<
        ExactDirectMorseForestCarrierCutReplaySession>(
        new ExactDirectMorseForestCarrierCutReplaySession(
            std::move(impl)));
    if (!result.certified_ready_session()) {
      return initialization_failure(
          std::move(result),
          ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
              no_replay_session_source_forest_contradiction);
    }
    return result;
  } catch (const std::bad_alloc&) {
    return initialization_failure(
        std::move(result),
        ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
            no_replay_session_allocation_failed);
  } catch (const std::length_error&) {
    return initialization_failure(
        std::move(result),
        ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
            no_replay_session_capacity_overflow);
  } catch (const std::exception&) {
    return initialization_failure(
        std::move(result),
        ExactDirectMorseForestCarrierCutReplaySessionInitializationDecision::
            no_replay_session_source_forest_contradiction);
  }
}

namespace {

[[nodiscard]] bool measure_level(
    ExactDirectMorseForestCarrierCutReplaySession::Impl& impl,
    const exact::ExactLevel& level) {
  if (!valid_nonnegative_level(level)) {
    return false;
  }
  const std::size_t observed = std::max(
      positive_integer_bit_count(level.numerator()),
      positive_integer_bit_count(level.denominator()));
  if (observed > impl.budget.carrier_state_budget
                     .maximum_single_exact_level_integer_bit_count) {
    return false;
  }
  impl.counters.maximum_observed_exact_level_integer_bit_count =
      std::max(
          impl.counters.maximum_observed_exact_level_integer_bit_count,
          observed);
  return true;
}

[[nodiscard]] std::optional<int> compare_levels(
    ExactDirectMorseForestCarrierCutReplaySession::Impl& impl,
    const exact::ExactLevel& left,
    const exact::ExactLevel& right) {
  if (impl.counters.exact_level_comparison_count >=
      impl.budget.carrier_state_budget
          .maximum_exact_level_comparison_count) {
    return std::nullopt;
  }
  ++impl.counters.exact_level_comparison_count;
  return left < right ? -1 : (right < left ? 1 : 0);
}

[[nodiscard]] std::optional<std::size_t> find_component_root(
    ExactDirectMorseForestCarrierCutReplaySession::Impl& impl,
    ExactDirectSparseComponentHandle handle) noexcept {
  if (handle >= impl.components.size() ||
      !impl.components[handle].active) {
    return std::nullopt;
  }
  while (impl.components[handle].parent != handle) {
    if (impl.counters.component_parent_hop_count >=
        impl.budget.carrier_state_budget.maximum_parent_hop_count) {
      return std::nullopt;
    }
    ++impl.counters.component_parent_hop_count;
    const std::size_t next = impl.components[handle].parent;
    if (next >= impl.components.size() ||
        !impl.components[next].active) {
      return std::nullopt;
    }
    handle = next;
  }
  return handle;
}

[[nodiscard]] std::optional<std::size_t> find_locator_root(
    ExactDirectMorseForestCarrierCutReplaySession::Impl& impl,
    ExactDirectSparseComponentHandle handle) noexcept {
  const auto& parents = impl.locator.component_parents();
  if (handle >= parents.size()) {
    return std::nullopt;
  }
  while (parents[handle] != handle) {
    if (impl.counters.component_parent_hop_count >=
        impl.budget.carrier_state_budget.maximum_parent_hop_count) {
      return std::nullopt;
    }
    ++impl.counters.component_parent_hop_count;
    if (parents[handle] >= parents.size()) {
      return std::nullopt;
    }
    handle = parents[handle];
  }
  return handle;
}

[[nodiscard]] std::optional<std::size_t> find_component_root_const(
    const ExactDirectMorseForestCarrierCutReplaySession::Impl& impl,
    ExactDirectSparseComponentHandle handle) noexcept {
  if (handle >= impl.components.size() ||
      !impl.components[handle].active) {
    return std::nullopt;
  }
  std::size_t hop_count = 0U;
  while (impl.components[handle].parent != handle) {
    if (hop_count >= impl.components.size()) {
      return std::nullopt;
    }
    const std::size_t next = impl.components[handle].parent;
    if (next >= impl.components.size() ||
        !impl.components[next].active) {
      return std::nullopt;
    }
    handle = next;
    ++hop_count;
  }
  return handle;
}

[[nodiscard]] bool add_counter(
    std::size_t increment,
    std::size_t& counter) noexcept {
  return checked_add(counter, increment, counter);
}

[[nodiscard]] bool singleton_batch_shape(
    const ExactDirectMorseForestCarrierCutReplaySession::Impl& impl,
    const ExactDirectMorseForestBatch& batch) noexcept {
  return batch.batch_index == 0U && batch.order == 1U &&
         batch.squared_level == exact::ExactLevel{} &&
         batch.birth_record_offset == 0U &&
         batch.birth_record_count == impl.source_forest->point_count &&
         batch.birth_record_count ==
             impl.source_forest->implicit_order_one_prefix_count &&
         batch.saddle_record_count == 0U &&
         batch.atomic_group_count == 0U &&
         batch.strict_pre_batch_carrier_count == 0U &&
         batch.strict_pre_batch_reduced_root_count == 0U;
}

[[nodiscard]] bool preflight_advance_scratch(
    ExactDirectMorseForestCarrierCutReplaySession::Impl& impl,
    std::size_t begin_batch,
    std::size_t end_batch,
    AdvanceScratchRequirements& requirements,
    ReplayFailure& failure) {
  const auto& forest = *impl.source_forest;
  const auto& session_budget = impl.budget;
  std::size_t replayed_after = 0U;
  if (!checked_add(
          impl.counters.replayed_global_batch_count,
          end_batch - begin_batch,
          replayed_after) ||
      replayed_after > session_budget.maximum_replayed_global_batch_count) {
    failure = ReplayFailure::budget_exhausted;
    return false;
  }

  std::size_t total_union_count = 0U;
  std::size_t total_binding_count = 0U;
  for (std::size_t batch_index = begin_batch;
       batch_index < end_batch;
       ++batch_index) {
    const auto& batch = forest.batches[batch_index];
    if (batch.batch_index != batch_index || batch.order == 0U ||
        batch.order > forest.effective_maximum_order ||
        batch.atomic_group_offset > forest.atomic_groups.size() ||
        batch.atomic_group_count >
            forest.atomic_groups.size() - batch.atomic_group_offset ||
        batch.saddle_record_offset > forest.saddle_records.size() ||
        batch.saddle_record_count >
            forest.saddle_records.size() - batch.saddle_record_offset ||
        batch.birth_record_offset > impl.birth_record_count ||
        batch.birth_record_count >
            impl.birth_record_count - batch.birth_record_offset) {
      failure = ReplayFailure::source_forest_contradiction;
      return false;
    }

    std::size_t batch_carrier_reference_count = 0U;
    std::size_t batch_prior_root_reference_count = 0U;
    std::size_t batch_union_count = 0U;
    for (std::size_t local_group = 0U;
         local_group < batch.atomic_group_count;
         ++local_group) {
      const std::size_t group_index =
          batch.atomic_group_offset + local_group;
      const auto& group = forest.atomic_groups[group_index];
      if (group.atomic_group_index != group_index ||
          group.batch_index != batch_index ||
          group.frozen_carrier_count == 0U ||
          group.frozen_carrier_count >
              session_budget.carrier_state_budget
                  .maximum_group_carrier_scratch_count ||
          group.prior_reduced_root_count >
              session_budget.carrier_state_budget
                  .maximum_group_prior_root_scratch_count ||
          !checked_add(
              batch_carrier_reference_count,
              group.frozen_carrier_count,
              batch_carrier_reference_count) ||
          !checked_add(
              batch_prior_root_reference_count,
              group.prior_reduced_root_count,
              batch_prior_root_reference_count) ||
          !checked_add(
              batch_union_count,
              group.frozen_carrier_count - 1U,
              batch_union_count)) {
        failure =
            group.frozen_carrier_count >
                    session_budget.carrier_state_budget
                        .maximum_group_carrier_scratch_count ||
                group.prior_reduced_root_count >
                    session_budget.carrier_state_budget
                        .maximum_group_prior_root_scratch_count
            ? ReplayFailure::budget_exhausted
            : ReplayFailure::source_forest_contradiction;
        return false;
      }
    }

    const std::size_t binding_scratch_count =
        singleton_batch_shape(impl, batch) ? 0U
                                           : batch.birth_record_count;
    if (batch.atomic_group_count >
            session_budget.maximum_batch_group_plan_count ||
        batch_carrier_reference_count >
            session_budget
                .maximum_batch_group_carrier_reference_count ||
        batch_prior_root_reference_count >
            session_budget
                .maximum_batch_group_prior_root_reference_count ||
        batch_union_count >
            session_budget.maximum_batch_locator_union_scratch_count ||
        binding_scratch_count >
            session_budget.maximum_batch_locator_binding_scratch_count ||
        batch_union_count >
            session_budget.locator_budget.maximum_batch_union_count ||
        binding_scratch_count >
            session_budget.locator_budget.maximum_batch_binding_count) {
      failure = ReplayFailure::budget_exhausted;
      return false;
    }
    requirements.maximum_group_plan_count = std::max(
        requirements.maximum_group_plan_count,
        batch.atomic_group_count);
    requirements.maximum_group_carrier_reference_count = std::max(
        requirements.maximum_group_carrier_reference_count,
        batch_carrier_reference_count);
    requirements.maximum_group_prior_root_reference_count = std::max(
        requirements.maximum_group_prior_root_reference_count,
        batch_prior_root_reference_count);
    requirements.maximum_locator_union_count = std::max(
        requirements.maximum_locator_union_count, batch_union_count);
    requirements.maximum_locator_binding_count = std::max(
        requirements.maximum_locator_binding_count,
        binding_scratch_count);
    if (!checked_add(
            total_union_count,
            batch_union_count,
            total_union_count) ||
        !checked_add(
            total_binding_count,
            batch.birth_record_count,
            total_binding_count)) {
      failure = ReplayFailure::capacity_overflow;
      return false;
    }
  }

  std::size_t union_after = 0U;
  std::size_t binding_after = 0U;
  if (!checked_add(
          impl.active_locator_union_count,
          total_union_count,
          union_after) ||
      !checked_add(
          impl.active_locator_binding_count,
          total_binding_count,
          binding_after)) {
    failure = ReplayFailure::capacity_overflow;
    return false;
  }
  if (union_after >
          session_budget.maximum_replayed_locator_union_count ||
      binding_after >
          session_budget.maximum_replayed_locator_binding_count) {
    failure = ReplayFailure::budget_exhausted;
    return false;
  }
  return true;
}

void reserve_advance_scratch(
    ExactDirectMorseForestCarrierCutReplaySession::Impl& impl,
    const AdvanceScratchRequirements& requirements) {
  impl.scratch_group_plans.reserve(
      requirements.maximum_group_plan_count);
  impl.scratch_group_carriers.reserve(
      requirements.maximum_group_carrier_reference_count);
  impl.scratch_group_prior_roots.reserve(
      requirements.maximum_group_prior_root_reference_count);
  impl.scratch_locator_unions.reserve(
      requirements.maximum_locator_union_count);
  impl.scratch_locator_bindings.reserve(
      requirements.maximum_locator_binding_count);
}

[[nodiscard]] bool validate_birth_record(
    const ExactDirectMorseForestCarrierCutReplaySession::Impl& impl,
    const ExactDirectMorseForestBatch& batch,
    const ExactDirectMorseForestBirthRecord& birth,
    std::size_t birth_index) noexcept {
  const auto token = replay_token(birth_index, 1U);
  return token.has_value() && birth.birth_record_index == birth_index &&
         birth.component_handle == birth_index &&
         birth.source_journal_batch_index ==
             batch.source_journal_batch_index &&
         birth.order == batch.order &&
         canonical_key_shape(
             birth.facet_key,
             impl.source_forest->point_count,
             batch.order) &&
         birth.binding_witness == ExactDirectSparseFacetWitness{
             impl.source_forest->config.locator_config
                 .external_authority_id,
             *token} &&
         (birth.order == 1U) ==
             birth.order_one_birth_node_id.has_value();
}

[[nodiscard]] ReplayFailure prepare_batch(
    ExactDirectMorseForestCarrierCutReplaySession::Impl& impl,
    std::size_t batch_index) {
  const auto& forest = *impl.source_forest;
  const auto& batch = forest.batches[batch_index];
  const ExactDirectMorseForestJournalView forest_view{forest};
  impl.scratch_group_plans.clear();
  impl.scratch_group_carriers.clear();
  impl.scratch_group_prior_roots.clear();
  impl.scratch_locator_unions.clear();
  impl.scratch_locator_bindings.clear();

  ++impl.counters.locator_stamp_equality_check_count;
  const auto strict_stamp = impl.locator.snapshot_stamp();
  if (strict_stamp != batch.strict_pre_batch_stamp ||
      strict_stamp.committed_batch_count != batch_index ||
      strict_stamp.component_union_count !=
          impl.active_locator_union_count ||
      strict_stamp.binding_count != impl.active_locator_binding_count) {
    return ReplayFailure::locator_stamp_contradiction;
  }
  if (batch.strict_pre_batch_carrier_count !=
          impl.active_carrier_root_counts_by_order[batch.order] ||
      batch.strict_pre_batch_reduced_root_count !=
          impl.active_reduced_root_counts_by_order[batch.order]) {
    return ReplayFailure::source_forest_contradiction;
  }

  if (singleton_batch_shape(impl, batch)) {
    for (std::size_t local_birth = 0U;
         local_birth < batch.birth_record_count;
         ++local_birth) {
      const std::size_t birth_index =
          batch.birth_record_offset + local_birth;
      const auto birth = forest_view.birth_record_at(birth_index);
      if (!validate_birth_record(impl, batch, birth, birth_index) ||
          birth.source_event_projection_index != birth_index ||
          birth.facet_key.point_count != 1U ||
          birth.facet_key.point_ids[0U] != birth_index ||
          birth.order_one_birth_node_id !=
              std::optional<ExactDirectMorseForestNodeId>{birth_index}) {
        return ReplayFailure::source_forest_contradiction;
      }
    }
    return ReplayFailure::none;
  }
  if (batch_index == 0U) {
    return ReplayFailure::source_forest_contradiction;
  }

  for (std::size_t local_group = 0U;
       local_group < batch.atomic_group_count;
       ++local_group) {
    const std::size_t group_index =
        batch.atomic_group_offset + local_group;
    const auto& group = forest.atomic_groups[group_index];
    if (group_index == std::numeric_limits<std::size_t>::max() ||
        group.atomic_group_index != group_index ||
        group.batch_index != batch_index ||
        group.saddle_record_count == 0U ||
        group.saddle_record_offset < batch.saddle_record_offset ||
        group.saddle_record_offset > forest.saddle_records.size() ||
        group.saddle_record_count >
            forest.saddle_records.size() -
                group.saddle_record_offset ||
        group.saddle_record_offset + group.saddle_record_count >
            batch.saddle_record_offset + batch.saddle_record_count) {
      return ReplayFailure::source_forest_contradiction;
    }

    BatchGroupPlan plan;
    plan.group_index = group_index;
    plan.carrier_offset = impl.scratch_group_carriers.size();
    plan.prior_root_offset = impl.scratch_group_prior_roots.size();
    const std::size_t group_marker = group_index + 1U;
    std::size_t latent_carrier_count = 0U;

    for (std::size_t local_saddle = 0U;
         local_saddle < group.saddle_record_count;
         ++local_saddle) {
      const std::size_t saddle_index =
          group.saddle_record_offset + local_saddle;
      const auto& saddle = forest.saddle_records[saddle_index];
      if (impl.counters.replayed_saddle_count >=
          impl.budget.carrier_state_budget
              .maximum_forest_saddle_scan_count) {
        return ReplayFailure::budget_exhausted;
      }
      ++impl.counters.replayed_saddle_count;
      if (saddle.saddle_record_index != saddle_index ||
          saddle.atomic_group_index != group_index ||
          saddle.source_journal_batch_index !=
              batch.source_journal_batch_index ||
          saddle.arm_binding_count == 0U ||
          saddle.arm_binding_count > 4U ||
          saddle.arm_binding_offset > forest.arm_root_bindings.size() ||
          saddle.arm_binding_count >
              forest.arm_root_bindings.size() -
                  saddle.arm_binding_offset) {
        return ReplayFailure::source_forest_contradiction;
      }

      std::array<ExactDirectSparseComponentHandle, 4U>
          saddle_carriers{};
      std::array<ExactDirectMorseForestNodeId, 4U> saddle_roots{};
      std::size_t saddle_carrier_count = 0U;
      std::size_t saddle_root_count = 0U;
      for (std::size_t local_binding = 0U;
           local_binding < saddle.arm_binding_count;
           ++local_binding) {
        const std::size_t binding_index =
            saddle.arm_binding_offset + local_binding;
        const auto& binding = forest.arm_root_bindings[binding_index];
        if (impl.counters.replayed_arm_binding_count >=
            impl.budget.carrier_state_budget
                .maximum_forest_arm_binding_scan_count) {
          return ReplayFailure::budget_exhausted;
        }
        ++impl.counters.replayed_arm_binding_count;
        const std::size_t handle =
            binding.frozen_carrier_component_handle;
        if (binding.binding_index != binding_index ||
            binding.source_family_index != saddle.source_family_index ||
            handle >= impl.components.size() ||
            impl.components[handle].order != batch.order ||
            !impl.components[handle].active) {
          return ReplayFailure::source_forest_contradiction;
        }
        const auto root = find_component_root(impl, handle);
        if (!root.has_value()) {
          return impl.counters.component_parent_hop_count >=
                         impl.budget.carrier_state_budget
                             .maximum_parent_hop_count
                     ? ReplayFailure::budget_exhausted
                     : ReplayFailure::source_forest_contradiction;
        }
        if (*root != handle ||
            binding.prior_reduced_root_node_id !=
                impl.components[handle].reduced_root_node_id) {
          return ReplayFailure::source_forest_contradiction;
        }

        bool saddle_carrier_seen = false;
        for (std::size_t prior = 0U;
             prior < saddle_carrier_count;
             ++prior) {
          saddle_carrier_seen =
              saddle_carrier_seen || saddle_carriers[prior] == handle;
        }
        if (!saddle_carrier_seen) {
          saddle_carriers[saddle_carrier_count] = handle;
          ++saddle_carrier_count;
        }

        if (impl.components[handle].last_group_marker != group_marker) {
          const std::size_t prior_marker =
              impl.components[handle].last_group_marker;
          if (prior_marker != 0U) {
            const std::size_t prior_group_index = prior_marker - 1U;
            if (prior_group_index >= forest.atomic_groups.size() ||
                forest.atomic_groups[prior_group_index].batch_index ==
                    batch_index) {
              return ReplayFailure::source_forest_contradiction;
            }
          }
          if (impl.scratch_group_carriers.size() -
                  plan.carrier_offset >=
              group.frozen_carrier_count) {
            return ReplayFailure::source_forest_contradiction;
          }
          impl.components[handle].last_group_marker = group_marker;
          impl.scratch_group_carriers.push_back(handle);
          if (!impl.components[handle].reduced_root_node_id.has_value()) {
            ++latent_carrier_count;
          }
        }

        if (binding.prior_reduced_root_node_id.has_value()) {
          const ExactDirectMorseForestNodeId prior_root =
              *binding.prior_reduced_root_node_id;
          if (prior_root >= impl.node_states.size() ||
              impl.node_states[prior_root].owner_component_handle !=
                  std::optional<ExactDirectSparseComponentHandle>{
                      handle}) {
            return ReplayFailure::source_forest_contradiction;
          }
          bool saddle_root_seen = false;
          for (std::size_t prior = 0U;
               prior < saddle_root_count;
               ++prior) {
            saddle_root_seen =
                saddle_root_seen || saddle_roots[prior] == prior_root;
          }
          if (!saddle_root_seen) {
            saddle_roots[saddle_root_count] = prior_root;
            ++saddle_root_count;
          }
          if (impl.node_states[prior_root].last_group_marker !=
              group_marker) {
            if (impl.scratch_group_prior_roots.size() -
                    plan.prior_root_offset >=
                group.prior_reduced_root_count) {
              return ReplayFailure::source_forest_contradiction;
            }
            impl.node_states[prior_root].last_group_marker = group_marker;
            impl.scratch_group_prior_roots.push_back(prior_root);
          }
        }
      }
      if (saddle.distinct_frozen_carrier_count !=
              saddle_carrier_count ||
          saddle.distinct_prior_reduced_root_count != saddle_root_count ||
          saddle.distinct_latent_carrier_count !=
              saddle_carrier_count - saddle_root_count) {
        return ReplayFailure::source_forest_contradiction;
      }
    }

    plan.carrier_count =
        impl.scratch_group_carriers.size() - plan.carrier_offset;
    plan.prior_root_count =
        impl.scratch_group_prior_roots.size() - plan.prior_root_offset;
    auto carrier_begin = impl.scratch_group_carriers.begin() +
        static_cast<std::ptrdiff_t>(plan.carrier_offset);
    auto carrier_end = carrier_begin +
        static_cast<std::ptrdiff_t>(plan.carrier_count);
    auto prior_root_begin = impl.scratch_group_prior_roots.begin() +
        static_cast<std::ptrdiff_t>(plan.prior_root_offset);
    auto prior_root_end = prior_root_begin +
        static_cast<std::ptrdiff_t>(plan.prior_root_count);
    std::sort(carrier_begin, carrier_end);
    std::sort(prior_root_begin, prior_root_end);
    if (plan.carrier_count == 0U ||
        group.frozen_carrier_count != plan.carrier_count ||
        group.latent_carrier_count != latent_carrier_count ||
        group.prior_reduced_root_count != plan.prior_root_count ||
        group.latent_carrier_count !=
            group.frozen_carrier_count -
                group.prior_reduced_root_count ||
        std::adjacent_find(carrier_begin, carrier_end) != carrier_end ||
        std::adjacent_find(prior_root_begin, prior_root_end) !=
            prior_root_end) {
      return ReplayFailure::source_forest_contradiction;
    }

    for (auto cursor = prior_root_begin; cursor != prior_root_end;
         ++cursor) {
      const auto prior_node = forest_view.node_at(*cursor);
      if (prior_node.order != batch.order) {
        return ReplayFailure::source_forest_contradiction;
      }
      const auto relation =
          compare_levels(impl, prior_node.squared_level, batch.squared_level);
      if (!relation.has_value()) {
        return ReplayFailure::budget_exhausted;
      }
      if (*relation >= 0) {
        return ReplayFailure::source_forest_contradiction;
      }
    }

    const auto validate_created_node =
        [&](ExactDirectMorseForestNodeKind expected_kind)
        -> ReplayFailure {
      if (!group.created_node_id.has_value() ||
          *group.created_node_id != group.resulting_root_node_id ||
          group.resulting_root_node_id >= impl.node_count) {
        return ReplayFailure::source_forest_contradiction;
      }
      const auto created =
          forest_view.node_at(group.resulting_root_node_id);
      if (created.order != batch.order || created.kind != expected_kind ||
          created.atomic_group_index !=
              std::optional<std::size_t>{group_index} ||
          created.child_offset != group.child_offset ||
          created.child_count != group.child_count) {
        return ReplayFailure::source_forest_contradiction;
      }
      const auto relation =
          compare_levels(impl, created.squared_level, batch.squared_level);
      if (!relation.has_value()) {
        return ReplayFailure::budget_exhausted;
      }
      return *relation == 0 ? ReplayFailure::none
                            : ReplayFailure::source_forest_contradiction;
    };

    switch (group.kind) {
      case ExactDirectMorseForestAtomicGroupKind::reduced_birth: {
        const ReplayFailure node_failure = validate_created_node(
            ExactDirectMorseForestNodeKind::reduced_birth);
        if (plan.prior_root_count != 0U || latent_carrier_count == 0U ||
            group.child_count != 0U ||
            node_failure != ReplayFailure::none) {
          return node_failure == ReplayFailure::budget_exhausted
                     ? node_failure
                     : ReplayFailure::source_forest_contradiction;
        }
        break;
      }
      case ExactDirectMorseForestAtomicGroupKind::continuation:
        if (plan.prior_root_count != 1U || group.child_count != 0U ||
            group.created_node_id.has_value() ||
            group.resulting_root_node_id != *prior_root_begin) {
          return ReplayFailure::source_forest_contradiction;
        }
        break;
      case ExactDirectMorseForestAtomicGroupKind::multifusion: {
        const ReplayFailure node_failure = validate_created_node(
            ExactDirectMorseForestNodeKind::multifusion);
        if (plan.prior_root_count < 2U ||
            group.child_offset > forest.child_node_ids.size() ||
            group.child_count >
                forest.child_node_ids.size() - group.child_offset ||
            group.child_count != plan.prior_root_count ||
            node_failure != ReplayFailure::none) {
          return node_failure == ReplayFailure::budget_exhausted
                     ? node_failure
                     : ReplayFailure::source_forest_contradiction;
        }
        for (std::size_t local_child = 0U;
             local_child < group.child_count;
             ++local_child) {
          if (impl.counters.replayed_child_reference_count >=
              impl.budget.carrier_state_budget
                  .maximum_forest_child_reference_scan_count) {
            return ReplayFailure::budget_exhausted;
          }
          ++impl.counters.replayed_child_reference_count;
          if (forest.child_node_ids[group.child_offset + local_child] !=
              *(prior_root_begin +
                static_cast<std::ptrdiff_t>(local_child))) {
            return ReplayFailure::source_forest_contradiction;
          }
        }
        break;
      }
      default:
        return ReplayFailure::source_forest_contradiction;
    }

    const ExactDirectSparseComponentHandle canonical_handle =
        *carrier_begin;
    for (std::size_t local = 1U; local < plan.carrier_count; ++local) {
      const std::size_t global_union_index =
          impl.active_locator_union_count +
          impl.scratch_locator_unions.size();
      const auto token = replay_token(global_union_index, 2U);
      if (!token.has_value()) {
        return ReplayFailure::capacity_overflow;
      }
      impl.scratch_locator_unions.push_back(
          {impl.scratch_locator_unions.size(),
           canonical_handle,
           *(carrier_begin + static_cast<std::ptrdiff_t>(local)),
           {forest.config.locator_config.external_authority_id, *token}});
    }
    impl.scratch_group_plans.push_back(plan);
  }

  if (impl.scratch_group_plans.size() != batch.atomic_group_count) {
    return ReplayFailure::source_forest_contradiction;
  }

  for (std::size_t local_birth = 0U;
       local_birth < batch.birth_record_count;
       ++local_birth) {
    const std::size_t birth_index =
        batch.birth_record_offset + local_birth;
    const auto birth = forest_view.birth_record_at(birth_index);
    if (!validate_birth_record(impl, batch, birth, birth_index)) {
      return ReplayFailure::source_forest_contradiction;
    }
    impl.scratch_locator_bindings.push_back(
        {impl.scratch_locator_bindings.size(),
         birth.facet_key,
         birth.component_handle,
         birth.binding_witness});
  }
  return ReplayFailure::none;
}

[[nodiscard]] ReplayFailure apply_scientific_batch_state(
    ExactDirectMorseForestCarrierCutReplaySession::Impl& impl,
    std::size_t batch_index) {
  const auto& forest = *impl.source_forest;
  const auto& batch = forest.batches[batch_index];
  const ExactDirectMorseForestJournalView forest_view{forest};

  for (const BatchGroupPlan& plan : impl.scratch_group_plans) {
    const auto& group = forest.atomic_groups[plan.group_index];
    if (plan.carrier_count == 0U ||
        plan.carrier_offset > impl.scratch_group_carriers.size() ||
        plan.carrier_count >
            impl.scratch_group_carriers.size() - plan.carrier_offset ||
        plan.prior_root_offset >
            impl.scratch_group_prior_roots.size() ||
        plan.prior_root_count >
            impl.scratch_group_prior_roots.size() -
                plan.prior_root_offset ||
        impl.active_reduced_root_counts_by_order[batch.order] <
            plan.prior_root_count) {
      return ReplayFailure::source_forest_contradiction;
    }
    for (std::size_t local_root = 0U;
         local_root < plan.prior_root_count;
         ++local_root) {
      const auto root = impl.scratch_group_prior_roots[
          plan.prior_root_offset + local_root];
      if (root >= impl.node_states.size()) {
        return ReplayFailure::source_forest_contradiction;
      }
      impl.node_states[root].owner_component_handle.reset();
    }

    const ExactDirectSparseComponentHandle canonical_handle =
        impl.scratch_group_carriers[plan.carrier_offset];
    for (std::size_t local_carrier = 0U;
         local_carrier < plan.carrier_count;
         ++local_carrier) {
      const ExactDirectSparseComponentHandle handle =
          impl.scratch_group_carriers[
              plan.carrier_offset + local_carrier];
      if (handle >= impl.components.size() ||
          !impl.components[handle].active ||
          impl.components[handle].parent != handle) {
        return ReplayFailure::source_forest_contradiction;
      }
      impl.components[handle].reduced_root_node_id.reset();
      if (handle != canonical_handle) {
        if (impl.active_carrier_root_counts_by_order[batch.order] == 0U) {
          return ReplayFailure::source_forest_contradiction;
        }
        impl.components[handle].parent = canonical_handle;
        --impl.active_carrier_root_counts_by_order[batch.order];
      }
    }
    if (group.resulting_root_node_id >= impl.node_states.size() ||
        impl.node_states[group.resulting_root_node_id]
            .owner_component_handle.has_value()) {
      return ReplayFailure::source_forest_contradiction;
    }
    impl.components[canonical_handle].reduced_root_node_id =
        group.resulting_root_node_id;
    impl.node_states[group.resulting_root_node_id]
        .owner_component_handle = canonical_handle;
    impl.active_reduced_root_counts_by_order[batch.order] -=
        plan.prior_root_count;
    ++impl.active_reduced_root_counts_by_order[batch.order];
  }

  for (std::size_t local_birth = 0U;
       local_birth < batch.birth_record_count;
       ++local_birth) {
    const std::size_t birth_index =
        batch.birth_record_offset + local_birth;
    const auto birth = forest_view.birth_record_at(birth_index);
    const std::size_t handle = birth.component_handle;
    if (handle >= impl.components.size() ||
        impl.components[handle].active ||
        impl.components[handle].parent != handle ||
        impl.components[handle].order != batch.order) {
      return ReplayFailure::source_forest_contradiction;
    }
    impl.components[handle].active = true;
    ++impl.active_carrier_root_counts_by_order[batch.order];
    if (batch.order == 1U) {
      if (!birth.order_one_birth_node_id.has_value() ||
          *birth.order_one_birth_node_id >= impl.node_states.size()) {
        return ReplayFailure::source_forest_contradiction;
      }
      const auto birth_node =
          forest_view.node_at(*birth.order_one_birth_node_id);
      if (birth_node.node_id !=
              static_cast<ExactDirectMorseForestNodeId>(handle) ||
          birth_node.order != 1U ||
          birth_node.kind !=
              ExactDirectMorseForestNodeKind::order_one_birth ||
          birth_node.birth_record_index !=
              std::optional<std::size_t>{birth_index} ||
          birth_node.child_count != 0U ||
          impl.node_states[birth_node.node_id]
              .owner_component_handle.has_value()) {
        return ReplayFailure::source_forest_contradiction;
      }
      const auto relation = compare_levels(
          impl, birth_node.squared_level, batch.squared_level);
      if (!relation.has_value()) {
        return ReplayFailure::budget_exhausted;
      }
      if (*relation != 0) {
        return ReplayFailure::source_forest_contradiction;
      }
      impl.components[handle].reduced_root_node_id = birth_node.node_id;
      impl.node_states[birth_node.node_id].owner_component_handle =
          handle;
      ++impl.active_reduced_root_counts_by_order[batch.order];
    } else if (birth.order_one_birth_node_id.has_value()) {
      return ReplayFailure::source_forest_contradiction;
    }
  }

  if (batch.closed_post_batch_carrier_count !=
          impl.active_carrier_root_counts_by_order[batch.order] ||
      batch.closed_post_batch_reduced_root_count !=
          impl.active_reduced_root_counts_by_order[batch.order]) {
    return ReplayFailure::source_forest_contradiction;
  }

  // The forest state and the live locator must carry the same canonical DSU
  // identity after the complete group phase and after current births.
  for (const BatchGroupPlan& plan : impl.scratch_group_plans) {
    for (std::size_t local_carrier = 0U;
         local_carrier < plan.carrier_count;
         ++local_carrier) {
      const auto handle = impl.scratch_group_carriers[
          plan.carrier_offset + local_carrier];
      const auto forest_root = find_component_root(impl, handle);
      const auto locator_root = find_locator_root(impl, handle);
      if (!forest_root.has_value() || !locator_root.has_value()) {
        return impl.counters.component_parent_hop_count >=
                       impl.budget.carrier_state_budget
                           .maximum_parent_hop_count
                   ? ReplayFailure::budget_exhausted
                   : ReplayFailure::source_forest_contradiction;
      }
      if (*forest_root != *locator_root) {
        return ReplayFailure::source_forest_contradiction;
      }
    }
  }
  for (std::size_t local_birth = 0U;
       local_birth < batch.birth_record_count;
       ++local_birth) {
    const auto birth = forest_view.birth_record_at(
        batch.birth_record_offset + local_birth);
    const auto forest_root =
        find_component_root(impl, birth.component_handle);
    const auto locator_root =
        find_locator_root(impl, birth.component_handle);
    if (!forest_root.has_value() || !locator_root.has_value()) {
      return impl.counters.component_parent_hop_count >=
                     impl.budget.carrier_state_budget
                         .maximum_parent_hop_count
                 ? ReplayFailure::budget_exhausted
                 : ReplayFailure::source_forest_contradiction;
    }
    if (*forest_root != *locator_root) {
      return ReplayFailure::source_forest_contradiction;
    }
  }
  return ReplayFailure::none;
}

[[nodiscard]] ReplayFailure replay_batch(
    ExactDirectMorseForestCarrierCutReplaySession::Impl& impl,
    std::size_t batch_index) {
  const auto& batch = impl.source_forest->batches[batch_index];
  const ReplayFailure preparation = prepare_batch(impl, batch_index);
  if (preparation != ReplayFailure::none) {
    return preparation;
  }

  const auto commit_shape_is_exact =
      [&](const ExactDirectSparsePositiveFacetBatchResult& committed) {
        return committed.certified_committed_batch() &&
               committed.candidate_batch_index == batch_index &&
               committed.lookups.empty() &&
               committed.counters.query_count == 0U &&
               committed.counters.union_request_count ==
                   impl.scratch_locator_unions.size() &&
               committed.counters.binding_request_count ==
                   batch.birth_record_count &&
               committed.counters.inserted_binding_count ==
                   batch.birth_record_count &&
               committed.counters.compatible_duplicate_binding_count ==
                   0U;
      };
  bool locator_commit_certified = false;
  if (singleton_batch_shape(impl, batch)) {
    const auto singleton_commit =
        impl.locator.apply_canonical_singleton_identity_batch(
            batch.birth_record_count);
    locator_commit_certified =
        singleton_commit.certified_committed_identity_batch() &&
        singleton_commit.audit.bulk_count == batch.birth_record_count &&
        commit_shape_is_exact(singleton_commit.batch_result);
  } else {
    const auto committed_batch = impl.locator.apply_batch(
        std::span<const ExactDirectSparseFacetQuery>{},
        impl.scratch_locator_unions,
        impl.scratch_locator_bindings);
    locator_commit_certified = commit_shape_is_exact(committed_batch);
  }
  if (!locator_commit_certified) {
    return ReplayFailure::locator_commit_rejected;
  }

  ++impl.counters.locator_stamp_equality_check_count;
  if (impl.locator.snapshot_stamp() != batch.committed_batch_stamp) {
    return ReplayFailure::locator_stamp_contradiction;
  }
  const ReplayFailure scientific =
      apply_scientific_batch_state(impl, batch_index);
  if (scientific != ReplayFailure::none) {
    return scientific;
  }

  if (!add_counter(
          impl.scratch_locator_unions.size(),
          impl.active_locator_union_count) ||
      !add_counter(
          batch.birth_record_count,
          impl.active_locator_binding_count) ||
      !add_counter(
          impl.scratch_locator_unions.size(),
          impl.counters.replayed_locator_union_count) ||
      !add_counter(
          batch.birth_record_count,
          impl.counters.replayed_locator_binding_count)) {
    return ReplayFailure::capacity_overflow;
  }
  ++impl.counters.replayed_global_batch_count;
  if (batch.order == impl.target_order) {
    ++impl.counters.replayed_target_batch_count;
  }
  if (!add_counter(
          batch.atomic_group_count,
          impl.counters.replayed_atomic_group_count)) {
    return ReplayFailure::capacity_overflow;
  }
  ++impl.next_global_batch_index;
  return ReplayFailure::none;
}

}  // namespace

}  // namespace morsehgp3d::hierarchy
