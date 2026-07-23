#include "morsehgp3d/hierarchy/direct_sparse_positive_facet_locator.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

[[nodiscard]] std::optional<std::size_t> checked_add(
    std::size_t left,
    std::size_t right) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return std::nullopt;
  }
  return left + right;
}

[[nodiscard]] std::optional<std::size_t> checked_multiply(
    std::size_t left,
    std::size_t right) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return std::nullopt;
  }
  return left * right;
}

[[nodiscard]] std::optional<std::size_t> probing_slot_capacity(
    std::size_t entry_capacity) {
  const auto doubled = checked_multiply(2U, entry_capacity);
  return doubled.has_value() ? checked_add(*doubled, 1U) : std::nullopt;
}

[[nodiscard]] std::optional<std::size_t>
structural_verification_scratch_byte_count(
    std::size_t binding_entry_count,
    std::size_t key_point_entry_count,
    std::size_t table_slot_entry_count,
    std::size_t component_parent_entry_count) {
  std::size_t total = 0U;
  const auto add_payload = [&total](
                               std::size_t entry_count,
                               std::size_t entry_size) {
    const auto bytes = checked_multiply(entry_count, entry_size);
    if (!bytes.has_value()) {
      return false;
    }
    const auto updated = checked_add(total, *bytes);
    if (!updated.has_value()) {
      return false;
    }
    total = *updated;
    return true;
  };
  constexpr std::size_t binding_entry_size =
      sizeof(std::uint8_t) + 2U * sizeof(std::size_t);
  if (!add_payload(binding_entry_count, binding_entry_size) ||
      !add_payload(key_point_entry_count, sizeof(std::uint8_t)) ||
      !add_payload(table_slot_entry_count, sizeof(std::uint8_t)) ||
      !add_payload(
          component_parent_entry_count,
          sizeof(ExactDirectSparseComponentHandle))) {
    return std::nullopt;
  }
  return total;
}

[[nodiscard]] bool canonical_key_shape(
    const ExactDirectSparseFacetKey& key) {
  if (key.point_count == 0U ||
      key.point_count >
          direct_sparse_positive_facet_maximum_point_count) {
    return false;
  }
  for (std::size_t index = 1U; index < key.point_count; ++index) {
    if (key.point_ids[index - 1U] >= key.point_ids[index]) {
      return false;
    }
  }
  return std::all_of(
      key.point_ids.begin() +
          static_cast<std::ptrdiff_t>(key.point_count),
      key.point_ids.end(),
      [](spatial::PointId point_id) { return point_id == 0U; });
}

[[nodiscard]] bool witness_matches_authority(
    const ExactDirectSparseFacetWitness& witness,
    std::uint64_t external_authority_id) {
  return witness.external_authority_id == external_authority_id &&
         external_authority_id != 0U && witness.replay_token != 0U;
}

[[nodiscard]] std::uint64_t mix_fingerprint_word(
    std::uint64_t hash,
    std::uint64_t word) noexcept {
  word ^= word >> 30U;
  word *= UINT64_C(0xbf58476d1ce4e5b9);
  word ^= word >> 27U;
  word *= UINT64_C(0x94d049bb133111eb);
  word ^= word >> 31U;
  hash ^= word + UINT64_C(0x9e3779b97f4a7c15) + (hash << 6U) +
          (hash >> 2U);
  return hash;
}

[[nodiscard]] bool complete_key_matches_arena(
    const ExactDirectSparsePositiveFacetSlot& slot,
    const ExactDirectSparseFacetKey& key,
    std::span<const spatial::PointId> key_point_arena) {
  if (slot.key_point_count != key.point_count ||
      slot.key_point_offset > key_point_arena.size() ||
      slot.key_point_count >
          key_point_arena.size() - slot.key_point_offset) {
    return false;
  }
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    if (key_point_arena[slot.key_point_offset + index] !=
        key.point_ids[index]) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool complete_keys_match(
    const ExactDirectSparseFacetKey& left,
    const ExactDirectSparseFacetKey& right) {
  if (left.point_count != right.point_count) {
    return false;
  }
  for (std::size_t index = 0U; index < left.point_count; ++index) {
    if (left.point_ids[index] != right.point_ids[index]) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] ExactDirectSparseComponentHandle find_component(
    std::span<const ExactDirectSparseComponentHandle> parents,
    ExactDirectSparseComponentHandle handle) {
  while (parents[handle] != handle) {
    handle = parents[handle];
  }
  return handle;
}

void unite_components(
    std::vector<ExactDirectSparseComponentHandle>& parents,
    ExactDirectSparseComponentHandle left,
    ExactDirectSparseComponentHandle right) {
  left = find_component(parents, left);
  right = find_component(parents, right);
  if (left != right) {
    parents[std::max(left, right)] = std::min(left, right);
  }
}

enum class StructuralUnionReplayStatus : std::uint8_t {
  complete,
  budget_exhausted,
  invalid_structure,
};

[[nodiscard]] StructuralUnionReplayStatus find_component_for_structure(
    std::span<const ExactDirectSparseComponentHandle> parents,
    ExactDirectSparseComponentHandle handle,
    std::size_t maximum_parent_hop_count,
    std::size_t& parent_hop_count,
    ExactDirectSparseComponentHandle& root) noexcept {
  if (handle >= parents.size()) {
    return StructuralUnionReplayStatus::invalid_structure;
  }
  while (parents[handle] != handle) {
    if (parents[handle] >= parents.size()) {
      return StructuralUnionReplayStatus::invalid_structure;
    }
    if (parent_hop_count >= maximum_parent_hop_count) {
      return StructuralUnionReplayStatus::budget_exhausted;
    }
    handle = parents[handle];
    ++parent_hop_count;
  }
  root = handle;
  return StructuralUnionReplayStatus::complete;
}

[[nodiscard]] StructuralUnionReplayStatus unite_components_for_structure(
    std::vector<ExactDirectSparseComponentHandle>& parents,
    ExactDirectSparseComponentHandle left,
    ExactDirectSparseComponentHandle right,
    std::size_t maximum_parent_hop_count,
    std::size_t& parent_hop_count) noexcept {
  ExactDirectSparseComponentHandle left_root = 0U;
  ExactDirectSparseComponentHandle right_root = 0U;
  const StructuralUnionReplayStatus left_status =
      find_component_for_structure(
          parents,
          left,
          maximum_parent_hop_count,
          parent_hop_count,
          left_root);
  if (left_status != StructuralUnionReplayStatus::complete) {
    return left_status;
  }
  const StructuralUnionReplayStatus right_status =
      find_component_for_structure(
          parents,
          right,
          maximum_parent_hop_count,
          parent_hop_count,
          right_root);
  if (right_status != StructuralUnionReplayStatus::complete) {
    return right_status;
  }
  if (left_root != right_root) {
    parents[std::max(left_root, right_root)] =
        std::min(left_root, right_root);
  }
  return StructuralUnionReplayStatus::complete;
}

struct SlotSearchResult {
  std::optional<std::size_t> matching_slot;
  std::size_t slot_visit_count{};
  std::size_t full_key_comparison_count{};
  std::size_t equal_fingerprint_distinct_key_count{};
  bool slot_visit_budget_exhausted{false};
};

[[nodiscard]] bool accumulate_search_counters(
    ExactDirectSparsePositiveFacetBatchCounters& counters,
    std::size_t full_key_comparison_count,
    std::size_t equal_fingerprint_distinct_key_count) {
  const auto full_key_total = checked_add(
      counters.full_key_comparison_count,
      full_key_comparison_count);
  const auto distinct_key_total = checked_add(
      counters.equal_fingerprint_distinct_key_count,
      equal_fingerprint_distinct_key_count);
  if (!full_key_total.has_value() || !distinct_key_total.has_value()) {
    return false;
  }
  counters.full_key_comparison_count = *full_key_total;
  counters.equal_fingerprint_distinct_key_count = *distinct_key_total;
  return true;
}

[[nodiscard]] SlotSearchResult search_committed_slots(
    std::span<const ExactDirectSparsePositiveFacetSlot> slots,
    std::span<const spatial::PointId> key_point_arena,
    const ExactDirectSparseFacetKey& key,
    std::uint64_t fingerprint,
    std::size_t maximum_slot_visit_count =
        std::numeric_limits<std::size_t>::max()) {
  SlotSearchResult result;
  if (slots.empty()) {
    return result;
  }
  std::size_t slot_index =
      static_cast<std::size_t>(fingerprint % slots.size());
  for (std::size_t probe = 0U; probe < slots.size(); ++probe) {
    if (result.slot_visit_count >= maximum_slot_visit_count) {
      result.slot_visit_budget_exhausted = true;
      return result;
    }
    const ExactDirectSparsePositiveFacetSlot& slot = slots[slot_index];
    ++result.slot_visit_count;
    if (!slot.occupied) {
      return result;
    }
    if (slot.fingerprint == fingerprint) {
      ++result.full_key_comparison_count;
      if (complete_key_matches_arena(slot, key, key_point_arena)) {
        result.matching_slot = slot_index;
        return result;
      }
      ++result.equal_fingerprint_distinct_key_count;
    }
    slot_index = (slot_index + 1U) % slots.size();
  }
  return result;
}

struct BatchRequirements {
  std::size_t key_point_count{};
  bool capacity_safe{true};
  bool input_shape_certified{true};
  bool witnesses_certified{true};
};

template <typename Record>
void analyze_keyed_records(
    std::span<const Record> records,
    std::uint64_t external_authority_id,
    BatchRequirements& requirements) {
  for (std::size_t index = 0U; index < records.size(); ++index) {
    const Record& record = records[index];
    if (record.query_index != index || !canonical_key_shape(record.key)) {
      requirements.input_shape_certified = false;
    }
    if (!witness_matches_authority(
            record.witness, external_authority_id)) {
      requirements.witnesses_certified = false;
    }
    const auto next = checked_add(
        requirements.key_point_count, record.key.point_count);
    if (!next.has_value()) {
      requirements.capacity_safe = false;
    } else {
      requirements.key_point_count = *next;
    }
  }
}

template <>
void analyze_keyed_records<ExactDirectSparseFacetBinding>(
    std::span<const ExactDirectSparseFacetBinding> records,
    std::uint64_t external_authority_id,
    BatchRequirements& requirements) {
  for (std::size_t index = 0U; index < records.size(); ++index) {
    const ExactDirectSparseFacetBinding& record = records[index];
    if (record.binding_index != index ||
        !canonical_key_shape(record.key)) {
      requirements.input_shape_certified = false;
    }
    if (!witness_matches_authority(
            record.witness, external_authority_id)) {
      requirements.witnesses_certified = false;
    }
    const auto next = checked_add(
        requirements.key_point_count, record.key.point_count);
    if (!next.has_value()) {
      requirements.capacity_safe = false;
    } else {
      requirements.key_point_count = *next;
    }
  }
}

[[nodiscard]] BatchRequirements analyze_batch_requirements(
    std::span<const ExactDirectSparseFacetQuery> queries,
    std::span<const ExactDirectSparseComponentUnion> unions,
    std::span<const ExactDirectSparseFacetBinding> bindings,
    std::size_t component_handle_count,
    std::uint64_t external_authority_id) {
  BatchRequirements requirements;
  analyze_keyed_records(queries, external_authority_id, requirements);
  analyze_keyed_records(bindings, external_authority_id, requirements);
  for (std::size_t index = 0U; index < unions.size(); ++index) {
    const ExactDirectSparseComponentUnion& component_union = unions[index];
    if (component_union.union_index != index ||
        component_union.left_handle >= component_handle_count ||
        component_union.right_handle >= component_handle_count) {
      requirements.input_shape_certified = false;
    }
    if (!witness_matches_authority(
            component_union.witness, external_authority_id)) {
      requirements.witnesses_certified = false;
    }
  }
  for (const ExactDirectSparseFacetBinding& binding : bindings) {
    if (binding.component_handle >= component_handle_count) {
      requirements.input_shape_certified = false;
    }
  }
  return requirements;
}

[[nodiscard]] bool batch_budget_is_sufficient(
    const ExactDirectSparsePositiveFacetLocatorBudget& budget,
    const ExactDirectSparsePositiveFacetLocatorCounters& counters,
    std::size_t committed_union_count,
    const BatchRequirements& requirements,
    std::size_t query_count,
    std::size_t union_count,
    std::size_t binding_count) {
  const auto union_total = checked_add(committed_union_count, union_count);
  const auto batch_total =
      checked_add(counters.committed_batch_count, 1U);
  return union_total.has_value() && batch_total.has_value() &&
         query_count <= budget.maximum_batch_query_count &&
         union_count <= budget.maximum_batch_union_count &&
         binding_count <= budget.maximum_batch_binding_count &&
         requirements.key_point_count <=
             budget.maximum_batch_key_point_count &&
         *union_total <= budget.maximum_committed_union_count &&
         *batch_total <= budget.maximum_committed_batch_count &&
         counters.inserted_binding_count <=
             budget.maximum_committed_binding_count &&
         counters.committed_key_point_count <=
             budget.maximum_committed_key_point_count;
}

struct PendingBinding {
  ExactDirectSparseFacetKey key{};
  std::uint64_t fingerprint{};
  ExactDirectSparseComponentHandle component_handle{};
  ExactDirectSparseFacetWitness witness{};
};

constexpr std::string_view locator_snapshot_initial_digest_domain =
    "MorseHGP3D/phase10/direct-sparse-positive-facet-locator/"
    "snapshot-initial/v1/sha256/";
constexpr std::string_view locator_snapshot_chain_digest_domain =
    "MorseHGP3D/phase10/direct-sparse-positive-facet-locator/"
    "snapshot-chain/v1/sha256/";

void digest_u64(
    contract::CanonicalSha256Builder& builder,
    std::uint64_t value) {
  std::array<std::uint8_t, 8U> encoded{};
  for (std::size_t index = 0U; index < encoded.size(); ++index) {
    const std::size_t shift = 8U * (encoded.size() - index - 1U);
    encoded[index] = static_cast<std::uint8_t>(value >> shift);
  }
  builder.update(encoded);
}

void digest_size(
    contract::CanonicalSha256Builder& builder,
    std::size_t value) {
  static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));
  digest_u64(builder, static_cast<std::uint64_t>(value));
}

void digest_identifier(
    contract::CanonicalSha256Builder& builder,
    const contract::CanonicalId& identifier) {
  builder.update(identifier.bytes());
}

void digest_witness(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparseFacetWitness& witness) {
  digest_u64(builder, witness.external_authority_id);
  digest_u64(builder, witness.replay_token);
}

void digest_budget(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparsePositiveFacetLocatorBudget& budget) {
  digest_size(builder, budget.maximum_component_handle_count);
  digest_size(builder, budget.maximum_committed_binding_count);
  digest_size(builder, budget.maximum_committed_key_point_count);
  digest_size(builder, budget.maximum_committed_union_count);
  digest_size(builder, budget.maximum_committed_batch_count);
  digest_size(builder, budget.maximum_batch_query_count);
  digest_size(builder, budget.maximum_batch_union_count);
  digest_size(builder, budget.maximum_batch_binding_count);
  digest_size(builder, budget.maximum_batch_key_point_count);
  digest_size(builder, budget.maximum_table_slot_count);
  digest_size(builder, budget.maximum_batch_scratch_slot_count);
}

void digest_batch_counters(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparsePositiveFacetBatchCounters& counters) {
  digest_size(builder, counters.query_count);
  digest_size(builder, counters.positive_lookup_count);
  digest_size(builder, counters.unresolved_lookup_count);
  digest_size(builder, counters.union_request_count);
  digest_size(builder, counters.binding_request_count);
  digest_size(builder, counters.inserted_binding_count);
  digest_size(builder, counters.compatible_duplicate_binding_count);
  digest_size(builder, counters.batch_input_key_point_count);
  digest_size(builder, counters.inserted_key_point_count);
  digest_size(builder, counters.full_key_comparison_count);
  digest_size(builder, counters.equal_fingerprint_distinct_key_count);
}

[[nodiscard]] contract::CanonicalId initial_locator_snapshot_digest(
    std::size_t component_handle_count,
    const ExactDirectSparsePositiveFacetLocatorBudget& budget,
    const ExactDirectSparsePositiveFacetLocatorConfig& config) {
  contract::CanonicalSha256Builder builder;
  builder.update(locator_snapshot_initial_digest_domain);
  digest_u64(
      builder, direct_sparse_positive_facet_locator_schema_version);
  digest_budget(builder, budget);
  digest_u64(builder, config.external_authority_id);
  digest_u64(builder, config.fingerprint_mask);
  digest_size(builder, component_handle_count);
  return builder.finalize();
}

class LocatorSnapshotChainDigestBuilder {
 public:
  LocatorSnapshotChainDigestBuilder(
      const contract::CanonicalId& previous,
      const ExactDirectSparseCommittedBatchRecord& batch) {
    builder_.update(locator_snapshot_chain_digest_domain);
    digest_identifier(builder_, previous);
    digest_size(builder_, batch.committed_batch_index);
    digest_batch_counters(builder_, batch.counters);
    digest_u64(builder_, batch.input_shape_certified ? 1U : 0U);
    digest_u64(
        builder_, batch.input_witness_structure_certified ? 1U : 0U);
    digest_u64(
        builder_, batch.strict_pre_batch_snapshot_certified ? 1U : 0U);
    digest_u64(
        builder_, batch.sequential_atomic_commit_certified ? 1U : 0U);
  }

  void component_union(
      ExactDirectSparseComponentHandle left_handle,
      ExactDirectSparseComponentHandle right_handle,
      const ExactDirectSparseFacetWitness& witness) {
    digest_size(builder_, left_handle);
    digest_size(builder_, right_handle);
    digest_witness(builder_, witness);
  }

  void binding(
      const ExactDirectSparseFacetKey& key,
      ExactDirectSparseComponentHandle component_handle,
      const ExactDirectSparseFacetWitness& witness) {
    digest_size(builder_, key.point_count);
    for (std::size_t index = 0U; index < key.point_count; ++index) {
      digest_u64(builder_, key.point_ids[index]);
    }
    digest_size(builder_, component_handle);
    digest_witness(builder_, witness);
  }

  [[nodiscard]] contract::CanonicalId finalize() {
    return builder_.finalize();
  }

 private:
  contract::CanonicalSha256Builder builder_;
};

struct PendingSearchResult {
  std::optional<std::size_t> pending_index;
  std::size_t full_key_comparison_count{};
  std::size_t equal_fingerprint_distinct_key_count{};
};

[[nodiscard]] PendingSearchResult search_pending_bindings(
    std::span<const std::size_t> scratch_slots,
    std::span<const PendingBinding> pending,
    const ExactDirectSparseFacetKey& key,
    std::uint64_t fingerprint) {
  PendingSearchResult result;
  const std::size_t empty = std::numeric_limits<std::size_t>::max();
  std::size_t slot_index =
      static_cast<std::size_t>(fingerprint % scratch_slots.size());
  for (std::size_t probe = 0U; probe < scratch_slots.size(); ++probe) {
    const std::size_t candidate_index = scratch_slots[slot_index];
    if (candidate_index == empty) {
      return result;
    }
    const PendingBinding& candidate = pending[candidate_index];
    if (candidate.fingerprint == fingerprint) {
      ++result.full_key_comparison_count;
      if (complete_keys_match(candidate.key, key)) {
        result.pending_index = candidate_index;
        return result;
      }
      ++result.equal_fingerprint_distinct_key_count;
    }
    slot_index = (slot_index + 1U) % scratch_slots.size();
  }
  return result;
}

void insert_pending_scratch_slot(
    std::vector<std::size_t>& scratch_slots,
    std::span<const PendingBinding> pending,
    std::size_t pending_index) {
  const std::size_t empty = std::numeric_limits<std::size_t>::max();
  std::size_t slot_index = static_cast<std::size_t>(
      pending[pending_index].fingerprint % scratch_slots.size());
  while (scratch_slots[slot_index] != empty) {
    slot_index = (slot_index + 1U) % scratch_slots.size();
  }
  scratch_slots[slot_index] = pending_index;
}

[[nodiscard]] std::size_t first_empty_committed_slot(
    std::span<const ExactDirectSparsePositiveFacetSlot> slots,
    std::uint64_t fingerprint) {
  std::size_t slot_index =
      static_cast<std::size_t>(fingerprint % slots.size());
  while (slots[slot_index].occupied) {
    slot_index = (slot_index + 1U) % slots.size();
  }
  return slot_index;
}

[[nodiscard]] std::optional<ExactDirectSparsePositiveFacetLocatorCounters>
updated_counters(
    const ExactDirectSparsePositiveFacetLocatorCounters& current,
    const ExactDirectSparsePositiveFacetBatchCounters& batch) {
  ExactDirectSparsePositiveFacetLocatorCounters next = current;
  const auto committed_batch_count =
      checked_add(next.committed_batch_count, 1U);
  const auto query_count = checked_add(next.query_count, batch.query_count);
  const auto positive_lookup_count = checked_add(
      next.positive_lookup_count, batch.positive_lookup_count);
  const auto unresolved_lookup_count = checked_add(
      next.unresolved_lookup_count, batch.unresolved_lookup_count);
  const auto union_request_count = checked_add(
      next.union_request_count, batch.union_request_count);
  const auto binding_request_count = checked_add(
      next.binding_request_count, batch.binding_request_count);
  const auto inserted_binding_count = checked_add(
      next.inserted_binding_count, batch.inserted_binding_count);
  const auto compatible_duplicate_binding_count = checked_add(
      next.compatible_duplicate_binding_count,
      batch.compatible_duplicate_binding_count);
  const auto committed_key_point_count = checked_add(
      next.committed_key_point_count, batch.inserted_key_point_count);
  const auto full_key_comparison_count = checked_add(
      next.full_key_comparison_count, batch.full_key_comparison_count);
  const auto equal_fingerprint_distinct_key_count = checked_add(
      next.equal_fingerprint_distinct_key_count,
      batch.equal_fingerprint_distinct_key_count);
  if (!committed_batch_count.has_value() || !query_count.has_value() ||
      !positive_lookup_count.has_value() ||
      !unresolved_lookup_count.has_value() ||
      !union_request_count.has_value() ||
      !binding_request_count.has_value() ||
      !inserted_binding_count.has_value() ||
      !compatible_duplicate_binding_count.has_value() ||
      !committed_key_point_count.has_value() ||
      !full_key_comparison_count.has_value() ||
      !equal_fingerprint_distinct_key_count.has_value()) {
    return std::nullopt;
  }
  next.committed_batch_count = *committed_batch_count;
  next.query_count = *query_count;
  next.positive_lookup_count = *positive_lookup_count;
  next.unresolved_lookup_count = *unresolved_lookup_count;
  next.union_request_count = *union_request_count;
  next.binding_request_count = *binding_request_count;
  next.inserted_binding_count = *inserted_binding_count;
  next.compatible_duplicate_binding_count =
      *compatible_duplicate_binding_count;
  next.committed_key_point_count = *committed_key_point_count;
  next.full_key_comparison_count = *full_key_comparison_count;
  next.equal_fingerprint_distinct_key_count =
      *equal_fingerprint_distinct_key_count;
  return next;
}

struct LocatorHistoryReplayCursor {
  ExactDirectSparsePositiveFacetLocatorCounters counters{};
  std::size_t binding_prefix{};
  std::size_t union_prefix{};
  contract::CanonicalId history_digest{};
};

enum class LocatorHistoryTransitionStatus : std::uint8_t {
  complete,
  capacity_overflow,
  malformed_history,
};

[[nodiscard]] ExactDirectSparsePositiveFacetLocatorSnapshotStamp
make_locator_snapshot_stamp(
    std::uint32_t schema_version,
    std::uint64_t external_authority_id,
    const ExactDirectSparsePositiveFacetLocatorCounters& counters,
    const contract::CanonicalId& history_digest) {
  return {
      schema_version,
      external_authority_id,
      counters.committed_batch_count,
      counters.inserted_binding_count,
      counters.union_request_count,
      counters.binding_request_count,
      history_digest};
}

// This is the single durable-history transition used by both the structural
// verifier and the public prefix-stamp sweep.  The transaction builder above
// uses the same LocatorSnapshotChainDigestBuilder directly on its pending
// canonical inputs, so digest serialization has exactly one implementation.
[[nodiscard]] LocatorHistoryTransitionStatus
replay_locator_history_digest_transition(
    std::size_t expected_batch_index,
    const ExactDirectSparseCommittedBatchRecord& record,
    std::size_t trusted_component_handle_count,
    const ExactDirectSparsePositiveFacetLocatorBudget& trusted_budget,
    std::uint64_t trusted_external_authority_id,
    std::span<const ExactDirectSparseCommittedUnionRecord> committed_unions,
    std::span<const ExactDirectSparsePositiveFacetSlot> slots,
    std::span<const spatial::PointId> key_point_arena,
    std::span<const std::size_t> binding_slot_indices_by_index,
    LocatorHistoryReplayCursor& cursor) {
  const auto query_partition = checked_add(
      record.counters.positive_lookup_count,
      record.counters.unresolved_lookup_count);
  const auto binding_partition = checked_add(
      record.counters.inserted_binding_count,
      record.counters.compatible_duplicate_binding_count);
  const auto minimum_full_key_comparison_count = checked_add(
      record.counters.positive_lookup_count,
      record.counters.compatible_duplicate_binding_count);
  const auto keyed_input_count = checked_add(
      record.counters.query_count,
      record.counters.binding_request_count);
  const auto maximum_input_key_point_count =
      keyed_input_count.has_value()
          ? checked_multiply(
                direct_sparse_positive_facet_maximum_point_count,
                *keyed_input_count)
          : std::nullopt;
  const auto maximum_inserted_key_point_count = checked_multiply(
      direct_sparse_positive_facet_maximum_point_count,
      record.counters.inserted_binding_count);
  const auto next_binding_prefix = checked_add(
      cursor.binding_prefix, record.counters.inserted_binding_count);
  const auto next_union_prefix = checked_add(
      cursor.union_prefix, record.counters.union_request_count);
  if (!query_partition.has_value() || !binding_partition.has_value() ||
      !minimum_full_key_comparison_count.has_value() ||
      !keyed_input_count.has_value() ||
      !maximum_input_key_point_count.has_value() ||
      !maximum_inserted_key_point_count.has_value() ||
      !next_binding_prefix.has_value() || !next_union_prefix.has_value()) {
    return LocatorHistoryTransitionStatus::capacity_overflow;
  }
  if (record.committed_batch_index != expected_batch_index ||
      *query_partition != record.counters.query_count ||
      *binding_partition != record.counters.binding_request_count ||
      record.counters.full_key_comparison_count <
          *minimum_full_key_comparison_count ||
      record.counters.query_count > trusted_budget.maximum_batch_query_count ||
      record.counters.union_request_count >
          trusted_budget.maximum_batch_union_count ||
      record.counters.binding_request_count >
          trusted_budget.maximum_batch_binding_count ||
      record.counters.batch_input_key_point_count >
          trusted_budget.maximum_batch_key_point_count ||
      record.counters.inserted_key_point_count >
          record.counters.batch_input_key_point_count ||
      record.counters.batch_input_key_point_count < *keyed_input_count ||
      record.counters.batch_input_key_point_count >
          *maximum_input_key_point_count ||
      record.counters.inserted_key_point_count <
          record.counters.inserted_binding_count ||
      record.counters.inserted_key_point_count >
          *maximum_inserted_key_point_count ||
      (record.counters.positive_lookup_count != 0U &&
       cursor.binding_prefix == 0U) ||
      (record.counters.compatible_duplicate_binding_count != 0U &&
       cursor.binding_prefix == 0U &&
       record.counters.inserted_binding_count == 0U) ||
      *next_binding_prefix > binding_slot_indices_by_index.size() ||
      *next_union_prefix > committed_unions.size() ||
      record.counters.equal_fingerprint_distinct_key_count >
          record.counters.full_key_comparison_count ||
      !record.input_shape_certified ||
      !record.input_witness_structure_certified ||
      !record.strict_pre_batch_snapshot_certified ||
      !record.sequential_atomic_commit_certified) {
    return LocatorHistoryTransitionStatus::malformed_history;
  }

  const auto next_counters = updated_counters(cursor.counters, record.counters);
  if (!next_counters.has_value()) {
    return LocatorHistoryTransitionStatus::capacity_overflow;
  }

  LocatorSnapshotChainDigestBuilder snapshot_digest_builder(
      cursor.history_digest, record);
  for (std::size_t union_index = cursor.union_prefix;
       union_index < *next_union_prefix;
       ++union_index) {
    const ExactDirectSparseCommittedUnionRecord& component_union =
        committed_unions[union_index];
    if (component_union.committed_union_index != union_index ||
        component_union.left_handle >= trusted_component_handle_count ||
        component_union.right_handle >= trusted_component_handle_count ||
        !witness_matches_authority(
            component_union.witness, trusted_external_authority_id)) {
      return LocatorHistoryTransitionStatus::malformed_history;
    }
    snapshot_digest_builder.component_union(
        component_union.left_handle,
        component_union.right_handle,
        component_union.witness);
  }

  std::size_t observed_inserted_key_point_count = 0U;
  for (std::size_t binding_index = cursor.binding_prefix;
       binding_index < *next_binding_prefix;
       ++binding_index) {
    const std::size_t slot_index =
        binding_slot_indices_by_index[binding_index];
    if (slot_index >= slots.size()) {
      return LocatorHistoryTransitionStatus::malformed_history;
    }
    const ExactDirectSparsePositiveFacetSlot& slot = slots[slot_index];
    if (!slot.occupied || slot.committed_binding_index != binding_index ||
        slot.component_handle >= trusted_component_handle_count ||
        !witness_matches_authority(
            slot.binding_witness, trusted_external_authority_id) ||
        slot.key_point_count == 0U ||
        slot.key_point_count >
            direct_sparse_positive_facet_maximum_point_count ||
        slot.key_point_offset > key_point_arena.size() ||
        slot.key_point_count >
            key_point_arena.size() - slot.key_point_offset) {
      return LocatorHistoryTransitionStatus::malformed_history;
    }
    const auto next_key_point_count = checked_add(
        observed_inserted_key_point_count, slot.key_point_count);
    if (!next_key_point_count.has_value()) {
      return LocatorHistoryTransitionStatus::capacity_overflow;
    }
    observed_inserted_key_point_count = *next_key_point_count;

    ExactDirectSparseFacetKey key;
    key.point_count = slot.key_point_count;
    for (std::size_t point_index = 0U;
         point_index < slot.key_point_count;
         ++point_index) {
      key.point_ids[point_index] =
          key_point_arena[slot.key_point_offset + point_index];
    }
    if (!canonical_key_shape(key)) {
      return LocatorHistoryTransitionStatus::malformed_history;
    }
    snapshot_digest_builder.binding(
        key, slot.component_handle, slot.binding_witness);
  }
  if (observed_inserted_key_point_count !=
      record.counters.inserted_key_point_count) {
    return LocatorHistoryTransitionStatus::malformed_history;
  }

  cursor.counters = *next_counters;
  cursor.binding_prefix = *next_binding_prefix;
  cursor.union_prefix = *next_union_prefix;
  cursor.history_digest = snapshot_digest_builder.finalize();
  return LocatorHistoryTransitionStatus::complete;
}

}  // namespace

std::uint64_t fingerprint_exact_direct_sparse_facet_key(
    const ExactDirectSparseFacetKey& key,
    std::uint64_t fingerprint_mask) noexcept {
  std::uint64_t hash = mix_fingerprint_word(
      UINT64_C(0x6a09e667f3bcc909),
      static_cast<std::uint64_t>(key.point_count));
  const std::size_t bounded_point_count =
      std::min(
          key.point_count,
          direct_sparse_positive_facet_maximum_point_count);
  for (std::size_t index = 0U; index < bounded_point_count; ++index) {
    hash = mix_fingerprint_word(hash, key.point_ids[index]);
  }
  return hash & fingerprint_mask;
}

bool ExactDirectSparsePositiveFacetLocatorPrefixStampSweepResult::
    certified_partial_refinement() const noexcept {
  const auto expected_batch_record_scan_count = checked_multiply(
      2U, required_committed_batch_prefix_count);
  const auto expected_scratch_bytes = checked_multiply(
      required_active_binding_prefix_count, sizeof(std::size_t));
  return schema_version ==
             direct_sparse_positive_facet_locator_prefix_stamp_sweep_schema_version &&
         expected_batch_record_scan_count.has_value() &&
         expected_scratch_bytes.has_value() &&
         *expected_batch_record_scan_count ==
             required_batch_record_scan_count &&
         *expected_scratch_bytes ==
             required_temporary_scratch_byte_count &&
         prefix_stamps.size() == prefix_request_count &&
         prefix_request_count <=
             requested_budget.maximum_prefix_request_count &&
         required_batch_record_scan_count <=
             requested_budget.maximum_batch_record_scan_count &&
         required_table_slot_scan_count <=
             requested_budget.maximum_table_slot_scan_count &&
         required_active_binding_prefix_count <=
             requested_budget.maximum_binding_slot_index_scratch_count &&
         required_union_record_replay_count <=
             requested_budget.maximum_union_record_replay_count &&
         required_active_binding_prefix_count <=
             requested_budget.maximum_binding_record_replay_count &&
         required_key_point_replay_count <=
             requested_budget.maximum_key_point_replay_count &&
         required_temporary_scratch_byte_count <=
             requested_budget.maximum_temporary_scratch_byte_count &&
         counters.prefix_request_scan_count == prefix_request_count &&
         counters.batch_record_scan_count ==
             required_batch_record_scan_count &&
         counters.table_slot_scan_count == required_table_slot_scan_count &&
         counters.union_record_replay_count ==
             required_union_record_replay_count &&
         counters.binding_record_replay_count ==
             required_active_binding_prefix_count &&
         counters.key_point_replay_count == required_key_point_replay_count &&
         counters.emitted_stamp_count == prefix_request_count &&
         counters.locator_snapshot_check_count == 2U &&
         locator_certified_at_entry &&
         prefix_requests_nondecreasing_and_in_history &&
         budget_preflight_certified && active_binding_slots_indexed_once &&
         every_requested_batch_preflighted_and_replayed_once &&
         every_union_binding_and_key_point_replayed_once &&
         every_requested_prefix_stamp_emitted_once &&
         final_prefix_matches_live_locator_when_requested &&
         common_frozen_locator_snapshot_certified &&
         no_partial_scientific_payload_published && !locator_state_mutated &&
         !locator_batch_committed && !external_authority_replayed_by_locator &&
         !forbidden_global_structure_materialized && !public_status_claimed &&
         partial_refinement_only &&
         decision ==
             ExactDirectSparsePositiveFacetLocatorPrefixStampSweepDecision::
                 complete_certified_locator_prefix_stamps &&
         scope ==
             ExactDirectSparsePositiveFacetLocatorPrefixStampSweepScope::
                 locator_internal_committed_batch_prefix_stamps_relative_to_frozen_history_only;
}

bool ExactDirectSparsePositiveFacetLocatorPrefixStampSweepResult::
    certified_atomic_failure() const noexcept {
  return schema_version ==
             direct_sparse_positive_facet_locator_prefix_stamp_sweep_schema_version &&
         prefix_stamps.empty() && no_partial_scientific_payload_published &&
         !locator_state_mutated && !locator_batch_committed &&
         !external_authority_replayed_by_locator &&
         !forbidden_global_structure_materialized && !public_status_claimed &&
         partial_refinement_only &&
         decision !=
             ExactDirectSparsePositiveFacetLocatorPrefixStampSweepDecision::
                 not_certified &&
         decision !=
             ExactDirectSparsePositiveFacetLocatorPrefixStampSweepDecision::
                 complete_certified_locator_prefix_stamps &&
         scope ==
             ExactDirectSparsePositiveFacetLocatorPrefixStampSweepScope::
                 locator_internal_committed_batch_prefix_stamps_relative_to_frozen_history_only;
}

bool ExactDirectSparsePositiveFacetLocatorPrefixStampSweepResult::
    certified_outcome() const noexcept {
  return certified_partial_refinement() || certified_atomic_failure();
}

bool ExactDirectSparsePositiveFacetBatchResult::certified_committed_batch()
    const noexcept {
  return schema_version ==
             direct_sparse_positive_facet_locator_schema_version &&
         lookups.size() == counters.query_count &&
         counters.positive_lookup_count + counters.unresolved_lookup_count ==
             counters.query_count &&
         budget_preflight_certified && input_shape_certified &&
         every_input_witness_non_null_and_authority_matched &&
         lookups_use_strict_pre_batch_snapshot &&
         current_batch_bindings_hidden_from_lookups &&
         every_positive_lookup_has_non_null_external_witness_tokens &&
         every_fingerprint_candidate_compared_by_full_key &&
         explicit_unions_applied_before_binding_compatibility &&
         exact_duplicate_bindings_compatible_after_explicit_unions &&
         atomic_commit_performed && locator_state_mutated &&
         !contradiction_detected && !missing_facet_means_isolated &&
         !total_facet_authority_claimed &&
         decision == ExactDirectSparsePositiveFacetBatchDecision::
                         complete_certified_sparse_positive_batch_commit &&
         scope == ExactDirectSparsePositiveFacetLocatorScope::
                      positive_bindings_relative_to_caller_asserted_external_authority_only;
}

bool ExactDirectSparsePositiveFacetProbeResult::certified_positive_hit()
    const noexcept {
  return schema_version ==
             direct_sparse_positive_facet_locator_schema_version &&
         canonical_key_shape(query_key) && slot_visit_count > 0U &&
         slot_visit_count <= budget.maximum_slot_visit_count &&
         component_parent_hop_count <=
             budget.maximum_component_parent_hop_count &&
         full_key_comparison_count > 0U &&
         equal_fingerprint_distinct_key_count ==
             full_key_comparison_count - 1U &&
         locator_certified_at_entry && input_shape_certified &&
         query_witness_non_null_and_authority_matched &&
         query_witness.external_authority_id != 0U &&
         query_witness.replay_token != 0U &&
         every_fingerprint_candidate_compared_by_full_key &&
         slot_search_completed && component_find_completed &&
         component_handle_present && source_binding_witness_present &&
         source_binding_witness.external_authority_id ==
             query_witness.external_authority_id &&
         source_binding_witness.replay_token != 0U &&
         !slot_visit_budget_exhausted &&
         !component_parent_hop_budget_exhausted &&
         !locator_state_mutated && !batch_committed &&
         !missing_facet_means_isolated &&
         !total_facet_authority_claimed &&
         disposition ==
             ExactDirectSparsePositiveFacetProbeDisposition::positive &&
         decision == ExactDirectSparsePositiveFacetProbeDecision::
                         complete_certified_positive_hit &&
         scope == ExactDirectSparsePositiveFacetLocatorScope::
                      positive_bindings_relative_to_caller_asserted_external_authority_only;
}

bool ExactDirectSparsePositiveFacetProbeResult::certified_unresolved_miss()
    const noexcept {
  return schema_version ==
             direct_sparse_positive_facet_locator_schema_version &&
         canonical_key_shape(query_key) && slot_visit_count > 0U &&
         slot_visit_count <= budget.maximum_slot_visit_count &&
         component_parent_hop_count == 0U &&
         equal_fingerprint_distinct_key_count ==
             full_key_comparison_count &&
         locator_certified_at_entry && input_shape_certified &&
         query_witness_non_null_and_authority_matched &&
         query_witness.external_authority_id != 0U &&
         query_witness.replay_token != 0U &&
         every_fingerprint_candidate_compared_by_full_key &&
         slot_search_completed && !component_find_completed &&
         !component_handle_present && !source_binding_witness_present &&
         component_handle == ExactDirectSparseComponentHandle{} &&
         source_binding_witness == ExactDirectSparseFacetWitness{} &&
         !slot_visit_budget_exhausted &&
         !component_parent_hop_budget_exhausted &&
         !locator_state_mutated && !batch_committed &&
         !missing_facet_means_isolated &&
         !total_facet_authority_claimed &&
         disposition ==
             ExactDirectSparsePositiveFacetProbeDisposition::unresolved &&
         decision == ExactDirectSparsePositiveFacetProbeDecision::
                         complete_certified_unresolved_miss &&
         scope == ExactDirectSparsePositiveFacetLocatorScope::
                      positive_bindings_relative_to_caller_asserted_external_authority_only;
}

bool ExactDirectSparsePositiveFacetProbeResult::certified_budget_exhaustion()
    const noexcept {
  const bool slot_budget_exhausted =
      slot_visit_budget_exhausted &&
      !component_parent_hop_budget_exhausted &&
      !slot_search_completed && !component_find_completed &&
      slot_visit_count == budget.maximum_slot_visit_count &&
      component_parent_hop_count == 0U &&
      full_key_comparison_count ==
          equal_fingerprint_distinct_key_count &&
      decision == ExactDirectSparsePositiveFacetProbeDecision::
                      no_positive_locator_slot_visit_budget_exhausted;
  const bool parent_budget_exhausted =
      !slot_visit_budget_exhausted &&
      component_parent_hop_budget_exhausted &&
      slot_search_completed && !component_find_completed &&
      slot_visit_count > 0U && full_key_comparison_count > 0U &&
      equal_fingerprint_distinct_key_count ==
          full_key_comparison_count - 1U &&
      component_parent_hop_count ==
          budget.maximum_component_parent_hop_count &&
      decision == ExactDirectSparsePositiveFacetProbeDecision::
                      no_positive_locator_component_parent_hop_budget_exhausted;
  return schema_version ==
             direct_sparse_positive_facet_locator_schema_version &&
         canonical_key_shape(query_key) &&
         slot_visit_count <= budget.maximum_slot_visit_count &&
         component_parent_hop_count <=
             budget.maximum_component_parent_hop_count &&
         locator_certified_at_entry && input_shape_certified &&
         query_witness_non_null_and_authority_matched &&
         query_witness.external_authority_id != 0U &&
         query_witness.replay_token != 0U &&
         every_fingerprint_candidate_compared_by_full_key &&
         !component_handle_present && !source_binding_witness_present &&
         component_handle == ExactDirectSparseComponentHandle{} &&
         source_binding_witness == ExactDirectSparseFacetWitness{} &&
         !locator_state_mutated && !batch_committed &&
         !missing_facet_means_isolated &&
         !total_facet_authority_claimed &&
         disposition == ExactDirectSparsePositiveFacetProbeDisposition::
                            budget_exhausted &&
         (slot_budget_exhausted || parent_budget_exhausted) &&
         scope == ExactDirectSparsePositiveFacetLocatorScope::
                      positive_bindings_relative_to_caller_asserted_external_authority_only;
}

bool ExactDirectSparsePositiveFacetLocator::certified_positive_locator()
    const noexcept {
  return schema_version_ ==
             direct_sparse_positive_facet_locator_schema_version &&
         required_component_handle_capacity_ == component_parents_.size() &&
         required_table_slot_capacity_ == slots_.size() &&
         counters_.inserted_binding_count <=
             budget_.maximum_committed_binding_count &&
         counters_.committed_key_point_count == key_point_arena_.size() &&
         counters_.committed_key_point_count <=
             budget_.maximum_committed_key_point_count &&
         counters_.union_request_count == committed_unions_.size() &&
         counters_.union_request_count <=
             budget_.maximum_committed_union_count &&
         counters_.committed_batch_count == committed_batches_.size() &&
         counters_.committed_batch_count <=
             budget_.maximum_committed_batch_count &&
         budget_preflight_certified_ && empty_table_initialized_ &&
         dense_component_handles_initialized_ &&
         flat_durable_key_arena_initialized_ && positive_bindings_only_ &&
         full_key_comparison_required_ && !missing_facet_means_isolated_ &&
         !total_facet_authority_claimed_ &&
         !forbidden_global_structure_materialized_ &&
         !public_status_claimed_ &&
         initialization_decision_ ==
             ExactDirectSparsePositiveFacetLocatorInitializationDecision::
                 complete_certified_empty_sparse_positive_locator &&
         scope_ == ExactDirectSparsePositiveFacetLocatorScope::
                       positive_bindings_relative_to_caller_asserted_external_authority_only;
}

ExactDirectSparsePositiveFacetLocatorSnapshotStamp
ExactDirectSparsePositiveFacetLocator::snapshot_stamp() const noexcept {
  return make_locator_snapshot_stamp(
      schema_version_,
      config_.external_authority_id,
      counters_,
      committed_history_digest_);
}

ExactDirectSparsePositiveFacetProbeResult
ExactDirectSparsePositiveFacetLocator::probe_positive_facet(
    const ExactDirectSparseFacetKey& key,
    const ExactDirectSparseFacetWitness& witness,
    const ExactDirectSparsePositiveFacetProbeBudget& budget) const noexcept {
  ExactDirectSparsePositiveFacetProbeResult result;
  result.budget = budget;
  result.query_key = key;
  result.query_witness = witness;
  result.scope = scope_;
  if (!certified_positive_locator()) {
    result.decision = ExactDirectSparsePositiveFacetProbeDecision::
        no_positive_locator_not_initialized;
    return result;
  }
  result.locator_certified_at_entry = true;
  if (!canonical_key_shape(key)) {
    result.decision = ExactDirectSparsePositiveFacetProbeDecision::
        no_positive_locator_input_shape_rejected;
    return result;
  }
  result.input_shape_certified = true;
  if (!witness_matches_authority(
          witness, config_.external_authority_id)) {
    result.decision = ExactDirectSparsePositiveFacetProbeDecision::
        no_positive_locator_external_witness_rejected;
    return result;
  }
  result.query_witness_non_null_and_authority_matched = true;
  result.every_fingerprint_candidate_compared_by_full_key = true;

  const std::uint64_t fingerprint =
      fingerprint_exact_direct_sparse_facet_key(
          key, config_.fingerprint_mask);
  std::size_t slot_index =
      static_cast<std::size_t>(fingerprint % slots_.size());
  std::optional<std::size_t> matching_slot;
  for (std::size_t probe = 0U; probe < slots_.size(); ++probe) {
    if (result.slot_visit_count >= budget.maximum_slot_visit_count) {
      result.slot_visit_budget_exhausted = true;
      result.disposition =
          ExactDirectSparsePositiveFacetProbeDisposition::budget_exhausted;
      result.decision = ExactDirectSparsePositiveFacetProbeDecision::
          no_positive_locator_slot_visit_budget_exhausted;
      return result;
    }
    const ExactDirectSparsePositiveFacetSlot& slot = slots_[slot_index];
    ++result.slot_visit_count;
    if (!slot.occupied) {
      result.slot_search_completed = true;
      result.disposition =
          ExactDirectSparsePositiveFacetProbeDisposition::unresolved;
      result.decision = ExactDirectSparsePositiveFacetProbeDecision::
          complete_certified_unresolved_miss;
      return result;
    }
    if (slot.fingerprint == fingerprint) {
      ++result.full_key_comparison_count;
      if (complete_key_matches_arena(slot, key, key_point_arena_)) {
        matching_slot = slot_index;
        break;
      }
      ++result.equal_fingerprint_distinct_key_count;
    }
    slot_index = (slot_index + 1U) % slots_.size();
  }

  if (!matching_slot.has_value()) {
    result.slot_search_completed = true;
    result.disposition =
        ExactDirectSparsePositiveFacetProbeDisposition::unresolved;
    result.decision = ExactDirectSparsePositiveFacetProbeDecision::
        complete_certified_unresolved_miss;
    return result;
  }
  result.slot_search_completed = true;

  const ExactDirectSparsePositiveFacetSlot& slot =
      slots_[*matching_slot];
  ExactDirectSparseComponentHandle component_handle = slot.component_handle;
  while (component_parents_[component_handle] != component_handle) {
    if (result.component_parent_hop_count >=
        budget.maximum_component_parent_hop_count) {
      result.component_parent_hop_budget_exhausted = true;
      result.disposition =
          ExactDirectSparsePositiveFacetProbeDisposition::budget_exhausted;
      result.decision = ExactDirectSparsePositiveFacetProbeDecision::
          no_positive_locator_component_parent_hop_budget_exhausted;
      return result;
    }
    component_handle = component_parents_[component_handle];
    ++result.component_parent_hop_count;
  }

  result.component_find_completed = true;
  result.component_handle = component_handle;
  result.source_binding_witness = slot.binding_witness;
  result.component_handle_present = true;
  result.source_binding_witness_present = true;
  result.disposition =
      ExactDirectSparsePositiveFacetProbeDisposition::positive;
  result.decision = ExactDirectSparsePositiveFacetProbeDecision::
      complete_certified_positive_hit;
  return result;
}

ExactDirectSparsePositiveFacetProbeVerification
verify_exact_direct_sparse_positive_facet_probe(
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseFacetKey& key,
    const ExactDirectSparseFacetWitness& witness,
    const ExactDirectSparsePositiveFacetProbeBudget& budget,
    const ExactDirectSparsePositiveFacetProbeResult& observed) noexcept {
  ExactDirectSparsePositiveFacetProbeVerification verification;
  verification.locator_certified_at_entry =
      locator.certified_positive_locator();
  verification.query_key_bound_to_observed_result =
      observed.query_key == key;
  verification.query_witness_bound_to_observed_result =
      observed.query_witness == witness;
  verification.budget_bound_to_observed_result =
      observed.budget == budget;

  const ExactDirectSparsePositiveFacetProbeResult replayed =
      locator.probe_positive_facet(key, witness, budget);
  verification.outcome_contract_certified =
      observed.certified_positive_hit() ||
      observed.certified_unresolved_miss() ||
      observed.certified_budget_exhaustion();
  verification.exact_fresh_probe_replay_certified =
      replayed == observed;
  verification.no_locator_mutation_or_batch_commit =
      !observed.locator_state_mutated && !observed.batch_committed &&
      !replayed.locator_state_mutated && !replayed.batch_committed;
  verification.external_authority_replayed_by_locator = false;
  verification.relative_external_authority_scope_preserved =
      locator.scope() == ExactDirectSparsePositiveFacetLocatorScope::
                             positive_bindings_relative_to_caller_asserted_external_authority_only &&
      observed.scope == locator.scope() &&
      !observed.total_facet_authority_claimed &&
      !observed.missing_facet_means_isolated;
  verification.result_certified =
      verification.locator_certified_at_entry &&
      verification.query_key_bound_to_observed_result &&
      verification.query_witness_bound_to_observed_result &&
      verification.budget_bound_to_observed_result &&
      verification.outcome_contract_certified &&
      verification.exact_fresh_probe_replay_certified &&
      verification.no_locator_mutation_or_batch_commit &&
      !verification.external_authority_replayed_by_locator &&
      verification.relative_external_authority_scope_preserved;
  return verification;
}

ExactDirectSparsePositiveFacetLocator
build_exact_direct_sparse_positive_facet_locator(
    std::size_t component_handle_count,
    const ExactDirectSparsePositiveFacetLocatorBudget& budget,
    const ExactDirectSparsePositiveFacetLocatorConfig& config) {
  ExactDirectSparsePositiveFacetLocator locator;
  locator.budget_ = budget;
  locator.config_ = config;
  locator.required_component_handle_capacity_ = component_handle_count;
  locator.scope_ = ExactDirectSparsePositiveFacetLocatorScope::
      positive_bindings_relative_to_caller_asserted_external_authority_only;

  const auto table_slot_capacity =
      probing_slot_capacity(budget.maximum_committed_binding_count);
  const auto batch_scratch_slot_capacity =
      probing_slot_capacity(budget.maximum_batch_binding_count);
  if (!table_slot_capacity.has_value() ||
      !batch_scratch_slot_capacity.has_value()) {
    locator.initialization_decision_ =
        ExactDirectSparsePositiveFacetLocatorInitializationDecision::
            no_locator_capacity_overflow;
    return locator;
  }
  locator.required_table_slot_capacity_ = *table_slot_capacity;
  locator.required_batch_scratch_slot_capacity_ =
      *batch_scratch_slot_capacity;

  if (config.external_authority_id == 0U) {
    locator.initialization_decision_ =
        ExactDirectSparsePositiveFacetLocatorInitializationDecision::
            no_locator_external_authority_rejected;
    return locator;
  }
  if (component_handle_count > budget.maximum_component_handle_count ||
      *table_slot_capacity > budget.maximum_table_slot_count ||
      *batch_scratch_slot_capacity >
          budget.maximum_batch_scratch_slot_count) {
    locator.initialization_decision_ =
        ExactDirectSparsePositiveFacetLocatorInitializationDecision::
            no_locator_budget_exhausted;
    return locator;
  }
  locator.budget_preflight_certified_ = true;

  locator.slots_.resize(*table_slot_capacity);
  locator.component_parents_.resize(component_handle_count);
  std::iota(
      locator.component_parents_.begin(),
      locator.component_parents_.end(),
      ExactDirectSparseComponentHandle{0U});
  locator.empty_table_initialized_ = true;
  locator.dense_component_handles_initialized_ = true;
  locator.flat_durable_key_arena_initialized_ = true;
  locator.positive_bindings_only_ = true;
  locator.full_key_comparison_required_ = true;
  locator.committed_history_digest_ = initial_locator_snapshot_digest(
      component_handle_count, budget, config);
  locator.initialization_decision_ =
      ExactDirectSparsePositiveFacetLocatorInitializationDecision::
          complete_certified_empty_sparse_positive_locator;
  return locator;
}

ExactDirectSparsePositiveFacetLocatorStateView
ExactDirectSparsePositiveFacetLocator::state_view() const noexcept {
  return {
      schema_version_,
      budget_,
      config_,
      required_component_handle_capacity_,
      required_table_slot_capacity_,
      required_batch_scratch_slot_capacity_,
      slots_,
      key_point_arena_,
      component_parents_,
      committed_unions_,
      committed_batches_,
      counters_,
      committed_history_digest_,
      budget_preflight_certified_,
      empty_table_initialized_,
      dense_component_handles_initialized_,
      flat_durable_key_arena_initialized_,
      positive_bindings_only_,
      full_key_comparison_required_,
      missing_facet_means_isolated_,
      total_facet_authority_claimed_,
      forbidden_global_structure_materialized_,
      public_status_claimed_,
      initialization_decision_,
      scope_};
}

ExactDirectSparsePositiveFacetLocatorPrefixStampSweepResult
build_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
    std::span<const std::size_t>
        nondecreasing_committed_batch_prefix_counts,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparsePositiveFacetLocatorPrefixStampSweepBudget&
        budget) {
  using Decision =
      ExactDirectSparsePositiveFacetLocatorPrefixStampSweepDecision;
  ExactDirectSparsePositiveFacetLocatorPrefixStampSweepResult result;
  result.requested_budget = budget;
  result.prefix_request_count =
      nondecreasing_committed_batch_prefix_counts.size();
  result.scope =
      ExactDirectSparsePositiveFacetLocatorPrefixStampSweepScope::
          locator_internal_committed_batch_prefix_stamps_relative_to_frozen_history_only;
  result.no_partial_scientific_payload_published = true;
  result.partial_refinement_only = true;

  bool entry_snapshot_captured = false;
  const auto fail = [&](Decision decision) {
    result.prefix_stamps.clear();
    result.no_partial_scientific_payload_published = true;
    result.decision = decision;
    if (entry_snapshot_captured &&
        result.counters.locator_snapshot_check_count == 1U) {
      ++result.counters.locator_snapshot_check_count;
      result.common_frozen_locator_snapshot_certified =
          locator.snapshot_stamp() == result.locator_snapshot_stamp;
    }
    return result;
  };

  result.locator_certified_at_entry = locator.certified_positive_locator();
  if (!result.locator_certified_at_entry) {
    return fail(Decision::no_prefix_stamp_locator_not_certified);
  }
  result.locator_snapshot_stamp = locator.snapshot_stamp();
  ++result.counters.locator_snapshot_check_count;
  entry_snapshot_captured = true;
  const ExactDirectSparsePositiveFacetLocatorStateView state =
      locator.state_view();

  if (result.prefix_request_count >
      budget.maximum_prefix_request_count) {
    return fail(Decision::no_prefix_stamp_budget_exhausted);
  }
  std::size_t previous_prefix = 0U;
  for (std::size_t request_index = 0U;
       request_index < nondecreasing_committed_batch_prefix_counts.size();
       ++request_index) {
    const std::size_t prefix =
        nondecreasing_committed_batch_prefix_counts[request_index];
    ++result.counters.prefix_request_scan_count;
    if (prefix > state.committed_batches.size() ||
        (request_index != 0U && prefix < previous_prefix)) {
      return fail(Decision::no_prefix_stamp_input_shape_rejected);
    }
    previous_prefix = prefix;
  }
  result.prefix_requests_nondecreasing_and_in_history = true;
  result.required_committed_batch_prefix_count =
      nondecreasing_committed_batch_prefix_counts.empty()
          ? 0U
          : nondecreasing_committed_batch_prefix_counts.back();
  const auto required_batch_record_scan_count = checked_multiply(
      2U, result.required_committed_batch_prefix_count);
  if (!required_batch_record_scan_count.has_value()) {
    return fail(Decision::no_prefix_stamp_capacity_overflow);
  }
  result.required_batch_record_scan_count =
      *required_batch_record_scan_count;
  if (result.required_batch_record_scan_count >
      budget.maximum_batch_record_scan_count) {
    return fail(Decision::no_prefix_stamp_budget_exhausted);
  }

  std::size_t active_binding_prefix_count = 0U;
  std::size_t union_record_replay_count = 0U;
  std::size_t key_point_replay_count = 0U;
  for (std::size_t batch_index = 0U;
       batch_index < result.required_committed_batch_prefix_count;
       ++batch_index) {
    ++result.counters.batch_record_scan_count;
    const ExactDirectSparseCommittedBatchRecord& record =
        state.committed_batches[batch_index];
    if (record.committed_batch_index != batch_index) {
      return fail(Decision::no_prefix_stamp_locator_history_rejected);
    }
    const auto next_binding_prefix = checked_add(
        active_binding_prefix_count,
        record.counters.inserted_binding_count);
    const auto next_union_prefix = checked_add(
        union_record_replay_count,
        record.counters.union_request_count);
    const auto next_key_point_count = checked_add(
        key_point_replay_count,
        record.counters.inserted_key_point_count);
    if (!next_binding_prefix.has_value() ||
        !next_union_prefix.has_value() ||
        !next_key_point_count.has_value()) {
      return fail(Decision::no_prefix_stamp_capacity_overflow);
    }
    active_binding_prefix_count = *next_binding_prefix;
    union_record_replay_count = *next_union_prefix;
    key_point_replay_count = *next_key_point_count;
  }
  result.required_active_binding_prefix_count = active_binding_prefix_count;
  result.required_union_record_replay_count = union_record_replay_count;
  result.required_key_point_replay_count = key_point_replay_count;
  result.required_table_slot_scan_count =
      active_binding_prefix_count == 0U ? 0U : state.slots.size();
  const auto required_temporary_scratch_byte_count = checked_multiply(
      active_binding_prefix_count, sizeof(std::size_t));
  if (!required_temporary_scratch_byte_count.has_value()) {
    return fail(Decision::no_prefix_stamp_capacity_overflow);
  }
  result.required_temporary_scratch_byte_count =
      *required_temporary_scratch_byte_count;
  const bool replaying_final_history =
      result.required_committed_batch_prefix_count ==
      state.committed_batches.size();
  if (active_binding_prefix_count > state.counters.inserted_binding_count ||
      union_record_replay_count > state.committed_unions.size() ||
      key_point_replay_count > state.key_point_arena.size()) {
    return fail(Decision::no_prefix_stamp_locator_history_rejected);
  }
  if (replaying_final_history &&
      (active_binding_prefix_count !=
           state.counters.inserted_binding_count ||
       union_record_replay_count != state.counters.union_request_count ||
       key_point_replay_count !=
           state.counters.committed_key_point_count)) {
    return fail(Decision::no_prefix_stamp_locator_history_rejected);
  }
  if (result.required_table_slot_scan_count >
          budget.maximum_table_slot_scan_count ||
      active_binding_prefix_count >
          budget.maximum_binding_slot_index_scratch_count ||
      union_record_replay_count >
          budget.maximum_union_record_replay_count ||
      active_binding_prefix_count >
          budget.maximum_binding_record_replay_count ||
      key_point_replay_count > budget.maximum_key_point_replay_count ||
      result.required_temporary_scratch_byte_count >
          budget.maximum_temporary_scratch_byte_count) {
    return fail(Decision::no_prefix_stamp_budget_exhausted);
  }
  result.budget_preflight_certified = true;

  const std::size_t missing_slot = state.slots.size();
  std::vector<std::size_t> binding_slot_indices_by_index(
      active_binding_prefix_count, missing_slot);
  if (active_binding_prefix_count != 0U) {
    for (std::size_t slot_index = 0U;
         slot_index < state.slots.size();
         ++slot_index) {
      ++result.counters.table_slot_scan_count;
      const ExactDirectSparsePositiveFacetSlot& slot =
          state.slots[slot_index];
      if (!slot.occupied) {
        continue;
      }
      if (slot.committed_binding_index >= active_binding_prefix_count) {
        if (replaying_final_history) {
          return fail(Decision::no_prefix_stamp_locator_history_rejected);
        }
        continue;
      }
      if (binding_slot_indices_by_index[slot.committed_binding_index] !=
              missing_slot ||
          slot.key_point_count == 0U ||
          slot.key_point_count >
              direct_sparse_positive_facet_maximum_point_count ||
          slot.key_point_offset > state.key_point_arena.size() ||
          slot.key_point_count >
              state.key_point_arena.size() - slot.key_point_offset) {
        return fail(Decision::no_prefix_stamp_locator_history_rejected);
      }
      binding_slot_indices_by_index[slot.committed_binding_index] =
          slot_index;
    }
    if (!std::all_of(
            binding_slot_indices_by_index.begin(),
            binding_slot_indices_by_index.end(),
            [missing_slot](std::size_t slot_index) {
              return slot_index != missing_slot;
            })) {
      return fail(Decision::no_prefix_stamp_locator_history_rejected);
    }
  }
  result.active_binding_slots_indexed_once = true;

  std::vector<ExactDirectSparsePositiveFacetLocatorSnapshotStamp>
      prefix_stamps;
  prefix_stamps.reserve(result.prefix_request_count);
  LocatorHistoryReplayCursor history_cursor{
      {},
      0U,
      0U,
      initial_locator_snapshot_digest(
          state.component_parents.size(), state.budget, state.config)};
  std::size_t next_request_index = 0U;
  const auto emit_current_prefix = [&]() {
    while (next_request_index <
               nondecreasing_committed_batch_prefix_counts.size() &&
           nondecreasing_committed_batch_prefix_counts[next_request_index] ==
               history_cursor.counters.committed_batch_count) {
      prefix_stamps.push_back(make_locator_snapshot_stamp(
          state.schema_version,
          state.config.external_authority_id,
          history_cursor.counters,
          history_cursor.history_digest));
      ++next_request_index;
      ++result.counters.emitted_stamp_count;
    }
  };
  emit_current_prefix();
  for (std::size_t batch_index = 0U;
       batch_index < result.required_committed_batch_prefix_count;
       ++batch_index) {
    ++result.counters.batch_record_scan_count;
    const LocatorHistoryTransitionStatus transition =
        replay_locator_history_digest_transition(
            batch_index,
            state.committed_batches[batch_index],
            state.component_parents.size(),
            state.budget,
            state.config.external_authority_id,
            state.committed_unions,
            state.slots,
            state.key_point_arena,
            binding_slot_indices_by_index,
            history_cursor);
    if (transition == LocatorHistoryTransitionStatus::capacity_overflow) {
      return fail(Decision::no_prefix_stamp_capacity_overflow);
    }
    if (transition != LocatorHistoryTransitionStatus::complete) {
      return fail(Decision::no_prefix_stamp_locator_history_rejected);
    }
    emit_current_prefix();
  }
  result.counters.union_record_replay_count = history_cursor.union_prefix;
  result.counters.binding_record_replay_count =
      history_cursor.binding_prefix;
  result.counters.key_point_replay_count =
      history_cursor.counters.committed_key_point_count;
  if (history_cursor.counters.committed_batch_count !=
          result.required_committed_batch_prefix_count ||
      history_cursor.binding_prefix != active_binding_prefix_count ||
      history_cursor.union_prefix != union_record_replay_count ||
      history_cursor.counters.committed_key_point_count !=
          key_point_replay_count ||
      next_request_index != result.prefix_request_count) {
    return fail(Decision::no_prefix_stamp_locator_history_rejected);
  }
  result.every_requested_batch_preflighted_and_replayed_once =
      result.counters.batch_record_scan_count ==
      result.required_batch_record_scan_count;
  result.every_union_binding_and_key_point_replayed_once = true;
  result.every_requested_prefix_stamp_emitted_once =
      prefix_stamps.size() == result.prefix_request_count;
  result.final_prefix_matches_live_locator_when_requested = true;
  if (!nondecreasing_committed_batch_prefix_counts.empty() &&
      replaying_final_history &&
      (prefix_stamps.empty() ||
       prefix_stamps.back() != result.locator_snapshot_stamp)) {
    result.final_prefix_matches_live_locator_when_requested = false;
    return fail(Decision::no_prefix_stamp_locator_history_rejected);
  }

  ++result.counters.locator_snapshot_check_count;
  result.common_frozen_locator_snapshot_certified =
      locator.snapshot_stamp() == result.locator_snapshot_stamp;
  if (!result.common_frozen_locator_snapshot_certified) {
    return fail(Decision::no_prefix_stamp_locator_history_rejected);
  }
  result.prefix_stamps = std::move(prefix_stamps);
  result.decision =
      Decision::complete_certified_locator_prefix_stamps;
  if (!result.certified_partial_refinement()) {
    throw std::logic_error(
        "a complete locator prefix-stamp sweep failed its contract");
  }
  return result;
}

ExactDirectSparsePositiveFacetLocatorPrefixStampSweepVerification
verify_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
    std::span<const std::size_t>
        nondecreasing_committed_batch_prefix_counts,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparsePositiveFacetLocatorPrefixStampSweepBudget& budget,
    const ExactDirectSparsePositiveFacetLocatorPrefixStampSweepResult&
        observed) {
  ExactDirectSparsePositiveFacetLocatorPrefixStampSweepVerification
      verification;
  verification.observed_storage_within_budget =
      observed.prefix_stamps.size() <= budget.maximum_prefix_request_count &&
      observed.prefix_stamps.size() <=
          nondecreasing_committed_batch_prefix_counts.size();
  const ExactDirectSparsePositiveFacetLocatorSnapshotStamp entry_stamp =
      locator.snapshot_stamp();
  verification.locator_snapshot_matches_observed_build =
      observed.locator_snapshot_stamp == entry_stamp;
  ExactDirectSparsePositiveFacetLocatorPrefixStampSweepResult expected;
  if (verification.observed_storage_within_budget &&
      verification.locator_snapshot_matches_observed_build) {
    expected =
        build_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
            nondecreasing_committed_batch_prefix_counts, locator, budget);
    verification.expected_sweep_freshly_rebuilt =
        expected.certified_outcome();
    verification.counters_and_stamps_freshly_replayed =
        expected == observed;
  }
  verification.no_locator_mutation_or_batch_commit =
      entry_stamp == locator.snapshot_stamp() &&
      !observed.locator_state_mutated && !observed.locator_batch_committed &&
      !expected.locator_state_mutated && !expected.locator_batch_committed;
  verification.external_authority_replayed_by_locator = false;
  verification.no_forbidden_global_structure_materialized =
      !observed.forbidden_global_structure_materialized &&
      !expected.forbidden_global_structure_materialized;
  verification.fresh_replay_certified =
      verification.observed_storage_within_budget &&
      verification.locator_snapshot_matches_observed_build &&
      verification.expected_sweep_freshly_rebuilt &&
      verification.counters_and_stamps_freshly_replayed &&
      verification.no_locator_mutation_or_batch_commit &&
      !verification.external_authority_replayed_by_locator &&
      verification.no_forbidden_global_structure_materialized;
  verification.result_certified =
      verification.fresh_replay_certified && observed.certified_outcome();
  return verification;
}

ExactDirectSparsePositiveFacetLocatorStructuralVerification
verify_exact_direct_sparse_positive_facet_locator_structure(
    std::size_t trusted_component_handle_count,
    const ExactDirectSparsePositiveFacetLocatorBudget& trusted_budget,
    const ExactDirectSparsePositiveFacetLocatorConfig& trusted_config,
    const ExactDirectSparsePositiveFacetLocatorStructuralVerificationBudget&
        verification_budget,
    const ExactDirectSparsePositiveFacetLocatorStateView& observed) {
  ExactDirectSparsePositiveFacetLocatorStructuralVerification verification;
  verification.requested_budget = verification_budget;
  verification.required_table_slot_count = observed.slots.size();
  verification.required_key_point_count = observed.key_point_arena.size();
  verification.required_component_parent_count =
      observed.component_parents.size();
  verification.required_union_record_count =
      observed.committed_unions.size();
  verification.required_batch_record_count =
      observed.committed_batches.size();
  verification.required_binding_scratch_entry_count =
      observed.counters.inserted_binding_count;
  verification.required_key_point_scratch_entry_count =
      observed.key_point_arena.size();
  verification.required_table_slot_scratch_entry_count =
      observed.slots.size();
  verification.required_component_parent_scratch_entry_count =
      trusted_component_handle_count;
  verification.external_authority_replayed_by_locator = false;

  verification.trusted_construction_parameters_certified =
      observed.schema_version ==
          direct_sparse_positive_facet_locator_schema_version &&
      observed.budget == trusted_budget &&
      observed.config == trusted_config &&
      trusted_config.external_authority_id != 0U;

  const auto table_slot_capacity =
      probing_slot_capacity(trusted_budget.maximum_committed_binding_count);
  const auto batch_scratch_slot_capacity =
      probing_slot_capacity(trusted_budget.maximum_batch_binding_count);
  verification.capacity_requirements_certified =
      table_slot_capacity.has_value() &&
      batch_scratch_slot_capacity.has_value() &&
      trusted_component_handle_count <=
          trusted_budget.maximum_component_handle_count &&
      *table_slot_capacity <= trusted_budget.maximum_table_slot_count &&
      *batch_scratch_slot_capacity <=
          trusted_budget.maximum_batch_scratch_slot_count &&
      observed.required_component_handle_capacity ==
          trusted_component_handle_count &&
      observed.required_table_slot_capacity == *table_slot_capacity &&
      observed.required_batch_scratch_slot_capacity ==
          *batch_scratch_slot_capacity &&
      observed.slots.size() == *table_slot_capacity &&
      observed.component_parents.size() == trusted_component_handle_count &&
      observed.counters.inserted_binding_count <=
          trusted_budget.maximum_committed_binding_count &&
      observed.key_point_arena.size() <=
          trusted_budget.maximum_committed_key_point_count &&
      observed.committed_unions.size() <=
          trusted_budget.maximum_committed_union_count &&
      observed.committed_batches.size() <=
          trusted_budget.maximum_committed_batch_count;
  if (!verification.trusted_construction_parameters_certified) {
    verification.structure_contract_rejected = true;
    verification.decision =
        ExactDirectSparsePositiveFacetLocatorStructuralVerificationDecision::
            no_verification_trusted_contract_rejected;
    return verification;
  }
  if (!verification.capacity_requirements_certified) {
    verification.structure_contract_rejected = true;
    verification.decision =
        ExactDirectSparsePositiveFacetLocatorStructuralVerificationDecision::
            no_verification_capacity_requirements_rejected;
    return verification;
  }

  const auto required_scratch_bytes =
      structural_verification_scratch_byte_count(
          verification.required_binding_scratch_entry_count,
          verification.required_key_point_scratch_entry_count,
          verification.required_table_slot_scratch_entry_count,
          verification.required_component_parent_scratch_entry_count);
  if (!required_scratch_bytes.has_value()) {
    verification.structure_contract_rejected = true;
    verification.decision =
        ExactDirectSparsePositiveFacetLocatorStructuralVerificationDecision::
            no_verification_scratch_requirement_overflow;
    return verification;
  }
  verification.scratch_requirement_arithmetic_certified = true;
  verification.required_temporary_scratch_byte_count =
      *required_scratch_bytes;
  verification.budget_preflight_certified =
      verification.required_table_slot_count <=
          verification_budget.maximum_table_slot_count &&
      verification.required_key_point_count <=
          verification_budget.maximum_key_point_count &&
      verification.required_component_parent_count <=
          verification_budget.maximum_component_parent_count &&
      verification.required_union_record_count <=
          verification_budget.maximum_union_record_count &&
      verification.required_batch_record_count <=
          verification_budget.maximum_batch_record_count &&
      verification.required_binding_scratch_entry_count <=
          verification_budget.maximum_binding_scratch_entry_count &&
      verification.required_key_point_scratch_entry_count <=
          verification_budget.maximum_key_point_scratch_entry_count &&
      verification.required_table_slot_scratch_entry_count <=
          verification_budget.maximum_table_slot_scratch_entry_count &&
      verification.required_component_parent_scratch_entry_count <=
          verification_budget
              .maximum_component_parent_scratch_entry_count &&
      verification.required_temporary_scratch_byte_count <=
          verification_budget.maximum_temporary_scratch_byte_count;
  if (!verification.budget_preflight_certified) {
    verification.budget_exhausted = true;
    verification.decision =
        ExactDirectSparsePositiveFacetLocatorStructuralVerificationDecision::
            no_verification_budget_preflight_exhausted;
    return verification;
  }
  verification.bounded_temporary_scratch_without_second_durable_output = true;

  std::size_t occupied_slot_count = 0U;
  std::vector<std::uint8_t> seen_binding_indices(
      verification.required_binding_scratch_entry_count,
      std::uint8_t{0U});
  std::vector<std::size_t> binding_key_point_counts(
      verification.required_binding_scratch_entry_count, 0U);
  std::vector<std::size_t> binding_slot_indices_by_index(
      verification.required_binding_scratch_entry_count,
      observed.slots.size());
  std::vector<std::uint8_t> seen_key_points(
      observed.key_point_arena.size(), std::uint8_t{0U});
  bool table_and_arena_valid = true;
  bool fingerprints_and_locations_valid = table_and_arena_valid;
  for (std::size_t slot_index = 0U;
       slot_index < observed.slots.size();
       ++slot_index) {
    ++verification.table_slot_scan_count;
    const ExactDirectSparsePositiveFacetSlot& slot =
        observed.slots[slot_index];
    if (!slot.occupied) {
      table_and_arena_valid =
          table_and_arena_valid &&
          slot == ExactDirectSparsePositiveFacetSlot{};
      continue;
    }
    ++occupied_slot_count;
    if (slot.committed_binding_index >=
            verification.required_binding_scratch_entry_count ||
        seen_binding_indices[slot.committed_binding_index] != 0U ||
        slot.component_handle >= trusted_component_handle_count ||
        !witness_matches_authority(
            slot.binding_witness,
            trusted_config.external_authority_id) ||
        slot.key_point_count == 0U ||
        slot.key_point_count >
            direct_sparse_positive_facet_maximum_point_count ||
        slot.key_point_offset > observed.key_point_arena.size() ||
        slot.key_point_count >
            observed.key_point_arena.size() - slot.key_point_offset) {
      table_and_arena_valid = false;
      fingerprints_and_locations_valid = false;
      continue;
    }
    seen_binding_indices[slot.committed_binding_index] = 1U;
    binding_key_point_counts[slot.committed_binding_index] =
        slot.key_point_count;
    binding_slot_indices_by_index[slot.committed_binding_index] =
        slot_index;

    ExactDirectSparseFacetKey reconstructed_key;
    reconstructed_key.point_count = slot.key_point_count;
    for (std::size_t point_index = 0U;
         point_index < slot.key_point_count;
         ++point_index) {
      const std::size_t arena_index = slot.key_point_offset + point_index;
      if (seen_key_points[arena_index] != 0U) {
        table_and_arena_valid = false;
      }
      ++verification.key_point_scan_count;
      seen_key_points[arena_index] = 1U;
      reconstructed_key.point_ids[point_index] =
          observed.key_point_arena[arena_index];
    }
    if (!canonical_key_shape(reconstructed_key)) {
      table_and_arena_valid = false;
      fingerprints_and_locations_valid = false;
      continue;
    }
    const std::uint64_t expected_fingerprint =
        fingerprint_exact_direct_sparse_facet_key(
            reconstructed_key, trusted_config.fingerprint_mask);
    const std::size_t remaining_fingerprint_slot_visits =
        verification_budget.maximum_fingerprint_search_slot_visit_count -
        verification.fingerprint_search_slot_visit_count;
    const SlotSearchResult located = search_committed_slots(
        observed.slots,
        observed.key_point_arena,
        reconstructed_key,
        expected_fingerprint,
        remaining_fingerprint_slot_visits);
    verification.fingerprint_search_slot_visit_count +=
        located.slot_visit_count;
    if (located.slot_visit_budget_exhausted) {
      verification.fingerprint_search_budget_exhausted = true;
      verification.budget_exhausted = true;
      verification.decision =
          ExactDirectSparsePositiveFacetLocatorStructuralVerificationDecision::
              no_verification_fingerprint_search_budget_exhausted;
      return verification;
    }
    if (slot.fingerprint != expected_fingerprint ||
        !located.matching_slot.has_value() ||
        *located.matching_slot != slot_index ||
        located.full_key_comparison_count == 0U) {
      fingerprints_and_locations_valid = false;
    }
  }
  table_and_arena_valid =
      table_and_arena_valid &&
      std::all_of(
          seen_binding_indices.begin(),
          seen_binding_indices.end(),
          [](std::uint8_t seen) { return seen == 1U; }) &&
      std::all_of(
          seen_key_points.begin(),
          seen_key_points.end(),
          [](std::uint8_t seen) { return seen == 1U; }) &&
      occupied_slot_count == observed.counters.inserted_binding_count &&
      observed.key_point_arena.size() ==
          observed.counters.committed_key_point_count;
  verification.flat_table_and_key_arena_certified = table_and_arena_valid;
  verification.every_fingerprint_recomputed_and_full_key_located =
      table_and_arena_valid && fingerprints_and_locations_valid;

  // Recreate the insert-only table in committed-binding order.  The binary
  // scratch records only historical occupancy; the durable table supplies
  // keys, fingerprints and physical destinations.  If L slots are visited
  // across all linear probes, this replay costs O(table slots + L) time and
  // O(table slots) bytes without copying the durable slots or key arena.
  std::vector<std::uint8_t> replayed_slot_occupancy(
      observed.slots.size(), std::uint8_t{0U});
  bool committed_slot_insertion_chronology_valid =
      verification.every_fingerprint_recomputed_and_full_key_located;
  if (committed_slot_insertion_chronology_valid) {
    for (std::size_t binding_index = 0U;
         binding_index < binding_slot_indices_by_index.size();
         ++binding_index) {
      const std::size_t committed_slot_index =
          binding_slot_indices_by_index[binding_index];
      if (committed_slot_index >= observed.slots.size()) {
        committed_slot_insertion_chronology_valid = false;
        break;
      }
      const ExactDirectSparsePositiveFacetSlot& committed_slot =
          observed.slots[committed_slot_index];
      std::size_t replayed_slot_index = static_cast<std::size_t>(
          committed_slot.fingerprint % observed.slots.size());
      bool empty_slot_reached = false;
      for (std::size_t probe = 0U;
           probe < observed.slots.size();
           ++probe) {
        if (verification.insertion_chronology_slot_visit_count >=
            verification_budget
                .maximum_insertion_chronology_slot_visit_count) {
          verification.insertion_chronology_budget_exhausted = true;
          verification.budget_exhausted = true;
          verification.decision =
              ExactDirectSparsePositiveFacetLocatorStructuralVerificationDecision::
                  no_verification_insertion_chronology_budget_exhausted;
          return verification;
        }
        ++verification.insertion_chronology_slot_visit_count;
        if (replayed_slot_occupancy[replayed_slot_index] == 0U) {
          empty_slot_reached = true;
          if (replayed_slot_index != committed_slot_index) {
            committed_slot_insertion_chronology_valid = false;
          } else {
            replayed_slot_occupancy[replayed_slot_index] = 1U;
          }
          break;
        }
        replayed_slot_index =
            (replayed_slot_index + 1U) % observed.slots.size();
      }
      if (!empty_slot_reached ||
          !committed_slot_insertion_chronology_valid) {
        committed_slot_insertion_chronology_valid = false;
        break;
      }
    }
  }
  verification.committed_slot_insertion_chronology_freshly_replayed =
      committed_slot_insertion_chronology_valid;

  std::vector<ExactDirectSparseComponentHandle> replayed_parents(
      trusted_component_handle_count);
  std::iota(
      replayed_parents.begin(),
      replayed_parents.end(),
      ExactDirectSparseComponentHandle{0U});
  bool union_records_valid =
      observed.committed_unions.size() <=
      trusted_budget.maximum_committed_union_count;
  for (std::size_t index = 0U;
       index < observed.committed_unions.size();
       ++index) {
    ++verification.union_record_scan_count;
    const ExactDirectSparseCommittedUnionRecord& record =
        observed.committed_unions[index];
    if (record.committed_union_index != index ||
        record.left_handle >= trusted_component_handle_count ||
        record.right_handle >= trusted_component_handle_count ||
        !witness_matches_authority(
            record.witness, trusted_config.external_authority_id)) {
      union_records_valid = false;
      continue;
    }
    const StructuralUnionReplayStatus replay_status =
        unite_components_for_structure(
            replayed_parents,
            record.left_handle,
            record.right_handle,
            verification_budget.maximum_union_parent_hop_count,
            verification.union_parent_hop_count);
    if (replay_status == StructuralUnionReplayStatus::budget_exhausted) {
      verification.union_parent_hop_budget_exhausted = true;
      verification.budget_exhausted = true;
      verification.decision =
          ExactDirectSparsePositiveFacetLocatorStructuralVerificationDecision::
              no_verification_union_parent_hop_budget_exhausted;
      return verification;
    }
    if (replay_status ==
        StructuralUnionReplayStatus::invalid_structure) {
      union_records_valid = false;
    }
  }
  bool observed_parents_valid =
      observed.component_parents.size() == trusted_component_handle_count;
  for (const ExactDirectSparseComponentHandle parent :
       observed.component_parents) {
    observed_parents_valid =
        observed_parents_valid && parent < trusted_component_handle_count;
  }
  verification.union_witness_structure_certified = union_records_valid;
  verification.dense_handle_dsu_replay_certified =
      union_records_valid && observed_parents_valid &&
      std::equal(
          replayed_parents.begin(),
          replayed_parents.end(),
          observed.component_parents.begin(),
          observed.component_parents.end());

  LocatorHistoryReplayCursor history_cursor{
      {},
      0U,
      0U,
      initial_locator_snapshot_digest(
          trusted_component_handle_count, trusted_budget, trusted_config)};
  const auto reject_malformed_durable_history = [&verification]() noexcept {
    verification.structure_contract_rejected = true;
    verification.decision =
        ExactDirectSparsePositiveFacetLocatorStructuralVerificationDecision::
            no_verification_durable_structure_rejected;
  };
  for (std::size_t index = 0U;
       index < observed.committed_batches.size();
       ++index) {
    ++verification.batch_record_scan_count;
    const ExactDirectSparseCommittedBatchRecord& record =
        observed.committed_batches[index];
    const LocatorHistoryTransitionStatus transition =
        replay_locator_history_digest_transition(
            index,
            record,
            trusted_component_handle_count,
            trusted_budget,
            trusted_config.external_authority_id,
            observed.committed_unions,
            observed.slots,
            observed.key_point_arena,
            binding_slot_indices_by_index,
            history_cursor);
    if (transition != LocatorHistoryTransitionStatus::complete) {
      reject_malformed_durable_history();
      return verification;
    }
  }
  verification.historical_batch_assertions_and_counters_well_formed =
      history_cursor.counters == observed.counters &&
      observed.counters.union_request_count ==
          observed.committed_unions.size() &&
      history_cursor.union_prefix == observed.committed_unions.size() &&
      observed.counters.inserted_binding_count == occupied_slot_count &&
      history_cursor.binding_prefix == occupied_slot_count &&
      observed.counters.committed_key_point_count ==
          observed.key_point_arena.size();
  verification.committed_history_digest_freshly_replayed =
      history_cursor.history_digest == observed.committed_history_digest;

  verification.internal_fact_fields_match_contract =
      observed.budget_preflight_certified &&
      observed.empty_table_initialized &&
      observed.dense_component_handles_initialized &&
      observed.flat_durable_key_arena_initialized &&
      observed.positive_bindings_only &&
      observed.full_key_comparison_required &&
      !observed.missing_facet_means_isolated &&
      !observed.total_facet_authority_claimed &&
      !observed.forbidden_global_structure_materialized &&
      !observed.public_status_claimed;
  verification.decision_and_scope_certified =
      observed.initialization_decision ==
          ExactDirectSparsePositiveFacetLocatorInitializationDecision::
              complete_certified_empty_sparse_positive_locator &&
      observed.scope == ExactDirectSparsePositiveFacetLocatorScope::
                            positive_bindings_relative_to_caller_asserted_external_authority_only;
  verification.fresh_durable_structure_verification_certified =
      verification.trusted_construction_parameters_certified &&
      verification.capacity_requirements_certified &&
      verification.scratch_requirement_arithmetic_certified &&
      verification.budget_preflight_certified &&
      !verification.budget_exhausted &&
      verification.fingerprint_search_slot_visit_count <=
          verification_budget
              .maximum_fingerprint_search_slot_visit_count &&
      verification.insertion_chronology_slot_visit_count <=
          verification_budget
              .maximum_insertion_chronology_slot_visit_count &&
      verification.union_parent_hop_count <=
          verification_budget.maximum_union_parent_hop_count &&
      verification.flat_table_and_key_arena_certified &&
      verification.every_fingerprint_recomputed_and_full_key_located &&
      verification
          .committed_slot_insertion_chronology_freshly_replayed &&
      verification.dense_handle_dsu_replay_certified &&
      verification.union_witness_structure_certified &&
      verification.historical_batch_assertions_and_counters_well_formed &&
      verification.committed_history_digest_freshly_replayed &&
      verification.internal_fact_fields_match_contract &&
      verification.decision_and_scope_certified &&
      verification
          .bounded_temporary_scratch_without_second_durable_output;
  verification.result_certified =
      verification.fresh_durable_structure_verification_certified &&
      !verification.external_authority_replayed_by_locator;
  if (verification.result_certified) {
    verification.decision =
        ExactDirectSparsePositiveFacetLocatorStructuralVerificationDecision::
            complete_certified_durable_structure_verification;
  } else {
    verification.structure_contract_rejected = true;
    verification.decision =
        ExactDirectSparsePositiveFacetLocatorStructuralVerificationDecision::
            no_verification_durable_structure_rejected;
  }
  return verification;
}

ExactDirectSparsePositiveFacetBatchResult
ExactDirectSparsePositiveFacetLocator::apply_batch(
    std::span<const ExactDirectSparseFacetQuery> queries,
    std::span<const ExactDirectSparseComponentUnion> unions,
    std::span<const ExactDirectSparseFacetBinding> bindings) {
  ExactDirectSparsePositiveFacetBatchResult result;
  result.candidate_batch_index = counters_.committed_batch_count;
  result.scope = scope_;
  if (!certified_positive_locator()) {
    result.decision = ExactDirectSparsePositiveFacetBatchDecision::
        no_positive_locator_not_initialized;
    return result;
  }

  const BatchRequirements requirements = analyze_batch_requirements(
      queries,
      unions,
      bindings,
      component_parents_.size(),
      config_.external_authority_id);
  if (!requirements.capacity_safe) {
    result.decision = ExactDirectSparsePositiveFacetBatchDecision::
        no_positive_locator_capacity_overflow;
    return result;
  }
  if (!requirements.input_shape_certified) {
    result.decision = ExactDirectSparsePositiveFacetBatchDecision::
        no_positive_locator_input_shape_rejected;
    return result;
  }
  result.input_shape_certified = true;
  if (!requirements.witnesses_certified) {
    result.decision = ExactDirectSparsePositiveFacetBatchDecision::
        no_positive_locator_external_witness_rejected;
    return result;
  }
  result.every_input_witness_non_null_and_authority_matched = true;

  if (!batch_budget_is_sufficient(
          budget_,
          counters_,
          committed_unions_.size(),
          requirements,
          queries.size(),
          unions.size(),
          bindings.size())) {
    result.decision = ExactDirectSparsePositiveFacetBatchDecision::
        no_positive_locator_budget_exhausted;
    return result;
  }
  result.budget_preflight_certified = true;
  result.counters.query_count = queries.size();
  result.counters.union_request_count = unions.size();
  result.counters.binding_request_count = bindings.size();
  result.counters.batch_input_key_point_count =
      requirements.key_point_count;

  result.lookups.reserve(queries.size());
  for (const ExactDirectSparseFacetQuery& query : queries) {
    ExactDirectSparseFacetLookupResult lookup;
    lookup.query_index = query.query_index;
    lookup.query_witness = query.witness;
    const std::uint64_t fingerprint =
        fingerprint_exact_direct_sparse_facet_key(
            query.key, config_.fingerprint_mask);
    const SlotSearchResult found = search_committed_slots(
        slots_, key_point_arena_, query.key, fingerprint);
    if (!accumulate_search_counters(
            result.counters,
            found.full_key_comparison_count,
            found.equal_fingerprint_distinct_key_count)) {
      result.decision = ExactDirectSparsePositiveFacetBatchDecision::
          no_positive_locator_capacity_overflow;
      return result;
    }
    if (found.matching_slot.has_value()) {
      const ExactDirectSparsePositiveFacetSlot& slot =
          slots_[*found.matching_slot];
      lookup.disposition = ExactDirectSparseFacetLookupDisposition::positive;
      lookup.pre_batch_component_handle =
          find_component(component_parents_, slot.component_handle);
      lookup.source_binding_witness = slot.binding_witness;
      lookup.component_handle_present = true;
      lookup.source_binding_witness_present = true;
      ++result.counters.positive_lookup_count;
    } else {
      lookup.disposition =
          ExactDirectSparseFacetLookupDisposition::unresolved;
      ++result.counters.unresolved_lookup_count;
    }
    result.lookups.push_back(lookup);
  }
  result.lookups_use_strict_pre_batch_snapshot = true;
  result.current_batch_bindings_hidden_from_lookups = true;
  result.every_positive_lookup_has_non_null_external_witness_tokens =
      std::all_of(
          result.lookups.begin(),
          result.lookups.end(),
          [this](const ExactDirectSparseFacetLookupResult& lookup) {
            return lookup.disposition ==
                       ExactDirectSparseFacetLookupDisposition::unresolved ||
                   (lookup.component_handle_present &&
                    lookup.source_binding_witness_present &&
                    witness_matches_authority(
                        lookup.query_witness,
                        config_.external_authority_id) &&
                    witness_matches_authority(
                        lookup.source_binding_witness,
                        config_.external_authority_id));
          });
  result.every_fingerprint_candidate_compared_by_full_key = true;

  std::vector<ExactDirectSparseComponentHandle> candidate_parents =
      component_parents_;
  for (const ExactDirectSparseComponentUnion& component_union : unions) {
    unite_components(
        candidate_parents,
        component_union.left_handle,
        component_union.right_handle);
  }
  result.explicit_unions_applied_before_binding_compatibility = true;

  const auto actual_scratch_capacity = probing_slot_capacity(bindings.size());
  if (!actual_scratch_capacity.has_value() ||
      *actual_scratch_capacity > required_batch_scratch_slot_capacity_) {
    result.decision = ExactDirectSparsePositiveFacetBatchDecision::
        no_positive_locator_capacity_overflow;
    return result;
  }
  const std::size_t empty = std::numeric_limits<std::size_t>::max();
  std::vector<std::size_t> pending_scratch_slots(
      *actual_scratch_capacity, empty);
  std::vector<PendingBinding> pending;
  pending.reserve(bindings.size());

  for (const ExactDirectSparseFacetBinding& binding : bindings) {
    const std::uint64_t fingerprint =
        fingerprint_exact_direct_sparse_facet_key(
            binding.key, config_.fingerprint_mask);
    const SlotSearchResult committed = search_committed_slots(
        slots_, key_point_arena_, binding.key, fingerprint);
    if (!accumulate_search_counters(
            result.counters,
            committed.full_key_comparison_count,
            committed.equal_fingerprint_distinct_key_count)) {
      result.decision = ExactDirectSparsePositiveFacetBatchDecision::
          no_positive_locator_capacity_overflow;
      return result;
    }
    if (committed.matching_slot.has_value()) {
      const ExactDirectSparsePositiveFacetSlot& slot =
          slots_[*committed.matching_slot];
      if (find_component(candidate_parents, slot.component_handle) !=
          find_component(candidate_parents, binding.component_handle)) {
        result.contradiction_detected = true;
        result.decision = ExactDirectSparsePositiveFacetBatchDecision::
            contradiction_incompatible_exact_facet_binding;
        return result;
      }
      ++result.counters.compatible_duplicate_binding_count;
      continue;
    }

    const PendingSearchResult staged = search_pending_bindings(
        pending_scratch_slots, pending, binding.key, fingerprint);
    if (!accumulate_search_counters(
            result.counters,
            staged.full_key_comparison_count,
            staged.equal_fingerprint_distinct_key_count)) {
      result.decision = ExactDirectSparsePositiveFacetBatchDecision::
          no_positive_locator_capacity_overflow;
      return result;
    }
    if (staged.pending_index.has_value()) {
      const PendingBinding& previous = pending[*staged.pending_index];
      if (find_component(candidate_parents, previous.component_handle) !=
          find_component(candidate_parents, binding.component_handle)) {
        result.contradiction_detected = true;
        result.decision = ExactDirectSparsePositiveFacetBatchDecision::
            contradiction_incompatible_exact_facet_binding;
        return result;
      }
      ++result.counters.compatible_duplicate_binding_count;
      continue;
    }

    pending.push_back(PendingBinding{
        binding.key,
        fingerprint,
        binding.component_handle,
        binding.witness});
    insert_pending_scratch_slot(
        pending_scratch_slots, pending, pending.size() - 1U);
  }
  result.exact_duplicate_bindings_compatible_after_explicit_unions = true;
  result.counters.inserted_binding_count = pending.size();
  std::size_t pending_index = 0U;
  for (const PendingBinding& binding : pending) {
    const auto next = checked_add(
        result.counters.inserted_key_point_count, binding.key.point_count);
    if (!next.has_value()) {
      result.decision = ExactDirectSparsePositiveFacetBatchDecision::
          no_positive_locator_capacity_overflow;
      return result;
    }
    result.counters.inserted_key_point_count = *next;
  }

  const auto committed_binding_total = checked_add(
      counters_.inserted_binding_count, pending.size());
  const auto committed_key_point_total = checked_add(
      key_point_arena_.size(), result.counters.inserted_key_point_count);
  const auto committed_union_total = checked_add(
      committed_unions_.size(), unions.size());
  const auto committed_batch_total = checked_add(
      committed_batches_.size(), 1U);
  if (!committed_binding_total.has_value() ||
      !committed_key_point_total.has_value() ||
      !committed_union_total.has_value() ||
      !committed_batch_total.has_value()) {
    result.decision = ExactDirectSparsePositiveFacetBatchDecision::
        no_positive_locator_capacity_overflow;
    return result;
  }
  if (*committed_binding_total >
          budget_.maximum_committed_binding_count ||
      *committed_key_point_total >
          budget_.maximum_committed_key_point_count) {
    result.decision = ExactDirectSparsePositiveFacetBatchDecision::
        no_positive_locator_budget_exhausted;
    return result;
  }

  const auto next_counters = updated_counters(counters_, result.counters);
  if (!next_counters.has_value()) {
    result.decision = ExactDirectSparsePositiveFacetBatchDecision::
        no_positive_locator_capacity_overflow;
    return result;
  }

  const ExactDirectSparseCommittedBatchRecord candidate_batch_record{
      counters_.committed_batch_count,
      result.counters,
      true,
      true,
      true,
      true};
  LocatorSnapshotChainDigestBuilder snapshot_digest_builder(
      committed_history_digest_, candidate_batch_record);
  for (const ExactDirectSparseComponentUnion& component_union : unions) {
    snapshot_digest_builder.component_union(
        component_union.left_handle,
        component_union.right_handle,
        component_union.witness);
  }
  for (const PendingBinding& binding : pending) {
    snapshot_digest_builder.binding(
        binding.key, binding.component_handle, binding.witness);
  }
  const contract::CanonicalId next_snapshot_digest =
      snapshot_digest_builder.finalize();

  // Every fallible allocation happens before the first logical state change.
  key_point_arena_.reserve(*committed_key_point_total);
  committed_unions_.reserve(*committed_union_total);
  committed_batches_.reserve(*committed_batch_total);
  std::copy(
      candidate_parents.begin(),
      candidate_parents.end(),
      component_parents_.begin());
  for (const ExactDirectSparseComponentUnion& component_union : unions) {
    committed_unions_.push_back(ExactDirectSparseCommittedUnionRecord{
        committed_unions_.size(),
        component_union.left_handle,
        component_union.right_handle,
        component_union.witness});
  }
  for (const PendingBinding& binding : pending) {
    const std::size_t slot_index =
        first_empty_committed_slot(slots_, binding.fingerprint);
    ExactDirectSparsePositiveFacetSlot& slot = slots_[slot_index];
    slot.fingerprint = binding.fingerprint;
    slot.committed_binding_index =
        counters_.inserted_binding_count + pending_index;
    slot.key_point_offset = key_point_arena_.size();
    slot.key_point_count = binding.key.point_count;
    slot.component_handle = binding.component_handle;
    slot.binding_witness = binding.witness;
    slot.occupied = true;
    key_point_arena_.insert(
        key_point_arena_.end(),
        binding.key.point_ids.begin(),
        binding.key.point_ids.begin() +
            static_cast<std::ptrdiff_t>(binding.key.point_count));
    ++pending_index;
  }
  committed_batches_.push_back(candidate_batch_record);
  counters_ = *next_counters;
  committed_history_digest_ = next_snapshot_digest;

  result.atomic_commit_performed = true;
  result.locator_state_mutated = true;
  result.decision = ExactDirectSparsePositiveFacetBatchDecision::
      complete_certified_sparse_positive_batch_commit;
  return result;
}

}  // namespace morsehgp3d::hierarchy
