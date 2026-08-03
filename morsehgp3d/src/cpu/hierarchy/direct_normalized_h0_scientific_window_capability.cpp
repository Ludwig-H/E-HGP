#include "morsehgp3d/hierarchy/direct_normalized_h0_scientific_window_capability.hpp"

#include "direct_frozen_unified_incidence_batch_internal.hpp"

#include <exception>
#include <new>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

struct ScientificWindowControl {
  ExactDirectNormalizedH0ResidentSourceManifest manifest{};
  const void* provider_identity{};
  std::uint64_t scientific_authority_id{};
  bool horizontal_incidence_authority_freshly_verified{false};
  bool expected_manifest_freshly_reconstructed{false};
};

[[nodiscard]] bool budget_has_bounded_rebuild_and_one_window(
    const ExactDirectNormalizedH0ScientificWindowCapabilityBudget& budget)
    noexcept {
  return budget.maximum_compatibility_facet_count != 0U &&
         budget.maximum_compatibility_batch_count != 0U &&
         budget.stream.maximum_outstanding_ticket_count == 1U &&
         budget.stream.provider_window.maximum_batch_scan_count != 0U &&
         budget.stream.provider_window.maximum_window_facet_count != 0U &&
         budget.stream.provider_window.maximum_window_facet_point_count !=
             0U &&
         budget.stream.provider_window.maximum_window_logical_entry_count !=
             0U;
}

[[nodiscard]] bool control_is_scientific(
    const ScientificWindowControl* control) noexcept {
  return control != nullptr && control->scientific_authority_id != 0U &&
         control->provider_identity != nullptr &&
         control->manifest.certified() &&
         control->horizontal_incidence_authority_freshly_verified &&
         control->expected_manifest_freshly_reconstructed;
}

[[nodiscard]] bool control_is_scientific(
    const std::shared_ptr<const ScientificWindowControl>& control) noexcept {
  return control_is_scientific(control.get());
}

}  // namespace

ExactDirectNormalizedH0ScientificSourceStamp::
    ExactDirectNormalizedH0ScientificSourceStamp() noexcept = default;
ExactDirectNormalizedH0ScientificSourceStamp::
    ~ExactDirectNormalizedH0ScientificSourceStamp() = default;
ExactDirectNormalizedH0ScientificSourceStamp::
    ExactDirectNormalizedH0ScientificSourceStamp(
        ExactDirectNormalizedH0ScientificSourceStamp&&) noexcept = default;
ExactDirectNormalizedH0ScientificSourceStamp&
ExactDirectNormalizedH0ScientificSourceStamp::operator=(
    ExactDirectNormalizedH0ScientificSourceStamp&&) noexcept = default;

ExactDirectNormalizedH0ScientificSourceStamp::
    ExactDirectNormalizedH0ScientificSourceStamp(
        std::shared_ptr<const void> private_control_identity) noexcept
    : private_control_identity_(std::move(private_control_identity)) {
  const auto* control = static_cast<const ScientificWindowControl*>(
      private_control_identity_.get());
  if (!control_is_scientific(control)) {
    private_control_identity_.reset();
    return;
  }
  schema_version_ =
      direct_normalized_h0_scientific_window_capability_schema_version;
  scientific_authority_id_ = control->scientific_authority_id;
  manifest_digest_ = control->manifest.manifest_digest;
  source_identity_digest_ = control->manifest.source_identity_digest;
  horizontal_incidence_authority_digest_ =
      control->manifest.horizontal_incidence_authority_digest;
  stable_facet_token_count_ = control->manifest.stable_facet_token_count;
  batch_count_ = control->manifest.batch_count;
  initial_chain_digest_ = control->manifest.initial_batch_chain_digest;
  final_chain_digest_ = control->manifest.final_batch_chain_digest;
  private_scientific_attestation_issued_ = true;
  vertical_maps_complete_ = false;
  durable_restart_supported_ = false;
  public_status_claimed_ = false;
}

bool ExactDirectNormalizedH0ScientificSourceStamp::
    certified_scientific_source_stamp() const noexcept {
  const auto* control = static_cast<const ScientificWindowControl*>(
      private_control_identity_.get());
  return private_scientific_attestation_issued_ &&
         control_is_scientific(control) &&
         schema_version_ ==
             direct_normalized_h0_scientific_window_capability_schema_version &&
         scientific_authority_id_ == control->scientific_authority_id &&
         manifest_digest_ == control->manifest.manifest_digest &&
         source_identity_digest_ == control->manifest.source_identity_digest &&
         horizontal_incidence_authority_digest_ ==
             control->manifest.horizontal_incidence_authority_digest &&
         stable_facet_token_count_ ==
             control->manifest.stable_facet_token_count &&
         batch_count_ == control->manifest.batch_count &&
         initial_chain_digest_ ==
             control->manifest.initial_batch_chain_digest &&
         final_chain_digest_ == control->manifest.final_batch_chain_digest &&
         !vertical_maps_complete_ && !durable_restart_supported_ &&
         !public_status_claimed_;
}

std::uint32_t ExactDirectNormalizedH0ScientificSourceStamp::schema_version()
    const noexcept {
  return schema_version_;
}

std::uint64_t
ExactDirectNormalizedH0ScientificSourceStamp::scientific_authority_id()
    const noexcept {
  return scientific_authority_id_;
}

const contract::CanonicalId&
ExactDirectNormalizedH0ScientificSourceStamp::manifest_digest()
    const noexcept {
  return manifest_digest_;
}

const contract::CanonicalId&
ExactDirectNormalizedH0ScientificSourceStamp::source_identity_digest()
    const noexcept {
  return source_identity_digest_;
}

const contract::CanonicalId& ExactDirectNormalizedH0ScientificSourceStamp::
    horizontal_incidence_authority_digest() const noexcept {
  return horizontal_incidence_authority_digest_;
}

std::size_t
ExactDirectNormalizedH0ScientificSourceStamp::stable_facet_token_count()
    const noexcept {
  return stable_facet_token_count_;
}

std::size_t ExactDirectNormalizedH0ScientificSourceStamp::batch_count()
    const noexcept {
  return batch_count_;
}

const contract::CanonicalId&
ExactDirectNormalizedH0ScientificSourceStamp::initial_chain_digest()
    const noexcept {
  return initial_chain_digest_;
}

const contract::CanonicalId&
ExactDirectNormalizedH0ScientificSourceStamp::final_chain_digest()
    const noexcept {
  return final_chain_digest_;
}

bool ExactDirectNormalizedH0ScientificSourceStamp::vertical_maps_complete()
    const noexcept {
  return vertical_maps_complete_;
}

bool ExactDirectNormalizedH0ScientificSourceStamp::durable_restart_supported()
    const noexcept {
  return durable_restart_supported_;
}

bool ExactDirectNormalizedH0ScientificSourceStamp::public_status_claimed()
    const noexcept {
  return public_status_claimed_;
}

struct ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow::Impl {
  ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow
      structural_ticket{};
  std::shared_ptr<const ScientificWindowControl> control;
  std::shared_ptr<const void> projection_capability_identity;
  const ExactDirectNormalizedH0ResidentOwnedBatchWindow*
      projection_owned_window_address{};
  const void* projection_local_to_stable_data_address{};
  const void* projection_facet_token_data_address{};
  const void* projection_batch_data_address{};
  const void* projection_direct_reference_data_address{};
  contract::CanonicalId source_chain_digest{};
  contract::CanonicalId batch_identity_digest{};
  contract::CanonicalId successor_chain_digest{};
  std::size_t projection_local_to_stable_size{};
  std::size_t projection_facet_token_size{};
  std::size_t projection_batch_size{};
  std::size_t projection_direct_reference_size{};
  exact::ExactLevel squared_level{};
  std::size_t source_batch_index{};
  std::size_t source_epoch{};
  std::size_t order{};
  bool scientific_binding_issued{false};
  bool projection_payload_privately_attested{false};
};

struct ExactDirectNormalizedH0ScientificWindowCapabilitySession::Impl {
  ExactDirectNormalizedH0AuthenticatedWindowStreamSession structural_stream{};
  std::shared_ptr<const ScientificWindowControl> control;
  ExactDirectNormalizedH0ScientificSourceStamp scientific_source_stamp{};
  std::optional<ExactDirectNormalizedH0ScientificWindowCapabilityFinalSeal>
      final_seal;
  std::size_t scientifically_committed_batch_count{};
  bool initialized{false};
  bool sealed{false};
};

ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow::
    ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow()
    noexcept = default;
ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow::
    ~ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow() =
        default;
ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow::
    ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow(
        ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow&&)
    noexcept = default;
ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow&
ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow::operator=(
    ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow&&)
    noexcept = default;
ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow::
    ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow(
        std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow::valid()
    const noexcept {
  if (impl_ == nullptr || !impl_->scientific_binding_issued ||
      !control_is_scientific(impl_->control) ||
      !impl_->structural_ticket.valid()) {
    return false;
  }
  const auto& owned = impl_->structural_ticket.owned_window();
  if (!owned.certified_relative_to(impl_->control->manifest) ||
      owned.manifest_digest != impl_->control->manifest.manifest_digest ||
      owned.source_batch_index != impl_->source_batch_index ||
      owned.source_chain_digest != impl_->source_chain_digest ||
      owned.batch_identity_digest != impl_->batch_identity_digest ||
      owned.successor_chain_digest != impl_->successor_chain_digest ||
      owned.local_plan.batches.size() != 1U) {
    return false;
  }
  const auto& batch = owned.local_plan.batches.front();
  return batch.batch_index == impl_->source_batch_index &&
         batch.squared_level == impl_->squared_level &&
         batch.order == impl_->order;
}

bool ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow::
    scientifically_attests_owned_window() const noexcept {
  return valid();
}

std::size_t ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow::
    stable_facet_token_count() const noexcept {
  if (impl_ == nullptr || !control_is_scientific(impl_->control)) {
    return 0U;
  }
  return impl_->control->manifest.stable_facet_token_count;
}

bool ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow::
    privately_attests_projection_payload() const noexcept {
  if (impl_ == nullptr || !impl_->scientific_binding_issued ||
      !impl_->projection_payload_privately_attested ||
      impl_->projection_capability_identity == nullptr ||
      !control_is_scientific(impl_->control) ||
      !impl_->structural_ticket.valid()) {
    return false;
  }
  const auto& owned = impl_->structural_ticket.owned_window();
  if (impl_->projection_owned_window_address != &owned ||
      impl_->projection_local_to_stable_data_address !=
          owned.local_to_stable_facet_token_indices.data() ||
      impl_->projection_local_to_stable_size !=
          owned.local_to_stable_facet_token_indices.size() ||
      impl_->projection_facet_token_data_address !=
          owned.local_plan.facet_tokens.data() ||
      impl_->projection_facet_token_size !=
          owned.local_plan.facet_tokens.size() ||
      impl_->projection_batch_data_address != owned.local_plan.batches.data() ||
      impl_->projection_batch_size != owned.local_plan.batches.size() ||
      impl_->projection_direct_reference_data_address !=
          owned.local_plan.direct_references.data() ||
      impl_->projection_direct_reference_size !=
          owned.local_plan.direct_references.size() ||
      owned.manifest_digest != impl_->control->manifest.manifest_digest ||
      owned.source_batch_index != impl_->source_batch_index ||
      owned.source_chain_digest != impl_->source_chain_digest ||
      owned.batch_identity_digest != impl_->batch_identity_digest ||
      owned.successor_chain_digest != impl_->successor_chain_digest ||
      owned.local_plan.batches.size() != 1U) {
    return false;
  }
  const auto& batch = owned.local_plan.batches.front();
  return batch.batch_index == impl_->source_batch_index &&
         batch.squared_level == impl_->squared_level &&
         batch.order == impl_->order;
}

const void* ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow::
    projection_capability_identity() const noexcept {
  return impl_ == nullptr ? nullptr
                          : impl_->projection_capability_identity.get();
}

std::shared_ptr<const void>
ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow::
    share_projection_capability_identity() const noexcept {
  return impl_ == nullptr ? std::shared_ptr<const void>{}
                          : impl_->projection_capability_identity;
}

const void*
ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow::
    scientific_control_identity() const noexcept {
  return impl_ == nullptr ? nullptr : impl_->control.get();
}

const ExactDirectNormalizedH0ResidentOwnedBatchWindow&
ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow::
    owned_window() const noexcept {
  static const ExactDirectNormalizedH0ResidentOwnedBatchWindow empty{};
  return impl_ == nullptr ? empty : impl_->structural_ticket.owned_window();
}

bool ExactDirectNormalizedH0ScientificWindowCapabilityPreparationResult::
    certified_scientific_preparation() const noexcept {
  return ticket.has_value() && ticket->valid() &&
         provider_callback_count == 1U &&
         horizontal_incidence_source_bound && exact_manifest_and_chain_bound &&
         !global_compatibility_plan_retained &&
         !resident_scientific_state_mutated && !vertical_maps_complete &&
         !public_status_claimed &&
         decision ==
             ExactDirectNormalizedH0ScientificWindowCapabilityPreparationDecision::
                 complete_scientifically_bound_window;
}

bool ExactDirectNormalizedH0ScientificWindowCapabilityCommitResult::
    certified_scientific_commit() const noexcept {
  return structural_commit.certified_provisional_commit() && ticket_consumed &&
         scientific_prefix_advanced_once &&
         horizontal_incidence_source_bound &&
         !resident_scientific_state_mutated && !vertical_maps_complete &&
         !public_status_claimed &&
         decision ==
             ExactDirectNormalizedH0ScientificWindowCapabilityCommitDecision::
                 complete_scientific_window_commit;
}

bool ExactDirectNormalizedH0ScientificWindowCapabilityFinalSeal::
    certified_scientific_terminal_seal() const noexcept {
  return schema_version ==
             direct_normalized_h0_scientific_window_capability_schema_version &&
         scientific_authority_id != 0U &&
         horizontal_incidence_authority_freshly_verified &&
         expected_manifest_freshly_reconstructed &&
         every_committed_window_scientifically_bound &&
         !global_compatibility_plan_retained && !resident_fold_executed &&
         !vertical_maps_complete && !durable_restart_supported &&
         !public_status_claimed && sealed;
}

bool ExactDirectNormalizedH0ScientificWindowCapabilitySealResult::
    certified_scientific_seal() const noexcept {
  return seal.has_value() && seal->certified_scientific_terminal_seal() &&
         decision ==
             ExactDirectNormalizedH0ScientificWindowCapabilitySealDecision::
                 complete_scientific_terminal_seal;
}

ExactDirectNormalizedH0ScientificWindowCapabilitySession::
    ExactDirectNormalizedH0ScientificWindowCapabilitySession() noexcept =
        default;
ExactDirectNormalizedH0ScientificWindowCapabilitySession::
    ~ExactDirectNormalizedH0ScientificWindowCapabilitySession() = default;
ExactDirectNormalizedH0ScientificWindowCapabilitySession::
    ExactDirectNormalizedH0ScientificWindowCapabilitySession(
        ExactDirectNormalizedH0ScientificWindowCapabilitySession&&) noexcept =
        default;
ExactDirectNormalizedH0ScientificWindowCapabilitySession&
ExactDirectNormalizedH0ScientificWindowCapabilitySession::operator=(
    ExactDirectNormalizedH0ScientificWindowCapabilitySession&&) noexcept =
        default;
ExactDirectNormalizedH0ScientificWindowCapabilitySession::
    ExactDirectNormalizedH0ScientificWindowCapabilitySession(
        std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {
  if (impl_ != nullptr) {
    impl_->scientific_source_stamp =
        ExactDirectNormalizedH0ScientificSourceStamp{impl_->control};
  }
}

bool ExactDirectNormalizedH0ScientificWindowCapabilitySession::
    certified_scientific_window_stream() const noexcept {
  if (impl_ == nullptr || !impl_->initialized ||
      !control_is_scientific(impl_->control) ||
      !impl_->scientific_source_stamp.certified_scientific_source_stamp() ||
      !impl_->structural_stream.provisional_bound_stream() ||
      impl_->structural_stream.provider_identity() !=
          impl_->control->provider_identity ||
      impl_->scientifically_committed_batch_count !=
          impl_->structural_stream.batch_cursor()) {
    return false;
  }
  if (!impl_->sealed) {
    return !impl_->final_seal.has_value() &&
           !impl_->structural_stream.sealed();
  }
  return impl_->structural_stream.sealed() &&
         impl_->final_seal.has_value() &&
         impl_->final_seal->certified_scientific_terminal_seal() &&
         impl_->final_seal->committed_batch_count ==
             impl_->scientifically_committed_batch_count &&
         impl_->final_seal->final_chain_digest ==
             impl_->structural_stream.current_chain_digest();
}

bool ExactDirectNormalizedH0ScientificWindowCapabilitySession::complete()
    const noexcept {
  return certified_scientific_window_stream() &&
         impl_->structural_stream.complete();
}

bool ExactDirectNormalizedH0ScientificWindowCapabilitySession::sealed()
    const noexcept {
  return certified_scientific_window_stream() && impl_->sealed;
}

std::size_t
ExactDirectNormalizedH0ScientificWindowCapabilitySession::batch_cursor()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->structural_stream.batch_cursor();
}

std::size_t ExactDirectNormalizedH0ScientificWindowCapabilitySession::epoch()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->structural_stream.epoch();
}

const contract::CanonicalId&
ExactDirectNormalizedH0ScientificWindowCapabilitySession::
    current_chain_digest() const noexcept {
  static const contract::CanonicalId empty{};
  return impl_ == nullptr ? empty
                          : impl_->structural_stream.current_chain_digest();
}

const void*
ExactDirectNormalizedH0ScientificWindowCapabilitySession::provider_identity()
    const noexcept {
  return impl_ == nullptr ? nullptr
                          : impl_->structural_stream.provider_identity();
}

bool ExactDirectNormalizedH0ScientificWindowCapabilitySession::
    horizontal_incidence_source_bound() const noexcept {
  return certified_scientific_window_stream();
}

const ExactDirectNormalizedH0ScientificSourceStamp&
ExactDirectNormalizedH0ScientificWindowCapabilitySession::
    scientific_source_stamp() const noexcept {
  static const ExactDirectNormalizedH0ScientificSourceStamp empty{};
  return impl_ == nullptr ? empty : impl_->scientific_source_stamp;
}

bool ExactDirectNormalizedH0ScientificWindowCapabilitySession::
    global_compatibility_plan_retained() const noexcept {
  return false;
}

bool ExactDirectNormalizedH0ScientificWindowCapabilitySession::
    public_status_claimed() const noexcept {
  return false;
}

ExactDirectNormalizedH0ScientificWindowCapabilityPreparationResult
ExactDirectNormalizedH0ScientificWindowCapabilitySession::prepare_next() {
  ExactDirectNormalizedH0ScientificWindowCapabilityPreparationResult output;
  if (!certified_scientific_window_stream() || impl_->sealed) {
    output.decision =
        ExactDirectNormalizedH0ScientificWindowCapabilityPreparationDecision::
            no_session_rejected;
    return output;
  }
  auto structural = impl_->structural_stream.prepare_next();
  output.structural_decision = structural.decision;
  output.provider_callback_count = structural.provider_callback_count;
  if (!structural.certified_provisional_preparation() ||
      !structural.ticket.has_value()) {
    output.decision =
        ExactDirectNormalizedH0ScientificWindowCapabilityPreparationDecision::
            no_underlying_preparation_rejected;
    return output;
  }
  try {
    const auto& owned = structural.ticket->owned_window();
    if (!owned.certified_relative_to(impl_->control->manifest) ||
        owned.source_batch_index != impl_->structural_stream.batch_cursor() ||
        owned.source_chain_digest !=
            impl_->structural_stream.current_chain_digest() ||
        owned.local_plan.batches.size() != 1U) {
      output.decision =
          ExactDirectNormalizedH0ScientificWindowCapabilityPreparationDecision::
              no_window_scientific_binding_rejected;
      return output;
    }
    const auto& batch = owned.local_plan.batches.front();
    auto impl = std::make_unique<
        ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow::Impl>();
    impl->control = impl_->control;
    impl->source_chain_digest = owned.source_chain_digest;
    impl->batch_identity_digest = owned.batch_identity_digest;
    impl->successor_chain_digest = owned.successor_chain_digest;
    impl->squared_level = batch.squared_level;
    impl->source_batch_index = owned.source_batch_index;
    impl->source_epoch = impl_->structural_stream.epoch();
    impl->order = batch.order;
    impl->scientific_binding_issued = true;
    impl->structural_ticket = std::move(*structural.ticket);
    impl->projection_capability_identity =
        std::make_shared<const std::uint8_t>(std::uint8_t{0U});
    const auto& projection_owned = impl->structural_ticket.owned_window();
    impl->projection_owned_window_address = &projection_owned;
    impl->projection_local_to_stable_data_address =
        projection_owned.local_to_stable_facet_token_indices.data();
    impl->projection_local_to_stable_size =
        projection_owned.local_to_stable_facet_token_indices.size();
    impl->projection_facet_token_data_address =
        projection_owned.local_plan.facet_tokens.data();
    impl->projection_facet_token_size =
        projection_owned.local_plan.facet_tokens.size();
    impl->projection_batch_data_address =
        projection_owned.local_plan.batches.data();
    impl->projection_batch_size = projection_owned.local_plan.batches.size();
    impl->projection_direct_reference_data_address =
        projection_owned.local_plan.direct_references.data();
    impl->projection_direct_reference_size =
        projection_owned.local_plan.direct_references.size();
    impl->projection_payload_privately_attested = true;
    ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow ticket{
        std::move(impl)};
    if (!ticket.valid()) {
      output.decision =
          ExactDirectNormalizedH0ScientificWindowCapabilityPreparationDecision::
              no_window_scientific_binding_rejected;
      return output;
    }
    output.ticket.emplace(std::move(ticket));
    output.horizontal_incidence_source_bound = true;
    output.exact_manifest_and_chain_bound = true;
    output.global_compatibility_plan_retained = false;
    output.resident_scientific_state_mutated = false;
    output.vertical_maps_complete = false;
    output.public_status_claimed = false;
    output.decision =
        ExactDirectNormalizedH0ScientificWindowCapabilityPreparationDecision::
            complete_scientifically_bound_window;
    return output;
  } catch (const std::bad_alloc&) {
    output.decision =
        ExactDirectNormalizedH0ScientificWindowCapabilityPreparationDecision::
            no_allocation_failed;
    return output;
  } catch (...) {
    output.decision =
        ExactDirectNormalizedH0ScientificWindowCapabilityPreparationDecision::
            no_window_scientific_binding_rejected;
    return output;
  }
}

ExactDirectNormalizedH0ScientificWindowCapabilityCommitResult
ExactDirectNormalizedH0ScientificWindowCapabilitySession::commit(
    ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow&& ticket)
    noexcept {
  ExactDirectNormalizedH0ScientificWindowCapabilityCommitResult output;
  auto consumed = std::move(ticket.impl_);
  output.ticket_consumed = consumed != nullptr;
  if (consumed == nullptr || !consumed->scientific_binding_issued ||
      !consumed->structural_ticket.valid()) {
    output.decision =
        ExactDirectNormalizedH0ScientificWindowCapabilityCommitDecision::
            no_ticket_invalid_or_consumed;
    return output;
  }
  const auto& owned = consumed->structural_ticket.owned_window();
  const bool owned_window_still_scientifically_bound =
      control_is_scientific(consumed->control) &&
      owned.certified_relative_to(consumed->control->manifest) &&
      owned.manifest_digest == consumed->control->manifest.manifest_digest &&
      owned.source_batch_index == consumed->source_batch_index &&
      owned.source_chain_digest == consumed->source_chain_digest &&
      owned.batch_identity_digest == consumed->batch_identity_digest &&
      owned.successor_chain_digest == consumed->successor_chain_digest &&
      owned.local_plan.batches.size() == 1U &&
      owned.local_plan.batches.front().batch_index ==
          consumed->source_batch_index &&
      owned.local_plan.batches.front().squared_level ==
          consumed->squared_level &&
      owned.local_plan.batches.front().order == consumed->order;
  if (!certified_scientific_window_stream() || impl_->sealed ||
      !owned_window_still_scientifically_bound ||
      consumed->control != impl_->control ||
      consumed->source_batch_index != impl_->structural_stream.batch_cursor() ||
      consumed->source_epoch != impl_->structural_stream.epoch() ||
      consumed->source_chain_digest !=
          impl_->structural_stream.current_chain_digest()) {
    output.decision =
        ExactDirectNormalizedH0ScientificWindowCapabilityCommitDecision::
            no_foreign_or_stale_ticket_rejected;
    return output;
  }
  const std::size_t cursor_before = impl_->structural_stream.batch_cursor();
  output.structural_commit = impl_->structural_stream.commit(
      std::move(consumed->structural_ticket));
  if (!output.structural_commit.certified_provisional_commit()) {
    output.decision =
        ExactDirectNormalizedH0ScientificWindowCapabilityCommitDecision::
            no_underlying_commit_rejected;
    return output;
  }
  ++impl_->scientifically_committed_batch_count;
  output.source_batch_index = output.structural_commit.source_batch_index;
  output.committed_cursor = output.structural_commit.committed_cursor;
  output.committed_epoch = output.structural_commit.committed_epoch;
  output.committed_chain_digest =
      output.structural_commit.committed_chain_digest;
  output.scientific_prefix_advanced_once =
      impl_->scientifically_committed_batch_count == cursor_before + 1U &&
      output.committed_cursor == impl_->scientifically_committed_batch_count;
  output.horizontal_incidence_source_bound = true;
  output.resident_scientific_state_mutated = false;
  output.vertical_maps_complete = false;
  output.public_status_claimed = false;
  output.decision =
      ExactDirectNormalizedH0ScientificWindowCapabilityCommitDecision::
          complete_scientific_window_commit;
  return output;
}

ExactDirectNormalizedH0ScientificWindowCapabilitySealResult
ExactDirectNormalizedH0ScientificWindowCapabilitySession::seal() noexcept {
  ExactDirectNormalizedH0ScientificWindowCapabilitySealResult output;
  if (!certified_scientific_window_stream()) {
    output.decision =
        ExactDirectNormalizedH0ScientificWindowCapabilitySealDecision::
            no_session_rejected;
    return output;
  }
  auto structural = impl_->structural_stream.seal();
  output.structural_decision = structural.decision;
  if (!structural.certified_provisional_seal() ||
      !structural.seal.has_value()) {
    output.decision =
        ExactDirectNormalizedH0ScientificWindowCapabilitySealDecision::
            no_underlying_seal_rejected;
    return output;
  }
  if (impl_->scientifically_committed_batch_count !=
          impl_->control->manifest.batch_count ||
      impl_->structural_stream.current_chain_digest() !=
          impl_->control->manifest.final_batch_chain_digest) {
    output.decision =
        ExactDirectNormalizedH0ScientificWindowCapabilitySealDecision::
            no_scientific_prefix_rejected;
    return output;
  }
  if (!impl_->final_seal.has_value()) {
    ExactDirectNormalizedH0ScientificWindowCapabilityFinalSeal final;
    final.scientific_authority_id =
        impl_->control->scientific_authority_id;
    final.manifest_digest = impl_->control->manifest.manifest_digest;
    final.source_identity_digest =
        impl_->control->manifest.source_identity_digest;
    final.horizontal_incidence_authority_digest =
        impl_->control->manifest.horizontal_incidence_authority_digest;
    final.final_chain_digest =
        impl_->control->manifest.final_batch_chain_digest;
    final.committed_batch_count =
        impl_->scientifically_committed_batch_count;
    final.horizontal_incidence_authority_freshly_verified = true;
    final.expected_manifest_freshly_reconstructed = true;
    final.every_committed_window_scientifically_bound = true;
    final.global_compatibility_plan_retained = false;
    final.resident_fold_executed = false;
    final.vertical_maps_complete = false;
    final.durable_restart_supported = false;
    final.public_status_claimed = false;
    final.sealed = true;
    if (!final.certified_scientific_terminal_seal()) {
      output.decision =
          ExactDirectNormalizedH0ScientificWindowCapabilitySealDecision::
              no_scientific_prefix_rejected;
      return output;
    }
    impl_->final_seal = final;
    impl_->sealed = true;
    output.newly_sealed = true;
  }
  output.seal = impl_->final_seal;
  output.decision =
      ExactDirectNormalizedH0ScientificWindowCapabilitySealDecision::
          complete_scientific_terminal_seal;
  return output;
}

bool ExactDirectNormalizedH0ScientificWindowCapabilityInitializationResult::
    certified_scientific_initialization() const noexcept {
  return session.has_value() &&
         session->certified_scientific_window_stream() &&
         horizontal_incidence_authority_verification_count == 1U &&
         normalized_source_plan_verification_count == 2U &&
         provider_full_replay_count == 1U &&
         horizontal_incidence_authority_freshly_verified &&
         compatibility_plan_transiently_reconstructed &&
         expected_manifest_freshly_reconstructed &&
         observed_manifest_recursively_equal && provider_identity_bound &&
         every_provider_window_authenticated &&
         !global_compatibility_plan_retained &&
         !resident_scientific_state_initialized && !vertical_maps_complete &&
         !public_status_claimed &&
         decision ==
             ExactDirectNormalizedH0ScientificWindowCapabilityInitializationDecision::
                 complete_scientifically_bound_window_stream;
}

ExactDirectNormalizedH0ScientificWindowCapabilityInitializationResult
initialize_exact_direct_normalized_h0_scientific_window_capability(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseGatewayCandidateBudget& source_gateway_budget,
    spatial::LbvhTraversalOrder source_gateway_traversal_order,
    const ExactDirectSparseGatewayCandidateJournalResult& source_gateway,
    const ExactDirectNormalizedH0SourcePlanBudget& source_plan_budget,
    const ExactDirectNormalizedH0SourcePlanResult& source_plan,
    const ExactDirectRankWindowSaturatedH0Authority& rank_window_authority,
    const ExactDirectNormalizedH0IncidenceReductionAuthority&
        incidence_authority,
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest,
    ExactDirectNormalizedH0ResidentBatchProviderView provider,
    std::uint64_t scientific_authority_id,
    const ExactDirectNormalizedH0ScientificWindowCapabilityBudget& budget) {
  ExactDirectNormalizedH0ScientificWindowCapabilityInitializationResult output;
  if (scientific_authority_id == 0U ||
      !budget_has_bounded_rebuild_and_one_window(budget)) {
    output.decision =
        ExactDirectNormalizedH0ScientificWindowCapabilityInitializationDecision::
            no_budget_rejected;
    return output;
  }

  output.horizontal_incidence_authority_verification_count = 1U;
  const auto incidence_verification =
      verify_exact_direct_normalized_h0_incidence_reduction_authority(
          index,
          cloud,
          source_facade,
          source_journal,
          source_arm_budget,
          source_arm_journal,
          source_incidence_budget,
          source_incidence_journal,
          source_gateway_budget,
          source_gateway_traversal_order,
          source_gateway,
          source_plan_budget,
          source_plan,
          rank_window_authority,
          incidence_authority);
  output.horizontal_incidence_authority_freshly_verified =
      incidence_verification.result_certified;
  output.normalized_source_plan_verification_count =
      incidence_verification.source_plan_freshly_replayed ? 1U : 0U;
  if (!output.horizontal_incidence_authority_freshly_verified) {
    output.decision =
        ExactDirectNormalizedH0ScientificWindowCapabilityInitializationDecision::
            no_horizontal_incidence_authority_rejected;
    return output;
  }

  try {
    ExactDirectMorseUnifiedResidentSessionBudget compatibility_shape_budget;
    compatibility_shape_budget.locator.maximum_component_handle_count =
        budget.maximum_compatibility_facet_count;
    compatibility_shape_budget.locator.maximum_committed_batch_count =
        budget.maximum_compatibility_batch_count;
    auto rebuilt =
        internal::ExactDirectFrozenUnifiedImmutablePlanAuthorityFactory::
            create_from_normalized_direct_source(
                index,
                cloud,
                source_facade,
                source_journal,
                source_arm_budget,
                source_arm_journal,
                source_incidence_budget,
                source_incidence_journal,
                source_gateway_budget,
                source_gateway_traversal_order,
                source_gateway,
                source_plan_budget,
                source_plan,
                budget.compatibility_adapter,
                compatibility_shape_budget);
    output.normalized_source_plan_verification_count +=
        rebuilt.source_plan_verification_count;
    if (!rebuilt.source_plan_freshly_verified_once ||
        !rebuilt.normalized_adapter_preflight_certified ||
        !rebuilt.normalized_adapter_construction_certified ||
        !rebuilt.every_normalized_coface_reconstructed_transiently ||
        rebuilt.normalized_immutable_plan == nullptr ||
        !rebuilt.authority.has_value() ||
        rebuilt.normalized_reconstructed_coface_count !=
            source_plan.cofaces.size()) {
      output.decision =
          ExactDirectNormalizedH0ScientificWindowCapabilityInitializationDecision::
              no_compatibility_rebuild_rejected;
      return output;
    }
    output.compatibility_plan_transiently_reconstructed = true;
    const auto expected_manifest =
        build_exact_direct_normalized_h0_resident_source_manifest_from_compatibility_plan(
            *rebuilt.normalized_immutable_plan, incidence_authority);
    output.expected_manifest_freshly_reconstructed = true;
    output.observed_manifest_recursively_equal = expected_manifest == manifest;
    if (!output.observed_manifest_recursively_equal) {
      output.decision =
          ExactDirectNormalizedH0ScientificWindowCapabilityInitializationDecision::
              no_scientific_manifest_mismatch;
      return output;
    }

    // The complete compatibility arena and its address-bound internal
    // authority die with `rebuilt` before this function publishes a session.
    // From this point onward only the scalar manifest and one provider window
    // are retained by the new capability.
    rebuilt.authority.reset();
    rebuilt.normalized_immutable_plan.reset();

    output.provider_full_replay_count = 1U;
    const auto provider_verification =
        verify_exact_direct_normalized_h0_resident_batch_provider(
            expected_manifest, provider, budget.stream.provider_window);
    output.provider_identity_bound =
        provider_verification.provider_identity_bound &&
        provider_verification.provider_identity == provider.identity();
    output.every_provider_window_authenticated =
        provider_verification.result_certified &&
        provider_verification.every_window_freshly_recertified &&
        provider_verification.final_chain_matches_manifest;
    if (!output.provider_identity_bound ||
        !output.every_provider_window_authenticated) {
      output.decision =
          ExactDirectNormalizedH0ScientificWindowCapabilityInitializationDecision::
              no_provider_verification_rejected;
      return output;
    }
    auto structural =
        initialize_exact_direct_normalized_h0_authenticated_window_stream(
            expected_manifest,
            provider_verification,
            provider,
            scientific_authority_id,
            budget.stream);
    if (!structural.certified_provisional_initialization() ||
        !structural.session.has_value()) {
      output.decision =
          ExactDirectNormalizedH0ScientificWindowCapabilityInitializationDecision::
              no_structural_stream_rejected;
      return output;
    }

    auto control = std::make_shared<const ScientificWindowControl>(
        ScientificWindowControl{
            expected_manifest,
            provider.identity(),
            scientific_authority_id,
            true,
            true});
    auto impl = std::make_unique<
        ExactDirectNormalizedH0ScientificWindowCapabilitySession::Impl>();
    impl->structural_stream = std::move(*structural.session);
    impl->control = std::move(control);
    impl->initialized = true;
    ExactDirectNormalizedH0ScientificWindowCapabilitySession session{
        std::move(impl)};
    if (!session.certified_scientific_window_stream()) {
      output.decision =
          ExactDirectNormalizedH0ScientificWindowCapabilityInitializationDecision::
              no_structural_stream_rejected;
      return output;
    }
    output.session.emplace(std::move(session));
    output.global_compatibility_plan_retained = false;
    output.resident_scientific_state_initialized = false;
    output.vertical_maps_complete = false;
    output.public_status_claimed = false;
    output.decision =
        ExactDirectNormalizedH0ScientificWindowCapabilityInitializationDecision::
            complete_scientifically_bound_window_stream;
    return output;
  } catch (const std::bad_alloc&) {
    output.decision =
        ExactDirectNormalizedH0ScientificWindowCapabilityInitializationDecision::
            no_allocation_failed;
    return output;
  } catch (const std::length_error&) {
    output.decision =
        ExactDirectNormalizedH0ScientificWindowCapabilityInitializationDecision::
            no_compatibility_rebuild_rejected;
    return output;
  } catch (const std::invalid_argument&) {
    output.decision =
        ExactDirectNormalizedH0ScientificWindowCapabilityInitializationDecision::
            no_compatibility_rebuild_rejected;
    return output;
  } catch (...) {
    output.decision =
        ExactDirectNormalizedH0ScientificWindowCapabilityInitializationDecision::
            no_compatibility_rebuild_rejected;
    return output;
  }
}

}  // namespace morsehgp3d::hierarchy
