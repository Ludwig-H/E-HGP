#include "morsehgp3d/hierarchy/direct_normalized_h0_sparse_forest_projection.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

static_assert(
    !std::is_copy_constructible_v<
        ExactDirectNormalizedH0ScientificWindowCapabilitySession> &&
    std::is_nothrow_move_constructible_v<
        ExactDirectNormalizedH0ScientificWindowCapabilitySession>);
static_assert(
    !std::is_copy_constructible_v<
        ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow> &&
    std::is_nothrow_move_constructible_v<
        ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow>);
static_assert(
    !std::is_copy_constructible_v<
        ExactDirectNormalizedH0ScientificSourceStamp> &&
    std::is_nothrow_move_constructible_v<
        ExactDirectNormalizedH0ScientificSourceStamp> &&
    !std::is_aggregate_v<ExactDirectNormalizedH0ScientificSourceStamp>);
static_assert(
    !std::is_copy_constructible_v<
        ExactDirectNormalizedH0RelativeFrozenIncidenceBatch> &&
    std::is_nothrow_move_constructible_v<
        ExactDirectNormalizedH0RelativeFrozenIncidenceBatch>);
static_assert(
    !std::is_copy_constructible_v<
        ExactDirectNormalizedH0SparseForestProjectionResult> &&
    std::is_nothrow_move_constructible_v<
        ExactDirectNormalizedH0SparseForestProjectionResult> &&
    !std::is_aggregate_v<
        ExactDirectNormalizedH0SparseForestProjectionResult>);

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(double x, double y) {
  return CertifiedPoint3::from_binary64(x, y, 0.0);
}

[[nodiscard]] ExactPairSupportStreamBudget unlimited_pair_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactHigherSupportStreamBudget unlimited_higher_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
  };
}

[[nodiscard]] ExactDirectSaddleArmSeedBudget unlimited_arm_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectClosedSaddleIncidenceBudget
unlimited_incidence_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectSparseFirstIncidenceBudget
generous_first_incidence_budget() {
  return {
      1024U,
      65536U,
      65536U,
      1048576U,
      65536U,
      1048576U,
      16777216U,
      65536U,
      65536U,
  };
}

[[nodiscard]] ExactDirectSparseGatewayCandidateBudget
generous_gateway_budget() {
  return {
      4096U,
      65536U,
      65536U,
      655360U,
      1048576U,
      1048576U,
      1048576U,
      8388608U,
      generous_first_incidence_budget(),
  };
}

[[nodiscard]] ExactDirectSparseGatewayCandidateScientificIdentityBudget
generous_gateway_identity_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
  };
}

[[nodiscard]] ExactDirectNormalizedH0SourcePlanBudget
generous_source_plan_budget() {
  return {
      4096U,
      65536U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      8388608U,
      generous_gateway_identity_budget(),
  };
}

[[nodiscard]] ExactDirectNormalizedH0ResidentAdapterBudget
generous_adapter_budget() {
  return {
      1048576U,
      1048576U,
      1048576U,
      8388608U,
      8388608U,
      67108864U,
      67108864U,
      1048576U,
      8388608U,
      8388608U,
      1048576U,
      268435456U,
  };
}

[[nodiscard]] ExactDirectFrozenUnifiedIncidenceBatchBudget
unlimited_frozen_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
  };
}

[[nodiscard]] ExactDirectMorseUnifiedResidentSessionBudget
generous_resident_budget() {
  ExactDirectMorseUnifiedResidentSessionBudget budget;
  budget.locator = {
      1024U,
      1024U,
      10240U,
      1024U,
      1024U,
      1024U,
      1024U,
      1024U,
      10240U,
      4097U,
      4097U,
  };
  budget.probe = {4097U, 1024U};
  budget.frozen_batch = unlimited_frozen_budget();
  budget.maximum_facet_resolution_count = 1024U;
  budget.maximum_prior_root_coverage_count = 1024U;
  budget.maximum_prior_root_coverage_point_reference_count = 10240U;
  budget.maximum_latent_carrier_coverage_count = 1024U;
  budget.maximum_latent_carrier_coverage_point_reference_count = 10240U;
  budget.maximum_fresh_facet_miniball_count = 1024U;
  budget.maximum_fresh_facet_miniball_support_enumeration_count = 1048576U;
  budget.maximum_normalized_facet_observation_scratch_count = 1024U;
  budget.maximum_normalized_strict_facet_scratch_count = 1024U;
  budget.maximum_normalized_strict_facet_simultaneous_scratch_entry_count =
      16384U;
  budget.maximum_resident_root_count = 1024U;
  budget.maximum_resident_root_point_reference_count = 10240U;
  budget.maximum_resident_component_latent_point_reference_count = 10240U;
  budget.maximum_group_record_count = 1024U;
  budget.maximum_group_child_reference_count = 1024U;
  budget.maximum_group_coverage_delta_point_reference_count = 10240U;
  budget.sparse_delta = {
      1024U,
      10240U,
      1024U,
      1024U,
      10240U,
      1024U,
      10240U,
      10240U,
      1U,
  };
  return budget;
}

[[nodiscard]] ExactDirectNormalizedH0ResidentBatchProviderBudget
provider_budget() {
  return {1024U, 1024U, 10240U, 1024U, 1024U, 10240U, 65536U};
}

[[nodiscard]] ExactDirectNormalizedH0ScientificWindowCapabilityBudget
capability_budget() {
  return {
      generous_adapter_budget(),
      1024U,
      1024U,
      {provider_budget(), 1U},
  };
}

[[nodiscard]] ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBudget
relative_batch_budget() {
  return {
      std::numeric_limits<std::size_t>::max(),
      std::numeric_limits<std::size_t>::max(), unlimited_frozen_budget()};
}

[[nodiscard]] ExactDirectNormalizedH0SparseForestProjectionBudget
unlimited_projection_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
  };
}

[[nodiscard]] ExactDirectSparseStableFacetForestBudget forest_budget(
    std::size_t stable_facet_token_count) {
  const std::size_t point_capacity =
      stable_facet_token_count <=
              std::numeric_limits<std::size_t>::max() / 10U
          ? stable_facet_token_count * 10U
          : std::numeric_limits<std::size_t>::max();
  return {
      stable_facet_token_count,
      point_capacity,
      point_capacity,
      64U,
      stable_facet_token_count,
      point_capacity,
      point_capacity,
      point_capacity,
      point_capacity,
      16U,
  };
}

struct RelativeBatchAuthority {
  std::vector<std::size_t> local_facet_indices;
  std::vector<ExactDirectNormalizedH0StableFacetResolution> resolutions;
  std::vector<ExactDirectFrozenUnifiedPriorRootCoverage> prior_coverages;
  std::vector<PointId> prior_coverage_points;
  std::vector<ExactDirectFrozenUnifiedLatentCarrierCoverage> latent_coverages;
  std::vector<PointId> latent_coverage_points;
};

[[nodiscard]] RelativeBatchAuthority latent_authority_for(
    const ExactDirectNormalizedH0ResidentOwnedBatchWindow& owned) {
  if (owned.local_plan.batches.size() != 1U ||
      owned.local_to_stable_facet_token_indices.size() !=
          owned.local_plan.facet_tokens.size()) {
    throw std::runtime_error(
        "the scientific window does not expose one bijective local plan");
  }
  const auto& batch = owned.local_plan.batches.front();
  std::vector<std::size_t> touched;
  for (std::size_t local = 0U;
       local < batch.coface_facet_reference_count;
       ++local) {
    touched.push_back(
        owned.local_plan
            .coface_facet_references
                [batch.coface_facet_reference_offset + local]
            .facet_token_index);
  }
  std::sort(touched.begin(), touched.end());
  touched.erase(std::unique(touched.begin(), touched.end()), touched.end());

  RelativeBatchAuthority authority;
  for (const std::size_t local_facet_index : touched) {
    if (local_facet_index >= owned.local_plan.facet_tokens.size()) {
      throw std::runtime_error(
          "the scientific window contains an out-of-range local facet");
    }
    const std::size_t stable_facet_index =
        owned.local_to_stable_facet_token_indices[local_facet_index];
    const ExactFrozenIncidenceTokenId token_id =
        10000U + stable_facet_index;
    authority.local_facet_indices.push_back(local_facet_index);
    authority.resolutions.push_back(
        {stable_facet_index,
         {ExactFrozenIncidenceTokenKind::latent_carrier, token_id},
         std::nullopt});
    const auto& key =
        owned.local_plan.facet_tokens[local_facet_index].facet_key;
    const std::size_t point_offset = authority.latent_coverage_points.size();
    authority.latent_coverage_points.insert(
        authority.latent_coverage_points.end(),
        key.point_ids.begin(),
        key.point_ids.begin() + static_cast<std::ptrdiff_t>(key.point_count));
    authority.latent_coverages.push_back(
        {token_id, point_offset, key.point_count});
  }
  return authority;
}

[[nodiscard]] bool carrier_births_are_disjoint(
    const ExactDirectNormalizedH0ResidentOwnedBatchWindow& owned,
    const RelativeBatchAuthority& authority) {
  if (owned.local_plan.batches.size() != 1U) {
    return false;
  }
  const auto& batch = owned.local_plan.batches.front();
  if (batch.direct_reference_offset > owned.local_plan.direct_references.size() ||
      batch.direct_reference_count >
          owned.local_plan.direct_references.size() -
              batch.direct_reference_offset) {
    return false;
  }
  std::vector<bool> direct_births(owned.local_plan.facet_tokens.size(), false);
  const std::size_t end =
      batch.direct_reference_offset + batch.direct_reference_count;
  for (std::size_t index = batch.direct_reference_offset; index < end;
       ++index) {
    const auto& reference = owned.local_plan.direct_references[index];
    if (reference.role == ExactDirectMorseH0Role::birth &&
        reference.direct_birth_facet_token_index.has_value() &&
        *reference.direct_birth_facet_token_index < direct_births.size()) {
      direct_births[*reference.direct_birth_facet_token_index] = true;
    }
  }
  return std::none_of(
      authority.local_facet_indices.begin(),
      authority.local_facet_indices.end(),
      [&](std::size_t local_index) {
        return local_index >= direct_births.size() || direct_births[local_index];
      });
}

[[nodiscard]] ExactDirectSparseStableFacetForest make_projection_forest(
    const ExactDirectNormalizedH0ResidentOwnedBatchWindow& owned,
    const RelativeBatchAuthority& authority,
    const ExactDirectNormalizedH0ScientificSourceStamp& source_stamp,
    bool preunion_first_two_carriers = false) {
  auto initialized = initialize_exact_direct_sparse_stable_facet_forest(
      {source_stamp.stable_facet_token_count(),
       source_stamp.source_identity_digest()},
      forest_budget(source_stamp.stable_facet_token_count()));
  if (!initialized.certified_initialized() ||
      !initialized.forest.has_value()) {
    throw std::runtime_error("the projection forest did not initialize");
  }
  auto forest = std::move(*initialized.forest);
  std::vector<ExactDirectSparseStableFacetInsertion> insertions;
  insertions.reserve(authority.local_facet_indices.size());
  for (const std::size_t local_index : authority.local_facet_indices) {
    if (local_index >= owned.local_plan.facet_tokens.size() ||
        local_index >= owned.local_to_stable_facet_token_indices.size()) {
      throw std::runtime_error("the projection authority names a bad facet");
    }
    insertions.push_back(
        {owned.local_to_stable_facet_token_indices[local_index],
         owned.local_plan.facet_tokens[local_index].facet_key});
  }
  std::vector<ExactDirectSparseStableFacetUnion> unions;
  if (preunion_first_two_carriers && insertions.size() >= 2U) {
    unions.push_back(
        {insertions[0].stable_source_facet_token_index,
         insertions[1].stable_source_facet_token_index});
  }
  auto prepared = forest.prepare_batch(insertions, unions);
  if (!prepared.certified_prepared() || !prepared.ticket.has_value()) {
    throw std::runtime_error("the projection carrier roots did not prepare");
  }
  const auto committed = forest.commit(std::move(*prepared.ticket));
  if (!committed.certified_commit()) {
    throw std::runtime_error("the projection carrier roots did not commit");
  }
  return forest;
}

[[nodiscard]] std::vector<
    ExactDirectNormalizedH0SparseForestCarrierAttachment>
projection_attachments(const RelativeBatchAuthority& authority) {
  std::vector<ExactDirectNormalizedH0SparseForestCarrierAttachment>
      attachments;
  attachments.reserve(authority.resolutions.size());
  for (const auto& resolution : authority.resolutions) {
    attachments.push_back(
        {resolution.token,
         resolution.stable_source_facet_token_index,
         resolution.prior_root_id});
  }
  return attachments;
}

[[nodiscard]] bool exercise_latent_sparse_forest_projection(
    const ExactDirectNormalizedH0ScientificSourceStamp& source_stamp,
    const ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow&
        scientific_window,
    const ExactDirectNormalizedH0RelativeFrozenIncidenceBatch& relative_batch,
    const RelativeBatchAuthority& authority) {
  const auto& owned = scientific_window.owned_window();
  if (authority.resolutions.empty() ||
      !carrier_births_are_disjoint(owned, authority)) {
    return false;
  }
  auto forest =
      make_projection_forest(owned, authority, source_stamp, false);
  const auto attachments = projection_attachments(authority);
  const auto pre_stamp = forest.current_stamp();

  const auto reject_without_mutation = [&](const auto& supplied,
                                           const std::string& message) {
    const std::size_t outstanding = forest.outstanding_ticket_count();
    const auto rejected =
        ExactDirectNormalizedH0SparseForestProjection::prepare(
            source_stamp,
            scientific_window,
            relative_batch,
            forest,
            supplied,
            unlimited_projection_budget());
    check(
        !rejected.has_forest_ticket() &&
            !rejected.certified_prepared_projection() &&
            forest.current_stamp() == pre_stamp &&
            forest.outstanding_ticket_count() == outstanding,
        message);
  };

  if (!attachments.empty()) {
    auto missing = attachments;
    missing.pop_back();
    reject_without_mutation(
        missing,
        "a missing external carrier attachment is rejected without forest mutation");

    auto extra = attachments;
    auto extra_token = extra.back().token;
    if (extra_token.token_id !=
        std::numeric_limits<ExactFrozenIncidenceTokenId>::max()) {
      ++extra_token.token_id;
      extra.push_back(
          {extra_token, extra.front().pre_batch_root_handle, std::nullopt});
      reject_without_mutation(
          extra,
          "an extra external carrier attachment is rejected without forest mutation");
    }

    auto equal_kind = attachments;
    equal_kind.front().token.kind = ExactFrozenIncidenceTokenKind::equal_facet;
    reject_without_mutation(
        equal_kind,
        "an equal-facet token cannot masquerade as an external carrier attachment");
  }
  if (attachments.size() >= 2U) {
    auto out_of_order = attachments;
    std::swap(out_of_order[0], out_of_order[1]);
    reject_without_mutation(
        out_of_order,
        "out-of-order external carrier attachments are rejected without forest mutation");

    auto duplicate_roots = attachments;
    duplicate_roots[1].pre_batch_root_handle =
        duplicate_roots[0].pre_batch_root_handle;
    reject_without_mutation(
        duplicate_roots,
        "two external carriers cannot claim the same canonical pre-batch forest root");

    auto mismatched_roots = attachments;
    std::swap(
        mismatched_roots[0].pre_batch_root_handle,
        mismatched_roots[1].pre_batch_root_handle);
    reject_without_mutation(
        mismatched_roots,
        "an observed carrier facet cannot be rebound to a different canonical root");

    auto non_root_forest =
        make_projection_forest(owned, authority, source_stamp, true);
    const auto non_root_pre_stamp = non_root_forest.current_stamp();
    const auto non_root =
        ExactDirectNormalizedH0SparseForestProjection::prepare(
            source_stamp,
            scientific_window,
            relative_batch,
            non_root_forest,
            attachments,
            unlimited_projection_budget());
    check(
        !non_root.has_forest_ticket() &&
            non_root.decision ==
                ExactDirectNormalizedH0SparseForestProjectionDecision::
                    contradiction_carrier_attachment_root_or_prior_root &&
            non_root_forest.current_stamp() == non_root_pre_stamp &&
            non_root_forest.outstanding_ticket_count() == 0U,
        "an attachment naming an observed non-root handle is rejected without mutation (decision=" +
            std::to_string(static_cast<unsigned>(non_root.decision)) + ")");
  }

  auto first = ExactDirectNormalizedH0SparseForestProjection::prepare(
      source_stamp,
      scientific_window,
      relative_batch,
      forest,
      attachments,
      unlimited_projection_budget());
  check(
      first.certified_prepared_projection() &&
          first.scientific_authority_id ==
              source_stamp.scientific_authority_id() &&
          first.manifest_digest == source_stamp.manifest_digest() &&
          first.source_identity_digest == source_stamp.source_identity_digest() &&
          first.batch_identity_digest == relative_batch.batch_identity_digest() &&
          first.external_carrier_attachment_digest !=
              morsehgp3d::contract::CanonicalId{} &&
          first.external_carrier_attachment_premise_digest_bound &&
          first.forest_pre_stamp == pre_stamp &&
          first.forest_pre_stamp_is_not_scientific_cursor_chain_or_root_ledger_alignment &&
          first.exact_relative_to_external_carrier_attachments &&
          !first.source_exactness_claimed && !first.vertical_maps_complete &&
          !first.durable_restart_supported && !first.public_status_claimed &&
          first.q_r_zero_group_count > 0U &&
          first.q_r_one_group_count == 0U &&
          first.q_r_multiple_group_count == 0U &&
          first.private_capability_check_count == 2U &&
          first.payload_full_revalidation_count == 0U &&
          forest.current_stamp() == pre_stamp &&
          forest.outstanding_ticket_count() == 1U,
      "the latent-only E5 quotient prepares an honest qR=0 forest ticket relative to opaque external attachments (decision=" +
          std::to_string(static_cast<unsigned>(first.decision)) + ")");
  if (!first.has_forest_ticket()) {
    return false;
  }
  const auto attested_manifest = first.manifest_digest;
  first.manifest_digest = {};
  check(
      !first.certified_prepared_projection(),
      "a public receipt mutation cannot preserve the private projection attestation");
  first.manifest_digest = attested_manifest;
  check(
      first.certified_prepared_projection(),
      "restoring every attested receipt field restores the still-private prepared projection");
  const auto attested_attachment_digest =
      first.external_carrier_attachment_digest;
  first.external_carrier_attachment_digest = {};
  check(
      !first.certified_prepared_projection(),
      "the external attachment premise digest is part of the private projection snapshot");
  first.external_carrier_attachment_digest = attested_attachment_digest;
  check(
      first.certified_prepared_projection(),
      "restoring the attachment premise digest restores the privately attested receipt");
  check(
      std::all_of(
          attachments.begin(),
          attachments.end(),
          [](const auto& attachment) {
            return attachment.token.token_id !=
                   attachment.pre_batch_root_handle;
          }),
      "typed carrier token ids remain opaque and are never equated with stable handles");

  const auto zero_budget_rejected =
      ExactDirectNormalizedH0SparseForestProjection::prepare(
          source_stamp,
          scientific_window,
          relative_batch,
          forest,
          attachments,
          {});
  check(
      !zero_budget_rejected.has_forest_ticket() &&
          zero_budget_rejected.decision ==
              ExactDirectNormalizedH0SparseForestProjectionDecision::
                  no_projection_budget_exhausted &&
          zero_budget_rejected.private_capability_check_count == 2U &&
          zero_budget_rejected.payload_full_revalidation_count == 0U &&
          forest.current_stamp() == pre_stamp &&
          forest.outstanding_ticket_count() == 1U,
      "a zero projection budget rejects after exactly two O(1) private capability checks and before any full payload revalidation");

  using BudgetMember =
      std::size_t ExactDirectNormalizedH0SparseForestProjectionBudget::*;
  const std::array<BudgetMember, 9U> budget_members{
      &ExactDirectNormalizedH0SparseForestProjectionBudget::
          maximum_window_facet_scan_count,
      &ExactDirectNormalizedH0SparseForestProjectionBudget::
          maximum_incidence_reference_scan_count,
      &ExactDirectNormalizedH0SparseForestProjectionBudget::
          maximum_direct_reference_scan_count,
      &ExactDirectNormalizedH0SparseForestProjectionBudget::
          maximum_group_token_scan_count,
      &ExactDirectNormalizedH0SparseForestProjectionBudget::
          maximum_carrier_attachment_count,
      &ExactDirectNormalizedH0SparseForestProjectionBudget::
          maximum_binding_union_count,
      &ExactDirectNormalizedH0SparseForestProjectionBudget::
          maximum_group_union_count,
      &ExactDirectNormalizedH0SparseForestProjectionBudget::
          maximum_canonical_union_count,
      &ExactDirectNormalizedH0SparseForestProjectionBudget::
          maximum_scratch_entry_count,
  };
  const std::array<std::size_t, 9U> required{
      first.required_window_facet_scan_count,
      first.required_incidence_reference_scan_count,
      first.required_direct_reference_scan_count,
      first.required_group_token_scan_count,
      first.required_carrier_attachment_count,
      first.required_binding_union_count,
      first.required_group_union_count,
      first.required_canonical_union_capacity,
      first.required_scratch_entry_capacity,
  };
  for (std::size_t index = 0U; index < budget_members.size(); ++index) {
    if (required[index] == 0U) {
      continue;
    }
    auto capped = unlimited_projection_budget();
    capped.*budget_members[index] = required[index] - 1U;
    const auto rejected =
        ExactDirectNormalizedH0SparseForestProjection::prepare(
            source_stamp,
            scientific_window,
            relative_batch,
            forest,
            attachments,
            capped);
    check(
        !rejected.has_forest_ticket() &&
            rejected.decision ==
                ExactDirectNormalizedH0SparseForestProjectionDecision::
                    no_projection_budget_exhausted &&
            forest.current_stamp() == pre_stamp &&
            forest.outstanding_ticket_count() == 1U,
        "each positive projection capacity rejects cap minus one without logical forest mutation");
  }

  auto sibling = ExactDirectNormalizedH0SparseForestProjection::prepare(
      source_stamp,
      scientific_window,
      relative_batch,
      forest,
      attachments,
      unlimited_projection_budget());
  check(
      sibling.certified_prepared_projection() &&
          forest.current_stamp() == pre_stamp &&
          forest.outstanding_ticket_count() == 2U,
      "two prepared projections remain siblings over one immutable forest pre-stamp");
  if (!sibling.has_forest_ticket()) {
    return false;
  }
  auto first_ticket = first.take_forest_ticket();
  auto sibling_ticket = sibling.take_forest_ticket();
  check(
      first_ticket.has_value() && sibling_ticket.has_value() &&
          !first.certified_prepared_projection() &&
          !sibling.certified_prepared_projection(),
      "extracting a private forest ticket invalidates its projection receipt");
  if (!first_ticket.has_value() || !sibling_ticket.has_value()) {
    return false;
  }
  const auto committed = forest.commit(std::move(*first_ticket));
  const auto stale = forest.commit(std::move(*sibling_ticket));
  check(
      committed.certified_commit() &&
          stale.decision ==
              ExactDirectSparseStableFacetForestCommitDecision::
                  no_stale_or_sibling_ticket_rejected &&
          stale.ticket_consumed && forest.outstanding_ticket_count() == 0U &&
          scientific_window.valid(),
      "the existing forest commit accepts one projection ticket and rejects its stale sibling without consuming the scientific window");
  for (const std::size_t stable_handle :
       owned.local_to_stable_facet_token_indices) {
    check(
        forest.lookup(stable_handle).certified_observed(),
        "a committed projection observes every stable facet of its scientific window");
  }
  return committed.certified_commit();
}

[[nodiscard]] RelativeBatchAuthority rooted_equal_authority_for(
    const ExactDirectNormalizedH0ResidentOwnedBatchWindow& owned,
    std::optional<std::size_t> extra_rooted_local_index = std::nullopt) {
  RelativeBatchAuthority authority;
  if (owned.local_plan.batches.size() != 1U ||
      owned.local_to_stable_facet_token_indices.size() !=
          owned.local_plan.facet_tokens.size()) {
    return authority;
  }
  const auto& batch = owned.local_plan.batches.front();
  if (batch.coface_facet_reference_offset >
          owned.local_plan.coface_facet_references.size() ||
      batch.coface_facet_reference_count >
          owned.local_plan.coface_facet_references.size() -
              batch.coface_facet_reference_offset) {
    return authority;
  }

  std::vector<std::size_t> parent(owned.local_plan.facet_tokens.size());
  for (std::size_t index = 0U; index < parent.size(); ++index) {
    parent[index] = index;
  }
  const auto find_root = [&](std::size_t start) {
    std::size_t cursor = start;
    while (cursor < parent.size() && parent[cursor] != cursor) {
      cursor = parent[cursor];
    }
    return cursor;
  };
  const auto unite = [&](std::size_t left, std::size_t right) {
    const std::size_t left_root = find_root(left);
    const std::size_t right_root = find_root(right);
    if (left_root < parent.size() && right_root < parent.size() &&
        left_root != right_root) {
      parent[std::max(left_root, right_root)] =
          std::min(left_root, right_root);
    }
  };

  const std::size_t coface_end =
      batch.coface_facet_reference_offset +
      batch.coface_facet_reference_count;
  std::optional<std::size_t> current_coface;
  std::optional<std::size_t> first_local;
  for (std::size_t index = batch.coface_facet_reference_offset;
       index < coface_end;
       ++index) {
    const auto& reference = owned.local_plan.coface_facet_references[index];
    if (reference.facet_token_index >= parent.size()) {
      return {};
    }
    authority.local_facet_indices.push_back(reference.facet_token_index);
    if (!current_coface.has_value() ||
        *current_coface != reference.source_star_coface_index) {
      current_coface = reference.source_star_coface_index;
      first_local = reference.facet_token_index;
    } else {
      unite(*first_local, reference.facet_token_index);
    }
  }
  std::sort(
      authority.local_facet_indices.begin(),
      authority.local_facet_indices.end());
  authority.local_facet_indices.erase(
      std::unique(
          authority.local_facet_indices.begin(),
          authority.local_facet_indices.end()),
      authority.local_facet_indices.end());
  if (authority.local_facet_indices.empty()) {
    return {};
  }

  std::vector<bool> direct_births(parent.size(), false);
  if (batch.direct_reference_offset > owned.local_plan.direct_references.size() ||
      batch.direct_reference_count >
          owned.local_plan.direct_references.size() -
              batch.direct_reference_offset) {
    return {};
  }
  const std::size_t direct_end =
      batch.direct_reference_offset + batch.direct_reference_count;
  for (std::size_t index = batch.direct_reference_offset; index < direct_end;
       ++index) {
    const auto& reference = owned.local_plan.direct_references[index];
    if (reference.role == ExactDirectMorseH0Role::birth &&
        reference.direct_birth_facet_token_index.has_value() &&
        *reference.direct_birth_facet_token_index < direct_births.size()) {
      direct_births[*reference.direct_birth_facet_token_index] = true;
    }
  }

  std::vector<std::optional<std::size_t>> representative(parent.size());
  for (const std::size_t local_index : authority.local_facet_indices) {
    const std::size_t component = find_root(local_index);
    if (component >= representative.size()) {
      return {};
    }
    if (!representative[component].has_value() &&
        !direct_births[local_index]) {
      representative[component] = local_index;
    }
  }
  for (const std::size_t local_index : authority.local_facet_indices) {
    if (!representative[find_root(local_index)].has_value()) {
      return {};
    }
  }
  if (extra_rooted_local_index.has_value()) {
    if (!std::binary_search(
            authority.local_facet_indices.begin(),
            authority.local_facet_indices.end(),
            *extra_rooted_local_index) ||
        direct_births[*extra_rooted_local_index] ||
        representative[find_root(*extra_rooted_local_index)] ==
            extra_rooted_local_index) {
      return {};
    }
  }

  std::vector<bool> equal_collision_used(parent.size(), false);
  for (const std::size_t local_index : authority.local_facet_indices) {
    const std::size_t component = find_root(local_index);
    const bool rooted = representative[component] == local_index ||
                        extra_rooted_local_index == local_index;
    const std::size_t stable_index =
        owned.local_to_stable_facet_token_indices[local_index];
    if (stable_index >
        static_cast<std::size_t>(
            std::numeric_limits<ExactFrozenIncidenceTokenId>::max() -
            UINT64_C(50000))) {
      return {};
    }
    ExactFrozenIncidenceTokenId token_id =
        UINT64_C(50000) +
        static_cast<ExactFrozenIncidenceTokenId>(stable_index);
    ExactFrozenIncidenceTokenKind kind =
        ExactFrozenIncidenceTokenKind::rooted_carrier;
    std::optional<ExactFrozenIncidencePriorRootId> prior_root_id;
    if (rooted) {
      prior_root_id =
          UINT64_C(70000) +
          static_cast<ExactFrozenIncidencePriorRootId>(stable_index);
    } else {
      kind = ExactFrozenIncidenceTokenKind::equal_facet;
      if (!equal_collision_used[component]) {
        const std::size_t representative_stable =
            owned.local_to_stable_facet_token_indices
                [*representative[component]];
        token_id =
            UINT64_C(50000) +
            static_cast<ExactFrozenIncidenceTokenId>(representative_stable);
        equal_collision_used[component] = true;
      } else {
        token_id =
            UINT64_C(90000) +
            static_cast<ExactFrozenIncidenceTokenId>(stable_index);
      }
    }
    authority.resolutions.push_back(
        {stable_index, {kind, token_id}, prior_root_id});
  }

  for (std::size_t resolution_index = 0U;
       resolution_index < authority.resolutions.size();
       ++resolution_index) {
    const auto& resolution = authority.resolutions[resolution_index];
    if (resolution.token.kind !=
        ExactFrozenIncidenceTokenKind::rooted_carrier) {
      continue;
    }
    const std::size_t local_index =
        authority.local_facet_indices[resolution_index];
    const std::size_t component = find_root(local_index);
    std::vector<PointId> coverage;
    for (const std::size_t candidate : authority.local_facet_indices) {
      if (find_root(candidate) != component) {
        continue;
      }
      const auto& key = owned.local_plan.facet_tokens[candidate].facet_key;
      coverage.insert(
          coverage.end(),
          key.point_ids.begin(),
          key.point_ids.begin() +
              static_cast<std::ptrdiff_t>(key.point_count));
    }
    std::sort(coverage.begin(), coverage.end());
    coverage.erase(std::unique(coverage.begin(), coverage.end()), coverage.end());
    const std::size_t point_offset = authority.prior_coverage_points.size();
    authority.prior_coverage_points.insert(
        authority.prior_coverage_points.end(),
        coverage.begin(),
        coverage.end());
    authority.prior_coverages.push_back(
        {*resolution.prior_root_id, point_offset, coverage.size()});
  }
  return authority;
}

struct RootedProjectionForest {
  ExactDirectSparseStableFacetForest forest;
  std::vector<ExactDirectNormalizedH0SparseForestCarrierAttachment>
      attachments;
  std::vector<ExactDirectSparseStableFacetHandle> carrier_stable_handles;
};

[[nodiscard]] std::optional<RootedProjectionForest>
make_rooted_projection_forest(
    const ExactDirectNormalizedH0ResidentOwnedBatchWindow& owned,
    const RelativeBatchAuthority& authority,
    const ExactDirectNormalizedH0ScientificSourceStamp& source_stamp) {
  std::vector<bool> handle_is_local(
      source_stamp.stable_facet_token_count(), false);
  for (const std::size_t handle :
       owned.local_to_stable_facet_token_indices) {
    if (handle >= handle_is_local.size()) {
      return std::nullopt;
    }
    handle_is_local[handle] = true;
  }
  std::vector<ExactDirectSparseStableFacetHandle> external_roots;
  for (std::size_t handle = 0U; handle < handle_is_local.size(); ++handle) {
    if (!handle_is_local[handle]) {
      external_roots.push_back(handle);
    }
  }

  std::size_t rooted_count = 0U;
  for (const auto& resolution : authority.resolutions) {
    if (resolution.token.kind ==
        ExactFrozenIncidenceTokenKind::rooted_carrier) {
      ++rooted_count;
    }
  }
  if (rooted_count == 0U || external_roots.size() < rooted_count) {
    return std::nullopt;
  }

  auto initialized = initialize_exact_direct_sparse_stable_facet_forest(
      {source_stamp.stable_facet_token_count(),
       source_stamp.source_identity_digest()},
      forest_budget(source_stamp.stable_facet_token_count()));
  if (!initialized.certified_initialized() ||
      !initialized.forest.has_value()) {
    return std::nullopt;
  }
  RootedProjectionForest result{std::move(*initialized.forest), {}, {}};
  std::vector<ExactDirectSparseStableFacetInsertion> root_insertions;
  root_insertions.reserve(rooted_count);
  std::size_t root_index = 0U;
  for (const auto& resolution : authority.resolutions) {
    if (resolution.token.kind !=
        ExactFrozenIncidenceTokenKind::rooted_carrier) {
      continue;
    }
    const auto stable = std::lower_bound(
        owned.local_to_stable_facet_token_indices.begin(),
        owned.local_to_stable_facet_token_indices.end(),
        resolution.stable_source_facet_token_index);
    if (stable == owned.local_to_stable_facet_token_indices.end() ||
        *stable != resolution.stable_source_facet_token_index) {
      return std::nullopt;
    }
    const auto root_handle = external_roots[root_index++];
    // External roots are a caller-supplied structure-only premise, not source
    // facets from this window.  Give them unique canonical keys so the
    // forest's positive full-key index never aliases a carrier's source key.
    const auto synthetic_index =
        static_cast<PointId>(root_index - 1U);
    const PointId synthetic_high =
        std::numeric_limits<PointId>::max() - UINT64_C(2) * synthetic_index;
    ExactDirectSparseFacetKey synthetic_root_key{};
    synthetic_root_key.point_ids[0] = synthetic_high - UINT64_C(1);
    synthetic_root_key.point_ids[1] = synthetic_high;
    synthetic_root_key.point_count = 2U;
    root_insertions.push_back({root_handle, synthetic_root_key});
    result.attachments.push_back(
        {resolution.token, root_handle, resolution.prior_root_id});
    result.carrier_stable_handles.push_back(
        resolution.stable_source_facet_token_index);
  }
  auto prepared = result.forest.prepare_batch(root_insertions, {});
  if (!prepared.certified_prepared() || !prepared.ticket.has_value()) {
    return std::nullopt;
  }
  const auto committed =
      result.forest.commit(std::move(*prepared.ticket));
  if (!committed.certified_commit()) {
    return std::nullopt;
  }
  return result;
}

struct RootedProjectionExercise {
  bool q_r_one{false};
  bool q_r_multiple{false};
  bool equal_facet{false};
  bool same_numeric_token_id_across_kinds{false};
  bool external_nonself_binding{false};
  bool attachment_digest_distinguishes_prior_root{false};
};

[[nodiscard]] RootedProjectionExercise exercise_rooted_projection_authority(
    const ExactDirectNormalizedH0ScientificSourceStamp& source_stamp,
    const ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow&
        scientific_window,
    const RelativeBatchAuthority& authority,
    bool require_multiple) {
  RootedProjectionExercise exercised;
  if (authority.resolutions.empty()) {
    return exercised;
  }
  auto relative =
      ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuilder::build(
          scientific_window,
          authority.resolutions,
          authority.prior_coverages,
          authority.prior_coverage_points,
          authority.latent_coverages,
          authority.latent_coverage_points,
          relative_batch_budget());
  if (!relative.certified_scientific_relative_build() ||
      !relative.batch.has_value()) {
    return exercised;
  }
  const auto& frozen = relative.batch->frozen_batch();
  if ((require_multiple && frozen.counters.q_r_multiple_group_count == 0U) ||
      (!require_multiple &&
       (frozen.counters.q_r_one_group_count == 0U ||
        frozen.counters.q_r_multiple_group_count != 0U))) {
    return exercised;
  }
  auto rooted_forest = make_rooted_projection_forest(
      scientific_window.owned_window(), authority, source_stamp);
  if (!rooted_forest.has_value()) {
    return exercised;
  }
  const auto pre_stamp = rooted_forest->forest.current_stamp();
  for (std::size_t index = 0U;
       index < rooted_forest->attachments.size();
       ++index) {
    check(
        rooted_forest->attachments[index].pre_batch_root_handle !=
                rooted_forest->carrier_stable_handles[index] &&
            rooted_forest->forest
                .lookup(rooted_forest->carrier_stable_handles[index])
                .certified_unobserved() &&
            rooted_forest->forest
                .lookup(rooted_forest->attachments[index]
                            .pre_batch_root_handle)
                .certified_observed(),
        "a rooted projection starts from an unobserved stable facet and a distinct observed external root");
  }

  if (!rooted_forest->attachments.empty()) {
    auto missing_prior = rooted_forest->attachments;
    missing_prior.front().prior_root_id.reset();
    const auto rejected_missing =
        ExactDirectNormalizedH0SparseForestProjection::prepare(
            source_stamp,
            scientific_window,
            *relative.batch,
            rooted_forest->forest,
            missing_prior,
            unlimited_projection_budget());
    auto mismatched_prior = rooted_forest->attachments;
    ++*mismatched_prior.front().prior_root_id;
    const auto rejected_mismatch =
        ExactDirectNormalizedH0SparseForestProjection::prepare(
            source_stamp,
            scientific_window,
            *relative.batch,
            rooted_forest->forest,
            mismatched_prior,
            unlimited_projection_budget());
    check(
        !rejected_missing.has_forest_ticket() &&
            !rejected_mismatch.has_forest_ticket() &&
            rejected_missing.decision ==
                ExactDirectNormalizedH0SparseForestProjectionDecision::
                    contradiction_carrier_attachment_root_or_prior_root &&
            rejected_mismatch.decision ==
                ExactDirectNormalizedH0SparseForestProjectionDecision::
                    contradiction_carrier_attachment_root_or_prior_root &&
            rooted_forest->forest.current_stamp() == pre_stamp &&
            rooted_forest->forest.outstanding_ticket_count() == 0U,
        "a rooted carrier requires the exact prior-root id and rejects absence or mismatch without mutation");
  }

  auto projection =
      ExactDirectNormalizedH0SparseForestProjection::prepare(
          source_stamp,
          scientific_window,
          *relative.batch,
          rooted_forest->forest,
          rooted_forest->attachments,
          unlimited_projection_budget());
  if (!projection.certified_prepared_projection()) {
    return exercised;
  }
  const auto attested_attachment_digest =
      projection.external_carrier_attachment_digest;
  projection.external_carrier_attachment_digest = {};
  check(
      !projection.certified_prepared_projection(),
      "a rooted attachment-premise digest mutation invalidates the private receipt snapshot");
  projection.external_carrier_attachment_digest = attested_attachment_digest;
  check(
      projection.certified_prepared_projection(),
      "restoring the rooted attachment-premise digest restores the private receipt snapshot");
  exercised.q_r_one = projection.q_r_one_group_count > 0U;
  exercised.q_r_multiple = projection.q_r_multiple_group_count > 0U;
  exercised.equal_facet = projection.equal_facet_count > 0U;
  exercised.external_nonself_binding =
      projection.canonical_union_request_count > 0U;

  for (const auto& rooted_binding : frozen.quotient.token_bindings) {
    if (rooted_binding.token.kind !=
        ExactFrozenIncidenceTokenKind::rooted_carrier) {
      continue;
    }
    exercised.same_numeric_token_id_across_kinds =
        exercised.same_numeric_token_id_across_kinds ||
        std::any_of(
            frozen.quotient.token_bindings.begin(),
            frozen.quotient.token_bindings.end(),
            [&](const auto& candidate) {
              return candidate.token.kind ==
                         ExactFrozenIncidenceTokenKind::equal_facet &&
                     candidate.token.token_id ==
                         rooted_binding.token.token_id;
            });
  }

  std::optional<ExactDirectSparseStableFacetForestPreparedBatch>
      alternative_ticket;
  auto alternative_authority = authority;
  auto alternative_attachments = rooted_forest->attachments;
  constexpr ExactFrozenIncidencePriorRootId prior_root_variant_offset =
      UINT64_C(1000000);
  bool alternative_prior_ids_fit = true;
  for (auto& resolution : alternative_authority.resolutions) {
    if (!resolution.prior_root_id.has_value()) {
      continue;
    }
    if (*resolution.prior_root_id >
        std::numeric_limits<ExactFrozenIncidencePriorRootId>::max() -
            prior_root_variant_offset) {
      alternative_prior_ids_fit = false;
      break;
    }
    *resolution.prior_root_id += prior_root_variant_offset;
  }
  for (auto& coverage : alternative_authority.prior_coverages) {
    if (coverage.prior_root_id >
        std::numeric_limits<ExactFrozenIncidencePriorRootId>::max() -
            prior_root_variant_offset) {
      alternative_prior_ids_fit = false;
      break;
    }
    coverage.prior_root_id += prior_root_variant_offset;
  }
  for (auto& attachment : alternative_attachments) {
    if (attachment.prior_root_id.has_value()) {
      *attachment.prior_root_id += prior_root_variant_offset;
    }
  }
  if (alternative_prior_ids_fit) {
    auto alternative_relative =
        ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuilder::build(
            scientific_window,
            alternative_authority.resolutions,
            alternative_authority.prior_coverages,
            alternative_authority.prior_coverage_points,
            alternative_authority.latent_coverages,
            alternative_authority.latent_coverage_points,
            relative_batch_budget());
    if (alternative_relative.certified_scientific_relative_build() &&
        alternative_relative.batch.has_value()) {
      auto alternative_projection =
          ExactDirectNormalizedH0SparseForestProjection::prepare(
              source_stamp,
              scientific_window,
              *alternative_relative.batch,
              rooted_forest->forest,
              alternative_attachments,
              unlimited_projection_budget());
      exercised.attachment_digest_distinguishes_prior_root =
          alternative_projection.certified_prepared_projection() &&
          alternative_projection.forest_prepared_batch_digest ==
              projection.forest_prepared_batch_digest &&
          alternative_projection.external_carrier_attachment_digest !=
              projection.external_carrier_attachment_digest;
      check(
          exercised.attachment_digest_distinguishes_prior_root,
          "two otherwise identical rooted projections retain the same forest batch digest but distinct external attachment digests when prior-root ids differ");
      alternative_ticket = alternative_projection.take_forest_ticket();
    }
  }

  auto ticket = projection.take_forest_ticket();
  if (!ticket.has_value()) {
    return {};
  }
  const auto committed =
      rooted_forest->forest.commit(std::move(*ticket));
  std::optional<ExactDirectSparseStableFacetForestCommitResult>
      stale_alternative;
  if (alternative_ticket.has_value()) {
    stale_alternative.emplace(
        rooted_forest->forest.commit(std::move(*alternative_ticket)));
  }
  check(
      committed.certified_commit() &&
          (!stale_alternative.has_value() ||
           stale_alternative->decision ==
               ExactDirectSparseStableFacetForestCommitDecision::
                   no_stale_or_sibling_ticket_rejected),
      "a rooted/equal projection commits only through the existing sparse forest commit");
  for (std::size_t index = 0U;
       index < rooted_forest->attachments.size();
       ++index) {
    const auto carrier = rooted_forest->forest.lookup(
        rooted_forest->carrier_stable_handles[index]);
    const auto external_root = rooted_forest->forest.lookup(
        rooted_forest->attachments[index].pre_batch_root_handle);
    check(
        carrier.certified_observed() && external_root.certified_observed() &&
            carrier.root_handle == external_root.root_handle,
        "a newly observed carrier stable facet and its distinct external pre-batch root resolve to the same post-batch component");
  }
  for (const auto& group : frozen.quotient.groups) {
    std::optional<ExactDirectSparseStableFacetHandle> group_root;
    bool every_group_token_resolved = true;
    for (std::size_t offset = 0U; offset < group.token_count; ++offset) {
      const auto& token =
          frozen.quotient.group_tokens[group.token_offset + offset];
      const auto resolution = std::find_if(
          authority.resolutions.begin(),
          authority.resolutions.end(),
          [&](const auto& candidate) { return candidate.token == token; });
      if (resolution == authority.resolutions.end()) {
        every_group_token_resolved = false;
        break;
      }
      const auto token_root = rooted_forest->forest.lookup(
          resolution->stable_source_facet_token_index);
      if (!token_root.certified_observed()) {
        every_group_token_resolved = false;
        break;
      }
      if (!group_root.has_value()) {
        group_root = token_root.root_handle;
      } else if (*group_root != token_root.root_handle) {
        every_group_token_resolved = false;
        break;
      }
    }
    check(
        every_group_token_resolved && group_root.has_value(),
        "all rooted/equal tokens of each frozen quotient group resolve to one post-batch forest root");
  }
  if (!committed.certified_commit()) {
    return {};
  }
  return exercised;
}

struct DirectSources {
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult event_journal;
  ExactDirectSaddleArmSeedBudget arm_budget;
  ExactDirectSaddleArmSeedJournalResult arm_journal;
  ExactDirectClosedSaddleIncidenceBudget incidence_budget;
  ExactDirectClosedSaddleIncidenceJournalResult incidence_journal;
};

[[nodiscard]] DirectSources direct_sources(
    const MortonLbvhIndex& index, const CanonicalPointCloud& cloud) {
  const ExactDirectSupportTerminalBudget terminal_budget{
      unlimited_pair_budget(), unlimited_higher_budget()};
  const auto pair = build_exact_pair_support_stream(
      index, cloud, 2U, terminal_budget.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, 2U, terminal_budget.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index, cloud, 2U, terminal_budget, pair, higher);
  auto event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  const auto arm_budget = unlimited_arm_budget();
  auto arm_journal = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, event_journal, arm_budget);
  const auto incidence_budget = unlimited_incidence_budget();
  auto incidence_journal =
      build_exact_direct_closed_saddle_incidence_journal(
          cloud,
          facade,
          event_journal,
          arm_budget,
          arm_journal,
          incidence_budget);
  return {
      std::move(facade),
      std::move(event_journal),
      arm_budget,
      std::move(arm_journal),
      incidence_budget,
      std::move(incidence_journal),
  };
}

struct E5Fixture {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  DirectSources source;
  ExactDirectSparseGatewayCandidateBudget gateway_budget;
  ExactDirectSparseGatewayCandidateJournalResult gateway;
  ExactDirectNormalizedH0SourcePlanBudget source_plan_budget;
  ExactDirectNormalizedH0SourcePlanResult source_plan;
  ExactDirectRankWindowSaturatedH0Authority rank_authority;
  ExactDirectNormalizedH0IncidenceReductionAuthority incidence_authority;
  ExactDirectSparseUnifiedLevelPlanResult compatibility_plan;
  ExactDirectNormalizedH0ResidentSourceManifest manifest;
};

[[nodiscard]] E5Fixture e5_fixture() {
  const std::array points{
      point(-2.0, -1.0),
      point(-2.0, 1.0),
      point(0.0, 0.0),
      point(3.0, 2.0),
      point(4.0, -1.0),
  };
  auto cloud = CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
  auto index = MortonLbvhIndex::build(cloud);
  auto source = direct_sources(index, cloud);
  const auto gateway_budget = generous_gateway_budget();
  auto gateway = build_exact_direct_sparse_gateway_candidate_journal(
      index,
      cloud,
      source.facade,
      source.event_journal,
      source.arm_budget,
      source.arm_journal,
      source.incidence_budget,
      source.incidence_journal,
      gateway_budget,
      LbvhTraversalOrder::near_first);
  const auto source_plan_budget = generous_source_plan_budget();
  auto source_plan = build_exact_direct_normalized_h0_source_plan(
      index,
      cloud,
      source.facade,
      source.event_journal,
      source.arm_budget,
      source.arm_journal,
      source.incidence_budget,
      source.incidence_journal,
      gateway_budget,
      LbvhTraversalOrder::near_first,
      gateway,
      source_plan_budget);
  auto rank_authority =
      build_exact_direct_rank_window_saturated_h0_authority(source.facade);
  auto incidence_authority =
      build_exact_direct_normalized_h0_incidence_reduction_authority(
          index,
          cloud,
          source.facade,
          source.event_journal,
          source.arm_budget,
          source.arm_journal,
          source.incidence_budget,
          source.incidence_journal,
          gateway_budget,
          LbvhTraversalOrder::near_first,
          gateway,
          source_plan_budget,
          source_plan,
          rank_authority);
  auto compatibility = initialize_exact_direct_normalized_h0_resident_session(
      index,
      cloud,
      source.facade,
      source.event_journal,
      source.arm_budget,
      source.arm_journal,
      source.incidence_budget,
      source.incidence_journal,
      gateway_budget,
      LbvhTraversalOrder::near_first,
      gateway,
      source_plan_budget,
      source_plan,
      generous_adapter_budget(),
      UINT64_C(0xE5C001),
      generous_resident_budget());
  if (!compatibility.certified_initialized_session() ||
      !compatibility.session.has_value()) {
    throw std::runtime_error(
        "the real E5 normalized compatibility session did not initialize");
  }
  auto compatibility_plan = compatibility.session->plan();
  auto manifest =
      build_exact_direct_normalized_h0_resident_source_manifest_from_compatibility_plan(
          compatibility_plan, incidence_authority);
  return {
      std::move(cloud),
      std::move(index),
      std::move(source),
      gateway_budget,
      std::move(gateway),
      source_plan_budget,
      std::move(source_plan),
      std::move(rank_authority),
      std::move(incidence_authority),
      std::move(compatibility_plan),
      std::move(manifest),
  };
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
      if (window.source_batch_index != index) {
        throw std::runtime_error(
            "the real E5 compatibility provider could not index a window");
      }
      chain_prefixes_.push_back(window.successor_chain_digest);
    }
  }

  bool corrupt_chain{false};

  [[nodiscard]] ExactDirectNormalizedH0ResidentBatchVisitDecision operator()(
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
    if (corrupt_chain) {
      window.source_chain_digest = {};
    }
    return consumer(window)
               ? ExactDirectNormalizedH0ResidentBatchVisitDecision::
                     complete_synchronous_visit
               : ExactDirectNormalizedH0ResidentBatchVisitDecision::
                     no_consumer_rejected;
  }

 private:
  const ExactDirectNormalizedH0ResidentSourceManifest* manifest_{};
  const ExactDirectSparseUnifiedLevelPlanResult* plan_{};
  std::vector<morsehgp3d::contract::CanonicalId> chain_prefixes_;
  ExactDirectNormalizedH0ResidentCompatibilityWindowScratch scratch_;
};

[[nodiscard]] ExactDirectNormalizedH0ScientificWindowCapabilityInitializationResult
initialize_capability(
    const E5Fixture& fixture,
    const ExactDirectNormalizedH0IncidenceReductionAuthority& authority,
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest,
    CompatibilityProvider& provider,
    std::uint64_t scientific_authority_id) {
  return initialize_exact_direct_normalized_h0_scientific_window_capability(
      fixture.index,
      fixture.cloud,
      fixture.source.facade,
      fixture.source.event_journal,
      fixture.source.arm_budget,
      fixture.source.arm_journal,
      fixture.source.incidence_budget,
      fixture.source.incidence_journal,
      fixture.gateway_budget,
      LbvhTraversalOrder::near_first,
      fixture.gateway,
      fixture.source_plan_budget,
      fixture.source_plan,
      fixture.rank_authority,
      authority,
      manifest,
      provider,
      scientific_authority_id,
      capability_budget());
}

void test_full_e5_stream_and_terminal_seal(const E5Fixture& fixture) {
  CompatibilityProvider provider{fixture.manifest, fixture.compatibility_plan};
  auto initialized = initialize_capability(
      fixture,
      fixture.incidence_authority,
      fixture.manifest,
      provider,
      UINT64_C(0xE5C101));
  check(
          initialized.certified_scientific_initialization() &&
          initialized.horizontal_incidence_authority_verification_count == 1U &&
          initialized.normalized_source_plan_verification_count == 2U &&
          initialized.provider_full_replay_count == 1U &&
          !initialized.global_compatibility_plan_retained &&
          !initialized.vertical_maps_complete &&
          !initialized.public_status_claimed,
      "the real E5 source mints one non-forgeable scientific window session after fresh recursive replay");
  if (!initialized.session.has_value()) {
    return;
  }
  auto& session = *initialized.session;
  const auto& initial_source_stamp = session.scientific_source_stamp();
  const auto* const source_stamp_address = &initial_source_stamp;
  const auto source_stamp_schema = initial_source_stamp.schema_version();
  const auto source_stamp_authority =
      initial_source_stamp.scientific_authority_id();
  const auto source_stamp_manifest = initial_source_stamp.manifest_digest();
  const auto source_stamp_identity =
      initial_source_stamp.source_identity_digest();
  const auto source_stamp_incidence =
      initial_source_stamp.horizontal_incidence_authority_digest();
  const auto source_stamp_facet_count =
      initial_source_stamp.stable_facet_token_count();
  const auto source_stamp_batch_count = initial_source_stamp.batch_count();
  const auto source_stamp_initial_chain =
      initial_source_stamp.initial_chain_digest();
  const auto source_stamp_final_chain =
      initial_source_stamp.final_chain_digest();
  const auto source_stamp_persists = [&]() {
    const auto& observed = session.scientific_source_stamp();
    return &observed == source_stamp_address &&
           observed.certified_scientific_source_stamp() &&
           observed.schema_version() == source_stamp_schema &&
           observed.scientific_authority_id() == source_stamp_authority &&
           observed.manifest_digest() == source_stamp_manifest &&
           observed.source_identity_digest() == source_stamp_identity &&
           observed.horizontal_incidence_authority_digest() ==
               source_stamp_incidence &&
           observed.stable_facet_token_count() == source_stamp_facet_count &&
           observed.batch_count() == source_stamp_batch_count &&
           observed.initial_chain_digest() == source_stamp_initial_chain &&
           observed.final_chain_digest() == source_stamp_final_chain &&
           !observed.vertical_maps_complete() &&
           !observed.durable_restart_supported() &&
           !observed.public_status_claimed();
  };
  check(
      fixture.manifest.batch_count == 9U &&
          fixture.manifest.batch_count ==
              fixture.compatibility_plan.batches.size() &&
          session.certified_scientific_window_stream() &&
          session.horizontal_incidence_source_bound() &&
          initial_source_stamp.certified_scientific_source_stamp() &&
          initial_source_stamp.schema_version() ==
              direct_normalized_h0_scientific_window_capability_schema_version &&
          initial_source_stamp.scientific_authority_id() ==
              UINT64_C(0xE5C101) &&
          initial_source_stamp.manifest_digest() ==
              fixture.manifest.manifest_digest &&
          initial_source_stamp.source_identity_digest() ==
              fixture.manifest.source_identity_digest &&
          initial_source_stamp.horizontal_incidence_authority_digest() ==
              fixture.manifest.horizontal_incidence_authority_digest &&
          initial_source_stamp.stable_facet_token_count() ==
              fixture.manifest.stable_facet_token_count &&
          initial_source_stamp.batch_count() == fixture.manifest.batch_count &&
          initial_source_stamp.initial_chain_digest() ==
              fixture.manifest.initial_batch_chain_digest &&
          initial_source_stamp.final_chain_digest() ==
              fixture.manifest.final_batch_chain_digest &&
          !initial_source_stamp.vertical_maps_complete() &&
          !initial_source_stamp.durable_restart_supported() &&
          !initial_source_stamp.public_status_claimed() &&
          !session.global_compatibility_plan_retained() &&
          session.provider_identity() == &provider,
      "the order-two E5 scientific session exposes one privately minted constant source stamp without retaining the global plan");

  std::size_t committed = 0U;
  bool relative_batch_checked = false;
  bool sparse_forest_projection_checked = false;
  RootedProjectionExercise rooted_projection_exercise;
  const auto merge_rooted_exercise =
      [&](const RootedProjectionExercise& observed) {
        rooted_projection_exercise.q_r_one =
            rooted_projection_exercise.q_r_one || observed.q_r_one;
        rooted_projection_exercise.q_r_multiple =
            rooted_projection_exercise.q_r_multiple || observed.q_r_multiple;
        rooted_projection_exercise.equal_facet =
            rooted_projection_exercise.equal_facet || observed.equal_facet;
        rooted_projection_exercise.same_numeric_token_id_across_kinds =
            rooted_projection_exercise.same_numeric_token_id_across_kinds ||
            observed.same_numeric_token_id_across_kinds;
        rooted_projection_exercise.external_nonself_binding =
            rooted_projection_exercise.external_nonself_binding ||
            observed.external_nonself_binding;
        rooted_projection_exercise
            .attachment_digest_distinguishes_prior_root =
            rooted_projection_exercise
                .attachment_digest_distinguishes_prior_root ||
            observed.attachment_digest_distinguishes_prior_root;
      };
  std::size_t relative_source_batch_index = 0U;
  std::vector<std::size_t> retained_local_to_stable;
  std::optional<ExactDirectNormalizedH0RelativeFrozenIncidenceBatch>
      retained_relative_batch;
  while (!session.complete()) {
    auto prepared = session.prepare_next();
    check(
        prepared.certified_scientific_preparation() &&
            prepared.ticket.has_value() &&
            prepared.ticket->owned_window().source_batch_index == committed &&
            prepared.horizontal_incidence_source_bound &&
            prepared.exact_manifest_and_chain_bound &&
            !prepared.global_compatibility_plan_retained,
        "every real E5 batch receives a window-local scientific capability");
    if (!prepared.ticket.has_value()) {
      return;
    }
    const auto& rooted_owned = prepared.ticket->owned_window();
    if (!rooted_projection_exercise.q_r_one ||
        !rooted_projection_exercise.q_r_multiple ||
        !rooted_projection_exercise.equal_facet ||
        !rooted_projection_exercise.same_numeric_token_id_across_kinds ||
        !rooted_projection_exercise.external_nonself_binding ||
        !rooted_projection_exercise
             .attachment_digest_distinguishes_prior_root) {
      const auto rooted_authority =
          rooted_equal_authority_for(rooted_owned);
      merge_rooted_exercise(exercise_rooted_projection_authority(
          initial_source_stamp,
          *prepared.ticket,
          rooted_authority,
          false));
      if (!rooted_projection_exercise.q_r_multiple) {
        for (std::size_t resolution_index = 0U;
             resolution_index < rooted_authority.resolutions.size();
             ++resolution_index) {
          if (rooted_authority.resolutions[resolution_index].token.kind !=
              ExactFrozenIncidenceTokenKind::equal_facet) {
            continue;
          }
          const auto multiple_authority = rooted_equal_authority_for(
              rooted_owned,
              rooted_authority.local_facet_indices[resolution_index]);
          merge_rooted_exercise(exercise_rooted_projection_authority(
              initial_source_stamp,
              *prepared.ticket,
              multiple_authority,
              true));
          if (rooted_projection_exercise.q_r_multiple) {
            break;
          }
        }
      }
    }
    if (!relative_batch_checked || !sparse_forest_projection_checked) {
      const auto& owned = prepared.ticket->owned_window();
      const auto authority = latent_authority_for(owned);
      const bool stable_mapping_is_non_identity = std::any_of(
          authority.local_facet_indices.begin(),
          authority.local_facet_indices.end(),
          [&](std::size_t local_facet_index) {
            return owned.local_to_stable_facet_token_indices
                       [local_facet_index] != local_facet_index;
          });
      if (committed > 0U && !authority.resolutions.empty() &&
          stable_mapping_is_non_identity) {
        auto relative =
            ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuilder::build(
                *prepared.ticket,
                authority.resolutions,
                authority.prior_coverages,
                authority.prior_coverage_points,
                authority.latent_coverages,
                authority.latent_coverage_points,
                relative_batch_budget());
        if (relative.certified_scientific_relative_build()) {
          check(
              relative.batch.has_value() &&
                  relative.batch->source_batch_index() == committed &&
                  owned.local_plan.batches.front().batch_index == committed &&
                  relative.batch->manifest_digest() ==
                      fixture.manifest.manifest_digest &&
                  std::equal(
                      relative.batch
                          ->local_to_stable_facet_token_indices()
                          .begin(),
                      relative.batch
                          ->local_to_stable_facet_token_indices()
                          .end(),
                      owned.local_to_stable_facet_token_indices.begin(),
                      owned.local_to_stable_facet_token_indices.end()) &&
                  relative.batch->frozen_batch()
                      .normalized_direct_source_authority &&
                  !relative.batch->frozen_batch()
                       .source_plan_freshly_verified &&
                  prepared.ticket->valid(),
              "a scientific window builds one exact frozen quotient/action batch through stable handles without consuming the window ticket");

          if (!sparse_forest_projection_checked &&
              relative.batch.has_value()) {
            sparse_forest_projection_checked =
                exercise_latent_sparse_forest_projection(
                    initial_source_stamp,
                    *prepared.ticket,
                    *relative.batch,
                    authority);
          }

          auto window_capped_budget = relative_batch_budget();
          window_capped_budget.maximum_window_facet_translation_count =
              owned.local_to_stable_facet_token_indices.size() - 1U;
          const auto window_capped =
              ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuilder::build(
                  *prepared.ticket,
                  authority.resolutions,
                  authority.prior_coverages,
                  authority.prior_coverage_points,
                  authority.latent_coverages,
                  authority.latent_coverage_points,
                  window_capped_budget);
          check(
              !window_capped.batch.has_value() &&
                  window_capped.decision ==
                      ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                          no_translation_budget_rejected &&
                  prepared.ticket->valid(),
              "the relative builder rejects a window-mapping cap minus one without consuming the scientific ticket");

          auto capped_budget = relative_batch_budget();
          capped_budget.maximum_stable_resolution_translation_count =
              authority.resolutions.size() - 1U;
          const auto capped =
              ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuilder::build(
                  *prepared.ticket,
                  authority.resolutions,
                  authority.prior_coverages,
                  authority.prior_coverage_points,
                  authority.latent_coverages,
                  authority.latent_coverage_points,
                  capped_budget);
          check(
              !capped.batch.has_value() &&
                  capped.decision ==
                      ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                          no_translation_budget_rejected &&
                  prepared.ticket->valid(),
              "the relative builder rejects a translation cap minus one without consuming the scientific ticket");

          auto duplicate_resolutions = authority.resolutions;
          duplicate_resolutions.push_back(authority.resolutions.front());
          const auto duplicate =
              ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuilder::build(
                  *prepared.ticket,
                  duplicate_resolutions,
                  authority.prior_coverages,
                  authority.prior_coverage_points,
                  authority.latent_coverages,
                  authority.latent_coverage_points,
                  relative_batch_budget());
          check(
              !duplicate.batch.has_value() &&
                  duplicate.decision ==
                      ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                          no_stable_resolution_order_rejected &&
                  prepared.ticket->valid(),
              "duplicate stable resolutions are rejected before frozen construction without consuming the scientific ticket");

          auto frozen_capped_budget = relative_batch_budget();
          frozen_capped_budget.frozen_batch.maximum_facet_resolution_count =
              authority.resolutions.size() - 1U;
          const auto frozen_capped =
              ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuilder::build(
                  *prepared.ticket,
                  authority.resolutions,
                  authority.prior_coverages,
                  authority.prior_coverage_points,
                  authority.latent_coverages,
                  authority.latent_coverage_points,
                  frozen_capped_budget);
          check(
              !frozen_capped.batch.has_value() &&
                  frozen_capped.decision ==
                      ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                          no_frozen_batch_rejected &&
                  prepared.ticket->valid(),
              "the frozen resolution cap minus one rejects atomically without consuming the scientific ticket");

          auto outside_resolutions = authority.resolutions;
          outside_resolutions.back().stable_source_facet_token_index =
              fixture.manifest.stable_facet_token_count;
          const auto outside =
              ExactDirectNormalizedH0RelativeFrozenIncidenceBatchBuilder::build(
                  *prepared.ticket,
                  outside_resolutions,
                  authority.prior_coverages,
                  authority.prior_coverage_points,
                  authority.latent_coverages,
                  authority.latent_coverage_points,
                  relative_batch_budget());
          check(
              !outside.batch.has_value() &&
                  outside.decision ==
                      ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                          no_stable_resolution_outside_window_rejected &&
                  prepared.ticket->valid(),
              "a stable handle outside the authenticated window is rejected without consuming the scientific ticket");
          relative_batch_checked =
              window_capped.decision ==
                  ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                      no_translation_budget_rejected &&
              capped.decision ==
                  ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                      no_translation_budget_rejected &&
              duplicate.decision ==
                  ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                      no_stable_resolution_order_rejected &&
              frozen_capped.decision ==
                  ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                      no_frozen_batch_rejected &&
              outside.decision ==
                  ExactDirectNormalizedH0RelativeFrozenIncidenceBatchDecision::
                      no_stable_resolution_outside_window_rejected;
          if (relative_batch_checked &&
              !retained_relative_batch.has_value() &&
              relative.batch.has_value()) {
            relative_source_batch_index = committed;
            retained_local_to_stable.assign(
                relative.batch->local_to_stable_facet_token_indices().begin(),
                relative.batch->local_to_stable_facet_token_indices().end());
            retained_relative_batch.emplace(std::move(*relative.batch));
          }
        }
      }
    }
    const auto committed_window = session.commit(std::move(*prepared.ticket));
    check(
        committed_window.certified_scientific_commit() &&
            committed_window.source_batch_index == committed &&
            committed_window.committed_cursor == committed + 1U &&
            committed_window.committed_epoch == committed + 1U &&
            !committed_window.resident_scientific_state_mutated &&
            !committed_window.vertical_maps_complete,
        "every real E5 window advances exactly one scientific prefix");
    ++committed;
  }
  check(
      rooted_projection_exercise.q_r_one,
      "the E5 rooted projection fixture exercises qR=1");
  check(
      rooted_projection_exercise.q_r_multiple,
      "the E5 rooted projection fixture exercises qR>1");
  check(
      rooted_projection_exercise.equal_facet,
      "the E5 rooted projection fixture exercises equal facets");
  check(
      rooted_projection_exercise.same_numeric_token_id_across_kinds,
      "the E5 rooted projection fixture keeps equal typed tokens distinct despite one shared numeric id");
  check(
      rooted_projection_exercise.external_nonself_binding,
      "the E5 rooted projection fixture exercises a distinct external root binding");
  check(
      rooted_projection_exercise.attachment_digest_distinguishes_prior_root,
      "the E5 rooted projection fixture distinguishes prior-root variants in the attachment digest");
  check(
      relative_batch_checked && relative_source_batch_index > 0U &&
          sparse_forest_projection_checked &&
          rooted_projection_exercise.q_r_one &&
          rooted_projection_exercise.q_r_multiple &&
          rooted_projection_exercise.equal_facet &&
          rooted_projection_exercise.same_numeric_token_id_across_kinds &&
          rooted_projection_exercise.external_nonself_binding &&
          rooted_projection_exercise
              .attachment_digest_distinguishes_prior_root &&
          retained_relative_batch.has_value() &&
          retained_relative_batch->valid() &&
          std::equal(
              retained_relative_batch->local_to_stable_facet_token_indices()
                  .begin(),
              retained_relative_batch->local_to_stable_facet_token_indices()
                  .end(),
              retained_local_to_stable.begin(),
              retained_local_to_stable.end()) &&
          committed == fixture.manifest.batch_count &&
          session.complete() &&
          source_stamp_persists() &&
          session.current_chain_digest() ==
              fixture.manifest.final_batch_chain_digest,
      "the qR=0/1/>1 projection seams, opaque typed tokens, external roots and prior-root attachment digest remain exact-relative while the scientific source stamp stays constant after all E5 commits");
  const auto sealed = session.seal();
  check(
      sealed.certified_scientific_seal() && sealed.newly_sealed &&
          sealed.seal->committed_batch_count == fixture.manifest.batch_count &&
          sealed.seal->manifest_digest == fixture.manifest.manifest_digest &&
          sealed.seal->final_chain_digest ==
              fixture.manifest.final_batch_chain_digest &&
          sealed.seal->every_committed_window_scientifically_bound &&
          !sealed.seal->resident_fold_executed &&
          !sealed.seal->vertical_maps_complete &&
          !sealed.seal->durable_restart_supported &&
          !sealed.seal->public_status_claimed && session.sealed() &&
          source_stamp_persists(),
      "the real E5 terminal seal preserves the privately certified horizontal, transient and non-public source stamp");
}

void test_forged_authority_is_rejected(const E5Fixture& fixture) {
  CompatibilityProvider provider{fixture.manifest, fixture.compatibility_plan};
  auto forged = fixture.incidence_authority;
  forged.vertical_maps_complete = true;
  const auto rejected = initialize_capability(
      fixture,
      forged,
      fixture.manifest,
      provider,
      UINT64_C(0xE5C201));
  check(
      !rejected.session.has_value() &&
          rejected.horizontal_incidence_authority_verification_count == 1U &&
          !rejected.horizontal_incidence_authority_freshly_verified &&
          rejected.decision ==
              ExactDirectNormalizedH0ScientificWindowCapabilityInitializationDecision::
                  no_horizontal_incidence_authority_rejected,
      "a public authority forged into a vertical claim cannot mint a scientific window capability");
}

void test_structural_plan_manifest_mutation_is_rejected(
    const E5Fixture& fixture) {
  auto mutated_plan = fixture.compatibility_plan;
  if (mutated_plan.direct_references.empty() ||
      mutated_plan.source_role_record_count < 2U) {
    check(false, "the real E5 compatibility plan exposes a mutation target");
    return;
  }
  auto& mutated_reference = mutated_plan.direct_references.front();
  mutated_reference.source_role_record_index =
      (mutated_reference.source_role_record_index + 1U) %
      mutated_plan.source_role_record_count;
  auto mutated_manifest =
      build_exact_direct_normalized_h0_resident_source_manifest_from_compatibility_plan(
          mutated_plan, fixture.incidence_authority);
  CompatibilityProvider provider{mutated_manifest, mutated_plan};
  const auto structural =
      verify_exact_direct_normalized_h0_resident_batch_provider(
          mutated_manifest, provider, provider_budget());
  check(
      mutated_manifest.certified() && structural.result_certified &&
          structural.every_window_freshly_recertified &&
          structural.final_chain_matches_manifest &&
          mutated_manifest != fixture.manifest,
      "the mutated E5 plan and manifest remain self-consistent under the structure-only verifier");
  const auto rejected = initialize_capability(
      fixture,
      fixture.incidence_authority,
      mutated_manifest,
      provider,
      UINT64_C(0xE5C301));
  check(
      !rejected.session.has_value() &&
          rejected.expected_manifest_freshly_reconstructed &&
          !rejected.observed_manifest_recursively_equal &&
          rejected.decision ==
              ExactDirectNormalizedH0ScientificWindowCapabilityInitializationDecision::
                  no_scientific_manifest_mismatch,
      "fresh exact E5 reconstruction rejects a structurally certified but scientifically mutated plan and manifest");
}

void test_foreign_and_replayed_tickets(const E5Fixture& fixture) {
  CompatibilityProvider first_provider{
      fixture.manifest, fixture.compatibility_plan};
  CompatibilityProvider second_provider{
      fixture.manifest, fixture.compatibility_plan};
  auto first = initialize_capability(
      fixture,
      fixture.incidence_authority,
      fixture.manifest,
      first_provider,
      UINT64_C(0xE5C401));
  auto second = initialize_capability(
      fixture,
      fixture.incidence_authority,
      fixture.manifest,
      second_provider,
      UINT64_C(0xE5C402));
  check(
      first.certified_scientific_initialization() &&
          second.certified_scientific_initialization() &&
          first.session->provider_identity() == &first_provider &&
          second.session->provider_identity() == &second_provider &&
          first.session->provider_identity() != second.session->provider_identity(),
      "two E5 capabilities retain distinct process-local provider authorities");
  if (!first.session.has_value() || !second.session.has_value()) {
    return;
  }
  const auto& first_stamp = first.session->scientific_source_stamp();
  const auto& second_stamp = second.session->scientific_source_stamp();
  check(
      first_stamp.certified_scientific_source_stamp() &&
          second_stamp.certified_scientific_source_stamp() &&
          first_stamp.scientific_authority_id() == UINT64_C(0xE5C401) &&
          second_stamp.scientific_authority_id() == UINT64_C(0xE5C402) &&
          first_stamp.scientific_authority_id() !=
              second_stamp.scientific_authority_id() &&
          first_stamp.manifest_digest() == second_stamp.manifest_digest() &&
          first_stamp.source_identity_digest() ==
              second_stamp.source_identity_digest() &&
          &first_stamp != &second_stamp,
      "two E5 sessions mint distinct non-forgeable authority stamps for the same exact source identity");
  auto foreign_ticket = first.session->prepare_next();
  check(
      foreign_ticket.certified_scientific_preparation(),
      "the first E5 provider prepares a ticket for the foreign-provider test");
  if (!foreign_ticket.ticket.has_value()) {
    return;
  }
  const auto foreign = second.session->commit(std::move(*foreign_ticket.ticket));
  check(
      foreign.ticket_consumed &&
          foreign.decision ==
              ExactDirectNormalizedH0ScientificWindowCapabilityCommitDecision::
                  no_foreign_or_stale_ticket_rejected &&
          first.session->batch_cursor() == 0U &&
          second.session->batch_cursor() == 0U,
      "a scientific ticket cannot cross E5 provider/session authority boundaries");

  auto replay_ticket = first.session->prepare_next();
  check(
      replay_ticket.certified_scientific_preparation(),
      "the original E5 session recovers after its foreign ticket was consumed");
  if (!replay_ticket.ticket.has_value()) {
    return;
  }
  const auto committed =
      first.session->commit(std::move(*replay_ticket.ticket));
  const auto replayed =
      first.session->commit(std::move(*replay_ticket.ticket));
  check(
      committed.certified_scientific_commit() &&
          !replayed.ticket_consumed &&
          replayed.decision ==
              ExactDirectNormalizedH0ScientificWindowCapabilityCommitDecision::
                  no_ticket_invalid_or_consumed &&
          first.session->batch_cursor() == 1U,
      "a consumed move-only E5 capability cannot be replayed");
}

void test_corrupt_provider_chain_is_rejected(const E5Fixture& fixture) {
  CompatibilityProvider provider{fixture.manifest, fixture.compatibility_plan};
  auto initialized = initialize_capability(
      fixture,
      fixture.incidence_authority,
      fixture.manifest,
      provider,
      UINT64_C(0xE5C501));
  check(
      initialized.certified_scientific_initialization(),
      "the clean E5 provider passes the mandatory full replay before corruption");
  if (!initialized.session.has_value()) {
    return;
  }
  provider.corrupt_chain = true;
  const auto rejected = initialized.session->prepare_next();
  check(
      !rejected.ticket.has_value() &&
          rejected.structural_decision ==
              ExactDirectNormalizedH0AuthenticatedWindowStreamPreparationDecision::
                  no_window_prefix_or_cursor_rejected &&
          rejected.decision ==
              ExactDirectNormalizedH0ScientificWindowCapabilityPreparationDecision::
                  no_underlying_preparation_rejected &&
          initialized.session->batch_cursor() == 0U &&
          initialized.session->epoch() == 0U &&
          initialized.session->current_chain_digest() ==
              fixture.manifest.initial_batch_chain_digest,
      "a provider corrupted after full replay cannot advance the scientific E5 prefix");
  provider.corrupt_chain = false;
  auto recovered = initialized.session->prepare_next();
  check(
      recovered.certified_scientific_preparation(),
      "the E5 session remains unmodified and can retry after a corrupt chain rejection");
}

void test_default_session_has_no_scientific_source_stamp() {
  ExactDirectNormalizedH0ScientificWindowCapabilitySession session;
  const auto& stamp = session.scientific_source_stamp();
  check(
      !stamp.certified_scientific_source_stamp() &&
          stamp.scientific_authority_id() == 0U &&
          stamp.stable_facet_token_count() == 0U &&
          stamp.batch_count() == 0U && !stamp.vertical_maps_complete() &&
          !stamp.durable_restart_supported() &&
          !stamp.public_status_claimed(),
      "a default session cannot forge a scientific source stamp");
}

void test_default_projection_result_is_not_an_attestation() {
  ExactDirectNormalizedH0SparseForestProjectionResult result;
  result.scientific_authority_id = 1U;
  result.scientific_source_stamp_bound = true;
  result.scientific_window_and_relative_batch_bound = true;
  result.manifest_source_identity_and_batch_bound = true;
  result.forest_pre_state_bound = true;
  result.carrier_attachments_canonical_exhaustive_and_rooted = true;
  result.exact_relative_to_external_carrier_attachments = true;
  result.decision =
      ExactDirectNormalizedH0SparseForestProjectionDecision::
          complete_external_attachment_relative_sparse_forest_preparation;
  check(
      !result.has_forest_ticket() &&
          !result.certified_prepared_projection(),
      "public flags and decisions cannot forge a default projection attestation or inject a forest ticket");
}

}  // namespace

int main() {
  try {
    test_default_session_has_no_scientific_source_stamp();
    test_default_projection_result_is_not_an_attestation();
    const E5Fixture fixture = e5_fixture();
    check(
        fixture.source_plan.certified_complete_candidate_source_plan() &&
            fixture.rank_authority.certified() &&
            fixture.incidence_authority
                .certified_horizontal_incidence_reduction() &&
            fixture.incidence_authority.incidence_complete_reduction_proved &&
            fixture.manifest.certified(),
        "the real E5 fixture supplies every exact scientific prerequisite");
    test_full_e5_stream_and_terminal_seal(fixture);
    test_forged_authority_is_rejected(fixture);
    test_structural_plan_manifest_mutation_is_rejected(fixture);
    test_foreign_and_replayed_tickets(fixture);
    test_corrupt_provider_chain_is_rejected(fixture);
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: unexpected exception: " << error.what() << '\n';
  }
  if (failures != 0) {
    std::cerr << failures
              << " direct normalized-H0 scientific-window capability test(s) failed\n";
    return 1;
  }
  std::cout
      << "direct normalized-H0 scientific-window capability tests passed\n";
  return 0;
}
