#include "morsehgp3d/hierarchy/direct_morse_terminal_reducer_source_bridge.hpp"

#include <memory>
#include <utility>

namespace morsehgp3d::hierarchy {

struct ExactDirectMorseTerminalReducerSourceBridge::Impl {
  Impl(
      const spatial::CanonicalPointCloud& cloud,
      ExactDirectSaddleArmSeedBudget seed_budget,
      ExactDirectSupportTerminalFacade source_facade,
      ExactDirectMorseEventJournalResult source_event_journal,
      ExactDirectSaddleArmSeedJournalResult source_seed_journal)
      : trusted_seed_budget(std::move(seed_budget)),
        facade(std::move(source_facade)),
        event_journal(std::move(source_event_journal)),
        seed_journal(std::move(source_seed_journal)),
        source_adapter(
            cloud,
            facade,
            event_journal,
            trusted_seed_budget,
            seed_journal) {}

  ExactDirectSaddleArmSeedBudget trusted_seed_budget{};
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult event_journal;
  ExactDirectSaddleArmSeedJournalResult seed_journal;
  ExactDirectMorseForestResidentSourceAdapter source_adapter;
};

bool ExactDirectMorseTerminalReducerSourceBridgeAudit::certified()
    const noexcept {
  return schema_version ==
             direct_morse_terminal_reducer_source_bridge_schema_version &&
      requested_order_is_five_or_ten && min_cluster_size_positive &&
      pair_terminal_authority_sealed_on_entry &&
      higher_terminal_authority_sealed_on_entry &&
      source_authorities_match && terminal_authorities_consumed_once &&
      all_support_arities_two_through_four_terminal &&
      no_relevant_extra_shell_diagnostics && event_journal_certified &&
      saddle_arm_seed_journal_certified &&
      reducer_source_manifest_certified && synchronous_batch_provider_ready &&
      min_cluster_size_is_downstream_view_only &&
      complete_equal_level_batch_required_before_cluster_size_view &&
      !source_pruning_by_min_cluster_size_performed &&
      !forbidden_global_structure_materialized &&
      !hierarchy_reduction_performed && !public_status_claimed &&
      decision == ExactDirectMorseTerminalReducerSourceBridgeDecision::
                      complete_certified_bounded_reducer_source &&
      scope == ExactDirectMorseTerminalReducerSourceBridgeScope::
                   terminal_direct_supports_arities_two_through_four_to_reducer_source_with_downstream_cluster_size_view;
}

ExactDirectMorseTerminalReducerSourceBridge::
    ExactDirectMorseTerminalReducerSourceBridge(
        std::unique_ptr<Impl> impl,
        ExactDirectMorseTerminalReducerSourceBridgeAudit audit) noexcept
    : impl_(std::move(impl)), audit_(std::move(audit)) {}

ExactDirectMorseTerminalReducerSourceBridge::
    ExactDirectMorseTerminalReducerSourceBridge(
        ExactDirectMorseTerminalReducerSourceBridge&&) noexcept = default;

ExactDirectMorseTerminalReducerSourceBridge&
ExactDirectMorseTerminalReducerSourceBridge::operator=(
    ExactDirectMorseTerminalReducerSourceBridge&&) noexcept = default;

ExactDirectMorseTerminalReducerSourceBridge::
    ~ExactDirectMorseTerminalReducerSourceBridge() = default;

const ExactDirectSupportTerminalFacade&
ExactDirectMorseTerminalReducerSourceBridge::direct_support_facade()
    const & noexcept {
  return impl_->facade;
}

const ExactDirectMorseEventJournalResult&
ExactDirectMorseTerminalReducerSourceBridge::event_journal()
    const & noexcept {
  return impl_->event_journal;
}

const ExactDirectSaddleArmSeedJournalResult&
ExactDirectMorseTerminalReducerSourceBridge::saddle_arm_seed_journal()
    const & noexcept {
  return impl_->seed_journal;
}

const ExactDirectSaddleArmSeedBudget&
ExactDirectMorseTerminalReducerSourceBridge::trusted_seed_budget()
    const & noexcept {
  return impl_->trusted_seed_budget;
}

const ExactDirectMorseForestSourceManifest&
ExactDirectMorseTerminalReducerSourceBridge::source_manifest()
    const & noexcept {
  return impl_->source_adapter.manifest();
}

ExactDirectMorseForestSourceBatchProviderView
ExactDirectMorseTerminalReducerSourceBridge::source_provider() & noexcept {
  return ExactDirectMorseForestSourceBatchProviderView{*this};
}

ExactDirectMorseForestSourceBatchVisitDecision
ExactDirectMorseTerminalReducerSourceBridge::visit_source_batch(
    std::size_t batch_index,
    ExactDirectMorseForestSourceBatchConsumerView consumer) const {
  return impl_->source_adapter.visit_batch(batch_index, consumer);
}

ExactDirectMorseForestSourceBatchVisitDecision
ExactDirectMorseTerminalReducerSourceBridge::operator()(
    std::size_t batch_index,
    ExactDirectMorseForestSourceBatchConsumerView consumer) {
  return visit_source_batch(batch_index, consumer);
}

CappedDistinctPointCoverage ExactDirectMorseTerminalReducerSourceBridge::
    make_downstream_complete_component_coverage() const {
  return CappedDistinctPointCoverage{audit_.min_cluster_size};
}

bool ExactDirectMorseTerminalReducerSourceBridgeBuildResult::certified()
    const noexcept {
  return bridge != nullptr && audit.certified() &&
      bridge->audit() == audit && bridge->source_manifest().certified();
}

ExactDirectMorseTerminalReducerSourceBridgeBuildResult
build_exact_direct_morse_terminal_reducer_source_bridge(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactHigherSupportStreamBudget& higher_budget,
    const ExactDirectSaddleArmSeedBudget& seed_budget,
    std::size_t min_cluster_size,
    ExactSparseAnchoredPairTerminalAuthority&& pair_authority,
    ExactHigherSupportTerminalAuthority&& higher_authority) {
  ExactDirectMorseTerminalReducerSourceBridgeBuildResult result;
  auto& audit = result.audit;
  audit.point_count = cloud.size();
  audit.requested_maximum_order = requested_maximum_order;
  audit.min_cluster_size = min_cluster_size;
  audit.requested_order_is_five_or_ten =
      requested_maximum_order == 5U || requested_maximum_order == 10U;
  audit.min_cluster_size_positive = min_cluster_size != 0U;
  audit.min_cluster_size_is_downstream_view_only = true;
  audit.complete_equal_level_batch_required_before_cluster_size_view = true;
  audit.scope = ExactDirectMorseTerminalReducerSourceBridgeScope::
      terminal_direct_supports_arities_two_through_four_to_reducer_source_with_downstream_cluster_size_view;

  if (!audit.requested_order_is_five_or_ten) {
    audit.decision =
        ExactDirectMorseTerminalReducerSourceBridgeDecision::
            no_bridge_requested_order_not_five_or_ten;
    return result;
  }
  if (!audit.min_cluster_size_positive) {
    audit.decision =
        ExactDirectMorseTerminalReducerSourceBridgeDecision::
            no_bridge_min_cluster_size_zero;
    return result;
  }

  audit.pair_terminal_authority_sealed_on_entry =
      pair_authority.sealed_in_process_terminal_authority();
  if (!audit.pair_terminal_authority_sealed_on_entry) {
    audit.decision =
        ExactDirectMorseTerminalReducerSourceBridgeDecision::
            no_bridge_pair_terminal_authority_not_sealed;
    return result;
  }
  audit.higher_terminal_authority_sealed_on_entry =
      higher_authority.sealed_in_process_terminal_authority();
  if (!audit.higher_terminal_authority_sealed_on_entry) {
    audit.decision =
        ExactDirectMorseTerminalReducerSourceBridgeDecision::
            no_bridge_higher_terminal_authority_not_sealed;
    return result;
  }

  ExactDirectSupportTerminalFacade facade =
      build_exact_direct_support_terminal_facade(
          index,
          cloud,
          requested_maximum_order,
          higher_budget,
          std::move(pair_authority),
          std::move(higher_authority));
  audit.effective_maximum_order =
      facade.certificate.requirements.effective_maximum_order;
  audit.source_authorities_match =
      facade.certificate.source_authorities_match &&
      facade.certificate.source_requirements_match;
  audit.terminal_authorities_consumed_once =
      facade.certificate.pair_terminal_authority_consumed &&
      facade.certificate.pair_terminal_records_captured_once &&
      facade.certificate.higher_terminal_authority_consumed &&
      facade.certificate.higher_terminal_records_captured_once;
  audit.all_support_arities_two_through_four_terminal =
      facade.certificate.all_arities_terminal;
  audit.hierarchy_reduction_performed =
      facade.certificate.hierarchy_reduction_performed;
  audit.public_status_claimed = facade.certificate.public_status_claimed;
  audit.forbidden_global_structure_materialized =
      !facade.certificate.no_forbidden_global_structure_materialized;
  if (!facade.terminal_catalog_certified()) {
    audit.decision =
        ExactDirectMorseTerminalReducerSourceBridgeDecision::
            no_bridge_direct_support_catalog_not_terminal;
    return result;
  }

  audit.no_relevant_extra_shell_diagnostics =
      facade.relevant_extra_shell_diagnostics.empty();
  if (!audit.no_relevant_extra_shell_diagnostics) {
    audit.decision =
        ExactDirectMorseTerminalReducerSourceBridgeDecision::
            no_bridge_relevant_extra_shell_diagnostics;
    return result;
  }

  ExactDirectMorseEventJournalResult event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  audit.event_journal_certified =
      event_journal.certified_partial_refinement();
  audit.hierarchy_reduction_performed =
      audit.hierarchy_reduction_performed ||
      event_journal.hierarchy_reduction_performed;
  audit.public_status_claimed =
      audit.public_status_claimed || event_journal.public_status_claimed;
  audit.forbidden_global_structure_materialized =
      audit.forbidden_global_structure_materialized ||
      !event_journal.no_forbidden_global_structure_materialized;
  if (!audit.event_journal_certified) {
    audit.decision =
        ExactDirectMorseTerminalReducerSourceBridgeDecision::
            no_bridge_event_journal_not_certified;
    return result;
  }

  ExactDirectSaddleArmSeedJournalResult seed_journal =
      build_exact_direct_saddle_arm_seed_journal(
          cloud, facade, event_journal, seed_budget);
  audit.saddle_arm_seed_journal_certified =
      seed_journal.certified_partial_refinement();
  audit.hierarchy_reduction_performed =
      audit.hierarchy_reduction_performed ||
      seed_journal.hierarchy_reduction_performed;
  audit.public_status_claimed =
      audit.public_status_claimed || seed_journal.public_status_claimed;
  audit.forbidden_global_structure_materialized =
      audit.forbidden_global_structure_materialized ||
      !seed_journal.no_forbidden_global_structure_materialized;
  if (!audit.saddle_arm_seed_journal_certified) {
    audit.decision =
        ExactDirectMorseTerminalReducerSourceBridgeDecision::
            no_bridge_saddle_arm_seed_journal_not_certified;
    return result;
  }

  auto impl =
      std::make_unique<ExactDirectMorseTerminalReducerSourceBridge::Impl>(
          cloud,
          seed_budget,
          std::move(facade),
          std::move(event_journal),
          std::move(seed_journal));
  audit.reducer_source_manifest_certified =
      impl->source_adapter.manifest().certified();
  if (!audit.reducer_source_manifest_certified) {
    audit.decision =
        ExactDirectMorseTerminalReducerSourceBridgeDecision::
            no_bridge_reducer_source_manifest_not_certified;
    return result;
  }
  audit.synchronous_batch_provider_ready = true;
  audit.decision =
      ExactDirectMorseTerminalReducerSourceBridgeDecision::
          complete_certified_bounded_reducer_source;
  result.bridge = std::unique_ptr<
      ExactDirectMorseTerminalReducerSourceBridge>(
      new ExactDirectMorseTerminalReducerSourceBridge(
          std::move(impl), audit));
  if (!result.certified()) {
    result.bridge.reset();
    audit.reducer_source_manifest_certified = false;
    audit.synchronous_batch_provider_ready = false;
    audit.decision =
        ExactDirectMorseTerminalReducerSourceBridgeDecision::
            no_bridge_reducer_source_manifest_not_certified;
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
