#include "morsehgp3d/hierarchy/direct_normalized_h0_source_forest_ledger_transaction.hpp"

#include <algorithm>
#include <limits>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

[[nodiscard]] bool checked_add(
    std::size_t left,
    std::size_t right,
    std::size_t& output) noexcept {
  if (left > std::numeric_limits<std::size_t>::max() - right) {
    return false;
  }
  output = left + right;
  return true;
}

[[nodiscard]] bool checked_multiply(
    std::size_t left,
    std::size_t right,
    std::size_t& output) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return false;
  }
  output = left * right;
  return true;
}

[[nodiscard]] bool valid_slice(
    std::size_t offset,
    std::size_t count,
    std::size_t extent) noexcept {
  return offset <= extent && count <= extent - offset;
}

[[nodiscard]] bool token_less(
    const ExactFrozenIncidenceToken& left,
    const ExactFrozenIncidenceToken& right) noexcept {
  if (left.kind != right.kind) {
    return static_cast<std::uint8_t>(left.kind) <
           static_cast<std::uint8_t>(right.kind);
  }
  return left.token_id < right.token_id;
}

[[nodiscard]] bool carrier_kind(
    ExactFrozenIncidenceTokenKind kind) noexcept {
  return kind == ExactFrozenIncidenceTokenKind::rooted_carrier ||
         kind == ExactFrozenIncidenceTokenKind::latent_carrier;
}

[[nodiscard]] bool strict_points(
    std::span<const spatial::PointId> points) noexcept {
  for (std::size_t index = 1U; index < points.size(); ++index) {
    if (points[index - 1U] >= points[index]) {
      return false;
    }
  }
  return true;
}

struct StandaloneBirthScratch {
  ExactDirectSparseStableFacetHandle handle{};
  ExactDirectSparseFacetKey key{};
};

}  // namespace

bool ExactDirectNormalizedH0SourceForestLedgerPreparedWindow::valid()
    const noexcept {
  if (!minted_ || source_session_identity_ == nullptr ||
      provider_identity_ == nullptr || !scientific_window_.has_value() ||
      !scientific_window_->valid()) {
    return false;
  }
  const auto& owned = scientific_window_->owned_window();
  return owned.exact_window_copied && owned.no_future_payload_owned &&
         owned.source_batch_index == source_batch_cursor_ &&
         owned.source_chain_digest == source_chain_digest_ &&
         owned.manifest_digest == manifest_digest_ &&
         owned.local_plan.batches.size() == 1U &&
         owned.local_plan.batches.front().batch_index ==
             source_batch_cursor_;
}

const ExactDirectNormalizedH0ResidentOwnedBatchWindow&
ExactDirectNormalizedH0SourceForestLedgerPreparedWindow::owned_window()
    const noexcept {
  static const ExactDirectNormalizedH0ResidentOwnedBatchWindow empty{};
  return scientific_window_.has_value() ? scientific_window_->owned_window()
                                        : empty;
}

std::size_t
ExactDirectNormalizedH0SourceForestLedgerPreparedWindow::source_batch_cursor()
    const noexcept {
  return source_batch_cursor_;
}

std::size_t ExactDirectNormalizedH0SourceForestLedgerPreparedWindow::
    source_epoch() const noexcept {
  return source_epoch_;
}

const contract::CanonicalId&
ExactDirectNormalizedH0SourceForestLedgerPreparedWindow::source_chain_digest()
    const noexcept {
  return source_chain_digest_;
}

bool ExactDirectNormalizedH0SourceForestLedgerWindowPreparationResult::
    certified_prepared_window() const noexcept {
  return window.has_value() && window->valid() &&
         source_batch_cursor == window->source_batch_cursor() &&
         source_epoch == window->source_epoch() &&
         source_chain_digest == window->source_chain_digest() &&
         source_window_prepared_exactly_once && source_cursor_unchanged &&
         !caller_supplied_source_ticket_accepted && !public_status_claimed &&
         decision ==
             ExactDirectNormalizedH0SourceForestLedgerWindowPreparationDecision::
                 complete_transaction_owned_scientific_window;
}

ExactDirectNormalizedH0SourceForestLedgerWindowPreparationResult
prepare_exact_direct_normalized_h0_source_forest_ledger_window(
    ExactDirectNormalizedH0ScientificWindowCapabilitySession& source_session)
    noexcept {
  ExactDirectNormalizedH0SourceForestLedgerWindowPreparationResult output;
  if (!source_session.certified_scientific_window_stream() ||
      source_session.sealed() || source_session.complete() ||
      !source_session.scientific_source_stamp()
           .certified_scientific_source_stamp()) {
    output.decision =
        ExactDirectNormalizedH0SourceForestLedgerWindowPreparationDecision::
            no_source_session_rejected;
    return output;
  }

  output.source_batch_cursor = source_session.batch_cursor();
  output.source_epoch = source_session.epoch();
  output.source_chain_digest = source_session.current_chain_digest();
  try {
    auto prepared = source_session.prepare_next();
    if (!prepared.certified_scientific_preparation() ||
        !prepared.ticket.has_value() ||
        source_session.batch_cursor() != output.source_batch_cursor ||
        source_session.epoch() != output.source_epoch ||
        source_session.current_chain_digest() != output.source_chain_digest) {
      output.decision =
          ExactDirectNormalizedH0SourceForestLedgerWindowPreparationDecision::
              no_source_window_rejected;
      return output;
    }

    ExactDirectNormalizedH0SourceForestLedgerPreparedWindow window;
    window.source_session_identity_ = &source_session;
    window.provider_identity_ = source_session.provider_identity();
    window.source_batch_cursor_ = output.source_batch_cursor;
    window.source_epoch_ = output.source_epoch;
    window.source_chain_digest_ = output.source_chain_digest;
    window.manifest_digest_ =
        source_session.scientific_source_stamp().manifest_digest();
    window.source_identity_digest_ =
        source_session.scientific_source_stamp().source_identity_digest();
    window.scientific_window_.emplace(std::move(*prepared.ticket));
    window.minted_ = true;
    if (!window.valid()) {
      output.decision =
          ExactDirectNormalizedH0SourceForestLedgerWindowPreparationDecision::
              no_source_window_rejected;
      return output;
    }
    output.window.emplace(std::move(window));
    output.source_window_prepared_exactly_once = true;
    output.source_cursor_unchanged = true;
    output.caller_supplied_source_ticket_accepted = false;
    output.public_status_claimed = false;
    output.decision =
        ExactDirectNormalizedH0SourceForestLedgerWindowPreparationDecision::
            complete_transaction_owned_scientific_window;
    return output;
  } catch (...) {
    output.decision =
        ExactDirectNormalizedH0SourceForestLedgerWindowPreparationDecision::
            no_source_window_rejected;
    return output;
  }
}

std::uint32_t
ExactDirectNormalizedH0SourceForestLedgerTransactionResult::schema_version()
    const noexcept {
  return schema_version_;
}

const ExactDirectNormalizedH0SourceForestLedgerTransactionBudget&
ExactDirectNormalizedH0SourceForestLedgerTransactionResult::requested_budget()
    const & noexcept {
  return requested_budget_;
}

const ExactDirectNormalizedH0SourceForestLedgerTransactionCounters&
ExactDirectNormalizedH0SourceForestLedgerTransactionResult::counters()
    const & noexcept {
  return counters_;
}

std::size_t
ExactDirectNormalizedH0SourceForestLedgerTransactionResult::source_batch_index()
    const noexcept {
  return source_batch_index_;
}

const ExactDirectSparseStableFacetForestStamp&
ExactDirectNormalizedH0SourceForestLedgerTransactionResult::forest_pre_stamp()
    const & noexcept {
  return forest_pre_stamp_;
}

const ExactDirectSparseStableFacetForestStamp&
ExactDirectNormalizedH0SourceForestLedgerTransactionResult::forest_post_stamp()
    const & noexcept {
  return forest_post_stamp_;
}

const ExactDirectSparseRootLedgerStamp&
ExactDirectNormalizedH0SourceForestLedgerTransactionResult::ledger_pre_stamp()
    const & noexcept {
  return ledger_pre_stamp_;
}

const ExactDirectSparseRootLedgerStamp&
ExactDirectNormalizedH0SourceForestLedgerTransactionResult::ledger_post_stamp()
    const & noexcept {
  return ledger_post_stamp_;
}

const ExactDirectSparseRootLedgerCommitResult&
ExactDirectNormalizedH0SourceForestLedgerTransactionResult::
    forest_ledger_commit() const & noexcept {
  return forest_ledger_commit_;
}

const ExactDirectNormalizedH0ScientificWindowCapabilityCommitResult&
ExactDirectNormalizedH0SourceForestLedgerTransactionResult::source_commit()
    const & noexcept {
  return source_commit_;
}

ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition
ExactDirectNormalizedH0SourceForestLedgerTransactionResult::disposition()
    const noexcept {
  return disposition_;
}

ExactDirectNormalizedH0SourceForestLedgerTransactionDecision
ExactDirectNormalizedH0SourceForestLedgerTransactionResult::decision()
    const noexcept {
  return decision_;
}

bool ExactDirectNormalizedH0SourceForestLedgerTransactionResult::
    equal_facet_authority_derived_from_scientific_window_and_exact_closure()
        const noexcept {
  return
      equal_facet_authority_derived_from_scientific_window_and_exact_closure_;
}

bool ExactDirectNormalizedH0SourceForestLedgerTransactionResult::
    carrier_and_equal_partition_exhaustive() const noexcept {
  return carrier_and_equal_partition_exhaustive_;
}

bool ExactDirectNormalizedH0SourceForestLedgerTransactionResult::
    all_fallible_work_completed_before_any_logical_commit() const noexcept {
  return all_fallible_work_completed_before_any_logical_commit_;
}

bool ExactDirectNormalizedH0SourceForestLedgerTransactionResult::
    source_commit_after_forest_ledger_commit() const noexcept {
  return source_commit_after_forest_ledger_commit_;
}

bool ExactDirectNormalizedH0SourceForestLedgerTransactionResult::
    partial_commit_requires_rebuild() const noexcept {
  return partial_commit_requires_rebuild_;
}

bool ExactDirectNormalizedH0SourceForestLedgerTransactionResult::
    durable_atomicity_claimed() const noexcept {
  return durable_atomicity_claimed_;
}

bool ExactDirectNormalizedH0SourceForestLedgerTransactionResult::
    global_forbidden_structure_materialized() const noexcept {
  return global_forbidden_structure_materialized_;
}

bool ExactDirectNormalizedH0SourceForestLedgerTransactionResult::
    durable_restart_supported() const noexcept {
  return durable_restart_supported_;
}

bool ExactDirectNormalizedH0SourceForestLedgerTransactionResult::
    vertical_maps_complete() const noexcept {
  return vertical_maps_complete_;
}

bool ExactDirectNormalizedH0SourceForestLedgerTransactionResult::
    public_status_claimed() const noexcept {
  return public_status_claimed_;
}

bool ExactDirectNormalizedH0SourceForestLedgerTransactionResult::
    certified_complete_transaction() const noexcept {
  std::size_t expected_cursor = 0U;
  std::size_t expected_equal_count = 0U;
  std::size_t expected_touched_count = 0U;
  return minted_ &&
         schema_version_ ==
             direct_normalized_h0_source_forest_ledger_transaction_schema_version &&
         checked_add(source_cursor_pre_, 1U, expected_cursor) &&
         checked_add(
             counters_.direct_birth_equal_facet_resolution_count,
             counters_.closure_exact_equal_facet_resolution_count,
             expected_equal_count) &&
         expected_equal_count == counters_.equal_facet_resolution_count &&
         checked_add(
             counters_.equal_facet_resolution_count,
             counters_.carrier_resolution_count,
             expected_touched_count) &&
         expected_touched_count == counters_.touched_facet_count &&
         source_cursor_post_ == expected_cursor &&
         source_batch_index_ == source_cursor_pre_ &&
         counters_.source_commit_attempt_count == 1U &&
         transaction_window_privately_minted_ &&
         equal_facet_authority_derived_from_scientific_window_and_exact_closure_ &&
         carrier_and_equal_partition_exhaustive_ &&
         carrier_origin_privately_derived_ &&
         relative_batch_and_projection_privately_bound_ &&
         root_ledger_transition_derived_from_frozen_batch_ &&
         all_fallible_work_completed_before_any_logical_commit_ &&
         forest_ledger_committed_ &&
         source_commit_after_forest_ledger_commit_ &&
         !partial_commit_requires_rebuild_ && !durable_atomicity_claimed_ &&
         forest_ledger_commit_.certified_commit() &&
         source_commit_.certified_scientific_commit() &&
         forest_ledger_commit_.pre_stamp == ledger_pre_stamp_ &&
         forest_ledger_commit_.post_stamp == ledger_post_stamp_ &&
         ledger_post_stamp_.aligned_forest_stamp == forest_post_stamp_ &&
         source_commit_.source_batch_index == source_batch_index_ &&
         source_commit_.committed_cursor == source_cursor_post_ &&
         source_commit_.committed_chain_digest == source_chain_post_ &&
         !global_forbidden_structure_materialized_ &&
         !durable_restart_supported_ && !vertical_maps_complete_ &&
         !source_exactness_claimed_ && !public_status_claimed_ &&
         disposition_ ==
             ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                 complete &&
         decision_ ==
             ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                 complete_forest_ledger_then_exactly_once_source_commit;
}

bool ExactDirectNormalizedH0SourceForestLedgerTransactionResult::
    certified_no_commit_failure() const noexcept {
  return minted_ &&
         schema_version_ ==
             direct_normalized_h0_source_forest_ledger_transaction_schema_version &&
         (disposition_ ==
              ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                  rejected ||
          disposition_ ==
              ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                  budget_exhausted ||
          disposition_ ==
              ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                  contradiction) &&
         decision_ !=
             ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                 contradiction_source_commit_after_forest_ledger_commit &&
         decision_ !=
             ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                 complete_forest_ledger_then_exactly_once_source_commit &&
         counters_.source_commit_attempt_count == 0U &&
         source_cursor_post_ == source_cursor_pre_ &&
         source_chain_post_ == source_chain_pre_ &&
         forest_post_stamp_ == forest_pre_stamp_ &&
         ledger_post_stamp_ == ledger_pre_stamp_ &&
         !forest_ledger_committed_ &&
         !source_commit_after_forest_ledger_commit_ &&
         !partial_commit_requires_rebuild_ && !durable_atomicity_claimed_ &&
         !global_forbidden_structure_materialized_ &&
         !durable_restart_supported_ && !vertical_maps_complete_ &&
         !source_exactness_claimed_ && !public_status_claimed_;
}

bool ExactDirectNormalizedH0SourceForestLedgerTransactionResult::
    certified_poisoned_partial_commit() const noexcept {
  std::size_t expected_cursor = 0U;
  return minted_ &&
         schema_version_ ==
             direct_normalized_h0_source_forest_ledger_transaction_schema_version &&
         checked_add(source_cursor_pre_, 1U, expected_cursor) &&
         source_batch_index_ == source_cursor_pre_ &&
         source_cursor_post_ == source_cursor_pre_ &&
         source_chain_post_ == source_chain_pre_ &&
         forest_post_stamp_.committed_batch_count == expected_cursor &&
         ledger_post_stamp_.committed_batch_count == expected_cursor &&
         ledger_post_stamp_.aligned_forest_stamp == forest_post_stamp_ &&
         counters_.source_commit_attempt_count == 1U &&
         transaction_window_privately_minted_ &&
         all_fallible_work_completed_before_any_logical_commit_ &&
         forest_ledger_committed_ &&
         !source_commit_after_forest_ledger_commit_ &&
         partial_commit_requires_rebuild_ && !durable_atomicity_claimed_ &&
         forest_ledger_commit_.certified_commit() &&
         !source_commit_.certified_scientific_commit() &&
         forest_ledger_commit_.pre_stamp == ledger_pre_stamp_ &&
         forest_ledger_commit_.post_stamp == ledger_post_stamp_ &&
         !global_forbidden_structure_materialized_ &&
         !durable_restart_supported_ && !vertical_maps_complete_ &&
         !source_exactness_claimed_ && !public_status_claimed_ &&
         disposition_ ==
             ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                 contradiction &&
         decision_ ==
             ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                 contradiction_source_commit_after_forest_ledger_commit;
}

bool ExactDirectNormalizedH0SourceForestLedgerTransactionResult::
    certified_outcome() const noexcept {
  return certified_complete_transaction() || certified_no_commit_failure() ||
         certified_poisoned_partial_commit();
}

ExactDirectNormalizedH0SourceForestLedgerTransactionResult
ExactDirectNormalizedH0SourceForestLedgerTransaction::execute(
    ExactDirectNormalizedH0ScientificWindowCapabilitySession& source_session,
    ExactDirectNormalizedH0SourceForestLedgerPreparedWindow prepared_window,
    const ExactDirectSparseStableFacetDescentClosureResult& candidate_closure,
    std::span<const ExactDirectSparseCarrierOriginSeedBinding>
        candidate_seed_bindings,
    ExactDirectSparseStableFacetForest& forest,
    ExactDirectSparseRootLedger& ledger,
    const ExactDirectNormalizedH0SourceForestLedgerTransactionBudget& budget)
    noexcept {
  ExactDirectNormalizedH0SourceForestLedgerTransactionResult output;
  output.requested_budget_ = budget;
  output.source_cursor_pre_ = source_session.batch_cursor();
  output.source_cursor_post_ = output.source_cursor_pre_;
  output.source_chain_pre_ = source_session.current_chain_digest();
  output.source_chain_post_ = output.source_chain_pre_;
  output.forest_pre_stamp_ = forest.current_stamp();
  output.forest_post_stamp_ = output.forest_pre_stamp_;
  output.ledger_pre_stamp_ = ledger.current_stamp();
  output.ledger_post_stamp_ = output.ledger_pre_stamp_;

  auto finish = [&](auto disposition, auto decision)
      -> ExactDirectNormalizedH0SourceForestLedgerTransactionResult {
    output.source_cursor_post_ = source_session.batch_cursor();
    output.source_chain_post_ = source_session.current_chain_digest();
    output.forest_post_stamp_ = forest.current_stamp();
    output.ledger_post_stamp_ = ledger.current_stamp();
    output.disposition_ = disposition;
    output.decision_ = decision;
    output.minted_ = true;
    prepared_window.scientific_window_.reset();
    return std::move(output);
  };

  if (!prepared_window.valid()) {
    return finish(
        ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
            rejected,
        ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
            no_prepared_window_rejected);
  }
  output.transaction_window_privately_minted_ = true;
  output.source_batch_index_ = prepared_window.source_batch_cursor_;

  const auto& source_stamp = source_session.scientific_source_stamp();
  if (prepared_window.source_session_identity_ != &source_session ||
      prepared_window.provider_identity_ != source_session.provider_identity() ||
      prepared_window.source_batch_cursor_ != source_session.batch_cursor() ||
      prepared_window.source_epoch_ != source_session.epoch() ||
      prepared_window.source_chain_digest_ !=
          source_session.current_chain_digest() ||
      prepared_window.manifest_digest_ != source_stamp.manifest_digest() ||
      prepared_window.source_identity_digest_ !=
          source_stamp.source_identity_digest()) {
    return finish(
        ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
            rejected,
        ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
            no_foreign_or_stale_source_window_rejected);
  }

  if (!source_session.certified_scientific_window_stream() ||
      !source_stamp.certified_scientific_source_stamp() ||
      !forest.certified_structure_only_forest() ||
      !ledger.certified_structure_only_ledger()) {
    return finish(
        ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
            rejected,
        ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
            no_forest_or_ledger_rejected);
  }
  if (output.ledger_pre_stamp_.aligned_forest_stamp !=
          output.forest_pre_stamp_ ||
      output.forest_pre_stamp_.source_identity_digest !=
          source_stamp.source_identity_digest() ||
      output.forest_pre_stamp_.stable_facet_token_count !=
          source_stamp.stable_facet_token_count() ||
      output.forest_pre_stamp_.committed_batch_count !=
          output.source_cursor_pre_ ||
      output.ledger_pre_stamp_.committed_batch_count !=
          output.source_cursor_pre_) {
    return finish(
        ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
            rejected,
        ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
            no_source_cursor_forest_ledger_alignment_rejected);
  }

  const auto& owned = prepared_window.scientific_window_->owned_window();
  if (owned.local_plan.batches.size() != 1U ||
      owned.local_to_stable_facet_token_indices.size() !=
          owned.local_plan.facet_tokens.size() ||
      owned.source_batch_index != output.source_cursor_pre_ ||
      owned.source_chain_digest != output.source_chain_pre_) {
    return finish(
        ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
            rejected,
        ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
            no_foreign_or_stale_source_window_rejected);
  }
  const auto& local_plan = owned.local_plan;
  const auto& plan_batch = local_plan.batches.front();
  output.counters_.window_facet_scan_count = local_plan.facet_tokens.size();
  output.counters_.direct_reference_scan_count =
      plan_batch.direct_reference_count;
  output.counters_.coface_facet_reference_scan_count =
      plan_batch.coface_facet_reference_count;

  std::size_t scratch_required = 0U;
  std::size_t classification_scratch = 0U;
  if (!checked_multiply(
          local_plan.facet_tokens.size(), 8U, classification_scratch) ||
      !checked_add(
          classification_scratch,
          plan_batch.direct_reference_count,
          scratch_required) ||
      !checked_add(
          scratch_required,
          plan_batch.coface_facet_reference_count,
          scratch_required)) {
    return finish(
        ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
            budget_exhausted,
        ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
            no_composition_budget_exhausted);
  }
  if (local_plan.facet_tokens.size() >
          budget.maximum_window_facet_scan_count ||
      plan_batch.direct_reference_count >
          budget.maximum_direct_reference_scan_count ||
      plan_batch.coface_facet_reference_count >
          budget.maximum_coface_facet_reference_scan_count ||
      scratch_required > budget.maximum_scratch_entry_count ||
      !valid_slice(
          plan_batch.direct_reference_offset,
          plan_batch.direct_reference_count,
          local_plan.direct_references.size()) ||
      !valid_slice(
          plan_batch.coface_facet_reference_offset,
          plan_batch.coface_facet_reference_count,
          local_plan.coface_facet_references.size())) {
    return finish(
        ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
            budget_exhausted,
        ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
            no_composition_budget_exhausted);
  }

  try {
    std::vector<std::uint8_t> direct_birth(local_plan.facet_tokens.size(), 0U);
    std::vector<std::uint8_t> touched(local_plan.facet_tokens.size(), 0U);
    for (std::size_t local = 0U; local < local_plan.facet_tokens.size();
         ++local) {
      if (local_plan.facet_tokens[local].facet_token_index != local ||
          owned.local_to_stable_facet_token_indices[local] >=
              source_stamp.stable_facet_token_count() ||
          (local != 0U &&
           owned.local_to_stable_facet_token_indices[local - 1U] >=
               owned.local_to_stable_facet_token_indices[local])) {
        return finish(
            ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                rejected,
            ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                no_carrier_closure_partition_rejected);
      }
    }

    const std::size_t direct_end =
        plan_batch.direct_reference_offset +
        plan_batch.direct_reference_count;
    for (std::size_t index = plan_batch.direct_reference_offset;
         index < direct_end;
         ++index) {
      const auto& reference = local_plan.direct_references[index];
      if (reference.role == ExactDirectMorseH0Role::birth) {
        if (!reference.direct_birth_facet_token_index.has_value() ||
            *reference.direct_birth_facet_token_index >=
                direct_birth.size() ||
            direct_birth[*reference.direct_birth_facet_token_index] != 0U) {
          return finish(
              ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                  rejected,
              ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                  no_carrier_closure_partition_rejected);
        }
        direct_birth[*reference.direct_birth_facet_token_index] = 1U;
      } else if (reference.role != ExactDirectMorseH0Role::saddle ||
                 reference.direct_birth_facet_token_index.has_value()) {
        return finish(
            ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                rejected,
            ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                no_carrier_closure_partition_rejected);
      }
    }

    const std::size_t coface_end =
        plan_batch.coface_facet_reference_offset +
        plan_batch.coface_facet_reference_count;
    for (std::size_t index = plan_batch.coface_facet_reference_offset;
         index < coface_end;
         ++index) {
      const std::size_t local =
          local_plan.coface_facet_references[index].facet_token_index;
      if (local >= touched.size()) {
        return finish(
            ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                rejected,
            ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                no_carrier_closure_partition_rejected);
      }
      touched[local] = 1U;
    }

    std::vector<std::size_t> expected_candidate_stable_handles;
    std::vector<ExactDirectNormalizedH0StableFacetResolution>
        equal_resolutions;
    std::vector<StandaloneBirthScratch> standalone_birth_scratch;
    expected_candidate_stable_handles.reserve(local_plan.facet_tokens.size());
    equal_resolutions.reserve(local_plan.facet_tokens.size());
    standalone_birth_scratch.reserve(local_plan.facet_tokens.size());
    for (std::size_t local = 0U; local < local_plan.facet_tokens.size();
         ++local) {
      const auto stable = owned.local_to_stable_facet_token_indices[local];
      if (touched[local] != 0U) {
        ++output.counters_.touched_facet_count;
        if (direct_birth[local] != 0U) {
          if (!std::in_range<ExactFrozenIncidenceTokenId>(stable)) {
            return finish(
                ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                    rejected,
                ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                    no_carrier_closure_partition_rejected);
          }
          equal_resolutions.push_back(
              {stable,
               {ExactFrozenIncidenceTokenKind::equal_facet,
                static_cast<ExactFrozenIncidenceTokenId>(stable)},
               std::nullopt});
          ++output.counters_.direct_birth_equal_facet_resolution_count;
        } else {
          expected_candidate_stable_handles.push_back(stable);
        }
      } else if (direct_birth[local] != 0U) {
        standalone_birth_scratch.push_back(
            {stable, local_plan.facet_tokens[local].facet_key});
      } else {
        // Every resident window token must be used by this exact batch.  A
        // token that is neither a direct birth nor a factorized coface facet
        // would be unauthorised staging outside the one-window source.
        return finish(
            ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                rejected,
            ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                no_carrier_closure_partition_rejected);
      }
    }
    output.counters_.standalone_birth_count =
        standalone_birth_scratch.size();
    std::size_t standalone_birth_point_reference_count = 0U;
    for (const auto& birth : standalone_birth_scratch) {
      if (!checked_add(
              standalone_birth_point_reference_count,
              birth.key.point_count,
              standalone_birth_point_reference_count)) {
        return finish(
            ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                budget_exhausted,
            ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                no_composition_budget_exhausted);
      }
    }
    output.counters_.standalone_birth_point_reference_count =
        standalone_birth_point_reference_count;
    if (output.counters_.touched_facet_count >
            budget.maximum_merged_resolution_count ||
        standalone_birth_scratch.size() >
            budget.maximum_standalone_birth_count ||
        standalone_birth_point_reference_count >
            budget.maximum_standalone_birth_point_reference_count) {
      return finish(
          ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
              budget_exhausted,
          ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              no_composition_budget_exhausted);
    }

    if (!candidate_closure.certified_complete_relative_positive_closure() ||
        !candidate_closure.certified_for(output.forest_pre_stamp_) ||
        candidate_closure.closed_batch_squared_level() !=
            plan_batch.squared_level ||
        candidate_seed_bindings.size() !=
            expected_candidate_stable_handles.size() ||
        candidate_closure.seed_projections().size() !=
            candidate_seed_bindings.size() ||
        (!candidate_seed_bindings.empty() &&
         candidate_closure.common_facet_cardinality() != plan_batch.order)) {
      return finish(
          ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
              rejected,
          ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              no_carrier_closure_partition_rejected);
    }
    const auto closure_nodes = candidate_closure.nodes();
    const auto closure_projections = candidate_closure.seed_projections();
    std::vector<ExactDirectSparseCarrierOriginSeedBinding>
        strict_carrier_bindings;
    std::vector<std::size_t> expected_carrier_stable_handles;
    strict_carrier_bindings.reserve(candidate_seed_bindings.size());
    expected_carrier_stable_handles.reserve(candidate_seed_bindings.size());
    for (std::size_t index = 0U; index < candidate_seed_bindings.size();
         ++index) {
      const auto& binding = candidate_seed_bindings[index];
      const auto& projection = closure_projections[index];
      if (binding.closure_seed_index != index ||
          binding.stable_source_facet_token_index !=
              expected_candidate_stable_handles[index] ||
          projection.seed_index != index ||
          projection.root_node_index >= closure_nodes.size()) {
        return finish(
            ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                rejected,
            ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                no_carrier_closure_partition_rejected);
      }
      const auto stable_found = std::lower_bound(
          owned.local_to_stable_facet_token_indices.begin(),
          owned.local_to_stable_facet_token_indices.end(),
          binding.stable_source_facet_token_index);
      if (stable_found ==
              owned.local_to_stable_facet_token_indices.end() ||
          *stable_found != binding.stable_source_facet_token_index) {
        return finish(
            ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                rejected,
            ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                no_carrier_closure_partition_rejected);
      }
      const std::size_t local = static_cast<std::size_t>(
          stable_found - owned.local_to_stable_facet_token_indices.begin());
      const auto& source_node = closure_nodes[projection.root_node_index];
      if (source_node.facet_key != local_plan.facet_tokens[local].facet_key) {
        return finish(
            ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                rejected,
            ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              no_carrier_closure_partition_rejected);
      }
      if (source_node.exact_squared_level.has_value()) {
        if (*source_node.exact_squared_level == plan_batch.squared_level) {
          const auto stable = binding.stable_source_facet_token_index;
          if (!std::in_range<ExactFrozenIncidenceTokenId>(stable)) {
            return finish(
                ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                    contradiction,
                ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                    contradiction_equal_facet_authority);
          }
          equal_resolutions.push_back(
              {stable,
               {ExactFrozenIncidenceTokenKind::equal_facet,
                static_cast<ExactFrozenIncidenceTokenId>(stable)},
               std::nullopt});
          ++output.counters_.closure_exact_equal_facet_resolution_count;
          continue;
        }
        if (!(*source_node.exact_squared_level < plan_batch.squared_level)) {
          return finish(
              ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                  contradiction,
              ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                  contradiction_equal_facet_authority);
        }
      } else if (
          source_node.kind !=
              ExactDirectSparseStableFacetDescentNodeKind::
                  stable_positive_terminal ||
          source_node.local_decision !=
              ExactDirectSparseStableFacetDescentLocalDecision::
                  complete_source_stable_positive_hit ||
          !source_node.stable_positive_attachment.has_value()) {
        return finish(
            ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                contradiction,
            ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                contradiction_equal_facet_authority);
      }
      strict_carrier_bindings.push_back(binding);
      expected_carrier_stable_handles.push_back(
          binding.stable_source_facet_token_index);
    }

    std::sort(
        equal_resolutions.begin(),
        equal_resolutions.end(),
        [](const auto& left, const auto& right) {
          return left.stable_source_facet_token_index <
                 right.stable_source_facet_token_index;
        });
    if (std::adjacent_find(
            equal_resolutions.begin(),
            equal_resolutions.end(),
            [](const auto& left, const auto& right) {
              return left.stable_source_facet_token_index ==
                     right.stable_source_facet_token_index;
            }) != equal_resolutions.end()) {
      return finish(
          ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
              contradiction,
          ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              contradiction_equal_facet_authority);
    }
    output.counters_.equal_facet_resolution_count = equal_resolutions.size();
    output.counters_.carrier_resolution_count =
        expected_carrier_stable_handles.size();
    if (equal_resolutions.size() >
        budget.maximum_equal_facet_resolution_count) {
      return finish(
          ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
              budget_exhausted,
          ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              no_composition_budget_exhausted);
    }
    output
        .equal_facet_authority_derived_from_scientific_window_and_exact_closure_ =
        true;

    auto carrier_origin =
        derive_exact_direct_sparse_carrier_origin_capability(
            candidate_closure,
            strict_carrier_bindings,
            forest,
            ledger,
            budget.carrier_origin);
    if (!carrier_origin.certified_exact_carrier_origin_capability() ||
        !carrier_origin.certified_for(
            output.forest_pre_stamp_, output.ledger_pre_stamp_)) {
      const auto disposition = carrier_origin.certified_budget_exhaustion()
                                   ? ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                                         budget_exhausted
                                   : ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                                         rejected;
      return finish(
          disposition,
          ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              no_carrier_origin_rejected);
    }
    output.carrier_origin_privately_derived_ = true;
    const auto carrier_resolutions =
        carrier_origin.carrier_stable_facet_resolutions();
    if (carrier_resolutions.size() !=
        expected_carrier_stable_handles.size()) {
      return finish(
          ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
              contradiction,
          ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              no_carrier_origin_rejected);
    }

    std::vector<ExactDirectNormalizedH0StableFacetResolution>
        merged_resolutions;
    merged_resolutions.reserve(output.counters_.touched_facet_count);
    std::size_t carrier_cursor = 0U;
    std::size_t equal_cursor = 0U;
    while (carrier_cursor < carrier_resolutions.size() ||
           equal_cursor < equal_resolutions.size()) {
      const bool use_carrier =
          equal_cursor == equal_resolutions.size() ||
          (carrier_cursor < carrier_resolutions.size() &&
           carrier_resolutions[carrier_cursor]
                   .stable_source_facet_token_index <
               equal_resolutions[equal_cursor]
                   .stable_source_facet_token_index);
      if (use_carrier) {
        merged_resolutions.push_back(carrier_resolutions[carrier_cursor++]);
      } else {
        merged_resolutions.push_back(equal_resolutions[equal_cursor++]);
      }
    }
    output.counters_.merged_resolution_count = merged_resolutions.size();
    if (merged_resolutions.size() != output.counters_.touched_facet_count ||
        merged_resolutions.size() > budget.maximum_merged_resolution_count) {
      return finish(
          ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
              rejected,
          ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              no_carrier_closure_partition_rejected);
    }
    std::size_t observed_touched = 0U;
    for (std::size_t local = 0U; local < touched.size(); ++local) {
      if (touched[local] == 0U) {
        continue;
      }
      if (observed_touched >= merged_resolutions.size() ||
          merged_resolutions[observed_touched]
                  .stable_source_facet_token_index !=
              owned.local_to_stable_facet_token_indices[local]) {
        return finish(
            ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                rejected,
            ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                no_carrier_closure_partition_rejected);
      }
      ++observed_touched;
    }
    output.carrier_and_equal_partition_exhaustive_ =
        observed_touched == merged_resolutions.size();
    if (!output.carrier_and_equal_partition_exhaustive_) {
      return finish(
          ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
              rejected,
          ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              no_carrier_closure_partition_rejected);
    }

    auto relative =
        ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuilder::build(
            *prepared_window.scientific_window_,
            merged_resolutions,
            carrier_origin.prior_root_coverages(),
            carrier_origin.prior_root_coverage_point_references(),
            carrier_origin.latent_carrier_coverages(),
            carrier_origin.latent_carrier_coverage_point_references(),
            budget.relative_batch);
    if (!relative.certified_scientific_relative_build() ||
        !relative.batch.has_value()) {
      const bool exhausted =
          relative.decision ==
              ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                  no_translation_budget_rejected;
      return finish(
          exhausted
              ? ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                    budget_exhausted
              : ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                    rejected,
          ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              no_relative_frozen_batch_rejected);
    }

    auto projection = ExactDirectNormalizedH0SparseForestProjection::prepare(
        source_stamp,
        *prepared_window.scientific_window_,
        *relative.batch,
        forest,
        carrier_origin.carrier_attachments(),
        budget.forest_projection);
    if (!projection.certified_prepared_projection() ||
        !projection.has_forest_ticket()) {
      const bool exhausted =
          projection.decision ==
              ExactDirectNormalizedH0SparseForestProjectionDecision::
                  no_projection_budget_exhausted;
      return finish(
          exhausted
              ? ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                    budget_exhausted
              : ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                    rejected,
          ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              no_sparse_forest_projection_rejected);
    }
    output.relative_batch_and_projection_privately_bound_ = true;
    const auto& frozen = relative.batch->frozen_batch();
    if (frozen.quotient.groups.size() != frozen.coverage_deltas.size() ||
        frozen.quotient.groups.size() != frozen.action_plan.groups.size()) {
      return finish(
          ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
              contradiction,
          ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              no_root_ledger_transition_derivation_rejected);
    }
    output.counters_.group_count = frozen.quotient.groups.size();
    output.counters_.group_token_scan_count =
        frozen.quotient.group_tokens.size();
    if (frozen.quotient.groups.size() > budget.maximum_group_count ||
        frozen.quotient.group_tokens.size() >
            budget.maximum_group_token_scan_count) {
      return finish(
          ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
              budget_exhausted,
          ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              no_composition_budget_exhausted);
    }

    const auto attachments = carrier_origin.carrier_attachments();
    std::size_t twice_group_token_scratch = 0U;
    std::size_t preview_handle_scratch = 0U;
    std::size_t total_composition_scratch = scratch_required;
    if (!checked_multiply(
            frozen.quotient.group_tokens.size(),
            2U,
            twice_group_token_scratch) ||
        !checked_add(
            owned.local_to_stable_facet_token_indices.size(),
            attachments.size(),
            preview_handle_scratch) ||
        !checked_add(
            total_composition_scratch,
            frozen.quotient.groups.size(),
            total_composition_scratch) ||
        !checked_add(
            total_composition_scratch,
            twice_group_token_scratch,
            total_composition_scratch) ||
        !checked_add(
            total_composition_scratch,
            frozen.coverage_delta_points.size(),
            total_composition_scratch) ||
        !checked_add(
            total_composition_scratch,
            standalone_birth_scratch.size(),
            total_composition_scratch) ||
        !checked_add(
            total_composition_scratch,
            standalone_birth_point_reference_count,
            total_composition_scratch) ||
        !checked_add(
            total_composition_scratch,
            preview_handle_scratch,
            total_composition_scratch) ||
        total_composition_scratch > budget.maximum_scratch_entry_count ||
        frozen.coverage_delta_points.size() >
            budget.maximum_group_point_delta_reference_count) {
      return finish(
          ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
              budget_exhausted,
          ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              no_composition_budget_exhausted);
    }
    const auto find_attachment = [&](const ExactFrozenIncidenceToken& token) {
      return std::lower_bound(
          attachments.begin(),
          attachments.end(),
          token,
          [](const auto& candidate, const auto& value) {
            return token_less(candidate.token, value);
          });
    };
    const auto find_equal = [&](ExactFrozenIncidenceTokenId token_id) {
      return std::lower_bound(
          equal_resolutions.begin(),
          equal_resolutions.end(),
          token_id,
          [](const auto& candidate, ExactFrozenIncidenceTokenId value) {
            return candidate.token.token_id < value;
          });
    };

    std::vector<ExactDirectSparseRootLedgerGroupTransition> groups;
    std::vector<ExactDirectSparseStableFacetHandle> parent_roots;
    std::vector<spatial::PointId> group_point_deltas;
    groups.reserve(frozen.quotient.groups.size());
    parent_roots.reserve(std::min(
        frozen.quotient.group_tokens.size(),
        budget.maximum_parent_root_reference_count));
    group_point_deltas.reserve(frozen.coverage_delta_points.size());
    for (std::size_t group_index = 0U;
         group_index < frozen.quotient.groups.size();
         ++group_index) {
      const auto& group = frozen.quotient.groups[group_index];
      const auto& action = frozen.action_plan.groups[group_index];
      const auto& delta = frozen.coverage_deltas[group_index];
      if (group.group_index != group_index ||
          action.group_index != group_index ||
          delta.coverage_delta_record_index != group_index ||
          delta.owner_group_index != group_index ||
          action.q_r != group.rooted_carrier_count ||
          delta.q_r != group.rooted_carrier_count ||
          delta.action != action.action || group.token_count == 0U ||
          !valid_slice(
              group.token_offset,
              group.token_count,
              frozen.quotient.group_tokens.size()) ||
          !valid_slice(
              delta.point_reference_offset,
              delta.point_reference_count,
              frozen.coverage_delta_points.size())) {
        return finish(
            ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                contradiction,
            ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                no_root_ledger_transition_derivation_rejected);
      }

      std::size_t expected_carrier_count = 0U;
      if (!checked_add(
              group.rooted_carrier_count,
              group.latent_carrier_count,
              expected_carrier_count) ||
          expected_carrier_count > group.token_count) {
        return finish(
            ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                contradiction,
            ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                no_root_ledger_transition_derivation_rejected);
      }
      if (parent_roots.size() >
              budget.maximum_parent_root_reference_count ||
          expected_carrier_count >
              budget.maximum_parent_root_reference_count -
                  parent_roots.size()) {
        return finish(
            ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                budget_exhausted,
            ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                no_composition_budget_exhausted);
      }
      std::vector<ExactDirectSparseStableFacetHandle> local_parents;
      local_parents.reserve(expected_carrier_count);
      std::optional<ExactDirectSparseStableFacetHandle> representative;
      std::size_t rooted_count = 0U;
      std::size_t latent_count = 0U;
      std::size_t equal_count = 0U;
      for (std::size_t offset = 0U; offset < group.token_count; ++offset) {
        const auto& token =
            frozen.quotient.group_tokens[group.token_offset + offset];
        ExactDirectSparseStableFacetHandle anchor = 0U;
        if (carrier_kind(token.kind)) {
          const auto found = find_attachment(token);
          if (found == attachments.end() || found->token != token) {
            return finish(
                ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                    contradiction,
                ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                    no_root_ledger_transition_derivation_rejected);
          }
          anchor = found->pre_batch_root_handle;
          local_parents.push_back(anchor);
          if (token.kind ==
              ExactFrozenIncidenceTokenKind::rooted_carrier) {
            ++rooted_count;
          } else {
            ++latent_count;
          }
        } else if (token.kind ==
                   ExactFrozenIncidenceTokenKind::equal_facet) {
          const auto found = find_equal(token.token_id);
          if (found == equal_resolutions.end() ||
              found->token.token_id != token.token_id ||
              found->token.kind != token.kind) {
            return finish(
                ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                    contradiction,
                ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                    no_root_ledger_transition_derivation_rejected);
          }
          anchor = found->stable_source_facet_token_index;
          ++equal_count;
        } else {
          return finish(
              ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                  contradiction,
              ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                  no_root_ledger_transition_derivation_rejected);
        }
        representative = representative.has_value()
                             ? std::min(*representative, anchor)
                             : anchor;
      }
      std::sort(local_parents.begin(), local_parents.end());
      local_parents.erase(
          std::unique(local_parents.begin(), local_parents.end()),
          local_parents.end());
      if (!representative.has_value() || local_parents.empty() ||
          local_parents.size() != expected_carrier_count ||
          rooted_count != group.rooted_carrier_count ||
          latent_count != group.latent_carrier_count ||
          equal_count != group.equal_facet_count) {
        return finish(
            ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                contradiction,
            ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                no_root_ledger_transition_derivation_rejected);
      }

      ExactDirectSparseRootLedgerGroupTransition transition;
      transition.source_group_index = group_index;
      transition.representative_handle = *representative;
      transition.parent_root_offset = parent_roots.size();
      transition.parent_root_count = local_parents.size();
      transition.point_delta_offset = group_point_deltas.size();
      transition.point_delta_count = delta.point_reference_count;
      transition.facet_delta_count = delta.facet_reference_count;
      parent_roots.insert(
          parent_roots.end(), local_parents.begin(), local_parents.end());
      const std::size_t delta_end =
          delta.point_reference_offset + delta.point_reference_count;
      for (std::size_t index = delta.point_reference_offset;
           index < delta_end;
           ++index) {
        const auto& point = frozen.coverage_delta_points[index];
        if (point.owner_group_index != group_index) {
          return finish(
              ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                  contradiction,
              ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                  no_root_ledger_transition_derivation_rejected);
        }
        group_point_deltas.push_back(point.point_id);
      }
      if (!strict_points(std::span<const spatial::PointId>{group_point_deltas}
                             .subspan(
                                 transition.point_delta_offset,
                                 transition.point_delta_count))) {
        return finish(
            ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                contradiction,
            ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
                no_root_ledger_transition_derivation_rejected);
      }
      groups.push_back(transition);
    }
    output.counters_.parent_root_reference_count = parent_roots.size();
    output.counters_.group_point_delta_reference_count =
        group_point_deltas.size();
    if (parent_roots.size() >
            budget.maximum_parent_root_reference_count ||
        group_point_deltas.size() >
            budget.maximum_group_point_delta_reference_count) {
      return finish(
          ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
              budget_exhausted,
          ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              no_composition_budget_exhausted);
    }

    std::sort(
        standalone_birth_scratch.begin(),
        standalone_birth_scratch.end(),
        [](const auto& left, const auto& right) {
          return left.handle < right.handle;
        });
    std::vector<ExactDirectSparseRootLedgerStandaloneBirth>
        standalone_births;
    std::vector<spatial::PointId> standalone_birth_points;
    standalone_births.reserve(standalone_birth_scratch.size());
    standalone_birth_points.reserve(standalone_birth_point_reference_count);
    for (const auto& birth : standalone_birth_scratch) {
      ExactDirectSparseRootLedgerStandaloneBirth record;
      record.representative_handle = birth.handle;
      record.point_delta_offset = standalone_birth_points.size();
      record.point_delta_count = birth.key.point_count;
      record.facet_delta_count = 1U;
      standalone_birth_points.insert(
          standalone_birth_points.end(),
          birth.key.point_ids.begin(),
          birth.key.point_ids.begin() +
              static_cast<std::ptrdiff_t>(birth.key.point_count));
      standalone_births.push_back(record);
    }
    if (standalone_birth_points.size() !=
        standalone_birth_point_reference_count) {
      return finish(
          ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
              contradiction,
          ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              no_root_ledger_transition_derivation_rejected);
    }

    std::vector<ExactDirectSparseStableFacetHandle> preview_handles;
    preview_handles.reserve(preview_handle_scratch);
    preview_handles.insert(
        preview_handles.end(),
        owned.local_to_stable_facet_token_indices.begin(),
        owned.local_to_stable_facet_token_indices.end());
    for (const auto& attachment : attachments) {
      preview_handles.push_back(attachment.pre_batch_root_handle);
    }
    std::sort(preview_handles.begin(), preview_handles.end());
    preview_handles.erase(
        std::unique(preview_handles.begin(), preview_handles.end()),
        preview_handles.end());
    output.counters_.preview_requested_handle_count = preview_handles.size();
    if (preview_handles.size() >
        budget.maximum_preview_requested_handle_count) {
      return finish(
          ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
              budget_exhausted,
          ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              no_composition_budget_exhausted);
    }

    auto forest_ticket = projection.take_forest_ticket();
    if (!forest_ticket.has_value() || !forest_ticket->valid()) {
      return finish(
          ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
              contradiction,
          ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              no_sparse_forest_projection_rejected);
    }
    auto preview = forest.preview_prepared_batch(
        *forest_ticket, preview_handles, budget.forest_preview);
    if (!preview.certified_prepared_preview() ||
        !preview.preview.has_value()) {
      const bool exhausted =
          preview.decision ==
              ExactDirectSparseStableFacetForestPreparedPreviewDecision::
                  no_budget_exhausted;
      return finish(
          exhausted
              ? ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                    budget_exhausted
              : ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                    rejected,
          ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              no_sparse_forest_preview_rejected);
    }
    output.root_ledger_transition_derived_from_frozen_batch_ = true;

    auto prepared_ledger = ledger.prepare_transition(
        forest,
        std::move(*forest_ticket),
        std::move(*preview.preview),
        groups,
        parent_roots,
        group_point_deltas,
        standalone_births,
        standalone_birth_points);
    if (!prepared_ledger.certified_prepared() ||
        !prepared_ledger.ticket.has_value()) {
      const bool exhausted =
          prepared_ledger.decision ==
              ExactDirectSparseRootLedgerPreparationDecision::
                  no_budget_exhausted ||
          prepared_ledger.decision ==
              ExactDirectSparseRootLedgerPreparationDecision::
                  no_outstanding_ticket_budget_exhausted;
      return finish(
          exhausted
              ? ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                    budget_exhausted
              : ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
                    rejected,
          ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              no_root_ledger_preparation_rejected);
    }

    // No allocations, source reads, closure work, quotient construction,
    // projection, preview or ledger preparation occur after this boundary.
    // Both following commit calls are noexcept.  External serialization is a
    // contract precondition, and every private ticket/stamp is rechecked now.
    if (!prepared_window.valid() ||
        prepared_window.source_session_identity_ != &source_session ||
        !source_session.certified_scientific_window_stream() ||
        source_session.complete() || source_session.sealed() ||
        source_session.batch_cursor() != output.source_cursor_pre_ ||
        source_session.epoch() != prepared_window.source_epoch_ ||
        source_session.current_chain_digest() != output.source_chain_pre_ ||
        forest.current_stamp() != output.forest_pre_stamp_ ||
        ledger.current_stamp() != output.ledger_pre_stamp_ ||
        !prepared_ledger.ticket->valid() ||
        prepared_ledger.ticket->pre_stamp() != output.ledger_pre_stamp_) {
      return finish(
          ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
              rejected,
          ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              no_precommit_stale_or_sibling_rejected);
    }
    output.all_fallible_work_completed_before_any_logical_commit_ = true;

    output.forest_ledger_commit_ = ledger.commit(
        std::move(*prepared_ledger.ticket), forest);
    if (!output.forest_ledger_commit_.certified_commit()) {
      return finish(
          ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
              contradiction,
          ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              no_root_ledger_commit_rejected);
    }
    output.forest_ledger_committed_ = true;

    ++output.counters_.source_commit_attempt_count;
    output.source_commit_ = source_session.commit(
        std::move(*prepared_window.scientific_window_));
    prepared_window.scientific_window_.reset();
    if (!output.source_commit_.certified_scientific_commit()) {
      // Every documented in-process precondition has just been revalidated,
      // so this branch denotes an invariant violation, not a retryable
      // rejection.  Forest and ledger have advanced already: advertise the
      // poisoned seam explicitly and require reconstruction of all three
      // objects before any further call.
      output.partial_commit_requires_rebuild_ = true;
      return finish(
          ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
              contradiction,
          ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
              contradiction_source_commit_after_forest_ledger_commit);
    }
    output.source_commit_after_forest_ledger_commit_ = true;
    output.global_forbidden_structure_materialized_ = false;
    output.durable_restart_supported_ = false;
    output.vertical_maps_complete_ = false;
    output.source_exactness_claimed_ = false;
    output.public_status_claimed_ = false;
    return finish(
        ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
            complete,
        ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
            complete_forest_ledger_then_exactly_once_source_commit);
  } catch (const std::bad_alloc&) {
    return finish(
        ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
            rejected,
        ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
            no_root_ledger_transition_derivation_rejected);
  } catch (const std::length_error&) {
    return finish(
        ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
            budget_exhausted,
        ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
            no_composition_budget_exhausted);
  } catch (...) {
    return finish(
        ExactDirectNormalizedH0SourceForestLedgerTransactionDisposition::
            contradiction,
        ExactDirectNormalizedH0SourceForestLedgerTransactionDecision::
            no_root_ledger_transition_derivation_rejected);
  }
}

}  // namespace morsehgp3d::hierarchy
