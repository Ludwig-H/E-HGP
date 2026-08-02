#include "morsehgp3d/hierarchy/direct_normalized_h0_authenticated_window_stream.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::contract::CanonicalId;
using morsehgp3d::contract::CanonicalSha256Builder;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::PointId;

static_assert(
    !std::is_copy_constructible_v<
        ExactDirectNormalizedH0AuthenticatedWindowStreamSession> &&
    std::is_nothrow_move_constructible_v<
        ExactDirectNormalizedH0AuthenticatedWindowStreamSession>);
static_assert(
    !std::is_copy_constructible_v<
        ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow> &&
    std::is_nothrow_move_constructible_v<
        ExactDirectNormalizedH0AuthenticatedWindowStreamPreparedWindow>);

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

[[nodiscard]] ExactDirectNormalizedH0IncidenceReductionAuthority
source_authority() {
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

[[nodiscard]] ExactDirectNormalizedH0ResidentBatchProviderBudget
provider_budget() {
  return {2U, 4U, 12U, 2U, 1U, 4U, 32U};
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

  bool corrupt_chain{false};
  bool replay_previous{false};
  bool duplicate_callback{false};

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
    const std::size_t supplied_index =
        replay_previous && batch_index != 0U ? batch_index - 1U : batch_index;
    auto window =
        borrow_exact_direct_normalized_h0_resident_compatibility_window(
            *manifest_,
            *plan_,
            supplied_index,
            chain_prefixes_[supplied_index],
            scratch_);
    if (corrupt_chain) {
      window.source_chain_digest = {};
    }
    const bool first = consumer(window);
    if (duplicate_callback) {
      const bool second = consumer(window);
      return first && second
                 ? ExactDirectNormalizedH0ResidentBatchVisitDecision::
                       complete_synchronous_visit
                 : ExactDirectNormalizedH0ResidentBatchVisitDecision::
                       no_consumer_rejected;
    }
    return first
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
  const auto authority = source_authority();
  const auto plan = compatibility_plan(authority);
  const auto manifest =
      build_exact_direct_normalized_h0_resident_source_manifest_from_compatibility_plan(
          plan, authority);
  CompatibilityProvider provider{manifest, plan};
  const auto verification =
      verify_exact_direct_normalized_h0_resident_batch_provider(
          manifest, provider, provider_budget());
  check(
      verification.result_certified &&
          verification.provider_identity == &provider &&
          verification.provider_identity_bound &&
          verification.manifest_digest == manifest.manifest_digest &&
          !verification.source_exactness_claimed &&
          !verification.public_status_claimed,
      "the prerequisite replay is bound to this provisional provider");

  const ExactDirectNormalizedH0AuthenticatedWindowStreamBudget budget{
      provider_budget(), 1U};
  auto initialization =
      initialize_exact_direct_normalized_h0_authenticated_window_stream(
          manifest, verification, provider, 71U, budget);
  check(
      initialization.certified_provisional_initialization() &&
          !initialization.resident_scientific_state_initialized &&
          !initialization.source_exactness_claimed &&
          !initialization.public_status_claimed,
      "initialization binds one provider without promoting source exactness");
  auto& session = *initialization.session;
  const auto genesis_checkpoint = session.make_checkpoint();
  check(
      genesis_checkpoint.certified_honest_nonrestartable_checkpoint() &&
          genesis_checkpoint.batch_cursor == 0U &&
          !genesis_checkpoint.restartable &&
          !genesis_checkpoint.resident_or_locator_state_serialized,
      "the genesis checkpoint is explicitly metadata-only and nonrestartable");
  const auto early_seal = session.seal();
  check(
      !early_seal.certified_provisional_seal() &&
          early_seal.decision ==
              ExactDirectNormalizedH0AuthenticatedWindowStreamSealDecision::
                  no_source_not_exhausted,
      "a nonterminal stream cannot be sealed");

  auto first = session.prepare_next();
  check(
      first.certified_provisional_preparation() &&
          first.ticket->owned_window().source_batch_index == 0U &&
          first.ticket->owned_window().local_plan.batches.size() == 1U,
      "prepare owns exactly one authenticated first window");
  const auto second_while_live = session.prepare_next();
  check(
      second_while_live.decision ==
          ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
              no_outstanding_ticket_budget_exhausted,
      "a second live ticket is rejected before another provider visit");

  auto foreign_initialization =
      initialize_exact_direct_normalized_h0_authenticated_window_stream(
          manifest, verification, provider, 72U, budget);
  check(
      foreign_initialization.certified_provisional_initialization(),
      "a second session has a distinct private seal");
  const auto foreign_commit = foreign_initialization.session->commit(
      std::move(*first.ticket));
  check(
      foreign_commit.decision ==
              ExactDirectNormalizedH0AuthenticatedWindowStreamCommitDecision::
                  no_foreign_ticket_rejected &&
          foreign_commit.ticket_consumed && session.batch_cursor() == 0U,
      "a ticket cannot cross session authority boundaries");

  auto committed_first = session.prepare_next();
  const auto first_commit = session.commit(std::move(*committed_first.ticket));
  check(
      first_commit.certified_provisional_commit() &&
          first_commit.source_batch_index == 0U &&
          session.batch_cursor() == 1U && session.epoch() == 1U &&
          !first_commit.resident_scientific_state_mutated,
      "commit advances only cursor, epoch and authenticated chain metadata");
  const auto consumed_replay =
      session.commit(std::move(*committed_first.ticket));
  check(
      consumed_replay.decision ==
          ExactDirectNormalizedH0AuthenticatedWindowStreamCommitDecision::
              no_ticket_invalid_or_consumed,
      "a consumed move-only ticket cannot be replayed");

  provider.replay_previous = true;
  const auto replayed_window = session.prepare_next();
  check(
      replayed_window.decision ==
          ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
              no_window_prefix_or_cursor_rejected,
      "a provider replaying the prior cursor is rejected");
  provider.replay_previous = false;
  provider.corrupt_chain = true;
  const auto mutated_window = session.prepare_next();
  check(
      mutated_window.decision ==
          ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
              no_window_prefix_or_cursor_rejected,
      "a provider mutation that breaks the committed prefix is rejected");
  provider.corrupt_chain = false;
  provider.duplicate_callback = true;
  const auto duplicate_window = session.prepare_next();
  check(
      duplicate_window.decision ==
          ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
              no_provider_callback_cardinality_rejected,
      "a provider cannot publish the same window callback twice");
  provider.duplicate_callback = false;

  CompatibilityProvider foreign_provider{manifest, plan};
  const auto foreign_provider_initialization =
      initialize_exact_direct_normalized_h0_authenticated_window_stream(
          manifest, verification, foreign_provider, 73U, budget);
  check(
      !foreign_provider_initialization.session.has_value() &&
          foreign_provider_initialization.decision ==
              ExactDirectNormalizedH0AuthenticatedWindowStreamInitializationDecision::
                  no_provider_identity_rejected,
      "a replay receipt cannot initialize a foreign provider object");

  auto final_window = session.prepare_next();
  check(
      final_window.certified_provisional_preparation() &&
          final_window.ticket->owned_window().source_batch_index == 1U,
      "the second exact prefix prepares after all rejection paths");
  const auto outstanding_seal = session.seal();
  check(
      outstanding_seal.decision ==
          ExactDirectNormalizedH0AuthenticatedWindowStreamSealDecision::
              no_outstanding_ticket_rejected,
      "a live ticket blocks terminal sealing");
  const auto final_commit = session.commit(std::move(*final_window.ticket));
  check(
      final_commit.certified_provisional_commit() && session.complete() &&
          !session.sealed() && session.batch_cursor() == manifest.batch_count &&
          session.current_chain_digest() == manifest.final_batch_chain_digest,
      "the terminal cursor is complete only at the manifest chain");
  const auto exhausted = session.prepare_next();
  check(
      exhausted.decision ==
          ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
              no_source_exhausted,
      "an unsealed but complete source cannot replay a terminal window");

  const auto final_seal = session.seal();
  check(
      final_seal.certified_provisional_seal() && final_seal.newly_sealed &&
          session.sealed() && !final_seal.seal->resident_fold_executed &&
          !final_seal.seal->source_exactness_claimed &&
          !final_seal.seal->public_status_claimed,
      "terminal sealing remains provisional and resident-free");
  const auto idempotent_seal = session.seal();
  check(
      idempotent_seal.certified_provisional_seal() &&
          !idempotent_seal.newly_sealed &&
          idempotent_seal.seal == final_seal.seal,
      "terminal sealing is idempotent");
  const auto sealed_prepare = session.prepare_next();
  check(
      sealed_prepare.decision ==
          ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
              no_stream_sealed,
      "a sealed stream rejects all further preparation");
  const auto sealed_checkpoint = session.make_checkpoint();
  check(
      sealed_checkpoint.certified_honest_nonrestartable_checkpoint() &&
          sealed_checkpoint.stream_complete &&
          sealed_checkpoint.stream_sealed &&
          !sealed_checkpoint.checkpoint_restore_supported,
      "even a sealed checkpoint never claims durable restoration");
}

}  // namespace

int main() {
  run_tests();
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "direct normalized H0 authenticated window stream tests passed\n";
  return 0;
}
