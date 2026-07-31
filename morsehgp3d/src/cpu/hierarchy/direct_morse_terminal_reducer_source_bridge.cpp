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
  const bool pair_source_certified =
      (pair_source_kind == ExactDirectSupportPairSourceKind::
                               sealed_sparse_anchored_session &&
       !pair_terminal_authority_qualified_cuda_on_entry) ||
      (pair_source_kind == ExactDirectSupportPairSourceKind::
                               sealed_transactional_pair_terminal_authority &&
       pair_terminal_authority_qualified_cuda_on_entry);
  return schema_version ==
             direct_morse_terminal_reducer_source_bridge_schema_version &&
      requested_order_is_five_or_ten && min_cluster_size_positive &&
      pair_source_certified && pair_terminal_authority_sealed_on_entry &&
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
  // Impl is heap-owned, so the adapter address remains stable when the
  // enclosing bridge is moved.  A view obtained before that move must never
  // retain the address of the moved-from bridge object itself.
  return ExactDirectMorseForestSourceBatchProviderView{
      impl_->source_adapter};
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
ExactDirectMorseTerminalReducerSourceBridge::finish_build(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSaddleArmSeedBudget& seed_budget,
    std::size_t min_cluster_size,
    ExactDirectMorseTerminalReducerSourceBridgeAudit audit,
    ExactDirectSupportTerminalFacade facade) {
  ExactDirectMorseTerminalReducerSourceBridgeBuildResult result;
  result.audit = std::move(audit);
  ExactDirectMorseTerminalReducerSourceBridgeAudit& final_audit =
      result.audit;
  final_audit.point_count = cloud.size();
  final_audit.min_cluster_size = min_cluster_size;
  final_audit.effective_maximum_order =
      facade.certificate.requirements.effective_maximum_order;
  final_audit.source_authorities_match =
      facade.certificate.source_authorities_match &&
      facade.certificate.source_requirements_match &&
      facade.certificate.pair_source_kind == final_audit.pair_source_kind;
  final_audit.terminal_authorities_consumed_once =
      facade.certificate.pair_terminal_authority_consumed &&
      facade.certificate.pair_terminal_records_captured_once &&
      facade.certificate.higher_terminal_authority_consumed &&
      facade.certificate.higher_terminal_records_captured_once;
  final_audit.all_support_arities_two_through_four_terminal =
      facade.certificate.all_arities_terminal;
  final_audit.hierarchy_reduction_performed =
      facade.certificate.hierarchy_reduction_performed;
  final_audit.public_status_claimed =
      facade.certificate.public_status_claimed;
  final_audit.forbidden_global_structure_materialized =
      !facade.certificate.no_forbidden_global_structure_materialized;
  if (!facade.terminal_catalog_certified()) {
    final_audit.decision =
        ExactDirectMorseTerminalReducerSourceBridgeDecision::
            no_bridge_direct_support_catalog_not_terminal;
    return result;
  }

  final_audit.no_relevant_extra_shell_diagnostics =
      facade.relevant_extra_shell_diagnostics.empty();
  if (!final_audit.no_relevant_extra_shell_diagnostics) {
    final_audit.decision =
        ExactDirectMorseTerminalReducerSourceBridgeDecision::
            no_bridge_relevant_extra_shell_diagnostics;
    return result;
  }

  ExactDirectMorseEventJournalResult event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  final_audit.event_journal_certified =
      event_journal.certified_partial_refinement();
  final_audit.hierarchy_reduction_performed =
      final_audit.hierarchy_reduction_performed ||
      event_journal.hierarchy_reduction_performed;
  final_audit.public_status_claimed =
      final_audit.public_status_claimed ||
      event_journal.public_status_claimed;
  final_audit.forbidden_global_structure_materialized =
      final_audit.forbidden_global_structure_materialized ||
      !event_journal.no_forbidden_global_structure_materialized;
  if (!final_audit.event_journal_certified) {
    final_audit.decision =
        ExactDirectMorseTerminalReducerSourceBridgeDecision::
            no_bridge_event_journal_not_certified;
    return result;
  }

  ExactDirectSaddleArmSeedJournalResult seed_journal =
      build_exact_direct_saddle_arm_seed_journal(
          cloud, facade, event_journal, seed_budget);
  final_audit.saddle_arm_seed_journal_certified =
      seed_journal.certified_partial_refinement();
  final_audit.hierarchy_reduction_performed =
      final_audit.hierarchy_reduction_performed ||
      seed_journal.hierarchy_reduction_performed;
  final_audit.public_status_claimed =
      final_audit.public_status_claimed || seed_journal.public_status_claimed;
  final_audit.forbidden_global_structure_materialized =
      final_audit.forbidden_global_structure_materialized ||
      !seed_journal.no_forbidden_global_structure_materialized;
  if (!final_audit.saddle_arm_seed_journal_certified) {
    final_audit.decision =
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
  final_audit.reducer_source_manifest_certified =
      impl->source_adapter.manifest().certified();
  if (!final_audit.reducer_source_manifest_certified) {
    final_audit.decision =
        ExactDirectMorseTerminalReducerSourceBridgeDecision::
            no_bridge_reducer_source_manifest_not_certified;
    return result;
  }
  final_audit.synchronous_batch_provider_ready = true;
  final_audit.decision =
      ExactDirectMorseTerminalReducerSourceBridgeDecision::
          complete_certified_bounded_reducer_source;
  result.bridge = std::unique_ptr<
      ExactDirectMorseTerminalReducerSourceBridge>(
      new ExactDirectMorseTerminalReducerSourceBridge(
          std::move(impl), final_audit));
  if (!result.certified()) {
    result.bridge.reset();
    final_audit.reducer_source_manifest_certified = false;
    final_audit.synchronous_batch_provider_ready = false;
    final_audit.decision =
        ExactDirectMorseTerminalReducerSourceBridgeDecision::
            no_bridge_reducer_source_manifest_not_certified;
  }
  return result;
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
  ExactDirectMorseTerminalReducerSourceBridgeAudit& audit = result.audit;
  audit.point_count = cloud.size();
  audit.requested_maximum_order = requested_maximum_order;
  audit.min_cluster_size = min_cluster_size;
  audit.requested_order_is_five_or_ten =
      requested_maximum_order == 5U || requested_maximum_order == 10U;
  audit.min_cluster_size_positive = min_cluster_size != 0U;
  audit.pair_source_kind =
      ExactDirectSupportPairSourceKind::sealed_sparse_anchored_session;
  audit.min_cluster_size_is_downstream_view_only = true;
  audit.complete_equal_level_batch_required_before_cluster_size_view = true;
  audit.scope = ExactDirectMorseTerminalReducerSourceBridgeScope::
      terminal_direct_supports_arities_two_through_four_to_reducer_source_with_downstream_cluster_size_view;

  if (!audit.requested_order_is_five_or_ten) {
    audit.decision = ExactDirectMorseTerminalReducerSourceBridgeDecision::
        no_bridge_requested_order_not_five_or_ten;
    return result;
  }
  if (!audit.min_cluster_size_positive) {
    audit.decision = ExactDirectMorseTerminalReducerSourceBridgeDecision::
        no_bridge_min_cluster_size_zero;
    return result;
  }
  audit.pair_terminal_authority_sealed_on_entry =
      pair_authority.sealed_in_process_terminal_authority();
  if (!audit.pair_terminal_authority_sealed_on_entry) {
    audit.decision = ExactDirectMorseTerminalReducerSourceBridgeDecision::
        no_bridge_pair_terminal_authority_not_sealed;
    return result;
  }
  audit.higher_terminal_authority_sealed_on_entry =
      higher_authority.sealed_in_process_terminal_authority();
  if (!audit.higher_terminal_authority_sealed_on_entry) {
    audit.decision = ExactDirectMorseTerminalReducerSourceBridgeDecision::
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
  return ExactDirectMorseTerminalReducerSourceBridge::finish_build(
      cloud,
      seed_budget,
      min_cluster_size,
      std::move(audit),
      std::move(facade));
}

ExactDirectMorseTerminalReducerSourceBridgeBuildResult
build_exact_direct_morse_terminal_reducer_source_bridge(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactHigherSupportStreamBudget& higher_budget,
    const ExactDirectSaddleArmSeedBudget& seed_budget,
    std::size_t min_cluster_size,
    ExactDirectPairTerminalAuthority&& pair_authority,
    ExactHigherSupportTerminalAuthority&& higher_authority) {
  ExactDirectMorseTerminalReducerSourceBridgeBuildResult result;
  ExactDirectMorseTerminalReducerSourceBridgeAudit& audit = result.audit;
  audit.point_count = cloud.size();
  audit.requested_maximum_order = requested_maximum_order;
  audit.min_cluster_size = min_cluster_size;
  audit.requested_order_is_five_or_ten =
      requested_maximum_order == 5U || requested_maximum_order == 10U;
  audit.min_cluster_size_positive = min_cluster_size != 0U;
  audit.pair_source_kind = ExactDirectSupportPairSourceKind::
      sealed_transactional_pair_terminal_authority;
  audit.min_cluster_size_is_downstream_view_only = true;
  audit.complete_equal_level_batch_required_before_cluster_size_view = true;
  audit.scope = ExactDirectMorseTerminalReducerSourceBridgeScope::
      terminal_direct_supports_arities_two_through_four_to_reducer_source_with_downstream_cluster_size_view;

  if (!audit.requested_order_is_five_or_ten) {
    audit.decision = ExactDirectMorseTerminalReducerSourceBridgeDecision::
        no_bridge_requested_order_not_five_or_ten;
    return result;
  }
  if (!audit.min_cluster_size_positive) {
    audit.decision = ExactDirectMorseTerminalReducerSourceBridgeDecision::
        no_bridge_min_cluster_size_zero;
    return result;
  }
  audit.pair_terminal_authority_sealed_on_entry =
      pair_authority.sealed_in_process_terminal_authority();
  if (!audit.pair_terminal_authority_sealed_on_entry) {
    audit.decision = ExactDirectMorseTerminalReducerSourceBridgeDecision::
        no_bridge_pair_terminal_authority_not_sealed;
    return result;
  }
  const ExactDirectPairTerminalAudit& pair_audit = pair_authority.audit();
  audit.pair_terminal_authority_qualified_cuda_on_entry =
      pair_audit.source_kind == ExactDirectPairTerminalSourceKind::
                                    qualified_cuda_transactional_pair_cut &&
      pair_audit.qualified_cuda_execution &&
      !pair_audit.host_fake_execution;
  if (!audit.pair_terminal_authority_qualified_cuda_on_entry) {
    audit.decision = ExactDirectMorseTerminalReducerSourceBridgeDecision::
        no_bridge_pair_terminal_authority_not_qualified_cuda;
    return result;
  }
  audit.higher_terminal_authority_sealed_on_entry =
      higher_authority.sealed_in_process_terminal_authority();
  if (!audit.higher_terminal_authority_sealed_on_entry) {
    audit.decision = ExactDirectMorseTerminalReducerSourceBridgeDecision::
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
  return ExactDirectMorseTerminalReducerSourceBridge::finish_build(
      cloud,
      seed_budget,
      min_cluster_size,
      std::move(audit),
      std::move(facade));
}

}  // namespace morsehgp3d::hierarchy
