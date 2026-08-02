#include "morsehgp3d/hierarchy/direct_normalized_h0_authenticated_window_stream.hpp"

#include <exception>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

struct StreamControl {
  std::uint64_t stream_authority_id{};
  const void* provider_identity{};
  contract::CanonicalId manifest_digest{};
  std::size_t live_ticket_count{};
  std::size_t maximum_ticket_count{};
};

[[nodiscard]] bool verification_is_bound(
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest,
    const ExactDirectNormalizedH0ResidentProviderVerification& verification,
    ExactDirectNormalizedH0ResidentBatchProviderView provider) noexcept {
  return static_cast<bool>(provider) && provider.identity() != nullptr &&
         verification.provider_identity == provider.identity() &&
         verification.provider_identity_bound &&
         verification.manifest_digest == manifest.manifest_digest &&
         verification.batch_visit_count == manifest.batch_count &&
         verification.consumer_callback_count == manifest.batch_count &&
         verification.direct_reference_count ==
             manifest.direct_reference_count &&
         verification.residual_reference_count ==
             manifest.residual_reference_count &&
         verification.coface_facet_reference_count ==
             manifest.coface_facet_reference_count &&
         verification.final_batch_chain_digest ==
             manifest.final_batch_chain_digest &&
         verification.manifest_certified &&
         verification.every_window_freshly_recertified &&
         verification.batches_dense_and_strictly_level_ordered &&
         verification.final_chain_matches_manifest &&
         verification.exact_source_totals_match_manifest &&
         verification.no_window_payload_retained &&
         !verification.multiple_consumer_callbacks_observed &&
         !verification.source_exactness_claimed &&
         !verification.public_status_claimed &&
         verification.result_certified &&
         verification.decision ==
             ExactDirectNormalizedH0ResidentProviderVerificationDecision::
                 complete_authenticated_stream_replay;
}

[[nodiscard]] bool budget_is_one_window_bounded(
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest,
    const ExactDirectNormalizedH0AuthenticatedWindowStreamBudget& budget)
    noexcept {
  return budget.maximum_outstanding_ticket_count == 1U &&
         budget.provider_window.maximum_batch_scan_count >=
             manifest.batch_count &&
         budget.provider_window.maximum_window_facet_count != 0U &&
         budget.provider_window.maximum_window_facet_point_count != 0U &&
         budget.provider_window.maximum_window_logical_entry_count != 0U;
}

}  // namespace

struct ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow::Impl {
  ~Impl() noexcept {
    if (!owns_ticket_slot) {
      return;
    }
    if (control == nullptr || control->live_ticket_count == 0U) {
      std::terminate();
    }
    --control->live_ticket_count;
  }

  ExactDirectNormalizedH0ResidentOwnedBatchWindow owned{};
  std::shared_ptr<StreamControl> control;
  std::uint64_t stream_authority_id{};
  const void* provider_identity{};
  contract::CanonicalId manifest_digest{};
  contract::CanonicalId source_chain_digest{};
  std::size_t source_cursor{};
  std::size_t source_epoch{};
  bool window_authenticated_at_prepare{false};
  bool consumed{false};
  bool owns_ticket_slot{false};
};

struct ExactDirectNormalizedH0AuthenticatedWindowStreamSession::Impl {
  ExactDirectNormalizedH0ResidentSourceManifest manifest{};
  ExactDirectNormalizedH0ResidentProviderVerification verification{};
  ExactDirectNormalizedH0ResidentBatchProviderView provider{};
  ExactDirectNormalizedH0AuthenticatedWindowStreamBudget budget{};
  std::shared_ptr<StreamControl> control;
  std::optional<ExactDirectNormalizedH0AuthenticatedWindowStreamFinalSeal>
      final_seal;
  contract::CanonicalId current_chain_digest{};
  std::uint64_t stream_authority_id{};
  std::size_t cursor{};
  std::size_t epoch{};
  bool initialized{false};
  bool sealed{false};
};

ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow::
    ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow()
    noexcept = default;
ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow::
    ~ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow() =
        default;
ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow::
    ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow(
        ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow&&)
    noexcept = default;
ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow&
ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow::operator=(
    ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow&&)
    noexcept = default;
ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow::
    ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow(
        std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow::valid()
    const noexcept {
  return impl_ != nullptr && !impl_->consumed && impl_->owns_ticket_slot &&
         impl_->control != nullptr && impl_->stream_authority_id != 0U &&
         impl_->provider_identity != nullptr &&
         impl_->window_authenticated_at_prepare &&
         impl_->owned.exact_window_copied &&
         impl_->owned.no_future_payload_owned;
}

bool
ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow::consumed()
    const noexcept {
  return impl_ != nullptr && impl_->consumed;
}

const ExactDirectNormalizedH0ResidentOwnedBatchWindow&
ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow::
    owned_window() const noexcept {
  static const ExactDirectNormalizedH0ResidentOwnedBatchWindow empty{};
  return impl_ == nullptr ? empty : impl_->owned;
}

bool ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationResult::
    certified_provisional_preparation() const noexcept {
  return ticket.has_value() && ticket->valid() &&
         provider_callback_count == 1U && one_window_owned &&
         cursor_and_chain_unchanged && !resident_scientific_state_mutated &&
         !source_exactness_claimed && !public_status_claimed &&
         decision ==
             ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
                 complete_provisional_owned_window;
}

bool ExactDirectNormalizedH0AuthenticatedWindowStreamCommitResult::
    certified_provisional_commit() const noexcept {
  return ticket_consumed && cursor_advanced_once &&
         chain_advanced_to_ticket_successor &&
         !no_session_metadata_mutated_on_failure &&
         !resident_scientific_state_mutated && !source_exactness_claimed &&
         !public_status_claimed &&
         decision ==
             ExactDirectNormalizedH0AuthenticatedWindowStreamCommitDecision::
                 complete_provisional_cursor_and_chain_commit;
}

bool ExactDirectNormalizedH0AuthenticatedWindowStreamCheckpoint::
    certified_honest_nonrestartable_checkpoint() const noexcept {
  return schema_version ==
             direct_normalized_h0_authenticated_window_stream_schema_version &&
         stream_authority_id != 0U &&
         process_local_provider_identity != nullptr &&
         provider_identity_process_local_only && provisional_metadata_only &&
         !resident_or_locator_state_serialized &&
         !checkpoint_restore_supported && !restartable &&
         !source_exactness_claimed && !public_status_claimed &&
         decision ==
             ExactDirectNormalizedH0AuthenticatedWindowStreamCheckpointDecision::
                 complete_honest_nonrestartable_metadata_checkpoint;
}

bool ExactDirectNormalizedH0AuthenticatedWindowStreamFinalSeal::
    certified_provisional_terminal_seal() const noexcept {
  return schema_version ==
             direct_normalized_h0_authenticated_window_stream_schema_version &&
         stream_authority_id != 0U &&
         process_local_provider_identity != nullptr &&
         provider_identity_bound && complete_cursor_reached &&
         terminal_chain_matches_manifest && provisional_metadata_only &&
         !resident_fold_executed && !source_exactness_claimed &&
         !public_status_claimed && sealed;
}

bool ExactDirectNormalizedH0AuthenticatedWindowStreamSealResult::
    certified_provisional_seal() const noexcept {
  return seal.has_value() && seal->certified_provisional_terminal_seal() &&
         decision ==
             ExactDirectNormalizedH0AuthenticatedWindowStreamSealDecision::
                 complete_provisional_terminal_seal;
}

ExactDirectNormalizedH0AuthenticatedWindowStreamSession::
    ExactDirectNormalizedH0AuthenticatedWindowStreamSession() noexcept =
        default;
ExactDirectNormalizedH0AuthenticatedWindowStreamSession::
    ~ExactDirectNormalizedH0AuthenticatedWindowStreamSession() = default;
ExactDirectNormalizedH0AuthenticatedWindowStreamSession::
    ExactDirectNormalizedH0AuthenticatedWindowStreamSession(
        ExactDirectNormalizedH0AuthenticatedWindowStreamSession&&) noexcept =
        default;
ExactDirectNormalizedH0AuthenticatedWindowStreamSession&
ExactDirectNormalizedH0AuthenticatedWindowStreamSession::operator=(
    ExactDirectNormalizedH0AuthenticatedWindowStreamSession&&) noexcept =
    default;
ExactDirectNormalizedH0AuthenticatedWindowStreamSession::
    ExactDirectNormalizedH0AuthenticatedWindowStreamSession(
        std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectNormalizedH0AuthenticatedWindowStreamSession::
    provisional_bound_stream() const noexcept {
  if (impl_ == nullptr || !impl_->initialized ||
      impl_->stream_authority_id == 0U || impl_->control == nullptr ||
      !impl_->manifest.certified() ||
      !verification_is_bound(
          impl_->manifest, impl_->verification, impl_->provider) ||
      !budget_is_one_window_bounded(impl_->manifest, impl_->budget) ||
      impl_->control->stream_authority_id != impl_->stream_authority_id ||
      impl_->control->provider_identity != impl_->provider.identity() ||
      impl_->control->manifest_digest != impl_->manifest.manifest_digest ||
      impl_->control->maximum_ticket_count != 1U ||
      impl_->control->live_ticket_count > 1U ||
      impl_->cursor > impl_->manifest.batch_count ||
      impl_->epoch != impl_->cursor) {
    return false;
  }
  const bool terminal_cursor_consistent =
      impl_->cursor != impl_->manifest.batch_count ||
      impl_->current_chain_digest == impl_->manifest.final_batch_chain_digest;
  const bool seal_consistent =
      !impl_->sealed
      ? !impl_->final_seal.has_value()
      : impl_->final_seal.has_value() &&
            impl_->final_seal->certified_provisional_terminal_seal() &&
            impl_->final_seal->committed_batch_count == impl_->cursor &&
            impl_->final_seal->final_chain_digest ==
                impl_->current_chain_digest &&
            impl_->control->live_ticket_count == 0U;
  return terminal_cursor_consistent && seal_consistent;
}

bool ExactDirectNormalizedH0AuthenticatedWindowStreamSession::complete()
    const noexcept {
  return provisional_bound_stream() &&
         impl_->cursor == impl_->manifest.batch_count &&
         impl_->current_chain_digest ==
             impl_->manifest.final_batch_chain_digest;
}

bool ExactDirectNormalizedH0AuthenticatedWindowStreamSession::sealed()
    const noexcept {
  return provisional_bound_stream() && impl_->sealed;
}

std::size_t
ExactDirectNormalizedH0AuthenticatedWindowStreamSession::batch_cursor()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->cursor;
}

std::size_t ExactDirectNormalizedH0AuthenticatedWindowStreamSession::epoch()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->epoch;
}

const contract::CanonicalId&
ExactDirectNormalizedH0AuthenticatedWindowStreamSession::
    current_chain_digest() const noexcept {
  static const contract::CanonicalId empty{};
  return impl_ == nullptr ? empty : impl_->current_chain_digest;
}

const void*
ExactDirectNormalizedH0AuthenticatedWindowStreamSession::provider_identity()
    const noexcept {
  return impl_ == nullptr ? nullptr : impl_->provider.identity();
}

bool ExactDirectNormalizedH0AuthenticatedWindowStreamSession::
    source_exactness_claimed() const noexcept {
  return false;
}

bool ExactDirectNormalizedH0AuthenticatedWindowStreamSession::
    public_status_claimed() const noexcept {
  return false;
}

ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationResult
ExactDirectNormalizedH0AuthenticatedWindowStreamSession::prepare_next() {
  ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationResult output;
  output.cursor_and_chain_unchanged = true;
  const auto reject = [&output](
                          ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision
                              decision) {
    output.ticket.reset();
    output.one_window_owned = false;
    output.decision = decision;
    return std::move(output);
  };
  if (!provisional_bound_stream()) {
    return reject(
        ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
            no_session_rejected);
  }
  if (impl_->sealed) {
    return reject(
        ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
            no_stream_sealed);
  }
  if (impl_->cursor >= impl_->manifest.batch_count) {
    return reject(
        ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
            no_source_exhausted);
  }
  if (impl_->control->live_ticket_count >=
      impl_->control->maximum_ticket_count) {
    return reject(
        ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
            no_outstanding_ticket_budget_exhausted);
  }
  if (impl_->provider.identity() == nullptr ||
      impl_->provider.identity() != impl_->control->provider_identity ||
      impl_->provider.identity() != impl_->verification.provider_identity) {
    return reject(
        ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
            no_provider_identity_rejected);
  }

  const std::size_t source_cursor = impl_->cursor;
  const std::size_t source_epoch = impl_->epoch;
  const contract::CanonicalId source_chain = impl_->current_chain_digest;
  try {
    struct Capture {
      const ExactDirectNormalizedH0ResidentSourceManifest& manifest;
      const ExactDirectNormalizedH0ResidentBatchProviderBudget& budget;
      std::size_t expected_cursor{};
      const contract::CanonicalId& expected_chain;
      std::size_t callback_count{};
      bool prefix_or_cursor_rejected{false};
      bool copy_rejected{false};
      bool accepted{false};
      ExactDirectNormalizedH0ResidentOwnedBatchWindow owned{};

      bool operator()(
          const ExactDirectNormalizedH0ResidentBatchWindow& window) {
        if (callback_count == std::numeric_limits<std::size_t>::max()) {
          prefix_or_cursor_rejected = true;
          return false;
        }
        ++callback_count;
        if (callback_count != 1U ||
            !window.certified_relative_to(manifest, budget) ||
            window.source_batch_index != expected_cursor ||
            window.source_future_snapshot_index != expected_cursor ||
            window.source_chain_digest != expected_chain ||
            window.manifest_digest != manifest.manifest_digest ||
            window.source_identity_digest != manifest.source_identity_digest) {
          prefix_or_cursor_rejected = true;
          return false;
        }
        auto copied = copy_exact_direct_normalized_h0_resident_batch_window(
            manifest, window, budget);
        // The copy helper publishes these two facts only after its private
        // final certified_relative_to replay.  Retain that attestation in the
        // move-only ticket so the noexcept commit path never allocates or
        // reconstructs another window-sized validation arena.
        if (!copied.exact_window_copied ||
            !copied.no_future_payload_owned ||
            copied.source_batch_index != expected_cursor ||
            copied.source_chain_digest != expected_chain) {
          copy_rejected = true;
          return false;
        }
        owned = std::move(copied);
        accepted = true;
        return true;
      }
    } capture{
        impl_->manifest,
        impl_->budget.provider_window,
        source_cursor,
        source_chain};

    ExactDirectNormalizedH0ResidentBatchConsumerView consumer{capture};
    ExactDirectNormalizedH0ResidentBatchVisitDecision visit;
    try {
      visit = impl_->provider(source_cursor, consumer);
    } catch (...) {
      visit = ExactDirectNormalizedH0ResidentBatchVisitDecision::
          no_window_inconsistent;
    }
    output.provider_callback_count = capture.callback_count;
    if (capture.callback_count != 1U) {
      return reject(
          ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
              no_provider_callback_cardinality_rejected);
    }
    if (capture.prefix_or_cursor_rejected) {
      return reject(
          ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
              no_window_prefix_or_cursor_rejected);
    }
    if (capture.copy_rejected || !capture.accepted) {
      return reject(
          ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
              no_window_copy_rejected);
    }
    if (visit != ExactDirectNormalizedH0ResidentBatchVisitDecision::
                     complete_synchronous_visit) {
      return reject(
          ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
              no_provider_visit_rejected);
    }
    if (impl_->cursor != source_cursor || impl_->epoch != source_epoch ||
        impl_->current_chain_digest != source_chain || impl_->sealed) {
      return reject(
          ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
              no_window_prefix_or_cursor_rejected);
    }

    auto prepared = std::make_unique<
        ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow::Impl>();
    prepared->owned = std::move(capture.owned);
    prepared->control = impl_->control;
    prepared->stream_authority_id = impl_->stream_authority_id;
    prepared->provider_identity = impl_->provider.identity();
    prepared->manifest_digest = impl_->manifest.manifest_digest;
    prepared->source_chain_digest = source_chain;
    prepared->source_cursor = source_cursor;
    prepared->source_epoch = source_epoch;
    prepared->window_authenticated_at_prepare = true;
    ++impl_->control->live_ticket_count;
    prepared->owns_ticket_slot = true;
    output.ticket.emplace(
        ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow{
            std::move(prepared)});
    output.one_window_owned = true;
    output.decision =
        ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
            complete_provisional_owned_window;
    return output;
  } catch (const std::bad_alloc&) {
    return reject(
        ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
            no_allocation_failed);
  } catch (const std::length_error&) {
    return reject(
        ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
            no_window_copy_rejected);
  }
}

ExactDirectNormalizedH0AuthenticatedWindowStreamCommitResult
ExactDirectNormalizedH0AuthenticatedWindowStreamSession::commit(
    ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow&& ticket)
    noexcept {
  ExactDirectNormalizedH0AuthenticatedWindowStreamCommitResult output;
  const std::size_t initial_cursor = impl_ == nullptr ? 0U : impl_->cursor;
  const std::size_t initial_epoch = impl_ == nullptr ? 0U : impl_->epoch;
  const contract::CanonicalId initial_chain =
      impl_ == nullptr ? contract::CanonicalId{}
                       : impl_->current_chain_digest;
  const bool initial_sealed = impl_ != nullptr && impl_->sealed;
  auto consumed = std::move(ticket.impl_);
  const auto reject = [&](
                          ExactDirectNormalizedH0AuthenticatedWindowStreamCommitDecision
                              decision) {
    if (consumed != nullptr) {
      consumed->consumed = true;
      output.ticket_consumed = true;
    }
    output.no_session_metadata_mutated_on_failure =
        impl_ == nullptr ||
        (impl_->cursor == initial_cursor && impl_->epoch == initial_epoch &&
         impl_->current_chain_digest == initial_chain &&
         impl_->sealed == initial_sealed);
    output.decision = decision;
    return output;
  };
  if (consumed == nullptr || consumed->consumed ||
      !consumed->owns_ticket_slot || consumed->control == nullptr) {
    return reject(
        ExactDirectNormalizedH0AuthenticatedWindowStreamCommitDecision::
            no_ticket_invalid_or_consumed);
  }
  if (impl_ == nullptr || consumed->control != impl_->control ||
      consumed->stream_authority_id != impl_->stream_authority_id ||
      consumed->provider_identity != impl_->provider.identity() ||
      consumed->manifest_digest != impl_->manifest.manifest_digest) {
    return reject(
        ExactDirectNormalizedH0AuthenticatedWindowStreamCommitDecision::
            no_foreign_ticket_rejected);
  }
  if (!provisional_bound_stream() || impl_->sealed ||
      consumed->source_cursor != impl_->cursor ||
      consumed->source_epoch != impl_->epoch ||
      consumed->source_chain_digest != impl_->current_chain_digest ||
      consumed->owned.source_batch_index != impl_->cursor ||
      consumed->owned.source_chain_digest != impl_->current_chain_digest ||
      impl_->cursor >= impl_->manifest.batch_count ||
      impl_->control->live_ticket_count != 1U) {
    return reject(
        ExactDirectNormalizedH0AuthenticatedWindowStreamCommitDecision::
            no_stale_ticket_rejected);
  }
  if (!consumed->window_authenticated_at_prepare ||
      !consumed->owned.exact_window_copied ||
      !consumed->owned.no_future_payload_owned ||
      consumed->owned.manifest_digest != impl_->manifest.manifest_digest ||
      (impl_->cursor + 1U == impl_->manifest.batch_count &&
       consumed->owned.successor_chain_digest !=
           impl_->manifest.final_batch_chain_digest)) {
    return reject(
        ExactDirectNormalizedH0AuthenticatedWindowStreamCommitDecision::
            no_owned_window_rejected);
  }

  output.source_batch_index = consumed->source_cursor;
  impl_->current_chain_digest = consumed->owned.successor_chain_digest;
  ++impl_->cursor;
  ++impl_->epoch;
  consumed->consumed = true;
  output.committed_cursor = impl_->cursor;
  output.committed_epoch = impl_->epoch;
  output.committed_chain_digest = impl_->current_chain_digest;
  output.ticket_consumed = true;
  output.cursor_advanced_once = impl_->cursor == initial_cursor + 1U &&
                                impl_->epoch == initial_epoch + 1U;
  output.chain_advanced_to_ticket_successor =
      output.committed_chain_digest ==
      consumed->owned.successor_chain_digest;
  output.decision =
      ExactDirectNormalizedH0AuthenticatedWindowStreamCommitDecision::
          complete_provisional_cursor_and_chain_commit;
  return output;
}

ExactDirectNormalizedH0AuthenticatedWindowStreamSealResult
ExactDirectNormalizedH0AuthenticatedWindowStreamSession::seal() noexcept {
  ExactDirectNormalizedH0AuthenticatedWindowStreamSealResult output;
  if (!provisional_bound_stream()) {
    output.decision =
        ExactDirectNormalizedH0AuthenticatedWindowStreamSealDecision::
            no_session_rejected;
    return output;
  }
  if (impl_->sealed) {
    output.seal = impl_->final_seal;
    output.newly_sealed = false;
    output.decision =
        ExactDirectNormalizedH0AuthenticatedWindowStreamSealDecision::
            complete_provisional_terminal_seal;
    return output;
  }
  if (impl_->control->live_ticket_count != 0U) {
    output.decision =
        ExactDirectNormalizedH0AuthenticatedWindowStreamSealDecision::
            no_outstanding_ticket_rejected;
    return output;
  }
  if (impl_->cursor != impl_->manifest.batch_count) {
    output.decision =
        ExactDirectNormalizedH0AuthenticatedWindowStreamSealDecision::
            no_source_not_exhausted;
    return output;
  }
  if (impl_->current_chain_digest !=
      impl_->manifest.final_batch_chain_digest) {
    output.decision =
        ExactDirectNormalizedH0AuthenticatedWindowStreamSealDecision::
            no_terminal_chain_mismatch;
    return output;
  }
  ExactDirectNormalizedH0AuthenticatedWindowStreamFinalSeal final{
      direct_normalized_h0_authenticated_window_stream_schema_version,
      impl_->stream_authority_id,
      impl_->provider.identity(),
      impl_->manifest.manifest_digest,
      impl_->manifest.source_identity_digest,
      impl_->current_chain_digest,
      impl_->cursor,
      true,
      true,
      true,
      true,
      false,
      false,
      false,
      true};
  if (!final.certified_provisional_terminal_seal()) {
    output.decision =
        ExactDirectNormalizedH0AuthenticatedWindowStreamSealDecision::
            no_terminal_chain_mismatch;
    return output;
  }
  impl_->final_seal = final;
  impl_->sealed = true;
  output.seal = final;
  output.newly_sealed = true;
  output.decision =
      ExactDirectNormalizedH0AuthenticatedWindowStreamSealDecision::
          complete_provisional_terminal_seal;
  return output;
}

ExactDirectNormalizedH0AuthenticatedWindowStreamCheckpoint
ExactDirectNormalizedH0AuthenticatedWindowStreamSession::make_checkpoint()
    const noexcept {
  ExactDirectNormalizedH0AuthenticatedWindowStreamCheckpoint checkpoint;
  if (!provisional_bound_stream()) {
    return checkpoint;
  }
  checkpoint.stream_authority_id = impl_->stream_authority_id;
  checkpoint.process_local_provider_identity = impl_->provider.identity();
  checkpoint.manifest_digest = impl_->manifest.manifest_digest;
  checkpoint.source_identity_digest = impl_->manifest.source_identity_digest;
  checkpoint.current_chain_digest = impl_->current_chain_digest;
  checkpoint.batch_cursor = impl_->cursor;
  checkpoint.epoch = impl_->epoch;
  checkpoint.stream_complete = complete();
  checkpoint.stream_sealed = impl_->sealed;
  checkpoint.provider_identity_process_local_only = true;
  checkpoint.provisional_metadata_only = true;
  checkpoint.resident_or_locator_state_serialized = false;
  checkpoint.checkpoint_restore_supported = false;
  checkpoint.restartable = false;
  checkpoint.source_exactness_claimed = false;
  checkpoint.public_status_claimed = false;
  checkpoint.decision =
      ExactDirectNormalizedH0AuthenticatedWindowStreamCheckpointDecision::
          complete_honest_nonrestartable_metadata_checkpoint;
  return checkpoint;
}

bool ExactDirectNormalizedH0AuthenticatedWindowStreamInitializationResult::
    certified_provisional_initialization() const noexcept {
  return session.has_value() && session->provisional_bound_stream() &&
         manifest_certified && provider_verification_bound &&
         provider_identity_bound && one_window_memory_bound &&
         !resident_scientific_state_initialized &&
         !source_exactness_claimed && !public_status_claimed &&
         decision ==
             ExactDirectNormalizedH0AuthenticatedWindowStreamInitializationDecision::
                 complete_provisional_bound_stream;
}

ExactDirectNormalizedH0AuthenticatedWindowStreamInitializationResult
initialize_exact_direct_normalized_h0_authenticated_window_stream(
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest,
    const ExactDirectNormalizedH0ResidentProviderVerification& verification,
    ExactDirectNormalizedH0ResidentBatchProviderView provider,
    std::uint64_t stream_authority_id,
    const ExactDirectNormalizedH0AuthenticatedWindowStreamBudget& budget) {
  ExactDirectNormalizedH0AuthenticatedWindowStreamInitializationResult output;
  output.manifest_certified = manifest.certified();
  if (!output.manifest_certified) {
    output.decision =
        ExactDirectNormalizedH0AuthenticatedWindowStreamInitializationDecision::
            no_manifest_rejected;
    return output;
  }
  output.provider_identity_bound =
      provider.identity() != nullptr && verification.provider_identity_bound &&
      verification.provider_identity == provider.identity();
  if (!output.provider_identity_bound) {
    output.decision =
        ExactDirectNormalizedH0AuthenticatedWindowStreamInitializationDecision::
            no_provider_identity_rejected;
    return output;
  }
  output.provider_verification_bound =
      verification_is_bound(manifest, verification, provider);
  if (!output.provider_verification_bound) {
    output.decision =
        ExactDirectNormalizedH0AuthenticatedWindowStreamInitializationDecision::
            no_provider_verification_rejected;
    return output;
  }
  output.one_window_memory_bound =
      budget_is_one_window_bounded(manifest, budget);
  if (stream_authority_id == 0U || !output.one_window_memory_bound) {
    output.decision =
        ExactDirectNormalizedH0AuthenticatedWindowStreamInitializationDecision::
            no_budget_rejected;
    return output;
  }
  try {
    auto impl = std::make_unique<
        ExactDirectNormalizedH0AuthenticatedWindowStreamSession::Impl>();
    impl->manifest = manifest;
    impl->verification = verification;
    impl->provider = provider;
    impl->budget = budget;
    impl->stream_authority_id = stream_authority_id;
    impl->current_chain_digest = manifest.initial_batch_chain_digest;
    impl->control = std::make_shared<StreamControl>(StreamControl{
        stream_authority_id,
        provider.identity(),
        manifest.manifest_digest,
        0U,
        1U});
    impl->initialized = true;
    ExactDirectNormalizedH0AuthenticatedWindowStreamSession session{
        std::move(impl)};
    if (!session.provisional_bound_stream()) {
      output.decision =
          ExactDirectNormalizedH0AuthenticatedWindowStreamInitializationDecision::
              no_provider_verification_rejected;
      return output;
    }
    output.session.emplace(std::move(session));
    output.resident_scientific_state_initialized = false;
    output.source_exactness_claimed = false;
    output.public_status_claimed = false;
    output.decision =
        ExactDirectNormalizedH0AuthenticatedWindowStreamInitializationDecision::
            complete_provisional_bound_stream;
    return output;
  } catch (const std::bad_alloc&) {
    output.decision =
        ExactDirectNormalizedH0AuthenticatedWindowStreamInitializationDecision::
            no_allocation_failed;
    return output;
  } catch (const std::length_error&) {
    output.decision =
        ExactDirectNormalizedH0AuthenticatedWindowStreamInitializationDecision::
            no_budget_rejected;
    return output;
  }
}

}  // namespace morsehgp3d::hierarchy
