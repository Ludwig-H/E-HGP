#include "morsehgp3d/hierarchy/direct_normalized_h0_relative_frozen_incidence_batch.hpp"

#include "direct_frozen_unified_incidence_batch_internal.hpp"

#include <algorithm>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {

struct ExactDirectNormalizedH0RelativeFrozenIncidenceBatch::Impl {
  ExactDirectFrozenUnifiedIncidenceBatchResult frozen{};
  std::optional<internal::ExactDirectFrozenUnifiedResidentBatchAttestation>
      attestation;
  std::shared_ptr<const void> scientific_window_projection_identity;
  const ExactDirectFrozenUnifiedIncidenceBatchResult*
      projection_frozen_payload_address{};
  const void* projection_frozen_attestation_address{};
  const void* projection_local_to_stable_data_address{};
  std::vector<std::size_t> local_to_stable_facet_token_indices;
  contract::CanonicalId manifest_digest{};
  contract::CanonicalId source_chain_digest{};
  contract::CanonicalId batch_identity_digest{};
  contract::CanonicalId successor_chain_digest{};
  std::size_t source_batch_index{};
  std::size_t stable_facet_token_count{};
  std::size_t stable_resolution_count{};
  std::size_t projection_local_to_stable_size{};
  bool scientific_window_privately_attested{false};
  bool required_stable_resolutions_translated_injectively{false};
  bool projection_payload_privately_attested{false};
};

ExactDirectNormalizedH0RelativeFrozenIncidenceBatch::
    ExactDirectNormalizedH0RelativeFrozenIncidenceBatch() noexcept = default;
ExactDirectNormalizedH0RelativeFrozenIncidenceBatch::
    ~ExactDirectNormalizedH0RelativeFrozenIncidenceBatch() = default;
ExactDirectNormalizedH0RelativeFrozenIncidenceBatch::
    ExactDirectNormalizedH0RelativeFrozenIncidenceBatch(
        ExactDirectNormalizedH0RelativeFrozenIncidenceBatch&&) noexcept =
        default;
ExactDirectNormalizedH0RelativeFrozenIncidenceBatch&
ExactDirectNormalizedH0RelativeFrozenIncidenceBatch::operator=(
    ExactDirectNormalizedH0RelativeFrozenIncidenceBatch&&) noexcept = default;
ExactDirectNormalizedH0RelativeFrozenIncidenceBatch::
    ExactDirectNormalizedH0RelativeFrozenIncidenceBatch(
        std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectNormalizedH0RelativeFrozenIncidenceBatch::valid()
    const noexcept {
  const auto local_index_is_retained = [&](std::size_t local_index) {
    return local_index < impl_->local_to_stable_facet_token_indices.size();
  };
  return impl_ != nullptr && impl_->scientific_window_privately_attested &&
         impl_->required_stable_resolutions_translated_injectively &&
         impl_->attestation.has_value() &&
         impl_->attestation->attests(impl_->frozen) &&
         impl_->stable_resolution_count <=
             impl_->local_to_stable_facet_token_indices.size() &&
         std::is_sorted(
             impl_->local_to_stable_facet_token_indices.begin(),
             impl_->local_to_stable_facet_token_indices.end()) &&
         std::adjacent_find(
             impl_->local_to_stable_facet_token_indices.begin(),
             impl_->local_to_stable_facet_token_indices.end()) ==
             impl_->local_to_stable_facet_token_indices.end() &&
         std::all_of(
             impl_->local_to_stable_facet_token_indices.begin(),
             impl_->local_to_stable_facet_token_indices.end(),
             [&](std::size_t stable_index) {
               return stable_index < impl_->stable_facet_token_count;
             }) &&
         std::all_of(
             impl_->frozen.incidence_facet_token_indices.begin(),
             impl_->frozen.incidence_facet_token_indices.end(),
             local_index_is_retained) &&
         std::all_of(
             impl_->frozen.equal_facet_binding_records.begin(),
             impl_->frozen.equal_facet_binding_records.end(),
             [&](const auto& record) {
               return local_index_is_retained(record.facet_token_index);
             }) &&
         std::all_of(
             impl_->frozen.coverage_delta_facets.begin(),
             impl_->frozen.coverage_delta_facets.end(),
             [&](const auto& reference) {
               return local_index_is_retained(reference.facet_token_index);
             }) &&
         impl_->frozen.source_batch_index == impl_->source_batch_index &&
         !impl_->frozen.source_plan_freshly_verified &&
         impl_->frozen
             .source_plan_verified_once_by_immutable_resident_authority &&
         impl_->frozen.normalized_direct_source_authority &&
         !impl_->frozen.successive_star_source_authority &&
         !impl_->frozen.global_facet_coface_or_gamma_catalog_materialized &&
         !impl_->frozen.public_status_claimed;
}

std::size_t
ExactDirectNormalizedH0RelativeFrozenIncidenceBatch::source_batch_index()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->source_batch_index;
}

const contract::CanonicalId&
ExactDirectNormalizedH0RelativeFrozenIncidenceBatch::manifest_digest()
    const noexcept {
  static const contract::CanonicalId empty{};
  return impl_ == nullptr ? empty : impl_->manifest_digest;
}

const contract::CanonicalId&
ExactDirectNormalizedH0RelativeFrozenIncidenceBatch::batch_identity_digest()
    const noexcept {
  static const contract::CanonicalId empty{};
  return impl_ == nullptr ? empty : impl_->batch_identity_digest;
}

std::span<const std::size_t>
ExactDirectNormalizedH0RelativeFrozenIncidenceBatch::
    local_to_stable_facet_token_indices() const noexcept {
  return impl_ == nullptr
      ? std::span<const std::size_t>{}
      : std::span<const std::size_t>{
            impl_->local_to_stable_facet_token_indices};
}

const ExactDirectFrozenUnifiedIncidenceBatchResult&
ExactDirectNormalizedH0RelativeFrozenIncidenceBatch::frozen_batch()
    const noexcept {
  static const ExactDirectFrozenUnifiedIncidenceBatchResult empty{};
  return impl_ == nullptr ? empty : impl_->frozen;
}

bool ExactDirectNormalizedH0RelativeFrozenIncidenceBatch::
    privately_attests_projection_payload() const noexcept {
  return impl_ != nullptr &&
         impl_->projection_payload_privately_attested &&
         impl_->scientific_window_privately_attested &&
         impl_->required_stable_resolutions_translated_injectively &&
         impl_->scientific_window_projection_identity != nullptr &&
         impl_->attestation.has_value() &&
         impl_->projection_frozen_attestation_address == &*impl_->attestation &&
         impl_->projection_frozen_payload_address == &impl_->frozen &&
         impl_->projection_local_to_stable_data_address ==
             impl_->local_to_stable_facet_token_indices.data() &&
         impl_->projection_local_to_stable_size ==
             impl_->local_to_stable_facet_token_indices.size() &&
         impl_->stable_resolution_count <=
             impl_->local_to_stable_facet_token_indices.size() &&
         impl_->frozen.source_batch_index == impl_->source_batch_index &&
         !impl_->frozen.source_plan_freshly_verified &&
         impl_->frozen
             .source_plan_verified_once_by_immutable_resident_authority &&
         impl_->frozen.normalized_direct_source_authority &&
         !impl_->frozen.successive_star_source_authority &&
         !impl_->frozen.global_facet_coface_or_gamma_catalog_materialized &&
         !impl_->frozen.public_status_claimed;
}

const void* ExactDirectNormalizedH0RelativeFrozenIncidenceBatch::
    scientific_window_projection_capability_identity() const noexcept {
  return impl_ == nullptr
             ? nullptr
             : impl_->scientific_window_projection_identity.get();
}

bool ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuildResult::
    certified_scientific_relative_build() const noexcept {
  return batch.has_value() && batch->valid() &&
         window_facet_translation_count ==
             batch->local_to_stable_facet_token_indices().size() &&
         stable_resolution_scan_count ==
             stable_to_local_translation_count &&
         !scientific_window_capability_consumed &&
         required_stable_resolutions_translated_injectively &&
         exactly_one_frozen_batch_construction &&
         quotient_freshly_streaming_verified &&
         action_plan_freshly_streaming_verified &&
         !complete_source_plan_materialized &&
         !global_facet_coface_incidence_or_gamma_catalog_materialized &&
         !resident_forest_or_caller_state_mutated && !vertical_maps_complete &&
         !public_status_claimed &&
         decision ==
             ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                 complete_scientific_window_relative_frozen_batch;
}

ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuildResult
ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuilder::build(
    const ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow&
        scientific_window,
    std::span<const ExactDirectNormalizedH0StableFacetResolution>
        stable_facet_resolutions,
    std::span<const ExactDirectFrozenUnifiedPriorRootCoverage>
        prior_root_coverages,
    std::span<const spatial::PointId> prior_root_coverage_point_references,
    std::span<const ExactDirectFrozenUnifiedLatentCarrierCoverage>
        latent_carrier_coverages,
    std::span<const spatial::PointId>
        latent_carrier_coverage_point_references,
    const ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBudget& budget) {
  ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuildResult output;
  if (!scientific_window.scientifically_attests_owned_window()) {
    output.decision =
        ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
            no_scientific_window_capability_rejected;
    return output;
  }
  if (stable_facet_resolutions.size() >
      budget.maximum_stable_resolution_translation_count) {
    output.decision =
        ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
            no_translation_budget_rejected;
    return output;
  }
  try {
    const auto& owned = scientific_window.owned_window();
    const auto& local_to_stable =
        owned.local_to_stable_facet_token_indices;
    output.window_facet_translation_count = local_to_stable.size();
    if (local_to_stable.size() >
        budget.maximum_window_facet_translation_count) {
      output.decision =
          ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
              no_translation_budget_rejected;
      return output;
    }
    if (local_to_stable.size() != owned.local_plan.facet_tokens.size() ||
        !std::is_sorted(local_to_stable.begin(), local_to_stable.end()) ||
        std::adjacent_find(local_to_stable.begin(), local_to_stable.end()) !=
            local_to_stable.end()) {
      output.decision =
          ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
              no_scientific_window_capability_rejected;
      return output;
    }

    std::vector<ExactDirectFrozenUnifiedFacetResolution> local_resolutions;
    local_resolutions.reserve(stable_facet_resolutions.size());
    std::optional<std::size_t> previous_stable;
    for (const auto& stable_resolution : stable_facet_resolutions) {
      ++output.stable_resolution_scan_count;
      if (previous_stable.has_value() &&
          stable_resolution.stable_source_facet_token_index <=
              *previous_stable) {
        output.decision =
            ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                no_stable_resolution_order_rejected;
        return output;
      }
      previous_stable =
          stable_resolution.stable_source_facet_token_index;
      const auto found = std::lower_bound(
          local_to_stable.begin(),
          local_to_stable.end(),
          stable_resolution.stable_source_facet_token_index);
      if (found == local_to_stable.end() ||
          *found != stable_resolution.stable_source_facet_token_index) {
        output.decision =
            ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                no_stable_resolution_outside_window_rejected;
        return output;
      }
      const auto local_index =
          static_cast<std::size_t>(found - local_to_stable.begin());
      if (owned.local_plan.facet_tokens[local_index].facet_token_index !=
          local_index) {
        output.decision =
            ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                no_scientific_window_capability_rejected;
        return output;
      }
      local_resolutions.push_back(
          {local_index,
           stable_resolution.token,
           stable_resolution.prior_root_id});
      ++output.stable_to_local_translation_count;
    }

    auto impl =
        std::make_unique<ExactDirectNormalizedH0RelativeFrozenIncidenceBatch::
                             Impl>();
    impl->local_to_stable_facet_token_indices = local_to_stable;
    impl->attestation =
        internal::
            ExactDirectFrozenUnifiedScientificWindowBatchAttestedBuilder::
                build_once(
                    owned.local_plan,
                    owned.source_batch_index,
                    local_resolutions,
                    prior_root_coverages,
                    prior_root_coverage_point_references,
                    latent_carrier_coverages,
                    latent_carrier_coverage_point_references,
                    budget.frozen_batch,
                    impl->frozen);
    if (!impl->attestation.has_value() ||
        !impl->attestation->attests(impl->frozen)) {
      output.decision =
          ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
              no_frozen_batch_rejected;
      return output;
    }
    impl->manifest_digest = owned.manifest_digest;
    impl->source_chain_digest = owned.source_chain_digest;
    impl->batch_identity_digest = owned.batch_identity_digest;
    impl->successor_chain_digest = owned.successor_chain_digest;
    impl->source_batch_index = owned.source_batch_index;
    impl->stable_facet_token_count =
        scientific_window.stable_facet_token_count();
    impl->stable_resolution_count = local_resolutions.size();
    impl->scientific_window_privately_attested = true;
    impl->required_stable_resolutions_translated_injectively =
        output.stable_resolution_scan_count ==
        output.stable_to_local_translation_count;
    impl->scientific_window_projection_identity =
        scientific_window.share_projection_capability_identity();
    if (impl->scientific_window_projection_identity == nullptr) {
      output.decision =
          ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
              no_scientific_window_capability_rejected;
      return output;
    }
    // From this point onward the batch owns an immutable payload and exposes
    // it only through const views.  Projection can therefore authenticate the
    // exact mint-time object and retained mapping in O(1), without replaying
    // the frozen quotient/action attestation.
    impl->projection_frozen_payload_address = &impl->frozen;
    impl->projection_frozen_attestation_address = &*impl->attestation;
    impl->projection_local_to_stable_data_address =
        impl->local_to_stable_facet_token_indices.data();
    impl->projection_local_to_stable_size =
        impl->local_to_stable_facet_token_indices.size();
    impl->projection_payload_privately_attested = true;
    ExactDirectNormalizedH0RelativeFrozenIncidenceBatch batch{
        std::move(impl)};
    if (!batch.valid()) {
      output.decision =
          ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
              no_frozen_batch_rejected;
      return output;
    }
    output.quotient_freshly_streaming_verified =
        batch.frozen_batch().frozen_quotient_freshly_streaming_verified;
    output.action_plan_freshly_streaming_verified =
        batch.frozen_batch().frozen_hgp_action_plan_freshly_streaming_verified;
    output.batch.emplace(std::move(batch));
    output.required_stable_resolutions_translated_injectively = true;
    output.exactly_one_frozen_batch_construction = true;
    output.complete_source_plan_materialized = false;
    output.global_facet_coface_incidence_or_gamma_catalog_materialized = false;
    output.resident_forest_or_caller_state_mutated = false;
    output.vertical_maps_complete = false;
    output.public_status_claimed = false;
    output.decision =
        ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
            complete_scientific_window_relative_frozen_batch;
    return output;
  } catch (const std::bad_alloc&) {
    output.decision =
        ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
            no_allocation_failed;
    return output;
  } catch (const std::length_error&) {
    output.decision =
        ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
            no_translation_budget_rejected;
    return output;
  } catch (...) {
    output.decision =
        ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
            no_frozen_batch_rejected;
    return output;
  }
}

}  // namespace morsehgp3d::hierarchy
