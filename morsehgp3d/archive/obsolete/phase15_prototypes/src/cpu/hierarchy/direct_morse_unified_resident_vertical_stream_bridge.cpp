// Archived Phase 15 prototype; excluded from the active API and build.

#include "morsehgp3d/hierarchy/direct_morse_unified_resident_vertical_stream_bridge.hpp"

#include <algorithm>
#include <exception>
#include <limits>
#include <memory>
#include <new>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

[[nodiscard]] bool add_overflow(
    std::size_t left,
    std::size_t right,
    std::size_t& sum) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return true;
  }
  sum = left + right;
  return false;
}

[[nodiscard]] bool increment_bounded(
    std::size_t& value,
    std::size_t increment,
    std::size_t maximum) noexcept {
  std::size_t next = 0U;
  if (add_overflow(value, increment, next) || next > maximum) {
    return false;
  }
  value = next;
  return true;
}

[[nodiscard]] bool canonical_key_shape(
    const ExactDirectSparseFacetKey& key) noexcept {
  if (key.point_count == 0U ||
      key.point_count > direct_sparse_positive_facet_maximum_point_count) {
    return false;
  }
  for (std::size_t index = 1U; index < key.point_count; ++index) {
    if (key.point_ids[index - 1U] >= key.point_ids[index]) {
      return false;
    }
  }
  for (std::size_t index = key.point_count;
       index < key.point_ids.size();
       ++index) {
    if (key.point_ids[index] != spatial::PointId{}) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool key_less(
    const ExactDirectSparseFacetKey& left,
    const ExactDirectSparseFacetKey& right) noexcept {
  const std::size_t common = std::min(left.point_count, right.point_count);
  for (std::size_t index = 0U; index < common; ++index) {
    if (left.point_ids[index] < right.point_ids[index]) {
      return true;
    }
    if (right.point_ids[index] < left.point_ids[index]) {
      return false;
    }
  }
  return left.point_count < right.point_count;
}

[[nodiscard]] bool exact_common_codimension_one_facet(
    const ExactDirectSparseFacetKey& left,
    const ExactDirectSparseFacetKey& right,
    std::size_t source_order,
    std::size_t maximum_point_scans,
    std::size_t& point_scans,
    ExactDirectSparseFacetKey& common) noexcept {
  common = {};
  if (source_order < 2U || left.point_count != source_order ||
      right.point_count != source_order || !canonical_key_shape(left) ||
      !canonical_key_shape(right) || left == right) {
    return false;
  }
  std::size_t left_index = 0U;
  std::size_t right_index = 0U;
  while (left_index < left.point_count &&
         right_index < right.point_count) {
    if (!increment_bounded(point_scans, 2U, maximum_point_scans)) {
      return false;
    }
    if (left.point_ids[left_index] < right.point_ids[right_index]) {
      ++left_index;
      continue;
    }
    if (right.point_ids[right_index] < left.point_ids[left_index]) {
      ++right_index;
      continue;
    }
    if (common.point_count >= common.point_ids.size()) {
      return false;
    }
    common.point_ids[common.point_count] = left.point_ids[left_index];
    ++common.point_count;
    ++left_index;
    ++right_index;
  }
  return common.point_count + 1U == source_order &&
         canonical_key_shape(common);
}

[[nodiscard]] bool action_matches_qr(
    std::size_t q_r,
    ExactFrozenIncidenceHgpAction action) noexcept {
  if (q_r == 0U) {
    return action == ExactFrozenIncidenceHgpAction::reduced_birth;
  }
  if (q_r == 1U) {
    return action == ExactFrozenIncidenceHgpAction::continuation;
  }
  return action == ExactFrozenIncidenceHgpAction::multifusion;
}

[[nodiscard]] bool disposition_matches_qr(
    std::size_t q_r,
    ExactFrozenIncidenceQuotientDisposition disposition) noexcept {
  if (q_r == 0U) {
    return disposition == ExactFrozenIncidenceQuotientDisposition::q_r_zero;
  }
  if (q_r == 1U) {
    return disposition == ExactFrozenIncidenceQuotientDisposition::q_r_one;
  }
  return disposition ==
         ExactFrozenIncidenceQuotientDisposition::q_r_multiple;
}

struct VerticalBridgeSeal {
  std::uint64_t bridge_authority_id{};
  std::uint64_t resident_session_authority_id{};
};

struct VerticalOutstandingTicketRegistry {
  std::size_t live_ticket_count{};
};

struct VerticalCarrierPatch {
  std::size_t carrier_index{};
  bool append{false};
  ExactDirectMorseUnifiedResidentVerticalRootCarrier next{};
};

struct VerticalExpectedGroupRecord {
  std::size_t group_record_index{};
  std::size_t owner_group_index{};
  ExactFrozenIncidencePriorRootId resultant_root_id{};
  std::size_t prior_root_offset{};
  std::size_t prior_root_count{};
  std::size_t q_r{};
  ExactFrozenIncidenceHgpAction action{
      ExactFrozenIncidenceHgpAction::reduced_birth};
};

struct TargetResolution {
  std::uint64_t root_id{};
  ExactDirectSparseFacetWitness binding_witness{};
  ExactDirectMorseUnifiedResidentVerticalTargetKind kind{
      ExactDirectMorseUnifiedResidentVerticalTargetKind::
          resident_adjacent_lower_order_root};
};

struct TargetResolutionContext {
  const ExactDirectMorseUnifiedResidentSession& resident;
  const ExactDirectMorseUnifiedResidentExternalK2K1BatchReceipt* seam_batch;
  const ExactDirectMorseUnifiedResidentVerticalStreamBridgeBudget& budget;
  std::uint64_t resident_authority_id{};
  std::uint64_t next_query_token{};
  std::size_t source_order{};
  ExactDirectMorseUnifiedResidentVerticalBatchCounters& counters;
  ExactDirectMorseUnifiedResidentVerticalPreparationDecision failure{
      ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
          no_bridge_target_carrier_not_rooted};
};

[[nodiscard]] bool next_probe_witness(
    TargetResolutionContext& context,
    ExactDirectSparseFacetWitness& witness) noexcept {
  if (context.next_query_token == 0U ||
      context.next_query_token == std::numeric_limits<std::uint64_t>::max()) {
    context.failure = ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
        no_bridge_capacity_overflow;
    return false;
  }
  witness = {context.resident_authority_id, context.next_query_token};
  ++context.next_query_token;
  return true;
}

[[nodiscard]] bool scan_external_binding(
    TargetResolutionContext& context,
    const ExactDirectSparseFacetKey& key,
    TargetResolution& resolution) noexcept {
  if (context.seam_batch == nullptr || key.point_count != 1U) {
    context.failure = ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
        no_bridge_external_seam_rejected;
    return false;
  }
  const auto& bindings = context.seam_batch->bindings;
  std::size_t begin = 0U;
  std::size_t end = bindings.size();
  while (begin < end) {
    if (!increment_bounded(
            context.counters.external_seam_binding_scan_count,
            1U,
            context.budget.maximum_external_seam_binding_scan_count)) {
      context.failure = ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
          no_bridge_budget_exhausted;
      return false;
    }
    const std::size_t middle = begin + (end - begin) / 2U;
    if (bindings[middle].point_id < key.point_ids[0U]) {
      begin = middle + 1U;
    } else {
      end = middle;
    }
  }
  if (begin >= bindings.size() ||
      bindings[begin].point_id != key.point_ids[0U] ||
      bindings[begin].closed_k1_root_id == 0U ||
      begin == std::numeric_limits<std::uint64_t>::max()) {
    context.failure = ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
        no_bridge_target_carrier_not_rooted;
    return false;
  }
  resolution.root_id = bindings[begin].closed_k1_root_id;
  resolution.binding_witness = {
      context.seam_batch->external_seam_authority_id,
      static_cast<std::uint64_t>(begin) + 1U};
  resolution.kind = ExactDirectMorseUnifiedResidentVerticalTargetKind::
      external_k1_root;
  return true;
}

[[nodiscard]] bool probe_resident_target(
    TargetResolutionContext& context,
    const ExactDirectSparseFacetKey& key,
    TargetResolution& resolution) noexcept {
  ExactDirectSparseFacetWitness query_witness;
  if (!next_probe_witness(context, query_witness)) {
    return false;
  }
  if (!increment_bounded(
          context.counters.target_probe_count,
          1U,
          context.budget.maximum_batch_target_probe_count)) {
    context.failure = ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
        no_bridge_budget_exhausted;
    return false;
  }
  const auto probe = context.resident.locator().probe_positive_facet(
      key, query_witness, context.budget.target_probe_budget);
  if (!increment_bounded(
          context.counters.target_probe_slot_visit_count,
          probe.slot_visit_count,
          context.budget.maximum_batch_target_probe_slot_visit_count) ||
      !increment_bounded(
          context.counters.target_probe_parent_hop_count,
          probe.component_parent_hop_count,
          context.budget.maximum_batch_target_probe_parent_hop_count)) {
    context.failure = ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
        no_bridge_budget_exhausted;
    return false;
  }
  if (!probe.certified_positive_hit() ||
      probe.component_handle >= context.resident.component_states().size()) {
    context.failure = ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
        no_bridge_target_carrier_not_rooted;
    return false;
  }
  const auto& component =
      context.resident.component_states()[probe.component_handle];
  if (!component.active ||
      component.component_handle != probe.component_handle ||
      component.parent_handle != probe.component_handle ||
      !component.root_id.has_value() || *component.root_id == 0U) {
    context.failure = ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
        no_bridge_target_carrier_not_rooted;
    return false;
  }
  resolution.root_id = *component.root_id;
  resolution.binding_witness = probe.source_binding_witness;
  resolution.kind = ExactDirectMorseUnifiedResidentVerticalTargetKind::
      resident_adjacent_lower_order_root;
  return true;
}

[[nodiscard]] bool resolve_target(
    TargetResolutionContext& context,
    const ExactDirectSparseFacetKey& key,
    TargetResolution& resolution) noexcept {
  if (!canonical_key_shape(key) ||
      key.point_count + 1U != context.source_order) {
    context.failure = ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
        no_bridge_common_target_facet_rejected;
    return false;
  }
  if (context.source_order == 2U) {
    return scan_external_binding(context, key, resolution);
  }
  return probe_resident_target(context, key, resolution);
}

[[nodiscard]] bool external_batch_shape_certified(
    const ExactDirectMorseUnifiedResidentExternalK2K1BatchReceipt& batch,
    const ExactDirectMorseUnifiedResidentExternalK2K1SeamAuthority& authority,
    const ExactDirectMorseUnifiedResidentAuthorityBundle& resident_bundle,
    const ExactDirectMorseUnifiedResidentVerticalStreamBridgeBudget& budget,
    std::size_t& binding_scans) noexcept {
  if (!authority.certified_external_seam() ||
      batch.schema_version !=
          direct_morse_unified_resident_vertical_stream_bridge_schema_version ||
      batch.external_seam_authority_id != authority.external_seam_authority_id ||
      batch.resident_session_authority_id !=
          resident_bundle.identity.session_authority_id ||
      batch.resident_locator_instance_id !=
          resident_bundle.identity.locator_instance_id ||
      batch.canonical_cloud_digest != authority.canonical_cloud_digest ||
      batch.point_count != authority.point_count ||
      batch.source_batch_index != resident_bundle.source_batch_index ||
      batch.closed_squared_level != resident_bundle.squared_level ||
      !batch.batch_receipt_freshly_verified ||
      !batch.exact_closed_cut_certified ||
      !batch.all_intermediate_k1_levels_accounted ||
      !batch.bindings_complete_for_requested_singletons ||
      !batch.roots_closed_at_exact_level ||
      batch.global_membership_star_gamma_or_delaunay_materialized ||
      batch.public_status_claimed ||
      batch.bindings.size() >
          budget.maximum_external_seam_binding_scan_count) {
    return false;
  }
  spatial::PointId previous{};
  bool first = true;
  for (std::size_t index = 0U; index < batch.bindings.size(); ++index) {
    if (!increment_bounded(
            binding_scans,
            1U,
            budget.maximum_external_seam_binding_scan_count)) {
      return false;
    }
    const auto& binding = batch.bindings[index];
    if (binding.binding_index != index ||
        binding.closed_k1_root_id == 0U ||
        static_cast<std::size_t>(binding.point_id) >= batch.point_count ||
        (!first && binding.point_id <= previous)) {
      return false;
    }
    previous = binding.point_id;
    first = false;
  }
  return true;
}

[[nodiscard]] bool root_coverages_are_dense(
    const ExactDirectMorseUnifiedResidentSession& resident) noexcept {
  const auto& roots = resident.root_coverages();
  if (roots.size() >
      std::numeric_limits<ExactFrozenIncidencePriorRootId>::max()) {
    return false;
  }
  for (std::size_t index = 0U; index < roots.size(); ++index) {
    if (roots[index].root_id !=
        static_cast<ExactFrozenIncidencePriorRootId>(index + 1U)) {
      return false;
    }
  }
  return true;
}

}  // namespace

struct ExactDirectMorseUnifiedResidentPreparedVerticalBatch::Impl {
  ~Impl() noexcept {
    if (!owns_ticket_slot) {
      return;
    }
    if (ticket_registry == nullptr ||
        ticket_registry->live_ticket_count == 0U) {
      std::terminate();
    }
    --ticket_registry->live_ticket_count;
  }

  const ExactDirectMorseUnifiedResidentAuthorityBundle* source_bundle{};
  std::shared_ptr<const VerticalBridgeSeal> seal;
  std::shared_ptr<VerticalOutstandingTicketRegistry> ticket_registry;
  ExactDirectMorseUnifiedSnapshotIdentity source_identity{};
  std::size_t source_cursor{};
  std::size_t source_epoch{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp source_stamp{};
  std::size_t roots_before{};
  std::size_t groups_before{};
  std::size_t new_root_count{};
  std::uint64_t final_query_token{};
  std::vector<VerticalCarrierPatch> carrier_patches;
  std::vector<VerticalExpectedGroupRecord> expected_groups;
  ExactDirectMorseUnifiedResidentVerticalBatchReceipt receipt{};
  bool owns_ticket_slot{false};
  bool consumed{false};
};

struct ExactDirectMorseUnifiedResidentVerticalStreamBridge::Impl {
  ExactDirectMorseUnifiedResidentVerticalStreamBridgeBudget budget{};
  ExactDirectMorseUnifiedResidentVerticalStreamBridgeConfig config{};
  contract::CanonicalId source_pair_canonical_cloud_digest{};
  contract::CanonicalId source_higher_canonical_cloud_digest{};
  contract::CanonicalId source_pair_semantic_digest{};
  contract::CanonicalId source_higher_semantic_digest{};
  std::size_t point_count{};
  std::uint64_t bound_resident_locator_instance_id{};
  std::size_t cursor{};
  std::size_t epoch{};
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp expected_stamp{};
  std::uint64_t next_query_token{};
  std::vector<ExactDirectMorseUnifiedResidentVerticalRootCarrier> carriers;
  std::shared_ptr<const VerticalBridgeSeal> seal;
  std::shared_ptr<VerticalOutstandingTicketRegistry> ticket_registry;
  bool initialized{false};
};

bool ExactDirectMorseUnifiedResidentExternalK2K1SeamAuthority::
    certified_external_seam() const noexcept {
  return schema_version ==
             direct_morse_unified_resident_vertical_stream_bridge_schema_version &&
         external_seam_authority_id != 0U &&
         canonical_cloud_digest != contract::CanonicalId{} &&
         point_count != 0U && authority_freshly_verified &&
         exact_k1_closed_cuts_certified &&
         every_requested_singleton_has_one_closed_k1_root &&
         horizontal_k1_successors_certified &&
         !global_membership_star_gamma_or_delaunay_materialized &&
         !public_status_claimed;
}

bool ExactDirectMorseUnifiedResidentVerticalBatchReceipt::
    certified_relative_streamed_batch() const noexcept {
  if (schema_version !=
          direct_morse_unified_resident_vertical_stream_bridge_schema_version ||
      bridge_authority_id == 0U ||
      source_prebatch_identity.session_authority_id == 0U ||
      source_prebatch_identity.locator_instance_id == 0U ||
      source_prebatch_identity.epoch != source_batch_index ||
      source_prebatch_identity.batch_cursor != source_batch_index ||
      source_postbatch_stamp.external_authority_id !=
          source_prebatch_identity.session_authority_id ||
      source_postbatch_stamp.committed_batch_count != source_batch_index + 1U ||
      source_order < 2U || target_order + 1U != source_order ||
      canonical_cloud_digest == contract::CanonicalId{} ||
      counters.group_count != group_images.size() ||
      counters.incidence_union_witness_count !=
          incidence_union_witnesses.size() ||
      resident_lower_order_locator_replayed == external_k2_k1_seam_replayed ||
      resident_lower_order_locator_replayed != (source_order >= 3U) ||
      external_k2_k1_seam_replayed != (source_order == 2U) ||
      !resident_prebatch_bundle_certified ||
      !merged_exact_level_order_prefix_certified ||
      !target_closed_cut_precedes_source_batch ||
      !every_source_coface_replayed_as_incidence_star ||
      !every_incidence_union_has_exact_common_target_facet ||
      !one_closed_target_root_per_source_group ||
      !continuations_propagated_from_prior_carrier ||
      !multifusions_propagated_from_all_prior_carriers ||
      !one_carrier_published_per_resultant_source_root ||
      !resident_batch_committed_before_vertical_publication ||
      !publication_suffix_noexcept ||
      !conditional_on_resident_source_and_external_k1_seam ||
      incidence_complete_reduction_claimed ||
      all_global_naturality_squares_claimed ||
      vertical_maps_complete_claimed ||
      global_membership_star_gamma_or_delaunay_materialized ||
      public_status_claimed ||
      scope != ExactDirectMorseUnifiedResidentVerticalBatchScope::
                   one_verified_resident_prebatch_relative_adjacent_order_vertical_step_only) {
    return false;
  }
  for (std::size_t index = 0U; index < group_images.size(); ++index) {
    const auto& group = group_images[index];
    if (group.group_image_index != index ||
        group.source_group_index != index ||
        group.resultant_source_root_id == 0U ||
        group.closed_target_root_id == 0U ||
        group.prior_source_root_offset > prior_source_root_ids.size() ||
        group.prior_source_root_count >
            prior_source_root_ids.size() - group.prior_source_root_offset ||
        group.incidence_union_witness_offset >
            incidence_union_witnesses.size() ||
        group.incidence_union_witness_count >
            incidence_union_witnesses.size() -
                group.incidence_union_witness_offset ||
        group.incidence_union_witness_count == 0U ||
        !group.all_prior_carriers_reverified ||
        !group.all_incidence_unions_have_same_target_root) {
      return false;
    }
  }
  for (std::size_t index = 0U;
       index < incidence_union_witnesses.size();
       ++index) {
    const auto& witness = incidence_union_witnesses[index];
    if (witness.witness_index != index ||
        witness.owner_group_index >= group_images.size() ||
        witness.closed_target_root_id == 0U ||
        witness.common_target_facet_key.point_count != target_order ||
        !canonical_key_shape(witness.common_target_facet_key) ||
        witness.target_binding_witness.external_authority_id == 0U ||
        witness.target_binding_witness.replay_token == 0U ||
        (source_order == 2U) !=
            (witness.target_kind ==
             ExactDirectMorseUnifiedResidentVerticalTargetKind::
                 external_k1_root)) {
      return false;
    }
  }
  return true;
}

ExactDirectMorseUnifiedResidentPreparedVerticalBatch::
    ExactDirectMorseUnifiedResidentPreparedVerticalBatch() noexcept = default;
ExactDirectMorseUnifiedResidentPreparedVerticalBatch::
    ~ExactDirectMorseUnifiedResidentPreparedVerticalBatch() = default;
ExactDirectMorseUnifiedResidentPreparedVerticalBatch::
    ExactDirectMorseUnifiedResidentPreparedVerticalBatch(
        ExactDirectMorseUnifiedResidentPreparedVerticalBatch&&) noexcept =
    default;
ExactDirectMorseUnifiedResidentPreparedVerticalBatch&
ExactDirectMorseUnifiedResidentPreparedVerticalBatch::operator=(
    ExactDirectMorseUnifiedResidentPreparedVerticalBatch&&) noexcept = default;
ExactDirectMorseUnifiedResidentPreparedVerticalBatch::
    ExactDirectMorseUnifiedResidentPreparedVerticalBatch(
        std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectMorseUnifiedResidentPreparedVerticalBatch::valid()
    const noexcept {
  return impl_ != nullptr && impl_->source_bundle != nullptr &&
         impl_->seal != nullptr && impl_->ticket_registry != nullptr &&
         impl_->owns_ticket_slot && !impl_->consumed &&
         impl_->ticket_registry->live_ticket_count == 1U &&
         impl_->source_cursor == impl_->source_identity.batch_cursor &&
         impl_->source_epoch == impl_->source_identity.epoch &&
         impl_->source_stamp == impl_->source_identity.locator_stamp &&
         impl_->source_bundle->identity == impl_->source_identity &&
         impl_->receipt.source_prebatch_identity == impl_->source_identity;
}

bool ExactDirectMorseUnifiedResidentPreparedVerticalBatch::consumed()
    const noexcept {
  return impl_ == nullptr || impl_->consumed;
}

bool ExactDirectMorseUnifiedResidentVerticalPreparationResult::
    certified_prepared_vertical_batch() const noexcept {
  return ticket.has_value() && ticket->valid() &&
         bridge_and_resident_state_unmodified &&
         decision == ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                         complete_certified_prepared_relative_vertical_batch;
}

bool ExactDirectMorseUnifiedResidentVerticalCommitResult::
    certified_atomic_vertical_commit() const noexcept {
  return resident_commit.certified_committed_batch() && receipt.has_value() &&
         receipt->certified_relative_streamed_batch() &&
         prepared_vertical_ticket_consumed && vertical_state_mutated &&
         !no_vertical_state_mutated_on_resident_failure &&
         resident_committed_before_vertical_publication &&
         postcommit_suffix_noexcept_or_fail_stop &&
         decision == ExactDirectMorseUnifiedResidentVerticalCommitDecision::
                         complete_certified_resident_and_vertical_atomic_commit;
}

ExactDirectMorseUnifiedResidentVerticalStreamBridge::
    ExactDirectMorseUnifiedResidentVerticalStreamBridge() noexcept = default;
ExactDirectMorseUnifiedResidentVerticalStreamBridge::
    ~ExactDirectMorseUnifiedResidentVerticalStreamBridge() = default;
ExactDirectMorseUnifiedResidentVerticalStreamBridge::
    ExactDirectMorseUnifiedResidentVerticalStreamBridge(
        ExactDirectMorseUnifiedResidentVerticalStreamBridge&&) noexcept =
    default;
ExactDirectMorseUnifiedResidentVerticalStreamBridge&
ExactDirectMorseUnifiedResidentVerticalStreamBridge::operator=(
    ExactDirectMorseUnifiedResidentVerticalStreamBridge&&) noexcept = default;
ExactDirectMorseUnifiedResidentVerticalStreamBridge::
    ExactDirectMorseUnifiedResidentVerticalStreamBridge(
        std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectMorseUnifiedResidentVerticalStreamBridge::
    certified_relative_bridge() const noexcept {
  if (impl_ == nullptr || !impl_->initialized || impl_->seal == nullptr ||
      impl_->ticket_registry == nullptr ||
      impl_->ticket_registry->live_ticket_count > 1U ||
      impl_->config.bridge_authority_id == 0U ||
      impl_->config.first_query_replay_token == 0U ||
      !impl_->config.query_replay_token_suffix_reserved_by_resident_authority ||
      !impl_->config.external_k2_k1.certified_external_seam() ||
      impl_->point_count == 0U || impl_->cursor != impl_->epoch ||
      impl_->cursor > impl_->budget.maximum_resident_batch_count ||
      impl_->carriers.size() >
          impl_->budget.maximum_persistent_source_root_carrier_count ||
      impl_->carriers.capacity() <
          impl_->budget.maximum_persistent_source_root_carrier_count ||
      impl_->expected_stamp.external_authority_id !=
          impl_->seal->resident_session_authority_id ||
      (impl_->bound_resident_locator_instance_id == 0U) !=
          (impl_->cursor == 0U) ||
      impl_->expected_stamp.committed_batch_count != impl_->cursor ||
      impl_->next_query_token == 0U) {
    return false;
  }
  for (std::size_t index = 0U; index < impl_->carriers.size(); ++index) {
    const auto& carrier = impl_->carriers[index];
    if (carrier.source_root_id !=
            static_cast<ExactFrozenIncidencePriorRootId>(index + 1U) ||
        carrier.source_order < 2U ||
        carrier.target_order + 1U != carrier.source_order ||
        carrier.closed_target_root_id == 0U ||
        carrier.target_facet_key.point_count != carrier.target_order ||
        !canonical_key_shape(carrier.target_facet_key) ||
        carrier.target_binding_witness.external_authority_id == 0U ||
        carrier.target_binding_witness.replay_token == 0U ||
        carrier.created_source_batch_index >
            carrier.last_verified_source_batch_index ||
        (carrier.source_order == 2U) !=
            (carrier.target_kind ==
             ExactDirectMorseUnifiedResidentVerticalTargetKind::
                 external_k1_root)) {
      return false;
    }
  }
  return true;
}

std::size_t ExactDirectMorseUnifiedResidentVerticalStreamBridge::
    resident_batch_cursor() const noexcept {
  return impl_ == nullptr ? 0U : impl_->cursor;
}

std::size_t ExactDirectMorseUnifiedResidentVerticalStreamBridge::
    resident_epoch() const noexcept {
  return impl_ == nullptr ? 0U : impl_->epoch;
}

const std::vector<ExactDirectMorseUnifiedResidentVerticalRootCarrier>&
ExactDirectMorseUnifiedResidentVerticalStreamBridge::root_carriers()
    const noexcept {
  static const std::vector<
      ExactDirectMorseUnifiedResidentVerticalRootCarrier>
      empty;
  return impl_ == nullptr ? empty : impl_->carriers;
}

ExactDirectMorseUnifiedResidentVerticalPreparationResult
ExactDirectMorseUnifiedResidentVerticalStreamBridge::prepare_next(
    const ExactDirectMorseUnifiedResidentSession& resident,
    const ExactDirectMorseUnifiedResidentPreparedBatch& resident_ticket,
    const ExactDirectMorseUnifiedResidentExternalK2K1BatchReceipt*
        external_k2_k1_batch) {
  ExactDirectMorseUnifiedResidentVerticalPreparationResult output;
  output.bridge_and_resident_state_unmodified = true;
  const auto reject = [&](ExactDirectMorseUnifiedResidentVerticalPreparationDecision
                              decision) {
    output.ticket.reset();
    output.decision = decision;
    return std::move(output);
  };
  if (!certified_relative_bridge()) {
    return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                      no_bridge_not_ready);
  }
  if (impl_->ticket_registry->live_ticket_count != 0U) {
    return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                      no_bridge_outstanding_ticket);
  }
  if (!resident.certified_resident_session() || !resident_ticket.valid()) {
    return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                      no_bridge_resident_or_ticket_rejected);
  }
  const auto& bundle = resident_ticket.authority_bundle();
  if (!bundle.certified_strict_pre_batch_bundle() ||
      resident.batch_cursor() != impl_->cursor ||
      resident.epoch() != impl_->epoch ||
      resident.locator().snapshot_stamp() != impl_->expected_stamp ||
      bundle.identity.batch_cursor != impl_->cursor ||
      bundle.identity.epoch != impl_->epoch ||
      bundle.identity.locator_stamp != impl_->expected_stamp ||
      bundle.identity.session_authority_id !=
          impl_->seal->resident_session_authority_id ||
      (impl_->bound_resident_locator_instance_id != 0U &&
       bundle.identity.locator_instance_id !=
           impl_->bound_resident_locator_instance_id) ||
      bundle.identity.source_pair_canonical_cloud_digest !=
          impl_->source_pair_canonical_cloud_digest ||
      bundle.identity.source_higher_canonical_cloud_digest !=
          impl_->source_higher_canonical_cloud_digest ||
      bundle.identity.source_pair_semantic_digest !=
          impl_->source_pair_semantic_digest ||
      bundle.identity.source_higher_semantic_digest !=
          impl_->source_higher_semantic_digest ||
      resident.locator().config().external_authority_id !=
          bundle.identity.session_authority_id ||
      resident.root_coverages().size() != impl_->carriers.size() ||
      !root_coverages_are_dense(resident)) {
    return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                      no_bridge_stale_resident_prefix);
  }
  const auto& plan = resident.plan();
  if (impl_->cursor >= plan.batches.size() ||
      plan.point_count != impl_->point_count ||
      plan.batches[impl_->cursor].batch_index != impl_->cursor ||
      plan.batches[impl_->cursor].future_snapshot_index !=
          bundle.source_future_snapshot_index ||
      plan.batches[impl_->cursor].squared_level != bundle.squared_level ||
      plan.batches[impl_->cursor].order != bundle.order ||
      bundle.source_batch_index != impl_->cursor || bundle.order < 2U ||
      bundle.order > direct_sparse_positive_facet_maximum_point_count) {
    return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                      no_bridge_batch_order_rejected);
  }

  try {
    auto prepared = std::make_unique<
        ExactDirectMorseUnifiedResidentPreparedVerticalBatch::Impl>();
    prepared->source_bundle = &bundle;
    prepared->seal = impl_->seal;
    prepared->source_identity = bundle.identity;
    prepared->source_cursor = impl_->cursor;
    prepared->source_epoch = impl_->epoch;
    prepared->source_stamp = impl_->expected_stamp;
    prepared->roots_before = resident.root_coverages().size();
    prepared->groups_before = resident.group_records().size();
    prepared->final_query_token = impl_->next_query_token;

    auto& receipt = prepared->receipt;
    receipt.bridge_authority_id = impl_->config.bridge_authority_id;
    receipt.source_prebatch_identity = bundle.identity;
    receipt.canonical_cloud_digest =
        impl_->source_pair_canonical_cloud_digest;
    receipt.source_batch_index = bundle.source_batch_index;
    receipt.closed_squared_level = bundle.squared_level;
    receipt.source_order = bundle.order;
    receipt.target_order = bundle.order - 1U;
    receipt.resident_prebatch_bundle_certified = true;
    receipt.conditional_on_resident_source_and_external_k1_seam = true;
    receipt.scope = ExactDirectMorseUnifiedResidentVerticalBatchScope::
        one_verified_resident_prebatch_relative_adjacent_order_vertical_step_only;

    for (std::size_t future = impl_->cursor + 1U;
         future < plan.batches.size();
         ++future) {
      const auto& candidate = plan.batches[future];
      if (candidate.order != receipt.target_order) {
        continue;
      }
      if (!increment_bounded(
              receipt.counters.exact_level_comparison_count,
              1U,
              impl_->budget.maximum_exact_level_comparison_count)) {
        return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                          no_bridge_budget_exhausted);
      }
      if (candidate.squared_level <= bundle.squared_level) {
        return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                          no_bridge_target_cut_not_closed);
      }
    }
    receipt.merged_exact_level_order_prefix_certified = true;
    receipt.target_closed_cut_precedes_source_batch = true;

    if (bundle.order == 2U) {
      if (external_k2_k1_batch == nullptr ||
          impl_->config.external_k2_k1.canonical_cloud_digest !=
              impl_->source_pair_canonical_cloud_digest ||
          impl_->config.external_k2_k1.point_count != impl_->point_count ||
          !external_batch_shape_certified(
              *external_k2_k1_batch,
              impl_->config.external_k2_k1,
              bundle,
              impl_->budget,
              receipt.counters.external_seam_binding_scan_count)) {
        return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                          no_bridge_external_seam_rejected);
      }
      receipt.external_k2_k1_seam_replayed = true;
    } else {
      if (external_k2_k1_batch != nullptr) {
        return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                          no_bridge_external_seam_rejected);
      }
      receipt.resident_lower_order_locator_replayed = true;
    }

    const auto& frozen = bundle.frozen_batch;
    const auto& quotient = frozen.quotient;
    const auto& actions = frozen.action_plan;
    const std::size_t hyperedge_count = frozen.quotient_hyperedges.size();
    const std::size_t token_count = frozen.quotient_token_references.size();
    const std::size_t group_count = quotient.groups.size();
    if (hyperedge_count > impl_->budget.maximum_batch_hyperedge_count ||
        token_count > impl_->budget.maximum_batch_token_reference_count ||
        group_count > impl_->budget.maximum_batch_group_count ||
        hyperedge_count != quotient.hyperedge_bindings.size() ||
        token_count != frozen.incidence_facet_token_indices.size() ||
        actions.groups.size() != group_count ||
        frozen.counters.hyperedge_count != hyperedge_count ||
        frozen.counters.token_reference_count != token_count ||
        frozen.counters.group_count != group_count) {
      return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                        no_bridge_source_incidence_rejected);
    }

    std::size_t union_count = 0U;
    for (std::size_t hyperedge_index = 0U;
         hyperedge_index < hyperedge_count;
         ++hyperedge_index) {
      const auto& hyperedge = frozen.quotient_hyperedges[hyperedge_index];
      const auto& binding = quotient.hyperedge_bindings[hyperedge_index];
      if (hyperedge.hyperedge_index != hyperedge_index ||
          binding.source_hyperedge_index != hyperedge_index ||
          binding.source_token_reference_offset !=
              hyperedge.token_reference_offset ||
          binding.source_token_reference_count !=
              hyperedge.token_reference_count ||
          binding.group_index >= group_count ||
          hyperedge.token_reference_offset > token_count ||
          hyperedge.token_reference_count >
              token_count - hyperedge.token_reference_offset ||
          hyperedge.token_reference_count != bundle.order + 1U ||
          add_overflow(
              union_count,
              hyperedge.token_reference_count - 1U,
              union_count)) {
        return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                          no_bridge_source_incidence_rejected);
      }
    }
    const std::size_t prior_count = actions.prior_root_ids.size();
    std::size_t output_count = 0U;
    std::size_t scratch_count = 0U;
    std::size_t maximum_resolutions = 0U;
    if (union_count >
            impl_->budget.maximum_batch_incidence_union_witness_count ||
        prior_count >
            impl_->budget.maximum_batch_prior_carrier_reprobe_count ||
        add_overflow(group_count, union_count, output_count) ||
        add_overflow(output_count, prior_count, output_count) ||
        output_count >
            impl_->budget.maximum_logical_batch_output_entry_count ||
        add_overflow(hyperedge_count, group_count, scratch_count) ||
        scratch_count > impl_->budget.maximum_scratch_entry_count ||
        add_overflow(union_count, prior_count, maximum_resolutions) ||
        (bundle.order >= 3U &&
         maximum_resolutions >
             impl_->budget.maximum_batch_target_probe_count)) {
      return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                        no_bridge_budget_exhausted);
    }
    std::size_t carrier_count_after = 0U;
    if (add_overflow(
            impl_->carriers.size(), group_count, carrier_count_after) ||
        carrier_count_after >
            impl_->budget.maximum_persistent_source_root_carrier_count) {
      return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                        no_bridge_capacity_overflow);
    }

    receipt.counters.group_count = group_count;
    receipt.counters.hyperedge_count = hyperedge_count;
    receipt.counters.token_reference_count = token_count;
    receipt.counters.incidence_union_witness_count = union_count;
    receipt.counters.logical_scratch_entry_count = scratch_count;
    receipt.counters.logical_output_entry_count = output_count;
    prepared->carrier_patches.reserve(group_count);
    prepared->expected_groups.reserve(group_count);
    receipt.group_images.reserve(group_count);
    receipt.prior_source_root_ids.reserve(prior_count);
    receipt.incidence_union_witnesses.reserve(union_count);
    std::vector<std::size_t> hyperedge_order(hyperedge_count);
    std::iota(hyperedge_order.begin(), hyperedge_order.end(), 0U);
    std::sort(
        hyperedge_order.begin(),
        hyperedge_order.end(),
        [&](std::size_t left, std::size_t right) {
          const std::size_t left_group =
              quotient.hyperedge_bindings[left].group_index;
          const std::size_t right_group =
              quotient.hyperedge_bindings[right].group_index;
          return left_group < right_group ||
                 (left_group == right_group && left < right);
        });

    TargetResolutionContext target_context{
        resident,
        external_k2_k1_batch,
        impl_->budget,
        bundle.identity.session_authority_id,
        impl_->next_query_token,
        bundle.order,
        receipt.counters};
    std::size_t hyperedge_cursor = 0U;
    std::size_t append_count = 0U;
    std::size_t expected_prior_offset = 0U;
    for (std::size_t group_index = 0U;
         group_index < group_count;
         ++group_index) {
      const auto& quotient_group = quotient.groups[group_index];
      const auto& action_group = actions.groups[group_index];
      if (quotient_group.group_index != group_index ||
          action_group.group_index != group_index ||
          action_group.prior_root_offset != expected_prior_offset ||
          action_group.prior_root_count != action_group.q_r ||
          action_group.prior_root_offset > prior_count ||
          action_group.prior_root_count >
              prior_count - action_group.prior_root_offset ||
          quotient_group.rooted_carrier_count != action_group.q_r ||
          !action_matches_qr(action_group.q_r, action_group.action) ||
          !disposition_matches_qr(
              action_group.q_r, quotient_group.disposition)) {
        return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                          no_bridge_source_incidence_rejected);
      }
      expected_prior_offset += action_group.prior_root_count;

      ExactDirectMorseUnifiedResidentVerticalGroupImage image;
      image.group_image_index = group_index;
      image.source_group_index = group_index;
      image.prior_source_root_offset =
          receipt.prior_source_root_ids.size();
      image.prior_source_root_count = action_group.prior_root_count;
      image.incidence_union_witness_offset =
          receipt.incidence_union_witnesses.size();
      image.source_action = action_group.action;

      bool target_present = false;
      TargetResolution group_target;
      bool carrier_candidate_present = false;
      ExactDirectSparseFacetKey carrier_key;
      ExactDirectSparseFacetWitness carrier_witness;
      ExactDirectMorseUnifiedResidentVerticalTargetKind carrier_kind{
          ExactDirectMorseUnifiedResidentVerticalTargetKind::
              resident_adjacent_lower_order_root};
      std::size_t observed_group_hyperedges = 0U;
      while (hyperedge_cursor < hyperedge_order.size() &&
             quotient.hyperedge_bindings
                     [hyperedge_order[hyperedge_cursor]]
                         .group_index == group_index) {
        const std::size_t source_hyperedge_index =
            hyperedge_order[hyperedge_cursor];
        ++hyperedge_cursor;
        ++observed_group_hyperedges;
        const auto& hyperedge =
            frozen.quotient_hyperedges[source_hyperedge_index];
        const std::size_t anchor_reference =
            hyperedge.token_reference_offset;
        const std::size_t anchor_token_index =
            frozen.incidence_facet_token_indices[anchor_reference];
        if (anchor_token_index >= plan.facet_tokens.size() ||
            plan.facet_tokens[anchor_token_index].facet_token_index !=
                anchor_token_index) {
          return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                            no_bridge_source_incidence_rejected);
        }
        const auto& anchor_key =
            plan.facet_tokens[anchor_token_index].facet_key;
        for (std::size_t local = 1U;
             local < hyperedge.token_reference_count;
             ++local) {
          const std::size_t right_reference =
              hyperedge.token_reference_offset + local;
          const std::size_t right_token_index =
              frozen.incidence_facet_token_indices[right_reference];
          if (right_token_index >= plan.facet_tokens.size() ||
              plan.facet_tokens[right_token_index].facet_token_index !=
                  right_token_index) {
            return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                              no_bridge_source_incidence_rejected);
          }
          ExactDirectSparseFacetKey common_key;
          if (!exact_common_codimension_one_facet(
                  anchor_key,
                  plan.facet_tokens[right_token_index].facet_key,
                  bundle.order,
                  impl_->budget.maximum_key_point_scan_count,
                  receipt.counters.key_point_scan_count,
                  common_key)) {
            const bool budget_exhausted =
                receipt.counters.key_point_scan_count >=
                impl_->budget.maximum_key_point_scan_count;
            return reject(
                budget_exhausted
                    ? ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                          no_bridge_budget_exhausted
                    : ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                          no_bridge_common_target_facet_rejected);
          }
          TargetResolution target;
          if (!resolve_target(target_context, common_key, target)) {
            return reject(target_context.failure);
          }
          if (target_present && target.root_id != group_target.root_id) {
            return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                              no_bridge_target_root_conflict);
          }
          if (!target_present) {
            group_target = target;
            target_present = true;
          }
          if (!carrier_candidate_present || key_less(common_key, carrier_key)) {
            carrier_candidate_present = true;
            carrier_key = common_key;
            carrier_witness = target.binding_witness;
            carrier_kind = target.kind;
          }
          receipt.incidence_union_witnesses.push_back(
              {receipt.incidence_union_witnesses.size(),
               group_index,
               source_hyperedge_index,
               anchor_token_index,
               right_token_index,
               common_key,
               target.root_id,
               target.binding_witness,
               target.kind});
        }
      }
      if (observed_group_hyperedges != quotient_group.hyperedge_count ||
          observed_group_hyperedges == 0U || !target_present ||
          !carrier_candidate_present) {
        return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                          no_bridge_source_incidence_rejected);
      }

      for (std::size_t local = 0U;
           local < action_group.prior_root_count;
           ++local) {
        const auto prior_root_id =
            actions.prior_root_ids[action_group.prior_root_offset + local];
        if (prior_root_id == 0U ||
            prior_root_id > impl_->carriers.size()) {
          return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                            no_bridge_prior_carrier_missing);
        }
        const auto& carrier = impl_->carriers[
            static_cast<std::size_t>(prior_root_id - 1U)];
        if (carrier.source_root_id != prior_root_id ||
            carrier.source_order != bundle.order ||
            carrier.target_order != bundle.order - 1U) {
          return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                            no_bridge_prior_carrier_missing);
        }
        TargetResolution prior_target;
        if (!resolve_target(
                target_context, carrier.target_facet_key, prior_target)) {
          return reject(target_context.failure);
        }
        if (prior_target.root_id != group_target.root_id) {
          return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                            no_bridge_target_root_conflict);
        }
        receipt.prior_source_root_ids.push_back(prior_root_id);
        if (!increment_bounded(
                receipt.counters.prior_carrier_reprobe_count,
                1U,
                impl_->budget.maximum_batch_prior_carrier_reprobe_count)) {
          return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                            no_bridge_budget_exhausted);
        }
      }

      ExactFrozenIncidencePriorRootId resultant_root_id{};
      bool append = action_group.q_r != 1U;
      std::size_t carrier_index = 0U;
      if (!append) {
        resultant_root_id =
            actions.prior_root_ids[action_group.prior_root_offset];
        carrier_index = static_cast<std::size_t>(resultant_root_id - 1U);
      } else {
        std::size_t root_number = 0U;
        if (add_overflow(
                prepared->roots_before,
                append_count + 1U,
                root_number) ||
            root_number >
                std::numeric_limits<ExactFrozenIncidencePriorRootId>::max()) {
          return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                            no_bridge_capacity_overflow);
        }
        resultant_root_id =
            static_cast<ExactFrozenIncidencePriorRootId>(root_number);
        carrier_index = impl_->carriers.size() + append_count;
        ++append_count;
      }
      image.resultant_source_root_id = resultant_root_id;
      image.closed_target_root_id = group_target.root_id;
      image.incidence_union_witness_count =
          receipt.incidence_union_witnesses.size() -
          image.incidence_union_witness_offset;
      image.all_prior_carriers_reverified = true;
      image.all_incidence_unions_have_same_target_root = true;
      receipt.group_images.push_back(image);

      std::size_t created_batch = bundle.source_batch_index;
      if (!append) {
        created_batch = impl_->carriers[carrier_index]
                            .created_source_batch_index;
      }
      prepared->carrier_patches.push_back(
          {carrier_index,
           append,
           {resultant_root_id,
            bundle.order,
            bundle.order - 1U,
            group_target.root_id,
            carrier_key,
            carrier_witness,
            created_batch,
            bundle.source_batch_index,
            carrier_kind}});
      prepared->expected_groups.push_back(
          {prepared->groups_before + group_index,
           group_index,
           resultant_root_id,
           action_group.prior_root_offset,
           action_group.prior_root_count,
           action_group.q_r,
           action_group.action});
    }
    if (hyperedge_cursor != hyperedge_order.size() ||
        expected_prior_offset != prior_count ||
        receipt.incidence_union_witnesses.size() != union_count ||
        receipt.prior_source_root_ids.size() != prior_count ||
        prepared->carrier_patches.size() != group_count ||
        prepared->expected_groups.size() != group_count) {
      return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                        no_bridge_source_incidence_rejected);
    }
    prepared->new_root_count = append_count;
    prepared->final_query_token = target_context.next_query_token;
    receipt.every_source_coface_replayed_as_incidence_star = true;
    receipt.every_incidence_union_has_exact_common_target_facet = true;
    receipt.one_closed_target_root_per_source_group = true;
    receipt.continuations_propagated_from_prior_carrier = true;
    receipt.multifusions_propagated_from_all_prior_carriers = true;

    prepared->ticket_registry = impl_->ticket_registry;
    ++prepared->ticket_registry->live_ticket_count;
    prepared->owns_ticket_slot = true;
    output.ticket.emplace(
        ExactDirectMorseUnifiedResidentPreparedVerticalBatch{
            std::move(prepared)});
    output.decision =
        ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
            complete_certified_prepared_relative_vertical_batch;
    return output;
  } catch (const std::bad_alloc&) {
    return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                      no_bridge_allocation_failed);
  } catch (const std::length_error&) {
    return reject(ExactDirectMorseUnifiedResidentVerticalPreparationDecision::
                      no_bridge_capacity_overflow);
  }
}

ExactDirectMorseUnifiedResidentVerticalCommitResult
ExactDirectMorseUnifiedResidentVerticalStreamBridge::commit(
    ExactDirectMorseUnifiedResidentSession& resident,
    ExactDirectMorseUnifiedResidentPreparedBatch&& resident_ticket,
    ExactDirectMorseUnifiedResidentPreparedVerticalBatch&& vertical_ticket)
    noexcept {
  static_assert(std::is_nothrow_move_constructible_v<
                ExactDirectMorseUnifiedResidentVerticalBatchReceipt>);
  static_assert(std::is_nothrow_move_assignable_v<
                ExactDirectMorseUnifiedResidentVerticalRootCarrier>);
  ExactDirectMorseUnifiedResidentVerticalCommitResult output;
  auto prepared = std::move(vertical_ticket.impl_);
  output.prepared_vertical_ticket_consumed = true;
  if (prepared == nullptr || prepared->consumed) {
    output.decision = ExactDirectMorseUnifiedResidentVerticalCommitDecision::
        no_bridge_prepared_ticket_rejected;
    return output;
  }
  prepared->consumed = true;
  if (impl_ == nullptr || prepared->seal.get() != impl_->seal.get() ||
      prepared->source_bundle != &resident_ticket.authority_bundle() ||
      !resident_ticket.valid() ||
      prepared->source_cursor != impl_->cursor ||
      prepared->source_epoch != impl_->epoch ||
      prepared->source_stamp != impl_->expected_stamp ||
      resident.batch_cursor() != impl_->cursor ||
      resident.epoch() != impl_->epoch ||
      resident.locator().snapshot_stamp() != impl_->expected_stamp ||
      prepared->carrier_patches.size() !=
          prepared->receipt.group_images.size() ||
      prepared->expected_groups.size() !=
          prepared->receipt.group_images.size()) {
    output.decision = ExactDirectMorseUnifiedResidentVerticalCommitDecision::
        no_bridge_prepared_ticket_rejected;
    return output;
  }

  output.resident_commit = resident.commit(std::move(resident_ticket));
  if (!output.resident_commit.certified_committed_batch()) {
    output.no_vertical_state_mutated_on_resident_failure = true;
    output.decision = ExactDirectMorseUnifiedResidentVerticalCommitDecision::
        no_bridge_resident_commit_rejected_without_vertical_mutation;
    return output;
  }
  output.resident_committed_before_vertical_publication = true;

  const auto committed_stamp = resident.locator().snapshot_stamp();
  const auto& records = resident.group_records();
  const auto& roots = resident.root_coverages();
  if (resident.batch_cursor() != prepared->source_cursor + 1U ||
      resident.epoch() != prepared->source_epoch + 1U ||
      committed_stamp.external_authority_id !=
          prepared->source_identity.session_authority_id ||
      committed_stamp.committed_batch_count != prepared->source_cursor + 1U ||
      records.size() !=
          prepared->groups_before + prepared->expected_groups.size() ||
      roots.size() != prepared->roots_before + prepared->new_root_count ||
      !root_coverages_are_dense(resident) ||
      impl_->carriers.capacity() <
          impl_->budget.maximum_persistent_source_root_carrier_count) {
    std::terminate();
  }
  for (std::size_t local = 0U;
       local < prepared->expected_groups.size();
       ++local) {
    const auto& expected = prepared->expected_groups[local];
    const auto& record = records[prepared->groups_before + local];
    if (record.group_record_index != expected.group_record_index ||
        record.batch_index != prepared->source_cursor ||
        record.owner_group_index != expected.owner_group_index ||
        record.squared_level != prepared->receipt.closed_squared_level ||
        record.order != prepared->receipt.source_order ||
        record.q_r != expected.q_r || record.action != expected.action ||
        record.resultant_root_id != expected.resultant_root_id ||
        expected.prior_root_offset >
            prepared->receipt.prior_source_root_ids.size() ||
        expected.prior_root_count >
            prepared->receipt.prior_source_root_ids.size() -
                expected.prior_root_offset ||
        record.child_root_ids.size() != expected.prior_root_count ||
        !std::equal(
            record.child_root_ids.begin(),
            record.child_root_ids.end(),
            prepared->receipt.prior_source_root_ids.begin() +
                static_cast<std::ptrdiff_t>(expected.prior_root_offset))) {
      std::terminate();
    }
  }

  std::size_t published_append_count = 0U;
  for (auto& patch : prepared->carrier_patches) {
    if (patch.append) {
      if (patch.carrier_index != impl_->carriers.size() ||
          impl_->carriers.size() == impl_->carriers.capacity()) {
        std::terminate();
      }
      impl_->carriers.push_back(std::move(patch.next));
      ++published_append_count;
    } else {
      if (patch.carrier_index >= impl_->carriers.size()) {
        std::terminate();
      }
      impl_->carriers[patch.carrier_index] = std::move(patch.next);
    }
  }
  if (published_append_count != prepared->new_root_count ||
      impl_->carriers.size() != roots.size()) {
    std::terminate();
  }
  impl_->cursor = prepared->source_cursor + 1U;
  impl_->epoch = prepared->source_epoch + 1U;
  if (impl_->bound_resident_locator_instance_id == 0U) {
    impl_->bound_resident_locator_instance_id =
        prepared->source_identity.locator_instance_id;
  } else if (impl_->bound_resident_locator_instance_id !=
             prepared->source_identity.locator_instance_id) {
    std::terminate();
  }
  impl_->expected_stamp = committed_stamp;
  impl_->next_query_token = prepared->final_query_token;

  prepared->receipt.source_postbatch_stamp = committed_stamp;
  prepared->receipt.one_carrier_published_per_resultant_source_root = true;
  prepared->receipt.resident_batch_committed_before_vertical_publication =
      true;
  prepared->receipt.publication_suffix_noexcept = true;
  if (!prepared->receipt.certified_relative_streamed_batch() ||
      !certified_relative_bridge()) {
    std::terminate();
  }
  output.receipt.emplace(std::move(prepared->receipt));
  output.vertical_state_mutated = true;
  output.resident_committed_before_vertical_publication = true;
  output.postcommit_suffix_noexcept_or_fail_stop = true;
  output.decision = ExactDirectMorseUnifiedResidentVerticalCommitDecision::
      complete_certified_resident_and_vertical_atomic_commit;
  return output;
}

bool ExactDirectMorseUnifiedResidentVerticalBridgeInitializationResult::
    certified_initialized_bridge() const noexcept {
  return bridge.has_value() && bridge->certified_relative_bridge() &&
         resident_empty_prefix_certified && external_k2_k1_seam_certified &&
         persistent_carrier_capacity_preallocated &&
         no_global_membership_star_gamma_or_delaunay_materialized &&
         !public_status_claimed &&
         decision ==
             ExactDirectMorseUnifiedResidentVerticalBridgeInitializationDecision::
                 complete_certified_empty_relative_vertical_bridge;
}

ExactDirectMorseUnifiedResidentVerticalBridgeInitializationResult
initialize_exact_direct_morse_unified_resident_vertical_stream_bridge(
    const ExactDirectMorseUnifiedResidentSession& resident,
    const ExactDirectMorseUnifiedResidentVerticalStreamBridgeBudget& budget,
    const ExactDirectMorseUnifiedResidentVerticalStreamBridgeConfig& config) {
  ExactDirectMorseUnifiedResidentVerticalBridgeInitializationResult output;
  if (!resident.certified_resident_session() || resident.batch_cursor() != 0U ||
      resident.epoch() != 0U || !resident.root_coverages().empty() ||
      !resident.group_records().empty() ||
      resident.locator().snapshot_stamp().committed_batch_count != 0U) {
    output.decision =
        ExactDirectMorseUnifiedResidentVerticalBridgeInitializationDecision::
            no_bridge_resident_not_at_empty_prefix;
    return output;
  }
  const auto& plan = resident.plan();
  const auto stamp = resident.locator().snapshot_stamp();
  if (!config.external_k2_k1.certified_external_seam() ||
      config.external_k2_k1.canonical_cloud_digest !=
          plan.source_pair_canonical_cloud_digest ||
      config.external_k2_k1.point_count != plan.point_count ||
      config.external_k2_k1.external_seam_authority_id ==
          stamp.external_authority_id) {
    output.decision =
        ExactDirectMorseUnifiedResidentVerticalBridgeInitializationDecision::
            no_bridge_external_seam_rejected;
    return output;
  }
  if (config.bridge_authority_id == 0U ||
      config.bridge_authority_id == stamp.external_authority_id ||
      config.bridge_authority_id ==
          config.external_k2_k1.external_seam_authority_id ||
      config.first_query_replay_token == 0U ||
      config.first_query_replay_token ==
          std::numeric_limits<std::uint64_t>::max() ||
      !config.query_replay_token_suffix_reserved_by_resident_authority ||
      plan.batches.size() > budget.maximum_resident_batch_count ||
      (plan.batches.size() != 0U &&
       budget.maximum_persistent_source_root_carrier_count == 0U)) {
    output.decision =
        ExactDirectMorseUnifiedResidentVerticalBridgeInitializationDecision::
            no_bridge_budget_rejected;
    return output;
  }
  try {
    auto impl = std::make_unique<
        ExactDirectMorseUnifiedResidentVerticalStreamBridge::Impl>();
    impl->budget = budget;
    impl->config = config;
    impl->source_pair_canonical_cloud_digest =
        plan.source_pair_canonical_cloud_digest;
    impl->source_higher_canonical_cloud_digest =
        plan.source_higher_canonical_cloud_digest;
    impl->source_pair_semantic_digest = plan.source_pair_semantic_digest;
    impl->source_higher_semantic_digest = plan.source_higher_semantic_digest;
    impl->point_count = plan.point_count;
    impl->expected_stamp = stamp;
    impl->next_query_token = config.first_query_replay_token;
    impl->seal = std::make_shared<const VerticalBridgeSeal>(
        VerticalBridgeSeal{
            config.bridge_authority_id,
            stamp.external_authority_id});
    impl->ticket_registry =
        std::make_shared<VerticalOutstandingTicketRegistry>();
    impl->carriers.reserve(
        budget.maximum_persistent_source_root_carrier_count);
    impl->initialized = true;
    ExactDirectMorseUnifiedResidentVerticalStreamBridge bridge{
        std::move(impl)};
    if (!bridge.certified_relative_bridge()) {
      output.decision =
          ExactDirectMorseUnifiedResidentVerticalBridgeInitializationDecision::
              no_bridge_budget_rejected;
      return output;
    }
    output.bridge.emplace(std::move(bridge));
    output.resident_empty_prefix_certified = true;
    output.external_k2_k1_seam_certified = true;
    output.persistent_carrier_capacity_preallocated = true;
    output.no_global_membership_star_gamma_or_delaunay_materialized = true;
    output.decision =
        ExactDirectMorseUnifiedResidentVerticalBridgeInitializationDecision::
            complete_certified_empty_relative_vertical_bridge;
    return output;
  } catch (const std::bad_alloc&) {
    output.decision =
        ExactDirectMorseUnifiedResidentVerticalBridgeInitializationDecision::
            no_bridge_allocation_failed;
    return output;
  } catch (const std::length_error&) {
    output.decision =
        ExactDirectMorseUnifiedResidentVerticalBridgeInitializationDecision::
            no_bridge_budget_rejected;
    return output;
  }
}

}  // namespace morsehgp3d::hierarchy
