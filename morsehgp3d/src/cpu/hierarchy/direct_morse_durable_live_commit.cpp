#include "morsehgp3d/hierarchy/direct_morse_durable_live_commit.hpp"

#include <exception>
#include <limits>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

struct LiveCommitParticipantState {
  ExactDirectSparseFacetDescentAnchoredBatchExecutor* executor{};
  ExactDirectMorseForestReducer* reducer{};
  ExactDirectSparseFacetDescentAnchoredBatchExecutor::
      PreparedTopKProposalBatch* prepared{};
  std::optional<ExactDirectMorseForestLiveCommitResult> live_commit;
  std::uint64_t expected_batch_index{};
  bool exception_observed{false};
};

AtomicLinearRunPreHeadCommitOutcome commit_live_participant(
    const AtomicLinearRunTransition& transition,
    const AtomicLinearRunAcceptedProjection*,
    void* opaque) noexcept {
  auto& state = *static_cast<LiveCommitParticipantState*>(opaque);
  if (transition.batch_begin_index != state.expected_batch_index ||
      transition.batch_end_index !=
          state.expected_batch_index + 1U) {
    return AtomicLinearRunPreHeadCommitOutcome::indeterminate;
  }
  try {
    state.live_commit.emplace(
        state.reducer->fold_prepared_ticket(
            *state.executor, std::move(*state.prepared)));
    return state.live_commit->certified_live_commit()
               ? AtomicLinearRunPreHeadCommitOutcome::committed
               : (state.live_commit->certified_atomic_rejection()
                      ? AtomicLinearRunPreHeadCommitOutcome::
                            rejected_atomically
                      : AtomicLinearRunPreHeadCommitOutcome::
                            indeterminate);
  } catch (...) {
    state.exception_observed = true;
    return AtomicLinearRunPreHeadCommitOutcome::indeterminate;
  }
}

class ActiveCallGuard {
 public:
  explicit ActiveCallGuard(bool& active) : active_(&active) {
    if (active) {
      throw std::logic_error(
          "a durable live commit coordinator is already active");
    }
    active = true;
  }

  ~ActiveCallGuard() {
    *active_ = false;
  }

  ActiveCallGuard(const ActiveCallGuard&) = delete;
  ActiveCallGuard& operator=(const ActiveCallGuard&) = delete;

 private:
  bool* active_;
};

[[nodiscard]] bool same_batch_cursor(
    std::uint64_t durable_batch_index,
    std::size_t live_batch_index) noexcept {
  return live_batch_index <=
             std::numeric_limits<std::uint64_t>::max() &&
         durable_batch_index ==
             static_cast<std::uint64_t>(live_batch_index);
}

}  // namespace

bool ExactDirectMorseDurableLiveCommitResult::certified_complete_commit()
    const noexcept {
  return schema_version ==
             direct_morse_durable_live_commit_schema_version &&
         one_batch_chunk_bound_to_live_delta &&
         publication_recertified_before_process_local_commit &&
         process_local_commit_preceded_HEAD &&
         authoritative_HEAD_and_live_cursors_aligned &&
         !retryable_without_reopen && !reopen_required &&
         !coordinator_poisoned &&
         !forbidden_global_structure_materialized &&
         !public_status_claimed &&
         publication.decision ==
             AtomicLinearRunPublishDecision::durably_published &&
         publication.recertification.accepted() &&
         publication.process_local_commit_attempted &&
         publication.process_local_commit_succeeded &&
         publication.trusted_state_advanced &&
         live_commit.has_value() &&
         live_commit->source_batch_index == source_batch_index &&
         live_commit->certified_live_commit() &&
         decision == ExactDirectMorseDurableLiveCommitDecision::
                         complete_durable_live_single_batch_commit;
}

bool ExactDirectMorseDurableLiveCommitResult::
    certified_retryable_rejection() const noexcept {
  const bool publication_rejected =
      decision == ExactDirectMorseDurableLiveCommitDecision::
                      no_commit_publication_rejected_retryable &&
      !publication.process_local_commit_attempted &&
      !live_commit.has_value();
  const bool participant_rejected =
      decision == ExactDirectMorseDurableLiveCommitDecision::
                      no_commit_live_participant_rejected_retryable &&
      publication.decision ==
          AtomicLinearRunPublishDecision::
              process_local_commit_rejected &&
      publication.process_local_commit_attempted &&
      !publication.process_local_commit_succeeded &&
      live_commit.has_value() &&
      live_commit->certified_atomic_rejection();
  return schema_version ==
             direct_morse_durable_live_commit_schema_version &&
         one_batch_chunk_bound_to_live_delta &&
         (publication_rejected || participant_rejected) &&
         !publication.trusted_state_advanced &&
         !process_local_commit_preceded_HEAD &&
         !authoritative_HEAD_and_live_cursors_aligned &&
         retryable_without_reopen && !reopen_required &&
         !coordinator_poisoned &&
         !forbidden_global_structure_materialized &&
         !public_status_claimed;
}

bool ExactDirectMorseDurableLiveCommitResult::
    certified_reopen_required() const noexcept {
  return schema_version ==
             direct_morse_durable_live_commit_schema_version &&
         one_batch_chunk_bound_to_live_delta &&
         !authoritative_HEAD_and_live_cursors_aligned &&
         !retryable_without_reopen && reopen_required &&
         coordinator_poisoned &&
         !forbidden_global_structure_materialized &&
         !public_status_claimed &&
         decision == ExactDirectMorseDurableLiveCommitDecision::
                         no_commit_indeterminate_reopen_required;
}

ExactDirectMorseDurableLiveCommitCoordinator::
    ExactDirectMorseDurableLiveCommitCoordinator(
        const ExactDirectMorseChunkRunContext& chunk_context,
        AtomicLinearRunStore& store,
        ExactDirectSparseFacetDescentAnchoredBatchExecutor& executor,
        ExactDirectMorseForestReducer& reducer)
    : chunk_context_(&chunk_context),
      store_(&store),
      executor_(&executor),
      reducer_(&reducer) {
  if (!same_batch_cursor(
          store_->trusted_state().next_batch_index,
          executor_->next_source_batch_index()) ||
      executor_->next_source_batch_index() !=
          reducer_->next_source_batch_index()) {
    throw std::invalid_argument(
        "a durable live coordinator requires aligned initial cursors");
  }
}

ExactDirectMorseDurableLiveCommitResult
ExactDirectMorseDurableLiveCommitCoordinator::
    commit_next_single_batch_chunk(
        ExactDirectSparseFacetDescentAnchoredBatchExecutor::
            PreparedTopKProposalBatch& prepared,
        AtomicLinearRunPublishOptions options) {
  if (poisoned_) {
    throw std::logic_error(
        "a poisoned durable live coordinator requires reopen");
  }
  if (options.pre_head_commit != nullptr ||
      options.pre_head_commit_state != nullptr) {
    throw std::invalid_argument(
        "a durable live coordinator owns the pre-HEAD participant");
  }
  if (!prepared.prepared() || prepared.scientific_delta() == nullptr) {
    throw std::invalid_argument(
        "a durable live coordinator requires one valid sealed ticket");
  }
  if (!same_batch_cursor(
          store_->trusted_state().next_batch_index,
          executor_->next_source_batch_index()) ||
      executor_->next_source_batch_index() !=
          reducer_->next_source_batch_index() ||
      prepared.scientific_delta()->source_batch_index !=
          executor_->next_source_batch_index()) {
    throw std::invalid_argument(
        "a durable live coordinator requires aligned current cursors");
  }
  if (store_->trusted_state().next_chunk_index >
      std::numeric_limits<std::size_t>::max()) {
    throw std::length_error(
        "a durable live chunk index is not addressable");
  }

  ActiveCallGuard active_guard{active_};
  ExactDirectMorseDurableLiveCommitResult result;
  result.source_batch_index =
      prepared.scientific_delta()->source_batch_index;
  AtomicLinearRunChunkProposal proposal =
      chunk_context_->
          prepare_single_batch_chunk_bound_to_live_delta(
              store_->trusted_state(),
              static_cast<std::size_t>(
                  store_->trusted_state().next_chunk_index),
              *prepared.scientific_delta());
  result.one_batch_chunk_bound_to_live_delta = true;

  LiveCommitParticipantState participant{
      executor_,
      reducer_,
      &prepared,
      std::nullopt,
      store_->trusted_state().next_batch_index,
      false};
  options.pre_head_commit = commit_live_participant;
  options.pre_head_commit_state = &participant;

  static_assert(
      std::is_nothrow_move_assignable_v<
          AtomicLinearRunPublishResult>);
  static_assert(
      std::is_nothrow_move_assignable_v<
          std::optional<ExactDirectMorseForestLiveCommitResult>>);
  static_assert(
      std::is_nothrow_move_constructible_v<
          ExactDirectMorseDurableLiveCommitResult>);
  result.publication =
      store_->publish_next(std::move(proposal), options);
  result.live_commit = std::move(participant.live_commit);
  result.publication_recertified_before_process_local_commit =
      result.publication.process_local_commit_attempted &&
      result.publication.recertification.accepted();

  if (result.publication.decision ==
      AtomicLinearRunPublishDecision::durably_published) {
    if (!result.publication.process_local_commit_succeeded ||
        !result.live_commit.has_value() ||
        !result.live_commit->certified_live_commit() ||
        !same_batch_cursor(
            store_->trusted_state().next_batch_index,
            executor_->next_source_batch_index()) ||
        executor_->next_source_batch_index() !=
            reducer_->next_source_batch_index()) {
      std::terminate();
    }
    result.process_local_commit_preceded_HEAD = true;
    result.authoritative_HEAD_and_live_cursors_aligned = true;
    result.decision = ExactDirectMorseDurableLiveCommitDecision::
        complete_durable_live_single_batch_commit;
    if (!result.certified_complete_commit()) {
      std::terminate();
    }
    return result;
  }

  if (result.publication.process_local_commit_succeeded ||
      result.publication.decision ==
          AtomicLinearRunPublishDecision::
              indeterminate_io_failure_reopen_required ||
      participant.exception_observed) {
    poisoned_ = true;
    result.process_local_commit_preceded_HEAD =
        result.publication.process_local_commit_succeeded;
    result.reopen_required = true;
    result.coordinator_poisoned = true;
    result.decision = ExactDirectMorseDurableLiveCommitDecision::
        no_commit_indeterminate_reopen_required;
    return result;
  }

  result.retryable_without_reopen = true;
  if (result.publication.decision ==
          AtomicLinearRunPublishDecision::
              process_local_commit_rejected &&
      result.live_commit.has_value() &&
      result.live_commit->certified_atomic_rejection()) {
    result.decision = ExactDirectMorseDurableLiveCommitDecision::
        no_commit_live_participant_rejected_retryable;
  } else if (!result.publication.process_local_commit_attempted) {
    result.decision = ExactDirectMorseDurableLiveCommitDecision::
        no_commit_publication_rejected_retryable;
  } else {
    poisoned_ = true;
    result.retryable_without_reopen = false;
    result.reopen_required = true;
    result.coordinator_poisoned = true;
    result.decision = ExactDirectMorseDurableLiveCommitDecision::
        no_commit_indeterminate_reopen_required;
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
