#include "morsehgp3d/hierarchy/direct_morse_forest_reducer.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <new>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

[[nodiscard]] bool try_add(
    std::size_t left,
    std::size_t right,
    std::size_t& sum) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  sum = left + right;
  return true;
}

[[nodiscard]] bool try_multiply(
    std::size_t left,
    std::size_t right,
    std::size_t& product) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return false;
  }
  product = left * right;
  return true;
}

void append_u64(
    contract::CanonicalSha256Builder& builder,
    std::uint64_t value) {
  std::array<std::uint8_t, 8U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    const std::size_t shift = (bytes.size() - 1U - index) * 8U;
    bytes[index] = static_cast<std::uint8_t>(value >> shift);
  }
  builder.update(bytes);
}

void append_size(
    contract::CanonicalSha256Builder& builder,
    std::size_t value) {
  static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));
  append_u64(builder, static_cast<std::uint64_t>(value));
}

void append_bool(
    contract::CanonicalSha256Builder& builder,
    bool value) {
  append_u64(builder, value ? 1U : 0U);
}

void append_text(
    contract::CanonicalSha256Builder& builder,
    std::string_view value) {
  append_size(builder, value.size());
  builder.update(value);
}

void append_id(
    contract::CanonicalSha256Builder& builder,
    const contract::CanonicalId& value) {
  builder.update(value.bytes());
}

void append_optional_size(
    contract::CanonicalSha256Builder& builder,
    const std::optional<std::size_t>& value) {
  append_bool(builder, value.has_value());
  if (value.has_value()) {
    append_size(builder, *value);
  }
}

void append_optional_node_id(
    contract::CanonicalSha256Builder& builder,
    const std::optional<ExactDirectMorseForestNodeId>& value) {
  append_bool(builder, value.has_value());
  if (value.has_value()) {
    append_u64(builder, *value);
  }
}

void append_level(
    contract::CanonicalSha256Builder& builder,
    const exact::ExactLevel& value) {
  append_text(builder, value.numerator_string());
  append_text(builder, value.denominator_string());
}

void append_center(
    contract::CanonicalSha256Builder& builder,
    const exact::ExactCenter3& value) {
  const exact::ExactRational3Record record = value.to_record();
  append_text(builder, record.x_numerator);
  append_text(builder, record.y_numerator);
  append_text(builder, record.z_numerator);
  append_text(builder, record.denominator);
}

void append_key(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparseFacetKey& value) {
  append_size(builder, value.point_count);
  for (const spatial::PointId point_id : value.point_ids) {
    append_u64(builder, static_cast<std::uint64_t>(point_id));
  }
}

void append_witness(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparseFacetWitness& value) {
  append_u64(builder, value.external_authority_id);
  append_u64(builder, value.replay_token);
}

void append_stamp(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparsePositiveFacetLocatorSnapshotStamp& value) {
  append_u64(builder, value.schema_version);
  append_u64(builder, value.external_authority_id);
  append_size(builder, value.committed_batch_count);
  append_size(builder, value.inserted_key_count);
  append_size(builder, value.component_union_count);
  append_size(builder, value.binding_count);
  append_id(builder, value.committed_history_digest);
}

void append_cursor_without_digest(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectMorseForestSegmentCursor& value) {
  append_size(builder, value.segment_count);
  append_size(builder, value.implicit_order_one_prefix_count);
  append_size(builder, value.birth_record_count);
  append_size(builder, value.arm_root_binding_count);
  append_size(builder, value.saddle_record_count);
  append_size(builder, value.atomic_group_count);
  append_size(builder, value.child_reference_count);
  append_size(builder, value.batch_record_count);
  append_size(builder, value.node_count);
  append_size(builder, value.logical_output_entry_count);
}

void append_batch_precommit_payload(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectMorseForestBatch& value) {
  append_size(builder, value.batch_index);
  append_size(builder, value.source_journal_batch_index);
  append_size(builder, value.order);
  append_level(builder, value.squared_level);
  append_size(builder, value.birth_record_offset);
  append_size(builder, value.birth_record_count);
  append_size(builder, value.saddle_record_offset);
  append_size(builder, value.saddle_record_count);
  append_size(builder, value.atomic_group_offset);
  append_size(builder, value.atomic_group_count);
  append_size(builder, value.strict_pre_batch_carrier_count);
  append_size(builder, value.strict_pre_batch_reduced_root_count);
  append_stamp(builder, value.strict_pre_batch_stamp);
  append_bool(builder, value.strict_arms_resolved_before_mutation);
  append_bool(builder, value.quotient_resolved_before_mutation);
}

[[nodiscard]] contract::CanonicalId segment_payload_digest_from_parts(
    const ExactDirectMorseForestSegmentCursor& begin_cursor,
    const ExactDirectMorseForestBatch& batch,
    std::span<const ExactDirectMorseForestBirthRecord> birth_records,
    std::span<const ExactDirectMorseForestArmRootBinding> arm_root_bindings,
    std::span<const ExactDirectMorseForestSaddleRecord> saddle_records,
    std::span<const ExactDirectMorseForestAtomicGroup> atomic_groups,
    std::span<const ExactDirectMorseForestNodeId> child_node_ids,
    std::span<const ExactDirectMorseForestNode> nodes,
    bool canonical_singleton_prefix_implicit) {
  contract::CanonicalSha256Builder builder;
  append_text(
      builder,
      "MorseHGP3D/phase15/forest-batch-segment-payload/v2");
  append_cursor_without_digest(builder, begin_cursor);
  append_batch_precommit_payload(builder, batch);
  append_size(builder, birth_records.size());
  for (const auto& record : birth_records) {
    append_size(builder, record.birth_record_index);
    append_size(builder, record.source_event_projection_index);
    append_size(builder, record.source_journal_batch_index);
    append_size(builder, record.order);
    append_key(builder, record.facet_key);
    append_size(builder, record.component_handle);
    append_optional_node_id(builder, record.order_one_birth_node_id);
    append_witness(builder, record.binding_witness);
  }
  append_size(builder, arm_root_bindings.size());
  for (const auto& record : arm_root_bindings) {
    append_size(builder, record.binding_index);
    append_size(builder, record.source_arm_seed_index);
    append_size(builder, record.source_family_index);
    append_key(builder, record.strict_arm_key);
    append_size(builder, record.frozen_carrier_component_handle);
    append_optional_node_id(builder, record.prior_reduced_root_node_id);
    append_size(builder, record.terminal_birth_record_index);
    append_key(builder, record.terminal_birth_facet_key);
    append_witness(builder, record.terminal_birth_binding_witness);
    append_u64(
        builder,
        static_cast<std::uint64_t>(record.removed_support_point_id));
    append_center(builder, record.terminal_birth_exact_center);
    append_level(builder, record.terminal_birth_exact_squared_level);
  }
  append_size(builder, saddle_records.size());
  for (const auto& record : saddle_records) {
    append_size(builder, record.saddle_record_index);
    append_size(builder, record.source_family_index);
    append_size(builder, record.source_event_index);
    append_size(builder, record.source_journal_batch_index);
    append_size(builder, record.arm_binding_offset);
    append_size(builder, record.arm_binding_count);
    append_size(builder, record.distinct_frozen_carrier_count);
    append_size(builder, record.distinct_latent_carrier_count);
    append_size(builder, record.distinct_prior_reduced_root_count);
    append_size(builder, record.atomic_group_index);
    append_size(builder, record.journal_event_projection_index);
    append_id(builder, record.source_event_arm_identity_digest);
  }
  append_size(builder, atomic_groups.size());
  for (const auto& record : atomic_groups) {
    append_size(builder, record.atomic_group_index);
    append_size(builder, record.batch_index);
    append_size(builder, record.saddle_record_offset);
    append_size(builder, record.saddle_record_count);
    append_size(builder, record.frozen_carrier_count);
    append_size(builder, record.latent_carrier_count);
    append_size(builder, record.prior_reduced_root_count);
    append_size(builder, record.child_offset);
    append_size(builder, record.child_count);
    append_optional_node_id(builder, record.created_node_id);
    append_u64(builder, record.resulting_root_node_id);
    append_u64(builder, static_cast<std::uint8_t>(record.kind));
  }
  append_size(builder, child_node_ids.size());
  for (const auto node_id : child_node_ids) {
    append_u64(builder, node_id);
  }
  append_size(builder, nodes.size());
  for (const auto& record : nodes) {
    append_u64(builder, record.node_id);
    append_size(builder, record.order);
    append_level(builder, record.squared_level);
    append_u64(builder, static_cast<std::uint8_t>(record.kind));
    append_size(builder, record.child_offset);
    append_size(builder, record.child_count);
    append_optional_size(builder, record.birth_record_index);
    append_optional_size(builder, record.atomic_group_index);
  }
  append_bool(builder, canonical_singleton_prefix_implicit);
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId segment_payload_digest(
    const ExactDirectMorseForestBatchSegment& segment) {
  return segment_payload_digest_from_parts(
      segment.begin_cursor,
      segment.batch,
      segment.birth_records,
      segment.arm_root_bindings,
      segment.saddle_records,
      segment.atomic_groups,
      segment.child_node_ids,
      segment.nodes,
      segment.canonical_singleton_prefix_implicit);
}

[[nodiscard]] contract::CanonicalId segment_chain_digest(
    const ExactDirectMorseForestBatchSegment& segment) {
  contract::CanonicalSha256Builder builder;
  append_text(
      builder,
      "MorseHGP3D/phase15/forest-batch-segment-chain/v1");
  append_id(builder, segment.begin_cursor.chain_digest);
  append_id(builder, segment.payload_digest);
  append_cursor_without_digest(builder, segment.end_cursor);
  append_size(builder, segment.batch.closed_post_batch_carrier_count);
  append_size(builder, segment.batch.closed_post_batch_reduced_root_count);
  append_stamp(builder, segment.batch.committed_batch_stamp);
  append_bool(builder, segment.batch.unions_then_births_committed_atomically);
  return builder.finalize();
}

[[nodiscard]] bool append_count_within(
    std::size_t current,
    std::size_t increment,
    std::size_t maximum) noexcept {
  std::size_t sum = 0U;
  return try_add(current, increment, sum) && sum <= maximum;
}

[[nodiscard]] ExactDirectSparsePositiveFacetProbeBudget
terminal_probe_budget(
    const ExactDirectSparseFacetDescentClosureBudget& budget) noexcept {
  const auto& source = budget.step_budget.source_locator_probe;
  const auto& successor = budget.step_budget.successor_locator_probe;
  return {
      std::max(
          source.maximum_slot_visit_count,
          successor.maximum_slot_visit_count),
      std::max(
          source.maximum_component_parent_hop_count,
          successor.maximum_component_parent_hop_count),
  };
}

[[nodiscard]] bool key_less(
    const ExactDirectSparseFacetKey& left,
    const ExactDirectSparseFacetKey& right) noexcept {
  const std::size_t common = std::min(left.point_count, right.point_count);
  for (std::size_t index = 0U; index < common; ++index) {
    if (left.point_ids[index] != right.point_ids[index]) {
      return left.point_ids[index] < right.point_ids[index];
    }
  }
  return left.point_count < right.point_count;
}

[[nodiscard]] bool valid_key(
    const ExactDirectSparseFacetKey& key,
    std::size_t point_count,
    std::size_t order) noexcept {
  if (order == 0U || order > point_count ||
      order > direct_sparse_positive_facet_maximum_point_count ||
      key.point_count != order) {
    return false;
  }
  for (std::size_t index = 0U; index < order; ++index) {
    if (static_cast<std::size_t>(key.point_ids[index]) >= point_count ||
        (index != 0U &&
         key.point_ids[index - 1U] >= key.point_ids[index])) {
      return false;
    }
  }
  for (std::size_t index = order; index < key.point_ids.size(); ++index) {
    if (key.point_ids[index] != 0U) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] std::optional<std::uint64_t> replay_token(
    std::size_t index,
    std::uint64_t residue) noexcept {
  constexpr std::uint64_t modulus = 3U;
  if (residue == 0U || residue > modulus ||
      index >
          (std::numeric_limits<std::uint64_t>::max() - residue) /
              modulus) {
    return std::nullopt;
  }
  return static_cast<std::uint64_t>(index) * modulus + residue;
}

void require_traversal_order(spatial::LbvhTraversalOrder order) {
  switch (order) {
    case spatial::LbvhTraversalOrder::near_first:
    case spatial::LbvhTraversalOrder::far_first:
      return;
  }
  throw std::invalid_argument(
      "a direct Morse forest reducer traversal order is invalid");
}

[[nodiscard]] ExactDirectSparseFacetKey birth_key(
    const ExactDirectMorseEventProjection& projection,
    ExactDirectMorseForestEventLookupView events,
    std::size_t direct_event_count,
    std::size_t point_count,
    std::size_t order,
    const exact::ExactLevel& level) {
  if (order == 0U ||
      order > direct_sparse_positive_facet_maximum_point_count ||
      projection.support_size == 0U ||
      static_cast<std::size_t>(projection.support_size) >
          projection.support_ids.size() ||
      direct_event_count >
          std::numeric_limits<std::size_t>::max() - point_count ||
      projection.event_projection_index >=
          direct_event_count + point_count ||
      projection.birth_order != std::optional<std::size_t>{order} ||
      projection.squared_level != level ||
      projection.closed_rank != order) {
    throw std::logic_error(
        "a reducer birth projection disagrees with its source batch");
  }

  ExactDirectSparseFacetKey key;
  key.point_count = order;
  if (projection.source ==
      ExactDirectMorseEventSource::canonical_singleton) {
    if (order != 1U || projection.source_index >= point_count ||
        projection.support_size != 1U ||
        projection.support_ids[0U] !=
            static_cast<PointId>(projection.source_index)) {
      throw std::logic_error(
          "a reducer singleton birth has an invalid identity");
    }
    key.point_ids[0U] = static_cast<PointId>(projection.source_index);
    return key;
  }

  if (projection.source !=
          ExactDirectMorseEventSource::direct_support_terminal_event ||
      projection.source_index >= direct_event_count) {
    throw std::logic_error(
        "a reducer direct birth has no Phase-9 source event");
  }
  const ExactDirectSupportEvent* event_pointer =
      events.find(projection.source_index);
  if (event_pointer == nullptr) {
    throw std::logic_error(
        "a reducer source window omitted one direct birth event");
  }
  const ExactDirectSupportEvent& event = *event_pointer;
  const std::size_t support_size =
      static_cast<std::size_t>(event.support_size);
  if (support_size == 0U || support_size > event.support_ids.size() ||
      event.event_index != projection.source_index ||
      event.birth_order != std::optional<std::size_t>{order} ||
      event.squared_level != level || event.closed_rank != order ||
      event.support_size != projection.support_size ||
      event.support_ids != projection.support_ids ||
      support_size + event.interior_ids.size() != order) {
    throw std::logic_error(
        "a reducer direct birth changed Phase-9 identity");
  }
  std::size_t write = 0U;
  for (const PointId point_id : event.interior_ids) {
    key.point_ids[write++] = point_id;
  }
  for (std::size_t index = 0U; index < support_size; ++index) {
    key.point_ids[write++] = event.support_ids[index];
  }
  std::sort(
      key.point_ids.begin(),
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(write));
  if (write != order || !valid_key(key, point_count, order)) {
    throw std::logic_error(
        "a reducer birth did not reconstruct one canonical key");
  }
  return key;
}

[[nodiscard]] ExactDirectSparseFacetKey arm_key(
    const ExactDirectSaddleArmFamilyRecord& family,
    const ExactDirectSaddleArmSeedRecord& seed,
    ExactDirectMorseForestEventLookupView events,
    std::size_t point_count,
    std::size_t order) {
  const ExactDirectSupportEvent* event_pointer =
      events.find(family.source_event_index);
  if (event_pointer == nullptr) {
    throw std::logic_error(
        "a reducer source window omitted one saddle event");
  }
  const ExactDirectSupportEvent& event = *event_pointer;
  const std::size_t support_size =
      static_cast<std::size_t>(event.support_size);
  if (order == 0U ||
      order > direct_sparse_positive_facet_maximum_point_count ||
      support_size == 0U || support_size > event.support_ids.size() ||
      seed.family_index != family.family_index ||
      event.event_index != family.source_event_index ||
      event.saddle_order != std::optional<std::size_t>{order} ||
      event.squared_level != family.critical_squared_level ||
      support_size != family.arm_seed_count ||
      event.interior_ids.size() + support_size - 1U != order) {
    throw std::logic_error(
        "a reducer saddle window changed its source identity");
  }
  ExactDirectSparseFacetKey key;
  key.point_count = order;
  std::size_t write = 0U;
  for (const PointId point_id : event.interior_ids) {
    key.point_ids[write++] = point_id;
  }
  bool removed = false;
  for (std::size_t index = 0U; index < support_size; ++index) {
    const PointId point_id = event.support_ids[index];
    if (point_id == seed.removed_support_point_id) {
      if (removed) {
        throw std::logic_error(
            "a reducer saddle seed removes a repeated support point");
      }
      removed = true;
      continue;
    }
    key.point_ids[write++] = point_id;
  }
  if (!removed || write != order) {
    throw std::logic_error(
        "a reducer saddle seed does not reconstruct one order-k arm");
  }
  std::sort(
      key.point_ids.begin(),
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(write));
  if (!valid_key(key, point_count, order)) {
    throw std::logic_error(
        "a reducer strict arm is not a canonical order-k key");
  }
  return key;
}

void initialize_scope(ExactDirectMorseForestJournalResult& result) noexcept {
  result.source_phase9_facade_freshly_replayed = false;
  result.conditional_on_caller_fresh_phase9_facade_replay = true;
  result.external_locator_authority_replayed = true;
  result.conditional_on_caller_external_locator_authority_replay = false;
  result.global_morse_obligation_replayed = false;
  result.conditional_on_separate_global_morse_obligation = true;
  result.equal_or_interior_facets_consumed = false;
  result.gateway_10_6_or_later_consumed = false;
  result.closure_graph_persisted = false;
  result.gamma_cells_or_global_cofaces_materialized = false;
  result.higher_order_delaunay_materialized = false;
  result.forbidden_global_structure_materialized = false;
  result.public_status_claimed = false;
  result.conditional_exact_h0_only = true;
  result.scope = ExactDirectMorseForestScope::
      all_orders_direct_minimum_carriers_strict_arms_recursive_positive_terminals_and_atomic_full_component_saddle_quotients_with_reduced_qr_only;
}

struct TemporaryArm {
  std::size_t source_seed_index{};
  ExactDirectSparseFacetKey key{};
  ExactDirectSparseComponentHandle carrier_handle{};
  std::optional<ExactDirectMorseForestNodeId> prior_reduced_root_node_id;
  std::size_t terminal_birth_record_index{};
  ExactDirectSparseFacetKey terminal_birth_facet_key{};
  ExactDirectSparseFacetWitness terminal_birth_binding_witness{};
  PointId removed_support_point_id{};
  exact::ExactCenter3 terminal_birth_exact_center{};
  exact::ExactLevel terminal_birth_exact_squared_level{};
};

struct TemporarySaddle {
  std::size_t source_family_index{};
  std::vector<TemporaryArm> arms;
};

struct ResolvedState {
  ExactDirectSparseFacetKey key{};
  ExactDirectSparseComponentHandle carrier_handle{};
  std::optional<ExactDirectMorseForestNodeId> prior_reduced_root_node_id;
  std::size_t terminal_birth_record_index{};
  ExactDirectSparseFacetKey terminal_birth_facet_key{};
  ExactDirectSparseFacetWitness terminal_birth_binding_witness{};
  exact::ExactCenter3 terminal_birth_exact_center{};
  exact::ExactLevel terminal_birth_exact_squared_level{};
};

struct GroupPlan {
  std::vector<std::size_t> saddle_indices;
  std::vector<ExactDirectSparseComponentHandle> carrier_handles;
  std::vector<ExactDirectMorseForestNodeId> prior_reduced_root_node_ids;
  ExactDirectSparseComponentHandle canonical_root_after_union{};
  std::optional<ExactDirectMorseForestNodeId> created_node_id;
  ExactDirectMorseForestNodeId resulting_root_node_id{};
  ExactDirectMorseForestAtomicGroupKind kind{
      ExactDirectMorseForestAtomicGroupKind::reduced_birth};
  std::size_t atomic_group_index{};
};

class CarrierDsu {
 public:
  CarrierDsu(
      const ExactDirectSparsePositiveFacetLocator& locator,
      std::size_t handle_count,
      std::size_t canonical_singleton_count,
      std::size_t maximum_order,
      std::size_t maximum_atomic_group_count)
      : locator_(&locator),
        handle_count_(handle_count),
        canonical_singleton_count_(canonical_singleton_count),
        direct_states_(
            handle_count >= canonical_singleton_count
                ? handle_count - canonical_singleton_count
                : 0U),
        reduced_root_overrides_(
            override_slot_capacity(maximum_atomic_group_count)),
        carrier_count_by_order_(maximum_order + 1U, 0U),
        reduced_count_by_order_(maximum_order + 1U, 0U),
        representative_handle_by_order_(maximum_order + 1U) {
    if (handle_count == 0U ||
        canonical_singleton_count == 0U ||
        canonical_singleton_count > handle_count ||
        locator.component_parents().size() != handle_count) {
      throw std::invalid_argument(
          "a reducer carrier authority has inconsistent handle intervals");
    }
  }

  [[nodiscard]] std::size_t root(std::size_t handle) const noexcept {
    const auto& parents = locator_->component_parents();
    while (parents[handle] != handle) {
      handle = parents[handle];
    }
    return handle;
  }

  [[nodiscard]] bool active(std::size_t handle) const noexcept {
    return handle < handle_count_ && root_active(root(handle));
  }

  [[nodiscard]] std::size_t canonical(
      std::size_t handle) const noexcept {
    return root(handle);
  }

  [[nodiscard]] std::size_t order(std::size_t handle) const noexcept {
    const std::size_t canonical_root = root(handle);
    if (canonical_root < canonical_singleton_count_) {
      return singletons_active_ ? 1U : 0U;
    }
    return direct_state(canonical_root).order;
  }

  [[nodiscard]] std::optional<ExactDirectMorseForestNodeId> reduced_root(
      std::size_t handle) const noexcept {
    const std::size_t canonical_root = root(handle);
    if (const auto overridden =
            find_reduced_root_override(canonical_root);
        overridden.has_value()) {
      return overridden;
    }
    if (canonical_root < canonical_singleton_count_) {
      return singletons_active_
                 ? std::optional<ExactDirectMorseForestNodeId>{
                       static_cast<ExactDirectMorseForestNodeId>(
                           canonical_root)}
                 : std::nullopt;
    }
    return direct_state(canonical_root).reduced_root;
  }

  [[nodiscard]] std::size_t handle_count() const noexcept {
    return handle_count_;
  }

  [[nodiscard]] std::size_t implicit_singleton_count() const noexcept {
    return canonical_singleton_count_;
  }

  [[nodiscard]] std::size_t materialized_direct_state_count()
      const noexcept {
    return direct_states_.size();
  }

  [[nodiscard]] std::size_t root_override_slot_capacity() const noexcept {
    return reduced_root_overrides_.size();
  }

  [[nodiscard]] std::size_t carrier_count(std::size_t order) const noexcept {
    return carrier_count_by_order_[order];
  }

  [[nodiscard]] std::size_t reduced_count(std::size_t order) const noexcept {
    return reduced_count_by_order_[order];
  }

  [[nodiscard]] bool active_root(std::size_t handle) const noexcept {
    return handle < handle_count_ && root(handle) == handle &&
           root_active(handle);
  }

  void commit_group(
      ExactDirectSparseComponentHandle canonical_root,
      std::size_t carrier_count,
      std::size_t group_order,
      std::size_t prior_reduced_count,
      ExactDirectMorseForestNodeId resulting_root) noexcept {
    set_reduced_root_override(canonical_root, resulting_root);
    carrier_count_by_order_[group_order] -= carrier_count - 1U;
    reduced_count_by_order_[group_order] -= prior_reduced_count;
    ++reduced_count_by_order_[group_order];
    representative_handle_by_order_[group_order] = canonical_root;
  }

  void activate(
      std::size_t handle,
      std::size_t birth_order,
      std::optional<ExactDirectMorseForestNodeId> root_node) noexcept {
    if (handle < canonical_singleton_count_ ||
        handle >= handle_count_) {
      std::terminate();
    }
    DirectCarrierState& state = direct_state(handle);
    if (state.active) {
      std::terminate();
    }
    state.active = true;
    state.order = birth_order;
    state.reduced_root = root_node;
    ++carrier_count_by_order_[birth_order];
    representative_handle_by_order_[birth_order] = handle;
    if (root_node.has_value()) {
      ++reduced_count_by_order_[birth_order];
    }
  }

  void activate_initial_canonical_singletons(
      std::size_t singleton_count) noexcept {
    if (singletons_active_ ||
        singleton_count != canonical_singleton_count_) {
      std::terminate();
    }
    singletons_active_ = true;
    carrier_count_by_order_[1U] = singleton_count;
    reduced_count_by_order_[1U] = singleton_count;
    representative_handle_by_order_[1U] = 0U;
  }

  [[nodiscard]] std::optional<std::size_t> representative_handle(
      std::size_t order) const noexcept {
    return representative_handle_by_order_[order];
  }

 private:
  struct DirectCarrierState {
    std::size_t order{};
    std::optional<ExactDirectMorseForestNodeId> reduced_root;
    bool active{false};
  };

  struct ReducedRootOverrideSlot {
    std::size_t canonical_root{};
    ExactDirectMorseForestNodeId reduced_root{};
    bool occupied{false};
  };

  [[nodiscard]] static std::size_t override_slot_capacity(
      std::size_t maximum_atomic_group_count) {
    std::size_t doubled = 0U;
    std::size_t capacity = 0U;
    if (!try_multiply(
            maximum_atomic_group_count, 2U, doubled) ||
        !try_add(doubled, 1U, capacity)) {
      throw std::length_error(
          "a reducer root-override table capacity overflowed");
    }
    return capacity;
  }

  [[nodiscard]] static std::size_t override_hash(
      std::size_t value) noexcept {
    std::uint64_t word = static_cast<std::uint64_t>(value);
    word ^= word >> 30U;
    word *= UINT64_C(0xbf58476d1ce4e5b9);
    word ^= word >> 27U;
    word *= UINT64_C(0x94d049bb133111eb);
    word ^= word >> 31U;
    return static_cast<std::size_t>(word);
  }

  [[nodiscard]] const DirectCarrierState& direct_state(
      std::size_t handle) const noexcept {
    return direct_states_[handle - canonical_singleton_count_];
  }

  [[nodiscard]] DirectCarrierState& direct_state(
      std::size_t handle) noexcept {
    return direct_states_[handle - canonical_singleton_count_];
  }

  [[nodiscard]] bool root_active(std::size_t canonical_root) const noexcept {
    if (canonical_root < canonical_singleton_count_) {
      return singletons_active_;
    }
    return direct_state(canonical_root).active;
  }

  [[nodiscard]] std::optional<ExactDirectMorseForestNodeId>
  find_reduced_root_override(
      std::size_t canonical_root) const noexcept {
    const std::size_t capacity = reduced_root_overrides_.size();
    std::size_t slot_index =
        override_hash(canonical_root) % capacity;
    for (std::size_t visit = 0U; visit < capacity; ++visit) {
      const ReducedRootOverrideSlot& slot =
          reduced_root_overrides_[slot_index];
      if (!slot.occupied) {
        return std::nullopt;
      }
      if (slot.canonical_root == canonical_root) {
        return slot.reduced_root;
      }
      slot_index = slot_index + 1U == capacity
                       ? 0U
                       : slot_index + 1U;
    }
    return std::nullopt;
  }

  void set_reduced_root_override(
      std::size_t canonical_root,
      ExactDirectMorseForestNodeId reduced_root) noexcept {
    const std::size_t capacity = reduced_root_overrides_.size();
    std::size_t slot_index =
        override_hash(canonical_root) % capacity;
    for (std::size_t visit = 0U; visit < capacity; ++visit) {
      ReducedRootOverrideSlot& slot =
          reduced_root_overrides_[slot_index];
      if (!slot.occupied) {
        slot.canonical_root = canonical_root;
        slot.reduced_root = reduced_root;
        slot.occupied = true;
        return;
      }
      if (slot.canonical_root == canonical_root) {
        slot.reduced_root = reduced_root;
        return;
      }
      slot_index = slot_index + 1U == capacity
                       ? 0U
                       : slot_index + 1U;
    }
    std::terminate();
  }

  const ExactDirectSparsePositiveFacetLocator* locator_{};
  std::size_t handle_count_{};
  std::size_t canonical_singleton_count_{};
  std::vector<DirectCarrierState> direct_states_;
  std::vector<ReducedRootOverrideSlot> reduced_root_overrides_;
  std::vector<std::size_t> carrier_count_by_order_;
  std::vector<std::size_t> reduced_count_by_order_;
  std::vector<std::optional<std::size_t>>
      representative_handle_by_order_;
  bool singletons_active_{false};
};

struct PayloadSizes {
  std::size_t birth_records{};
  std::size_t arm_root_bindings{};
  std::size_t saddle_records{};
  std::size_t atomic_groups{};
  std::size_t child_node_ids{};
  std::size_t batches{};
  std::size_t nodes{};
};

[[nodiscard]] std::size_t logical_birth_record_count(
    const ExactDirectMorseForestJournalResult& result) noexcept {
  return result.implicit_order_one_prefix_count +
         result.birth_records.size();
}

[[nodiscard]] std::size_t logical_node_count(
    const ExactDirectMorseForestJournalResult& result) noexcept {
  return result.implicit_order_one_prefix_count +
         result.nodes.size();
}

[[nodiscard]] PayloadSizes payload_sizes(
    const ExactDirectMorseForestJournalResult& result) noexcept {
  return {
      result.birth_records.size(),
      result.arm_root_bindings.size(),
      result.saddle_records.size(),
      result.atomic_groups.size(),
      result.child_node_ids.size(),
      result.batches.size(),
      result.nodes.size(),
  };
}

class PayloadRollback {
 public:
  PayloadRollback(
      ExactDirectMorseForestJournalResult& result,
      PayloadSizes sizes) noexcept
      : result_(&result), sizes_(sizes) {}

  PayloadRollback(const PayloadRollback&) = delete;
  PayloadRollback& operator=(const PayloadRollback&) = delete;

  ~PayloadRollback() {
    if (armed_) {
      result_->birth_records.resize(sizes_.birth_records);
      result_->arm_root_bindings.resize(sizes_.arm_root_bindings);
      result_->saddle_records.resize(sizes_.saddle_records);
      result_->atomic_groups.resize(sizes_.atomic_groups);
      result_->child_node_ids.resize(sizes_.child_node_ids);
      result_->batches.resize(sizes_.batches);
      result_->nodes.resize(sizes_.nodes);
    }
  }

  void release() noexcept { armed_ = false; }

 private:
  ExactDirectMorseForestJournalResult* result_{};
  PayloadSizes sizes_{};
  bool armed_{true};
};

template <typename Value>
void append_moved(std::vector<Value>& destination, std::vector<Value>& source) {
  destination.insert(
      destination.end(),
      std::make_move_iterator(source.begin()),
      std::make_move_iterator(source.end()));
}

[[nodiscard]] bool certified_carrier_layout(
    const ExactDirectMorseForestReducerFoldResult& result) noexcept {
  const std::size_t maximum_safe_group_count =
      (std::numeric_limits<std::size_t>::max() - 1U) / 2U;
  return result.implicit_singleton_carrier_count != 0U &&
         result.total_carrier_handle_count >=
             result.implicit_singleton_carrier_count &&
         result.materialized_direct_carrier_state_count ==
             result.total_carrier_handle_count -
                 result.implicit_singleton_carrier_count &&
         result.maximum_atomic_group_count <= maximum_safe_group_count &&
         result.root_override_slot_capacity ==
             2U * result.maximum_atomic_group_count + 1U &&
         result.locator_parent_authority_reused_by_carrier_state &&
         result.no_dense_singleton_carrier_state_materialized;
}

}  // namespace

bool ExactDirectMorseForestBatchSegment::certified_structure()
    const noexcept {
  std::size_t expected = 0U;
  const auto advances = [](std::size_t begin,
                           std::size_t increment,
                           std::size_t end) noexcept {
    std::size_t sum = 0U;
    return try_add(begin, increment, sum) && sum == end;
  };
  const std::size_t implicit_increment =
      end_cursor.implicit_order_one_prefix_count >=
              begin_cursor.implicit_order_one_prefix_count
          ? end_cursor.implicit_order_one_prefix_count -
                begin_cursor.implicit_order_one_prefix_count
          : std::numeric_limits<std::size_t>::max();
  std::size_t expected_birth_offset = 0U;
  std::size_t expected_committed_batch_count = 0U;
  if (!try_add(
          begin_cursor.implicit_order_one_prefix_count,
          begin_cursor.birth_record_count,
          expected_birth_offset) ||
      !try_add(
          batch.strict_pre_batch_stamp.committed_batch_count,
          1U,
          expected_committed_batch_count)) {
    return false;
  }
  if (schema_version != direct_morse_forest_output_segment_schema_version ||
      !derived_cache_only || forbidden_global_structure_materialized ||
      public_status_claimed ||
      !advances(begin_cursor.segment_count, 1U, end_cursor.segment_count) ||
      !advances(
          begin_cursor.birth_record_count,
          birth_records.size(),
          end_cursor.birth_record_count) ||
      !advances(
          begin_cursor.arm_root_binding_count,
          arm_root_bindings.size(),
          end_cursor.arm_root_binding_count) ||
      !advances(
          begin_cursor.saddle_record_count,
          saddle_records.size(),
          end_cursor.saddle_record_count) ||
      !advances(
          begin_cursor.atomic_group_count,
          atomic_groups.size(),
          end_cursor.atomic_group_count) ||
      !advances(
          begin_cursor.child_reference_count,
          child_node_ids.size(),
          end_cursor.child_reference_count) ||
      !advances(
          begin_cursor.batch_record_count,
          1U,
          end_cursor.batch_record_count) ||
      !advances(
          begin_cursor.node_count,
          nodes.size(),
          end_cursor.node_count) ||
      batch.batch_index != begin_cursor.batch_record_count ||
      batch.source_journal_batch_index != batch.batch_index ||
      batch.birth_record_offset != expected_birth_offset ||
      batch.birth_record_count !=
          (canonical_singleton_prefix_implicit
               ? implicit_increment
               : birth_records.size()) ||
      batch.saddle_record_offset != begin_cursor.saddle_record_count ||
      batch.saddle_record_count != saddle_records.size() ||
      batch.atomic_group_offset != begin_cursor.atomic_group_count ||
      batch.atomic_group_count != atomic_groups.size() ||
      !batch.strict_arms_resolved_before_mutation ||
      !batch.quotient_resolved_before_mutation ||
      !batch.unions_then_births_committed_atomically ||
      batch.committed_batch_stamp.committed_batch_count !=
          expected_committed_batch_count ||
      implicit_increment == std::numeric_limits<std::size_t>::max() ||
      (canonical_singleton_prefix_implicit
           ? (begin_cursor.segment_count != 0U || implicit_increment == 0U ||
              !birth_records.empty() || !nodes.empty())
           : implicit_increment != 0U)) {
    return false;
  }
  expected = begin_cursor.logical_output_entry_count;
  const auto add = [&](std::size_t value) noexcept {
    std::size_t sum = 0U;
    if (!try_add(expected, value, sum)) {
      return false;
    }
    expected = sum;
    return true;
  };
  if (!add(implicit_increment) || !add(implicit_increment) ||
      !add(implicit_increment)) {
    return false;
  }
  for (const auto& record : birth_records) {
    if (!add(record.facet_key.point_count)) {
      return false;
    }
  }
  for (const auto& record : arm_root_bindings) {
    if (!add(record.strict_arm_key.point_count)) {
      return false;
    }
  }
  for (const std::size_t count : {
           birth_records.size(),
           arm_root_bindings.size(),
           saddle_records.size(),
           atomic_groups.size(),
           child_node_ids.size(),
           std::size_t{1U},
           nodes.size()}) {
    if (!add(count)) {
      return false;
    }
  }
  if (expected != end_cursor.logical_output_entry_count) {
    return false;
  }
  try {
    return payload_digest == segment_payload_digest(*this) &&
           end_cursor.chain_digest == segment_chain_digest(*this);
  } catch (...) {
    return false;
  }
}

bool ExactDirectMorseForestSegmentDrainResult::certified_acknowledged()
    const noexcept {
  return decision == ExactDirectMorseForestSegmentDrainDecision::
                         complete_segment_acknowledged &&
         !pending_segment_retained && reducer_output_history_released &&
         post_drain_cursor.segment_count ==
             pre_drain_cursor.segment_count + 1U;
}

bool ExactDirectMorseForestSegmentDrainResult::
    certified_retryable_rejection() const noexcept {
  return (decision ==
              ExactDirectMorseForestSegmentDrainDecision::no_invalid_sink ||
          decision ==
              ExactDirectMorseForestSegmentDrainDecision::no_sink_rejected) &&
         pending_segment_retained && !reducer_output_history_released &&
         pre_drain_cursor == post_drain_cursor;
}

bool ExactDirectMorseForestFinalSeal::certified_conditional_h0_candidate()
    const noexcept {
  return schema_version == direct_morse_forest_output_segment_schema_version &&
         point_count != 0U && effective_maximum_order != 0U &&
         source_higher_canonical_cloud_digest != contract::CanonicalId{} &&
         final_cursor.implicit_order_one_prefix_count == point_count &&
         final_cursor.batch_record_count == final_cursor.segment_count &&
         counters.birth_record_count ==
             final_cursor.implicit_order_one_prefix_count +
                 final_cursor.birth_record_count &&
         counters.latent_higher_order_birth_count ==
             final_cursor.birth_record_count &&
         counters.order_one_birth_node_count == point_count &&
         counters.arm_root_binding_count ==
             final_cursor.arm_root_binding_count &&
         counters.saddle_record_count == final_cursor.saddle_record_count &&
         counters.atomic_group_count == final_cursor.atomic_group_count &&
         counters.child_reference_count ==
             final_cursor.child_reference_count &&
         counters.batch_record_count == final_cursor.batch_record_count &&
         counters.node_count == point_count + final_cursor.node_count &&
         counters.final_root_count == final_roots.size() &&
         logical_output_entry_count ==
             final_cursor.logical_output_entry_count + final_roots.size() &&
         final_locator_stamp.committed_batch_count ==
             final_cursor.segment_count &&
         all_source_batches_committed && every_segment_acknowledged &&
         derived_cache_only && conditional_exact_h0_only &&
         !forbidden_global_structure_materialized &&
         !public_status_claimed;
}

ExactDirectMorseForestReducerBatch
project_exact_direct_morse_forest_reducer_batch(
    const ExactDirectSparseFacetDescentBatchExecutionResult& source) {
  if (!source.complete_architecture_execution()) {
    throw std::invalid_argument(
        "a forest reducer projection requires one complete 14D delta");
  }
  ExactDirectMorseForestReducerBatch projected;
  projected.source_batch_index = source.source_batch_index;
  projected.source_chunk_index = source.source_chunk_index;
  projected.source_family_begin_index = source.source_family_begin_index;
  projected.source_family_end_index = source.source_family_end_index;
  projected.source_arm_seed_begin_index =
      source.source_arm_seed_begin_index;
  projected.source_arm_seed_end_index = source.source_arm_seed_end_index;
  projected.order = source.source_facet_cardinality;
  projected.squared_level = source.closed_batch_squared_level;
  projected.traversal_order = source.traversal_order;
  projected.locator_query_witness = source.locator_query_witness;
  projected.strict_pre_batch_locator_stamp =
      source.locator_snapshot_stamp;
  projected.requested_closure_budget = source.requested_closure_budget;
  projected.transient_closure_node_count =
      source.closure_summary.transient_node_count;
  projected.transient_closure_step_call_count =
      source.closure_summary.counters.evaluated_step_source_count;
  projected.shared_closure_build_count =
      source.counters.shared_closure_build_count;
  projected.resolved_keys = source.resolved_keys;
  projected.arm_joins = source.arm_joins;
  return projected;
}

bool verify_exact_direct_morse_forest_reducer_fold_layout(
    const ExactDirectMorseForestReducerFoldResult& observed,
    std::size_t trusted_total_carrier_handle_count,
    std::size_t trusted_implicit_singleton_carrier_count,
    std::size_t trusted_maximum_atomic_group_count) noexcept {
  return observed.schema_version ==
             direct_morse_forest_reducer_schema_version &&
         certified_carrier_layout(observed) &&
         observed.total_carrier_handle_count ==
             trusted_total_carrier_handle_count &&
         observed.implicit_singleton_carrier_count ==
             trusted_implicit_singleton_carrier_count &&
         observed.maximum_atomic_group_count ==
             trusted_maximum_atomic_group_count;
}

bool ExactDirectMorseForestReducerFoldResult::certified_committed_batch()
    const noexcept {
  const bool staging_audit_consistent =
      canonical_singleton_bulk_count == 0U ||
      (staged_birth_record_count == 0U &&
       staged_birth_node_count == 0U &&
       staged_locator_binding_count == 0U);
  return schema_version == direct_morse_forest_reducer_schema_version &&
         staging_audit_consistent && certified_carrier_layout(*this) &&
         complete_batch_staged_before_mutation &&
         full_equal_level_quotient_resolved_before_mutation &&
         locator_committed_before_scientific_state &&
         scientific_state_committed && reducer_state_mutated &&
         !source_chunk_index_has_scientific_authority &&
         !forbidden_global_structure_materialized &&
         !public_status_claimed &&
         post_fold_locator_stamp.committed_batch_count ==
             pre_fold_locator_stamp.committed_batch_count + 1U &&
         decision == ExactDirectMorseForestReducerFoldDecision::
                         complete_reducer_batch_commit;
}

bool ExactDirectMorseForestReducerFoldResult::certified_atomic_rejection()
    const noexcept {
  return schema_version == direct_morse_forest_reducer_schema_version &&
         certified_carrier_layout(*this) &&
         decision != ExactDirectMorseForestReducerFoldDecision::not_folded &&
         decision != ExactDirectMorseForestReducerFoldDecision::
                         complete_reducer_batch_commit &&
         !locator_committed_before_scientific_state &&
         !scientific_state_committed && !reducer_state_mutated &&
         !source_chunk_index_has_scientific_authority &&
         !forbidden_global_structure_materialized &&
         !public_status_claimed &&
         pre_fold_locator_stamp == post_fold_locator_stamp;
}

bool ExactDirectMorseForestLiveCommitResult::certified_live_commit()
    const noexcept {
  return schema_version == direct_morse_forest_live_commit_schema_version &&
         scientific_delta.has_value() &&
         source_batch_index == scientific_delta->source_batch_index &&
         pre_commit_executor_batch_index == source_batch_index &&
         post_commit_executor_batch_index == successor_batch_index &&
         ticket_was_valid_and_unconsumed &&
         shared_session_seal_matches &&
         source_epoch_and_full_cursor_match &&
         exact_scientific_delta_provenance_minted &&
         reducer_and_executor_share_locator_instance &&
         reducer_and_executor_batch_cursors_match &&
         executor_commit_capacity_preflighted &&
         all_fallible_scientific_work_precedes_irreversible_mutation &&
         reducer_fold_attempted &&
         reducer_fold.certified_committed_batch() &&
         reducer_fold.source_batch_index == source_batch_index &&
         reducer_fold.pre_fold_locator_stamp ==
             pre_commit_locator_stamp &&
         reducer_fold.post_fold_locator_stamp ==
             post_commit_locator_stamp &&
         reducer_committed_before_executor_cursor &&
         no_fallible_operation_after_reducer_commit &&
         executor_cursor_advanced &&
         scientific_delta_moved_to_result && ticket_consumed &&
         !executor_cursor_unchanged_on_rejection &&
         !locator_unchanged_on_rejection &&
         !independent_geometry_replay_performed &&
         !forbidden_global_structure_materialized &&
         !public_status_claimed &&
         decision == ExactDirectMorseForestLiveCommitDecision::
                         complete_live_reducer_then_cursor_commit;
}

bool ExactDirectMorseForestLiveCommitResult::certified_atomic_rejection()
    const noexcept {
  const bool fold_is_atomic =
      reducer_fold_attempted
          ? reducer_fold.certified_atomic_rejection()
          : reducer_fold.decision ==
                ExactDirectMorseForestReducerFoldDecision::not_folded;
  return schema_version == direct_morse_forest_live_commit_schema_version &&
         decision !=
             ExactDirectMorseForestLiveCommitDecision::not_committed &&
         decision != ExactDirectMorseForestLiveCommitDecision::
                         complete_live_reducer_then_cursor_commit &&
         fold_is_atomic &&
         !reducer_committed_before_executor_cursor &&
         !executor_cursor_advanced &&
         !scientific_delta_moved_to_result &&
         !scientific_delta.has_value() && ticket_consumed &&
         executor_cursor_unchanged_on_rejection &&
         locator_unchanged_on_rejection &&
         pre_commit_executor_batch_index ==
             post_commit_executor_batch_index &&
         pre_commit_locator_stamp == post_commit_locator_stamp &&
         !independent_geometry_replay_performed &&
         !forbidden_global_structure_materialized &&
         !public_status_claimed;
}

class ExactDirectMorseForestReducer::Impl {
 public:
  Impl(
      const spatial::CanonicalPointCloud& cloud,
      const ExactDirectSupportTerminalFacade& source_facade,
      const ExactDirectMorseEventJournalResult& source_journal,
      const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
      const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
      const ExactDirectMorseForestBudget& budget,
      const ExactDirectMorseForestConfig& config,
      spatial::LbvhTraversalOrder traversal_order,
      std::optional<ExactDirectMorseForestSegmentLimits> segment_limits =
          std::nullopt,
      contract::CanonicalId initial_chain_digest = {})
      : owned_source_adapter_(
            std::make_unique<ExactDirectMorseForestResidentSourceAdapter>(
                cloud,
                source_facade,
                source_journal,
                trusted_seed_budget,
                source_seed_journal)),
        source_manifest_(owned_source_adapter_->manifest()),
        source_provider_(*owned_source_adapter_),
        budget_(budget),
        config_(config),
        traversal_order_(traversal_order),
        segmented_output_enabled_(segment_limits.has_value()),
        segment_limits_(segment_limits.value_or(
            ExactDirectMorseForestSegmentLimits{})),
        locator_(build_locator(
            source_manifest_,
            budget,
            config,
            traversal_order,
            birth_count_)),
        components_(
            locator_,
            birth_count_,
            source_manifest_.point_count,
            source_manifest_.effective_maximum_order,
            budget.maximum_atomic_group_count) {
    initialize(initial_chain_digest);
  }

  Impl(
      const ExactDirectMorseForestSourceManifest& source_manifest,
      ExactDirectMorseForestSourceBatchProviderView source_provider,
      const ExactDirectMorseForestBudget& budget,
      const ExactDirectMorseForestConfig& config,
      spatial::LbvhTraversalOrder traversal_order,
      std::optional<ExactDirectMorseForestSegmentLimits> segment_limits =
          std::nullopt,
      contract::CanonicalId initial_chain_digest = {})
      : source_manifest_(source_manifest),
        source_provider_(std::move(source_provider)),
        budget_(budget),
        config_(config),
        traversal_order_(traversal_order),
        segmented_output_enabled_(segment_limits.has_value()),
        segment_limits_(segment_limits.value_or(
            ExactDirectMorseForestSegmentLimits{})),
        locator_(build_locator(
            source_manifest,
            budget,
            config,
            traversal_order,
            birth_count_)),
        components_(
            locator_,
            birth_count_,
            source_manifest.point_count,
            source_manifest.effective_maximum_order,
            budget.maximum_atomic_group_count) {
    if (!source_provider_) {
      throw std::invalid_argument(
          "a forest reducer requires one synchronous source provider");
    }
    initialize(initial_chain_digest);
  }

 private:
  void initialize(const contract::CanonicalId& initial_chain_digest) {
    result_.requested_budget = budget_;
    result_.config = config_;
    result_.traversal_order = traversal_order_;
    result_.point_count = source_manifest_.point_count;
    result_.effective_maximum_order =
        source_manifest_.effective_maximum_order;
    result_.source_higher_canonical_cloud_digest =
        source_manifest_.source_higher_canonical_cloud_digest;
    result_.source_event_projection_count =
        source_manifest_.logical_event_projection_count;
    initialize_scope(result_);
    result_.source_event_journal_freshly_replayed = false;
    result_.source_strict_arm_journal_freshly_replayed = false;
    result_.budget_preflight_certified = true;
    output_cursor_.chain_digest = initial_chain_digest;
    expected_source_chain_digest_ =
        source_manifest_.initial_batch_chain_digest;

    const std::size_t family_count = source_manifest_.family_count;
    const std::size_t arm_count = source_manifest_.arm_seed_count;
    const std::size_t batch_count = source_manifest_.batch_count;
    std::size_t node_capacity = 0U;
    if (!try_add(birth_count_, family_count, node_capacity)) {
      throw std::length_error("a reducer node capacity overflowed");
    }
    if (segmented_output_enabled_) {
      if (segment_limits_.maximum_birth_record_count >
              budget_.maximum_birth_record_count ||
          segment_limits_.maximum_arm_root_binding_count >
              budget_.maximum_arm_root_binding_count ||
          segment_limits_.maximum_saddle_record_count >
              budget_.maximum_saddle_record_count ||
          segment_limits_.maximum_atomic_group_count >
              budget_.maximum_atomic_group_count ||
          segment_limits_.maximum_child_reference_count >
              budget_.maximum_child_reference_count ||
          segment_limits_.maximum_node_count >
              budget_.maximum_node_count) {
        throw std::invalid_argument(
            "forest segment limits must refine the global budget");
      }
      result_.birth_records.reserve(
          segment_limits_.maximum_birth_record_count);
      result_.arm_root_bindings.reserve(
          segment_limits_.maximum_arm_root_binding_count);
      result_.saddle_records.reserve(
          segment_limits_.maximum_saddle_record_count);
      result_.atomic_groups.reserve(
          segment_limits_.maximum_atomic_group_count);
      result_.child_node_ids.reserve(
          segment_limits_.maximum_child_reference_count);
      result_.batches.reserve(1U);
      result_.nodes.reserve(segment_limits_.maximum_node_count);
    } else {
      result_.birth_records.reserve(
          birth_count_ - source_manifest_.point_count);
      result_.arm_root_bindings.reserve(arm_count);
      result_.saddle_records.reserve(family_count);
      result_.atomic_groups.reserve(
          std::min(family_count, budget_.maximum_atomic_group_count));
      result_.child_node_ids.reserve(
          std::min(arm_count, budget_.maximum_child_reference_count));
      result_.batches.reserve(batch_count);
      result_.nodes.reserve(
          node_capacity - source_manifest_.point_count);
    }
    result_.final_roots.reserve(std::min(
        result_.effective_maximum_order,
        budget_.maximum_final_root_count));
  }

 public:

  [[nodiscard]] ExactDirectMorseForestReducerFoldResult fold(
      const ExactDirectMorseForestReducerBatch& batch) {
    ExactDirectMorseForestReducerFoldResult folded;
    folded.source_batch_index = batch.source_batch_index;
    folded.pre_fold_locator_stamp = locator_.snapshot_stamp();
    folded.post_fold_locator_stamp = folded.pre_fold_locator_stamp;
    folded.implicit_singleton_carrier_count =
        components_.implicit_singleton_count();
    folded.materialized_direct_carrier_state_count =
        components_.materialized_direct_state_count();
    folded.total_carrier_handle_count = components_.handle_count();
    folded.maximum_atomic_group_count =
        budget_.maximum_atomic_group_count;
    folded.root_override_slot_capacity =
        components_.root_override_slot_capacity();
    folded.locator_parent_authority_reused_by_carrier_state = true;
    folded.no_dense_singleton_carrier_state_materialized = true;

    if (finished_) {
      folded.decision =
          ExactDirectMorseForestReducerFoldDecision::no_reducer_finished;
      return folded;
    }
    if (pending_segment_.has_value()) {
      folded.decision = ExactDirectMorseForestReducerFoldDecision::
          no_reducer_output_segment_pending;
      return folded;
    }
    if (batch.source_batch_index != next_batch_index_) {
      folded.decision = ExactDirectMorseForestReducerFoldDecision::
          no_reducer_batch_out_of_order;
      return folded;
    }

    try {
      return fold_checked(batch, std::move(folded));
    } catch (const std::length_error&) {
      folded.post_fold_locator_stamp = locator_.snapshot_stamp();
      folded.decision = ExactDirectMorseForestReducerFoldDecision::
          no_reducer_budget_exhausted;
      return folded;
    } catch (const std::bad_alloc&) {
      folded.post_fold_locator_stamp = locator_.snapshot_stamp();
      folded.decision = ExactDirectMorseForestReducerFoldDecision::
          no_reducer_operational_allocation_failed;
      return folded;
    } catch (const std::logic_error&) {
      folded.post_fold_locator_stamp = locator_.snapshot_stamp();
      folded.decision = ExactDirectMorseForestReducerFoldDecision::
          no_reducer_batch_inconsistent;
      return folded;
    }
  }

  [[nodiscard]] const ExactDirectSparsePositiveFacetLocator& locator()
      const noexcept {
    return locator_;
  }

  [[nodiscard]] std::size_t next_batch_index() const noexcept {
    return next_batch_index_;
  }

  [[nodiscard]] bool complete() const noexcept {
    return !finished_ &&
           next_batch_index_ == source_manifest_.batch_count;
  }

  [[nodiscard]] bool segmented_output_enabled() const noexcept {
    return segmented_output_enabled_;
  }

  [[nodiscard]] bool has_pending_output_segment() const noexcept {
    return pending_segment_.has_value();
  }

  [[nodiscard]] const ExactDirectMorseForestBatchSegment*
  pending_output_segment() const noexcept {
    return pending_segment_.has_value() ? &*pending_segment_ : nullptr;
  }

  [[nodiscard]] const ExactDirectMorseForestSegmentCursor& output_cursor()
      const noexcept {
    return output_cursor_;
  }

  [[nodiscard]] ExactDirectMorseForestSegmentDrainResult
  drain_pending_output_segment(
      ExactDirectMorseForestSegmentSinkView sink) noexcept {
    ExactDirectMorseForestSegmentDrainResult drained;
    drained.pre_drain_cursor = output_cursor_;
    drained.post_drain_cursor = output_cursor_;
    if (!pending_segment_.has_value()) {
      drained.decision =
          ExactDirectMorseForestSegmentDrainDecision::no_pending_segment;
      return drained;
    }
    drained.pending_segment_retained = true;
    if (!sink.valid()) {
      drained.decision =
          ExactDirectMorseForestSegmentDrainDecision::no_invalid_sink;
      return drained;
    }
    if (pending_segment_->schema_version !=
            direct_morse_forest_output_segment_schema_version ||
        pending_segment_->begin_cursor != output_cursor_ ||
        pending_segment_->end_cursor.segment_count !=
            output_cursor_.segment_count + 1U ||
        !pending_segment_->derived_cache_only ||
        pending_segment_->forbidden_global_structure_materialized ||
        pending_segment_->public_status_claimed) {
      std::terminate();
    }
    if (!sink.try_append(*pending_segment_)) {
      drained.decision =
          ExactDirectMorseForestSegmentDrainDecision::no_sink_rejected;
      return drained;
    }

    output_cursor_ = pending_segment_->end_cursor;
    pending_segment_->birth_records.clear();
    pending_segment_->arm_root_bindings.clear();
    pending_segment_->saddle_records.clear();
    pending_segment_->atomic_groups.clear();
    pending_segment_->child_node_ids.clear();
    pending_segment_->nodes.clear();
    result_.birth_records.swap(pending_segment_->birth_records);
    result_.arm_root_bindings.swap(
        pending_segment_->arm_root_bindings);
    result_.saddle_records.swap(pending_segment_->saddle_records);
    result_.atomic_groups.swap(pending_segment_->atomic_groups);
    result_.child_node_ids.swap(pending_segment_->child_node_ids);
    result_.nodes.swap(pending_segment_->nodes);
    pending_segment_.reset();

    drained.post_drain_cursor = output_cursor_;
    drained.pending_segment_retained = false;
    drained.reducer_output_history_released = true;
    drained.decision = ExactDirectMorseForestSegmentDrainDecision::
        complete_segment_acknowledged;
    return drained;
  }

  [[nodiscard]] ExactDirectMorseForestJournalResult finish() {
    if (segmented_output_enabled_) {
      throw std::logic_error(
          "a segmented forest reducer requires finish_segmented");
    }
    if (finished_ || !complete() ||
        expected_source_chain_digest_ !=
            source_manifest_.final_batch_chain_digest ||
        recertified_source_window_count_ != source_manifest_.batch_count ||
        next_family_index_ != source_manifest_.family_count ||
        next_arm_seed_index_ != source_manifest_.arm_seed_count ||
        logical_birth_record_count(result_) != birth_count_ ||
        result_.saddle_records.size() != source_manifest_.family_count ||
        result_.arm_root_bindings.size() !=
            source_manifest_.arm_seed_count) {
      throw std::logic_error(
          "a direct Morse forest reducer cannot finish an incomplete stream");
    }

    std::vector<ExactDirectMorseForestFinalRoot> pending_final_roots;
    pending_final_roots.reserve(std::min(
        result_.effective_maximum_order,
        budget_.maximum_final_root_count));
    for (std::size_t order = 1U;
         order <= result_.effective_maximum_order;
         ++order) {
      const bool expected = order == 1U || order < result_.point_count;
      if (components_.carrier_count(order) != (expected ? 1U : 0U)) {
        throw std::logic_error(
            "a reducer final carrier partition is incomplete");
      }
      if (!expected) {
        continue;
      }
      const auto representative =
          components_.representative_handle(order);
      if (!representative.has_value() ||
          !components_.active_root(*representative) ||
          components_.order(*representative) != order) {
        throw std::logic_error(
            "a reducer final carrier representative is invalid");
      }
      const auto reduced_root =
          components_.reduced_root(*representative);
      if (!reduced_root.has_value()) {
        throw std::logic_error(
            "a reducer final carrier has no reduced root");
      }
      if (pending_final_roots.size() >=
          budget_.maximum_final_root_count) {
        throw std::logic_error(
            "a reducer final-root budget is exhausted");
      }
      pending_final_roots.push_back(
          {pending_final_roots.size(),
           order,
           components_.canonical(*representative),
           *reduced_root});
    }
    std::sort(
        pending_final_roots.begin(),
        pending_final_roots.end(),
        [](const ExactDirectMorseForestFinalRoot& left,
           const ExactDirectMorseForestFinalRoot& right) {
          return left.order < right.order ||
                 (left.order == right.order &&
                  left.root_node_id < right.root_node_id);
        });
    for (std::size_t index = 0U;
         index < pending_final_roots.size();
         ++index) {
      pending_final_roots[index].final_root_index = index;
    }

    std::vector<std::size_t> root_count_by_order(
        result_.effective_maximum_order + 1U, 0U);
    for (const auto& root : pending_final_roots) {
      if (root.order == 0U ||
          root.order > result_.effective_maximum_order) {
        throw std::logic_error("a reducer final root has an invalid order");
      }
      ++root_count_by_order[root.order];
    }
    for (std::size_t order = 1U;
         order <= result_.effective_maximum_order;
         ++order) {
      const bool expected = order == 1U || order < result_.point_count;
      if (root_count_by_order[order] != (expected ? 1U : 0U)) {
        throw std::logic_error(
            "a reducer final-root partition is incomplete");
      }
    }

    std::size_t logical = result_.implicit_order_one_prefix_count;
    for (const auto& birth : result_.birth_records) {
      if (!try_add(logical, birth.facet_key.point_count, logical)) {
        throw std::length_error("a reducer output count overflowed");
      }
    }
    for (const auto& arm : result_.arm_root_bindings) {
      if (!try_add(logical, arm.strict_arm_key.point_count, logical)) {
        throw std::length_error("a reducer output count overflowed");
      }
    }
    for (const std::size_t increment : {
             logical_birth_record_count(result_),
             result_.arm_root_bindings.size(),
             result_.saddle_records.size(),
             result_.atomic_groups.size(),
             result_.child_node_ids.size(),
             result_.batches.size(),
             pending_final_roots.size(),
             logical_node_count(result_)}) {
      if (!try_add(logical, increment, logical)) {
        throw std::length_error("a reducer output count overflowed");
      }
    }
    if (logical > budget_.maximum_logical_output_entry_count) {
      throw std::logic_error(
          "a reducer logical-output budget is exhausted");
    }
    result_.final_roots.swap(pending_final_roots);
    result_.logical_output_entry_count = logical;
    result_.final_locator_stamp = locator_.snapshot_stamp();
    result_.counters.birth_record_count =
        logical_birth_record_count(result_);
    result_.counters.order_one_birth_node_count =
        result_.implicit_order_one_prefix_count;
    result_.counters.latent_higher_order_birth_count =
        result_.birth_records.size();
    result_.counters.arm_root_binding_count =
        result_.arm_root_bindings.size();
    result_.counters.saddle_record_count = result_.saddle_records.size();
    result_.counters.atomic_group_count = result_.atomic_groups.size();
    result_.counters.child_reference_count =
        result_.child_node_ids.size();
    result_.counters.batch_record_count = result_.batches.size();
    result_.counters.node_count = logical_node_count(result_);
    result_.counters.final_root_count = result_.final_roots.size();

    result_.every_birth_key_reconstructed_from_closed_direct_event = true;
    result_.deterministic_disjoint_birth_union_and_query_tokens = true;
    result_.batches_processed_in_strict_order_level_order = true;
    result_.cardinality_isolates_orders_in_shared_locator = true;
    result_.current_level_births_hidden_from_arm_descent = true;
    result_.higher_order_direct_births_are_latent_carriers = true;
    result_.one_10_5c_call_per_nonempty_strict_arm_batch = true;
    result_.every_strict_arm_has_positive_terminal = true;
    result_.all_catalogued_saddle_families_consumed_once = true;
    result_.carrier_to_optional_reduced_root_authority_maintained = true;
    result_.every_saddle_has_positive_carrier = true;
    result_.typed_root_or_latent_carrier_hyperedges_closed_transitively = true;
    result_.q_r_counts_only_distinct_prior_reduced_roots = true;
    result_.all_equal_level_saddles_quotiented_before_mutation = true;
    result_.saddle_records_grouped_with_source_family_provenance = true;
    result_.q_zero_groups_create_one_reduced_birth = true;
    result_.q_one_continuations_create_no_node = true;
    result_.q_at_least_two_groups_create_one_multifusion = true;
    result_.current_batch_birth_nodes_never_same_batch_children = true;
    result_.all_group_carriers_attached_to_resulting_root_atomically = true;
    result_.locator_commits_unions_before_current_birth_bindings = true;
    result_.final_roots_cover_exactly_nonterminal_reduced_orders = true;
    result_.no_partial_scientific_payload_published = true;
    result_.source_event_journal_freshly_replayed = true;
    result_.source_strict_arm_journal_freshly_replayed = true;
    result_.decision = ExactDirectMorseForestDecision::
        complete_conditional_exact_direct_morse_forest;
    if (!result_.certified_conditional_h0_candidate()) {
      throw std::logic_error(
          "a direct Morse forest reducer failed the forest contract");
    }
    finished_ = true;
    return std::move(result_);
  }

  [[nodiscard]] ExactDirectMorseForestFinalSeal finish_segmented() {
    if (!segmented_output_enabled_ || finished_ || !complete() ||
        pending_segment_.has_value() ||
        expected_source_chain_digest_ !=
            source_manifest_.final_batch_chain_digest ||
        recertified_source_window_count_ != source_manifest_.batch_count ||
        next_family_index_ != source_manifest_.family_count ||
        next_arm_seed_index_ != source_manifest_.arm_seed_count ||
        output_cursor_.implicit_order_one_prefix_count +
                output_cursor_.birth_record_count !=
            birth_count_ ||
        output_cursor_.saddle_record_count !=
            source_manifest_.family_count ||
        output_cursor_.arm_root_binding_count !=
            source_manifest_.arm_seed_count ||
        output_cursor_.batch_record_count !=
            source_manifest_.batch_count ||
        output_cursor_.segment_count != next_batch_index_) {
      throw std::logic_error(
          "a segmented forest reducer cannot seal an incomplete stream");
    }

    std::vector<ExactDirectMorseForestFinalRoot> final_roots;
    final_roots.reserve(std::min(
        result_.effective_maximum_order,
        budget_.maximum_final_root_count));
    for (std::size_t order = 1U;
         order <= result_.effective_maximum_order;
         ++order) {
      const bool expected = order == 1U || order < result_.point_count;
      if (components_.carrier_count(order) != (expected ? 1U : 0U)) {
        throw std::logic_error(
            "a segmented reducer final carrier partition is incomplete");
      }
      if (!expected) {
        continue;
      }
      const auto representative = components_.representative_handle(order);
      if (!representative.has_value() ||
          !components_.active_root(*representative) ||
          components_.order(*representative) != order) {
        throw std::logic_error(
            "a segmented reducer final representative is invalid");
      }
      const auto reduced_root = components_.reduced_root(*representative);
      if (!reduced_root.has_value() ||
          final_roots.size() >= budget_.maximum_final_root_count) {
        throw std::logic_error(
            "a segmented reducer final root is invalid or over budget");
      }
      final_roots.push_back(
          {final_roots.size(),
           order,
           components_.canonical(*representative),
           *reduced_root});
    }
    std::sort(
        final_roots.begin(),
        final_roots.end(),
        [](const ExactDirectMorseForestFinalRoot& left,
           const ExactDirectMorseForestFinalRoot& right) {
          return left.order < right.order ||
                 (left.order == right.order &&
                  left.root_node_id < right.root_node_id);
        });
    for (std::size_t index = 0U; index < final_roots.size(); ++index) {
      final_roots[index].final_root_index = index;
    }

    std::size_t logical_output_entry_count = 0U;
    if (!try_add(
            output_cursor_.logical_output_entry_count,
            final_roots.size(),
            logical_output_entry_count) ||
        logical_output_entry_count >
            budget_.maximum_logical_output_entry_count) {
      throw std::logic_error(
          "a segmented reducer logical output budget is exhausted");
    }

    ExactDirectMorseForestFinalSeal seal;
    seal.point_count = result_.point_count;
    seal.effective_maximum_order = result_.effective_maximum_order;
    seal.source_higher_canonical_cloud_digest =
        result_.source_higher_canonical_cloud_digest;
    seal.final_cursor = output_cursor_;
    seal.final_locator_stamp = locator_.snapshot_stamp();
    seal.counters = result_.counters;
    seal.counters.birth_record_count =
        output_cursor_.implicit_order_one_prefix_count +
        output_cursor_.birth_record_count;
    seal.counters.order_one_birth_node_count =
        output_cursor_.implicit_order_one_prefix_count;
    seal.counters.latent_higher_order_birth_count =
        output_cursor_.birth_record_count;
    seal.counters.arm_root_binding_count =
        output_cursor_.arm_root_binding_count;
    seal.counters.saddle_record_count = output_cursor_.saddle_record_count;
    seal.counters.atomic_group_count = output_cursor_.atomic_group_count;
    seal.counters.child_reference_count =
        output_cursor_.child_reference_count;
    seal.counters.batch_record_count = output_cursor_.batch_record_count;
    seal.counters.node_count =
        output_cursor_.implicit_order_one_prefix_count +
        output_cursor_.node_count;
    seal.counters.final_root_count = final_roots.size();
    seal.final_roots = std::move(final_roots);
    seal.logical_output_entry_count = logical_output_entry_count;
    seal.all_source_batches_committed = true;
    seal.every_segment_acknowledged = true;
    if (!seal.certified_conditional_h0_candidate()) {
      throw std::logic_error(
          "a segmented reducer failed its terminal seal contract");
    }
    finished_ = true;
    return seal;
  }

 private:
  [[nodiscard]] std::size_t global_physical_birth_record_count()
      const noexcept {
    return output_cursor_.birth_record_count +
           result_.birth_records.size();
  }

  [[nodiscard]] std::size_t global_logical_birth_record_count()
      const noexcept {
    return result_.implicit_order_one_prefix_count +
           global_physical_birth_record_count();
  }

  [[nodiscard]] std::size_t global_arm_root_binding_count()
      const noexcept {
    return output_cursor_.arm_root_binding_count +
           result_.arm_root_bindings.size();
  }

  [[nodiscard]] std::size_t global_saddle_record_count() const noexcept {
    return output_cursor_.saddle_record_count +
           result_.saddle_records.size();
  }

  [[nodiscard]] std::size_t global_atomic_group_count() const noexcept {
    return output_cursor_.atomic_group_count +
           result_.atomic_groups.size();
  }

  [[nodiscard]] std::size_t global_child_reference_count() const noexcept {
    return output_cursor_.child_reference_count +
           result_.child_node_ids.size();
  }

  [[nodiscard]] std::size_t global_batch_record_count() const noexcept {
    return output_cursor_.batch_record_count + result_.batches.size();
  }

  [[nodiscard]] std::size_t global_physical_node_count() const noexcept {
    return output_cursor_.node_count + result_.nodes.size();
  }

  [[nodiscard]] std::size_t global_logical_node_count() const noexcept {
    return result_.implicit_order_one_prefix_count +
           global_physical_node_count();
  }

  void prepare_pending_segment(
      bool canonical_singleton_prefix,
      std::size_t committed_implicit_prefix_count) {
    if (!segmented_output_enabled_) {
      return;
    }
    const PayloadSizes sizes = payload_sizes(result_);
    if (pending_segment_.has_value() || sizes.batches != 1U) {
      throw std::logic_error(
          "a segmented reducer must stage exactly one fresh batch");
    }
    if (sizes.birth_records >
            segment_limits_.maximum_birth_record_count ||
        sizes.arm_root_bindings >
            segment_limits_.maximum_arm_root_binding_count ||
        sizes.saddle_records >
            segment_limits_.maximum_saddle_record_count ||
        sizes.atomic_groups >
            segment_limits_.maximum_atomic_group_count ||
        sizes.child_node_ids >
            segment_limits_.maximum_child_reference_count ||
        sizes.nodes > segment_limits_.maximum_node_count) {
      throw std::length_error(
          "a forest batch exceeds its segment output limits");
    }

    ExactDirectMorseForestSegmentCursor end = output_cursor_;
    const auto advance = [](std::size_t& value, std::size_t increment) {
      std::size_t sum = 0U;
      if (!try_add(value, increment, sum)) {
        throw std::length_error("a forest segment cursor overflowed");
      }
      value = sum;
    };
    advance(end.segment_count, 1U);
    advance(end.birth_record_count, sizes.birth_records);
    advance(end.arm_root_binding_count, sizes.arm_root_bindings);
    advance(end.saddle_record_count, sizes.saddle_records);
    advance(end.atomic_group_count, sizes.atomic_groups);
    advance(end.child_reference_count, sizes.child_node_ids);
    advance(end.batch_record_count, 1U);
    advance(end.node_count, sizes.nodes);
    if (canonical_singleton_prefix) {
      if (output_cursor_.segment_count != 0U ||
          output_cursor_.implicit_order_one_prefix_count != 0U ||
          committed_implicit_prefix_count == 0U) {
        throw std::logic_error(
            "a segmented singleton prefix is not the first batch");
      }
      end.implicit_order_one_prefix_count =
          committed_implicit_prefix_count;
    } else if (committed_implicit_prefix_count !=
               output_cursor_.implicit_order_one_prefix_count) {
      throw std::logic_error(
          "a segmented reducer changed its implicit singleton prefix");
    }

    const std::size_t implicit_increment =
        end.implicit_order_one_prefix_count -
        output_cursor_.implicit_order_one_prefix_count;
    for (std::size_t copy = 0U; copy < 3U; ++copy) {
      advance(end.logical_output_entry_count, implicit_increment);
    }
    for (const auto& birth : result_.birth_records) {
      advance(end.logical_output_entry_count, birth.facet_key.point_count);
    }
    for (const auto& arm : result_.arm_root_bindings) {
      advance(
          end.logical_output_entry_count,
          arm.strict_arm_key.point_count);
    }
    for (const std::size_t count : {
             sizes.birth_records,
             sizes.arm_root_bindings,
             sizes.saddle_records,
             sizes.atomic_groups,
             sizes.child_node_ids,
             std::size_t{1U},
             sizes.nodes}) {
      advance(end.logical_output_entry_count, count);
    }
    if (end.logical_output_entry_count >
        budget_.maximum_logical_output_entry_count) {
      throw std::length_error(
          "a forest segment exceeds the logical output budget");
    }

    try {
      pending_segment_.emplace();
      pending_segment_->begin_cursor = output_cursor_;
      pending_segment_->end_cursor = end;
      pending_segment_->batch = result_.batches.back();
      pending_segment_->canonical_singleton_prefix_implicit =
          canonical_singleton_prefix;
      pending_segment_->payload_digest = segment_payload_digest_from_parts(
          pending_segment_->begin_cursor,
          pending_segment_->batch,
          result_.birth_records,
          result_.arm_root_bindings,
          result_.saddle_records,
          result_.atomic_groups,
          result_.child_node_ids,
          result_.nodes,
          canonical_singleton_prefix);
    } catch (...) {
      pending_segment_.reset();
      throw;
    }
  }

  void discard_prepared_pending_segment() noexcept {
    if (segmented_output_enabled_) {
      pending_segment_.reset();
    }
  }

  void publish_pending_segment_after_commit() noexcept {
    if (!segmented_output_enabled_) {
      return;
    }
    if (!pending_segment_.has_value() || result_.batches.size() != 1U) {
      std::terminate();
    }
    const auto& committed_batch = result_.batches.back();
    pending_segment_->batch.closed_post_batch_carrier_count =
        committed_batch.closed_post_batch_carrier_count;
    pending_segment_->batch.closed_post_batch_reduced_root_count =
        committed_batch.closed_post_batch_reduced_root_count;
    pending_segment_->batch.committed_batch_stamp =
        committed_batch.committed_batch_stamp;
    pending_segment_->batch.unions_then_births_committed_atomically =
        committed_batch.unions_then_births_committed_atomically;
    pending_segment_->end_cursor.chain_digest =
        segment_chain_digest(*pending_segment_);

    pending_segment_->birth_records.swap(result_.birth_records);
    pending_segment_->arm_root_bindings.swap(result_.arm_root_bindings);
    pending_segment_->saddle_records.swap(result_.saddle_records);
    pending_segment_->atomic_groups.swap(result_.atomic_groups);
    pending_segment_->child_node_ids.swap(result_.child_node_ids);
    pending_segment_->nodes.swap(result_.nodes);
    result_.batches.clear();
  }

  static ExactDirectSparsePositiveFacetLocator build_locator(
      const ExactDirectMorseForestSourceManifest& source_manifest,
      const ExactDirectMorseForestBudget& budget,
      const ExactDirectMorseForestConfig& config,
      spatial::LbvhTraversalOrder traversal_order,
      std::size_t& birth_count) {
    require_traversal_order(traversal_order);
    if (config.locator_config.external_authority_id == 0U) {
      throw std::invalid_argument(
          "a reducer requires one nonzero locator authority");
    }
    if (budget.closure_budget.maximum_seed_count >
            direct_sparse_facet_descent_closure_maximum_seed_count ||
        budget.closure_budget.maximum_node_count >
            direct_sparse_facet_descent_closure_maximum_node_count ||
        budget.closure_budget.maximum_step_call_count >
            direct_sparse_facet_descent_closure_maximum_step_call_count ||
        budget.closure_budget.maximum_memo_slot_count >
            direct_sparse_facet_descent_closure_maximum_memo_slot_count) {
      throw std::invalid_argument(
          "a reducer closure budget exceeds its confidence cap");
    }
    if (!source_manifest.certified()) {
      throw std::invalid_argument(
          "a reducer requires one certified source manifest");
    }
    if (source_manifest.logical_role_record_count >
            budget.maximum_source_role_scan_count ||
        source_manifest.batch_count >
            budget.maximum_source_batch_scan_count ||
        source_manifest.family_count >
            budget.maximum_source_family_scan_count ||
        source_manifest.arm_seed_count >
            budget.maximum_source_arm_seed_scan_count ||
        source_manifest.batch_count >
            budget.maximum_batch_record_count ||
        source_manifest.family_count >
            budget.maximum_saddle_record_count ||
        source_manifest.arm_seed_count >
            budget.maximum_arm_root_binding_count) {
      throw std::invalid_argument(
          "a reducer source exceeds its global scan budget");
    }
    if (!try_add(
            source_manifest.point_count,
            source_manifest.direct_birth_count,
            birth_count)) {
      throw std::length_error("a reducer birth count overflowed");
    }
    const std::size_t required_final_root_count =
        source_manifest.effective_maximum_order == 0U
            ? 0U
            : (source_manifest.point_count <= 1U
                   ? 1U
                   : std::min(
                         source_manifest.effective_maximum_order,
                         source_manifest.point_count - 1U));
    std::size_t birth_plus_family = 0U;
    std::size_t safe_birth_entries = 0U;
    std::size_t safe_arm_entries = 0U;
    std::size_t safe_family_entries = 0U;
    std::size_t safe_output_bound = 0U;
    if (birth_count == 0U ||
        !try_add(
            birth_count,
            source_manifest.family_count,
            birth_plus_family) ||
        !try_multiply(13U, birth_count, safe_birth_entries) ||
        !try_multiply(
            12U,
            source_manifest.arm_seed_count,
            safe_arm_entries) ||
        !try_multiply(
            3U,
            source_manifest.family_count,
            safe_family_entries) ||
        !try_add(
            safe_birth_entries,
            safe_arm_entries,
            safe_output_bound) ||
        !try_add(
            safe_output_bound,
            safe_family_entries,
            safe_output_bound) ||
        !try_add(
            safe_output_bound,
            source_manifest.batch_count,
            safe_output_bound)) {
      throw std::length_error("a reducer preflight capacity overflowed");
    }
    if (birth_count > budget.maximum_birth_record_count ||
        birth_plus_family > budget.maximum_node_count ||
        required_final_root_count >
            budget.maximum_final_root_count ||
        safe_output_bound >
            budget.maximum_logical_output_entry_count) {
      throw std::invalid_argument(
          "a reducer source exceeds its forest-output budget");
    }
    auto locator = build_exact_direct_sparse_positive_facet_locator(
        birth_count, budget.locator_budget, config.locator_config);
    if (!locator.certified_positive_locator()) {
      throw std::invalid_argument(
          "a reducer locator cannot satisfy its capacity budget");
    }
    return locator;
  }

  [[nodiscard]] ExactDirectMorseForestReducerFoldResult reject(
      ExactDirectMorseForestReducerFoldResult folded,
      ExactDirectMorseForestReducerFoldDecision decision) const noexcept {
    folded.post_fold_locator_stamp = locator_.snapshot_stamp();
    folded.decision = decision;
    return folded;
  }

  [[nodiscard]] ExactDirectMorseForestReducerFoldResult fold_checked(
      const ExactDirectMorseForestReducerBatch& batch,
      ExactDirectMorseForestReducerFoldResult folded) {
    std::optional<ExactDirectMorseForestReducerFoldResult> observed;
    bool callback_invoked = false;
    auto consume = [&](const ExactDirectMorseForestSourceBatchWindow& window) {
      if (callback_invoked ||
          !window.certified_relative_to(source_manifest_) ||
          window.source_chain_digest != expected_source_chain_digest_ ||
          recertified_source_window_count_ != batch.source_batch_index) {
        return false;
      }
      callback_invoked = true;
      observed.emplace(fold_checked_window(batch, folded, window));
      if (observed->certified_committed_batch()) {
        expected_source_chain_digest_ = window.successor_chain_digest;
        ++recertified_source_window_count_;
      }
      return true;
    };
    ExactDirectMorseForestSourceBatchVisitDecision visit{
        ExactDirectMorseForestSourceBatchVisitDecision::no_provider};
    try {
      visit = source_provider_(
          batch.source_batch_index,
          ExactDirectMorseForestSourceBatchConsumerView{consume});
    } catch (...) {
      if (observed.has_value() && observed->reducer_state_mutated) {
        // The source-provider contract forbids throwing after its consumer
        // accepted an irreversible fold.  Returning an atomic rejection here
        // would lie about the locator, so fail-stop instead.
        std::terminate();
      }
      throw;
    }
    if (visit == ExactDirectMorseForestSourceBatchVisitDecision::
                     complete_synchronous_visit &&
        callback_invoked && observed.has_value()) {
      return std::move(*observed);
    }
    if (observed.has_value() && observed->reducer_state_mutated) {
      // A provider that reports failure after accepting a successful
      // synchronous consumer has violated the authority protocol after an
      // irreversible locator commit.  This is not a retryable source error.
      std::terminate();
    }
    return reject(
        std::move(folded),
        ExactDirectMorseForestReducerFoldDecision::
            no_reducer_batch_inconsistent);
  }

  [[nodiscard]] ExactDirectMorseForestReducerFoldResult fold_checked_window(
      const ExactDirectMorseForestReducerBatch& batch,
      ExactDirectMorseForestReducerFoldResult folded,
      const ExactDirectMorseForestSourceBatchWindow& source_window) {
    if (batch.source_batch_index >= source_manifest_.batch_count ||
        source_window.batch == nullptr ||
        source_window.source_batch_index != batch.source_batch_index) {
      return reject(
          std::move(folded),
          ExactDirectMorseForestReducerFoldDecision::
              no_reducer_batch_inconsistent);
    }
    const ExactDirectMorseH0Batch& source_batch =
        *source_window.batch;
    if (batch.source_batch_index >
        std::numeric_limits<std::uint64_t>::max() / 3U - 1U) {
      return reject(
          std::move(folded),
          ExactDirectMorseForestReducerFoldDecision::
              no_reducer_batch_inconsistent);
    }
    const std::uint64_t expected_query_token =
        (static_cast<std::uint64_t>(batch.source_batch_index) + 1U) * 3U;
    if (source_batch.batch_index != batch.source_batch_index ||
        batch.order != source_batch.order ||
        batch.squared_level != source_batch.squared_level ||
        batch.traversal_order != traversal_order_ ||
        batch.requested_closure_budget != budget_.closure_budget ||
        batch.strict_pre_batch_locator_stamp !=
            folded.pre_fold_locator_stamp ||
        batch.locator_query_witness.external_authority_id !=
            config_.locator_config.external_authority_id ||
        batch.locator_query_witness.replay_token !=
            expected_query_token ||
        batch.source_family_begin_index != next_family_index_ ||
        batch.source_arm_seed_begin_index != next_arm_seed_index_ ||
        batch.source_family_end_index <
            batch.source_family_begin_index ||
        batch.source_arm_seed_end_index <
            batch.source_arm_seed_begin_index ||
        batch.source_family_end_index >
            source_manifest_.family_count ||
        batch.source_arm_seed_end_index >
            source_manifest_.arm_seed_count ||
        source_window.logical_role_begin_index !=
            source_batch.role_record_offset ||
        source_window.family_begin_index !=
            batch.source_family_begin_index ||
        source_window.families.size() !=
            batch.source_family_end_index -
                batch.source_family_begin_index ||
        source_window.arm_seed_begin_index !=
            batch.source_arm_seed_begin_index ||
        source_window.arm_seeds.size() !=
            batch.source_arm_seed_end_index -
                batch.source_arm_seed_begin_index ||
        batch.source_family_end_index -
                batch.source_family_begin_index !=
            source_batch.saddle_role_count ||
        batch.resolved_keys.size() >
            budget_.maximum_batch_distinct_arm_count ||
        batch.resolved_keys.size() >
            budget_.closure_budget.maximum_seed_count ||
        batch.shared_closure_build_count !=
            (batch.resolved_keys.empty() ? 0U : 1U)) {
      return reject(
          std::move(folded),
          ExactDirectMorseForestReducerFoldDecision::
              no_reducer_batch_inconsistent);
    }
    if (!append_count_within(
            result_.counters.aggregate_closure_node_count,
            batch.transient_closure_node_count,
            budget_.maximum_aggregate_closure_node_count) ||
        !append_count_within(
            result_.counters.aggregate_closure_step_call_count,
            batch.transient_closure_step_call_count,
            budget_.maximum_aggregate_closure_step_call_count)) {
      return reject(
          std::move(folded),
          ExactDirectMorseForestReducerFoldDecision::
              no_reducer_budget_exhausted);
    }

    const std::size_t singleton_count = source_manifest_.point_count;
    const bool canonical_singleton_bulk_shape =
        batch.source_batch_index == 0U &&
        source_batch.order == 1U &&
        source_batch.squared_level == exact::ExactLevel{} &&
        source_batch.role_record_offset == 0U &&
        source_batch.role_record_count == singleton_count &&
        source_batch.birth_role_count == singleton_count &&
        source_batch.saddle_role_count == 0U &&
        batch.source_family_begin_index == 0U &&
        batch.source_family_end_index == 0U &&
        batch.source_arm_seed_begin_index == 0U &&
        batch.source_arm_seed_end_index == 0U &&
        batch.resolved_keys.empty() && batch.arm_joins.empty() &&
        batch.transient_closure_node_count == 0U &&
        batch.transient_closure_step_call_count == 0U &&
        batch.shared_closure_build_count == 0U;
    if (canonical_singleton_bulk_shape) {
      if (singleton_count == 0U ||
          source_manifest_.logical_role_record_count < singleton_count ||
          source_manifest_.logical_event_projection_count <
              singleton_count ||
          !source_manifest_.singleton_prefix_implicit ||
          source_window.implicit_singleton_role_count != singleton_count ||
          !source_window.roles.empty() ||
          singleton_count - 1U >
              spatial::CanonicalPointCloud::max_point_id ||
          singleton_count - 1U >
              std::numeric_limits<
                  ExactDirectMorseForestNodeId>::max()) {
        return reject(
            std::move(folded),
                ExactDirectMorseForestReducerFoldDecision::
                    no_reducer_batch_inconsistent);
      }

      const PayloadSizes before = payload_sizes(result_);
      if (before.birth_records != 0U ||
          before.arm_root_bindings != 0U ||
          before.saddle_records != 0U ||
          before.atomic_groups != 0U ||
          before.child_node_ids != 0U ||
          before.batches != 0U || before.nodes != 0U ||
          result_.implicit_order_one_prefix_count != 0U ||
          result_
              .order_one_birth_and_node_prefix_implicit_and_unmaterialized ||
          result_.batches.capacity() == 0U) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }

      PayloadRollback rollback(result_, before);
      result_.batches.push_back(
          {0U,
           0U,
           1U,
           exact::ExactLevel{},
           0U,
           singleton_count,
           0U,
           0U,
           0U,
           0U,
           0U,
           0U,
           0U,
           0U,
           folded.pre_fold_locator_stamp,
           {},
           true,
           true,
           false});
      folded.canonical_singleton_bulk_count = singleton_count;
      folded.complete_batch_staged_before_mutation = true;
      folded.full_equal_level_quotient_resolved_before_mutation = true;
      prepare_pending_segment(true, singleton_count);

      const auto locator_commit =
          locator_.apply_canonical_singleton_identity_batch(
              singleton_count);
      if (!locator_commit.certified_committed_identity_batch()) {
        discard_prepared_pending_segment();
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_locator_commit_rejected);
      }
      if (locator_commit.audit.bulk_count != singleton_count) {
        // The specialized locator transaction is already irreversible.
        // A mismatched bulk length would be an internal contract breach.
        std::terminate();
      }

      // No allocating operation follows the certified locator commit.
      result_.implicit_order_one_prefix_count = singleton_count;
      result_
          .order_one_birth_and_node_prefix_implicit_and_unmaterialized =
          true;
      components_.activate_initial_canonical_singletons(singleton_count);
      auto& committed_batch = result_.batches.back();
      committed_batch.closed_post_batch_carrier_count =
          components_.carrier_count(1U);
      committed_batch.closed_post_batch_reduced_root_count =
          components_.reduced_count(1U);
      committed_batch.committed_batch_stamp = locator_.snapshot_stamp();
      committed_batch.unions_then_births_committed_atomically = true;
      ++next_batch_index_;
      rollback.release();
      publish_pending_segment_after_commit();

      folded.locator_committed_before_scientific_state = true;
      folded.scientific_state_committed = true;
      folded.reducer_state_mutated = true;
      folded.post_fold_locator_stamp = locator_.snapshot_stamp();
      folded.decision = ExactDirectMorseForestReducerFoldDecision::
          complete_reducer_batch_commit;
      return folded;
    }

    std::vector<ResolvedState> resolved_states;
    resolved_states.reserve(batch.resolved_keys.size());
    for (std::size_t index = 0U;
         index < batch.resolved_keys.size();
         ++index) {
      const auto& resolved = batch.resolved_keys[index];
      if (resolved.resolved_key_index != index ||
          !resolved.source_projection_and_terminal_certified ||
          resolved.closure_disposition !=
              ExactDirectSparseFacetDescentClosureDisposition::
                  relative_positive ||
          !valid_key(
              resolved.source_facet_key,
              source_manifest_.point_count,
              source_batch.order) ||
          !valid_key(
              resolved.resolved_terminal_facet_key,
              source_manifest_.point_count,
              source_batch.order) ||
          (index != 0U &&
           !key_less(
               batch.resolved_keys[index - 1U].source_facet_key,
               resolved.source_facet_key)) ||
          resolved.resolved_binding_witness.external_authority_id !=
              config_.locator_config.external_authority_id ||
          resolved.resolved_binding_witness.replay_token == 0U ||
          resolved.resolved_binding_witness.replay_token % 3U != 1U) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }
      const std::uint64_t birth_index_u64 =
          (resolved.resolved_binding_witness.replay_token - 1U) / 3U;
      if (birth_index_u64 >
              std::numeric_limits<std::size_t>::max()) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }
      const std::size_t birth_index =
          static_cast<std::size_t>(birth_index_u64);
      if (resolved.terminal_birth_record_index != birth_index ||
          birth_index >= global_logical_birth_record_count() ||
          !(resolved.terminal_birth_exact_squared_level <
            source_batch.squared_level) ||
          resolved.resolved_component_handle >=
              components_.handle_count() ||
          !components_.active(resolved.resolved_component_handle) ||
          components_.order(resolved.resolved_component_handle) !=
              source_batch.order ||
          components_.canonical(birth_index) !=
              resolved.resolved_component_handle) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }
      // No historical birth arena is needed here.  The replay token fixes
      // birth_index, canonical(birth_index) fixes the current component, and
      // the strict locator probe below freshly binds the exact terminal key,
      // component and same witness.  Re-reading the old record was therefore
      // redundant and would make acknowledged segments scientific inputs.
      const ExactDirectSparsePositiveFacetProbeResult terminal_probe =
          locator_.probe_positive_facet(
              resolved.resolved_terminal_facet_key,
              batch.locator_query_witness,
              terminal_probe_budget(budget_.closure_budget));
      if (!terminal_probe.certified_positive_hit() ||
          terminal_probe.component_handle !=
              resolved.resolved_component_handle ||
          terminal_probe.source_binding_witness !=
              resolved.resolved_binding_witness) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }
      resolved_states.push_back(
          {resolved.source_facet_key,
           resolved.resolved_component_handle,
           components_.reduced_root(
               resolved.resolved_component_handle),
           birth_index,
           resolved.resolved_terminal_facet_key,
           resolved.resolved_binding_witness,
           resolved.terminal_birth_exact_center,
           resolved.terminal_birth_exact_squared_level});
    }

    std::vector<TemporarySaddle> temporary_saddles;
    temporary_saddles.reserve(
        batch.source_family_end_index -
        batch.source_family_begin_index);
    std::vector<bool> resolved_seen(batch.resolved_keys.size(), false);
    std::size_t arm_cursor = batch.source_arm_seed_begin_index;
    std::size_t join_cursor = 0U;
    for (std::size_t family_index = batch.source_family_begin_index;
         family_index < batch.source_family_end_index;
         ++family_index) {
      const auto& family =
          source_window.families[
              family_index - batch.source_family_begin_index];
      std::size_t expected_projection_index = 0U;
      if (!try_add(
              source_manifest_.point_count,
              family.source_event_index,
              expected_projection_index) ||
          family.family_index != family_index ||
          family.journal_batch_index != batch.source_batch_index ||
          family.journal_event_projection_index !=
              expected_projection_index ||
          family.journal_event_projection_index <
              source_manifest_.point_count ||
          family.journal_event_projection_index >=
              source_manifest_.logical_event_projection_count ||
          family.source_event_arm_identity_digest ==
              contract::CanonicalId{} ||
          family.order != source_batch.order ||
          family.critical_squared_level != source_batch.squared_level ||
          family.arm_seed_count == 0U ||
          family.arm_seed_count > 4U ||
          family.arm_seed_offset != arm_cursor) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }
      const ExactDirectMorseEventProjection* family_projection =
          source_window.projections.find(
              family.journal_event_projection_index);
      if (family_projection == nullptr ||
          family_projection->event_projection_index !=
              family.journal_event_projection_index ||
          family_projection->source !=
              ExactDirectMorseEventSource::
                  direct_support_terminal_event ||
          family_projection->source_index != family.source_event_index ||
          family_projection->saddle_order !=
              std::optional<std::size_t>{source_batch.order} ||
          family_projection->squared_level !=
              source_batch.squared_level) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }
      TemporarySaddle saddle;
      saddle.source_family_index = family_index;
      saddle.arms.reserve(family.arm_seed_count);
      for (std::size_t local = 0U;
           local < family.arm_seed_count;
           ++local) {
        if (join_cursor >= batch.arm_joins.size() ||
            arm_cursor >= batch.source_arm_seed_end_index) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_batch_inconsistent);
        }
        const auto& seed =
            source_window.arm_seeds[
                arm_cursor - batch.source_arm_seed_begin_index];
        const auto& join = batch.arm_joins[join_cursor];
        if (seed.arm_seed_index != arm_cursor ||
            seed.family_index != family_index ||
            join.arm_seed_index != arm_cursor ||
            join.family_index != family_index ||
            !join.arm_identity_and_full_key_joined ||
            join.resolved_key_index >= resolved_states.size()) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_batch_inconsistent);
        }
        const auto reconstructed = arm_key(
            family,
            seed,
            source_window.events,
            source_manifest_.point_count,
            source_batch.order);
        const auto& resolved =
            resolved_states[join.resolved_key_index];
        if (reconstructed != resolved.key) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_batch_inconsistent);
        }
        resolved_seen[join.resolved_key_index] = true;
        saddle.arms.push_back(
            {arm_cursor,
             reconstructed,
             resolved.carrier_handle,
             resolved.prior_reduced_root_node_id,
             resolved.terminal_birth_record_index,
             resolved.terminal_birth_facet_key,
             resolved.terminal_birth_binding_witness,
             seed.removed_support_point_id,
             resolved.terminal_birth_exact_center,
             resolved.terminal_birth_exact_squared_level});
        ++arm_cursor;
        ++join_cursor;
      }
      temporary_saddles.push_back(std::move(saddle));
    }
    if (arm_cursor != batch.source_arm_seed_end_index ||
        join_cursor != batch.arm_joins.size() ||
        batch.arm_joins.size() !=
            batch.source_arm_seed_end_index -
                batch.source_arm_seed_begin_index ||
        std::find(resolved_seen.begin(), resolved_seen.end(), false) !=
            resolved_seen.end()) {
      return reject(
          std::move(folded),
          ExactDirectMorseForestReducerFoldDecision::
              no_reducer_batch_inconsistent);
    }

    std::vector<ExactFrozenRootHyperedge> hyperedges;
    std::vector<ExactFrozenRootId> carrier_references;
    hyperedges.reserve(temporary_saddles.size());
    carrier_references.reserve(batch.arm_joins.size());
    for (std::size_t saddle_index = 0U;
         saddle_index < temporary_saddles.size();
         ++saddle_index) {
      const auto& saddle = temporary_saddles[saddle_index];
      std::vector<ExactFrozenRootId> carriers;
      carriers.reserve(saddle.arms.size());
      for (const auto& arm : saddle.arms) {
        if (arm.carrier_handle >
            std::numeric_limits<ExactFrozenRootId>::max()) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_batch_inconsistent);
        }
        carriers.push_back(
            static_cast<ExactFrozenRootId>(arm.carrier_handle));
      }
      std::sort(carriers.begin(), carriers.end());
      carriers.erase(std::unique(carriers.begin(), carriers.end()),
                     carriers.end());
      if (carriers.empty()) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }
      hyperedges.push_back(
          {saddle_index, carrier_references.size(), carriers.size()});
      carrier_references.insert(
          carrier_references.end(), carriers.begin(), carriers.end());
    }

    std::optional<ExactFrozenRootQuotientResult> quotient;
    if (!hyperedges.empty()) {
      quotient.emplace(build_exact_direct_frozen_root_quotient(
          hyperedges, carrier_references, budget_.quotient_budget));
      if (!quotient->certified_frozen_root_quotient()) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_frozen_carrier_quotient_rejected);
      }
    }

    std::vector<GroupPlan> plans;
    if (quotient.has_value()) {
      plans.resize(quotient->groups.size());
      for (std::size_t group_index = 0U;
           group_index < quotient->groups.size();
           ++group_index) {
        const auto& group = quotient->groups[group_index];
        if (group.group_index != group_index || group.root_count == 0U ||
            group.root_offset > quotient->group_root_ids.size() ||
            group.root_count >
                quotient->group_root_ids.size() - group.root_offset) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_frozen_carrier_quotient_rejected);
        }
        GroupPlan& plan = plans[group_index];
        plan.atomic_group_index =
            global_atomic_group_count() + group_index;
        plan.carrier_handles.reserve(group.root_count);
        for (std::size_t local = 0U; local < group.root_count; ++local) {
          const ExactFrozenRootId frozen =
              quotient->group_root_ids[group.root_offset + local];
          if (frozen >
              std::numeric_limits<
                  ExactDirectSparseComponentHandle>::max()) {
            return reject(
                std::move(folded),
                ExactDirectMorseForestReducerFoldDecision::
                    no_reducer_batch_inconsistent);
          }
          const auto carrier =
              static_cast<ExactDirectSparseComponentHandle>(frozen);
          if (carrier >= components_.handle_count() ||
              !components_.active(carrier) ||
              components_.canonical(carrier) != carrier ||
              components_.order(carrier) != source_batch.order) {
            return reject(
                std::move(folded),
                ExactDirectMorseForestReducerFoldDecision::
                    no_reducer_batch_inconsistent);
          }
          plan.carrier_handles.push_back(carrier);
        }
        plan.canonical_root_after_union =
            *std::min_element(
                plan.carrier_handles.begin(),
                plan.carrier_handles.end());
      }
      for (const auto& binding : quotient->hyperedge_bindings) {
        if (binding.source_hyperedge_index >=
                temporary_saddles.size() ||
            binding.group_index >= plans.size()) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_frozen_carrier_quotient_rejected);
        }
        plans[binding.group_index].saddle_indices.push_back(
            binding.source_hyperedge_index);
      }

      std::size_t created_node_count = 0U;
      for (GroupPlan& plan : plans) {
        if (plan.saddle_indices.empty() ||
            plan.carrier_handles.empty()) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_batch_inconsistent);
        }
        for (const auto carrier : plan.carrier_handles) {
          const auto prior_root = components_.reduced_root(carrier);
          if (prior_root.has_value()) {
            plan.prior_reduced_root_node_ids.push_back(*prior_root);
          }
        }
        std::sort(
            plan.prior_reduced_root_node_ids.begin(),
            plan.prior_reduced_root_node_ids.end());
        if (std::adjacent_find(
                plan.prior_reduced_root_node_ids.begin(),
                plan.prior_reduced_root_node_ids.end()) !=
            plan.prior_reduced_root_node_ids.end()) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_batch_inconsistent);
        }
        if (plan.prior_reduced_root_node_ids.size() == 1U) {
          plan.kind =
              ExactDirectMorseForestAtomicGroupKind::continuation;
          plan.resulting_root_node_id =
              plan.prior_reduced_root_node_ids.front();
        } else {
          if (plan.prior_reduced_root_node_ids.empty()) {
            if (source_batch.order == 1U) {
              return reject(
                  std::move(folded),
                  ExactDirectMorseForestReducerFoldDecision::
                      no_reducer_batch_inconsistent);
            }
            plan.kind =
                ExactDirectMorseForestAtomicGroupKind::reduced_birth;
          } else {
            plan.kind =
                ExactDirectMorseForestAtomicGroupKind::multifusion;
          }
          const std::size_t node_index =
              global_logical_node_count() + created_node_count;
          if (node_index >= budget_.maximum_node_count ||
              node_index >
                  std::numeric_limits<
                      ExactDirectMorseForestNodeId>::max()) {
            return reject(
                std::move(folded),
                ExactDirectMorseForestReducerFoldDecision::
                    no_reducer_budget_exhausted);
          }
          plan.created_node_id =
              static_cast<ExactDirectMorseForestNodeId>(node_index);
          plan.resulting_root_node_id = *plan.created_node_id;
          ++created_node_count;
        }
      }
    }

    if (!append_count_within(
            global_atomic_group_count(),
            plans.size(),
            budget_.maximum_atomic_group_count) ||
        !append_count_within(
            global_saddle_record_count(),
            temporary_saddles.size(),
            budget_.maximum_saddle_record_count) ||
        !append_count_within(
            global_arm_root_binding_count(),
            batch.arm_joins.size(),
            budget_.maximum_arm_root_binding_count)) {
      return reject(
          std::move(folded),
          ExactDirectMorseForestReducerFoldDecision::
              no_reducer_budget_exhausted);
    }

    std::vector<ExactDirectMorseForestArmRootBinding>
        pending_arm_bindings;
    std::vector<ExactDirectMorseForestSaddleRecord> pending_saddles;
    std::vector<ExactDirectMorseForestAtomicGroup> pending_groups;
    std::vector<ExactDirectMorseForestNodeId> pending_children;
    std::vector<ExactDirectMorseForestNode> pending_group_nodes;
    std::vector<ExactDirectSparseComponentUnion> locator_unions;
    pending_arm_bindings.reserve(batch.arm_joins.size());
    pending_saddles.reserve(temporary_saddles.size());
    pending_groups.reserve(plans.size());
    std::size_t reduced_birth_groups = 0U;
    std::size_t continuation_groups = 0U;
    std::size_t multifusion_groups = 0U;
    std::size_t maximum_carrier_arity = 0U;
    std::size_t maximum_merge_arity = 0U;

    for (GroupPlan& plan : plans) {
      const std::size_t group_saddle_offset =
          global_saddle_record_count() + pending_saddles.size();
      for (const std::size_t saddle_index : plan.saddle_indices) {
        const auto& saddle = temporary_saddles[saddle_index];
        const auto& family =
            source_window.families[
                saddle.source_family_index -
                    batch.source_family_begin_index];
        const std::size_t arm_offset =
            global_arm_root_binding_count() +
            pending_arm_bindings.size();
        std::vector<ExactDirectSparseComponentHandle> saddle_carriers;
        std::vector<ExactDirectMorseForestNodeId> saddle_roots;
        saddle_carriers.reserve(saddle.arms.size());
        saddle_roots.reserve(saddle.arms.size());
        for (const auto& arm : saddle.arms) {
          pending_arm_bindings.push_back(
              {global_arm_root_binding_count() +
                   pending_arm_bindings.size(),
               arm.source_seed_index,
               saddle.source_family_index,
               arm.key,
               arm.carrier_handle,
               arm.prior_reduced_root_node_id,
               arm.terminal_birth_record_index,
               arm.terminal_birth_facet_key,
               arm.terminal_birth_binding_witness,
               arm.removed_support_point_id,
               arm.terminal_birth_exact_center,
               arm.terminal_birth_exact_squared_level});
          saddle_carriers.push_back(arm.carrier_handle);
          if (arm.prior_reduced_root_node_id.has_value()) {
            saddle_roots.push_back(*arm.prior_reduced_root_node_id);
          }
        }
        std::sort(saddle_carriers.begin(), saddle_carriers.end());
        saddle_carriers.erase(
            std::unique(saddle_carriers.begin(), saddle_carriers.end()),
            saddle_carriers.end());
        std::sort(saddle_roots.begin(), saddle_roots.end());
        saddle_roots.erase(
            std::unique(saddle_roots.begin(), saddle_roots.end()),
            saddle_roots.end());
        if (saddle_carriers.empty() ||
            saddle_roots.size() > saddle_carriers.size()) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_batch_inconsistent);
        }
        pending_saddles.push_back(
            {global_saddle_record_count() + pending_saddles.size(),
             saddle.source_family_index,
             family.source_event_index,
             batch.source_batch_index,
             arm_offset,
             saddle.arms.size(),
             saddle_carriers.size(),
             saddle_carriers.size() - saddle_roots.size(),
             saddle_roots.size(),
             plan.atomic_group_index,
             family.journal_event_projection_index,
             family.source_event_arm_identity_digest});
      }

      ExactDirectMorseForestAtomicGroup group_record;
      group_record.atomic_group_index = plan.atomic_group_index;
      group_record.batch_index = global_batch_record_count();
      group_record.saddle_record_offset = group_saddle_offset;
      group_record.saddle_record_count =
          global_saddle_record_count() + pending_saddles.size() -
          group_saddle_offset;
      group_record.frozen_carrier_count = plan.carrier_handles.size();
      group_record.latent_carrier_count =
          plan.carrier_handles.size() -
          plan.prior_reduced_root_node_ids.size();
      group_record.prior_reduced_root_count =
          plan.prior_reduced_root_node_ids.size();
      group_record.child_offset =
          global_child_reference_count() + pending_children.size();
      group_record.created_node_id = plan.created_node_id;
      group_record.resulting_root_node_id =
          plan.resulting_root_node_id;
      group_record.kind = plan.kind;
      maximum_carrier_arity =
          std::max(maximum_carrier_arity, plan.carrier_handles.size());
      maximum_merge_arity = std::max(
          maximum_merge_arity,
          plan.prior_reduced_root_node_ids.size());
      switch (plan.kind) {
        case ExactDirectMorseForestAtomicGroupKind::reduced_birth:
          ++reduced_birth_groups;
          break;
        case ExactDirectMorseForestAtomicGroupKind::continuation:
          ++continuation_groups;
          break;
        case ExactDirectMorseForestAtomicGroupKind::multifusion:
          ++multifusion_groups;
          group_record.child_count =
              plan.prior_reduced_root_node_ids.size();
          if (!append_count_within(
                  global_child_reference_count() +
                      pending_children.size(),
                  plan.prior_reduced_root_node_ids.size(),
                  budget_.maximum_child_reference_count)) {
            return reject(
                std::move(folded),
                ExactDirectMorseForestReducerFoldDecision::
                    no_reducer_budget_exhausted);
          }
          pending_children.insert(
              pending_children.end(),
              plan.prior_reduced_root_node_ids.begin(),
              plan.prior_reduced_root_node_ids.end());
          break;
      }
      if (plan.kind !=
          ExactDirectMorseForestAtomicGroupKind::continuation) {
        pending_group_nodes.push_back(
            {*plan.created_node_id,
             source_batch.order,
             source_batch.squared_level,
             plan.kind ==
                     ExactDirectMorseForestAtomicGroupKind::reduced_birth
                 ? ExactDirectMorseForestNodeKind::reduced_birth
                 : ExactDirectMorseForestNodeKind::multifusion,
             group_record.child_offset,
             group_record.child_count,
             std::nullopt,
             plan.atomic_group_index});
      }
      for (std::size_t local = 1U;
           local < plan.carrier_handles.size();
           ++local) {
        const std::size_t local_union_index = locator_unions.size();
        const std::size_t global_union_index =
            result_.counters.locator_union_count + local_union_index;
        const auto token = replay_token(global_union_index, 2U);
        if (!token.has_value()) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_batch_inconsistent);
        }
        locator_unions.push_back(
            {local_union_index,
             plan.carrier_handles.front(),
             plan.carrier_handles[local],
             {config_.locator_config.external_authority_id, *token}});
      }
      pending_groups.push_back(group_record);
    }

    std::vector<ExactDirectMorseForestBirthRecord> pending_births;
    std::vector<ExactDirectMorseForestNode> pending_birth_nodes;
    std::vector<ExactDirectSparseFacetBinding> locator_bindings;
    const std::size_t role_begin = source_batch.role_record_offset;
    if (source_window.implicit_singleton_role_count != 0U ||
        source_window.roles.size() != source_batch.role_record_count ||
        role_begin > source_manifest_.logical_role_record_count ||
        source_batch.role_record_count >
            source_manifest_.logical_role_record_count - role_begin) {
      return reject(
          std::move(folded),
          ExactDirectMorseForestReducerFoldDecision::
              no_reducer_batch_inconsistent);
    }
    pending_births.reserve(source_batch.birth_role_count);
    pending_birth_nodes.reserve(source_batch.birth_role_count);
    locator_bindings.reserve(source_batch.birth_role_count);
    for (std::size_t local = 0U;
         local < source_batch.role_record_count;
         ++local) {
      const std::size_t logical_role_index = role_begin + local;
      if (logical_role_index < singleton_count) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }
      const auto& role = source_window.roles[local];
      if (role.role_record_index != logical_role_index) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }
      if (role.batch_index != batch.source_batch_index ||
          role.event_projection_index >=
              source_manifest_.logical_event_projection_count ||
          role.event_projection_index < singleton_count) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }
      if (role.role != ExactDirectMorseH0Role::birth) {
        continue;
      }
      const ExactDirectMorseEventProjection* projection_pointer =
          source_window.projections.find(role.event_projection_index);
      if (projection_pointer == nullptr ||
          projection_pointer->event_projection_index !=
              role.event_projection_index) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }
      const auto key = birth_key(
          *projection_pointer,
          source_window.events,
          source_manifest_.direct_event_count,
          source_manifest_.point_count,
          source_batch.order,
          source_batch.squared_level);
      const std::size_t birth_index =
          global_logical_birth_record_count() + pending_births.size();
      const std::size_t node_index =
          global_logical_node_count() + pending_group_nodes.size() +
          pending_birth_nodes.size();
      if (birth_index >= birth_count_) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_budget_exhausted);
      }
      const auto token = replay_token(birth_index, 1U);
      if (!token.has_value()) {
        return reject(
            std::move(folded),
            ExactDirectMorseForestReducerFoldDecision::
                no_reducer_batch_inconsistent);
      }
      const ExactDirectSparseFacetWitness witness{
          config_.locator_config.external_authority_id, *token};
      std::optional<ExactDirectMorseForestNodeId> birth_node;
      if (source_batch.order == 1U) {
        if (node_index >= budget_.maximum_node_count ||
            node_index >
                std::numeric_limits<
                    ExactDirectMorseForestNodeId>::max()) {
          return reject(
              std::move(folded),
              ExactDirectMorseForestReducerFoldDecision::
                  no_reducer_budget_exhausted);
        }
        birth_node =
            static_cast<ExactDirectMorseForestNodeId>(node_index);
      }
      pending_births.push_back(
          {birth_index,
           role.event_projection_index,
           batch.source_batch_index,
           source_batch.order,
           key,
           birth_index,
           birth_node,
           witness});
      if (birth_node.has_value()) {
        pending_birth_nodes.push_back(
            {*birth_node,
             source_batch.order,
             source_batch.squared_level,
             ExactDirectMorseForestNodeKind::order_one_birth,
             global_child_reference_count() + pending_children.size(),
             0U,
             birth_index,
             std::nullopt});
      }
      locator_bindings.push_back(
          {locator_bindings.size(), key, birth_index, witness});
    }
    if (pending_births.size() != source_batch.birth_role_count ||
        !append_count_within(
            global_logical_birth_record_count(),
            pending_births.size(),
            budget_.maximum_birth_record_count) ||
        !append_count_within(
            global_logical_node_count(),
            pending_group_nodes.size() + pending_birth_nodes.size(),
            budget_.maximum_node_count)) {
      return reject(
          std::move(folded),
          ExactDirectMorseForestReducerFoldDecision::
              no_reducer_batch_inconsistent);
    }
    folded.staged_birth_record_count = pending_births.size();
    folded.staged_birth_node_count = pending_birth_nodes.size();
    folded.staged_locator_binding_count = locator_bindings.size();

    std::size_t pending_node_count = 0U;
    if (!try_add(
            pending_group_nodes.size(),
            pending_birth_nodes.size(),
            pending_node_count) ||
        (segmented_output_enabled_ &&
         (pending_births.size() >
              segment_limits_.maximum_birth_record_count ||
          pending_arm_bindings.size() >
              segment_limits_.maximum_arm_root_binding_count ||
          pending_saddles.size() >
              segment_limits_.maximum_saddle_record_count ||
          pending_groups.size() >
              segment_limits_.maximum_atomic_group_count ||
          pending_children.size() >
              segment_limits_.maximum_child_reference_count ||
          pending_node_count > segment_limits_.maximum_node_count))) {
      return reject(
          std::move(folded),
          ExactDirectMorseForestReducerFoldDecision::
              no_reducer_budget_exhausted);
    }

    const std::size_t strict_carrier_count =
        components_.carrier_count(source_batch.order);
    const std::size_t strict_reduced_count =
        components_.reduced_count(source_batch.order);
    const PayloadSizes before = payload_sizes(result_);
    PayloadRollback rollback(result_, before);
    append_moved(result_.arm_root_bindings, pending_arm_bindings);
    append_moved(result_.saddle_records, pending_saddles);
    append_moved(result_.atomic_groups, pending_groups);
    append_moved(result_.child_node_ids, pending_children);
    append_moved(result_.nodes, pending_group_nodes);
    append_moved(result_.birth_records, pending_births);
    append_moved(result_.nodes, pending_birth_nodes);
    result_.batches.push_back(
        {output_cursor_.batch_record_count + before.batches,
         batch.source_batch_index,
         source_batch.order,
         source_batch.squared_level,
         result_.implicit_order_one_prefix_count +
             output_cursor_.birth_record_count + before.birth_records,
         result_.birth_records.size() - before.birth_records,
         output_cursor_.saddle_record_count + before.saddle_records,
         result_.saddle_records.size() - before.saddle_records,
         output_cursor_.atomic_group_count + before.atomic_groups,
         result_.atomic_groups.size() - before.atomic_groups,
         strict_carrier_count,
         strict_reduced_count,
         0U,
         0U,
         folded.pre_fold_locator_stamp,
         {},
         true,
         true,
         false});
    folded.complete_batch_staged_before_mutation = true;
    folded.full_equal_level_quotient_resolved_before_mutation = true;
    prepare_pending_segment(
        false, result_.implicit_order_one_prefix_count);

    const auto locator_commit = locator_.apply_batch(
        std::span<const ExactDirectSparseFacetQuery>{},
        locator_unions,
        locator_bindings);
    if (!locator_commit.certified_committed_batch()) {
      discard_prepared_pending_segment();
      return reject(
          std::move(folded),
          ExactDirectMorseForestReducerFoldDecision::
              no_reducer_locator_commit_rejected);
    }
    if (locator_commit.counters.union_request_count !=
            locator_unions.size() ||
        locator_commit.counters.binding_request_count !=
            locator_bindings.size() ||
        locator_commit.counters.inserted_binding_count !=
            locator_bindings.size() ||
        locator_commit.counters.compatible_duplicate_binding_count !=
            0U) {
      // A certified locator commit is irreversible.  These equalities are
      // consequences of the already validated disjoint batch, so violating
      // one is an internal contract breach, never a recoverable rejection.
      std::terminate();
    }

    // No allocating operation follows this point.  The carrier state reuses
    // the locator's deterministic canonical-minimum parent authority, so no
    // second dense singleton DSU or physical root identity can leak into the
    // hierarchy protocol.
    for (const GroupPlan& plan : plans) {
      components_.commit_group(
          plan.canonical_root_after_union,
          plan.carrier_handles.size(),
          source_batch.order,
          plan.prior_reduced_root_node_ids.size(),
          plan.resulting_root_node_id);
    }
    for (std::size_t index = before.birth_records;
         index < result_.birth_records.size();
         ++index) {
      const auto& birth = result_.birth_records[index];
      components_.activate(
          birth.component_handle,
          birth.order,
          birth.order_one_birth_node_id);
    }
    auto& committed_batch = result_.batches.back();
    committed_batch.closed_post_batch_carrier_count =
        components_.carrier_count(source_batch.order);
    committed_batch.closed_post_batch_reduced_root_count =
        components_.reduced_count(source_batch.order);
    committed_batch.committed_batch_stamp = locator_.snapshot_stamp();
    committed_batch.unions_then_births_committed_atomically = true;

    result_.counters.reduced_birth_group_count += reduced_birth_groups;
    result_.counters.continuation_group_count += continuation_groups;
    result_.counters.multifusion_group_count += multifusion_groups;
    result_.counters.locator_union_count += locator_unions.size();
    result_.counters.closure_call_count +=
        batch.shared_closure_build_count;
    result_.counters.quotient_call_count +=
        hyperedges.empty() ? 0U : 1U;
    result_.counters.distinct_strict_arm_count +=
        batch.resolved_keys.size();
    result_.counters.duplicate_strict_arm_reference_count +=
        batch.arm_joins.size() - batch.resolved_keys.size();
    result_.counters.aggregate_closure_node_count +=
        batch.transient_closure_node_count;
    result_.counters.aggregate_closure_step_call_count +=
        batch.transient_closure_step_call_count;
    result_.counters.maximum_batch_arm_count = std::max(
        result_.counters.maximum_batch_arm_count,
        batch.arm_joins.size());
    result_.counters.maximum_batch_carrier_arity = std::max(
        result_.counters.maximum_batch_carrier_arity,
        maximum_carrier_arity);
    result_.counters.maximum_batch_merge_arity = std::max(
        result_.counters.maximum_batch_merge_arity,
        maximum_merge_arity);
    next_family_index_ = batch.source_family_end_index;
    next_arm_seed_index_ = batch.source_arm_seed_end_index;
    ++next_batch_index_;
    rollback.release();
    publish_pending_segment_after_commit();

    folded.locator_committed_before_scientific_state = true;
    folded.scientific_state_committed = true;
    folded.reducer_state_mutated = true;
    folded.post_fold_locator_stamp = locator_.snapshot_stamp();
    folded.decision = ExactDirectMorseForestReducerFoldDecision::
        complete_reducer_batch_commit;
    return folded;
  }

  std::unique_ptr<ExactDirectMorseForestResidentSourceAdapter>
      owned_source_adapter_;
  ExactDirectMorseForestSourceManifest source_manifest_{};
  ExactDirectMorseForestSourceBatchProviderView source_provider_;
  ExactDirectMorseForestBudget budget_{};
  ExactDirectMorseForestConfig config_{};
  spatial::LbvhTraversalOrder traversal_order_{
      spatial::LbvhTraversalOrder::near_first};
  bool segmented_output_enabled_{false};
  ExactDirectMorseForestSegmentLimits segment_limits_{};
  ExactDirectMorseForestSegmentCursor output_cursor_{};
  contract::CanonicalId expected_source_chain_digest_{};
  std::size_t recertified_source_window_count_{};
  std::optional<ExactDirectMorseForestBatchSegment> pending_segment_;
  std::size_t birth_count_{};
  ExactDirectSparsePositiveFacetLocator locator_;
  CarrierDsu components_;
  ExactDirectMorseForestJournalResult result_;
  std::size_t next_batch_index_{};
  std::size_t next_family_index_{};
  std::size_t next_arm_seed_index_{};
  bool finished_{false};
};

ExactDirectMorseForestReducer::ExactDirectMorseForestReducer(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectMorseForestBudget& budget,
    const ExactDirectMorseForestConfig& config,
    spatial::LbvhTraversalOrder traversal_order)
    : impl_(std::make_unique<Impl>(
          cloud,
          source_facade,
          source_journal,
          trusted_seed_budget,
          source_seed_journal,
          budget,
          config,
          traversal_order)) {}

ExactDirectMorseForestReducer::ExactDirectMorseForestReducer(
    const ExactDirectMorseForestSourceManifest& source_manifest,
    ExactDirectMorseForestSourceBatchProviderView source_provider,
    const ExactDirectMorseForestBudget& budget,
    const ExactDirectMorseForestConfig& config,
    spatial::LbvhTraversalOrder traversal_order)
    : impl_(std::make_unique<Impl>(
          source_manifest,
          source_provider,
          budget,
          config,
          traversal_order)) {}

ExactDirectMorseForestReducer::ExactDirectMorseForestReducer(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectMorseForestBudget& budget,
    const ExactDirectMorseForestConfig& config,
    const ExactDirectMorseForestSegmentLimits& segment_limits,
    const contract::CanonicalId& initial_chain_digest,
    spatial::LbvhTraversalOrder traversal_order)
    : impl_(std::make_unique<Impl>(
          cloud,
          source_facade,
          source_journal,
          trusted_seed_budget,
          source_seed_journal,
          budget,
          config,
          traversal_order,
          segment_limits,
          initial_chain_digest)) {}

ExactDirectMorseForestReducer::ExactDirectMorseForestReducer(
    const ExactDirectMorseForestSourceManifest& source_manifest,
    ExactDirectMorseForestSourceBatchProviderView source_provider,
    const ExactDirectMorseForestBudget& budget,
    const ExactDirectMorseForestConfig& config,
    const ExactDirectMorseForestSegmentLimits& segment_limits,
    const contract::CanonicalId& initial_chain_digest,
    spatial::LbvhTraversalOrder traversal_order)
    : impl_(std::make_unique<Impl>(
          source_manifest,
          source_provider,
          budget,
          config,
          traversal_order,
          segment_limits,
          initial_chain_digest)) {}

ExactDirectMorseForestReducer::~ExactDirectMorseForestReducer() = default;

ExactDirectMorseForestReducer::ExactDirectMorseForestReducer(
    ExactDirectMorseForestReducer&&) noexcept = default;

ExactDirectMorseForestReducer&
ExactDirectMorseForestReducer::operator=(
    ExactDirectMorseForestReducer&&) noexcept = default;

ExactDirectMorseForestReducerFoldResult
ExactDirectMorseForestReducer::fold(
    const ExactDirectMorseForestReducerBatch& batch) {
  if (!impl_) {
    throw std::logic_error("a moved forest reducer cannot fold");
  }
  return impl_->fold(batch);
}

ExactDirectMorseForestLiveCommitResult
ExactDirectMorseForestReducer::fold_prepared_ticket(
    ExactDirectSparseFacetDescentAnchoredBatchExecutor& executor,
    ExactDirectSparseFacetDescentAnchoredBatchExecutor::
        PreparedTopKProposalBatch&& prepared) {
  if (!impl_) {
    throw std::logic_error(
        "a moved forest reducer cannot commit a live batch");
  }
  static_assert(
      std::is_nothrow_move_constructible_v<
          ExactDirectSparseFacetDescentBatchExecutionResult>);
  static_assert(
      std::is_nothrow_move_constructible_v<
          ExactDirectSparseFacetDescentClosureTopKProposalConsumptionAudit>);
  static_assert(
      std::is_nothrow_move_constructible_v<
          ExactDirectMorseForestLiveCommitResult>);
  static_assert(
      std::is_nothrow_move_assignable_v<
          ExactDirectMorseForestReducerFoldResult>);

  ExactDirectMorseForestLiveCommitResult result;
  result.source_batch_index = prepared.source_batch_index_;
  result.successor_batch_index = prepared.successor_batch_index_;
  result.pre_commit_executor_batch_index =
      executor.next_source_batch_index_;
  result.post_commit_executor_batch_index =
      result.pre_commit_executor_batch_index;
  result.pre_commit_locator_stamp = impl_->locator().snapshot_stamp();
  result.post_commit_locator_stamp = result.pre_commit_locator_stamp;

  const std::size_t pre_epoch = executor.source_epoch_;
  const std::size_t pre_batch_index =
      executor.next_source_batch_index_;
  const std::size_t pre_chunk_index =
      executor.next_source_chunk_index_;
  const std::size_t pre_lane_index =
      executor.next_source_lane_index_;
  const std::size_t pre_family_index =
      executor.next_source_family_index_;
  const std::size_t pre_arm_seed_index =
      executor.next_source_arm_seed_index_;

  const auto executor_cursor_unchanged = [&]() noexcept {
    return executor.source_epoch_ == pre_epoch &&
           executor.next_source_batch_index_ == pre_batch_index &&
           executor.next_source_chunk_index_ == pre_chunk_index &&
           executor.next_source_lane_index_ == pre_lane_index &&
           executor.next_source_family_index_ == pre_family_index &&
           executor.next_source_arm_seed_index_ == pre_arm_seed_index;
  };
  const auto consume_ticket = [&]() noexcept {
    prepared.session_seal_.reset();
    prepared.exact_scientific_delta_provenance_minted_ = false;
    prepared.valid_ = false;
    prepared.consumed_ = true;
    result.ticket_consumed = true;
  };
  const auto reject =
      [&](ExactDirectMorseForestLiveCommitDecision decision,
          bool account_rejection) {
        if (account_rejection) {
          if (executor.audit_.sealed_ticket_rejected_commit_count !=
              std::numeric_limits<std::size_t>::max()) {
            ++executor.audit_.sealed_ticket_rejected_commit_count;
          } else {
            decision = ExactDirectMorseForestLiveCommitDecision::
                no_live_commit_executor_audit_capacity_exhausted;
          }
        }
        consume_ticket();
        result.post_commit_executor_batch_index =
            executor.next_source_batch_index_;
        result.post_commit_locator_stamp =
            impl_->locator().snapshot_stamp();
        result.executor_cursor_unchanged_on_rejection =
            executor_cursor_unchanged();
        result.locator_unchanged_on_rejection =
            result.post_commit_locator_stamp ==
            result.pre_commit_locator_stamp;
        result.decision = decision;
        return result;
      };

  if (executor.audit_.sealed_ticket_commit_attempt_count ==
      std::numeric_limits<std::size_t>::max()) {
    return reject(
        ExactDirectMorseForestLiveCommitDecision::
            no_live_commit_executor_audit_capacity_exhausted,
        false);
  }
  ++executor.audit_.sealed_ticket_commit_attempt_count;

  result.ticket_was_valid_and_unconsumed =
      prepared.valid_ && !prepared.consumed_;
  if (!result.ticket_was_valid_and_unconsumed) {
    return reject(
        ExactDirectMorseForestLiveCommitDecision::
            no_live_commit_invalid_moved_or_consumed_ticket,
        true);
  }

  result.shared_session_seal_matches =
      prepared.session_seal_ &&
      prepared.session_seal_ == executor.session_seal_;
  if (!result.shared_session_seal_matches) {
    return reject(
        ExactDirectMorseForestLiveCommitDecision::
            no_live_commit_foreign_session,
        true);
  }

  result.source_epoch_and_full_cursor_match =
      prepared.source_epoch_ == executor.source_epoch_ &&
      prepared.source_batch_index_ ==
          executor.next_source_batch_index_ &&
      prepared.source_chunk_index_ ==
          executor.next_source_chunk_index_ &&
      prepared.source_lane_index_ ==
          executor.next_source_lane_index_ &&
      prepared.source_family_index_ ==
          executor.next_source_family_index_ &&
      prepared.source_arm_seed_index_ ==
          executor.next_source_arm_seed_index_ &&
      !executor.complete();
  if (!result.source_epoch_and_full_cursor_match) {
    return reject(
        ExactDirectMorseForestLiveCommitDecision::
            no_live_commit_stale_epoch_or_cursor,
        true);
  }

  result.exact_scientific_delta_provenance_minted =
      prepared.exact_scientific_delta_provenance_minted_ &&
      prepared.preparation_.scientific_delta.has_value();
  result.reducer_and_executor_share_locator_instance =
      executor.locator_ == &impl_->locator();
  const bool locator_snapshot_matches =
      result.reducer_and_executor_share_locator_instance &&
      prepared.locator_snapshot_stamp_ ==
          result.pre_commit_locator_stamp &&
      executor.locator_->snapshot_stamp() ==
          result.pre_commit_locator_stamp;
  if (!result.exact_scientific_delta_provenance_minted ||
      !locator_snapshot_matches) {
    return reject(
        result.exact_scientific_delta_provenance_minted
            ? ExactDirectMorseForestLiveCommitDecision::
                  no_live_commit_distinct_locator_or_snapshot
            : ExactDirectMorseForestLiveCommitDecision::
                  no_live_commit_invalid_moved_or_consumed_ticket,
        true);
  }

  result.reducer_and_executor_batch_cursors_match =
      impl_->next_batch_index() ==
          executor.next_source_batch_index_;
  if (!result.reducer_and_executor_batch_cursors_match) {
    return reject(
        ExactDirectMorseForestLiveCommitDecision::
            no_live_commit_reducer_cursor_mismatch,
        true);
  }

  if (executor.audit_.sealed_ticket_rejected_commit_count ==
          std::numeric_limits<std::size_t>::max() ||
      executor.audit_.accepted_batch_count ==
          std::numeric_limits<std::size_t>::max() ||
      executor.audit_.sealed_ticket_accepted_commit_count ==
          std::numeric_limits<std::size_t>::max() ||
      executor.audit_.sealed_ticket_exact_replay_avoided_count ==
          std::numeric_limits<std::size_t>::max() ||
      executor.source_epoch_ ==
          std::numeric_limits<std::size_t>::max()) {
    return reject(
        ExactDirectMorseForestLiveCommitDecision::
            no_live_commit_executor_audit_capacity_exhausted,
        false);
  }
  result.executor_commit_capacity_preflighted = true;

  ExactDirectMorseForestReducerBatch projected;
  try {
    projected = project_exact_direct_morse_forest_reducer_batch(
        *prepared.preparation_.scientific_delta);
  } catch (const std::invalid_argument&) {
    return reject(
        ExactDirectMorseForestLiveCommitDecision::
            no_live_commit_invalid_moved_or_consumed_ticket,
        true);
  }
  result.all_fallible_scientific_work_precedes_irreversible_mutation = true;
  result.reducer_fold_attempted = true;
  result.reducer_fold = impl_->fold(projected);
  if (!result.reducer_fold.certified_committed_batch()) {
    if (!result.reducer_fold.certified_atomic_rejection() ||
        !executor_cursor_unchanged() ||
        impl_->locator().snapshot_stamp() !=
            result.pre_commit_locator_stamp) {
      std::terminate();
    }
    return reject(
        ExactDirectMorseForestLiveCommitDecision::
            no_live_commit_reducer_fold_rejected,
        true);
  }

  // From here to the synchronized successor cursor there is no allocation,
  // hash, probe, quotient, replay or budget decision.  Any contradiction is
  // therefore an internal fail-stop contract breach, not a recoverable
  // rejection after an irreversible locator commit.
  if (result.reducer_fold.source_batch_index !=
          result.source_batch_index ||
      result.reducer_fold.pre_fold_locator_stamp !=
          result.pre_commit_locator_stamp ||
      result.reducer_fold.post_fold_locator_stamp !=
          impl_->locator().snapshot_stamp() ||
      !executor_cursor_unchanged()) {
    std::terminate();
  }
  result.reducer_committed_before_executor_cursor = true;
  result.no_fallible_operation_after_reducer_commit = true;

  result.scientific_delta.emplace(
      std::move(*prepared.preparation_.scientific_delta));
  prepared.preparation_.scientific_delta.reset();
  if (prepared.preparation_.proposal_consumption_audit.has_value()) {
    result.operational_audit.emplace(std::move(
        *prepared.preparation_.proposal_consumption_audit));
    prepared.preparation_.proposal_consumption_audit.reset();
  }

  executor.next_source_batch_index_ =
      prepared.successor_batch_index_;
  executor.next_source_chunk_index_ =
      prepared.successor_chunk_index_;
  executor.next_source_lane_index_ =
      prepared.successor_lane_index_;
  executor.next_source_family_index_ =
      prepared.successor_family_index_;
  executor.next_source_arm_seed_index_ =
      prepared.successor_arm_seed_index_;
  ++executor.source_epoch_;
  ++executor.audit_.accepted_batch_count;
  ++executor.audit_.sealed_ticket_accepted_commit_count;
  ++executor.audit_.sealed_ticket_exact_replay_avoided_count;
  executor.audit_.sealed_ticket_or_delta_retained_by_session = false;

  result.executor_cursor_advanced = true;
  result.scientific_delta_moved_to_result = true;
  consume_ticket();
  result.post_commit_executor_batch_index =
      executor.next_source_batch_index_;
  result.post_commit_locator_stamp =
      impl_->locator().snapshot_stamp();
  result.decision = ExactDirectMorseForestLiveCommitDecision::
      complete_live_reducer_then_cursor_commit;
  if (!result.certified_live_commit()) {
    std::terminate();
  }
  return result;
}

const ExactDirectSparsePositiveFacetLocator&
ExactDirectMorseForestReducer::strict_locator() const noexcept {
  return impl_->locator();
}

std::size_t ExactDirectMorseForestReducer::next_source_batch_index()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->next_batch_index();
}

bool ExactDirectMorseForestReducer::complete() const noexcept {
  return impl_ != nullptr && impl_->complete();
}

bool ExactDirectMorseForestReducer::segmented_output_enabled()
    const noexcept {
  return impl_ != nullptr && impl_->segmented_output_enabled();
}

bool ExactDirectMorseForestReducer::has_pending_output_segment()
    const noexcept {
  return impl_ != nullptr && impl_->has_pending_output_segment();
}

const ExactDirectMorseForestBatchSegment*
ExactDirectMorseForestReducer::pending_output_segment() const noexcept {
  return impl_ == nullptr ? nullptr : impl_->pending_output_segment();
}

const ExactDirectMorseForestSegmentCursor&
ExactDirectMorseForestReducer::output_cursor() const noexcept {
  static const ExactDirectMorseForestSegmentCursor empty{};
  return impl_ == nullptr ? empty : impl_->output_cursor();
}

ExactDirectMorseForestSegmentDrainResult
ExactDirectMorseForestReducer::drain_pending_output_segment(
    ExactDirectMorseForestSegmentSinkView sink) noexcept {
  if (impl_ == nullptr) {
    return {};
  }
  return impl_->drain_pending_output_segment(sink);
}

ExactDirectMorseForestJournalResult ExactDirectMorseForestReducer::finish() {
  if (!impl_) {
    throw std::logic_error("a moved forest reducer cannot finish");
  }
  return impl_->finish();
}

ExactDirectMorseForestFinalSeal
ExactDirectMorseForestReducer::finish_segmented() {
  if (!impl_) {
    throw std::logic_error(
        "a moved forest reducer cannot finish a segmented stream");
  }
  return impl_->finish_segmented();
}

}  // namespace morsehgp3d::hierarchy
