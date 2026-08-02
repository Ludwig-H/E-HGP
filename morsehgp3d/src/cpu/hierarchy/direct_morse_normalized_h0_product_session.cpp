#include "morsehgp3d/hierarchy/direct_morse_normalized_h0_product_session.hpp"

#include "morsehgp3d/hierarchy/direct_morse_resident_at_least20_adapter.hpp"

#include <array>
#include <atomic>
#include <exception>
#include <limits>
#include <new>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::string_view scientific_digest_domain =
    "MorseHGP3D/phase15/direct-morse-normalized-h0-product/science/v1/sha256/";
constexpr std::string_view initial_chain_domain =
    "MorseHGP3D/phase15/direct-morse-normalized-h0-product/initial/v1/sha256/";
constexpr std::string_view batch_chain_domain =
    "MorseHGP3D/phase15/direct-morse-normalized-h0-product/batch/v1/sha256/";
constexpr std::string_view checkpoint_domain =
    "MorseHGP3D/phase15/direct-morse-normalized-h0-product/checkpoint/v1/sha256/";
constexpr std::string_view seal_domain =
    "MorseHGP3D/phase15/direct-morse-normalized-h0-product/seal/v1/sha256/";

static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));

std::atomic<std::uint64_t> next_product_session_instance_id{1U};

struct ProductSessionSeal final {};

struct ProductTicketRegistry final {
  std::size_t live_ticket_count{};
};

[[nodiscard]] std::uint64_t allocate_session_instance_id() noexcept {
  std::uint64_t candidate =
      next_product_session_instance_id.load(std::memory_order_relaxed);
  while (candidate != 0U) {
    const std::uint64_t successor = candidate + 1U;
    if (next_product_session_instance_id.compare_exchange_weak(
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

[[nodiscard]] bool id_is_zero(
    const contract::CanonicalId& value) noexcept {
  return value == contract::CanonicalId{};
}

[[nodiscard]] bool honest_normalized_compatibility_cursor(
    const ExactDirectSparseUnifiedLevelPlanResult& plan) noexcept {
  // A normalized-v7 resident deliberately does not impersonate a freshly
  // verified successive-incidence Star plan.  Its exact authority is the
  // separately certified normalized H0 incidence reduction carried by the
  // resident session and vertical bridge.
  return !plan.source_star_freshly_verified &&
      !plan.direct_star_cofaces_crosschecked_bijectively &&
      !plan.bounded_star_global_completeness_claimed &&
      !plan.public_status_claimed && plan.no_k_plus_one_coface_key_persisted &&
      plan.no_global_facet_or_coface_catalog_materialized &&
      plan.batches_sorted_by_exact_level_then_order &&
      plan.unique_batch_per_exact_level_and_order &&
      plan.decision == ExactDirectSparseUnifiedLevelPlanDecision::not_certified &&
      plan.scope == ExactDirectSparseUnifiedLevelPlanScope::unspecified;
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

void append_bool(contract::CanonicalSha256Builder& builder, bool value) {
  append_u64(builder, value ? 1U : 0U);
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
  append_text(builder, level.numerator_string());
  append_text(builder, level.denominator_string());
}

[[nodiscard]] contract::CanonicalId compute_scientific_digest(
    const ExactDirectMorseResidentAllOrdersVerticalBridge& vertical,
    const ExactDirectK1NormalizedProductStreamSession& k1,
    const contract::CanonicalId& higher_scientific_digest,
    std::size_t maximum_order,
    std::size_t product_batch_count) {
  contract::CanonicalSha256Builder builder;
  builder.update(scientific_digest_domain);
  append_u32(
      builder, direct_morse_normalized_h0_product_session_schema_version);
  append_size(builder, maximum_order);
  append_size(builder, product_batch_count);
  append_id(builder, k1.canonical_cloud_digest());
  append_id(builder, k1.source_forest_digest());
  append_id(builder, k1.scientific_digest());
  append_id(builder, higher_scientific_digest);
  append_u64(
      builder, static_cast<std::uint8_t>(vertical.resident_source_kind()));
  append_bool(builder, vertical.resident_normalized_direct_source_session());
  append_u64(
      builder,
      static_cast<std::uint8_t>(
          vertical.resident_normalized_h0_retraction_mode()));
  append_bool(
      builder,
      vertical.resident_normalized_horizontal_incidence_reduction_certified());
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId compute_initial_chain_digest(
    const contract::CanonicalId& scientific_digest,
    const contract::CanonicalId& k1_initial_chain,
    const contract::CanonicalId& higher_scientific_digest) {
  contract::CanonicalSha256Builder builder;
  builder.update(initial_chain_domain);
  append_u32(
      builder, direct_morse_normalized_h0_product_session_schema_version);
  append_id(builder, scientific_digest);
  append_id(builder, k1_initial_chain);
  append_id(builder, higher_scientific_digest);
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId compute_batch_chain_digest(
    const ExactNormalizedHartiganLevelInput& input,
    ExactDirectMorseNormalizedH0ProductNextSource source,
    const contract::CanonicalId& component_chain_digest) {
  contract::CanonicalSha256Builder builder;
  builder.update(batch_chain_domain);
  append_u32(
      builder, direct_morse_normalized_h0_product_session_schema_version);
  append_id(builder, input.previous_normalized_batch_chain_digest);
  append_u64(builder, static_cast<std::uint8_t>(source));
  append_size(builder, input.normalized_batch_index);
  append_size(builder, input.order);
  append_size(builder, input.order_batch_index);
  append_level(builder, input.squared_level);
  append_size(builder, input.core_facet_delta_count);
  append_size(builder, input.point_delta_count);
  append_size(builder, input.parent_delta_count);
  append_size(builder, input.node_delta_count);
  append_size(builder, input.group_count);
  append_size(builder, input.qr0_group_count);
  append_size(builder, input.qr1_group_count);
  append_size(builder, input.qr_greater_equal2_group_count);
  append_u64(builder, static_cast<std::uint8_t>(input.disposition));
  append_id(builder, component_chain_digest);
  return builder.finalize();
}

[[nodiscard]] bool resident_suffix_matches(
    std::size_t group_record_count_before,
    std::size_t source_batch_index,
    std::size_t order,
    const exact::ExactLevel& squared_level,
    std::span<const ExactDirectAtLeast20CommittedGroupInput> expected_groups,
    std::span<const ExactDirectMorseUnifiedResidentGroupRecord>
        actual_records) noexcept {
  std::size_t expected_count = 0U;
  if (!checked_add(
          group_record_count_before, expected_groups.size(), expected_count) ||
      actual_records.size() != expected_count) {
    return false;
  }
  for (std::size_t local = 0U; local < expected_groups.size(); ++local) {
    const auto& expected = expected_groups[local];
    const auto& actual = actual_records[group_record_count_before + local];
    const bool action_matches =
        (expected.source_action ==
             ExactDirectAtLeast20SourceAction::reduced_birth &&
         actual.action == ExactFrozenIncidenceHgpAction::reduced_birth) ||
        (expected.source_action ==
             ExactDirectAtLeast20SourceAction::continuation &&
         actual.action == ExactFrozenIncidenceHgpAction::continuation) ||
        (expected.source_action ==
             ExactDirectAtLeast20SourceAction::multifusion &&
         actual.action == ExactFrozenIncidenceHgpAction::multifusion);
    if (actual.group_record_index != group_record_count_before + local ||
        actual.batch_index != source_batch_index ||
        actual.owner_group_index != expected.source_group_index ||
        actual.squared_level != squared_level || actual.order != order ||
        actual.q_r != expected.q_r || !action_matches ||
        actual.resultant_root_id != expected.resultant_root_id ||
        actual.child_root_ids != expected.prior_root_ids ||
        actual.coverage_delta_points != expected.coverage_delta_point_ids ||
        actual.empty_coverage_delta !=
            expected.coverage_delta_point_ids.empty()) {
      return false;
    }
  }
  return true;
}

template <typename Enum>
void append_enum(contract::CanonicalSha256Builder& builder, Enum value) {
  static_assert(std::is_enum_v<Enum>);
  using Underlying = std::underlying_type_t<Enum>;
  append_u64(
      builder,
      static_cast<std::uint64_t>(static_cast<Underlying>(value)));
}

void append_optional_level(
    contract::CanonicalSha256Builder& builder,
    const std::optional<exact::ExactLevel>& level) {
  append_bool(builder, level.has_value());
  if (level.has_value()) {
    append_level(builder, *level);
  }
}

void append_product_stamp(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectMorseNormalizedH0ProductStamp& stamp) {
  append_u32(builder, stamp.schema_version);
  append_u64(builder, stamp.session_instance_id);
  append_size(builder, stamp.product_batch_cursor);
  append_size(builder, stamp.epoch);
  append_size(builder, stamp.hartigan_record_count);
  append_size(builder, stamp.k1_batch_cursor);
  append_size(builder, stamp.resident_batch_cursor);
  append_id(builder, stamp.scientific_digest);
  append_id(builder, stamp.initial_product_chain_digest);
  append_id(builder, stamp.product_chain_digest);
  append_id(builder, stamp.hartigan_chain_digest);
}

void append_k1_stamp(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectK1NormalizedProductStreamStamp& stamp) {
  append_u32(builder, stamp.schema_version);
  append_u64(builder, stamp.session_instance_id);
  append_size(builder, stamp.next_batch_cursor);
  append_size(builder, stamp.epoch);
  append_level(builder, stamp.last_committed_squared_level);
  append_id(builder, stamp.canonical_cloud_digest);
  append_id(builder, stamp.source_forest_digest);
  append_id(builder, stamp.scientific_digest);
  append_id(builder, stamp.committed_scientific_chain_digest);
  append_id(builder, stamp.at_least20_view_digest);
  append_size(builder, stamp.active_at_least20_root_count);
}

void append_vertical_stamp(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectMorseResidentAllOrdersVerticalBridgeStamp& stamp) {
  append_u32(builder, stamp.schema_version);
  append_u64(builder, stamp.bridge_session_instance_id);
  append_u64(builder, stamp.owned_k2_k1_bridge_session_instance_id);
  append_u64(builder, stamp.resident_session_authority_id);
  append_u64(builder, stamp.resident_locator_instance_id);
  append_size(builder, stamp.resident_batch_cursor);
  append_size(builder, stamp.resident_epoch);
  append_size(builder, stamp.committed_k2_batch_count);
  append_size(builder, stamp.committed_k2_group_count);
  append_size(builder, stamp.committed_higher_batch_count);
  append_size(builder, stamp.committed_higher_group_count);
  append_size(builder, stamp.persistent_source_root_witness_count);
  append_u64(builder, stamp.next_query_replay_token);
  append_id(builder, stamp.canonical_cloud_digest);
  append_id(builder, stamp.resident_higher_canonical_cloud_digest);
}

void append_locator_stamp(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparsePositiveFacetLocatorSnapshotStamp& stamp) {
  append_u32(builder, stamp.schema_version);
  append_u64(builder, stamp.external_authority_id);
  append_size(builder, stamp.committed_batch_count);
  append_size(builder, stamp.inserted_key_count);
  append_size(builder, stamp.component_union_count);
  append_size(builder, stamp.binding_count);
  append_id(builder, stamp.committed_history_digest);
}

void append_k1_checkpoint(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectK1NormalizedProductStreamCheckpoint& checkpoint) {
  append_u32(builder, checkpoint.schema_version);
  append_k1_stamp(builder, checkpoint.stamp);
  append_size(builder, checkpoint.total_batch_count);
  append_size(builder, checkpoint.total_merge_node_count);
  append_size(builder, checkpoint.total_child_reference_count);
  append_size(builder, checkpoint.active_at_least20_root_ids.size());
  for (const K1NodeId root_id : checkpoint.active_at_least20_root_ids) {
    append_u64(builder, root_id);
  }
  append_id(builder, checkpoint.state_digest);
  append_bool(builder, checkpoint.durable_file_codec_claimed);
  append_bool(builder, checkpoint.public_status_claimed);
}

void append_at_least20_checkpoint(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectAtLeast20StreamViewCheckpoint& checkpoint) {
  append_u32(builder, checkpoint.schema_version);
  append_u64(builder, checkpoint.source_session_authority_id);
  append_size(builder, checkpoint.next_source_batch_cursor);
  append_size(builder, checkpoint.source_epoch);
  append_id(builder, checkpoint.source_scientific_digest);
  append_id(builder, checkpoint.source_commit_digest);
  append_id(builder, checkpoint.view_digest);
  append_size(builder, checkpoint.roots.size());
  for (const auto& root : checkpoint.roots) {
    append_size(builder, root.order);
    append_u64(builder, root.root_id);
    append_size(builder, root.retained_smallest_point_ids.size());
    for (const spatial::PointId point_id :
         root.retained_smallest_point_ids) {
      append_u64(builder, static_cast<std::uint64_t>(point_id));
    }
    append_bool(builder, root.active);
  }
  append_size(builder, checkpoint.order_cursors.size());
  for (const auto& cursor : checkpoint.order_cursors) {
    append_size(builder, cursor.order);
    append_level(builder, cursor.last_squared_level);
  }
  append_size(builder, checkpoint.active_root_count);
  append_size(builder, checkpoint.retained_point_reference_count);
  append_id(builder, checkpoint.checkpoint_digest);
  append_bool(builder, checkpoint.source_pruning_performed);
  append_bool(builder, checkpoint.public_status_claimed);
}

void append_hartigan_order_summary(
    contract::CanonicalSha256Builder& builder,
    const ExactNormalizedHartiganOrderSummary& summary) {
  append_size(builder, summary.order);
  append_size(builder, summary.rational_record_count);
  append_size(
      builder, summary.normalized_omitted_noop_qr1_batch_count);
  append_size(builder, summary.normalized_group_count);
  append_size(builder, summary.normalized_qr0_group_count);
  append_size(builder, summary.normalized_qr1_group_count);
  append_size(
      builder, summary.normalized_qr_greater_equal2_group_count);
  append_size(
      builder, summary.legacy_single_group_scalar_qr_record_count);
  append_optional_level(builder, summary.first_level);
  append_optional_level(builder, summary.last_level);
}

void append_hartigan_checkpoint(
    contract::CanonicalSha256Builder& builder,
    const ExactNormalizedHartiganLevelManifestCheckpoint& checkpoint) {
  append_u32(builder, checkpoint.schema_version);
  append_u32(builder, checkpoint.manifest_schema_version);
  append_enum(builder, checkpoint.contract_mode);
  append_size(builder, checkpoint.maximum_order);
  for (const std::size_t count :
       checkpoint.expected_record_count_by_order) {
    append_size(builder, count);
  }
  for (const std::size_t count :
       checkpoint.expected_omitted_noop_count_by_order) {
    append_size(builder, count);
  }
  append_id(builder, checkpoint.normalized_source_scientific_digest);
  append_id(builder, checkpoint.normalized_batch_initial_chain_digest);
  append_id(builder, checkpoint.normalized_batch_final_chain_digest);
  for (const auto& summary : checkpoint.records_by_order) {
    append_hartigan_order_summary(builder, summary);
  }
  append_optional_level(builder, checkpoint.last_global_level);
  append_size(builder, checkpoint.last_global_order);
  append_id(builder, checkpoint.current_source_chain_digest);
  append_id(builder, checkpoint.current_rational_chain_digest);
  append_size(builder, checkpoint.expected_total_record_count);
  append_size(builder, checkpoint.record_count);
  append_id(builder, checkpoint.checkpoint_digest);
  append_bool(builder, checkpoint.process_local_authority_serialized);
  append_bool(builder, checkpoint.resource_budget_serialized);
  append_bool(builder, checkpoint.manifest_record_arena_materialized);
  append_bool(builder, checkpoint.public_status_claimed);
}

void append_k1_final_seal(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectK1NormalizedProductFinalSeal& seal) {
  append_u32(builder, seal.schema_version);
  append_k1_stamp(builder, seal.terminal_stamp);
  append_u64(builder, seal.root_node_id);
  append_size(builder, seal.total_batch_count);
  append_size(builder, seal.total_merge_node_count);
  append_size(builder, seal.total_child_reference_count);
  append_id(builder, seal.terminal_scientific_chain_digest);
  append_id(builder, seal.terminal_at_least20_view_digest);
  append_id(builder, seal.seal_digest);
  append_bool(builder, seal.source_stream_exhausted);
  append_bool(builder, seal.compact_k1_root_closed);
  append_bool(builder, seal.at_least20_terminal_state_closed);
  append_bool(builder, seal.no_outstanding_prepared_capability);
  append_bool(builder, seal.gamma_star_or_delaunay_materialized);
  append_bool(builder, seal.public_status_claimed);
  append_enum(builder, seal.decision);
}

void append_vertical_final_seal(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectMorseResidentAllOrdersVerticalFinalSeal& seal) {
  append_u32(builder, seal.schema_version);
  append_vertical_stamp(builder, seal.sealed_stamp);
  append_size(builder, seal.required_resident_batch_count);
  append_size(builder, seal.required_k2_batch_count);
  append_size(builder, seal.required_higher_batch_count);
  append_size(builder, seal.required_resident_group_count);
  append_size(builder, seal.sealed_k2_group_count);
  append_size(builder, seal.sealed_higher_group_count);
  append_size(builder, seal.sealed_nonroot_source_facet_resolution_count);
  append_size(
      builder, seal.sealed_expected_projected_target_facet_probe_count);
  append_size(builder, seal.sealed_projected_target_facet_probe_count);
  append_size(builder, seal.sealed_persistent_source_root_witness_count);
  append_size(builder, seal.sealed_final_root_witness_probe_count);
  append_size(builder, seal.sealed_terminal_locator_union_count);
  append_size(builder, seal.sealed_terminal_locator_batch_count);
  append_u64(
      builder, seal.final_root_witness_query_replay_token_begin);
  append_u64(builder, seal.final_root_witness_query_replay_token_end);
  append_u64(builder, seal.terminal_k1_session_instance_id);
  append_size(builder, seal.pre_terminal_k1_level_cursor);
  append_size(builder, seal.terminal_k1_level_cursor);
  append_size(builder, seal.distinct_k1_level_count);
  append_size(builder, seal.sealed_target_only_k1_level_count);
  append_id(builder, seal.k1_source_forest_digest);
  append_id(builder, seal.terminal_k1_history_digest);
  append_locator_stamp(builder, seal.terminal_locator_stamp);
  append_enum(builder, seal.resident_source_kind);
  append_enum(builder, seal.resident_normalized_h0_retraction_mode);
  append_bool(builder, seal.resident_normalized_direct_source_session);
  append_bool(
      builder,
      seal.resident_normalized_horizontal_incidence_reduction_certified);
  append_bool(
      builder, seal.normalized_incidence_complete_v7_source_capability);
  append_bool(
      builder, seal.owned_k1_terminal_advance_receipt_live_verified);
  append_bool(builder, seal.every_target_only_k1_level_consumed);
  append_bool(builder, seal.owned_k1_terminal_cursor_complete);
  append_bool(builder, seal.resident_cursor_exhausted);
  append_bool(builder, seal.no_outstanding_prepared_ticket);
  append_bool(
      builder, seal.every_required_plan_batch_replayed_in_exact_product_order);
  append_bool(builder, seal.every_k2_batch_and_group_replayed_to_sealed_k1);
  append_bool(
      builder, seal.every_higher_group_bound_to_one_live_adjacent_order_root);
  append_bool(
      builder,
      seal.every_nonroot_source_key_projected_to_all_codimension_one_deletions);
  append_bool(builder, seal.every_resident_group_covered_exactly_once);
  append_bool(
      builder,
      seal.every_persistent_target_binding_reprobed_at_terminal_locator);
  append_bool(
      builder, seal.terminal_locator_monotone_union_history_owned_and_exhausted);
  append_bool(
      builder,
      seal.every_intermediate_target_fusion_composed_by_terminal_root_reprobe);
  append_bool(builder, seal.all_naturality_squares_replayed);
  append_bool(builder, seal.vertical_maps_complete);
  append_bool(builder, seal.global_morse_obligation_replayed);
  append_bool(builder, seal.m1_replayed);
  append_bool(builder, seal.global_facet_coface_or_gamma_catalog_materialized);
  append_bool(builder, seal.ordinary_or_higher_order_delaunay_materialized);
  append_bool(builder, seal.pair_matrix_materialized);
  append_bool(builder, seal.public_status_claimed);
}

void append_hartigan_final_seal(
    contract::CanonicalSha256Builder& builder,
    const ExactNormalizedHartiganLevelManifestSeal& seal) {
  append_u32(builder, seal.schema_version);
  append_u64(builder, seal.authority_id);
  append_size(builder, seal.maximum_order);
  append_size(builder, seal.manifest_record_count);
  append_enum(builder, seal.contract_mode);
  for (const auto& summary : seal.records_by_order) {
    append_hartigan_order_summary(builder, summary);
  }
  append_id(builder, seal.normalized_batch_final_chain_digest);
  append_id(builder, seal.ordered_rational_chain_digest);
  append_bool(builder, seal.all_normalized_equal_level_batches_covered);
  append_bool(builder, seal.one_record_per_batch_including_omitted_noop);
  append_bool(builder, seal.binary64_level_count_zero);
  append_bool(builder, seal.bounded_memory_summary_only);
  append_bool(builder, seal.manifest_file_bytes_materialized);
  append_bool(builder, seal.every_record_uses_complete_batch_qr_partition);
  append_bool(
      builder, seal.legacy_single_group_scalar_qr_compatibility_used);
  append_bool(
      builder,
      seal.streaming_counts_closed_from_move_only_exhaustion_attestation);
  append_bool(builder, seal.public_status_claimed);
  append_enum(builder, seal.decision);
}

[[nodiscard]] contract::CanonicalId semantic_checkpoint_digest(
    const ExactDirectMorseNormalizedH0ProductSemanticCheckpoint& checkpoint) {
  contract::CanonicalSha256Builder builder;
  builder.update(checkpoint_domain);
  append_u32(builder, checkpoint.schema_version);
  append_product_stamp(builder, checkpoint.stamp);
  append_vertical_stamp(builder, checkpoint.vertical_stamp);
  append_k1_checkpoint(builder, checkpoint.k1);
  append_at_least20_checkpoint(builder, checkpoint.higher_at_least20);
  append_hartigan_checkpoint(builder, checkpoint.hartigan);
  for (const std::size_t count : checkpoint.record_count_by_order) {
    append_size(builder, count);
  }
  for (const std::size_t count : checkpoint.omitted_noop_count_by_order) {
    append_size(builder, count);
  }
  append_bool(builder, checkpoint.vertical_restore_available);
  append_bool(builder, checkpoint.restart_supported);
  append_bool(builder, checkpoint.durable_file_codec_claimed);
  append_bool(builder, checkpoint.public_status_claimed);
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId final_seal_digest(
    const ExactDirectMorseNormalizedH0ProductFinalSeal& seal) {
  contract::CanonicalSha256Builder builder;
  builder.update(seal_domain);
  append_u32(builder, seal.schema_version);
  append_product_stamp(builder, seal.terminal_stamp);
  append_k1_final_seal(builder, seal.k1);
  append_vertical_final_seal(builder, seal.vertical);
  append_at_least20_checkpoint(builder, seal.higher_at_least20);
  append_hartigan_final_seal(builder, seal.hartigan);
  append_size(builder, seal.structural_source_transition_count);
  append_size(builder, seal.hartigan_record_count);
  append_bool(
      builder, seal.normalized_incidence_complete_v7_source_capability);
  append_bool(
      builder, seal.every_product_batch_committed_in_exact_level_order);
  append_bool(
      builder,
      seal.empty_structural_batches_excluded_from_hartigan_manifest);
  append_bool(builder, seal.higher_at_least20_complete);
  append_bool(builder, seal.hartigan_streaming_closed);
  append_bool(
      builder, seal.no_global_facet_coface_or_gamma_catalog_materialized);
  append_bool(builder, seal.ordinary_or_higher_order_delaunay_materialized);
  append_bool(builder, seal.global_morse_obligation_replayed);
  append_bool(builder, seal.m1_replayed);
  append_bool(builder, seal.public_status_claimed);
  return builder.finalize();
}

[[nodiscard]] bool product_stamp_shape_certified(
    const ExactDirectMorseNormalizedH0ProductStamp& stamp) noexcept {
  std::size_t source_cursor = 0U;
  return stamp.schema_version ==
             direct_morse_normalized_h0_product_session_schema_version &&
      stamp.session_instance_id != 0U &&
      checked_add(
          stamp.k1_batch_cursor,
          stamp.resident_batch_cursor,
          source_cursor) &&
      source_cursor == stamp.product_batch_cursor &&
      stamp.epoch == stamp.product_batch_cursor &&
      stamp.hartigan_record_count <= stamp.product_batch_cursor &&
      !id_is_zero(stamp.scientific_digest) &&
      !id_is_zero(stamp.initial_product_chain_digest) &&
      !id_is_zero(stamp.product_chain_digest) &&
      !id_is_zero(stamp.hartigan_chain_digest) &&
      (stamp.product_batch_cursor != 0U ||
       stamp.product_chain_digest == stamp.initial_product_chain_digest) &&
      (stamp.hartigan_record_count != 0U ||
       stamp.hartigan_chain_digest == stamp.initial_product_chain_digest);
}

[[nodiscard]] bool k1_checkpoint_shape_certified(
    const ExactDirectK1NormalizedProductStreamCheckpoint& checkpoint)
    noexcept {
  if (checkpoint.schema_version !=
          direct_k1_normalized_product_stream_checkpoint_schema_version ||
      checkpoint.stamp.schema_version !=
          direct_k1_normalized_product_stream_schema_version ||
      checkpoint.stamp.session_instance_id == 0U ||
      checkpoint.stamp.next_batch_cursor != checkpoint.stamp.epoch ||
      checkpoint.stamp.next_batch_cursor > checkpoint.total_batch_count ||
      checkpoint.active_at_least20_root_ids.size() !=
          checkpoint.stamp.active_at_least20_root_count ||
      id_is_zero(checkpoint.stamp.canonical_cloud_digest) ||
      id_is_zero(checkpoint.stamp.source_forest_digest) ||
      id_is_zero(checkpoint.stamp.scientific_digest) ||
      id_is_zero(checkpoint.stamp.committed_scientific_chain_digest) ||
      id_is_zero(checkpoint.stamp.at_least20_view_digest) ||
      id_is_zero(checkpoint.state_digest) ||
      checkpoint.durable_file_codec_claimed || checkpoint.public_status_claimed) {
    return false;
  }
  for (std::size_t index = 1U;
       index < checkpoint.active_at_least20_root_ids.size(); ++index) {
    if (checkpoint.active_at_least20_root_ids[index - 1U] >=
        checkpoint.active_at_least20_root_ids[index]) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool at_least20_checkpoint_shape_certified(
    const ExactDirectAtLeast20StreamViewCheckpoint& checkpoint) noexcept {
  if (checkpoint.schema_version != direct_at_least20_stream_view_schema_version ||
      checkpoint.source_session_authority_id == 0U ||
      checkpoint.next_source_batch_cursor != checkpoint.source_epoch ||
      id_is_zero(checkpoint.source_scientific_digest) ||
      id_is_zero(checkpoint.view_digest) ||
      id_is_zero(checkpoint.checkpoint_digest) ||
      ((checkpoint.next_source_batch_cursor == 0U) !=
       id_is_zero(checkpoint.source_commit_digest)) ||
      checkpoint.source_pruning_performed || checkpoint.public_status_claimed) {
    return false;
  }
  std::size_t active_count = 0U;
  std::size_t retained_count = 0U;
  for (std::size_t index = 0U; index < checkpoint.roots.size(); ++index) {
    const auto& root = checkpoint.roots[index];
    if (root.order == 0U ||
        root.order > direct_at_least20_stream_view_maximum_order ||
        root.root_id == 0U ||
        root.retained_smallest_point_ids.size() >
            direct_at_least20_stream_view_threshold ||
        !checked_add(
            retained_count,
            root.retained_smallest_point_ids.size(),
            retained_count) ||
        !checked_add(active_count, root.active ? 1U : 0U, active_count)) {
      return false;
    }
    for (std::size_t point_index = 1U;
         point_index < root.retained_smallest_point_ids.size(); ++point_index) {
      if (root.retained_smallest_point_ids[point_index - 1U] >=
          root.retained_smallest_point_ids[point_index]) {
        return false;
      }
    }
    if (index != 0U) {
      const auto& previous = checkpoint.roots[index - 1U];
      if (previous.order > root.order ||
          (previous.order == root.order &&
           previous.root_id >= root.root_id)) {
        return false;
      }
    }
  }
  for (std::size_t index = 0U;
       index < checkpoint.order_cursors.size(); ++index) {
    const auto& cursor = checkpoint.order_cursors[index];
    if (cursor.order == 0U ||
        cursor.order > direct_at_least20_stream_view_maximum_order ||
        (index != 0U &&
         checkpoint.order_cursors[index - 1U].order >= cursor.order)) {
      return false;
    }
  }
  return active_count == checkpoint.active_root_count &&
      retained_count == checkpoint.retained_point_reference_count;
}

[[nodiscard]] bool hartigan_checkpoint_shape_certified(
    const ExactNormalizedHartiganLevelManifestCheckpoint& checkpoint,
    const std::array<
        std::size_t,
        normalized_exact_hartigan_level_manifest_maximum_order>&
        record_count_by_order,
    const std::array<
        std::size_t,
        normalized_exact_hartigan_level_manifest_maximum_order>&
        omitted_count_by_order) noexcept {
  if (checkpoint.schema_version !=
          normalized_exact_hartigan_level_manifest_checkpoint_schema_version ||
      checkpoint.manifest_schema_version !=
          normalized_exact_hartigan_level_manifest_schema_version ||
      checkpoint.contract_mode !=
          ExactNormalizedHartiganManifestContractMode::
              streaming_observed_open_contract ||
      checkpoint.maximum_order == 0U ||
      checkpoint.maximum_order >
          normalized_exact_hartigan_level_manifest_maximum_order ||
      id_is_zero(checkpoint.normalized_source_scientific_digest) ||
      id_is_zero(checkpoint.normalized_batch_initial_chain_digest) ||
      !id_is_zero(checkpoint.normalized_batch_final_chain_digest) ||
      id_is_zero(checkpoint.current_source_chain_digest) ||
      id_is_zero(checkpoint.current_rational_chain_digest) ||
      id_is_zero(checkpoint.checkpoint_digest) ||
      checkpoint.expected_total_record_count != 0U ||
      checkpoint.process_local_authority_serialized ||
      checkpoint.resource_budget_serialized ||
      checkpoint.manifest_record_arena_materialized ||
      checkpoint.public_status_claimed) {
    return false;
  }
  std::size_t total = 0U;
  for (std::size_t index = 0U;
       index < normalized_exact_hartigan_level_manifest_maximum_order;
       ++index) {
    const auto& summary = checkpoint.records_by_order[index];
    std::size_t group_partition_count = 0U;
    if (checkpoint.expected_record_count_by_order[index] != 0U ||
        checkpoint.expected_omitted_noop_count_by_order[index] != 0U ||
        summary.order != index + 1U ||
        summary.rational_record_count != record_count_by_order[index] ||
        summary.normalized_omitted_noop_qr1_batch_count !=
            omitted_count_by_order[index] ||
        summary.normalized_omitted_noop_qr1_batch_count >
            summary.rational_record_count ||
        summary.normalized_omitted_noop_qr1_batch_count >
            summary.normalized_qr1_group_count ||
        summary.legacy_single_group_scalar_qr_record_count != 0U ||
        !checked_add(
            group_partition_count,
            summary.normalized_qr0_group_count,
            group_partition_count) ||
        !checked_add(
            group_partition_count,
            summary.normalized_qr1_group_count,
            group_partition_count) ||
        !checked_add(
            group_partition_count,
            summary.normalized_qr_greater_equal2_group_count,
            group_partition_count) ||
        group_partition_count != summary.normalized_group_count ||
        ((summary.rational_record_count == 0U) !=
         !summary.first_level.has_value()) ||
        ((summary.rational_record_count == 0U) !=
         !summary.last_level.has_value()) ||
        (summary.first_level.has_value() &&
         *summary.last_level < *summary.first_level) ||
        (index >= checkpoint.maximum_order &&
         summary.rational_record_count != 0U) ||
        !checked_add(total, summary.rational_record_count, total)) {
      return false;
    }
  }
  return total == checkpoint.record_count &&
      ((checkpoint.record_count == 0U) ==
       !checkpoint.last_global_level.has_value()) &&
      (checkpoint.record_count == 0U
           ? checkpoint.last_global_order == 0U
           : checkpoint.last_global_order != 0U &&
                 checkpoint.last_global_order <= checkpoint.maximum_order);
}

[[nodiscard]] bool semantic_checkpoint_links_certified(
    const ExactDirectMorseNormalizedH0ProductSemanticCheckpoint& checkpoint) {
  if (checkpoint.schema_version !=
          direct_morse_normalized_h0_product_session_schema_version ||
      !product_stamp_shape_certified(checkpoint.stamp) ||
      checkpoint.vertical_stamp.schema_version !=
          direct_morse_resident_all_orders_vertical_bridge_schema_version ||
      checkpoint.vertical_stamp.bridge_session_instance_id == 0U ||
      checkpoint.vertical_stamp.resident_session_authority_id == 0U ||
      checkpoint.vertical_stamp.resident_batch_cursor !=
          checkpoint.stamp.resident_batch_cursor ||
      checkpoint.vertical_stamp.resident_epoch !=
          checkpoint.stamp.resident_batch_cursor ||
      !k1_checkpoint_shape_certified(checkpoint.k1) ||
      checkpoint.k1.stamp.next_batch_cursor !=
          checkpoint.stamp.k1_batch_cursor ||
      checkpoint.k1.stamp.canonical_cloud_digest !=
          checkpoint.vertical_stamp.canonical_cloud_digest ||
      !at_least20_checkpoint_shape_certified(
          checkpoint.higher_at_least20) ||
      checkpoint.higher_at_least20.source_session_authority_id !=
          checkpoint.vertical_stamp.resident_session_authority_id ||
      checkpoint.higher_at_least20.next_source_batch_cursor !=
          checkpoint.stamp.resident_batch_cursor ||
      checkpoint.higher_at_least20.source_epoch !=
          checkpoint.stamp.resident_batch_cursor ||
      !hartigan_checkpoint_shape_certified(
          checkpoint.hartigan,
          checkpoint.record_count_by_order,
          checkpoint.omitted_noop_count_by_order) ||
      checkpoint.hartigan.normalized_source_scientific_digest !=
          checkpoint.stamp.scientific_digest ||
      checkpoint.hartigan.normalized_batch_initial_chain_digest !=
          checkpoint.stamp.initial_product_chain_digest ||
      checkpoint.hartigan.current_source_chain_digest !=
          checkpoint.stamp.hartigan_chain_digest ||
      checkpoint.hartigan.record_count !=
          checkpoint.stamp.hartigan_record_count ||
      checkpoint.vertical_restore_available || checkpoint.restart_supported ||
      checkpoint.durable_file_codec_claimed || checkpoint.public_status_claimed ||
      id_is_zero(checkpoint.semantic_digest)) {
    return false;
  }
  return checkpoint.semantic_digest == semantic_checkpoint_digest(checkpoint);
}

[[nodiscard]] bool hartigan_final_seals_equal(
    const ExactNormalizedHartiganLevelManifestSeal& left,
    const ExactNormalizedHartiganLevelManifestSeal& right) noexcept {
  return left.schema_version == right.schema_version &&
      left.authority_id == right.authority_id &&
      left.maximum_order == right.maximum_order &&
      left.manifest_record_count == right.manifest_record_count &&
      left.contract_mode == right.contract_mode &&
      left.records_by_order == right.records_by_order &&
      left.normalized_batch_final_chain_digest ==
          right.normalized_batch_final_chain_digest &&
      left.ordered_rational_chain_digest ==
          right.ordered_rational_chain_digest &&
      left.all_normalized_equal_level_batches_covered ==
          right.all_normalized_equal_level_batches_covered &&
      left.one_record_per_batch_including_omitted_noop ==
          right.one_record_per_batch_including_omitted_noop &&
      left.binary64_level_count_zero == right.binary64_level_count_zero &&
      left.bounded_memory_summary_only == right.bounded_memory_summary_only &&
      left.manifest_file_bytes_materialized ==
          right.manifest_file_bytes_materialized &&
      left.every_record_uses_complete_batch_qr_partition ==
          right.every_record_uses_complete_batch_qr_partition &&
      left.legacy_single_group_scalar_qr_compatibility_used ==
          right.legacy_single_group_scalar_qr_compatibility_used &&
      left.streaming_counts_closed_from_move_only_exhaustion_attestation ==
          right.streaming_counts_closed_from_move_only_exhaustion_attestation &&
      left.public_status_claimed == right.public_status_claimed &&
      left.decision == right.decision;
}

[[nodiscard]] bool final_seal_links_certified(
    const ExactDirectMorseNormalizedH0ProductFinalSeal& seal) {
  return seal.schema_version ==
             direct_morse_normalized_h0_product_session_schema_version &&
      product_stamp_shape_certified(seal.terminal_stamp) &&
      seal.k1.certified_terminal_closure() &&
      seal.vertical.certified_final_vertical_seal() &&
      at_least20_checkpoint_shape_certified(seal.higher_at_least20) &&
      seal.hartigan.certified_seal() &&
      seal.k1.terminal_stamp.next_batch_cursor ==
          seal.terminal_stamp.k1_batch_cursor &&
      seal.k1.total_batch_count == seal.terminal_stamp.k1_batch_cursor &&
      seal.k1.terminal_scientific_chain_digest ==
          seal.k1.terminal_stamp.committed_scientific_chain_digest &&
      seal.k1.terminal_at_least20_view_digest ==
          seal.k1.terminal_stamp.at_least20_view_digest &&
      seal.k1.terminal_stamp.canonical_cloud_digest ==
          seal.vertical.sealed_stamp.canonical_cloud_digest &&
      seal.k1.terminal_stamp.source_forest_digest ==
          seal.vertical.k1_source_forest_digest &&
      seal.vertical.sealed_stamp.resident_batch_cursor ==
          seal.terminal_stamp.resident_batch_cursor &&
      seal.vertical.required_resident_batch_count ==
          seal.terminal_stamp.resident_batch_cursor &&
      seal.vertical.sealed_stamp.resident_epoch ==
          seal.terminal_stamp.resident_batch_cursor &&
      seal.higher_at_least20.source_session_authority_id ==
          seal.vertical.sealed_stamp.resident_session_authority_id &&
      seal.higher_at_least20.next_source_batch_cursor ==
          seal.terminal_stamp.resident_batch_cursor &&
      seal.higher_at_least20.source_epoch ==
          seal.terminal_stamp.resident_batch_cursor &&
      seal.hartigan.contract_mode ==
          ExactNormalizedHartiganManifestContractMode::
              streaming_observed_open_contract &&
      seal.hartigan.manifest_record_count ==
          seal.terminal_stamp.hartigan_record_count &&
      seal.hartigan.normalized_batch_final_chain_digest ==
          seal.terminal_stamp.hartigan_chain_digest &&
      seal.structural_source_transition_count ==
          seal.terminal_stamp.product_batch_cursor &&
      seal.hartigan_record_count ==
          seal.terminal_stamp.hartigan_record_count &&
      seal.normalized_incidence_complete_v7_source_capability ==
          seal.vertical.normalized_incidence_complete_v7_source_capability &&
      seal.normalized_incidence_complete_v7_source_capability &&
      seal.every_product_batch_committed_in_exact_level_order &&
      seal.empty_structural_batches_excluded_from_hartigan_manifest &&
      seal.higher_at_least20_complete && seal.hartigan_streaming_closed &&
      seal.no_global_facet_coface_or_gamma_catalog_materialized &&
      !seal.ordinary_or_higher_order_delaunay_materialized &&
      !seal.global_morse_obligation_replayed && !seal.m1_replayed &&
      !seal.public_status_claimed && !id_is_zero(seal.seal_digest) &&
      seal.seal_digest == final_seal_digest(seal);
}

}  // namespace

struct ExactDirectMorseNormalizedH0ProductPreparedBatch::Impl {
  ~Impl() noexcept {
    if (!owns_ticket_slot) {
      return;
    }
    if (registry == nullptr || registry->live_ticket_count == 0U) {
      std::terminate();
    }
    --registry->live_ticket_count;
  }

  std::shared_ptr<const ProductSessionSeal> seal;
  std::shared_ptr<ProductTicketRegistry> registry;
  std::optional<ExactDirectK1NormalizedProductPreparedBatch> k1_ticket;
  std::optional<ExactDirectMorseResidentAllOrdersVerticalPreparedBatch>
      resident_ticket;
  std::optional<ExactDirectAtLeast20StreamViewPreparedBatch> view_ticket;
  std::optional<ExactNormalizedHartiganLevelPreparedAppend> hartigan_ticket;
  std::vector<ExactDirectAtLeast20CommittedGroupInput> expected_groups;
  ExactDirectMorseNormalizedH0ProductStamp expected_stamp{};
  ExactDirectMorseNormalizedH0ProductNextSource source{
      ExactDirectMorseNormalizedH0ProductNextSource::complete};
  exact::ExactLevel squared_level{};
  std::size_t normalized_batch_index{};
  std::size_t order{};
  std::size_t order_batch_index{};
  std::size_t resident_group_record_count_before{};
  std::size_t resident_source_batch_index{};
  std::size_t resident_source_epoch{};
  ExactDirectAtLeast20RootId next_root_id_before{};
  ExactDirectAtLeast20RootId next_root_id_after{};
  contract::CanonicalId expected_higher_source_commit_digest{};
  contract::CanonicalId next_product_chain_digest{};
  contract::CanonicalId next_hartigan_chain_digest{};
  bool emits_hartigan_record{false};
  bool owns_ticket_slot{false};
  bool consumed{false};
};

struct ExactDirectMorseNormalizedH0ProductSession::Impl {
  ExactDirectMorseResidentAllOrdersVerticalBridge vertical;
  ExactDirectK1NormalizedProductStreamSession k1;
  ExactDirectAtLeast20StreamViewSession higher_at_least20;
  ExactNormalizedHartiganLevelManifestSession hartigan;
  ExactDirectMorseNormalizedH0ProductBudget budget{};
  ExactDirectMorseNormalizedH0ProductRequirements requirements{};
  std::shared_ptr<const ProductSessionSeal> seal;
  std::shared_ptr<ProductTicketRegistry> registry;
  std::shared_ptr<const ExactDirectMorseNormalizedH0ProductFinalSeal>
      final_seal;
  std::array<
      std::size_t,
      normalized_exact_hartigan_level_manifest_maximum_order>
      record_count_by_order{};
  std::array<
      std::size_t,
      normalized_exact_hartigan_level_manifest_maximum_order>
      omitted_noop_count_by_order{};
  contract::CanonicalId higher_scientific_digest{};
  contract::CanonicalId scientific_digest{};
  contract::CanonicalId initial_product_chain_digest{};
  contract::CanonicalId product_chain_digest{};
  contract::CanonicalId hartigan_chain_digest{};
  std::uint64_t session_instance_id{};
  std::uint64_t product_authority_id{};
  ExactDirectAtLeast20RootId next_higher_root_id{1U};
  std::size_t product_batch_cursor{};
  std::size_t epoch{};
  std::size_t hartigan_record_count{};
  bool initialized{false};
};

ExactDirectMorseNormalizedH0ProductPreparedBatch::
    ExactDirectMorseNormalizedH0ProductPreparedBatch() noexcept = default;
ExactDirectMorseNormalizedH0ProductPreparedBatch::
    ~ExactDirectMorseNormalizedH0ProductPreparedBatch() = default;
ExactDirectMorseNormalizedH0ProductPreparedBatch::
    ExactDirectMorseNormalizedH0ProductPreparedBatch(
        ExactDirectMorseNormalizedH0ProductPreparedBatch&&) noexcept = default;
ExactDirectMorseNormalizedH0ProductPreparedBatch&
ExactDirectMorseNormalizedH0ProductPreparedBatch::operator=(
    ExactDirectMorseNormalizedH0ProductPreparedBatch&&) noexcept = default;
ExactDirectMorseNormalizedH0ProductPreparedBatch::
    ExactDirectMorseNormalizedH0ProductPreparedBatch(
        std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectMorseNormalizedH0ProductPreparedBatch::valid() const noexcept {
  if (impl_ == nullptr || impl_->consumed || !impl_->owns_ticket_slot ||
      impl_->seal == nullptr || impl_->registry == nullptr ||
      impl_->registry->live_ticket_count != 1U ||
      (impl_->emits_hartigan_record != impl_->hartigan_ticket.has_value()) ||
      (impl_->hartigan_ticket.has_value() &&
       !impl_->hartigan_ticket->valid())) {
    return false;
  }
  if (impl_->source == ExactDirectMorseNormalizedH0ProductNextSource::k1) {
    return impl_->k1_ticket.has_value() && impl_->k1_ticket->valid() &&
        !impl_->resident_ticket.has_value() && !impl_->view_ticket.has_value();
  }
  return impl_->source ==
             ExactDirectMorseNormalizedH0ProductNextSource::resident &&
      !impl_->k1_ticket.has_value() && impl_->resident_ticket.has_value() &&
      impl_->resident_ticket->valid() && impl_->view_ticket.has_value() &&
      impl_->view_ticket->valid();
}

bool ExactDirectMorseNormalizedH0ProductPreparedBatch::consumed()
    const noexcept {
  return impl_ == nullptr || impl_->consumed;
}

ExactDirectMorseNormalizedH0ProductNextSource
ExactDirectMorseNormalizedH0ProductPreparedBatch::source() const noexcept {
  return impl_ == nullptr
             ? ExactDirectMorseNormalizedH0ProductNextSource::complete
             : impl_->source;
}

std::size_t
ExactDirectMorseNormalizedH0ProductPreparedBatch::normalized_batch_index()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->normalized_batch_index;
}

std::size_t ExactDirectMorseNormalizedH0ProductPreparedBatch::order()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->order;
}

const exact::ExactLevel&
ExactDirectMorseNormalizedH0ProductPreparedBatch::squared_level()
    const noexcept {
  static const exact::ExactLevel empty{};
  return impl_ == nullptr ? empty : impl_->squared_level;
}

bool ExactDirectMorseNormalizedH0ProductPreparedBatch::
    emits_hartigan_record() const noexcept {
  return impl_ != nullptr && impl_->emits_hartigan_record;
}

const ExactNormalizedHartiganLevelManifestRecord*
ExactDirectMorseNormalizedH0ProductPreparedBatch::prepared_hartigan_record()
    const noexcept {
  return impl_ == nullptr || !impl_->hartigan_ticket.has_value() ||
          !impl_->hartigan_ticket->valid()
             ? nullptr
             : &impl_->hartigan_ticket->prepared_record();
}

ExactDirectMorseNormalizedH0ProductSession::
    ExactDirectMorseNormalizedH0ProductSession() noexcept = default;
ExactDirectMorseNormalizedH0ProductSession::
    ~ExactDirectMorseNormalizedH0ProductSession() = default;
ExactDirectMorseNormalizedH0ProductSession::
    ExactDirectMorseNormalizedH0ProductSession(
        ExactDirectMorseNormalizedH0ProductSession&&) noexcept = default;
ExactDirectMorseNormalizedH0ProductSession&
ExactDirectMorseNormalizedH0ProductSession::operator=(
    ExactDirectMorseNormalizedH0ProductSession&&) noexcept = default;
ExactDirectMorseNormalizedH0ProductSession::
    ExactDirectMorseNormalizedH0ProductSession(
        std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

contract::CanonicalId
ExactDirectMorseNormalizedH0ProductSession::
    resident_scientific_source_digest(
        const ExactDirectSparseUnifiedLevelPlanResult& plan,
        ExactDirectMorseUnifiedResidentSourceKind source_kind) {
  return ExactDirectMorseResidentAtLeast20ProjectionSeam::
      scientific_source_digest(plan, source_kind);
}

ExactNormalizedHartiganLevelInitializationResult
ExactDirectMorseNormalizedH0ProductSession::
    initialize_owned_streaming_hartigan(
        std::uint64_t authority_id,
        std::size_t maximum_order,
        const contract::CanonicalId& scientific_digest,
        const contract::CanonicalId& initial_chain_digest,
        const ExactNormalizedHartiganLevelManifestBudget& budget) {
  return ExactNormalizedHartiganLevelManifestSession::
      initialize_streaming_manifest_session(
          authority_id,
          maximum_order,
          scientific_digest,
          initial_chain_digest,
          budget);
}

bool ExactDirectMorseNormalizedH0ProductSession::ready() const noexcept {
  const bool hartigan_state_ready =
      impl_ != nullptr &&
      (impl_->final_seal != nullptr ? impl_->hartigan.sealed()
                                    : impl_->hartigan.certified_ready());
  return impl_ != nullptr && impl_->initialized && impl_->seal != nullptr &&
      impl_->registry != nullptr && impl_->k1.certified_stream() &&
      impl_->vertical.ready() && impl_->higher_at_least20.initialized() &&
      hartigan_state_ready &&
      impl_->product_batch_cursor <= impl_->requirements.product_batch_count &&
      impl_->epoch == impl_->product_batch_cursor &&
      impl_->hartigan_record_count <= impl_->product_batch_cursor &&
      impl_->hartigan.record_count() == impl_->hartigan_record_count &&
      !id_is_zero(impl_->scientific_digest) &&
      !id_is_zero(impl_->initial_product_chain_digest) &&
      !id_is_zero(impl_->product_chain_digest) &&
      !id_is_zero(impl_->hartigan_chain_digest);
}

bool ExactDirectMorseNormalizedH0ProductSession::complete() const noexcept {
  return ready() && impl_->k1.complete() && impl_->vertical.resident_complete() &&
      impl_->product_batch_cursor == impl_->requirements.product_batch_count;
}

bool ExactDirectMorseNormalizedH0ProductSession::sealed() const noexcept {
  return impl_ != nullptr && impl_->final_seal != nullptr;
}

ExactDirectMorseNormalizedH0ProductNextSource
ExactDirectMorseNormalizedH0ProductSession::next_source() const noexcept {
  if (!ready() || sealed()) {
    return ExactDirectMorseNormalizedH0ProductNextSource::complete;
  }
  const bool k1_done = impl_->k1.complete();
  const bool resident_done = impl_->vertical.resident_complete();
  if (k1_done && resident_done) {
    return ExactDirectMorseNormalizedH0ProductNextSource::complete;
  }
  if (resident_done) {
    return ExactDirectMorseNormalizedH0ProductNextSource::k1;
  }
  if (k1_done) {
    return ExactDirectMorseNormalizedH0ProductNextSource::resident;
  }
  exact::ExactLevel k1_level{};
  if (impl_->k1.next_batch_cursor() != 0U) {
    const auto& forest = impl_->k1.private_forest();
    const auto& batch =
        forest.equal_level_batches[impl_->k1.next_batch_cursor() - 1U];
    k1_level = forest.levels[batch.level_index];
  }
  const auto resident_cursor =
      impl_->vertical.current_stamp().resident_batch_cursor;
  const auto& resident_batch =
      impl_->vertical.resident_plan().batches[resident_cursor];
  return k1_level <= resident_batch.squared_level
             ? ExactDirectMorseNormalizedH0ProductNextSource::k1
             : ExactDirectMorseNormalizedH0ProductNextSource::resident;
}

std::size_t
ExactDirectMorseNormalizedH0ProductSession::product_batch_count()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->requirements.product_batch_count;
}

std::size_t
ExactDirectMorseNormalizedH0ProductSession::product_batch_cursor()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->product_batch_cursor;
}

std::size_t
ExactDirectMorseNormalizedH0ProductSession::hartigan_record_count()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->hartigan_record_count;
}

std::size_t ExactDirectMorseNormalizedH0ProductSession::maximum_order()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->requirements.maximum_order;
}

std::size_t
ExactDirectMorseNormalizedH0ProductSession::outstanding_prepared_batch_count()
    const noexcept {
  return impl_ == nullptr || impl_->registry == nullptr
             ? 0U
             : impl_->registry->live_ticket_count;
}

const contract::CanonicalId&
ExactDirectMorseNormalizedH0ProductSession::scientific_digest()
    const noexcept {
  static const contract::CanonicalId empty{};
  return impl_ == nullptr ? empty : impl_->scientific_digest;
}

const contract::CanonicalId&
ExactDirectMorseNormalizedH0ProductSession::product_chain_digest()
    const noexcept {
  static const contract::CanonicalId empty{};
  return impl_ == nullptr ? empty : impl_->product_chain_digest;
}

const contract::CanonicalId&
ExactDirectMorseNormalizedH0ProductSession::hartigan_chain_digest()
    const noexcept {
  static const contract::CanonicalId empty{};
  return impl_ == nullptr ? empty : impl_->hartigan_chain_digest;
}

ExactDirectMorseNormalizedH0ProductStamp
ExactDirectMorseNormalizedH0ProductSession::current_stamp() const noexcept {
  ExactDirectMorseNormalizedH0ProductStamp output;
  if (impl_ == nullptr) {
    return output;
  }
  output.session_instance_id = impl_->session_instance_id;
  output.product_batch_cursor = impl_->product_batch_cursor;
  output.epoch = impl_->epoch;
  output.hartigan_record_count = impl_->hartigan_record_count;
  output.k1_batch_cursor = impl_->k1.next_batch_cursor();
  output.resident_batch_cursor =
      impl_->vertical.current_stamp().resident_batch_cursor;
  output.scientific_digest = impl_->scientific_digest;
  output.initial_product_chain_digest = impl_->initial_product_chain_digest;
  output.product_chain_digest = impl_->product_chain_digest;
  output.hartigan_chain_digest = impl_->hartigan_chain_digest;
  return output;
}

const ExactDirectK1NormalizedProductStreamSession&
ExactDirectMorseNormalizedH0ProductSession::k1() const noexcept {
  static const ExactDirectK1NormalizedProductStreamSession empty{};
  return impl_ == nullptr ? empty : impl_->k1;
}

const ExactDirectMorseResidentAllOrdersVerticalBridge&
ExactDirectMorseNormalizedH0ProductSession::vertical() const noexcept {
  static const ExactDirectMorseResidentAllOrdersVerticalBridge empty{};
  return impl_ == nullptr ? empty : impl_->vertical;
}

const ExactDirectAtLeast20StreamViewSession&
ExactDirectMorseNormalizedH0ProductSession::higher_at_least20()
    const noexcept {
  static const ExactDirectAtLeast20StreamViewSession empty{};
  return impl_ == nullptr ? empty : impl_->higher_at_least20;
}

const ExactNormalizedHartiganLevelManifestSession&
ExactDirectMorseNormalizedH0ProductSession::hartigan() const noexcept {
  static const ExactNormalizedHartiganLevelManifestSession empty{};
  return impl_ == nullptr ? empty : impl_->hartigan;
}

bool ExactDirectMorseNormalizedH0ProductPreparationResult::
    certified_prepared_batch() const noexcept {
  return ticket.has_value() && ticket->valid() &&
      source != ExactDirectMorseNormalizedH0ProductNextSource::complete &&
      emits_hartigan_record == ticket->emits_hartigan_record() &&
      no_scientific_state_mutated && all_fallible_work_completed &&
      no_global_facet_coface_or_gamma_catalog_materialized &&
      decision == ExactDirectMorseNormalizedH0ProductPreparationDecision::
                      complete_prepared_atomic_product_batch;
}

bool ExactDirectMorseNormalizedH0ProductCommitResult::
    certified_atomic_product_commit() const noexcept {
  const bool k1_ok =
      source == ExactDirectMorseNormalizedH0ProductNextSource::k1 &&
      k1_commit.has_value() && k1_commit->certified_atomic_batch_commit() &&
      !resident_commit.has_value() && !higher_at_least20_commit.has_value();
  const bool resident_ok =
      source == ExactDirectMorseNormalizedH0ProductNextSource::resident &&
      !k1_commit.has_value() && resident_commit.has_value() &&
      resident_commit->certified_committed_batch() &&
      higher_at_least20_commit.has_value() &&
      higher_at_least20_commit->certified_atomic_downstream_batch_fold();
  const bool hartigan_ok =
      emits_hartigan_record
          ? hartigan_commit.has_value() &&
                hartigan_commit->certified_appended_record()
          : !hartigan_commit.has_value();
  return (k1_ok || resident_ok) && hartigan_ok && ticket_consumed &&
      state_mutated && no_allocation_after_preparation &&
      product_chain_advanced &&
      pre_stamp.product_batch_cursor + 1U ==
          post_stamp.product_batch_cursor &&
      pre_stamp.product_chain_digest != post_stamp.product_chain_digest &&
      !public_status_claimed &&
      decision == ExactDirectMorseNormalizedH0ProductCommitDecision::
                      complete_committed_atomic_product_batch;
}

bool ExactDirectMorseNormalizedH0ProductCheckpointResult::
    certified_nonresumable_semantic_snapshot() const noexcept {
  if (!checkpoint.has_value() ||
      decision != ExactDirectMorseNormalizedH0ProductCheckpointDecision::
                      complete_semantic_snapshot_restart_explicitly_unavailable) {
    return false;
  }
  try {
    return semantic_checkpoint_links_certified(*checkpoint);
  } catch (...) {
    return false;
  }
}

bool ExactDirectMorseNormalizedH0ProductSealResult::
    certified_bounded_product_seal() const noexcept {
  if (seal == nullptr ||
      final_seal_published == existing_final_seal_returned_without_mutation ||
      decision != ExactDirectMorseNormalizedH0ProductSealDecision::
                      complete_certified_bounded_normalized_product_seal) {
    return false;
  }
  try {
    return final_seal_links_certified(*seal);
  } catch (...) {
    return false;
  }
}

bool ExactDirectMorseNormalizedH0ProductInitialization::
    certified_initialized_session() const noexcept {
  return session.ready() && source_sessions_consumed &&
      exact_level_order_scheduler_certified &&
      no_global_facet_coface_or_gamma_catalog_materialized &&
      !public_status_claimed &&
      requirements.product_batch_count <=
          requested_budget.maximum_product_batch_count &&
      decision == ExactDirectMorseNormalizedH0ProductInitializationDecision::
                      complete_certified_bounded_product_session;
}

ExactDirectMorseNormalizedH0ProductInitialization
initialize_exact_direct_morse_normalized_h0_product_session(
    ExactDirectMorseResidentAllOrdersVerticalBridge&& vertical,
    ExactDirectK1NormalizedProductStreamSession&& k1,
    std::uint64_t product_authority_id,
    const ExactDirectMorseNormalizedH0ProductBudget& budget) noexcept {
  ExactDirectMorseNormalizedH0ProductInitialization output;
  output.requested_budget = budget;
  try {
    if (!vertical.ready() || !k1.certified_stream() ||
        product_authority_id == 0U || vertical.final_vertical_sealed() ||
        k1.outstanding_prepared_batch_count() != 0U) {
      output.decision =
          ExactDirectMorseNormalizedH0ProductInitializationDecision::
              no_source_session_rejected;
      return output;
    }
    const auto vertical_stamp = vertical.current_stamp();
    if (vertical_stamp.resident_batch_cursor != 0U ||
        vertical_stamp.resident_epoch != 0U ||
        vertical_stamp.committed_k2_batch_count != 0U ||
        vertical_stamp.committed_higher_batch_count != 0U ||
        !vertical.resident_group_records().empty() ||
        k1.next_batch_cursor() != 0U || k1.epoch() != 0U) {
      output.decision =
          ExactDirectMorseNormalizedH0ProductInitializationDecision::
              no_non_genesis_source_rejected;
      return output;
    }
    if (!vertical.resident_normalized_direct_source_session() ||
        vertical.resident_source_kind() !=
            ExactDirectMorseUnifiedResidentSourceKind::
                normalized_direct_h0_candidate_source ||
        vertical.resident_normalized_h0_retraction_mode() !=
            ExactDirectNormalizedH0ResidentRetractionMode::
                certified_horizontal_incidence_complete_rank_window_and_sparse_strict_facet_closure ||
        !vertical
             .resident_normalized_horizontal_incidence_reduction_certified()) {
      output.decision =
          ExactDirectMorseNormalizedH0ProductInitializationDecision::
              no_normalized_v7_source_premise_rejected;
      return output;
    }
    const auto& plan = vertical.resident_plan();
    if (!honest_normalized_compatibility_cursor(plan) ||
        vertical_stamp.canonical_cloud_digest != k1.canonical_cloud_digest() ||
        !vertical.verify_owned_k1_source_forest_digest(
            k1.source_forest_digest()) ||
        id_is_zero(plan.source_pair_canonical_cloud_digest) ||
        id_is_zero(plan.source_higher_canonical_cloud_digest) ||
        id_is_zero(plan.source_pair_semantic_digest) ||
        id_is_zero(plan.source_higher_semantic_digest)) {
      output.decision =
          ExactDirectMorseNormalizedH0ProductInitializationDecision::
              no_source_identity_mismatch;
      return output;
    }

    output.requirements.k1_batch_count = k1.total_batch_count();
    output.requirements.resident_batch_count = plan.batches.size();
    if (!checked_add(
            output.requirements.k1_batch_count,
            output.requirements.resident_batch_count,
            output.requirements.product_batch_count)) {
      output.decision =
          ExactDirectMorseNormalizedH0ProductInitializationDecision::
              no_product_capacity_overflow;
      return output;
    }
    output.requirements.maximum_order = 1U;
    for (const auto& batch : plan.batches) {
      if (batch.order < 2U ||
          batch.order >
              normalized_exact_hartigan_level_manifest_maximum_order) {
        output.decision =
            ExactDirectMorseNormalizedH0ProductInitializationDecision::
                no_source_identity_mismatch;
        return output;
      }
      output.requirements.maximum_order =
          std::max(output.requirements.maximum_order, batch.order);
    }
    if (budget.maximum_outstanding_prepared_batch_count == 0U ||
        output.requirements.product_batch_count == 0U ||
        output.requirements.product_batch_count >
            budget.maximum_product_batch_count ||
        output.requirements.product_batch_count >
            budget.hartigan.maximum_record_count) {
      output.decision =
          ExactDirectMorseNormalizedH0ProductInitializationDecision::
              no_product_budget_rejected;
      return output;
    }

    const auto higher_scientific_digest =
        ExactDirectMorseNormalizedH0ProductSession::
            resident_scientific_source_digest(
                plan, vertical.resident_source_kind());
    if (id_is_zero(higher_scientific_digest)) {
      output.decision =
          ExactDirectMorseNormalizedH0ProductInitializationDecision::
              no_source_identity_mismatch;
      return output;
    }
    auto higher_view_initialization =
        initialize_exact_direct_at_least20_stream_view_session(
            vertical_stamp.resident_session_authority_id,
            0U,
            0U,
            higher_scientific_digest,
            contract::CanonicalId{},
            budget.higher_at_least20);
    if (!higher_view_initialization.certified_initialized_session() ||
        !higher_view_initialization.session.has_value()) {
      output.decision =
          ExactDirectMorseNormalizedH0ProductInitializationDecision::
              no_higher_view_initialization_rejected;
      return output;
    }

    const auto scientific_digest = compute_scientific_digest(
        vertical,
        k1,
        higher_scientific_digest,
        output.requirements.maximum_order,
        output.requirements.product_batch_count);
    const auto initial_chain_digest = compute_initial_chain_digest(
        scientific_digest,
        k1.scientific_chain_digest(),
        higher_scientific_digest);
    if (id_is_zero(scientific_digest) || id_is_zero(initial_chain_digest)) {
      output.decision =
          ExactDirectMorseNormalizedH0ProductInitializationDecision::
              no_source_identity_mismatch;
      return output;
    }
    auto hartigan_initialization =
        ExactDirectMorseNormalizedH0ProductSession::
            initialize_owned_streaming_hartigan(
                product_authority_id,
                output.requirements.maximum_order,
                scientific_digest,
                initial_chain_digest,
                budget.hartigan);
    if (!hartigan_initialization.certified_initialized_session() ||
        !hartigan_initialization.session.has_value()) {
      output.decision =
          ExactDirectMorseNormalizedH0ProductInitializationDecision::
              no_hartigan_initialization_rejected;
      return output;
    }

    const std::uint64_t instance_id = allocate_session_instance_id();
    if (instance_id == 0U) {
      output.decision =
          ExactDirectMorseNormalizedH0ProductInitializationDecision::
              no_session_instance_id_exhausted;
      return output;
    }
    auto impl =
        std::make_unique<ExactDirectMorseNormalizedH0ProductSession::Impl>();
    impl->seal = std::make_shared<const ProductSessionSeal>();
    impl->registry = std::make_shared<ProductTicketRegistry>();
    impl->vertical = std::move(vertical);
    impl->k1 = std::move(k1);
    impl->higher_at_least20 =
        std::move(higher_view_initialization.session.value());
    impl->hartigan = std::move(hartigan_initialization.session.value());
    impl->budget = budget;
    impl->requirements = output.requirements;
    impl->higher_scientific_digest = higher_scientific_digest;
    impl->scientific_digest = scientific_digest;
    impl->initial_product_chain_digest = initial_chain_digest;
    impl->product_chain_digest = initial_chain_digest;
    impl->hartigan_chain_digest = initial_chain_digest;
    impl->session_instance_id = instance_id;
    impl->product_authority_id = product_authority_id;
    impl->initialized = true;

    output.scientific_digest = scientific_digest;
    output.initial_product_chain_digest = initial_chain_digest;
    output.session = ExactDirectMorseNormalizedH0ProductSession{
        std::move(impl)};
    if (!output.session.ready()) {
      output = {};
      output.requested_budget = budget;
      output.decision =
          ExactDirectMorseNormalizedH0ProductInitializationDecision::
              no_source_session_rejected;
      return output;
    }
    output.source_sessions_consumed = true;
    output.exact_level_order_scheduler_certified = true;
    output.no_global_facet_coface_or_gamma_catalog_materialized = true;
    output.public_status_claimed = false;
    output.decision =
        ExactDirectMorseNormalizedH0ProductInitializationDecision::
            complete_certified_bounded_product_session;
    return output;
  } catch (const std::bad_alloc&) {
    output.decision =
        ExactDirectMorseNormalizedH0ProductInitializationDecision::
            no_allocation_failed;
    return output;
  } catch (const std::exception&) {
    output.decision =
        ExactDirectMorseNormalizedH0ProductInitializationDecision::
            no_source_session_rejected;
    return output;
  }
}

ExactDirectMorseNormalizedH0ProductPreparationResult
ExactDirectMorseNormalizedH0ProductSession::prepare_next() noexcept {
  ExactDirectMorseNormalizedH0ProductPreparationResult output;
  const auto reject = [&output](
                          ExactDirectMorseNormalizedH0ProductPreparationDecision
                              decision) {
    output.no_scientific_state_mutated = true;
    output.all_fallible_work_completed = false;
    output.no_global_facet_coface_or_gamma_catalog_materialized = true;
    output.decision = decision;
    return std::move(output);
  };
  if (!ready() || sealed()) {
    return reject(
        ExactDirectMorseNormalizedH0ProductPreparationDecision::
            no_session_not_ready);
  }
  output.source = next_source();
  if (output.source ==
      ExactDirectMorseNormalizedH0ProductNextSource::complete) {
    return reject(
        ExactDirectMorseNormalizedH0ProductPreparationDecision::
            no_product_complete);
  }
  if (impl_->registry->live_ticket_count >=
          impl_->budget.maximum_outstanding_prepared_batch_count ||
      impl_->registry->live_ticket_count != 0U) {
    return reject(
        ExactDirectMorseNormalizedH0ProductPreparationDecision::
            no_outstanding_ticket_budget_exhausted);
  }

  try {
    auto prepared =
        std::make_unique<ExactDirectMorseNormalizedH0ProductPreparedBatch::
                             Impl>();
    prepared->seal = impl_->seal;
    prepared->registry = impl_->registry;
    prepared->expected_stamp = current_stamp();
    prepared->source = output.source;
    prepared->normalized_batch_index = impl_->product_batch_cursor;

    ExactNormalizedHartiganLevelInput hartigan_input;
    hartigan_input.normalized_batch_index = impl_->hartigan_record_count;
    hartigan_input.previous_normalized_batch_chain_digest =
        impl_->hartigan_chain_digest;
    hartigan_input.q_r = 0U;
    hartigan_input.group_summary_encoding =
        ExactNormalizedHartiganGroupSummaryEncoding::
            complete_batch_qr_partition_v4;
    contract::CanonicalId component_chain_digest{};
    std::size_t source_order_batch_index{};

    if (output.source ==
        ExactDirectMorseNormalizedH0ProductNextSource::k1) {
      auto source_preparation = impl_->k1.prepare_next();
      if (!source_preparation.certified_prepared_batch() ||
          !source_preparation.ticket.has_value()) {
        return reject(
            ExactDirectMorseNormalizedH0ProductPreparationDecision::
                no_source_preparation_rejected);
      }
      const auto& record = source_preparation.ticket->prepared_record();
      if (record.normalized_batch_index != impl_->k1.next_batch_cursor() ||
          record.order != 1U ||
          record.order_batch_index != impl_->record_count_by_order[0U]) {
        return reject(
            ExactDirectMorseNormalizedH0ProductPreparationDecision::
                no_source_schedule_mismatch);
      }
      prepared->squared_level = record.squared_level;
      prepared->order = record.order;
      prepared->order_batch_index = record.order_batch_index;
      prepared->emits_hartigan_record = true;
      source_order_batch_index = record.order_batch_index;
      hartigan_input.order = record.order;
      hartigan_input.order_batch_index = record.order_batch_index;
      hartigan_input.squared_level = record.squared_level;
      hartigan_input.core_facet_delta_count =
          record.core_facet_delta_count;
      hartigan_input.point_delta_count = record.point_delta_count;
      hartigan_input.parent_delta_count = record.parent_delta_count;
      hartigan_input.node_delta_count = record.node_delta_count;
      hartigan_input.group_count = record.group_count;
      hartigan_input.qr0_group_count = record.qr0_group_count;
      hartigan_input.qr1_group_count = record.qr1_group_count;
      hartigan_input.qr_greater_equal2_group_count =
          record.qr_greater_equal2_group_count;
      hartigan_input.disposition =
          record.group_count == 1U && record.qr1_group_count == 1U &&
                  record.qr0_group_count == 0U &&
                  record.qr_greater_equal2_group_count == 0U &&
                  record.core_facet_delta_count == 0U &&
                  record.point_delta_count == 0U &&
                  record.parent_delta_count == 0U &&
                  record.node_delta_count == 0U
              ? ExactNormalizedHartiganBatchDisposition::
                    omitted_certified_qr1_noop
              : ExactNormalizedHartiganBatchDisposition::
                    materialized_normalized_equal_level_batch;
      component_chain_digest = record.scientific_chain_digest;
      prepared->k1_ticket.emplace(
          std::move(source_preparation.ticket.value()));
    } else {
      auto source_preparation = impl_->vertical.prepare_next();
      if (!source_preparation.certified_prepared_batch() ||
          !source_preparation.ticket.has_value()) {
        return reject(
            ExactDirectMorseNormalizedH0ProductPreparationDecision::
                no_source_preparation_rejected);
      }
      const auto& bundle =
          source_preparation.ticket->resident_authority_bundle();
      const auto vertical_stamp = impl_->vertical.current_stamp();
      const auto& scheduled =
          impl_->vertical.resident_plan().batches
              [vertical_stamp.resident_batch_cursor];
      if (!bundle.certified_strict_pre_batch_bundle() ||
          bundle.identity.session_authority_id !=
              impl_->higher_at_least20.source_session_authority_id() ||
          bundle.identity.batch_cursor !=
              vertical_stamp.resident_batch_cursor ||
          bundle.identity.epoch != vertical_stamp.resident_epoch ||
          bundle.source_batch_index != scheduled.batch_index ||
          bundle.order != scheduled.order ||
          bundle.squared_level != scheduled.squared_level ||
          bundle.order < 2U ||
          bundle.order > impl_->requirements.maximum_order) {
        return reject(
            ExactDirectMorseNormalizedH0ProductPreparationDecision::
                no_source_schedule_mismatch);
      }

      auto projection_result =
          ExactDirectMorseResidentAtLeast20ProjectionSeam::
              project_verified_bundle(
                  bundle,
                  impl_->higher_at_least20.source_session_authority_id(),
                  impl_->higher_scientific_digest,
                  impl_->higher_at_least20.source_commit_digest(),
                  impl_->vertical.resident_group_records().size(),
                  impl_->next_higher_root_id,
                  impl_->budget.higher_at_least20);
      if (projection_result.decision !=
              ExactDirectMorseResidentAtLeast20ProjectionSeam::Decision::
                  complete ||
          !projection_result.projection.has_value()) {
        return reject(
            ExactDirectMorseNormalizedH0ProductPreparationDecision::
                no_higher_projection_rejected);
      }
      auto projection = std::move(projection_result.projection.value());
      auto view_preparation = impl_->higher_at_least20.prepare_resident_batch(
          std::move(projection.view_input));
      if (!view_preparation.certified_prepared_batch() ||
          !view_preparation.ticket.has_value() ||
          !view_preparation.resident_commit_capability_bound ||
          id_is_zero(view_preparation.result.source_commit_digest) ||
          view_preparation.result.source_commit_digest ==
              impl_->higher_at_least20.source_commit_digest()) {
        return reject(
            ExactDirectMorseNormalizedH0ProductPreparationDecision::
                no_higher_view_preparation_rejected);
      }

      const auto& frozen_counters = bundle.frozen_batch.counters;
      const auto& action_counters =
          bundle.frozen_batch.action_plan.counters;
      if (action_counters.prior_root_reference_count <
          action_counters.continuation_group_count) {
        return reject(
            ExactDirectMorseNormalizedH0ProductPreparationDecision::
                no_higher_projection_rejected);
      }
      std::size_t node_delta_count = 0U;
      if (!checked_add(
              action_counters.reduced_birth_group_count,
              action_counters.multifusion_group_count,
              node_delta_count)) {
        return reject(
            ExactDirectMorseNormalizedH0ProductPreparationDecision::
                no_higher_projection_rejected);
      }
      prepared->squared_level = bundle.squared_level;
      prepared->order = bundle.order;
      prepared->order_batch_index =
          impl_->record_count_by_order[bundle.order - 1U];
      prepared->emits_hartigan_record =
          frozen_counters.group_count != 0U;
      source_order_batch_index = bundle.source_batch_index;
      prepared->resident_group_record_count_before =
          projection.group_record_count_before;
      prepared->resident_source_batch_index = projection.source_batch_index;
      prepared->resident_source_epoch = projection.source_epoch;
      prepared->next_root_id_before = projection.next_root_id_before;
      prepared->next_root_id_after = projection.next_root_id_after;
      prepared->expected_higher_source_commit_digest =
          view_preparation.result.source_commit_digest;
      prepared->expected_groups = std::move(projection.expected_groups);
      hartigan_input.order = prepared->order;
      hartigan_input.order_batch_index = prepared->order_batch_index;
      hartigan_input.squared_level = prepared->squared_level;
      hartigan_input.core_facet_delta_count =
          frozen_counters.coverage_delta_facet_reference_count;
      hartigan_input.point_delta_count =
          frozen_counters.coverage_delta_point_reference_count;
      hartigan_input.parent_delta_count =
          action_counters.prior_root_reference_count -
          action_counters.continuation_group_count;
      hartigan_input.node_delta_count = node_delta_count;
      hartigan_input.group_count = frozen_counters.group_count;
      hartigan_input.qr0_group_count = frozen_counters.q_r_zero_group_count;
      hartigan_input.qr1_group_count = frozen_counters.q_r_one_group_count;
      hartigan_input.qr_greater_equal2_group_count =
          frozen_counters.q_r_multiple_group_count;
      hartigan_input.disposition =
          hartigan_input.group_count == 1U &&
                  hartigan_input.qr1_group_count == 1U &&
                  hartigan_input.qr0_group_count == 0U &&
                  hartigan_input.qr_greater_equal2_group_count == 0U &&
                  hartigan_input.core_facet_delta_count == 0U &&
                  hartigan_input.point_delta_count == 0U &&
                  hartigan_input.parent_delta_count == 0U &&
                  hartigan_input.node_delta_count == 0U
              ? ExactNormalizedHartiganBatchDisposition::
                    omitted_certified_qr1_noop
              : ExactNormalizedHartiganBatchDisposition::
                    materialized_normalized_equal_level_batch;
      component_chain_digest =
          prepared->expected_higher_source_commit_digest;
      prepared->resident_ticket.emplace(
          std::move(source_preparation.ticket.value()));
      prepared->view_ticket.emplace(
          std::move(view_preparation.ticket.value()));
    }

    auto source_transition_input = hartigan_input;
    source_transition_input.normalized_batch_index =
        impl_->product_batch_cursor;
    source_transition_input.order_batch_index = source_order_batch_index;
    source_transition_input.previous_normalized_batch_chain_digest =
        impl_->product_chain_digest;
    prepared->next_product_chain_digest = compute_batch_chain_digest(
        source_transition_input, output.source, component_chain_digest);
    prepared->next_hartigan_chain_digest = impl_->hartigan_chain_digest;
    if (prepared->emits_hartigan_record) {
      if (impl_->hartigan_record_count ==
              std::numeric_limits<std::size_t>::max() ||
          impl_->record_count_by_order[prepared->order - 1U] ==
              std::numeric_limits<std::size_t>::max()) {
        return reject(
            ExactDirectMorseNormalizedH0ProductPreparationDecision::
                no_hartigan_preparation_rejected);
      }
      hartigan_input.normalized_batch_index = impl_->hartigan_record_count;
      hartigan_input.order_batch_index =
          impl_->record_count_by_order[prepared->order - 1U];
      hartigan_input.previous_normalized_batch_chain_digest =
          impl_->hartigan_chain_digest;
      hartigan_input.normalized_batch_chain_digest =
          prepared->next_product_chain_digest;
      auto hartigan_preparation =
          impl_->hartigan.prepare_append(hartigan_input);
      output.hartigan_decision = hartigan_preparation.decision;
      if (!hartigan_preparation.certified_prepared_append() ||
          !hartigan_preparation.ticket.has_value()) {
        return reject(
            ExactDirectMorseNormalizedH0ProductPreparationDecision::
                no_hartigan_preparation_rejected);
      }
      prepared->next_hartigan_chain_digest =
          hartigan_input.normalized_batch_chain_digest;
      prepared->hartigan_ticket.emplace(
          std::move(hartigan_preparation.ticket.value()));
    }
    prepared->owns_ticket_slot = true;
    ++impl_->registry->live_ticket_count;
    output.emits_hartigan_record = prepared->emits_hartigan_record;
    output.ticket.emplace(
        ExactDirectMorseNormalizedH0ProductPreparedBatch{
            std::move(prepared)});
    output.no_scientific_state_mutated = true;
    output.all_fallible_work_completed = true;
    output.no_global_facet_coface_or_gamma_catalog_materialized = true;
    output.decision =
        ExactDirectMorseNormalizedH0ProductPreparationDecision::
            complete_prepared_atomic_product_batch;
    return output;
  } catch (const std::bad_alloc&) {
    return reject(
        ExactDirectMorseNormalizedH0ProductPreparationDecision::
            no_allocation_failed);
  } catch (const std::length_error&) {
    return reject(
        ExactDirectMorseNormalizedH0ProductPreparationDecision::
            no_allocation_failed);
  } catch (const std::exception&) {
    return reject(
        ExactDirectMorseNormalizedH0ProductPreparationDecision::
            no_source_preparation_rejected);
  }
}

ExactDirectMorseNormalizedH0ProductCommitResult
ExactDirectMorseNormalizedH0ProductSession::commit(
    ExactDirectMorseNormalizedH0ProductPreparedBatch&& ticket) noexcept {
  ExactDirectMorseNormalizedH0ProductCommitResult output;
  if (!ready() || sealed()) {
    output.decision =
        ExactDirectMorseNormalizedH0ProductCommitDecision::
            no_session_not_ready;
    return output;
  }
  auto prepared = std::move(ticket.impl_);
  if (prepared == nullptr || !prepared->owns_ticket_slot ||
      prepared->consumed || prepared->registry == nullptr ||
      prepared->registry->live_ticket_count != 1U ||
      (prepared->emits_hartigan_record !=
       prepared->hartigan_ticket.has_value()) ||
      (prepared->hartigan_ticket.has_value() &&
       !prepared->hartigan_ticket->valid())) {
    output.decision =
        ExactDirectMorseNormalizedH0ProductCommitDecision::
            no_ticket_not_valid;
    return output;
  }
  const auto release_slot = [&prepared]() noexcept {
    if (!prepared->owns_ticket_slot) {
      return;
    }
    if (prepared->registry == nullptr ||
        prepared->registry->live_ticket_count == 0U) {
      std::terminate();
    }
    --prepared->registry->live_ticket_count;
    prepared->owns_ticket_slot = false;
  };
  prepared->consumed = true;
  output.ticket_consumed = true;
  output.source = prepared->source;
  output.emits_hartigan_record = prepared->emits_hartigan_record;
  output.pre_stamp = current_stamp();
  if (prepared->seal.get() != impl_->seal.get() ||
      prepared->expected_stamp != output.pre_stamp ||
      prepared->source != next_source() ||
      prepared->normalized_batch_index != impl_->product_batch_cursor ||
      prepared->next_product_chain_digest == impl_->product_chain_digest ||
      (prepared->emits_hartigan_record &&
       prepared->next_hartigan_chain_digest ==
           impl_->hartigan_chain_digest) ||
      (!prepared->emits_hartigan_record &&
       prepared->next_hartigan_chain_digest !=
           impl_->hartigan_chain_digest)) {
    release_slot();
    output.decision =
        ExactDirectMorseNormalizedH0ProductCommitDecision::
            no_foreign_or_stale_ticket_rejected;
    return output;
  }

  if (prepared->source ==
      ExactDirectMorseNormalizedH0ProductNextSource::k1) {
    if (!prepared->k1_ticket.has_value() ||
        !prepared->k1_ticket->valid()) {
      release_slot();
      output.decision =
          ExactDirectMorseNormalizedH0ProductCommitDecision::
              no_ticket_not_valid;
      return output;
    }
    auto source_commit =
        impl_->k1.commit(std::move(prepared->k1_ticket.value()));
    if (!source_commit.certified_atomic_batch_commit()) {
      output.k1_commit.emplace(std::move(source_commit));
      release_slot();
      output.decision =
          ExactDirectMorseNormalizedH0ProductCommitDecision::
              no_source_commit_rejected;
      return output;
    }
    output.k1_commit.emplace(std::move(source_commit));
  } else if (prepared->source ==
             ExactDirectMorseNormalizedH0ProductNextSource::resident) {
    if (!prepared->resident_ticket.has_value() ||
        !prepared->resident_ticket->valid() ||
        !prepared->view_ticket.has_value() ||
        !prepared->view_ticket->valid()) {
      release_slot();
      output.decision =
          ExactDirectMorseNormalizedH0ProductCommitDecision::
              no_ticket_not_valid;
      return output;
    }
    auto source_commit = impl_->vertical.commit(
        std::move(prepared->resident_ticket.value()));
    if (!source_commit.certified_committed_batch()) {
      auto rejected_view = impl_->higher_at_least20.commit_prepared_batch(
          std::move(prepared->view_ticket.value()), false);
      output.resident_commit.emplace(std::move(source_commit));
      output.higher_at_least20_commit.emplace(std::move(rejected_view));
      release_slot();
      output.decision =
          ExactDirectMorseNormalizedH0ProductCommitDecision::
              no_source_commit_rejected;
      return output;
    }

    const auto post_vertical_stamp = impl_->vertical.current_stamp();
    if (prepared->next_root_id_before != impl_->next_higher_root_id ||
        post_vertical_stamp.resident_batch_cursor !=
            prepared->resident_source_batch_index + 1U ||
        post_vertical_stamp.resident_epoch !=
            prepared->resident_source_epoch + 1U ||
        !resident_suffix_matches(
            prepared->resident_group_record_count_before,
            prepared->resident_source_batch_index,
            prepared->order,
            prepared->squared_level,
            prepared->expected_groups,
            impl_->vertical.resident_group_records())) {
      std::terminate();
    }
    auto view_commit = impl_->higher_at_least20.commit_prepared_batch(
        std::move(prepared->view_ticket.value()), true);
    if (!view_commit.certified_atomic_downstream_batch_fold() ||
        !view_commit.upstream_source_commit_capability_verified ||
        view_commit.source_batch_cursor_after !=
            post_vertical_stamp.resident_batch_cursor ||
        view_commit.source_epoch_after != post_vertical_stamp.resident_epoch ||
        view_commit.source_commit_digest !=
            prepared->expected_higher_source_commit_digest ||
        impl_->higher_at_least20.next_source_batch_cursor() !=
            post_vertical_stamp.resident_batch_cursor ||
        impl_->higher_at_least20.source_epoch() !=
            post_vertical_stamp.resident_epoch) {
      std::terminate();
    }
    impl_->next_higher_root_id = prepared->next_root_id_after;
    output.resident_commit.emplace(std::move(source_commit));
    output.higher_at_least20_commit.emplace(std::move(view_commit));
  } else {
    release_slot();
    output.decision =
        ExactDirectMorseNormalizedH0ProductCommitDecision::
            no_ticket_not_valid;
    return output;
  }

  // From this point the selected source is irreversible.  A non-empty
  // normalized batch owns a fully prepared allocation-free Hartigan commit.
  // An empty structural resident transit deliberately owns no such ticket and
  // cannot mint either a materialized record or a fake qR=1 omission.
  if (impl_->product_batch_cursor ==
          std::numeric_limits<std::size_t>::max() ||
      impl_->epoch == std::numeric_limits<std::size_t>::max()) {
    std::terminate();
  }
  if (prepared->emits_hartigan_record) {
    if (!prepared->hartigan_ticket.has_value()) {
      std::terminate();
    }
    const bool omitted_noop =
        prepared->hartigan_ticket->prepared_record().input.disposition ==
        ExactNormalizedHartiganBatchDisposition::omitted_certified_qr1_noop;
    auto hartigan_commit = impl_->hartigan.commit(
        std::move(prepared->hartigan_ticket.value()));
    if (!hartigan_commit.certified_appended_record() ||
        !hartigan_commit.record.has_value() ||
        hartigan_commit.record->input.normalized_batch_index !=
            impl_->hartigan_record_count ||
        hartigan_commit.record->input.normalized_batch_chain_digest !=
            prepared->next_hartigan_chain_digest) {
      std::terminate();
    }
    const std::size_t order_index = prepared->order - 1U;
    if (order_index >= impl_->record_count_by_order.size() ||
        impl_->record_count_by_order[order_index] ==
            std::numeric_limits<std::size_t>::max() ||
        impl_->hartigan_record_count ==
            std::numeric_limits<std::size_t>::max() ||
        (omitted_noop &&
         impl_->omitted_noop_count_by_order[order_index] ==
             std::numeric_limits<std::size_t>::max())) {
      std::terminate();
    }
    ++impl_->record_count_by_order[order_index];
    if (omitted_noop) {
      ++impl_->omitted_noop_count_by_order[order_index];
    }
    ++impl_->hartigan_record_count;
    impl_->hartigan_chain_digest = prepared->next_hartigan_chain_digest;
    output.hartigan_commit.emplace(std::move(hartigan_commit));
  }
  impl_->product_chain_digest = prepared->next_product_chain_digest;
  ++impl_->product_batch_cursor;
  ++impl_->epoch;
  release_slot();

  output.post_stamp = current_stamp();
  output.state_mutated = true;
  output.no_allocation_after_preparation = true;
  output.product_chain_advanced =
      output.pre_stamp.product_chain_digest !=
      output.post_stamp.product_chain_digest;
  output.public_status_claimed = false;
  output.decision =
      ExactDirectMorseNormalizedH0ProductCommitDecision::
          complete_committed_atomic_product_batch;
  return output;
}

ExactDirectMorseNormalizedH0ProductCheckpointResult
ExactDirectMorseNormalizedH0ProductSession::make_semantic_checkpoint()
    const noexcept {
  ExactDirectMorseNormalizedH0ProductCheckpointResult output;
  if (!ready()) {
    output.decision =
        ExactDirectMorseNormalizedH0ProductCheckpointDecision::
            no_session_not_ready;
    return output;
  }
  if (impl_->registry->live_ticket_count != 0U) {
    output.decision =
        ExactDirectMorseNormalizedH0ProductCheckpointDecision::
            no_outstanding_prepared_ticket;
    return output;
  }
  try {
    auto k1_checkpoint = impl_->k1.export_checkpoint();
    auto higher_checkpoint = impl_->higher_at_least20.make_checkpoint();
    auto hartigan_checkpoint = impl_->hartigan.make_checkpoint();
    if (!k1_checkpoint.certified_checkpoint() ||
        !k1_checkpoint.checkpoint.has_value() ||
        !higher_checkpoint.certified_checkpoint() ||
        !higher_checkpoint.checkpoint.has_value() ||
        !hartigan_checkpoint.certified_semantic_checkpoint() ||
        !hartigan_checkpoint.checkpoint.has_value()) {
      output.decision =
          ExactDirectMorseNormalizedH0ProductCheckpointDecision::
              no_component_checkpoint_rejected;
      return output;
    }
    ExactDirectMorseNormalizedH0ProductSemanticCheckpoint checkpoint;
    checkpoint.stamp = current_stamp();
    checkpoint.vertical_stamp = impl_->vertical.current_stamp();
    checkpoint.k1 = std::move(k1_checkpoint.checkpoint.value());
    checkpoint.higher_at_least20 =
        std::move(higher_checkpoint.checkpoint.value());
    checkpoint.hartigan =
        std::move(hartigan_checkpoint.checkpoint.value());
    checkpoint.record_count_by_order = impl_->record_count_by_order;
    checkpoint.omitted_noop_count_by_order =
        impl_->omitted_noop_count_by_order;
    checkpoint.vertical_restore_available = false;
    checkpoint.restart_supported = false;
    checkpoint.durable_file_codec_claimed = false;
    checkpoint.public_status_claimed = false;
    checkpoint.semantic_digest = semantic_checkpoint_digest(checkpoint);
    if (!semantic_checkpoint_links_certified(checkpoint)) {
      output.decision =
          ExactDirectMorseNormalizedH0ProductCheckpointDecision::
              no_component_checkpoint_rejected;
      return output;
    }
    output.checkpoint.emplace(std::move(checkpoint));
    output.decision =
        ExactDirectMorseNormalizedH0ProductCheckpointDecision::
            complete_semantic_snapshot_restart_explicitly_unavailable;
    return output;
  } catch (const std::bad_alloc&) {
    output.decision =
        ExactDirectMorseNormalizedH0ProductCheckpointDecision::
            no_allocation_failed;
    return output;
  } catch (const std::exception&) {
    output.decision =
        ExactDirectMorseNormalizedH0ProductCheckpointDecision::
            no_component_checkpoint_rejected;
    return output;
  }
}

ExactDirectMorseNormalizedH0ProductSealResult
ExactDirectMorseNormalizedH0ProductSession::seal() noexcept {
  ExactDirectMorseNormalizedH0ProductSealResult output;
  if (!ready()) {
    output.decision =
        ExactDirectMorseNormalizedH0ProductSealDecision::
            no_session_not_ready;
    return output;
  }
  if (impl_->registry->live_ticket_count != 0U) {
    output.decision =
        ExactDirectMorseNormalizedH0ProductSealDecision::
            no_outstanding_prepared_ticket;
    return output;
  }
  try {
    if (impl_->final_seal != nullptr) {
      output.seal = impl_->final_seal;
      output.existing_final_seal_returned_without_mutation = true;
      output.decision =
          ExactDirectMorseNormalizedH0ProductSealDecision::
              complete_certified_bounded_normalized_product_seal;
      return output;
    }
    if (!complete()) {
      output.decision =
          ExactDirectMorseNormalizedH0ProductSealDecision::
              no_product_not_complete;
      return output;
    }

    // Allocate the only product-seal storage before sealing either owned
    // scientific sink.  Every field publication after Hartigan closure is a
    // noexcept move or shared_ptr control-block increment into this storage.
    auto final_storage =
        std::make_shared<ExactDirectMorseNormalizedH0ProductFinalSeal>();
    auto k1_seal = impl_->k1.seal();
    if (!k1_seal.certified_terminal_closure() ||
        !impl_->k1.verify_final_seal(k1_seal)) {
      output.decision =
          ExactDirectMorseNormalizedH0ProductSealDecision::
              no_k1_seal_rejected;
      return output;
    }
    auto vertical_seal_result = impl_->vertical.seal();
    if (!vertical_seal_result.certified_final_vertical_seal() ||
        !vertical_seal_result.seal.has_value() ||
        !impl_->vertical.normalized_incidence_complete_v7_source_capability()) {
      output.decision =
          ExactDirectMorseNormalizedH0ProductSealDecision::
              no_vertical_seal_rejected;
      return output;
    }
    auto higher_checkpoint_result =
        impl_->higher_at_least20.make_checkpoint();
    if (!higher_checkpoint_result.certified_checkpoint() ||
        !higher_checkpoint_result.checkpoint.has_value()) {
      output.decision =
          ExactDirectMorseNormalizedH0ProductSealDecision::
              no_higher_view_checkpoint_rejected;
      return output;
    }
    const auto& higher_checkpoint =
        higher_checkpoint_result.checkpoint.value();
    const auto vertical_stamp = impl_->vertical.current_stamp();
    if (higher_checkpoint.next_source_batch_cursor !=
            impl_->requirements.resident_batch_count ||
        higher_checkpoint.source_epoch !=
            impl_->requirements.resident_batch_count ||
        vertical_stamp.resident_batch_cursor !=
            impl_->requirements.resident_batch_count ||
        higher_checkpoint.source_scientific_digest !=
            impl_->higher_scientific_digest ||
        higher_checkpoint.public_status_claimed) {
      output.decision =
          ExactDirectMorseNormalizedH0ProductSealDecision::
              no_higher_view_checkpoint_rejected;
      return output;
    }
    auto hartigan_checkpoint_result = impl_->hartigan.make_checkpoint();
    if (!hartigan_checkpoint_result.certified_semantic_checkpoint() ||
        !hartigan_checkpoint_result.checkpoint.has_value() ||
        !hartigan_checkpoint_shape_certified(
            hartigan_checkpoint_result.checkpoint.value(),
            impl_->record_count_by_order,
            impl_->omitted_noop_count_by_order)) {
      output.decision =
          ExactDirectMorseNormalizedH0ProductSealDecision::
              no_hartigan_closure_rejected;
      return output;
    }

    const auto& hartigan_checkpoint =
        hartigan_checkpoint_result.checkpoint.value();
    ExactNormalizedHartiganLevelManifestSeal predicted_hartigan;
    predicted_hartigan.authority_id = impl_->product_authority_id;
    predicted_hartigan.maximum_order = hartigan_checkpoint.maximum_order;
    predicted_hartigan.manifest_record_count =
        hartigan_checkpoint.record_count;
    predicted_hartigan.contract_mode =
        ExactNormalizedHartiganManifestContractMode::
            streaming_observed_open_contract;
    predicted_hartigan.records_by_order =
        hartigan_checkpoint.records_by_order;
    predicted_hartigan.normalized_batch_final_chain_digest =
        hartigan_checkpoint.current_source_chain_digest;
    predicted_hartigan.ordered_rational_chain_digest =
        hartigan_checkpoint.current_rational_chain_digest;
    predicted_hartigan.all_normalized_equal_level_batches_covered = true;
    predicted_hartigan.one_record_per_batch_including_omitted_noop = true;
    predicted_hartigan.binary64_level_count_zero = true;
    predicted_hartigan.bounded_memory_summary_only = true;
    predicted_hartigan.manifest_file_bytes_materialized = false;
    predicted_hartigan.every_record_uses_complete_batch_qr_partition = true;
    predicted_hartigan.legacy_single_group_scalar_qr_compatibility_used =
        false;
    predicted_hartigan
        .streaming_counts_closed_from_move_only_exhaustion_attestation = true;
    predicted_hartigan.public_status_claimed = false;
    predicted_hartigan.decision =
        ExactNormalizedHartiganLevelSealDecision::
            complete_certified_manifest_seal;
    if (!predicted_hartigan.certified_seal()) {
      output.decision =
          ExactDirectMorseNormalizedH0ProductSealDecision::
              no_hartigan_closure_rejected;
      return output;
    }

    final_storage->terminal_stamp = current_stamp();
    final_storage->k1 = std::move(k1_seal);
    final_storage->vertical =
        std::move(vertical_seal_result.seal.value());
    final_storage->higher_at_least20 =
        std::move(higher_checkpoint_result.checkpoint.value());
    final_storage->hartigan = std::move(predicted_hartigan);
    final_storage->structural_source_transition_count =
        impl_->product_batch_cursor;
    final_storage->hartigan_record_count = impl_->hartigan_record_count;
    final_storage->normalized_incidence_complete_v7_source_capability = true;
    final_storage->every_product_batch_committed_in_exact_level_order = true;
    final_storage->empty_structural_batches_excluded_from_hartigan_manifest =
        true;
    final_storage->higher_at_least20_complete = true;
    final_storage->hartigan_streaming_closed = true;
    final_storage->no_global_facet_coface_or_gamma_catalog_materialized = true;
    final_storage->ordinary_or_higher_order_delaunay_materialized = false;
    final_storage->global_morse_obligation_replayed = false;
    final_storage->m1_replayed = false;
    final_storage->public_status_claimed = false;
    final_storage->seal_digest = final_seal_digest(*final_storage);
    if (!final_seal_links_certified(*final_storage)) {
      output.decision =
          ExactDirectMorseNormalizedH0ProductSealDecision::
              no_hartigan_closure_rejected;
      return output;
    }

    auto closure =
        impl_->hartigan.prepare_streaming_closure_from_exhausted_product_source(
            impl_->scientific_digest,
            impl_->hartigan_record_count,
            impl_->record_count_by_order,
            impl_->omitted_noop_count_by_order,
            impl_->hartigan_chain_digest);
    if (!closure.has_value() || !closure->valid()) {
      output.decision =
          ExactDirectMorseNormalizedH0ProductSealDecision::
              no_hartigan_closure_rejected;
      return output;
    }
    static_assert(std::is_nothrow_move_assignable_v<
                  ExactNormalizedHartiganLevelManifestSeal>);
    static_assert(std::is_nothrow_move_assignable_v<
                  ExactDirectMorseNormalizedH0ProductFinalSeal>);
    auto hartigan_seal =
        impl_->hartigan.seal_streaming(std::move(closure.value()));
    if (!hartigan_seal.certified_seal() ||
        !hartigan_seal.streaming_counts_closed_from_move_only_exhaustion_attestation ||
        hartigan_seal.normalized_batch_final_chain_digest !=
            impl_->hartigan_chain_digest ||
        hartigan_seal.manifest_record_count !=
            impl_->hartigan_record_count ||
        !hartigan_final_seals_equal(
            hartigan_seal, final_storage->hartigan)) {
      // The streaming sink is now irreversibly sealed.  Returning an ordinary
      // failure would leave a falsely reusable half-published product.
      std::terminate();
    }
    final_storage->hartigan = std::move(hartigan_seal);
    std::shared_ptr<const ExactDirectMorseNormalizedH0ProductFinalSeal>
        immutable_final = final_storage;
    impl_->final_seal = immutable_final;
    output.seal = impl_->final_seal;
    output.final_seal_published = true;
    output.decision =
        ExactDirectMorseNormalizedH0ProductSealDecision::
            complete_certified_bounded_normalized_product_seal;
    return output;
  } catch (const std::bad_alloc&) {
    output.decision =
        ExactDirectMorseNormalizedH0ProductSealDecision::
            no_higher_view_checkpoint_rejected;
    return output;
  } catch (const std::exception&) {
    output.decision =
        ExactDirectMorseNormalizedH0ProductSealDecision::
            no_hartigan_closure_rejected;
    return output;
  }
}

}  // namespace morsehgp3d::hierarchy
