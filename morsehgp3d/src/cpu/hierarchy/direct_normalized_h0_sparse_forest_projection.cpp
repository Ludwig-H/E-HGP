#include "morsehgp3d/hierarchy/direct_normalized_h0_sparse_forest_projection.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::string_view external_carrier_attachment_digest_domain =
    "MorseHGP3D/phase15/direct-normalized-h0-sparse-forest-projection/"
    "external-carrier-attachments/v1/sha256/";

void append_u8(
    contract::CanonicalSha256Builder& builder, std::uint8_t value) {
  const std::array<std::uint8_t, 1U> bytes{value};
  builder.update(bytes);
}

void append_u64(
    contract::CanonicalSha256Builder& builder, std::uint64_t value) {
  std::array<std::uint8_t, 8U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    bytes[index] = static_cast<std::uint8_t>(
        value >> ((bytes.size() - 1U - index) * 8U));
  }
  builder.update(bytes);
}

void append_size(
    contract::CanonicalSha256Builder& builder, std::size_t value) {
  static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));
  append_u64(builder, static_cast<std::uint64_t>(value));
}

void append_id(
    contract::CanonicalSha256Builder& builder,
    const contract::CanonicalId& value) {
  builder.update(value.bytes());
}

[[nodiscard]] contract::CanonicalId compute_external_carrier_attachment_digest(
    const contract::CanonicalId& manifest_digest,
    const contract::CanonicalId& source_identity_digest,
    const contract::CanonicalId& batch_identity_digest,
    std::size_t source_batch_index,
    std::span<const ExactDirectNormalizedH0SparseForestCarrierAttachment>
        attachments) {
  contract::CanonicalSha256Builder builder;
  builder.update(external_carrier_attachment_digest_domain);
  append_id(builder, manifest_digest);
  append_id(builder, source_identity_digest);
  append_id(builder, batch_identity_digest);
  append_size(builder, source_batch_index);
  append_size(builder, attachments.size());
  for (const auto& attachment : attachments) {
    append_u8(builder, static_cast<std::uint8_t>(attachment.token.kind));
    append_u64(builder, attachment.token.token_id);
    append_size(builder, attachment.pre_batch_root_handle);
    append_u8(builder, attachment.prior_root_id.has_value()
                           ? std::uint8_t{1U}
                           : std::uint8_t{0U});
    if (attachment.prior_root_id.has_value()) {
      append_u64(builder, *attachment.prior_root_id);
    }
  }
  return builder.finalize();
}

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

[[nodiscard]] bool valid_slice(
    std::size_t offset,
    std::size_t count,
    std::size_t extent) noexcept {
  return offset <= extent && count <= extent - offset;
}

[[nodiscard]] bool token_less(
    const ExactFrozenIncidenceToken& left,
    const ExactFrozenIncidenceToken& right) noexcept {
  const auto left_kind = static_cast<std::uint8_t>(left.kind);
  const auto right_kind = static_cast<std::uint8_t>(right.kind);
  return left_kind < right_kind ||
         (left_kind == right_kind && left.token_id < right.token_id);
}

[[nodiscard]] bool carrier_kind(
    ExactFrozenIncidenceTokenKind kind) noexcept {
  return kind == ExactFrozenIncidenceTokenKind::rooted_carrier ||
         kind == ExactFrozenIncidenceTokenKind::latent_carrier;
}

struct FacetProjectionState {
  bool direct_birth{false};
  bool token_bound{false};
  ExactFrozenIncidenceToken token{};
};

struct TokenAnchor {
  ExactFrozenIncidenceToken token{};
  ExactDirectSparseStableFacetHandle handle{};
  bool handle_bound{false};
};

struct GroupAnchor {
  ExactDirectSparseStableFacetHandle handle{};
  std::size_t observed_token_count{};
  bool handle_bound{false};
};

[[nodiscard]] bool union_less(
    const ExactDirectSparseStableFacetUnion& left,
    const ExactDirectSparseStableFacetUnion& right) noexcept {
  return left.left_handle < right.left_handle ||
         (left.left_handle == right.left_handle &&
          left.right_handle < right.right_handle);
}

}  // namespace

struct ExactDirectNormalizedH0SparseForestProjectionResult::
    PrivateAttestation {
  ExactDirectNormalizedH0SparseForestProjectionReceipt receipt{};
};

static_assert(std::is_nothrow_copy_assignable_v<
              ExactDirectNormalizedH0SparseForestProjectionReceipt>);

ExactDirectNormalizedH0SparseForestProjectionResult::
    ExactDirectNormalizedH0SparseForestProjectionResult() noexcept = default;
ExactDirectNormalizedH0SparseForestProjectionResult::
    ~ExactDirectNormalizedH0SparseForestProjectionResult() = default;
ExactDirectNormalizedH0SparseForestProjectionResult::
    ExactDirectNormalizedH0SparseForestProjectionResult(
        ExactDirectNormalizedH0SparseForestProjectionResult&&) noexcept =
        default;
ExactDirectNormalizedH0SparseForestProjectionResult&
ExactDirectNormalizedH0SparseForestProjectionResult::operator=(
    ExactDirectNormalizedH0SparseForestProjectionResult&&) noexcept = default;

bool ExactDirectNormalizedH0SparseForestProjectionResult::has_forest_ticket()
    const noexcept {
  return forest_ticket_.has_value();
}

const ExactDirectSparseStableFacetForestPreparedBatch*
ExactDirectNormalizedH0SparseForestProjectionResult::forest_ticket()
    const noexcept {
  return forest_ticket_.has_value() ? &*forest_ticket_ : nullptr;
}

std::optional<ExactDirectSparseStableFacetForestPreparedBatch>
ExactDirectNormalizedH0SparseForestProjectionResult::take_forest_ticket()
    noexcept {
  std::optional<ExactDirectSparseStableFacetForestPreparedBatch> output;
  if (!forest_ticket_.has_value()) {
    return output;
  }
  output.emplace(std::move(*forest_ticket_));
  forest_ticket_.reset();
  private_attestation_.reset();
  return output;
}

bool ExactDirectNormalizedH0SparseForestProjectionResult::
    certified_prepared_projection() const noexcept {
  std::size_t observed_carrier_attachment_count = 0U;
  std::size_t observed_union_capacity = 0U;
  if (!checked_add(
          rooted_carrier_attachment_count,
          latent_carrier_attachment_count,
          observed_carrier_attachment_count) ||
      !checked_add(
          required_binding_union_count,
          required_group_union_count,
          observed_union_capacity)) {
    return false;
  }
  return schema_version ==
             direct_normalized_h0_sparse_forest_projection_schema_version &&
         scientific_authority_id != 0U &&
         private_attestation_ != nullptr &&
         static_cast<const ExactDirectNormalizedH0SparseForestProjectionReceipt&>(
             *this) == private_attestation_->receipt &&
         forest_ticket_.has_value() && forest_ticket_->valid() &&
         !forest_ticket_->consumed() &&
         forest_ticket_->pre_stamp() == forest_pre_stamp &&
         forest_ticket_->canonical_batch_digest() ==
             forest_prepared_batch_digest &&
         external_carrier_attachment_digest != contract::CanonicalId{} &&
         forest_ticket_->insertion_request_count() == insertion_request_count &&
         forest_ticket_->union_request_count() ==
             canonical_union_request_count &&
         observed_carrier_attachment_count ==
             required_carrier_attachment_count &&
         observed_union_capacity == required_canonical_union_capacity &&
         insertion_request_count == required_window_facet_scan_count &&
         canonical_union_request_count <= required_canonical_union_capacity &&
         equal_facet_count <= required_window_facet_scan_count &&
         direct_birth_facet_count <= required_window_facet_scan_count &&
         private_capability_check_count == 2U &&
         payload_full_revalidation_count == 0U &&
         required_window_facet_scan_count <=
             requested_budget.maximum_window_facet_scan_count &&
         required_incidence_reference_scan_count <=
             requested_budget.maximum_incidence_reference_scan_count &&
         required_direct_reference_scan_count <=
             requested_budget.maximum_direct_reference_scan_count &&
         required_group_token_scan_count <=
             requested_budget.maximum_group_token_scan_count &&
         required_carrier_attachment_count <=
             requested_budget.maximum_carrier_attachment_count &&
         required_binding_union_count <=
             requested_budget.maximum_binding_union_count &&
         required_group_union_count <=
             requested_budget.maximum_group_union_count &&
         required_canonical_union_capacity <=
             requested_budget.maximum_canonical_union_count &&
         required_scratch_entry_capacity <=
             requested_budget.maximum_scratch_entry_count &&
         source_identity_digest == forest_pre_stamp.source_identity_digest &&
         scientific_source_stamp_bound &&
         scientific_window_and_relative_batch_bound &&
         manifest_source_identity_and_batch_bound &&
         external_carrier_attachment_premise_digest_bound &&
         forest_pre_state_bound &&
         forest_pre_stamp_is_not_scientific_cursor_chain_or_root_ledger_alignment &&
         carrier_attachments_canonical_exhaustive_and_rooted &&
         opaque_token_ids_kept_distinct_from_stable_handles &&
         exact_relative_to_external_carrier_attachments &&
         all_fallible_projection_work_completed_before_commit &&
         !forest_logical_state_mutated &&
         forest_physical_capacity_may_have_changed &&
         !source_session_or_window_committed && !source_exactness_claimed &&
         !vertical_maps_complete && !durable_restart_supported &&
         !global_facet_coface_incidence_or_gamma_catalog_materialized &&
         !public_status_claimed &&
         forest_decision ==
             ExactDirectSparseStableFacetForestPreparationDecision::
                 complete_canonical_staging &&
         decision ==
             ExactDirectNormalizedH0SparseForestProjectionDecision::
                 complete_external_attachment_relative_sparse_forest_preparation &&
         scope ==
             ExactDirectNormalizedH0SparseForestProjectionScope::
                 exact_scientific_window_relative_to_publicly_supplied_external_carrier_attachments_only;
}

ExactDirectNormalizedH0SparseForestProjectionResult
ExactDirectNormalizedH0SparseForestProjection::prepare(
    const ExactDirectNormalizedH0ScientificSourceStamp& source_stamp,
    const ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow&
        scientific_window,
    const ExactDirectNormalizedH0RelativeFrozenIncidenceBatch& relative_batch,
    ExactDirectSparseStableFacetForest& forest,
    std::span<const ExactDirectNormalizedH0SparseForestCarrierAttachment>
        carrier_attachments,
    const ExactDirectNormalizedH0SparseForestProjectionBudget& budget)
    noexcept {
  ExactDirectNormalizedH0SparseForestProjectionResult output;
  output.requested_budget = budget;

  if (!source_stamp.certified_scientific_source_stamp()) {
    output.decision =
        ExactDirectNormalizedH0SparseForestProjectionDecision::
            no_scientific_source_stamp_rejected;
    return output;
  }
  output.scientific_authority_id = source_stamp.scientific_authority_id();
  output.manifest_digest = source_stamp.manifest_digest();
  output.source_identity_digest = source_stamp.source_identity_digest();
  output.horizontal_incidence_authority_digest =
      source_stamp.horizontal_incidence_authority_digest();

  ++output.private_capability_check_count;
  if (!scientific_window.privately_attests_projection_payload()) {
    output.decision =
        ExactDirectNormalizedH0SparseForestProjectionDecision::
            no_scientific_window_rejected;
    return output;
  }
  ++output.private_capability_check_count;
  if (!relative_batch.privately_attests_projection_payload() ||
      relative_batch.scientific_window_projection_capability_identity() !=
          scientific_window.projection_capability_identity()) {
    output.decision =
        ExactDirectNormalizedH0SparseForestProjectionDecision::
            no_relative_frozen_batch_rejected;
    return output;
  }

  const auto& owned = scientific_window.owned_window();
  const auto& local_plan = owned.local_plan;
  const auto& frozen = relative_batch.frozen_batch();
  const auto local_to_stable =
      relative_batch.local_to_stable_facet_token_indices();
  if (local_plan.batches.size() != 1U ||
      owned.manifest_digest != source_stamp.manifest_digest() ||
      relative_batch.manifest_digest() != source_stamp.manifest_digest() ||
      owned.batch_identity_digest != relative_batch.batch_identity_digest() ||
      owned.source_batch_index != relative_batch.source_batch_index() ||
      frozen.source_batch_index != owned.source_batch_index ||
      local_plan.batches.front().batch_index != owned.source_batch_index ||
      frozen.squared_level != local_plan.batches.front().squared_level ||
      frozen.order != local_plan.batches.front().order ||
      source_stamp.stable_facet_token_count() == 0U ||
      owned.source_batch_index >= source_stamp.batch_count() ||
      local_to_stable.size() !=
          owned.local_to_stable_facet_token_indices.size() ||
      local_to_stable.size() != local_plan.facet_tokens.size()) {
    output.decision =
        ExactDirectNormalizedH0SparseForestProjectionDecision::
            no_scientific_identity_or_batch_binding_rejected;
    return output;
  }
  output.source_chain_digest = owned.source_chain_digest;
  output.batch_identity_digest = owned.batch_identity_digest;
  output.successor_chain_digest = owned.successor_chain_digest;
  output.source_batch_index = owned.source_batch_index;

  const auto& plan_batch = local_plan.batches.front();
  output.required_window_facet_scan_count = local_to_stable.size();
  output.required_incidence_reference_scan_count =
      frozen.quotient_token_references.size();
  output.required_direct_reference_scan_count =
      plan_batch.direct_reference_count;
  output.required_group_token_scan_count =
      frozen.quotient.token_bindings.size();
  if (!checked_add(
          frozen.quotient.counters.distinct_rooted_carrier_count,
          frozen.quotient.counters.distinct_latent_carrier_count,
          output.required_carrier_attachment_count)) {
    output.decision =
        ExactDirectNormalizedH0SparseForestProjectionDecision::
            no_projection_capacity_overflow;
    return output;
  }
  output.equal_facet_count = frozen.equal_facet_binding_records.size();
  if (frozen.counters.facet_resolution_count < output.equal_facet_count ||
      frozen.quotient.group_tokens.size() !=
          output.required_group_token_scan_count ||
      output.required_group_token_scan_count < frozen.quotient.groups.size()) {
    output.decision =
        ExactDirectNormalizedH0SparseForestProjectionDecision::
            no_projection_capacity_overflow;
    return output;
  }
  output.required_binding_union_count =
      frozen.counters.facet_resolution_count - output.equal_facet_count;
  output.required_group_union_count =
      output.required_group_token_scan_count - frozen.quotient.groups.size();
  if (!checked_add(
          output.required_binding_union_count,
          output.required_group_union_count,
          output.required_canonical_union_capacity)) {
    output.decision =
        ExactDirectNormalizedH0SparseForestProjectionDecision::
            no_projection_capacity_overflow;
    return output;
  }
  std::size_t twice_window = 0U;
  std::size_t scratch_without_unions = 0U;
  if (!checked_add(
          output.required_window_facet_scan_count,
          output.required_window_facet_scan_count,
          twice_window) ||
      !checked_add(
          twice_window,
          frozen.quotient.token_bindings.size(),
          scratch_without_unions) ||
      !checked_add(
          scratch_without_unions,
          output.required_carrier_attachment_count,
          scratch_without_unions) ||
      !checked_add(
          scratch_without_unions,
          frozen.quotient.groups.size(),
          scratch_without_unions) ||
      !checked_add(
          scratch_without_unions,
          output.required_canonical_union_capacity,
          output.required_scratch_entry_capacity)) {
    output.decision =
        ExactDirectNormalizedH0SparseForestProjectionDecision::
            no_projection_capacity_overflow;
    return output;
  }

  // No payload traversal, equality scan, forest lookup or allocation occurs
  // before every projection-sized quantity has been checked and capped.
  // The two O(1) checks above authenticate const payloads already fully
  // validated by their respective minting builders.

  if (carrier_attachments.size() >
          budget.maximum_carrier_attachment_count ||
      output.required_window_facet_scan_count >
          budget.maximum_window_facet_scan_count ||
      output.required_incidence_reference_scan_count >
          budget.maximum_incidence_reference_scan_count ||
      output.required_direct_reference_scan_count >
          budget.maximum_direct_reference_scan_count ||
      output.required_group_token_scan_count >
          budget.maximum_group_token_scan_count ||
      output.required_carrier_attachment_count >
          budget.maximum_carrier_attachment_count ||
      output.required_binding_union_count >
          budget.maximum_binding_union_count ||
      output.required_group_union_count >
          budget.maximum_group_union_count ||
      output.required_canonical_union_capacity >
          budget.maximum_canonical_union_count ||
      output.required_scratch_entry_capacity >
          budget.maximum_scratch_entry_count) {
    output.decision =
        ExactDirectNormalizedH0SparseForestProjectionDecision::
            no_projection_budget_exhausted;
    return output;
  }

  if (!std::equal(
          local_to_stable.begin(),
          local_to_stable.end(),
          owned.local_to_stable_facet_token_indices.begin()) ||
      (owned.source_batch_index == 0U &&
       owned.source_chain_digest != source_stamp.initial_chain_digest()) ||
      (owned.source_batch_index + 1U == source_stamp.batch_count() &&
       owned.successor_chain_digest != source_stamp.final_chain_digest())) {
    output.decision =
        ExactDirectNormalizedH0SparseForestProjectionDecision::
            no_scientific_identity_or_batch_binding_rejected;
    return output;
  }

  if (!forest.certified_structure_only_forest()) {
    output.decision =
        ExactDirectNormalizedH0SparseForestProjectionDecision::
            no_sparse_forest_rejected;
    return output;
  }
  output.forest_pre_stamp = forest.current_stamp();
  if (output.forest_pre_stamp.source_identity_digest !=
          source_stamp.source_identity_digest() ||
      output.forest_pre_stamp.stable_facet_token_count !=
          source_stamp.stable_facet_token_count()) {
    output.decision =
        ExactDirectNormalizedH0SparseForestProjectionDecision::
            no_scientific_identity_or_batch_binding_rejected;
    return output;
  }

  if (carrier_attachments.size() !=
      output.required_carrier_attachment_count) {
    output.decision =
        ExactDirectNormalizedH0SparseForestProjectionDecision::
            no_carrier_attachment_exhaustiveness_rejected;
    return output;
  }
  for (std::size_t index = 0U; index < carrier_attachments.size(); ++index) {
    const auto& attachment = carrier_attachments[index];
    if (!carrier_kind(attachment.token.kind) ||
        (index != 0U &&
         !token_less(carrier_attachments[index - 1U].token,
                     attachment.token))) {
      output.decision =
          ExactDirectNormalizedH0SparseForestProjectionDecision::
              no_carrier_attachment_order_or_kind_rejected;
      return output;
    }
  }

  std::size_t attachment_cursor = 0U;
  for (const auto& binding : frozen.quotient.token_bindings) {
    if (!carrier_kind(binding.token.kind)) {
      continue;
    }
    if (attachment_cursor >= carrier_attachments.size() ||
        carrier_attachments[attachment_cursor].token != binding.token) {
      output.decision =
          ExactDirectNormalizedH0SparseForestProjectionDecision::
              no_carrier_attachment_exhaustiveness_rejected;
      return output;
    }
    ++attachment_cursor;
  }
  if (attachment_cursor != carrier_attachments.size()) {
    output.decision =
        ExactDirectNormalizedH0SparseForestProjectionDecision::
            no_carrier_attachment_exhaustiveness_rejected;
    return output;
  }

  for (const auto& attachment : carrier_attachments) {
    const bool rooted = attachment.token.kind ==
                        ExactFrozenIncidenceTokenKind::rooted_carrier;
    if (rooted != attachment.prior_root_id.has_value()) {
      output.decision =
          ExactDirectNormalizedH0SparseForestProjectionDecision::
              contradiction_carrier_attachment_root_or_prior_root;
      return output;
    }
    if (rooted) {
      const auto found = std::lower_bound(
          frozen.root_attachments.begin(),
          frozen.root_attachments.end(),
          attachment.token.token_id,
          [](const ExactFrozenIncidenceRootAttachment& candidate,
             ExactFrozenIncidenceTokenId token_id) {
            return candidate.rooted_carrier_token_id < token_id;
          });
      if (found == frozen.root_attachments.end() ||
          found->rooted_carrier_token_id != attachment.token.token_id ||
          found->prior_root_id != *attachment.prior_root_id) {
        output.decision =
            ExactDirectNormalizedH0SparseForestProjectionDecision::
                contradiction_carrier_attachment_root_or_prior_root;
        return output;
      }
      ++output.rooted_carrier_attachment_count;
    } else {
      ++output.latent_carrier_attachment_count;
    }
    const auto root_lookup = forest.lookup(attachment.pre_batch_root_handle);
    if (!root_lookup.certified_observed() ||
        root_lookup.root_handle != attachment.pre_batch_root_handle) {
      output.decision =
          ExactDirectNormalizedH0SparseForestProjectionDecision::
              contradiction_carrier_attachment_root_or_prior_root;
      return output;
    }
  }

  if (output.required_incidence_reference_scan_count !=
          frozen.incidence_facet_token_indices.size() ||
      output.required_incidence_reference_scan_count !=
          frozen.counters.token_reference_count ||
      output.required_direct_reference_scan_count !=
          frozen.counters.batch_direct_reference_scan_count ||
      !valid_slice(
          plan_batch.direct_reference_offset,
          plan_batch.direct_reference_count,
          local_plan.direct_references.size())) {
    output.decision =
        ExactDirectNormalizedH0SparseForestProjectionDecision::
            contradiction_window_facet_projection;
    return output;
  }

  try {
    output.external_carrier_attachment_digest =
        compute_external_carrier_attachment_digest(
            output.manifest_digest,
            output.source_identity_digest,
            output.batch_identity_digest,
            output.source_batch_index,
            carrier_attachments);
    if (output.external_carrier_attachment_digest == contract::CanonicalId{}) {
      output.decision =
          ExactDirectNormalizedH0SparseForestProjectionDecision::
              contradiction_carrier_attachment_root_or_prior_root;
      return output;
    }
    std::vector<FacetProjectionState> facet_states(local_to_stable.size());
    std::vector<ExactDirectSparseStableFacetInsertion> insertions;
    insertions.reserve(local_to_stable.size());
    std::vector<TokenAnchor> token_anchors;
    token_anchors.reserve(frozen.quotient.token_bindings.size());
    std::vector<ExactDirectSparseStableFacetHandle> carrier_roots;
    carrier_roots.reserve(output.required_carrier_attachment_count);
    std::vector<GroupAnchor> group_anchors(frozen.quotient.groups.size());
    std::vector<ExactDirectSparseStableFacetUnion> unions;
    unions.reserve(output.required_canonical_union_capacity);

    for (std::size_t local_index = 0U;
         local_index < local_plan.facet_tokens.size();
         ++local_index) {
      if (local_plan.facet_tokens[local_index].facet_token_index !=
              local_index ||
          local_to_stable[local_index] >=
              source_stamp.stable_facet_token_count() ||
          (local_index != 0U &&
           local_to_stable[local_index - 1U] >=
               local_to_stable[local_index])) {
        output.decision =
            ExactDirectNormalizedH0SparseForestProjectionDecision::
                contradiction_window_facet_projection;
        return output;
      }
    }

    const std::size_t direct_end =
        plan_batch.direct_reference_offset + plan_batch.direct_reference_count;
    for (std::size_t index = plan_batch.direct_reference_offset;
         index < direct_end;
         ++index) {
      const auto& reference = local_plan.direct_references[index];
      if (reference.role == ExactDirectMorseH0Role::birth) {
        if (!reference.direct_birth_facet_token_index.has_value() ||
            *reference.direct_birth_facet_token_index >= facet_states.size() ||
            facet_states[*reference.direct_birth_facet_token_index]
                .direct_birth) {
          output.decision =
              ExactDirectNormalizedH0SparseForestProjectionDecision::
                  contradiction_window_facet_projection;
          return output;
        }
        facet_states[*reference.direct_birth_facet_token_index].direct_birth =
            true;
        ++output.direct_birth_facet_count;
      } else if (reference.role != ExactDirectMorseH0Role::saddle ||
                 reference.direct_birth_facet_token_index.has_value()) {
        output.decision =
            ExactDirectNormalizedH0SparseForestProjectionDecision::
                contradiction_window_facet_projection;
        return output;
      }
    }
    if (output.direct_birth_facet_count !=
        frozen.counters.deferred_direct_birth_count) {
      output.decision =
          ExactDirectNormalizedH0SparseForestProjectionDecision::
              contradiction_window_facet_projection;
      return output;
    }

    for (std::size_t index = 0U;
         index < frozen.quotient_token_references.size();
         ++index) {
      const std::size_t local_index =
          frozen.incidence_facet_token_indices[index];
      if (local_index >= facet_states.size()) {
        output.decision =
            ExactDirectNormalizedH0SparseForestProjectionDecision::
                contradiction_window_facet_projection;
        return output;
      }
      auto& state = facet_states[local_index];
      if (state.token_bound &&
          state.token != frozen.quotient_token_references[index]) {
        output.decision =
            ExactDirectNormalizedH0SparseForestProjectionDecision::
                contradiction_window_facet_projection;
        return output;
      }
      state.token = frozen.quotient_token_references[index];
      state.token_bound = true;
    }

    std::size_t observed_equal_facet_count = 0U;
    for (const auto& record : frozen.equal_facet_binding_records) {
      if (record.facet_token_index >= facet_states.size()) {
        output.decision =
            ExactDirectNormalizedH0SparseForestProjectionDecision::
                contradiction_window_facet_projection;
        return output;
      }
      const auto& state = facet_states[record.facet_token_index];
      if (!state.token_bound ||
          state.token.kind != ExactFrozenIncidenceTokenKind::equal_facet ||
          state.token.token_id != record.equal_facet_token_id) {
        output.decision =
            ExactDirectNormalizedH0SparseForestProjectionDecision::
                contradiction_window_facet_projection;
        return output;
      }
      ++observed_equal_facet_count;
    }
    if (observed_equal_facet_count != output.equal_facet_count) {
      output.decision =
          ExactDirectNormalizedH0SparseForestProjectionDecision::
              contradiction_window_facet_projection;
      return output;
    }

    attachment_cursor = 0U;
    for (const auto& binding : frozen.quotient.token_bindings) {
      TokenAnchor anchor;
      anchor.token = binding.token;
      if (carrier_kind(binding.token.kind)) {
        if (attachment_cursor >= carrier_attachments.size() ||
            carrier_attachments[attachment_cursor].token != binding.token) {
          output.decision =
              ExactDirectNormalizedH0SparseForestProjectionDecision::
                  no_carrier_attachment_exhaustiveness_rejected;
          return output;
        }
        anchor.handle =
            carrier_attachments[attachment_cursor].pre_batch_root_handle;
        anchor.handle_bound = true;
        carrier_roots.push_back(anchor.handle);
        ++attachment_cursor;
      }
      token_anchors.push_back(anchor);
    }

    for (const auto& record : frozen.equal_facet_binding_records) {
      const ExactFrozenIncidenceToken token{
          ExactFrozenIncidenceTokenKind::equal_facet,
          record.equal_facet_token_id};
      const auto found = std::lower_bound(
          token_anchors.begin(),
          token_anchors.end(),
          token,
          [](const TokenAnchor& candidate,
             const ExactFrozenIncidenceToken& value) {
            return token_less(candidate.token, value);
          });
      if (found == token_anchors.end() || found->token != token ||
          found->handle_bound) {
        output.decision =
            ExactDirectNormalizedH0SparseForestProjectionDecision::
                contradiction_window_facet_projection;
        return output;
      }
      found->handle = local_to_stable[record.facet_token_index];
      found->handle_bound = true;
    }
    if (std::any_of(
            token_anchors.begin(),
            token_anchors.end(),
            [](const TokenAnchor& anchor) { return !anchor.handle_bound; })) {
      output.decision =
          ExactDirectNormalizedH0SparseForestProjectionDecision::
              contradiction_window_facet_projection;
      return output;
    }

    std::sort(carrier_roots.begin(), carrier_roots.end());
    if (std::adjacent_find(carrier_roots.begin(), carrier_roots.end()) !=
        carrier_roots.end()) {
      output.decision =
          ExactDirectNormalizedH0SparseForestProjectionDecision::
              contradiction_carrier_attachment_root_or_prior_root;
      return output;
    }

    const auto find_attachment = [&](const ExactFrozenIncidenceToken& token) {
      return std::lower_bound(
          carrier_attachments.begin(),
          carrier_attachments.end(),
          token,
          [](const ExactDirectNormalizedH0SparseForestCarrierAttachment&
                 candidate,
             const ExactFrozenIncidenceToken& value) {
            return token_less(candidate.token, value);
          });
    };
    const auto append_union = [&](ExactDirectSparseStableFacetHandle left,
                                  ExactDirectSparseStableFacetHandle right) {
      if (left == right) {
        return;
      }
      if (right < left) {
        std::swap(left, right);
      }
      unions.push_back({left, right});
    };

    std::size_t carrier_facet_count = 0U;
    std::size_t equal_state_count = 0U;
    for (std::size_t local_index = 0U;
         local_index < facet_states.size();
         ++local_index) {
      const auto stable_handle = local_to_stable[local_index];
      const auto& state = facet_states[local_index];
      const auto observed = forest.lookup(stable_handle);
      if ((!state.direct_birth && !state.token_bound) ||
          (state.direct_birth && observed.certified_observed())) {
        output.decision =
            ExactDirectNormalizedH0SparseForestProjectionDecision::
                contradiction_window_facet_projection;
        return output;
      }
      if (state.token_bound) {
        if (state.token.kind == ExactFrozenIncidenceTokenKind::equal_facet) {
          ++equal_state_count;
          if (observed.certified_observed()) {
            output.decision =
                ExactDirectNormalizedH0SparseForestProjectionDecision::
                    contradiction_window_facet_projection;
            return output;
          }
        } else if (carrier_kind(state.token.kind)) {
          if (state.direct_birth) {
            output.decision =
                ExactDirectNormalizedH0SparseForestProjectionDecision::
                    contradiction_window_facet_projection;
            return output;
          }
          const auto attachment = find_attachment(state.token);
          if (attachment == carrier_attachments.end() ||
              attachment->token != state.token) {
            output.decision =
                ExactDirectNormalizedH0SparseForestProjectionDecision::
                    no_carrier_attachment_exhaustiveness_rejected;
            return output;
          }
          if (observed.certified_observed() &&
              (observed.facet_key !=
                   local_plan.facet_tokens[local_index].facet_key ||
               observed.root_handle != attachment->pre_batch_root_handle)) {
            output.decision =
                ExactDirectNormalizedH0SparseForestProjectionDecision::
                    contradiction_carrier_attachment_root_or_prior_root;
            return output;
          }
          append_union(
              stable_handle, attachment->pre_batch_root_handle);
          ++carrier_facet_count;
        } else {
          output.decision =
              ExactDirectNormalizedH0SparseForestProjectionDecision::
                  contradiction_window_facet_projection;
          return output;
        }
      }
      if (!observed.certified_observed() && !observed.certified_unobserved()) {
        output.decision =
            ExactDirectNormalizedH0SparseForestProjectionDecision::
                contradiction_window_facet_projection;
        return output;
      }
      insertions.push_back(
          {stable_handle, local_plan.facet_tokens[local_index].facet_key});
    }
    if (carrier_facet_count != output.required_binding_union_count ||
        equal_state_count != output.equal_facet_count) {
      output.decision =
          ExactDirectNormalizedH0SparseForestProjectionDecision::
              contradiction_window_facet_projection;
      return output;
    }

    if (frozen.quotient.groups.size() != frozen.action_plan.groups.size()) {
      output.decision =
          ExactDirectNormalizedH0SparseForestProjectionDecision::
              contradiction_window_facet_projection;
      return output;
    }
    for (std::size_t group_index = 0U;
         group_index < frozen.quotient.groups.size();
         ++group_index) {
      const auto& group = frozen.quotient.groups[group_index];
      const auto& action = frozen.action_plan.groups[group_index];
      if (group.group_index != group_index ||
          action.group_index != group_index ||
          action.q_r != group.rooted_carrier_count ||
          group.token_count == 0U ||
          !valid_slice(
              group.token_offset,
              group.token_count,
              frozen.quotient.group_tokens.size())) {
        output.decision =
            ExactDirectNormalizedH0SparseForestProjectionDecision::
                contradiction_window_facet_projection;
        return output;
      }
    }

    // token_bindings are globally canonical and exhaustive.  One linear pass
    // binds the first anchor of each group and emits exactly D-G star edges;
    // no per-group lower_bound over the complete token table is required.
    if (token_anchors.size() != frozen.quotient.token_bindings.size()) {
      output.decision =
          ExactDirectNormalizedH0SparseForestProjectionDecision::
              contradiction_window_facet_projection;
      return output;
    }
    for (std::size_t token_index = 0U;
         token_index < frozen.quotient.token_bindings.size();
         ++token_index) {
      const auto& binding = frozen.quotient.token_bindings[token_index];
      const auto& anchor = token_anchors[token_index];
      if (binding.token != anchor.token ||
          binding.group_index >= group_anchors.size()) {
        output.decision =
            ExactDirectNormalizedH0SparseForestProjectionDecision::
                contradiction_window_facet_projection;
        return output;
      }
      auto& group_anchor = group_anchors[binding.group_index];
      const auto& group = frozen.quotient.groups[binding.group_index];
      if (group_anchor.observed_token_count >= group.token_count) {
        output.decision =
            ExactDirectNormalizedH0SparseForestProjectionDecision::
                contradiction_window_facet_projection;
        return output;
      }
      if (!group_anchor.handle_bound) {
        group_anchor.handle = anchor.handle;
        group_anchor.handle_bound = true;
      } else {
        append_union(group_anchor.handle, anchor.handle);
      }
      ++group_anchor.observed_token_count;
    }
    for (std::size_t group_index = 0U;
         group_index < group_anchors.size();
         ++group_index) {
      if (!group_anchors[group_index].handle_bound ||
          group_anchors[group_index].observed_token_count !=
              frozen.quotient.groups[group_index].token_count) {
        output.decision =
            ExactDirectNormalizedH0SparseForestProjectionDecision::
                contradiction_window_facet_projection;
        return output;
      }
    }

    if (unions.size() > output.required_canonical_union_capacity) {
      output.decision =
          ExactDirectNormalizedH0SparseForestProjectionDecision::
              contradiction_window_facet_projection;
      return output;
    }
    std::sort(unions.begin(), unions.end(), union_less);
    unions.erase(std::unique(unions.begin(), unions.end()), unions.end());
    output.insertion_request_count = insertions.size();
    output.canonical_union_request_count = unions.size();
    output.q_r_zero_group_count = frozen.counters.q_r_zero_group_count;
    output.q_r_one_group_count = frozen.counters.q_r_one_group_count;
    output.q_r_multiple_group_count = frozen.counters.q_r_multiple_group_count;

    if (forest.current_stamp() != output.forest_pre_stamp) {
      output.decision =
          ExactDirectNormalizedH0SparseForestProjectionDecision::
              no_sparse_forest_rejected;
      return output;
    }
    auto private_attestation =
        std::make_unique<
            ExactDirectNormalizedH0SparseForestProjectionResult::
                PrivateAttestation>();
    auto forest_preparation = forest.prepare_batch(insertions, unions);
    output.forest_decision = forest_preparation.decision;
    output.forest_physical_capacity_may_have_changed = true;
    if (!forest_preparation.certified_prepared() ||
        !forest_preparation.ticket.has_value() ||
        forest_preparation.pre_stamp != output.forest_pre_stamp ||
        forest_preparation.insertion_request_count != insertions.size() ||
        forest_preparation.union_request_count != unions.size() ||
        forest_preparation.forest_logical_state_mutated) {
      output.decision =
          ExactDirectNormalizedH0SparseForestProjectionDecision::
              no_sparse_forest_preparation_rejected;
      return output;
    }

    output.forest_prepared_batch_digest =
        forest_preparation.ticket->canonical_batch_digest();
    output.scientific_source_stamp_bound = true;
    output.scientific_window_and_relative_batch_bound = true;
    output.manifest_source_identity_and_batch_bound = true;
    output.external_carrier_attachment_premise_digest_bound = true;
    output.forest_pre_state_bound = true;
    output
        .forest_pre_stamp_is_not_scientific_cursor_chain_or_root_ledger_alignment =
        true;
    output.carrier_attachments_canonical_exhaustive_and_rooted = true;
    output.opaque_token_ids_kept_distinct_from_stable_handles = true;
    output.exact_relative_to_external_carrier_attachments = true;
    output.all_fallible_projection_work_completed_before_commit = true;
    output.forest_logical_state_mutated = false;
    output.source_session_or_window_committed = false;
    output.source_exactness_claimed = false;
    output.vertical_maps_complete = false;
    output.durable_restart_supported = false;
    output.global_facet_coface_incidence_or_gamma_catalog_materialized = false;
    output.public_status_claimed = false;
    output.scope =
        ExactDirectNormalizedH0SparseForestProjectionScope::
            exact_scientific_window_relative_to_publicly_supplied_external_carrier_attachments_only;
    output.decision =
        ExactDirectNormalizedH0SparseForestProjectionDecision::
            complete_external_attachment_relative_sparse_forest_preparation;
    output.forest_ticket_.emplace(
        std::move(*forest_preparation.ticket));
    private_attestation->receipt =
        static_cast<const ExactDirectNormalizedH0SparseForestProjectionReceipt&>(
            output);
    output.private_attestation_ = std::move(private_attestation);
    return output;
  } catch (const std::bad_alloc&) {
    output.decision =
        ExactDirectNormalizedH0SparseForestProjectionDecision::
            no_projection_allocation_failed;
    return output;
  } catch (const std::length_error&) {
    output.decision =
        ExactDirectNormalizedH0SparseForestProjectionDecision::
            no_projection_capacity_overflow;
    return output;
  } catch (...) {
    output.decision =
        ExactDirectNormalizedH0SparseForestProjectionDecision::
            contradiction_window_facet_projection;
    return output;
  }
}

}  // namespace morsehgp3d::hierarchy
