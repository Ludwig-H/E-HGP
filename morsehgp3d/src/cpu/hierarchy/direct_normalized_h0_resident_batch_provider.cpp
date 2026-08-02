#include "morsehgp3d/hierarchy/direct_normalized_h0_resident_batch_provider.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::string_view authority_domain =
    "MorseHGP3D/phase15/normalized-resident-provider/authority/v1/sha256/";
constexpr std::string_view source_identity_domain =
    "MorseHGP3D/phase15/normalized-resident-provider/source/v1/sha256/";
constexpr std::string_view initial_chain_domain =
    "MorseHGP3D/phase15/normalized-resident-provider/initial/v1/sha256/";
constexpr std::string_view batch_identity_domain =
    "MorseHGP3D/phase15/normalized-resident-provider/batch/v1/sha256/";
constexpr std::string_view chain_step_domain =
    "MorseHGP3D/phase15/normalized-resident-provider/chain/v1/sha256/";
constexpr std::string_view manifest_domain =
    "MorseHGP3D/phase15/normalized-resident-provider/manifest/v1/sha256/";

[[nodiscard]] bool checked_add(
    std::size_t left, std::size_t right, std::size_t& result) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  result = left + right;
  return true;
}

[[nodiscard]] bool checked_multiply(
    std::size_t left, std::size_t right, std::size_t& result) noexcept {
  if (right != 0U &&
      left > std::numeric_limits<std::size_t>::max() / right) {
    return false;
  }
  result = left * right;
  return true;
}

[[nodiscard]] bool checked_increment(std::size_t& value) noexcept {
  return checked_add(value, 1U, value);
}

void append_u8(
    contract::CanonicalSha256Builder& builder, std::uint8_t value) {
  const std::array<std::uint8_t, 1U> bytes{value};
  builder.update(bytes);
}

void append_u32(
    contract::CanonicalSha256Builder& builder, std::uint32_t value) {
  std::array<std::uint8_t, 4U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    bytes[index] = static_cast<std::uint8_t>(
        value >> ((bytes.size() - index - 1U) * 8U));
  }
  builder.update(bytes);
}

void append_u64(
    contract::CanonicalSha256Builder& builder, std::uint64_t value) {
  std::array<std::uint8_t, 8U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    bytes[index] = static_cast<std::uint8_t>(
        value >> ((bytes.size() - index - 1U) * 8U));
  }
  builder.update(bytes);
}

void append_size(
    contract::CanonicalSha256Builder& builder, std::size_t value) {
  append_u64(builder, static_cast<std::uint64_t>(value));
}

void append_bool(
    contract::CanonicalSha256Builder& builder, bool value) {
  append_u8(builder, value ? std::uint8_t{1U} : std::uint8_t{0U});
}

void append_id(
    contract::CanonicalSha256Builder& builder,
    const contract::CanonicalId& value) {
  builder.update(value.bytes());
}

void append_text(
    contract::CanonicalSha256Builder& builder, std::string_view value) {
  append_size(builder, value.size());
  builder.update(value);
}

void append_level(
    contract::CanonicalSha256Builder& builder,
    const exact::ExactLevel& level) {
  append_text(builder, level.canonical_key());
}

void append_optional_size(
    contract::CanonicalSha256Builder& builder,
    const std::optional<std::size_t>& value) {
  append_bool(builder, value.has_value());
  if (value.has_value()) {
    append_size(builder, *value);
  }
}

[[nodiscard]] bool facet_key_less(
    const ExactDirectSparseFacetKey& left,
    const ExactDirectSparseFacetKey& right) noexcept {
  if (left.point_count != right.point_count) {
    return left.point_count < right.point_count;
  }
  return std::lexicographical_compare(
      left.point_ids.begin(),
      left.point_ids.begin() + static_cast<std::ptrdiff_t>(left.point_count),
      right.point_ids.begin(),
      right.point_ids.begin() +
          static_cast<std::ptrdiff_t>(right.point_count));
}

[[nodiscard]] bool canonical_facet_key(
    const ExactDirectSparseFacetKey& key,
    std::size_t point_count,
    std::size_t expected_order = 0U) noexcept {
  if (key.point_count < 2U || key.point_count > key.point_ids.size() ||
      key.point_count > point_count ||
      (expected_order != 0U && key.point_count != expected_order)) {
    return false;
  }
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    if (static_cast<std::size_t>(key.point_ids[index]) >= point_count ||
        (index != 0U && key.point_ids[index - 1U] >= key.point_ids[index])) {
      return false;
    }
  }
  return std::all_of(
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(key.point_count),
      key.point_ids.end(),
      [](spatial::PointId point_id) { return point_id == 0U; });
}

void append_facet_key(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparseFacetKey& key) {
  append_size(builder, key.point_count);
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    append_u64(builder, static_cast<std::uint64_t>(key.point_ids[index]));
  }
}

void append_direct_reference(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparseUnifiedLevelPlanDirectReference& reference) {
  append_size(builder, reference.direct_reference_index);
  append_size(builder, reference.source_role_record_index);
  append_size(builder, reference.source_event_projection_index);
  append_u8(builder, static_cast<std::uint8_t>(reference.role));
  append_optional_size(builder, reference.source_incidence_family_index);
  append_optional_size(builder, reference.source_star_direct_coface_index);
  append_optional_size(builder, reference.direct_birth_facet_token_index);
}

void append_residual_reference(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparseUnifiedLevelPlanResidualReference& reference) {
  append_size(builder, reference.residual_reference_index);
  append_size(builder, reference.source_star_coface_index);
}

void append_coface_facet_reference(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparseUnifiedLevelPlanCofaceFacetReference& reference) {
  append_size(builder, reference.coface_facet_reference_index);
  append_size(builder, reference.source_star_coface_index);
  append_size(builder, reference.removed_union_point_index);
  append_u64(builder, static_cast<std::uint64_t>(reference.removed_point_id));
  append_size(builder, reference.facet_token_index);
}

[[nodiscard]] contract::CanonicalId compute_authority_digest(
    const ExactDirectNormalizedH0IncidenceReductionAuthority& authority) {
  contract::CanonicalSha256Builder builder;
  builder.update(authority_domain);
  append_u32(builder, authority.schema_version);
  for (const std::size_t value : {
           authority.point_count,
           authority.requested_maximum_order,
           authority.effective_maximum_order,
           authority.maximum_relevant_closed_rank,
           authority.source_direct_event_count,
           authority.source_core_facet_count,
           authority.source_gateway_candidate_count,
           authority.normalized_coface_count,
           authority.normalized_batch_count}) {
    append_size(builder, value);
  }
  append_id(builder, authority.source_pair_canonical_cloud_digest);
  append_id(builder, authority.source_higher_canonical_cloud_digest);
  append_id(builder, authority.source_pair_semantic_digest);
  append_id(builder, authority.source_higher_semantic_digest);
  append_id(builder, authority.source_terminal_digest);
  append_id(builder, authority.source_gateway_scientific_identity_digest);
  append_u8(builder, static_cast<std::uint8_t>(authority.source_plan_decision));
  append_u8(builder, static_cast<std::uint8_t>(authority.rank_window_decision));
  const bool claims[]{
      authority.source_plan_freshly_replayed,
      authority.rank_window_authority_freshly_replayed,
      authority.source_authorities_bound_to_one_terminal_facade,
      authority.every_higher_order_direct_birth_present,
      authority.every_higher_order_direct_coface_present,
      authority.every_direct_core_facet_present,
      authority.every_first_incidence_family_complete,
      authority.every_first_incidence_cominimizer_retained,
      authority.normalized_batches_are_complete_atomic_quotient_inputs,
      authority.factorized_cofaces_reconstructible_for_latent_carriers,
      authority.every_rank_relevant_ball_has_support_only_shell,
      authority.every_above_window_saturated_block_h0_quiescent,
      authority.rank_relevant_corollary_4_1_certified,
      authority.above_window_theorem_4_2_certified,
      authority.normalized_horizontal_h0_equivalence_certified,
      authority.incidence_complete_reduction_proved,
      authority.geometric_global_regularity_claimed,
      authority.resident_fold_executed,
      authority.order_one_boruvka_seam_certified,
      authority.vertical_maps_complete,
      authority.all_naturality_squares_replayed,
      authority.contract_v2_identity_compatible,
      authority.campaign_product_claimed,
      authority.global_star_or_gamma_materialized,
      authority.ordinary_or_higher_order_delaunay_materialized,
      authority.public_status_claimed,
      authority.performance_claimed};
  for (const bool claim : claims) {
    append_bool(builder, claim);
  }
  append_u8(builder, static_cast<std::uint8_t>(authority.decision));
  append_u8(builder, static_cast<std::uint8_t>(authority.scope));
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId compute_source_identity_digest(
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest) {
  contract::CanonicalSha256Builder builder;
  builder.update(source_identity_domain);
  append_u32(builder, manifest.schema_version);
  for (const std::size_t value : {
           manifest.point_count,
           manifest.requested_maximum_order,
           manifest.effective_maximum_order,
           manifest.batch_count,
           manifest.normalized_coface_count,
           manifest.stable_facet_token_count,
           manifest.direct_reference_count,
           manifest.residual_reference_count,
           manifest.coface_facet_reference_count}) {
    append_size(builder, value);
  }
  append_id(builder, manifest.source_pair_canonical_cloud_digest);
  append_id(builder, manifest.source_higher_canonical_cloud_digest);
  append_id(builder, manifest.source_pair_semantic_digest);
  append_id(builder, manifest.source_higher_semantic_digest);
  append_id(builder, manifest.source_terminal_digest);
  append_id(builder, manifest.source_gateway_scientific_identity_digest);
  append_id(builder, manifest.horizontal_incidence_authority_digest);
  const bool claims[]{
      manifest.horizontal_incidence_reduction_certified,
      manifest.incidence_complete_reduction_proved,
      manifest.stable_facet_ids_are_source_global_and_immutable,
      manifest.batches_dense_complete_and_strictly_level_ordered,
      manifest.exact_rational_levels_retained,
      manifest.no_complete_source_plan_owned,
      manifest.no_global_facet_coface_incidence_or_gamma_catalog_owned,
      manifest.no_star_cell_or_higher_order_delaunay_owned,
      manifest.source_exactness_claimed,
      manifest.vertical_maps_complete,
      manifest.public_status_claimed};
  for (const bool claim : claims) {
    append_bool(builder, claim);
  }
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId compute_initial_chain_digest(
    const contract::CanonicalId& source_identity) {
  contract::CanonicalSha256Builder builder;
  builder.update(initial_chain_domain);
  append_id(builder, source_identity);
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId compute_successor_chain_digest(
    const contract::CanonicalId& source_chain,
    const contract::CanonicalId& batch_identity) {
  contract::CanonicalSha256Builder builder;
  builder.update(chain_step_domain);
  append_id(builder, source_chain);
  append_id(builder, batch_identity);
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId compute_manifest_digest(
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest) {
  contract::CanonicalSha256Builder builder;
  builder.update(manifest_domain);
  append_id(builder, manifest.source_identity_digest);
  append_id(builder, manifest.initial_batch_chain_digest);
  append_id(builder, manifest.final_batch_chain_digest);
  append_size(builder, manifest.batch_count);
  append_size(builder, manifest.direct_reference_count);
  append_size(builder, manifest.residual_reference_count);
  append_size(builder, manifest.coface_facet_reference_count);
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId compute_batch_identity_digest(
    const ExactDirectNormalizedH0ResidentBatchWindow& window) {
  contract::CanonicalSha256Builder builder;
  builder.update(batch_identity_domain);
  append_id(builder, window.source_identity_digest);
  append_size(builder, window.source_batch_index);
  append_size(builder, window.source_future_snapshot_index);
  append_level(builder, window.squared_level);
  append_size(builder, window.order);
  append_bool(builder, window.scientific_source_freshly_recertified);
  append_bool(builder, window.resident_records_borrowed_without_copy);
  append_bool(builder, window.synchronous_lifetime_only);
  append_bool(builder, window.no_future_facet_or_batch_payload_retained);
  append_bool(builder, window.no_complete_source_plan_materialized);
  append_bool(
      builder,
      window.no_global_facet_coface_incidence_or_gamma_catalog_materialized);
  append_bool(builder, window.public_status_claimed);
  append_size(builder, window.facet_tokens.size());
  for (const auto& token : window.facet_tokens) {
    append_size(builder, token.window_facet_token_index);
    append_size(builder, token.stable_source_facet_token_index);
    append_facet_key(builder, token.facet_key);
  }
  append_size(builder, window.direct_references.size());
  for (const auto& reference : window.direct_references) {
    append_direct_reference(builder, reference);
  }
  append_size(builder, window.residual_references.size());
  for (const auto& reference : window.residual_references) {
    append_residual_reference(builder, reference);
  }
  append_size(builder, window.coface_facet_references.size());
  for (const auto& reference : window.coface_facet_references) {
    append_coface_facet_reference(builder, reference);
  }
  return builder.finalize();
}

[[nodiscard]] bool manifest_claims_are_certified(
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest) noexcept {
  return manifest.horizontal_incidence_reduction_certified &&
         manifest.incidence_complete_reduction_proved &&
         manifest.stable_facet_ids_are_source_global_and_immutable &&
         manifest.batches_dense_complete_and_strictly_level_ordered &&
         manifest.exact_rational_levels_retained &&
         manifest.no_complete_source_plan_owned &&
         manifest.no_global_facet_coface_incidence_or_gamma_catalog_owned &&
         manifest.no_star_cell_or_higher_order_delaunay_owned &&
         !manifest.source_exactness_claimed && !manifest.vertical_maps_complete &&
         !manifest.public_status_claimed;
}

[[nodiscard]] bool scalar_plan_matches_manifest(
    const ExactDirectSparseUnifiedLevelPlanResult& plan,
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest) noexcept {
  if (plan.schema_version != direct_sparse_unified_level_plan_schema_version ||
      plan.point_count != manifest.point_count ||
      plan.batches.size() != manifest.batch_count ||
      plan.facet_tokens.size() != manifest.stable_facet_token_count ||
      plan.direct_references.size() != manifest.direct_reference_count ||
      plan.residual_references.size() != manifest.residual_reference_count ||
      plan.coface_facet_references.size() !=
          manifest.coface_facet_reference_count ||
      plan.required_distinct_facet_token_count != plan.facet_tokens.size() ||
      plan.required_batch_count != plan.batches.size() ||
      plan.required_direct_reference_count != plan.direct_references.size() ||
      plan.required_residual_reference_count !=
          plan.residual_references.size() ||
      plan.required_coface_deletion_reference_count !=
          plan.coface_facet_references.size() ||
      plan.required_direct_birth_reference_count >
          plan.direct_references.size() ||
      plan.source_pair_canonical_cloud_digest !=
          manifest.source_pair_canonical_cloud_digest ||
      plan.source_higher_canonical_cloud_digest !=
          manifest.source_higher_canonical_cloud_digest ||
      plan.source_pair_semantic_digest !=
          manifest.source_pair_semantic_digest ||
      plan.source_higher_semantic_digest !=
          manifest.source_higher_semantic_digest ||
      !plan.order_one_roles_and_families_excluded_to_preserve_boruvka_authority ||
      !plan.every_higher_order_direct_role_projected_once ||
      !plan.every_higher_order_saddle_role_joined_to_one_family ||
      !plan.facet_tokens_canonical_and_deduplicated ||
      !plan.unique_batch_per_exact_level_and_order ||
      !plan.direct_and_residual_same_level_order_share_one_future_snapshot ||
      !plan.batches_sorted_by_exact_level_then_order ||
      !plan.logical_storage_within_budget ||
      !plan.no_partial_scientific_payload_published ||
      !plan.no_k_plus_one_coface_key_persisted ||
      !plan.no_global_facet_or_coface_catalog_materialized ||
      plan.reducer_snapshot_or_forest_mutated ||
      plan.bounded_star_global_completeness_claimed ||
      plan.public_status_claimed || !plan.partial_refinement_only ||
      plan.source_star_freshly_verified ||
      plan.direct_star_cofaces_crosschecked_bijectively ||
      plan.star_direct_cofaces_are_crosscheck_only_not_residual_references ||
      plan.every_star_residual_coface_referenced_once ||
      plan.every_star_coface_contributes_all_factorized_deletions ||
      plan.decision != ExactDirectSparseUnifiedLevelPlanDecision::not_certified ||
      plan.scope != ExactDirectSparseUnifiedLevelPlanScope::unspecified) {
    return false;
  }
  const std::size_t direct_coface_count =
      plan.direct_references.size() -
      plan.required_direct_birth_reference_count;
  std::size_t coface_count = 0U;
  return checked_add(
             direct_coface_count,
             plan.residual_references.size(),
             coface_count) &&
         coface_count == manifest.normalized_coface_count;
}

[[nodiscard]] bool coface_deletions_are_complete(
    const ExactDirectNormalizedH0ResidentBatchWindow& window,
    std::span<const std::size_t> coface_ids) {
  std::size_t coface_point_count = window.order;
  if (!checked_increment(coface_point_count)) {
    return false;
  }
  std::size_t expected_reference_count = 0U;
  if (!checked_multiply(
          coface_ids.size(), coface_point_count, expected_reference_count) ||
      expected_reference_count != window.coface_facet_references.size()) {
    return false;
  }
  std::size_t reference_cursor = 0U;
  for (const std::size_t coface_id : coface_ids) {
    std::array<bool, 11U> removed_positions{};
    std::array<spatial::PointId, 11U> coface_points{};
    bool first = true;
    for (std::size_t observed = 0U;
         observed < coface_point_count;
         ++observed) {
      if (reference_cursor >= window.coface_facet_references.size()) {
        return false;
      }
      const auto& reference =
          window.coface_facet_references[reference_cursor];
      if (reference.source_star_coface_index != coface_id ||
          !checked_increment(reference_cursor)) {
        return false;
      }
      if (reference.removed_union_point_index > window.order ||
          removed_positions[reference.removed_union_point_index] ||
          reference.facet_token_index >= window.facet_tokens.size()) {
        return false;
      }
      removed_positions[reference.removed_union_point_index] = true;
      const auto& key =
          window.facet_tokens[reference.facet_token_index].facet_key;
      if (key.point_count >= coface_points.size()) {
        return false;
      }
      std::array<spatial::PointId, 11U> reconstructed{};
      std::size_t destination = 0U;
      bool removed_inserted = false;
      for (std::size_t key_index = 0U;
           key_index < key.point_count;
           ++key_index) {
        if (!removed_inserted &&
            reference.removed_point_id < key.point_ids[key_index]) {
          if (destination >= reconstructed.size()) {
            return false;
          }
          reconstructed[destination] = reference.removed_point_id;
          if (!checked_increment(destination)) {
            return false;
          }
          removed_inserted = true;
        }
        if (destination >= reconstructed.size()) {
          return false;
        }
        reconstructed[destination] = key.point_ids[key_index];
        if (!checked_increment(destination)) {
          return false;
        }
      }
      if (!removed_inserted) {
        if (destination >= reconstructed.size()) {
          return false;
        }
        reconstructed[destination] = reference.removed_point_id;
        if (!checked_increment(destination)) {
          return false;
        }
      }
      if (destination != coface_point_count ||
          reconstructed[reference.removed_union_point_index] !=
              reference.removed_point_id ||
          (!first && reconstructed != coface_points)) {
        return false;
      }
      if (first) {
        coface_points = reconstructed;
        first = false;
      }
    }
    if (first ||
        !std::all_of(
            removed_positions.begin(),
            removed_positions.begin() +
                static_cast<std::ptrdiff_t>(coface_point_count),
            [](bool value) { return value; })) {
      return false;
    }
  }
  return reference_cursor == window.coface_facet_references.size();
}

[[nodiscard]] bool window_payload_valid(
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest,
    const ExactDirectNormalizedH0ResidentBatchProviderBudget& budget,
    const ExactDirectNormalizedH0ResidentBatchWindow& window,
    bool require_certified_manifest,
    bool require_terminal_chain) {
  if ((require_certified_manifest && !manifest.certified()) ||
      !manifest_claims_are_certified(manifest) ||
      window.schema_version != manifest.schema_version ||
      window.manifest_digest != manifest.manifest_digest ||
      window.source_identity_digest != manifest.source_identity_digest ||
      window.source_batch_index >= manifest.batch_count ||
      window.source_future_snapshot_index != window.source_batch_index ||
      window.order < 2U || window.order > manifest.effective_maximum_order ||
      window.order > direct_sparse_positive_facet_maximum_point_count ||
      !window.scientific_source_freshly_recertified ||
      !window.resident_records_borrowed_without_copy ||
      !window.synchronous_lifetime_only ||
      !window.no_future_facet_or_batch_payload_retained ||
      !window.no_complete_source_plan_materialized ||
      !window.no_global_facet_coface_incidence_or_gamma_catalog_materialized ||
      window.public_status_claimed ||
      window.facet_tokens.size() > budget.maximum_window_facet_count ||
      window.direct_references.size() >
          budget.maximum_window_direct_reference_count ||
      window.residual_references.size() >
          budget.maximum_window_residual_reference_count ||
      window.coface_facet_references.size() >
          budget.maximum_window_coface_facet_reference_count ||
      (window.source_batch_index == 0U &&
       window.source_chain_digest != manifest.initial_batch_chain_digest)) {
    return false;
  }

  std::size_t facet_point_count = 0U;
  std::vector<std::size_t> token_use_count(window.facet_tokens.size(), 0U);
  for (std::size_t index = 0U; index < window.facet_tokens.size(); ++index) {
    const auto& token = window.facet_tokens[index];
    if (token.window_facet_token_index != index ||
        token.stable_source_facet_token_index >=
            manifest.stable_facet_token_count ||
        (index != 0U &&
         window.facet_tokens[index - 1U].stable_source_facet_token_index >=
             token.stable_source_facet_token_index) ||
        !canonical_facet_key(
            token.facet_key, manifest.point_count, window.order) ||
        !checked_add(
            facet_point_count,
            token.facet_key.point_count,
            facet_point_count)) {
      return false;
    }
  }
  if (facet_point_count > budget.maximum_window_facet_point_count) {
    return false;
  }
  std::size_t logical = window.facet_tokens.size();
  for (const std::size_t increment : {
           facet_point_count,
           window.direct_references.size(),
           window.residual_references.size(),
           window.coface_facet_references.size()}) {
    if (!checked_add(logical, increment, logical)) {
      return false;
    }
  }
  if (logical > budget.maximum_window_logical_entry_count) {
    return false;
  }

  std::vector<std::size_t> coface_ids;
  std::size_t coface_id_capacity = 0U;
  if (!checked_add(
          window.direct_references.size(),
          window.residual_references.size(),
          coface_id_capacity)) {
    return false;
  }
  coface_ids.reserve(coface_id_capacity);
  for (const auto& reference : window.direct_references) {
    if (reference.direct_reference_index >= manifest.direct_reference_count) {
      return false;
    }
    if (reference.role == ExactDirectMorseH0Role::birth) {
      if (reference.source_incidence_family_index.has_value() ||
          reference.source_star_direct_coface_index.has_value() ||
          !reference.direct_birth_facet_token_index.has_value() ||
          *reference.direct_birth_facet_token_index >=
              window.facet_tokens.size()) {
        return false;
      }
      if (!checked_increment(
              token_use_count[*reference.direct_birth_facet_token_index])) {
        return false;
      }
    } else if (reference.role == ExactDirectMorseH0Role::saddle) {
      if (!reference.source_incidence_family_index.has_value() ||
          !reference.source_star_direct_coface_index.has_value() ||
          *reference.source_star_direct_coface_index >=
              manifest.normalized_coface_count ||
          reference.direct_birth_facet_token_index.has_value()) {
        return false;
      }
      coface_ids.push_back(*reference.source_star_direct_coface_index);
    } else {
      return false;
    }
  }
  for (const auto& reference : window.residual_references) {
    if (reference.residual_reference_index >=
            manifest.residual_reference_count ||
        reference.source_star_coface_index >=
            manifest.normalized_coface_count) {
      return false;
    }
    coface_ids.push_back(reference.source_star_coface_index);
  }
  std::sort(coface_ids.begin(), coface_ids.end());
  if (std::adjacent_find(coface_ids.begin(), coface_ids.end()) !=
      coface_ids.end()) {
    return false;
  }
  for (const auto& reference : window.coface_facet_references) {
    if (reference.coface_facet_reference_index >=
            manifest.coface_facet_reference_count ||
        reference.source_star_coface_index >=
            manifest.normalized_coface_count ||
        !std::binary_search(
            coface_ids.begin(),
            coface_ids.end(),
            reference.source_star_coface_index) ||
        reference.removed_union_point_index > window.order ||
        static_cast<std::size_t>(reference.removed_point_id) >=
            manifest.point_count ||
        reference.facet_token_index >= window.facet_tokens.size() ||
        std::binary_search(
            window.facet_tokens[reference.facet_token_index]
                .facet_key.point_ids.begin(),
            window.facet_tokens[reference.facet_token_index]
                    .facet_key.point_ids.begin() +
                static_cast<std::ptrdiff_t>(
                    window.facet_tokens[reference.facet_token_index]
                        .facet_key.point_count),
            reference.removed_point_id)) {
      return false;
    }
    if (!checked_increment(token_use_count[reference.facet_token_index])) {
      return false;
    }
  }
  if (!coface_deletions_are_complete(window, coface_ids) ||
      std::any_of(
          token_use_count.begin(),
          token_use_count.end(),
          [](std::size_t count) { return count == 0U; })) {
    return false;
  }
  const auto batch_digest = compute_batch_identity_digest(window);
  if (window.batch_identity_digest != batch_digest ||
      window.successor_chain_digest != compute_successor_chain_digest(
          window.source_chain_digest, batch_digest) ||
      (require_terminal_chain &&
       window.source_batch_index == manifest.batch_count - 1U &&
       window.successor_chain_digest != manifest.final_batch_chain_digest)) {
    return false;
  }
  return true;
}

[[nodiscard]] std::optional<std::size_t> local_token_index(
    std::span<const ExactDirectNormalizedH0ResidentWindowFacetToken> tokens,
    std::size_t stable_index) noexcept {
  const auto found = std::lower_bound(
      tokens.begin(),
      tokens.end(),
      stable_index,
      [](const ExactDirectNormalizedH0ResidentWindowFacetToken& token,
         std::size_t value) {
        return token.stable_source_facet_token_index < value;
      });
  if (found == tokens.end() ||
      found->stable_source_facet_token_index != stable_index) {
    return std::nullopt;
  }
  return static_cast<std::size_t>(found - tokens.begin());
}

[[nodiscard]] ExactDirectNormalizedH0ResidentBatchWindow make_window(
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest,
    const ExactDirectSparseUnifiedLevelPlanResult& plan,
    std::size_t batch_index,
    const contract::CanonicalId& source_chain_digest,
    ExactDirectNormalizedH0ResidentCompatibilityWindowScratch& scratch) {
  ExactDirectNormalizedH0ResidentBatchWindow empty;
  if (!scalar_plan_matches_manifest(plan, manifest) ||
      batch_index >= plan.batches.size()) {
    return empty;
  }
  const auto& batch = plan.batches[batch_index];
  if (batch.batch_index != batch_index ||
      batch.future_snapshot_index != batch_index || batch.order < 2U ||
      batch.order > manifest.effective_maximum_order ||
      batch.direct_reference_offset > plan.direct_references.size() ||
      batch.direct_reference_count >
          plan.direct_references.size() - batch.direct_reference_offset ||
      batch.residual_reference_offset > plan.residual_references.size() ||
      batch.residual_reference_count >
          plan.residual_references.size() - batch.residual_reference_offset ||
      batch.coface_facet_reference_offset >
          plan.coface_facet_references.size() ||
      batch.coface_facet_reference_count >
          plan.coface_facet_references.size() -
              batch.coface_facet_reference_offset) {
    return empty;
  }

  scratch.facet_tokens.clear();
  scratch.direct_references.assign(
      plan.direct_references.begin() +
          static_cast<std::ptrdiff_t>(batch.direct_reference_offset),
      plan.direct_references.begin() + static_cast<std::ptrdiff_t>(
          batch.direct_reference_offset + batch.direct_reference_count));
  scratch.residual_references.assign(
      plan.residual_references.begin() +
          static_cast<std::ptrdiff_t>(batch.residual_reference_offset),
      plan.residual_references.begin() + static_cast<std::ptrdiff_t>(
          batch.residual_reference_offset + batch.residual_reference_count));
  scratch.coface_facet_references.assign(
      plan.coface_facet_references.begin() + static_cast<std::ptrdiff_t>(
          batch.coface_facet_reference_offset),
      plan.coface_facet_references.begin() + static_cast<std::ptrdiff_t>(
          batch.coface_facet_reference_offset +
          batch.coface_facet_reference_count));

  std::size_t maximum_window_facet_occurrence_count = 0U;
  if (!checked_add(
          scratch.direct_references.size(),
          scratch.coface_facet_references.size(),
          maximum_window_facet_occurrence_count)) {
    return empty;
  }
  scratch.facet_tokens.reserve(maximum_window_facet_occurrence_count);

  const auto add_stable_token = [&](std::size_t stable_index) {
    if (stable_index >= plan.facet_tokens.size()) {
      throw std::logic_error("a normalized resident facet id is invalid");
    }
    scratch.facet_tokens.push_back(
        {0U, stable_index, plan.facet_tokens[stable_index].facet_key});
  };
  for (const auto& reference : scratch.direct_references) {
    if (reference.direct_birth_facet_token_index.has_value()) {
      add_stable_token(*reference.direct_birth_facet_token_index);
    }
  }
  for (const auto& reference : scratch.coface_facet_references) {
    add_stable_token(reference.facet_token_index);
  }
  std::sort(
      scratch.facet_tokens.begin(),
      scratch.facet_tokens.end(),
      [](const auto& left, const auto& right) {
        return left.stable_source_facet_token_index <
               right.stable_source_facet_token_index;
      });
  scratch.facet_tokens.erase(
      std::unique(
          scratch.facet_tokens.begin(),
          scratch.facet_tokens.end(),
          [](const auto& left, const auto& right) {
            return left.stable_source_facet_token_index ==
                   right.stable_source_facet_token_index;
          }),
      scratch.facet_tokens.end());
  for (std::size_t local = 0U; local < scratch.facet_tokens.size(); ++local) {
    scratch.facet_tokens[local].window_facet_token_index = local;
  }
  const auto token_span = std::span<
      const ExactDirectNormalizedH0ResidentWindowFacetToken>{
      scratch.facet_tokens};
  for (auto& reference : scratch.direct_references) {
    if (reference.direct_birth_facet_token_index.has_value()) {
      const auto local =
          local_token_index(token_span, *reference.direct_birth_facet_token_index);
      if (!local.has_value()) {
        throw std::logic_error("a normalized resident birth id is missing");
      }
      reference.direct_birth_facet_token_index = *local;
    }
  }
  for (auto& reference : scratch.coface_facet_references) {
    const auto local = local_token_index(token_span, reference.facet_token_index);
    if (!local.has_value()) {
      throw std::logic_error("a normalized resident coface id is missing");
    }
    reference.facet_token_index = *local;
  }

  ExactDirectNormalizedH0ResidentBatchWindow window;
  window.manifest_digest = manifest.manifest_digest;
  window.source_identity_digest = manifest.source_identity_digest;
  window.source_chain_digest = source_chain_digest;
  window.source_batch_index = batch_index;
  window.source_future_snapshot_index = batch.future_snapshot_index;
  window.squared_level = batch.squared_level;
  window.order = batch.order;
  window.facet_tokens = scratch.facet_tokens;
  window.direct_references = scratch.direct_references;
  window.residual_references = scratch.residual_references;
  window.coface_facet_references = scratch.coface_facet_references;
  window.scientific_source_freshly_recertified = true;
  window.resident_records_borrowed_without_copy = true;
  window.synchronous_lifetime_only = true;
  window.no_future_facet_or_batch_payload_retained = true;
  window.no_complete_source_plan_materialized = true;
  window.no_global_facet_coface_incidence_or_gamma_catalog_materialized = true;
  window.batch_identity_digest = compute_batch_identity_digest(window);
  window.successor_chain_digest = compute_successor_chain_digest(
      window.source_chain_digest, window.batch_identity_digest);
  return window;
}

[[nodiscard]] bool validate_plan_and_compute_final_chain(
    const ExactDirectSparseUnifiedLevelPlanResult& plan,
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest,
    contract::CanonicalId& final_chain) {
  if (!scalar_plan_matches_manifest(plan, manifest)) {
    return false;
  }
  std::size_t facet_point_count = 0U;
  std::vector<std::size_t> birth_uses(plan.facet_tokens.size(), 0U);
  std::vector<std::size_t> deletion_uses(plan.facet_tokens.size(), 0U);
  for (std::size_t index = 0U; index < plan.facet_tokens.size(); ++index) {
    const auto& token = plan.facet_tokens[index];
    if (token.facet_token_index != index ||
        !canonical_facet_key(token.facet_key, manifest.point_count) ||
        (index != 0U &&
         !facet_key_less(plan.facet_tokens[index - 1U].facet_key,
                         token.facet_key)) ||
        !checked_add(
            facet_point_count, token.facet_key.point_count, facet_point_count)) {
      return false;
    }
  }
  if (facet_point_count != plan.required_facet_key_point_count) {
    return false;
  }
  std::size_t logical = plan.facet_tokens.size();
  for (const std::size_t increment : {
           facet_point_count,
           plan.direct_references.size(),
           plan.residual_references.size(),
           plan.coface_facet_references.size(),
           plan.batches.size()}) {
    if (!checked_add(logical, increment, logical)) {
      return false;
    }
  }
  if (logical != plan.logical_storage_entry_count) {
    return false;
  }

  ExactDirectNormalizedH0ResidentBatchProviderBudget unbounded{
      manifest.batch_count,
      manifest.stable_facet_token_count,
      plan.required_facet_key_point_count,
      manifest.direct_reference_count,
      manifest.residual_reference_count,
      manifest.coface_facet_reference_count,
      plan.logical_storage_entry_count};
  std::vector<bool> seen_cofaces(manifest.normalized_coface_count, false);
  std::size_t direct_cursor = 0U;
  std::size_t residual_cursor = 0U;
  std::size_t coface_facet_cursor = 0U;
  std::size_t birth_count = 0U;
  std::optional<exact::ExactLevel> previous_level;
  std::size_t previous_order = 0U;
  contract::CanonicalId chain = manifest.initial_batch_chain_digest;
  ExactDirectNormalizedH0ResidentCompatibilityWindowScratch scratch;
  for (std::size_t batch_index = 0U;
       batch_index < manifest.batch_count;
       ++batch_index) {
    auto window = make_window(manifest, plan, batch_index, chain, scratch);
    if (!window_payload_valid(manifest, unbounded, window, false, false) ||
        (previous_level.has_value() &&
         !(*previous_level < window.squared_level ||
           (*previous_level == window.squared_level &&
            previous_order < window.order)))) {
      return false;
    }
    for (const auto& reference : window.direct_references) {
      if (reference.direct_reference_index != direct_cursor ||
          !checked_increment(direct_cursor)) {
        return false;
      }
      if (reference.role == ExactDirectMorseH0Role::birth) {
        if (!checked_increment(birth_count)) {
          return false;
        }
        const auto stable = window.facet_tokens
            [*reference.direct_birth_facet_token_index]
                .stable_source_facet_token_index;
        if (!checked_increment(birth_uses[stable])) {
          return false;
        }
      } else {
        const std::size_t coface =
            *reference.source_star_direct_coface_index;
        if (seen_cofaces[coface]) {
          return false;
        }
        seen_cofaces[coface] = true;
      }
    }
    for (const auto& reference : window.residual_references) {
      if (reference.residual_reference_index != residual_cursor ||
          !checked_increment(residual_cursor) ||
          seen_cofaces[reference.source_star_coface_index]) {
        return false;
      }
      seen_cofaces[reference.source_star_coface_index] = true;
    }
    for (const auto& reference : window.coface_facet_references) {
      if (reference.coface_facet_reference_index != coface_facet_cursor ||
          !checked_increment(coface_facet_cursor)) {
        return false;
      }
      const std::size_t stable =
          window.facet_tokens[reference.facet_token_index]
              .stable_source_facet_token_index;
      if (!checked_increment(deletion_uses[stable])) {
        return false;
      }
    }
    previous_level = window.squared_level;
    previous_order = window.order;
    chain = window.successor_chain_digest;
  }
  if (direct_cursor != manifest.direct_reference_count ||
      residual_cursor != manifest.residual_reference_count ||
      coface_facet_cursor != manifest.coface_facet_reference_count ||
      birth_count != plan.required_direct_birth_reference_count ||
      std::any_of(
          seen_cofaces.begin(),
          seen_cofaces.end(),
          [](bool value) { return !value; })) {
    return false;
  }
  for (std::size_t index = 0U; index < plan.facet_tokens.size(); ++index) {
    std::size_t total_uses = 0U;
    if (!checked_add(birth_uses[index], deletion_uses[index], total_uses)) {
      return false;
    }
    if (plan.facet_tokens[index].direct_birth_reference_count !=
            birth_uses[index] ||
        plan.facet_tokens[index].coface_deletion_reference_count !=
            deletion_uses[index] ||
        total_uses == 0U) {
      return false;
    }
  }
  final_chain = chain;
  return true;
}

}  // namespace

bool ExactDirectNormalizedH0ResidentSourceManifest::certified() const noexcept {
  try {
    const bool empty_stream = batch_count == 0U;
    const bool empty_shape = normalized_coface_count == 0U &&
                             stable_facet_token_count == 0U &&
                             direct_reference_count == 0U &&
                             residual_reference_count == 0U &&
                             coface_facet_reference_count == 0U &&
                             final_batch_chain_digest ==
                                 initial_batch_chain_digest;
    return schema_version ==
               direct_normalized_h0_resident_batch_provider_schema_version &&
           point_count != 0U && requested_maximum_order >= 2U &&
           effective_maximum_order >= 2U &&
           effective_maximum_order ==
               std::min(requested_maximum_order, point_count) &&
           ((empty_stream && empty_shape) ||
            (!empty_stream && stable_facet_token_count != 0U)) &&
           manifest_claims_are_certified(*this) &&
           source_identity_digest == compute_source_identity_digest(*this) &&
           initial_batch_chain_digest ==
               compute_initial_chain_digest(source_identity_digest) &&
           manifest_digest == compute_manifest_digest(*this);
  } catch (...) {
    return false;
  }
}

bool ExactDirectNormalizedH0ResidentBatchWindow::certified_relative_to(
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest,
    const ExactDirectNormalizedH0ResidentBatchProviderBudget& budget) const {
  try {
    return window_payload_valid(manifest, budget, *this, true, true);
  } catch (...) {
    return false;
  }
}

ExactDirectNormalizedH0ResidentProviderVerification
verify_exact_direct_normalized_h0_resident_batch_provider(
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest,
    ExactDirectNormalizedH0ResidentBatchProviderView provider,
    const ExactDirectNormalizedH0ResidentBatchProviderBudget& budget) {
  ExactDirectNormalizedH0ResidentProviderVerification result;
  result.provider_identity = provider.identity();
  result.manifest_digest = manifest.manifest_digest;
  result.provider_identity_bound =
      static_cast<bool>(provider) && result.provider_identity != nullptr;
  result.manifest_certified = manifest.certified();
  if (!result.manifest_certified) {
    result.decision =
        ExactDirectNormalizedH0ResidentProviderVerificationDecision::
            no_manifest_rejected;
    return result;
  }
  if (!result.provider_identity_bound) {
    result.decision =
        ExactDirectNormalizedH0ResidentProviderVerificationDecision::
            no_provider;
    return result;
  }
  if (manifest.batch_count > budget.maximum_batch_scan_count) {
    result.decision =
        ExactDirectNormalizedH0ResidentProviderVerificationDecision::
            no_batch_budget_exhausted;
    return result;
  }

  contract::CanonicalId chain = manifest.initial_batch_chain_digest;
  std::optional<exact::ExactLevel> previous_level;
  std::size_t previous_order = 0U;
  std::size_t direct_cursor = 0U;
  std::size_t residual_cursor = 0U;
  std::size_t coface_facet_cursor = 0U;
  bool every_fresh = true;
  for (std::size_t batch_index = 0U;
       batch_index < manifest.batch_count;
       ++batch_index) {
    struct Capture {
      const ExactDirectNormalizedH0ResidentSourceManifest& manifest;
      const ExactDirectNormalizedH0ResidentBatchProviderBudget& budget;
      std::size_t expected_batch{};
      const contract::CanonicalId& expected_chain;
      const std::optional<exact::ExactLevel>& previous_level;
      std::size_t previous_order{};
      std::size_t direct_cursor{};
      std::size_t residual_cursor{};
      std::size_t coface_facet_cursor{};
      std::size_t callback_count{};
      bool multiple_calls{false};
      bool accepted{false};
      bool fresh{false};
      exact::ExactLevel level{};
      std::size_t order{};
      contract::CanonicalId successor{};

      bool operator()(
          const ExactDirectNormalizedH0ResidentBatchWindow& window) {
        if (callback_count != 0U) {
          multiple_calls = true;
          accepted = false;
          if (callback_count < 2U) {
            static_cast<void>(checked_increment(callback_count));
          }
          return false;
        }
        if (!checked_increment(callback_count)) {
          multiple_calls = true;
          return false;
        }
        if (!window.certified_relative_to(manifest, budget) ||
            window.source_batch_index != expected_batch ||
            window.source_future_snapshot_index != expected_batch ||
            window.source_chain_digest != expected_chain ||
            (previous_level.has_value() &&
             !(*previous_level < window.squared_level ||
               (*previous_level == window.squared_level &&
                previous_order < window.order)))) {
          return false;
        }
        for (std::size_t local = 0U;
             local < window.direct_references.size();
             ++local) {
          std::size_t expected_index = 0U;
          if (!checked_add(direct_cursor, local, expected_index)) {
            return false;
          }
          if (window.direct_references[local].direct_reference_index !=
              expected_index) {
            return false;
          }
        }
        for (std::size_t local = 0U;
             local < window.residual_references.size();
             ++local) {
          std::size_t expected_index = 0U;
          if (!checked_add(residual_cursor, local, expected_index)) {
            return false;
          }
          if (window.residual_references[local].residual_reference_index !=
              expected_index) {
            return false;
          }
        }
        for (std::size_t local = 0U;
             local < window.coface_facet_references.size();
             ++local) {
          std::size_t expected_index = 0U;
          if (!checked_add(coface_facet_cursor, local, expected_index)) {
            return false;
          }
          if (window.coface_facet_references[local]
                  .coface_facet_reference_index !=
              expected_index) {
            return false;
          }
        }
        if (!checked_add(
                direct_cursor,
                window.direct_references.size(),
                direct_cursor) ||
            !checked_add(
                residual_cursor,
                window.residual_references.size(),
                residual_cursor) ||
            !checked_add(
                coface_facet_cursor,
                window.coface_facet_references.size(),
                coface_facet_cursor)) {
          return false;
        }
        fresh = window.scientific_source_freshly_recertified;
        level = window.squared_level;
        order = window.order;
        successor = window.successor_chain_digest;
        accepted = true;
        return true;
      }
    } capture{
        manifest,
        budget,
        batch_index,
        chain,
        previous_level,
        previous_order,
        direct_cursor,
        residual_cursor,
        coface_facet_cursor};
    ExactDirectNormalizedH0ResidentBatchConsumerView consumer{capture};
    ExactDirectNormalizedH0ResidentBatchVisitDecision visit;
    try {
      visit = provider(batch_index, consumer);
    } catch (...) {
      visit = ExactDirectNormalizedH0ResidentBatchVisitDecision::
          no_window_inconsistent;
    }
    if (!checked_add(
            result.consumer_callback_count,
            capture.callback_count,
            result.consumer_callback_count)) {
      result.multiple_consumer_callbacks_observed = true;
      result.decision =
          ExactDirectNormalizedH0ResidentProviderVerificationDecision::
              no_window_rejected;
      return result;
    }
    result.multiple_consumer_callbacks_observed =
        result.multiple_consumer_callbacks_observed || capture.multiple_calls;
    if (visit != ExactDirectNormalizedH0ResidentBatchVisitDecision::
                     complete_synchronous_visit ||
        capture.callback_count != 1U || capture.multiple_calls ||
        !capture.accepted) {
      result.decision =
          ExactDirectNormalizedH0ResidentProviderVerificationDecision::
              no_window_rejected;
      return result;
    }
    direct_cursor = capture.direct_cursor;
    residual_cursor = capture.residual_cursor;
    coface_facet_cursor = capture.coface_facet_cursor;
    every_fresh = every_fresh && capture.fresh;
    previous_level = std::move(capture.level);
    previous_order = capture.order;
    chain = capture.successor;
    if (!checked_increment(result.batch_visit_count)) {
      result.decision =
          ExactDirectNormalizedH0ResidentProviderVerificationDecision::
              no_source_totals_rejected;
      return result;
    }
  }

  result.direct_reference_count = direct_cursor;
  result.residual_reference_count = residual_cursor;
  result.coface_facet_reference_count = coface_facet_cursor;
  result.final_batch_chain_digest = chain;
  result.every_window_freshly_recertified = every_fresh;
  result.batches_dense_and_strictly_level_ordered =
      result.batch_visit_count == manifest.batch_count &&
      result.consumer_callback_count == manifest.batch_count &&
      !result.multiple_consumer_callbacks_observed;
  result.final_chain_matches_manifest =
      chain == manifest.final_batch_chain_digest;
  result.exact_source_totals_match_manifest =
      direct_cursor == manifest.direct_reference_count &&
      residual_cursor == manifest.residual_reference_count &&
      coface_facet_cursor == manifest.coface_facet_reference_count;
  result.no_window_payload_retained = true;
  if (!result.final_chain_matches_manifest || !every_fresh ||
      !result.batches_dense_and_strictly_level_ordered) {
    result.decision =
        ExactDirectNormalizedH0ResidentProviderVerificationDecision::
            no_chain_or_order_rejected;
    return result;
  }
  if (!result.exact_source_totals_match_manifest) {
    result.decision =
        ExactDirectNormalizedH0ResidentProviderVerificationDecision::
            no_source_totals_rejected;
    return result;
  }
  result.result_certified =
      result.provider_identity_bound &&
      result.manifest_digest == manifest.manifest_digest &&
      !result.source_exactness_claimed && !result.public_status_claimed;
  result.decision =
      ExactDirectNormalizedH0ResidentProviderVerificationDecision::
          complete_authenticated_stream_replay;
  return result;
}

bool ExactDirectNormalizedH0ResidentOwnedBatchWindow::certified_relative_to(
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest) const
    noexcept {
  try {
    if (!manifest.certified() || !exact_window_copied ||
        !no_future_payload_owned || manifest_digest != manifest.manifest_digest ||
        source_batch_index >= manifest.batch_count ||
        local_plan.batches.size() != 1U ||
        local_plan.facet_tokens.size() !=
            local_to_stable_facet_token_indices.size() ||
        local_plan.source_pair_canonical_cloud_digest !=
            manifest.source_pair_canonical_cloud_digest ||
        local_plan.source_higher_canonical_cloud_digest !=
            manifest.source_higher_canonical_cloud_digest ||
        local_plan.source_pair_semantic_digest !=
            manifest.source_pair_semantic_digest ||
        local_plan.source_higher_semantic_digest !=
            manifest.source_higher_semantic_digest ||
        local_plan.point_count != manifest.point_count ||
        !local_plan.no_k_plus_one_coface_key_persisted ||
        !local_plan.no_global_facet_or_coface_catalog_materialized ||
        local_plan.public_status_claimed ||
        local_plan.decision !=
            ExactDirectSparseUnifiedLevelPlanDecision::not_certified ||
        local_plan.scope != ExactDirectSparseUnifiedLevelPlanScope::unspecified) {
      return false;
    }
    std::vector<ExactDirectNormalizedH0ResidentWindowFacetToken> tokens;
    tokens.reserve(local_plan.facet_tokens.size());
    std::size_t facet_points = 0U;
    for (std::size_t local = 0U;
         local < local_plan.facet_tokens.size();
         ++local) {
      const auto& source = local_plan.facet_tokens[local];
      if (source.facet_token_index != local ||
          (local != 0U &&
           local_to_stable_facet_token_indices[local - 1U] >=
               local_to_stable_facet_token_indices[local]) ||
          !checked_add(
              facet_points, source.facet_key.point_count, facet_points)) {
        return false;
      }
      tokens.push_back(
          {local,
           local_to_stable_facet_token_indices[local],
           source.facet_key});
    }
    const auto& batch = local_plan.batches.front();
    if (batch.batch_index != source_batch_index ||
        batch.direct_reference_offset != 0U ||
        batch.direct_reference_count != local_plan.direct_references.size() ||
        batch.residual_reference_offset != 0U ||
        batch.residual_reference_count !=
            local_plan.residual_references.size() ||
        batch.coface_facet_reference_offset != 0U ||
        batch.coface_facet_reference_count !=
            local_plan.coface_facet_references.size()) {
      return false;
    }
    ExactDirectNormalizedH0ResidentBatchWindow window;
    window.manifest_digest = manifest_digest;
    window.source_identity_digest = manifest.source_identity_digest;
    window.source_chain_digest = source_chain_digest;
    window.batch_identity_digest = batch_identity_digest;
    window.successor_chain_digest = successor_chain_digest;
    window.source_batch_index = source_batch_index;
    window.source_future_snapshot_index = batch.future_snapshot_index;
    window.squared_level = batch.squared_level;
    window.order = batch.order;
    window.facet_tokens = tokens;
    window.direct_references = local_plan.direct_references;
    window.residual_references = local_plan.residual_references;
    window.coface_facet_references = local_plan.coface_facet_references;
    window.scientific_source_freshly_recertified = true;
    window.resident_records_borrowed_without_copy = true;
    window.synchronous_lifetime_only = true;
    window.no_future_facet_or_batch_payload_retained = true;
    window.no_complete_source_plan_materialized = true;
    window.no_global_facet_coface_incidence_or_gamma_catalog_materialized = true;
    const ExactDirectNormalizedH0ResidentBatchProviderBudget budget{
        1U,
        tokens.size(),
        facet_points,
        local_plan.direct_references.size(),
        local_plan.residual_references.size(),
        local_plan.coface_facet_references.size(),
        local_plan.logical_storage_entry_count};
    return window.certified_relative_to(manifest, budget);
  } catch (...) {
    return false;
  }
}

ExactDirectNormalizedH0ResidentOwnedBatchWindow
copy_exact_direct_normalized_h0_resident_batch_window(
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest,
    const ExactDirectNormalizedH0ResidentBatchWindow& window,
    const ExactDirectNormalizedH0ResidentBatchProviderBudget& budget) {
  ExactDirectNormalizedH0ResidentOwnedBatchWindow output;
  try {
    if (!window.certified_relative_to(manifest, budget)) {
      return output;
    }
    auto& plan = output.local_plan;
    plan.point_count = manifest.point_count;
    plan.required_distinct_facet_token_count = window.facet_tokens.size();
    plan.required_batch_count = 1U;
    plan.required_direct_reference_count = window.direct_references.size();
    plan.required_residual_reference_count = window.residual_references.size();
    plan.required_coface_deletion_reference_count =
        window.coface_facet_references.size();
    plan.source_pair_canonical_cloud_digest =
        manifest.source_pair_canonical_cloud_digest;
    plan.source_higher_canonical_cloud_digest =
        manifest.source_higher_canonical_cloud_digest;
    plan.source_pair_semantic_digest = manifest.source_pair_semantic_digest;
    plan.source_higher_semantic_digest = manifest.source_higher_semantic_digest;
    plan.facet_tokens.reserve(window.facet_tokens.size());
    output.local_to_stable_facet_token_indices.reserve(
        window.facet_tokens.size());
    std::size_t facet_points = 0U;
    std::vector<std::size_t> birth_counts(window.facet_tokens.size(), 0U);
    std::vector<std::size_t> deletion_counts(window.facet_tokens.size(), 0U);
    for (const auto& reference : window.direct_references) {
      if (reference.role == ExactDirectMorseH0Role::birth) {
        if (!checked_increment(plan.required_direct_birth_reference_count) ||
            !checked_increment(
                birth_counts[*reference.direct_birth_facet_token_index])) {
          return {};
        }
      } else {
        if (!checked_increment(plan.required_direct_saddle_reference_count)) {
          return {};
        }
      }
    }
    for (const auto& reference : window.coface_facet_references) {
      if (!checked_increment(deletion_counts[reference.facet_token_index])) {
        return {};
      }
    }
    for (std::size_t local = 0U; local < window.facet_tokens.size(); ++local) {
      const auto& token = window.facet_tokens[local];
      plan.facet_tokens.push_back(
          {local,
           token.facet_key,
           std::nullopt,
           birth_counts[local],
           deletion_counts[local]});
      output.local_to_stable_facet_token_indices.push_back(
          token.stable_source_facet_token_index);
      if (!checked_add(
              facet_points, token.facet_key.point_count, facet_points)) {
        return {};
      }
    }
    plan.required_facet_key_point_count = facet_points;
    plan.direct_references.assign(
        window.direct_references.begin(), window.direct_references.end());
    plan.residual_references.assign(
        window.residual_references.begin(), window.residual_references.end());
    plan.coface_facet_references.assign(
        window.coface_facet_references.begin(),
        window.coface_facet_references.end());
    plan.batches.push_back(
        {window.source_batch_index,
         window.source_future_snapshot_index,
         window.squared_level,
         window.order,
         0U,
         plan.direct_references.size(),
         0U,
         plan.residual_references.size(),
         0U,
         plan.coface_facet_references.size()});
    std::size_t logical = plan.facet_tokens.size();
    for (const std::size_t increment : {
             facet_points,
             plan.direct_references.size(),
             plan.residual_references.size(),
             plan.coface_facet_references.size(),
             plan.batches.size()}) {
      if (!checked_add(logical, increment, logical)) {
        return {};
      }
    }
    plan.logical_storage_entry_count = logical;
    plan.no_partial_scientific_payload_published = true;
    plan.no_k_plus_one_coface_key_persisted = true;
    plan.no_global_facet_or_coface_catalog_materialized = true;
    plan.partial_refinement_only = true;
    output.manifest_digest = window.manifest_digest;
    output.source_chain_digest = window.source_chain_digest;
    output.batch_identity_digest = window.batch_identity_digest;
    output.successor_chain_digest = window.successor_chain_digest;
    output.source_batch_index = window.source_batch_index;
    output.exact_window_copied = true;
    output.no_future_payload_owned = true;
    if (!output.certified_relative_to(manifest)) {
      return {};
    }
    return output;
  } catch (...) {
    return {};
  }
}

ExactDirectNormalizedH0ResidentSourceManifest
build_exact_direct_normalized_h0_resident_source_manifest_from_compatibility_plan(
    const ExactDirectSparseUnifiedLevelPlanResult& compatibility_plan,
    const ExactDirectNormalizedH0IncidenceReductionAuthority&
        incidence_authority) {
  if (!incidence_authority.certified_horizontal_incidence_reduction() ||
      !incidence_authority.incidence_complete_reduction_proved ||
      compatibility_plan.point_count != incidence_authority.point_count ||
      compatibility_plan.batches.size() !=
          incidence_authority.normalized_batch_count ||
      compatibility_plan.required_direct_birth_reference_count >
          compatibility_plan.direct_references.size()) {
    throw std::invalid_argument(
        "a normalized resident manifest requires a certified compatible source");
  }
  const std::size_t direct_coface_count =
      compatibility_plan.direct_references.size() -
      compatibility_plan.required_direct_birth_reference_count;
  std::size_t coface_count = 0U;
  if (!checked_add(
          direct_coface_count,
          compatibility_plan.residual_references.size(),
          coface_count) ||
      coface_count != incidence_authority.normalized_coface_count) {
    throw std::invalid_argument(
        "a normalized resident source has inconsistent coface totals");
  }

  ExactDirectNormalizedH0ResidentSourceManifest manifest;
  manifest.point_count = incidence_authority.point_count;
  manifest.requested_maximum_order =
      incidence_authority.requested_maximum_order;
  manifest.effective_maximum_order =
      incidence_authority.effective_maximum_order;
  manifest.batch_count = compatibility_plan.batches.size();
  manifest.normalized_coface_count = coface_count;
  manifest.stable_facet_token_count = compatibility_plan.facet_tokens.size();
  manifest.direct_reference_count = compatibility_plan.direct_references.size();
  manifest.residual_reference_count =
      compatibility_plan.residual_references.size();
  manifest.coface_facet_reference_count =
      compatibility_plan.coface_facet_references.size();
  manifest.source_pair_canonical_cloud_digest =
      incidence_authority.source_pair_canonical_cloud_digest;
  manifest.source_higher_canonical_cloud_digest =
      incidence_authority.source_higher_canonical_cloud_digest;
  manifest.source_pair_semantic_digest =
      incidence_authority.source_pair_semantic_digest;
  manifest.source_higher_semantic_digest =
      incidence_authority.source_higher_semantic_digest;
  manifest.source_terminal_digest = incidence_authority.source_terminal_digest;
  manifest.source_gateway_scientific_identity_digest =
      incidence_authority.source_gateway_scientific_identity_digest;
  manifest.horizontal_incidence_authority_digest =
      compute_authority_digest(incidence_authority);
  manifest.horizontal_incidence_reduction_certified = true;
  manifest.incidence_complete_reduction_proved = true;
  manifest.stable_facet_ids_are_source_global_and_immutable = true;
  manifest.batches_dense_complete_and_strictly_level_ordered = true;
  manifest.exact_rational_levels_retained = true;
  manifest.no_complete_source_plan_owned = true;
  manifest.no_global_facet_coface_incidence_or_gamma_catalog_owned = true;
  manifest.no_star_cell_or_higher_order_delaunay_owned = true;
  manifest.source_identity_digest = compute_source_identity_digest(manifest);
  manifest.initial_batch_chain_digest =
      compute_initial_chain_digest(manifest.source_identity_digest);
  contract::CanonicalId final_chain;
  if (!validate_plan_and_compute_final_chain(
          compatibility_plan, manifest, final_chain)) {
    throw std::invalid_argument(
        "a normalized resident compatibility cursor is inconsistent");
  }
  manifest.final_batch_chain_digest = final_chain;
  manifest.manifest_digest = compute_manifest_digest(manifest);
  if (!manifest.certified()) {
    throw std::logic_error(
        "a freshly built normalized resident source manifest is invalid");
  }
  return manifest;
}

ExactDirectNormalizedH0ResidentBatchWindow
borrow_exact_direct_normalized_h0_resident_compatibility_window(
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest,
    const ExactDirectSparseUnifiedLevelPlanResult& compatibility_plan,
    std::size_t batch_index,
    const contract::CanonicalId& source_chain_digest,
    ExactDirectNormalizedH0ResidentCompatibilityWindowScratch& scratch) {
  try {
    if (!manifest.certified()) {
      return {};
    }
    auto window = make_window(
        manifest,
        compatibility_plan,
        batch_index,
        source_chain_digest,
        scratch);
    const ExactDirectNormalizedH0ResidentBatchProviderBudget budget{
        manifest.batch_count,
        manifest.stable_facet_token_count,
        compatibility_plan.required_facet_key_point_count,
        manifest.direct_reference_count,
        manifest.residual_reference_count,
        manifest.coface_facet_reference_count,
        compatibility_plan.logical_storage_entry_count};
    return window.certified_relative_to(manifest, budget)
               ? window
               : ExactDirectNormalizedH0ResidentBatchWindow{};
  } catch (...) {
    return {};
  }
}

}  // namespace morsehgp3d::hierarchy
