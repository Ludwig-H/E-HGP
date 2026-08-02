#include "morsehgp3d/hierarchy/direct_morse_resident_k2_k1_closed_cut_bridge.hpp"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

std::atomic<std::uint64_t> next_bridge_session_instance_id{1U};

[[nodiscard]] std::uint64_t allocate_bridge_session_instance_id() noexcept {
  std::uint64_t candidate =
      next_bridge_session_instance_id.load(std::memory_order_relaxed);
  while (candidate != 0U) {
    const std::uint64_t successor = candidate + 1U;
    if (next_bridge_session_instance_id.compare_exchange_weak(
            candidate,
            successor,
            std::memory_order_relaxed,
            std::memory_order_relaxed)) {
      return candidate;
    }
  }
  return 0U;
}

[[nodiscard]] bool checked_add(
    std::size_t left, std::size_t right, std::size_t& output) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  output = left + right;
  return true;
}

[[nodiscard]] bool valid_slice(
    std::size_t offset,
    std::size_t count,
    std::size_t arena_size) noexcept {
  return offset <= arena_size && count <= arena_size - offset;
}

[[nodiscard]] const ExactDirectFrozenUnifiedPriorRootCoverage*
find_prior_root_coverage(
    const ExactDirectMorseUnifiedResidentAuthorityBundle& bundle,
    ExactFrozenIncidencePriorRootId root_id) noexcept {
  const auto iterator = std::lower_bound(
      bundle.prior_root_coverages.begin(),
      bundle.prior_root_coverages.end(),
      root_id,
      [](const ExactDirectFrozenUnifiedPriorRootCoverage& coverage,
         ExactFrozenIncidencePriorRootId value) {
        return coverage.prior_root_id < value;
      });
  if (iterator == bundle.prior_root_coverages.end() ||
      iterator->prior_root_id != root_id) {
    return nullptr;
  }
  return &*iterator;
}

[[nodiscard]] const ExactDirectFrozenUnifiedLatentCarrierCoverage*
find_latent_carrier_coverage(
    const ExactDirectMorseUnifiedResidentAuthorityBundle& bundle,
    ExactFrozenIncidenceTokenId token_id) noexcept {
  const auto iterator = std::lower_bound(
      bundle.latent_carrier_coverages.begin(),
      bundle.latent_carrier_coverages.end(),
      token_id,
      [](const ExactDirectFrozenUnifiedLatentCarrierCoverage& coverage,
         ExactFrozenIncidenceTokenId value) {
        return coverage.latent_carrier_token_id < value;
      });
  if (iterator == bundle.latent_carrier_coverages.end() ||
      iterator->latent_carrier_token_id != token_id) {
    return nullptr;
  }
  return &*iterator;
}

[[nodiscard]] bool count_group_image_references(
    const ExactDirectMorseUnifiedResidentAuthorityBundle& bundle,
    std::size_t& output_count) noexcept {
  output_count = 0U;
  const auto& frozen = bundle.frozen_batch;
  const auto& actions = frozen.action_plan;
  const auto& quotient = frozen.quotient;
  if (actions.groups.size() != quotient.groups.size() ||
      actions.groups.size() != frozen.coverage_deltas.size()) {
    return false;
  }
  for (std::size_t group_index = 0U;
       group_index < actions.groups.size();
       ++group_index) {
    const auto& action_group = actions.groups[group_index];
    const auto& quotient_group = quotient.groups[group_index];
    const auto& delta = frozen.coverage_deltas[group_index];
    if (action_group.group_index != group_index ||
        quotient_group.group_index != group_index ||
        delta.owner_group_index != group_index ||
        !valid_slice(
            action_group.prior_root_offset,
            action_group.prior_root_count,
            actions.prior_root_ids.size()) ||
        !valid_slice(
            quotient_group.token_offset,
            quotient_group.token_count,
            quotient.group_tokens.size()) ||
        !valid_slice(
            delta.point_reference_offset,
            delta.point_reference_count,
            frozen.coverage_delta_points.size())) {
      return false;
    }

    for (std::size_t local_index = 0U;
         local_index < action_group.prior_root_count;
         ++local_index) {
      const auto root_id = actions.prior_root_ids[
          action_group.prior_root_offset + local_index];
      const auto* coverage = find_prior_root_coverage(bundle, root_id);
      if (coverage == nullptr ||
          !valid_slice(
              coverage->point_reference_offset,
              coverage->point_reference_count,
              bundle.prior_root_coverage_point_references.size()) ||
          !checked_add(
              output_count, coverage->point_reference_count, output_count)) {
        return false;
      }
    }

    for (std::size_t local_index = 0U;
         local_index < quotient_group.token_count;
         ++local_index) {
      const auto& token = quotient.group_tokens[
          quotient_group.token_offset + local_index];
      if (token.kind != ExactFrozenIncidenceTokenKind::latent_carrier) {
        continue;
      }
      const auto* coverage =
          find_latent_carrier_coverage(bundle, token.token_id);
      if (coverage == nullptr ||
          !valid_slice(
              coverage->point_reference_offset,
              coverage->point_reference_count,
              bundle.latent_carrier_coverage_point_references.size()) ||
          !checked_add(
              output_count, coverage->point_reference_count, output_count)) {
        return false;
      }
    }
    if (!checked_add(
            output_count, delta.point_reference_count, output_count)) {
      return false;
    }
  }
  return true;
}

struct GroupPointSlice {
  std::size_t offset{};
  std::size_t count{};
};

[[nodiscard]] bool append_group_image_points(
    const ExactDirectMorseUnifiedResidentAuthorityBundle& bundle,
    std::size_t group_index,
    std::vector<spatial::PointId>& point_scratch,
    GroupPointSlice& output_slice) {
  const auto& frozen = bundle.frozen_batch;
  const auto& actions = frozen.action_plan;
  const auto& quotient = frozen.quotient;
  const auto& action_group = actions.groups[group_index];
  const auto& quotient_group = quotient.groups[group_index];
  const auto& delta = frozen.coverage_deltas[group_index];
  output_slice.offset = point_scratch.size();

  for (std::size_t local_index = 0U;
       local_index < action_group.prior_root_count;
       ++local_index) {
    const auto root_id = actions.prior_root_ids[
        action_group.prior_root_offset + local_index];
    const auto* coverage = find_prior_root_coverage(bundle, root_id);
    if (coverage == nullptr) {
      return false;
    }
    const auto first =
        bundle.prior_root_coverage_point_references.begin() +
        static_cast<std::ptrdiff_t>(coverage->point_reference_offset);
    point_scratch.insert(
        point_scratch.end(),
        first,
        first + static_cast<std::ptrdiff_t>(coverage->point_reference_count));
  }

  for (std::size_t local_index = 0U;
       local_index < quotient_group.token_count;
       ++local_index) {
    const auto& token = quotient.group_tokens[
        quotient_group.token_offset + local_index];
    if (token.kind != ExactFrozenIncidenceTokenKind::latent_carrier) {
      continue;
    }
    const auto* coverage =
        find_latent_carrier_coverage(bundle, token.token_id);
    if (coverage == nullptr) {
      return false;
    }
    const auto first =
        bundle.latent_carrier_coverage_point_references.begin() +
        static_cast<std::ptrdiff_t>(coverage->point_reference_offset);
    point_scratch.insert(
        point_scratch.end(),
        first,
        first + static_cast<std::ptrdiff_t>(coverage->point_reference_count));
  }

  for (std::size_t local_index = 0U;
       local_index < delta.point_reference_count;
       ++local_index) {
    const auto& point_reference = frozen.coverage_delta_points[
        delta.point_reference_offset + local_index];
    if (point_reference.owner_group_index != group_index) {
      return false;
    }
    point_scratch.push_back(point_reference.point_id);
  }

  auto first = point_scratch.begin() +
      static_cast<std::ptrdiff_t>(output_slice.offset);
  std::sort(first, point_scratch.end());
  const auto unique_end = std::unique(first, point_scratch.end());
  point_scratch.erase(unique_end, point_scratch.end());
  output_slice.count = point_scratch.size() - output_slice.offset;
  return output_slice.count != 0U;
}

[[nodiscard]] bool prepared_group_image_valid(
    const ExactDirectMorseResidentK2K1ClosedCutGroupImage& image) noexcept {
  return image.exhaustive_distinct_point_count != 0U &&
         image.singleton_root_query_count ==
             image.exhaustive_distinct_point_count &&
         image.prior_root_coverage_csr_consumed &&
         image.latent_carrier_coverage_csr_consumed &&
         image.exact_coverage_delta_consumed &&
         image.exhaustive_group_image_certified &&
         image.every_singleton_query_live_verified &&
         image.unique_closed_k1_root_certified;
}

}  // namespace

struct ExactDirectMorseResidentK2K1ClosedCutBridge::Impl {
  struct Seal {
    std::uint64_t bridge_session_instance_id{};
    std::uint64_t resident_session_authority_id{};
    std::uint64_t k1_session_instance_id{};
    contract::CanonicalId canonical_cloud_digest{};
    contract::CanonicalId resident_higher_canonical_cloud_digest{};
  };

  ExactDirectMorseUnifiedResidentSession resident{};
  ExactDirectK1BoruvkaClosedCutSession k1{};
  ExactDirectMorseResidentK2K1ClosedCutBridgeBudget budget{};
  std::shared_ptr<const Seal> seal;
  ExactDirectMorseResidentK2K1ClosedCutBridgeStamp committed_stamp{};
  std::vector<ExactDirectMorseResidentK2K1ClosedCutBatchRecord>
      committed_k2_batches;
  std::optional<ExactDirectMorseResidentK2K1ClosedCutTerminalTargetResult>
      terminal_k1_target_history;
  bool initialized{false};

  [[nodiscard]] bool structurally_ready() const noexcept {
    const bool terminal_history_ready =
        !terminal_k1_target_history.has_value() ||
        (terminal_k1_target_history->certified_terminal_target_history() &&
         resident.complete() && k1.terminal_complete() &&
         terminal_k1_target_history->k1_source_forest_digest ==
             k1.source_forest_digest() &&
         terminal_k1_target_history->distinct_k1_level_count ==
             k1.distinct_level_count() &&
         committed_stamp.committed_k1_stamp.committed_level_cursor ==
             terminal_k1_target_history->terminal_k1_level_cursor &&
         committed_stamp.committed_k1_stamp.committed_history_digest ==
             terminal_k1_target_history->terminal_k1_history_digest);
    return terminal_history_ready && initialized && seal != nullptr &&
           seal->bridge_session_instance_id != 0U &&
           seal->resident_session_authority_id != 0U &&
           seal->k1_session_instance_id != 0U &&
           seal->canonical_cloud_digest != contract::CanonicalId{} &&
           seal->resident_higher_canonical_cloud_digest !=
               contract::CanonicalId{} &&
           resident.certified_resident_session() && k1.ready() &&
           resident.locator().snapshot_stamp().external_authority_id ==
               seal->resident_session_authority_id &&
           committed_stamp.schema_version ==
               direct_morse_resident_k2_k1_closed_cut_bridge_schema_version &&
           committed_stamp.bridge_session_instance_id ==
               seal->bridge_session_instance_id &&
           committed_stamp.resident_session_authority_id ==
               seal->resident_session_authority_id &&
           committed_stamp.resident_batch_cursor == resident.batch_cursor() &&
           committed_stamp.resident_epoch == resident.epoch() &&
           committed_stamp.committed_k2_batch_count ==
               committed_k2_batches.size() &&
           committed_stamp.committed_k2_group_count <=
               budget.maximum_committed_k2_group_count &&
           committed_k2_batches.size() <=
               budget.maximum_committed_k2_batch_count &&
           committed_stamp.canonical_cloud_digest ==
               seal->canonical_cloud_digest &&
           committed_stamp.resident_higher_canonical_cloud_digest ==
               seal->resident_higher_canonical_cloud_digest &&
           committed_stamp.committed_k1_stamp.session_instance_id ==
               seal->k1_session_instance_id &&
           committed_stamp.committed_k1_stamp.canonical_cloud_digest ==
               seal->canonical_cloud_digest;
  }
};

struct ExactDirectMorseResidentK2K1ClosedCutPreparedBatch::Impl {
  std::shared_ptr<const ExactDirectMorseResidentK2K1ClosedCutBridge::Impl::Seal>
      seal;
  std::optional<ExactDirectMorseUnifiedResidentPreparedBatch> resident_ticket;
  ExactDirectMorseUnifiedSnapshotIdentity resident_identity{};
  std::optional<ExactDirectMorseResidentK2K1ClosedCutBatchRecord> k2_record;
  ExactDirectK1BoruvkaClosedCutSessionStamp next_committed_k1_stamp{};
  std::size_t resident_group_record_count_before{};
  bool consumed{false};
};

bool ExactDirectMorseResidentK2K1ClosedCutGroupImage::
    certified_conditional_group_image() const noexcept {
  return resident_resultant_root_id != 0U &&
         prepared_group_image_valid(*this);
}

bool ExactDirectMorseResidentK2K1ClosedCutBatchRecord::
    certified_conditional_k2_to_k1_batch() const noexcept {
  if (schema_version !=
          direct_morse_resident_k2_k1_closed_cut_bridge_schema_version ||
      order != 2U || resident_pre_batch_identity.batch_cursor !=
                         source_batch_index ||
      resident_pre_batch_identity.epoch != source_batch_index ||
      resident_pre_batch_identity.session_authority_id == 0U ||
      resident_pre_batch_identity.locator_instance_id == 0U ||
      resident_pre_batch_identity.source_pair_canonical_cloud_digest !=
          published_pre_k1_stamp.canonical_cloud_digest ||
      resident_pre_batch_identity.source_higher_canonical_cloud_digest ==
          contract::CanonicalId{} ||
      live_post_k1_stamp.session_instance_id !=
          published_pre_k1_stamp.session_instance_id ||
      live_post_k1_stamp.canonical_cloud_digest !=
          published_pre_k1_stamp.canonical_cloud_digest ||
      live_post_k1_stamp.committed_level_cursor <
          published_pre_k1_stamp.committed_level_cursor ||
      live_post_k1_stamp.closed_squared_level != squared_level ||
      consumed_intermediate_k1_level_count !=
          live_post_k1_stamp.committed_level_cursor -
              published_pre_k1_stamp.committed_level_cursor ||
      distinct_group_point_count != singleton_root_query_count ||
      !k1_cut_advanced_before_resident_commit ||
      !every_intermediate_k1_level_consumed ||
      !group_images_exhaustive_from_frozen_csr ||
      !every_group_has_one_live_closed_k1_root || !resident_batch_committed ||
      !post_commit_publication_allocation_free ||
      incidence_complete_reduction || vertical_maps_complete ||
      global_facet_coface_or_gamma_catalog_materialized ||
      ordinary_or_higher_order_delaunay_materialized ||
      pair_matrix_materialized || public_status_claimed) {
    return false;
  }
  std::size_t expected_distinct_count = 0U;
  std::size_t expected_query_count = 0U;
  for (std::size_t group_index = 0U;
       group_index < group_images.size();
       ++group_index) {
    const auto& image = group_images[group_index];
    if (image.owner_group_index != group_index ||
        !image.certified_conditional_group_image() ||
        !checked_add(
            expected_distinct_count,
            image.exhaustive_distinct_point_count,
            expected_distinct_count) ||
        !checked_add(
            expected_query_count,
            image.singleton_root_query_count,
            expected_query_count)) {
      return false;
    }
  }
  return expected_distinct_count == distinct_group_point_count &&
         expected_query_count == singleton_root_query_count;
}

ExactDirectMorseResidentK2K1ClosedCutPreparedBatch::
    ExactDirectMorseResidentK2K1ClosedCutPreparedBatch() noexcept = default;
ExactDirectMorseResidentK2K1ClosedCutPreparedBatch::
    ~ExactDirectMorseResidentK2K1ClosedCutPreparedBatch() = default;
ExactDirectMorseResidentK2K1ClosedCutPreparedBatch::
    ExactDirectMorseResidentK2K1ClosedCutPreparedBatch(
        ExactDirectMorseResidentK2K1ClosedCutPreparedBatch&&) noexcept =
    default;
ExactDirectMorseResidentK2K1ClosedCutPreparedBatch&
ExactDirectMorseResidentK2K1ClosedCutPreparedBatch::operator=(
    ExactDirectMorseResidentK2K1ClosedCutPreparedBatch&&) noexcept = default;
ExactDirectMorseResidentK2K1ClosedCutPreparedBatch::
    ExactDirectMorseResidentK2K1ClosedCutPreparedBatch(
        std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectMorseResidentK2K1ClosedCutPreparedBatch::valid() const
    noexcept {
  return impl_ != nullptr && impl_->seal != nullptr &&
         impl_->resident_ticket.has_value() &&
         impl_->resident_ticket->valid() && !impl_->consumed &&
         (!impl_->k2_record.has_value() ||
          (impl_->k2_record->order == 2U &&
           impl_->k2_record->resident_pre_batch_identity ==
               impl_->resident_identity &&
           impl_->k2_record->group_images_exhaustive_from_frozen_csr &&
           impl_->k2_record->every_group_has_one_live_closed_k1_root &&
           !impl_->k2_record->resident_batch_committed));
}

bool ExactDirectMorseResidentK2K1ClosedCutPreparedBatch::k2_vertical_batch()
    const noexcept {
  return impl_ != nullptr && impl_->k2_record.has_value();
}

const ExactDirectMorseUnifiedSnapshotIdentity&
ExactDirectMorseResidentK2K1ClosedCutPreparedBatch::
    resident_pre_batch_identity() const noexcept {
  static const ExactDirectMorseUnifiedSnapshotIdentity empty{};
  return impl_ == nullptr ? empty : impl_->resident_identity;
}

const ExactDirectMorseUnifiedResidentAuthorityBundle&
ExactDirectMorseResidentK2K1ClosedCutPreparedBatch::
    resident_authority_bundle() const noexcept {
  static const ExactDirectMorseUnifiedResidentAuthorityBundle empty{};
  return impl_ == nullptr || !impl_->resident_ticket.has_value()
             ? empty
             : impl_->resident_ticket->authority_bundle();
}

const ExactDirectMorseResidentK2K1ClosedCutBatchRecord*
ExactDirectMorseResidentK2K1ClosedCutPreparedBatch::
    conditional_batch_record() const noexcept {
  return impl_ == nullptr || !impl_->k2_record.has_value()
             ? nullptr
             : &*impl_->k2_record;
}

bool ExactDirectMorseResidentK2K1ClosedCutPreparationResult::
    certified_prepared_batch() const noexcept {
  const bool transit =
      decision == ExactDirectMorseResidentK2K1ClosedCutPreparationDecision::
                      complete_prepared_non_k2_transit_batch &&
      non_k2_transit_only &&
      !conditional_k2_to_k1_group_images_prepared;
  const bool k2 =
      decision == ExactDirectMorseResidentK2K1ClosedCutPreparationDecision::
                      complete_prepared_conditional_k2_to_k1_batch &&
      !non_k2_transit_only &&
      conditional_k2_to_k1_group_images_prepared;
  return ticket.has_value() && ticket->valid() && (transit || k2) &&
         no_resident_or_published_vertical_state_mutated_on_failure;
}

bool ExactDirectMorseResidentK2K1ClosedCutCommitResult::
    certified_committed_batch() const noexcept {
  const bool transit =
      decision == ExactDirectMorseResidentK2K1ClosedCutCommitDecision::
                      complete_committed_non_k2_transit_batch &&
      !vertical_state_mutated &&
      !post_resident_commit_publication_allocation_free;
  const bool k2 =
      decision == ExactDirectMorseResidentK2K1ClosedCutCommitDecision::
                      complete_committed_conditional_k2_to_k1_batch &&
      vertical_state_mutated &&
      post_resident_commit_publication_allocation_free;
  return ticket_consumed && resident_commit.certified_committed_batch() &&
         !no_vertical_state_mutated_on_resident_rejection &&
         (transit || k2);
}

bool ExactDirectMorseResidentK2K1ClosedCutTerminalTargetResult::
    certified_terminal_target_history() const noexcept {
  return schema_version ==
             direct_morse_resident_k2_k1_closed_cut_bridge_schema_version &&
         k1_session_instance_id != 0U &&
         pre_k1_level_cursor <= terminal_k1_level_cursor &&
         terminal_k1_level_cursor == distinct_k1_level_count &&
         consumed_target_only_k1_level_count ==
             terminal_k1_level_cursor - pre_k1_level_cursor &&
         k1_source_forest_digest != contract::CanonicalId{} &&
         terminal_k1_history_digest != contract::CanonicalId{} &&
         resident_cursor_exhausted && live_k1_advance_receipt_verified &&
         every_remaining_k1_equal_level_batch_consumed &&
         terminal_k1_cursor_complete &&
         owned_k1_state_mutated ==
             (consumed_target_only_k1_level_count != 0U) &&
         !global_facet_coface_or_gamma_catalog_materialized &&
         !ordinary_or_higher_order_delaunay_materialized &&
         !public_status_claimed &&
         decision ==
             ExactDirectMorseResidentK2K1ClosedCutTerminalTargetDecision::
                 complete_certified_owned_k1_terminal_target_history;
}

ExactDirectMorseResidentK2K1ClosedCutBridge::
    ExactDirectMorseResidentK2K1ClosedCutBridge() noexcept = default;
ExactDirectMorseResidentK2K1ClosedCutBridge::
    ~ExactDirectMorseResidentK2K1ClosedCutBridge() = default;
ExactDirectMorseResidentK2K1ClosedCutBridge::
    ExactDirectMorseResidentK2K1ClosedCutBridge(
        ExactDirectMorseResidentK2K1ClosedCutBridge&&) noexcept = default;
ExactDirectMorseResidentK2K1ClosedCutBridge&
ExactDirectMorseResidentK2K1ClosedCutBridge::operator=(
    ExactDirectMorseResidentK2K1ClosedCutBridge&&) noexcept = default;
ExactDirectMorseResidentK2K1ClosedCutBridge::
    ExactDirectMorseResidentK2K1ClosedCutBridge(
        std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectMorseResidentK2K1ClosedCutBridge::ready() const noexcept {
  return impl_ != nullptr && impl_->structurally_ready();
}

bool ExactDirectMorseResidentK2K1ClosedCutBridge::resident_complete() const
    noexcept {
  return ready() && impl_->resident.complete();
}

ExactDirectMorseResidentK2K1ClosedCutBridgeStamp
ExactDirectMorseResidentK2K1ClosedCutBridge::current_stamp() const {
  if (!ready()) {
    throw std::logic_error("the K2-to-K1 resident bridge is not ready");
  }
  return impl_->committed_stamp;
}

bool ExactDirectMorseResidentK2K1ClosedCutBridge::verify_stamp(
    const ExactDirectMorseResidentK2K1ClosedCutBridgeStamp& stamp) const
    noexcept {
  return ready() && stamp == impl_->committed_stamp;
}

const std::vector<ExactDirectMorseResidentK2K1ClosedCutBatchRecord>&
ExactDirectMorseResidentK2K1ClosedCutBridge::committed_k2_batches() const
    noexcept {
  static const std::vector<
      ExactDirectMorseResidentK2K1ClosedCutBatchRecord>
      empty;
  return impl_ == nullptr ? empty : impl_->committed_k2_batches;
}

ExactDirectMorseUnifiedResidentSourceKind
ExactDirectMorseResidentK2K1ClosedCutBridge::resident_source_kind() const
    noexcept {
  return impl_ == nullptr
             ? ExactDirectMorseUnifiedResidentSourceKind::
                   successive_incidence_star
             : impl_->resident.source_kind();
}

bool ExactDirectMorseResidentK2K1ClosedCutBridge::
    resident_normalized_direct_source_session() const noexcept {
  return impl_ != nullptr && impl_->resident.normalized_direct_source_session();
}

ExactDirectNormalizedH0ResidentRetractionMode
ExactDirectMorseResidentK2K1ClosedCutBridge::
    resident_normalized_h0_retraction_mode() const noexcept {
  return impl_ == nullptr
             ? ExactDirectNormalizedH0ResidentRetractionMode::
                   not_applicable_successive_incidence_star
             : impl_->resident.normalized_h0_retraction_mode();
}

bool ExactDirectMorseResidentK2K1ClosedCutBridge::
    resident_normalized_horizontal_incidence_reduction_certified()
        const noexcept {
  return impl_ != nullptr &&
         impl_->resident
             .normalized_horizontal_incidence_reduction_certified();
}

ExactDirectK1BoruvkaClosedCutSessionStamp
ExactDirectMorseResidentK2K1ClosedCutBridge::owned_k1_current_stamp() const {
  if (!ready()) {
    throw std::logic_error("the K2-to-K1 resident bridge is not ready");
  }
  return impl_->k1.current_stamp();
}

contract::CanonicalId
ExactDirectMorseResidentK2K1ClosedCutBridge::owned_k1_source_forest_digest()
    const noexcept {
  return impl_ == nullptr ? contract::CanonicalId{}
                          : impl_->k1.source_forest_digest();
}

bool ExactDirectMorseResidentK2K1ClosedCutBridge::
    verify_owned_k1_source_forest_digest(
        const contract::CanonicalId& digest) const noexcept {
  return impl_ != nullptr && impl_->k1.verify_source_forest_digest(digest);
}

std::size_t
ExactDirectMorseResidentK2K1ClosedCutBridge::owned_k1_distinct_level_count()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->k1.distinct_level_count();
}

bool ExactDirectMorseResidentK2K1ClosedCutBridge::owned_k1_terminal_complete()
    const noexcept {
  return impl_ != nullptr && impl_->k1.terminal_complete();
}

const ExactDirectSparseUnifiedLevelPlanResult&
ExactDirectMorseResidentK2K1ClosedCutBridge::resident_plan() const noexcept {
  static const ExactDirectSparseUnifiedLevelPlanResult empty{};
  return impl_ == nullptr ? empty : impl_->resident.plan();
}

const ExactDirectSparsePositiveFacetLocator&
ExactDirectMorseResidentK2K1ClosedCutBridge::resident_locator() const noexcept {
  static const ExactDirectSparsePositiveFacetLocator empty{};
  return impl_ == nullptr ? empty : impl_->resident.locator();
}

const std::vector<ExactDirectMorseUnifiedResidentComponentState>&
ExactDirectMorseResidentK2K1ClosedCutBridge::resident_component_states()
    const noexcept {
  static const std::vector<ExactDirectMorseUnifiedResidentComponentState>
      empty;
  return impl_ == nullptr ? empty : impl_->resident.component_states();
}

const std::vector<ExactDirectMorseUnifiedResidentRootCoverage>&
ExactDirectMorseResidentK2K1ClosedCutBridge::resident_root_coverages() const
    noexcept {
  static const std::vector<ExactDirectMorseUnifiedResidentRootCoverage> empty;
  return impl_ == nullptr ? empty : impl_->resident.root_coverages();
}

const std::vector<ExactDirectMorseUnifiedResidentGroupRecord>&
ExactDirectMorseResidentK2K1ClosedCutBridge::resident_group_records() const
    noexcept {
  static const std::vector<ExactDirectMorseUnifiedResidentGroupRecord> empty;
  return impl_ == nullptr ? empty : impl_->resident.group_records();
}

ExactDirectMorseResidentK2K1ClosedCutPreparationResult
ExactDirectMorseResidentK2K1ClosedCutBridge::prepare_next() {
  ExactDirectMorseResidentK2K1ClosedCutPreparationResult output;
  output.no_resident_or_published_vertical_state_mutated_on_failure = true;
  const auto reject = [&output](
                          ExactDirectMorseResidentK2K1ClosedCutPreparationDecision
                              decision) {
    output.ticket.reset();
    output.non_k2_transit_only = false;
    output.conditional_k2_to_k1_group_images_prepared = false;
    output.decision = decision;
    return std::move(output);
  };
  if (!ready()) {
    return reject(
        ExactDirectMorseResidentK2K1ClosedCutPreparationDecision::
            no_bridge_not_ready);
  }

  auto resident_preparation = impl_->resident.prepare_next();
  if (!resident_preparation.certified_prepared_batch() ||
      !resident_preparation.ticket.has_value()) {
    return reject(
        ExactDirectMorseResidentK2K1ClosedCutPreparationDecision::
            no_resident_preparation_rejected);
  }
  const auto& bundle = resident_preparation.ticket->authority_bundle();
  const auto current_locator_stamp = impl_->resident.locator().snapshot_stamp();
  if (!bundle.certified_strict_pre_batch_bundle() ||
      bundle.identity.session_authority_id !=
          impl_->seal->resident_session_authority_id ||
      bundle.identity.locator_instance_id == 0U ||
      (impl_->committed_stamp.resident_locator_instance_id != 0U &&
       bundle.identity.locator_instance_id !=
           impl_->committed_stamp.resident_locator_instance_id) ||
      bundle.identity.batch_cursor != impl_->resident.batch_cursor() ||
      bundle.identity.epoch != impl_->resident.epoch() ||
      bundle.identity.locator_stamp != current_locator_stamp ||
      bundle.identity.source_pair_canonical_cloud_digest !=
          impl_->seal->canonical_cloud_digest ||
      bundle.identity.source_higher_canonical_cloud_digest !=
          impl_->seal->resident_higher_canonical_cloud_digest ||
      bundle.source_batch_index != bundle.identity.batch_cursor) {
    return reject(
        ExactDirectMorseResidentK2K1ClosedCutPreparationDecision::
            no_resident_pre_batch_identity_rejected);
  }

  output.requirements.resident_batch_count = impl_->resident.plan().batches.size();
  for (const auto& planned_batch : impl_->resident.plan().batches) {
    if (planned_batch.order == 2U) {
      ++output.requirements.resident_k2_batch_count;
    }
  }
  try {
    auto prepared = std::make_unique<
        ExactDirectMorseResidentK2K1ClosedCutPreparedBatch::Impl>();
    prepared->seal = impl_->seal;
    prepared->resident_identity = bundle.identity;
    prepared->resident_group_record_count_before =
        impl_->resident.group_records().size();
    prepared->resident_ticket.emplace(
        std::move(*resident_preparation.ticket));

    if (bundle.order != 2U) {
      output.ticket.emplace(
          ExactDirectMorseResidentK2K1ClosedCutPreparedBatch{
              std::move(prepared)});
      output.non_k2_transit_only = true;
      output.decision =
          ExactDirectMorseResidentK2K1ClosedCutPreparationDecision::
              complete_prepared_non_k2_transit_batch;
      return output;
    }

    std::size_t reference_scan_count = 0U;
    if (!count_group_image_references(bundle, reference_scan_count)) {
      return reject(
          ExactDirectMorseResidentK2K1ClosedCutPreparationDecision::
              no_frozen_k2_group_image_rejected);
    }
    const std::size_t group_count =
        bundle.frozen_batch.action_plan.groups.size();
    std::size_t next_committed_group_count = 0U;
    if (impl_->committed_k2_batches.size() >=
            impl_->budget.maximum_committed_k2_batch_count ||
        group_count > impl_->budget.maximum_prepared_k2_group_count ||
        reference_scan_count >
            impl_->budget.maximum_group_coverage_point_reference_scan_count ||
        reference_scan_count >
            impl_->budget.maximum_group_point_scratch_count ||
        !checked_add(
            impl_->committed_stamp.committed_k2_group_count,
            group_count,
            next_committed_group_count) ||
        next_committed_group_count >
            impl_->budget.maximum_committed_k2_group_count) {
      return reject(
          ExactDirectMorseResidentK2K1ClosedCutPreparationDecision::
              no_preparation_budget_exhausted);
    }

    output.requirements.prepared_group_count = group_count;
    output.requirements.group_coverage_point_reference_scan_count =
        reference_scan_count;
    prepared->k2_record.emplace();
    auto& record = *prepared->k2_record;
    record.resident_pre_batch_identity = bundle.identity;
    record.source_batch_index = bundle.source_batch_index;
    record.squared_level = bundle.squared_level;
    record.order = bundle.order;
    record.published_pre_k1_stamp = impl_->committed_stamp.committed_k1_stamp;
    record.group_coverage_point_reference_scan_count = reference_scan_count;
    record.group_images.resize(group_count);

    std::vector<GroupPointSlice> group_slices(group_count);
    std::vector<spatial::PointId> point_scratch;
    point_scratch.reserve(reference_scan_count);
    for (std::size_t group_index = 0U;
         group_index < group_count;
         ++group_index) {
      if (!append_group_image_points(
              bundle,
              group_index,
              point_scratch,
              group_slices[group_index])) {
        return reject(
            ExactDirectMorseResidentK2K1ClosedCutPreparationDecision::
                no_frozen_k2_group_image_rejected);
      }
      auto& image = record.group_images[group_index];
      image.owner_group_index = group_index;
      image.exhaustive_distinct_point_count = group_slices[group_index].count;
      image.canonical_representative_point_id =
          point_scratch[group_slices[group_index].offset];
      image.prior_root_coverage_csr_consumed = true;
      image.latent_carrier_coverage_csr_consumed = true;
      image.exact_coverage_delta_consumed = true;
      image.exhaustive_group_image_certified = true;
      if (!checked_add(
              record.distinct_group_point_count,
              group_slices[group_index].count,
              record.distinct_group_point_count)) {
        return reject(
            ExactDirectMorseResidentK2K1ClosedCutPreparationDecision::
                no_preparation_budget_exhausted);
      }
    }
    if (record.distinct_group_point_count >
            impl_->budget.maximum_distinct_group_point_count ||
        record.distinct_group_point_count >
            impl_->budget.maximum_singleton_root_query_count) {
      return reject(
          ExactDirectMorseResidentK2K1ClosedCutPreparationDecision::
              no_preparation_budget_exhausted);
    }
    output.requirements.distinct_group_point_count =
        record.distinct_group_point_count;
    output.requirements.singleton_root_query_count =
        record.distinct_group_point_count;

    const auto live_pre_stamp = impl_->k1.current_stamp();
    if (live_pre_stamp.session_instance_id !=
            impl_->seal->k1_session_instance_id ||
        live_pre_stamp.canonical_cloud_digest !=
            impl_->seal->canonical_cloud_digest ||
        bundle.squared_level < live_pre_stamp.closed_squared_level) {
      return reject(
          ExactDirectMorseResidentK2K1ClosedCutPreparationDecision::
              no_k1_cut_advance_rejected);
    }
    auto advance = impl_->k1.advance_closed_to(bundle.squared_level);
    if (advance.decision !=
        ExactDirectK1BoruvkaClosedCutAdvanceDecision::
            complete_certified_monotone_closed_cut) {
      if (advance.state_mutated || impl_->k1.poisoned()) {
        std::terminate();
      }
      return reject(
          ExactDirectMorseResidentK2K1ClosedCutPreparationDecision::
              no_k1_cut_advance_rejected);
    }
    bool advance_verified = false;
    try {
      advance_verified = impl_->k1.verify_advance_receipt(advance);
    } catch (...) {
      std::terminate();
    }
    if (!advance_verified || !impl_->k1.verify_stamp(advance.post_stamp) ||
        advance.post_stamp.closed_squared_level != bundle.squared_level ||
        advance.post_stamp.committed_level_cursor <
            record.published_pre_k1_stamp.committed_level_cursor) {
      std::terminate();
    }
    record.live_post_k1_stamp = advance.post_stamp;
    prepared->next_committed_k1_stamp = advance.post_stamp;
    record.consumed_intermediate_k1_level_count =
        advance.post_stamp.committed_level_cursor -
        record.published_pre_k1_stamp.committed_level_cursor;
    record.k1_cut_advanced_before_resident_commit = true;
    record.every_intermediate_k1_level_consumed = true;
    output.k1_lower_cut_may_have_advanced_without_vertical_publication =
        advance.state_mutated;

    for (std::size_t group_index = 0U;
         group_index < group_count;
         ++group_index) {
      const auto slice = group_slices[group_index];
      auto& image = record.group_images[group_index];
      std::optional<K1NodeId> unique_root;
      for (std::size_t local_index = 0U;
           local_index < slice.count;
           ++local_index) {
        const spatial::PointId point_id =
            point_scratch[slice.offset + local_index];
        const auto query = impl_->k1.query_singleton_root(
            point_id, advance.post_stamp);
        if (!impl_->k1.verify_root_query_result(query) ||
            !query.closed_root_node_id.has_value()) {
          return reject(
              ExactDirectMorseResidentK2K1ClosedCutPreparationDecision::
                  no_k1_group_root_query_rejected);
        }
        if (!unique_root.has_value()) {
          unique_root = query.closed_root_node_id;
        } else if (*unique_root != *query.closed_root_node_id) {
          return reject(
              ExactDirectMorseResidentK2K1ClosedCutPreparationDecision::
                  no_k1_group_image_inconsistent);
        }
        ++image.singleton_root_query_count;
        ++record.singleton_root_query_count;
      }
      image.closed_k1_root_node_id = *unique_root;
      image.every_singleton_query_live_verified = true;
      image.unique_closed_k1_root_certified = true;
      if (!prepared_group_image_valid(image)) {
        std::terminate();
      }
    }
    record.group_images_exhaustive_from_frozen_csr = true;
    record.every_group_has_one_live_closed_k1_root = true;
    if (record.singleton_root_query_count !=
        record.distinct_group_point_count) {
      std::terminate();
    }

    // All PointId and exact-level allocations are complete.  The lower K1
    // cut is a monotone target-authority cache: a later resident rejection
    // may leave it at this level, but retrying the same batch is idempotent
    // and no vertical batch is published until the resident commit succeeds.
    std::vector<spatial::PointId>{}.swap(point_scratch);
    std::vector<GroupPointSlice>{}.swap(group_slices);
    output.ticket.emplace(
        ExactDirectMorseResidentK2K1ClosedCutPreparedBatch{
            std::move(prepared)});
    output.conditional_k2_to_k1_group_images_prepared = true;
    output.decision =
        ExactDirectMorseResidentK2K1ClosedCutPreparationDecision::
            complete_prepared_conditional_k2_to_k1_batch;
    return output;
  } catch (const std::bad_alloc&) {
    return reject(
        ExactDirectMorseResidentK2K1ClosedCutPreparationDecision::
            no_allocation_failed);
  } catch (const std::length_error&) {
    return reject(
        ExactDirectMorseResidentK2K1ClosedCutPreparationDecision::
            no_preparation_budget_exhausted);
  } catch (const std::exception&) {
    return reject(
        ExactDirectMorseResidentK2K1ClosedCutPreparationDecision::
            no_frozen_k2_group_image_rejected);
  }
}

ExactDirectMorseResidentK2K1ClosedCutCommitResult
ExactDirectMorseResidentK2K1ClosedCutBridge::commit(
    ExactDirectMorseResidentK2K1ClosedCutPreparedBatch&& ticket) noexcept {
  static_assert(std::is_nothrow_move_constructible_v<exact::ExactLevel>);
  static_assert(std::is_nothrow_move_assignable_v<exact::ExactLevel>);
  static_assert(std::is_nothrow_move_constructible_v<
                ExactDirectMorseResidentK2K1ClosedCutBatchRecord>);

  ExactDirectMorseResidentK2K1ClosedCutCommitResult output;
  auto prepared = std::move(ticket.impl_);
  if (prepared == nullptr || prepared->consumed ||
      !prepared->resident_ticket.has_value()) {
    output.ticket_consumed = true;
    output.decision =
        ExactDirectMorseResidentK2K1ClosedCutCommitDecision::
            no_ticket_already_consumed;
    return output;
  }
  prepared->consumed = true;
  output.ticket_consumed = true;
  if (!ready() || prepared->seal.get() != impl_->seal.get()) {
    output.decision =
        ExactDirectMorseResidentK2K1ClosedCutCommitDecision::
            no_foreign_ticket_rejected;
    return output;
  }

  const bool k2_vertical_batch = prepared->k2_record.has_value();
  output.resident_commit =
      impl_->resident.commit(std::move(*prepared->resident_ticket));
  prepared->resident_ticket.reset();
  if (!output.resident_commit.certified_committed_batch()) {
    output.no_vertical_state_mutated_on_resident_rejection = true;
    output.committed_resident_batch_cursor =
        impl_->committed_stamp.resident_batch_cursor;
    output.committed_k2_batch_count =
        impl_->committed_stamp.committed_k2_batch_count;
    output.committed_k2_group_count =
        impl_->committed_stamp.committed_k2_group_count;
    output.decision =
        ExactDirectMorseResidentK2K1ClosedCutCommitDecision::
            no_resident_commit_rejected_without_vertical_mutation;
    return output;
  }

  if (impl_->resident.batch_cursor() !=
          prepared->resident_identity.batch_cursor + 1U ||
      impl_->resident.epoch() != prepared->resident_identity.epoch + 1U ||
      impl_->resident.locator().snapshot_stamp().committed_batch_count !=
          impl_->resident.batch_cursor()) {
    std::terminate();
  }

  if (k2_vertical_batch) {
    auto& record = *prepared->k2_record;
    const auto& resident_groups = impl_->resident.group_records();
    if (prepared->resident_group_record_count_before > resident_groups.size() ||
        resident_groups.size() - prepared->resident_group_record_count_before !=
            record.group_images.size() ||
        impl_->committed_k2_batches.size() >=
            impl_->committed_k2_batches.capacity()) {
      std::terminate();
    }
    for (std::size_t group_index = 0U;
         group_index < record.group_images.size();
         ++group_index) {
      const auto& resident_group = resident_groups[
          prepared->resident_group_record_count_before + group_index];
      if (resident_group.batch_index != record.source_batch_index ||
          resident_group.owner_group_index != group_index ||
          resident_group.squared_level != record.squared_level ||
          resident_group.order != 2U ||
          resident_group.resultant_root_id == 0U) {
        std::terminate();
      }
      record.group_images[group_index].resident_resultant_root_id =
          resident_group.resultant_root_id;
    }
    record.resident_batch_committed = true;
    record.post_commit_publication_allocation_free = true;
    if (!record.certified_conditional_k2_to_k1_batch()) {
      std::terminate();
    }
    impl_->committed_k2_batches.emplace_back(std::move(record));
    impl_->committed_stamp.committed_k1_stamp =
        std::move(prepared->next_committed_k1_stamp);
    ++impl_->committed_stamp.committed_k2_batch_count;
    impl_->committed_stamp.committed_k2_group_count +=
        impl_->committed_k2_batches.back().group_images.size();
    output.vertical_state_mutated = true;
    output.post_resident_commit_publication_allocation_free = true;
    output.decision =
        ExactDirectMorseResidentK2K1ClosedCutCommitDecision::
            complete_committed_conditional_k2_to_k1_batch;
  } else {
    output.decision =
        ExactDirectMorseResidentK2K1ClosedCutCommitDecision::
            complete_committed_non_k2_transit_batch;
  }

  if (impl_->committed_stamp.resident_locator_instance_id == 0U) {
    impl_->committed_stamp.resident_locator_instance_id =
        prepared->resident_identity.locator_instance_id;
  } else if (impl_->committed_stamp.resident_locator_instance_id !=
             prepared->resident_identity.locator_instance_id) {
    std::terminate();
  }
  impl_->committed_stamp.resident_batch_cursor = impl_->resident.batch_cursor();
  impl_->committed_stamp.resident_epoch = impl_->resident.epoch();
  output.committed_resident_batch_cursor =
      impl_->committed_stamp.resident_batch_cursor;
  output.committed_k2_batch_count =
      impl_->committed_stamp.committed_k2_batch_count;
  output.committed_k2_group_count =
      impl_->committed_stamp.committed_k2_group_count;
  if (!impl_->structurally_ready()) {
    std::terminate();
  }
  return output;
}

ExactDirectMorseResidentK2K1ClosedCutTerminalTargetResult
ExactDirectMorseResidentK2K1ClosedCutBridge::
    seal_owned_k1_terminal_target_history() noexcept {
  ExactDirectMorseResidentK2K1ClosedCutTerminalTargetResult output;
  if (!ready()) {
    output.decision =
        ExactDirectMorseResidentK2K1ClosedCutTerminalTargetDecision::
            no_bridge_not_ready;
    return output;
  }
  if (impl_->terminal_k1_target_history.has_value()) {
    return *impl_->terminal_k1_target_history;
  }
  if (!impl_->resident.complete()) {
    output.decision =
        ExactDirectMorseResidentK2K1ClosedCutTerminalTargetDecision::
            no_resident_not_exhausted;
    return output;
  }
  output.resident_cursor_exhausted = true;
  try {
    const auto pre_stamp = impl_->k1.current_stamp();
    output.k1_session_instance_id = pre_stamp.session_instance_id;
    output.pre_k1_level_cursor = pre_stamp.committed_level_cursor;
    output.distinct_k1_level_count = impl_->k1.distinct_level_count();
    output.k1_source_forest_digest = impl_->k1.source_forest_digest();

    auto advance = impl_->k1.advance_closed_to_terminal();
    if (advance.decision !=
            ExactDirectK1BoruvkaClosedCutAdvanceDecision::
                complete_certified_monotone_closed_cut ||
        !impl_->k1.verify_advance_receipt(advance) ||
        !impl_->k1.verify_stamp(advance.post_stamp)) {
      output.decision =
          ExactDirectMorseResidentK2K1ClosedCutTerminalTargetDecision::
              no_owned_k1_terminal_advance_rejected;
      return output;
    }
    output.live_k1_advance_receipt_verified = true;
    output.terminal_k1_level_cursor =
        advance.post_stamp.committed_level_cursor;
    output.consumed_target_only_k1_level_count =
        advance.consumed_intermediate_level_count;
    output.terminal_k1_history_digest =
        advance.post_stamp.committed_history_digest;
    output.owned_k1_state_mutated = advance.state_mutated;
    output.every_remaining_k1_equal_level_batch_consumed =
        output.terminal_k1_level_cursor == output.distinct_k1_level_count &&
        output.consumed_target_only_k1_level_count ==
            output.terminal_k1_level_cursor - output.pre_k1_level_cursor;
    output.terminal_k1_cursor_complete = impl_->k1.terminal_complete();
    if (!output.every_remaining_k1_equal_level_batch_consumed ||
        !output.terminal_k1_cursor_complete) {
      output.decision =
          ExactDirectMorseResidentK2K1ClosedCutTerminalTargetDecision::
              no_owned_k1_terminal_advance_rejected;
      return output;
    }
    impl_->committed_stamp.committed_k1_stamp =
        std::move(advance.post_stamp);
    output.decision =
        ExactDirectMorseResidentK2K1ClosedCutTerminalTargetDecision::
            complete_certified_owned_k1_terminal_target_history;
    if (!output.certified_terminal_target_history() ||
        !impl_->structurally_ready()) {
      std::terminate();
    }
    impl_->terminal_k1_target_history.emplace(output);
    if (!impl_->structurally_ready()) {
      std::terminate();
    }
    return *impl_->terminal_k1_target_history;
  } catch (...) {
    output = {};
    output.decision =
        ExactDirectMorseResidentK2K1ClosedCutTerminalTargetDecision::
            no_owned_k1_terminal_advance_rejected;
    return output;
  }
}

bool ExactDirectMorseResidentK2K1ClosedCutInitialization::
    certified_ready_bridge() const noexcept {
  return schema_version ==
             direct_morse_resident_k2_k1_closed_cut_bridge_schema_version &&
         resident_live_session_consumed && k1_live_session_consumed &&
         process_local_bridge_capability_issued &&
         cloud_and_point_namespace_bound &&
         persistent_k2_batch_arena_preallocated &&
         !incidence_complete_reduction && !vertical_maps_complete &&
         !global_facet_coface_or_gamma_catalog_materialized &&
         !ordinary_or_higher_order_delaunay_materialized &&
         !pair_matrix_materialized && !public_status_claimed &&
         decision ==
             ExactDirectMorseResidentK2K1ClosedCutInitializationDecision::
                 complete_certified_bounded_conditional_bridge &&
         bridge.ready();
}

ExactDirectMorseResidentK2K1ClosedCutInitialization
initialize_exact_direct_morse_resident_k2_k1_closed_cut_bridge(
    ExactDirectMorseUnifiedResidentSession&& resident_session,
    ExactDirectK1BoruvkaClosedCutSession&& k1_session,
    const ExactDirectMorseResidentK2K1ClosedCutBridgeBudget& budget) {
  ExactDirectMorseResidentK2K1ClosedCutInitialization output;
  output.requested_budget = budget;
  if (!resident_session.certified_resident_session()) {
    output.decision =
        ExactDirectMorseResidentK2K1ClosedCutInitializationDecision::
            no_resident_session_rejected;
    return output;
  }
  if (!k1_session.ready()) {
    output.decision =
        ExactDirectMorseResidentK2K1ClosedCutInitializationDecision::
            no_k1_session_rejected;
    return output;
  }

  output.requirements.resident_batch_count =
      resident_session.plan().batches.size();
  for (const auto& batch : resident_session.plan().batches) {
    if (batch.order == 2U) {
      ++output.requirements.resident_k2_batch_count;
    }
  }
  if (output.requirements.resident_k2_batch_count >
          budget.maximum_committed_k2_batch_count ||
      budget.maximum_committed_k2_group_count == 0U ||
      budget.maximum_prepared_k2_group_count == 0U ||
      budget.maximum_group_coverage_point_reference_scan_count == 0U ||
      budget.maximum_group_point_scratch_count == 0U ||
      budget.maximum_distinct_group_point_count == 0U ||
      budget.maximum_singleton_root_query_count == 0U) {
    output.decision =
        ExactDirectMorseResidentK2K1ClosedCutInitializationDecision::
            no_budget_rejected;
    return output;
  }

  try {
    const auto k1_stamp = k1_session.current_stamp();
    const auto& plan = resident_session.plan();
    const auto resident_authority_id =
        resident_session.locator().snapshot_stamp().external_authority_id;
    if (resident_authority_id == 0U ||
        plan.point_count != k1_session.point_count() ||
        plan.source_pair_canonical_cloud_digest == contract::CanonicalId{} ||
        plan.source_higher_canonical_cloud_digest == contract::CanonicalId{} ||
        plan.source_pair_canonical_cloud_digest !=
            k1_stamp.canonical_cloud_digest) {
      output.decision =
          ExactDirectMorseResidentK2K1ClosedCutInitializationDecision::
              no_cloud_or_point_namespace_mismatch;
      return output;
    }
    output.canonical_cloud_digest = plan.source_pair_canonical_cloud_digest;
    output.resident_higher_canonical_cloud_digest =
        plan.source_higher_canonical_cloud_digest;
    output.cloud_and_point_namespace_bound = true;

    const std::uint64_t bridge_session_instance_id =
        allocate_bridge_session_instance_id();
    if (bridge_session_instance_id == 0U) {
      output.decision =
          ExactDirectMorseResidentK2K1ClosedCutInitializationDecision::
              no_session_instance_id_exhausted;
      return output;
    }
    auto impl =
        std::make_unique<ExactDirectMorseResidentK2K1ClosedCutBridge::Impl>();
    impl->budget = budget;
    impl->committed_k2_batches.reserve(
        budget.maximum_committed_k2_batch_count);
    output.persistent_k2_batch_arena_preallocated = true;
    impl->resident = std::move(resident_session);
    impl->k1 = std::move(k1_session);
    output.resident_live_session_consumed = true;
    output.k1_live_session_consumed = true;
    impl->seal = std::make_shared<const
        ExactDirectMorseResidentK2K1ClosedCutBridge::Impl::Seal>(
        ExactDirectMorseResidentK2K1ClosedCutBridge::Impl::Seal{
            bridge_session_instance_id,
            resident_authority_id,
            k1_stamp.session_instance_id,
            output.canonical_cloud_digest,
            output.resident_higher_canonical_cloud_digest});
    output.process_local_bridge_capability_issued = true;
    impl->committed_stamp = {
        direct_morse_resident_k2_k1_closed_cut_bridge_schema_version,
        bridge_session_instance_id,
        resident_authority_id,
        0U,
        impl->resident.batch_cursor(),
        impl->resident.epoch(),
        0U,
        0U,
        output.canonical_cloud_digest,
        output.resident_higher_canonical_cloud_digest,
        std::move(k1_stamp),
    };
    impl->initialized = true;
    if (!impl->structurally_ready()) {
      output.decision =
          ExactDirectMorseResidentK2K1ClosedCutInitializationDecision::
              no_cloud_or_point_namespace_mismatch;
      return output;
    }
    output.bridge = ExactDirectMorseResidentK2K1ClosedCutBridge{
        std::move(impl)};
    output.decision =
        ExactDirectMorseResidentK2K1ClosedCutInitializationDecision::
            complete_certified_bounded_conditional_bridge;
    return output;
  } catch (const std::bad_alloc&) {
    output.decision =
        ExactDirectMorseResidentK2K1ClosedCutInitializationDecision::
            no_allocation_failed;
    return output;
  } catch (const std::exception&) {
    output.decision =
        ExactDirectMorseResidentK2K1ClosedCutInitializationDecision::
            no_cloud_or_point_namespace_mismatch;
    return output;
  }
}

}  // namespace morsehgp3d::hierarchy
