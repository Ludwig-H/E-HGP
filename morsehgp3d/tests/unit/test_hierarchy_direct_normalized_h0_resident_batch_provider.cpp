#include "morsehgp3d/hierarchy/direct_normalized_h0_resident_batch_provider.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::contract::CanonicalId;
using morsehgp3d::contract::CanonicalSha256Builder;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::PointId;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] ExactLevel level(std::int64_t numerator) {
  return ExactLevel{BigInt{numerator}, BigInt{1}};
}

[[nodiscard]] CanonicalId digest(std::string_view text) {
  CanonicalSha256Builder builder;
  builder.update(text);
  return builder.finalize();
}

template <std::size_t Size>
[[nodiscard]] ExactDirectSparseFacetKey facet(
    const std::array<PointId, Size>& point_ids) {
  ExactDirectSparseFacetKey key;
  key.point_count = Size;
  std::copy(point_ids.begin(), point_ids.end(), key.point_ids.begin());
  return key;
}

[[nodiscard]] ExactDirectNormalizedH0IncidenceReductionAuthority authority() {
  ExactDirectNormalizedH0IncidenceReductionAuthority result;
  result.point_count = 6U;
  result.requested_maximum_order = 3U;
  result.effective_maximum_order = 3U;
  result.maximum_relevant_closed_rank = 4U;
  result.source_direct_event_count = 4U;
  result.source_core_facet_count = 7U;
  result.source_gateway_candidate_count = 2U;
  result.normalized_coface_count = 2U;
  result.normalized_batch_count = 2U;
  result.source_pair_canonical_cloud_digest = digest("pair-cloud");
  result.source_higher_canonical_cloud_digest = digest("higher-cloud");
  result.source_pair_semantic_digest = digest("pair-science");
  result.source_higher_semantic_digest = digest("higher-science");
  result.source_terminal_digest = digest("terminal");
  result.source_gateway_scientific_identity_digest = digest("gateway");
  result.source_plan_decision =
      ExactDirectNormalizedH0SourcePlanDecision::
          complete_certified_direct_plus_first_incidence_source_plan;
  result.rank_window_decision =
      ExactDirectRankWindowSaturatedH0Decision::
          certified_rank_window_saturated_h0_quiescence;
  result.source_plan_freshly_replayed = true;
  result.rank_window_authority_freshly_replayed = true;
  result.source_authorities_bound_to_one_terminal_facade = true;
  result.every_higher_order_direct_birth_present = true;
  result.every_higher_order_direct_coface_present = true;
  result.every_direct_core_facet_present = true;
  result.every_first_incidence_family_complete = true;
  result.every_first_incidence_cominimizer_retained = true;
  result.normalized_batches_are_complete_atomic_quotient_inputs = true;
  result.factorized_cofaces_reconstructible_for_latent_carriers = true;
  result.every_rank_relevant_ball_has_support_only_shell = true;
  result.every_above_window_saturated_block_h0_quiescent = true;
  result.rank_relevant_corollary_4_1_certified = true;
  result.above_window_theorem_4_2_certified = true;
  result.normalized_horizontal_h0_equivalence_certified = true;
  result.incidence_complete_reduction_proved = true;
  result.decision =
      ExactDirectNormalizedH0IncidenceReductionDecision::
          complete_certified_horizontal_incidence_reduction;
  result.scope = ExactDirectNormalizedH0IncidenceReductionScope::
      normalized_horizontal_h0_orders_two_through_effective_maximum;
  return result;
}

[[nodiscard]] ExactDirectSparseUnifiedLevelPlanResult compatibility_plan(
    const ExactDirectNormalizedH0IncidenceReductionAuthority& source) {
  ExactDirectSparseUnifiedLevelPlanResult plan;
  plan.point_count = source.point_count;
  plan.source_event_projection_count = 16U;
  plan.source_role_record_count = 16U;
  plan.source_incidence_family_count = 1U;
  plan.required_source_role_scan_count = 3U;
  plan.required_higher_order_direct_role_count = 2U;
  plan.excluded_order_one_role_count = 1U;
  plan.required_direct_birth_reference_count = 1U;
  plan.required_direct_saddle_reference_count = 1U;
  plan.required_source_family_scan_count = 1U;
  plan.required_higher_order_saddle_family_count = 1U;
  plan.required_coface_deletion_reference_count = 7U;
  plan.required_distinct_facet_token_count = 7U;
  plan.required_facet_key_point_count = 18U;
  plan.required_batch_count = 2U;
  plan.required_direct_reference_count = 2U;
  plan.required_residual_reference_count = 1U;
  plan.logical_storage_entry_count = 37U;
  plan.source_pair_canonical_cloud_digest =
      source.source_pair_canonical_cloud_digest;
  plan.source_higher_canonical_cloud_digest =
      source.source_higher_canonical_cloud_digest;
  plan.source_pair_semantic_digest = source.source_pair_semantic_digest;
  plan.source_higher_semantic_digest = source.source_higher_semantic_digest;
  plan.facet_tokens = {
      {0U, facet(std::array<PointId, 2U>{0U, 1U}), std::nullopt, 1U, 1U},
      {1U, facet(std::array<PointId, 2U>{0U, 2U}), std::nullopt, 0U, 1U},
      {2U, facet(std::array<PointId, 2U>{1U, 2U}), std::nullopt, 0U, 1U},
      {3U, facet(std::array<PointId, 3U>{0U, 1U, 2U}), std::nullopt, 0U, 1U},
      {4U, facet(std::array<PointId, 3U>{0U, 1U, 3U}), std::nullopt, 0U, 1U},
      {5U, facet(std::array<PointId, 3U>{0U, 2U, 3U}), std::nullopt, 0U, 1U},
      {6U, facet(std::array<PointId, 3U>{1U, 2U, 3U}), std::nullopt, 0U, 1U},
  };
  plan.direct_references = {
      {0U,
       6U,
       6U,
       ExactDirectMorseH0Role::birth,
       std::nullopt,
       std::nullopt,
       0U},
      {1U,
       7U,
       7U,
       ExactDirectMorseH0Role::saddle,
       0U,
       0U,
       std::nullopt},
  };
  plan.residual_references = {{0U, 1U}};
  plan.coface_facet_references = {
      {0U, 0U, 0U, 0U, 2U},
      {1U, 0U, 1U, 1U, 1U},
      {2U, 0U, 2U, 2U, 0U},
      {3U, 1U, 0U, 0U, 6U},
      {4U, 1U, 1U, 1U, 5U},
      {5U, 1U, 2U, 2U, 4U},
      {6U, 1U, 3U, 3U, 3U},
  };
  plan.batches = {
      {0U, 0U, level(1), 2U, 0U, 2U, 0U, 0U, 0U, 3U},
      {1U, 1U, level(2), 3U, 2U, 0U, 0U, 1U, 3U, 4U},
  };
  plan.order_one_roles_and_families_excluded_to_preserve_boruvka_authority =
      true;
  plan.every_higher_order_direct_role_projected_once = true;
  plan.every_higher_order_saddle_role_joined_to_one_family = true;
  plan.facet_tokens_canonical_and_deduplicated = true;
  plan.unique_batch_per_exact_level_and_order = true;
  plan.direct_and_residual_same_level_order_share_one_future_snapshot = true;
  plan.batches_sorted_by_exact_level_then_order = true;
  plan.logical_storage_within_budget = true;
  plan.no_partial_scientific_payload_published = true;
  plan.no_k_plus_one_coface_key_persisted = true;
  plan.no_global_facet_or_coface_catalog_materialized = true;
  plan.partial_refinement_only = true;
  return plan;
}

[[nodiscard]] ExactDirectNormalizedH0ResidentBatchProviderBudget budget() {
  return {2U, 4U, 12U, 2U, 1U, 4U, 32U};
}

[[nodiscard]] ExactDirectSparseUnifiedLevelPlanResult empty_compatibility_plan(
    const ExactDirectNormalizedH0IncidenceReductionAuthority& source) {
  auto plan = compatibility_plan(source);
  plan.required_higher_order_direct_role_count = 0U;
  plan.required_direct_birth_reference_count = 0U;
  plan.required_direct_saddle_reference_count = 0U;
  plan.required_higher_order_saddle_family_count = 0U;
  plan.required_coface_deletion_reference_count = 0U;
  plan.required_distinct_facet_token_count = 0U;
  plan.required_facet_key_point_count = 0U;
  plan.required_batch_count = 0U;
  plan.required_direct_reference_count = 0U;
  plan.required_residual_reference_count = 0U;
  plan.logical_storage_entry_count = 0U;
  plan.facet_tokens.clear();
  plan.direct_references.clear();
  plan.residual_references.clear();
  plan.coface_facet_references.clear();
  plan.batches.clear();
  return plan;
}

class CompatibilityProvider {
 public:
  CompatibilityProvider(
      const ExactDirectNormalizedH0ResidentSourceManifest& manifest,
      const ExactDirectSparseUnifiedLevelPlanResult& plan)
      : manifest_(&manifest), plan_(&plan) {
    chain_prefixes_.push_back(manifest.initial_batch_chain_digest);
    ExactDirectNormalizedH0ResidentCompatibilityWindowScratch scratch;
    for (std::size_t index = 0U; index < manifest.batch_count; ++index) {
      const auto window =
          borrow_exact_direct_normalized_h0_resident_compatibility_window(
              manifest,
              plan,
              index,
              chain_prefixes_.back(),
              scratch);
      chain_prefixes_.push_back(window.successor_chain_digest);
    }
  }

  bool corrupt_first_chain{false};
  bool call_consumer_twice{false};

  ExactDirectNormalizedH0ResidentBatchVisitDecision operator()(
      std::size_t batch_index,
      ExactDirectNormalizedH0ResidentBatchConsumerView consumer) {
    if (!consumer) {
      return ExactDirectNormalizedH0ResidentBatchVisitDecision::
          no_consumer_rejected;
    }
    if (batch_index >= manifest_->batch_count) {
      return ExactDirectNormalizedH0ResidentBatchVisitDecision::
          no_batch_out_of_range;
    }
    auto window =
        borrow_exact_direct_normalized_h0_resident_compatibility_window(
            *manifest_,
            *plan_,
            batch_index,
            chain_prefixes_[batch_index],
            scratch_);
    if (corrupt_first_chain && batch_index == 0U) {
      window.source_chain_digest = {};
    }
    if (window.schema_version !=
        direct_normalized_h0_resident_batch_provider_schema_version) {
      return ExactDirectNormalizedH0ResidentBatchVisitDecision::
          no_window_inconsistent;
    }
    const bool accepted = consumer(window);
    if (call_consumer_twice) {
      static_cast<void>(consumer(window));
      // Deliberately malicious: ignore the permanent rejection of the second
      // callback and claim that the visit completed.
      return ExactDirectNormalizedH0ResidentBatchVisitDecision::
          complete_synchronous_visit;
    }
    return accepted
               ? ExactDirectNormalizedH0ResidentBatchVisitDecision::
                     complete_synchronous_visit
               : ExactDirectNormalizedH0ResidentBatchVisitDecision::
                     no_consumer_rejected;
  }

 private:
  const ExactDirectNormalizedH0ResidentSourceManifest* manifest_{};
  const ExactDirectSparseUnifiedLevelPlanResult* plan_{};
  std::vector<CanonicalId> chain_prefixes_;
  ExactDirectNormalizedH0ResidentCompatibilityWindowScratch scratch_;
};

void run_tests() {
  const auto source_authority = authority();
  check(
      source_authority.certified_horizontal_incidence_reduction(),
      "the compact horizontal authority is certified");
  const auto plan = compatibility_plan(source_authority);
  const auto manifest =
      build_exact_direct_normalized_h0_resident_source_manifest_from_compatibility_plan(
          plan, source_authority);
  check(manifest.certified(), "the O(1) source manifest is certified");
  check(
      !manifest.source_exactness_claimed && !manifest.vertical_maps_complete &&
          !manifest.public_status_claimed,
      "the provider does not promote source, vertical or public exactness");

  CompatibilityProvider provider{manifest, plan};
  ExactDirectNormalizedH0ResidentBatchProviderView provider_view{provider};
  const auto verification =
      verify_exact_direct_normalized_h0_resident_batch_provider(
          manifest, provider_view, budget());
  check(
      verification.result_certified &&
          verification.decision ==
              ExactDirectNormalizedH0ResidentProviderVerificationDecision::
                  complete_authenticated_stream_replay &&
          verification.batch_visit_count == 2U &&
          verification.consumer_callback_count == 2U &&
          verification.provider_identity == provider_view.identity() &&
          verification.provider_identity_bound &&
          verification.manifest_digest == manifest.manifest_digest &&
          !verification.source_exactness_claimed &&
          !verification.public_status_claimed &&
          verification.final_batch_chain_digest ==
              manifest.final_batch_chain_digest,
      "the replay binds the provider identity and manifest without an exactness claim");

  ExactDirectNormalizedH0ResidentCompatibilityWindowScratch scratch;
  const auto first =
      borrow_exact_direct_normalized_h0_resident_compatibility_window(
          manifest,
          plan,
          0U,
          manifest.initial_batch_chain_digest,
          scratch);
  check(
      first.certified_relative_to(manifest, budget()) &&
          first.facet_tokens.size() == 3U &&
          first.facet_tokens.front().stable_source_facet_token_index == 0U,
      "one batch borrows only its sorted local-to-stable facet window");
  const auto owned = copy_exact_direct_normalized_h0_resident_batch_window(
      manifest, first, budget());
  check(
      owned.certified_relative_to(manifest) &&
          owned.local_plan.batches.size() == 1U &&
          owned.local_to_stable_facet_token_indices ==
              std::vector<std::size_t>({0U, 1U, 2U}),
      "the ticket copy owns exactly one bounded compatibility image");

  auto scan_limited = budget();
  scan_limited.maximum_batch_scan_count = 1U;
  const auto scan_rejection =
      verify_exact_direct_normalized_h0_resident_batch_provider(
          manifest, provider, scan_limited);
  check(
      !scan_rejection.result_certified &&
          scan_rejection.decision ==
              ExactDirectNormalizedH0ResidentProviderVerificationDecision::
                  no_batch_budget_exhausted,
      "the authentication replay fails closed before exceeding its scan cap");
  auto window_limited = budget();
  window_limited.maximum_window_facet_count = 2U;
  check(
      !first.certified_relative_to(manifest, window_limited),
      "a borrowed window cannot exceed its facet budget");
  auto logical_limited = budget();
  logical_limited.maximum_window_logical_entry_count = 13U;
  check(
      !first.certified_relative_to(manifest, logical_limited),
      "the checked window accounting rejects an insufficient logical cap");

  CompatibilityProvider corrupt_provider{manifest, plan};
  corrupt_provider.corrupt_first_chain = true;
  const auto corrupt_verification =
      verify_exact_direct_normalized_h0_resident_batch_provider(
          manifest, corrupt_provider, budget());
  check(
      !corrupt_verification.result_certified &&
          corrupt_verification.decision ==
              ExactDirectNormalizedH0ResidentProviderVerificationDecision::
                  no_window_rejected,
      "a provider cannot alter a chain prefix after manifest authentication");

  CompatibilityProvider multiple_callback_provider{manifest, plan};
  multiple_callback_provider.call_consumer_twice = true;
  const auto multiple_callback_verification =
      verify_exact_direct_normalized_h0_resident_batch_provider(
          manifest, multiple_callback_provider, budget());
  check(
      !multiple_callback_verification.result_certified &&
          multiple_callback_verification.multiple_consumer_callbacks_observed &&
          multiple_callback_verification.consumer_callback_count == 2U &&
          multiple_callback_verification.decision ==
              ExactDirectNormalizedH0ResidentProviderVerificationDecision::
                  no_window_rejected,
      "a second callback permanently invalidates a visit even when ignored");

  auto empty_authority = source_authority;
  empty_authority.normalized_coface_count = 0U;
  empty_authority.normalized_batch_count = 0U;
  const auto empty_plan = empty_compatibility_plan(empty_authority);
  const auto empty_manifest =
      build_exact_direct_normalized_h0_resident_source_manifest_from_compatibility_plan(
          empty_plan, empty_authority);
  CompatibilityProvider empty_provider{empty_manifest, empty_plan};
  const ExactDirectNormalizedH0ResidentBatchProviderBudget empty_budget{};
  const auto empty_verification =
      verify_exact_direct_normalized_h0_resident_batch_provider(
          empty_manifest, empty_provider, empty_budget);
  check(
      empty_manifest.certified() && empty_manifest.batch_count == 0U &&
          empty_manifest.final_batch_chain_digest ==
              empty_manifest.initial_batch_chain_digest &&
          empty_verification.result_certified &&
          empty_verification.batch_visit_count == 0U &&
          empty_verification.consumer_callback_count == 0U &&
          !empty_verification.source_exactness_claimed,
      "an authenticated empty structural stream is certified vacuously");

  auto forged_manifest = manifest;
  forged_manifest.stable_facet_token_count += 1U;
  check(!forged_manifest.certified(), "a manifest scalar forgery is rejected");

  auto malformed_plan = plan;
  malformed_plan.coface_facet_references[0U].removed_point_id = 5U;
  bool malformed_rejected = false;
  try {
    static_cast<void>(
        build_exact_direct_normalized_h0_resident_source_manifest_from_compatibility_plan(
            malformed_plan, source_authority));
  } catch (const std::invalid_argument&) {
    malformed_rejected = true;
  }
  check(
      malformed_rejected,
      "the compatibility-only manifest builder rejects an invalid deletion");

  auto overflow_shaped_plan = plan;
  overflow_shaped_plan.required_direct_birth_reference_count =
      std::numeric_limits<std::size_t>::max();
  bool overflow_shape_rejected = false;
  try {
    static_cast<void>(
        build_exact_direct_normalized_h0_resident_source_manifest_from_compatibility_plan(
            overflow_shaped_plan, source_authority));
  } catch (const std::invalid_argument&) {
    overflow_shape_rejected = true;
  }
  check(
      overflow_shape_rejected,
      "an overflow-shaped source counter is rejected before cursor arithmetic");
}

}  // namespace

int main() {
  run_tests();
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "direct normalized H0 resident batch provider tests passed\n";
  return 0;
}
