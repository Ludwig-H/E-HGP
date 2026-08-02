#include "morsehgp3d/hierarchy/direct_sparse_stable_facet_forest.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <limits>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::string_view initial_digest_domain =
    "MorseHGP3D/phase15/stable-facet-forest/initial/v1/sha256/";
constexpr std::string_view batch_digest_domain =
    "MorseHGP3D/phase15/stable-facet-forest/batch/v1/sha256/";
constexpr std::string_view history_step_domain =
    "MorseHGP3D/phase15/stable-facet-forest/history/v1/sha256/";
constexpr std::string_view checkpoint_digest_domain =
    "MorseHGP3D/phase15/stable-facet-forest/checkpoint/v1/sha256/";

std::atomic<std::uint64_t> next_session_instance_id{1U};

[[nodiscard]] bool checked_add(
    std::size_t left, std::size_t right, std::size_t& result) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  result = left + right;
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

void append_key(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparseFacetKey& key) {
  append_size(builder, key.point_count);
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    append_u64(builder, static_cast<std::uint64_t>(key.point_ids[index]));
  }
}

[[nodiscard]] bool canonical_key(
    const ExactDirectSparseFacetKey& key) noexcept {
  if (key.point_count < 2U || key.point_count > key.point_ids.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    if (index != 0U && key.point_ids[index - 1U] >= key.point_ids[index]) {
      return false;
    }
  }
  return std::all_of(
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(key.point_count),
      key.point_ids.end(),
      [](spatial::PointId point_id) { return point_id == 0U; });
}

[[nodiscard]] bool key_less(
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

[[nodiscard]] bool insertion_less(
    const ExactDirectSparseStableFacetInsertion& left,
    const ExactDirectSparseStableFacetInsertion& right) noexcept {
  if (left.stable_source_facet_token_index !=
      right.stable_source_facet_token_index) {
    return left.stable_source_facet_token_index <
           right.stable_source_facet_token_index;
  }
  return key_less(left.facet_key, right.facet_key);
}

[[nodiscard]] bool union_less(
    const ExactDirectSparseStableFacetUnion& left,
    const ExactDirectSparseStableFacetUnion& right) noexcept {
  if (left.left_handle != right.left_handle) {
    return left.left_handle < right.left_handle;
  }
  return left.right_handle < right.right_handle;
}

[[nodiscard]] std::uint64_t allocate_session_instance_id() noexcept {
  std::uint64_t candidate =
      next_session_instance_id.load(std::memory_order_relaxed);
  while (candidate != 0U) {
    const std::uint64_t successor = candidate + 1U;
    if (next_session_instance_id.compare_exchange_weak(
            candidate,
            successor,
            std::memory_order_relaxed,
            std::memory_order_relaxed)) {
      return candidate;
    }
  }
  return 0U;
}

[[nodiscard]] contract::CanonicalId initial_digest(
    const ExactDirectSparseStableFacetForestConfig& config) {
  contract::CanonicalSha256Builder builder;
  builder.update(initial_digest_domain);
  append_u32(builder, direct_sparse_stable_facet_forest_schema_version);
  append_size(builder, config.stable_facet_token_count);
  append_id(builder, config.source_identity_digest);
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId canonical_batch_digest(
    const ExactDirectSparseStableFacetForestConfig& config,
    std::span<const ExactDirectSparseStableFacetInsertion> insertions,
    std::span<const ExactDirectSparseStableFacetUnion> unions) {
  contract::CanonicalSha256Builder builder;
  builder.update(batch_digest_domain);
  append_id(builder, config.source_identity_digest);
  append_size(builder, config.stable_facet_token_count);
  append_size(builder, insertions.size());
  for (const auto& insertion : insertions) {
    append_size(builder, insertion.stable_source_facet_token_index);
    append_key(builder, insertion.facet_key);
  }
  append_size(builder, unions.size());
  for (const auto& operation : unions) {
    append_size(builder, operation.left_handle);
    append_size(builder, operation.right_handle);
  }
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId successor_digest(
    const contract::CanonicalId& previous,
    const contract::CanonicalId& batch) {
  contract::CanonicalSha256Builder builder;
  builder.update(history_step_domain);
  append_id(builder, previous);
  append_id(builder, batch);
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId compute_checkpoint_digest(
    const ExactDirectSparseStableFacetForestCheckpoint& checkpoint) {
  contract::CanonicalSha256Builder builder;
  builder.update(checkpoint_digest_domain);
  const auto& stamp = checkpoint.stamp;
  append_u32(builder, checkpoint.schema_version);
  append_u64(builder, stamp.session_instance_id);
  for (const std::size_t value : {
           stamp.stable_facet_token_count,
           stamp.epoch,
           stamp.committed_batch_count,
           stamp.observed_handle_count,
           stamp.component_count,
           stamp.committed_key_point_count,
           stamp.committed_operation_count,
           stamp.committed_effective_union_count}) {
    append_size(builder, value);
  }
  append_id(builder, stamp.source_identity_digest);
  append_id(builder, stamp.semantic_history_digest);
  append_bool(builder, checkpoint.process_local_session_identity_only);
  append_bool(builder, checkpoint.semantic_metadata_complete);
  append_bool(builder, checkpoint.sparse_entries_serialized);
  append_bool(builder, checkpoint.immutable_keys_serialized);
  append_bool(builder, checkpoint.dsu_state_serialized);
  append_bool(builder, checkpoint.checkpoint_restore_supported);
  append_bool(builder, checkpoint.restartable);
  append_bool(builder, checkpoint.source_exactness_claimed);
  append_bool(builder, checkpoint.vertical_maps_complete);
  append_bool(builder, checkpoint.public_status_claimed);
  append_u8(builder, static_cast<std::uint8_t>(checkpoint.decision));
  return builder.finalize();
}

struct PreparedTicketRegistry {
  std::size_t outstanding_count{};
};

[[nodiscard]] std::size_t mix_stable_handle(
    ExactDirectSparseStableFacetHandle handle) noexcept {
  std::uint64_t word = static_cast<std::uint64_t>(handle) +
                       UINT64_C(0x9e3779b97f4a7c15);
  word = (word ^ (word >> 30U)) * UINT64_C(0xbf58476d1ce4e5b9);
  word = (word ^ (word >> 27U)) * UINT64_C(0x94d049bb133111eb);
  word ^= word >> 31U;
  return static_cast<std::size_t>(word);
}

[[nodiscard]] bool power_of_two(std::size_t value) noexcept {
  return value != 0U && (value & (value - 1U)) == 0U;
}

[[nodiscard]] bool required_handle_index_slot_count(
    std::size_t entry_count, std::size_t& slot_count) noexcept {
  if (entry_count == 0U) {
    slot_count = 0U;
    return true;
  }
  if (entry_count > std::numeric_limits<std::size_t>::max() / 2U) {
    return false;
  }
  const std::size_t required = 2U * entry_count;
  std::size_t candidate = 2U;
  while (candidate < required) {
    if (candidate > std::numeric_limits<std::size_t>::max() / 2U) {
      return false;
    }
    candidate *= 2U;
  }
  slot_count = candidate;
  return true;
}

[[nodiscard]] bool geometric_entry_capacity(
    std::size_t current_capacity,
    std::size_t required_capacity,
    std::size_t maximum_capacity,
    std::size_t& target_capacity) noexcept {
  if (required_capacity > maximum_capacity) {
    return false;
  }
  target_capacity = current_capacity;
  if (required_capacity <= current_capacity) {
    return true;
  }
  target_capacity = current_capacity == 0U ? 1U : current_capacity;
  while (target_capacity < required_capacity) {
    if (target_capacity >= maximum_capacity) {
      target_capacity = maximum_capacity;
      break;
    }
    const std::size_t available = maximum_capacity - target_capacity;
    const std::size_t growth = std::min(target_capacity, available);
    if (growth == 0U) {
      target_capacity = maximum_capacity;
      break;
    }
    target_capacity += growth;
  }
  return target_capacity >= required_capacity;
}

[[nodiscard]] std::optional<std::size_t> handle_index_row(
    std::span<const ExactDirectSparseStableFacetForestEntry> entries,
    std::span<const std::size_t> slots,
    ExactDirectSparseStableFacetHandle handle) noexcept {
  if (slots.empty() || !power_of_two(slots.size())) {
    return std::nullopt;
  }
  const std::size_t mask = slots.size() - 1U;
  std::size_t slot_index = mix_stable_handle(handle) & mask;
  for (std::size_t probe = 0U; probe < slots.size(); ++probe) {
    const std::size_t row_plus_one = slots[slot_index];
    if (row_plus_one == 0U) {
      return std::nullopt;
    }
    const std::size_t row = row_plus_one - 1U;
    if (row >= entries.size()) {
      return std::nullopt;
    }
    if (entries[row].stable_source_facet_token_index == handle) {
      return row;
    }
    slot_index = (slot_index + 1U) & mask;
  }
  return std::nullopt;
}

[[nodiscard]] bool insert_handle_index_row(
    std::span<const ExactDirectSparseStableFacetForestEntry> entries,
    std::span<std::size_t> slots,
    std::size_t row) noexcept {
  if (row >= entries.size() || row == std::numeric_limits<std::size_t>::max() ||
      slots.empty() || !power_of_two(slots.size())) {
    return false;
  }
  const auto handle = entries[row].stable_source_facet_token_index;
  const std::size_t mask = slots.size() - 1U;
  std::size_t slot_index = mix_stable_handle(handle) & mask;
  for (std::size_t probe = 0U; probe < slots.size(); ++probe) {
    const std::size_t row_plus_one = slots[slot_index];
    if (row_plus_one == 0U) {
      slots[slot_index] = row + 1U;
      return true;
    }
    const std::size_t existing_row = row_plus_one - 1U;
    if (existing_row >= entries.size() ||
        entries[existing_row].stable_source_facet_token_index == handle) {
      return false;
    }
    slot_index = (slot_index + 1U) & mask;
  }
  return false;
}

[[nodiscard]] bool rebuild_handle_index(
    std::span<const ExactDirectSparseStableFacetForestEntry> entries,
    std::span<std::size_t> slots) noexcept {
  std::fill(slots.begin(), slots.end(), 0U);
  for (std::size_t row = 0U; row < entries.size(); ++row) {
    if (!insert_handle_index_row(entries, slots, row)) {
      return false;
    }
  }
  return true;
}

enum class PositiveKeyIndexSearchDisposition : std::uint8_t {
  unobserved,
  observed,
  contradiction,
};

struct PositiveKeyIndexSearchResult {
  PositiveKeyIndexSearchDisposition disposition{
      PositiveKeyIndexSearchDisposition::contradiction};
  std::size_t row{};
  std::size_t slot_visit_count{};
  std::size_t full_key_comparison_count{};
};

[[nodiscard]] PositiveKeyIndexSearchResult positive_key_index_search(
    std::span<const ExactDirectSparseStableFacetForestEntry> entries,
    std::span<const std::size_t> slots,
    const ExactDirectSparseFacetKey& key,
    std::uint64_t fingerprint_mask) noexcept {
  PositiveKeyIndexSearchResult result;
  if (slots.empty()) {
    result.disposition = entries.empty()
                             ? PositiveKeyIndexSearchDisposition::unobserved
                             : PositiveKeyIndexSearchDisposition::contradiction;
    return result;
  }
  if (!power_of_two(slots.size())) {
    return result;
  }
  const std::uint64_t requested_fingerprint =
      fingerprint_exact_direct_sparse_facet_key(key, fingerprint_mask);
  const std::size_t mask = slots.size() - 1U;
  std::size_t slot_index =
      static_cast<std::size_t>(requested_fingerprint) & mask;
  for (std::size_t probe = 0U; probe < slots.size(); ++probe) {
    ++result.slot_visit_count;
    const std::size_t row_plus_one = slots[slot_index];
    if (row_plus_one == 0U) {
      result.disposition = PositiveKeyIndexSearchDisposition::unobserved;
      return result;
    }
    const std::size_t row = row_plus_one - 1U;
    if (row >= entries.size()) {
      return result;
    }
    const auto& stored_key = entries[row].facet_key;
    if (fingerprint_exact_direct_sparse_facet_key(
            stored_key, fingerprint_mask) == requested_fingerprint) {
      ++result.full_key_comparison_count;
      if (stored_key == key) {
        result.disposition = PositiveKeyIndexSearchDisposition::observed;
        result.row = row;
        return result;
      }
    }
    slot_index = (slot_index + 1U) & mask;
  }
  return result;
}

[[nodiscard]] bool insert_positive_key_index_row(
    std::span<const ExactDirectSparseStableFacetForestEntry> entries,
    std::span<std::size_t> slots,
    std::size_t row,
    std::uint64_t fingerprint_mask) noexcept {
  if (row >= entries.size() || row == std::numeric_limits<std::size_t>::max() ||
      slots.empty() || !power_of_two(slots.size())) {
    return false;
  }
  const auto& key = entries[row].facet_key;
  const std::uint64_t fingerprint =
      fingerprint_exact_direct_sparse_facet_key(key, fingerprint_mask);
  const std::size_t mask = slots.size() - 1U;
  std::size_t slot_index = static_cast<std::size_t>(fingerprint) & mask;
  for (std::size_t probe = 0U; probe < slots.size(); ++probe) {
    const std::size_t row_plus_one = slots[slot_index];
    if (row_plus_one == 0U) {
      slots[slot_index] = row + 1U;
      return true;
    }
    const std::size_t existing_row = row_plus_one - 1U;
    if (existing_row >= entries.size()) {
      return false;
    }
    const auto& existing_key = entries[existing_row].facet_key;
    if (fingerprint_exact_direct_sparse_facet_key(
            existing_key, fingerprint_mask) == fingerprint &&
        existing_key == key) {
      return false;
    }
    slot_index = (slot_index + 1U) & mask;
  }
  return false;
}

[[nodiscard]] bool rebuild_positive_key_index(
    std::span<const ExactDirectSparseStableFacetForestEntry> entries,
    std::span<std::size_t> slots,
    std::uint64_t fingerprint_mask) noexcept {
  std::fill(slots.begin(), slots.end(), 0U);
  for (std::size_t row = 0U; row < entries.size(); ++row) {
    if (!insert_positive_key_index_row(
            entries, slots, row, fingerprint_mask)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool key_then_handle_insertion_less(
    const ExactDirectSparseStableFacetInsertion& left,
    const ExactDirectSparseStableFacetInsertion& right) noexcept {
  if (left.facet_key != right.facet_key) {
    return key_less(left.facet_key, right.facet_key);
  }
  return left.stable_source_facet_token_index <
         right.stable_source_facet_token_index;
}

[[nodiscard]] bool insertion_span_contains_handle(
    std::span<const ExactDirectSparseStableFacetInsertion> insertions,
    ExactDirectSparseStableFacetHandle handle) noexcept {
  const auto found = std::lower_bound(
      insertions.begin(),
      insertions.end(),
      handle,
      [](const ExactDirectSparseStableFacetInsertion& insertion,
         ExactDirectSparseStableFacetHandle value) {
        return insertion.stable_source_facet_token_index < value;
      });
  return found != insertions.end() &&
         found->stable_source_facet_token_index == handle;
}

}  // namespace

struct ExactDirectSparseStableFacetForestPreparedBatch::Impl {
  std::shared_ptr<PreparedTicketRegistry> registry;
  ExactDirectSparseStableFacetForestStamp pre_stamp{};
  contract::CanonicalId batch_digest{};
  contract::CanonicalId successor_history_digest{};
  std::vector<ExactDirectSparseStableFacetInsertion> insertions;
  std::vector<ExactDirectSparseStableFacetUnion> unions;
  std::size_t batch_key_point_count{};
  std::size_t new_handle_count{};
  std::size_t new_key_point_count{};
  std::size_t compatible_repeat_count{};
  std::size_t duplicate_union_request_count{};
  bool registry_counted{false};
  bool consumed{false};

  ~Impl() { release_registry(); }

  void release_registry() noexcept {
    if (registry_counted && registry != nullptr) {
      if (registry->outstanding_count != 0U) {
        --registry->outstanding_count;
      }
      registry_counted = false;
    }
  }

  void consume() noexcept {
    consumed = true;
    release_registry();
  }
};

struct ExactDirectSparseStableFacetForest::Impl {
  ExactDirectSparseStableFacetForestConfig config{};
  ExactDirectSparseStableFacetForestBudget budget{};
  std::uint64_t session_instance_id{};
  std::vector<ExactDirectSparseStableFacetForestEntry> entries;
  // Open-addressed stable-handle index.  A zero slot is empty; every occupied
  // slot stores durable row_index + 1.  Stable handles are compared against the
  // append-only row itself, so the table does not duplicate the handle payload.
  std::vector<std::size_t> handle_index_slots;
  std::size_t handle_index_entry_count{};
  // Open-addressed full-key positive index.  Like the handle index, every
  // occupied slot stores row_index + 1 only.  The row's complete key is the
  // sole identity authority; the configured fingerprint is just a probe
  // accelerator and is never persisted or digested.
  std::vector<std::size_t> positive_key_index_slots;
  std::size_t positive_key_index_entry_count{};
  std::shared_ptr<PreparedTicketRegistry> registry;
  std::size_t epoch{};
  std::size_t committed_batch_count{};
  std::size_t component_count{};
  std::size_t committed_key_point_count{};
  std::size_t committed_operation_count{};
  std::size_t committed_effective_union_count{};
  contract::CanonicalId semantic_history_digest{};
  bool initialized{false};
  bool poisoned{false};

  [[nodiscard]] ExactDirectSparseStableFacetForestStamp stamp()
      const noexcept {
    ExactDirectSparseStableFacetForestStamp result;
    result.session_instance_id = session_instance_id;
    result.stable_facet_token_count = config.stable_facet_token_count;
    result.epoch = epoch;
    result.committed_batch_count = committed_batch_count;
    result.observed_handle_count = entries.size();
    result.component_count = component_count;
    result.committed_key_point_count = committed_key_point_count;
    result.committed_operation_count = committed_operation_count;
    result.committed_effective_union_count =
        committed_effective_union_count;
    result.source_identity_digest = config.source_identity_digest;
    result.semantic_history_digest = semantic_history_digest;
    return result;
  }

  [[nodiscard]] std::optional<std::size_t> root_index(
      ExactDirectSparseStableFacetHandle handle) const noexcept {
    std::size_t hop_count = 0U;
    auto cursor = handle_index_row(entries, handle_index_slots, handle);
    while (hop_count <= entries.size()) {
      if (!cursor.has_value()) {
        return std::nullopt;
      }
      const auto& entry = entries[*cursor];
      if (entry.parent_handle == entry.stable_source_facet_token_index) {
        return entry.component_size != 0U
                   ? std::optional<std::size_t>{*cursor}
                   : std::nullopt;
      }
      cursor = handle_index_row(
          entries, handle_index_slots, entry.parent_handle);
      if (!checked_increment(hop_count)) {
        return std::nullopt;
      }
    }
    return std::nullopt;
  }

  [[nodiscard]] bool apply_union(
      ExactDirectSparseStableFacetHandle left,
      ExactDirectSparseStableFacetHandle right) noexcept {
    const auto left_root = root_index(left);
    const auto right_root = root_index(right);
    if (!left_root.has_value() || !right_root.has_value() ||
        *left_root == *right_root) {
      return false;
    }
    auto& left_entry = entries[*left_root];
    auto& right_entry = entries[*right_root];
    std::size_t combined_size = 0U;
    if (!checked_add(
            left_entry.component_size,
            right_entry.component_size,
            combined_size)) {
      return false;
    }
    const bool left_wins =
        left_entry.component_size > right_entry.component_size ||
        (left_entry.component_size == right_entry.component_size &&
         left_entry.stable_source_facet_token_index <
             right_entry.stable_source_facet_token_index);
    auto& root = left_wins ? left_entry : right_entry;
    auto& child = left_wins ? right_entry : left_entry;
    child.parent_handle = root.stable_source_facet_token_index;
    child.component_size = 0U;
    root.component_size = combined_size;
    return true;
  }

  [[nodiscard]] bool ensure_handle_index_capacity(
      std::size_t required_entry_count) {
    std::size_t required_slots = 0U;
    if (!required_handle_index_slot_count(
            required_entry_count, required_slots)) {
      return false;
    }
    if (required_slots <= handle_index_slots.size()) {
      return true;
    }
    std::vector<std::size_t> replacement(required_slots, 0U);
    if (!rebuild_handle_index(entries, replacement)) {
      return false;
    }
    handle_index_slots.swap(replacement);
    return true;
  }

  [[nodiscard]] bool rebuild_current_handle_index() noexcept {
    if (!rebuild_handle_index(entries, handle_index_slots)) {
      return false;
    }
    handle_index_entry_count = entries.size();
    return true;
  }

  [[nodiscard]] bool ensure_positive_key_index_capacity(
      std::size_t required_entry_count) {
    std::size_t required_slots = 0U;
    if (!required_handle_index_slot_count(
            required_entry_count, required_slots)) {
      return false;
    }
    if (required_slots <= positive_key_index_slots.size()) {
      return true;
    }
    std::vector<std::size_t> replacement(required_slots, 0U);
    if (!rebuild_positive_key_index(
            entries,
            replacement,
            config.positive_key_fingerprint_mask)) {
      return false;
    }
    positive_key_index_slots.swap(replacement);
    return true;
  }

  [[nodiscard]] bool rebuild_current_positive_key_index() noexcept {
    if (!rebuild_positive_key_index(
            entries,
            positive_key_index_slots,
            config.positive_key_fingerprint_mask)) {
      return false;
    }
    positive_key_index_entry_count = entries.size();
    return true;
  }
};

static_assert(std::is_nothrow_move_constructible_v<
              ExactDirectSparseStableFacetForestEntry>);
static_assert(std::is_nothrow_move_assignable_v<
              ExactDirectSparseStableFacetForestEntry>);

bool ExactDirectSparseStableFacetLookupResult::certified_observed()
    const noexcept {
  return forest_certified_at_entry &&
         immutable_handle_key_binding_observed &&
         root_resolved_without_mutation && component_size != 0U &&
         !missing_handle_means_source_absent && !source_exactness_claimed &&
         !public_status_claimed &&
         disposition == ExactDirectSparseStableFacetLookupDisposition::observed;
}

bool ExactDirectSparseStableFacetLookupResult::certified_unobserved()
    const noexcept {
  return forest_certified_at_entry &&
         !immutable_handle_key_binding_observed &&
         root_resolved_without_mutation && component_size == 0U &&
         !missing_handle_means_source_absent && !source_exactness_claimed &&
         !public_status_claimed &&
         disposition ==
             ExactDirectSparseStableFacetLookupDisposition::unobserved;
}

ExactDirectSparseStableFacetPositiveLookupResult::
    ExactDirectSparseStableFacetPositiveLookupResult(
        const ExactDirectSparseStableFacetPositiveLookupReceipt& receipt)
    noexcept
    : ExactDirectSparseStableFacetPositiveLookupReceipt(receipt),
      private_receipt_snapshot_(receipt),
      minted_(
          receipt.disposition ==
              ExactDirectSparseStableFacetPositiveLookupDisposition::observed ||
          receipt.disposition ==
              ExactDirectSparseStableFacetPositiveLookupDisposition::
                  unobserved) {}

bool ExactDirectSparseStableFacetPositiveLookupResult::certified_observed()
    const noexcept {
  return minted_ &&
         static_cast<const ExactDirectSparseStableFacetPositiveLookupReceipt&>(
             *this) == private_receipt_snapshot_ &&
         forest_stamp.session_instance_id != 0U &&
         forest_certified_at_entry && requested_key_canonical &&
         lookup_completed_without_mutation &&
         fingerprint_used_only_as_accelerator &&
         complete_key_comparison_authoritative &&
         immutable_key_handle_binding_observed &&
         root_resolved_without_mutation && full_key_comparison_count != 0U &&
         component_size != 0U && !source_exactness_claimed &&
         !public_status_claimed &&
         disposition ==
             ExactDirectSparseStableFacetPositiveLookupDisposition::observed;
}

bool ExactDirectSparseStableFacetPositiveLookupResult::certified_unobserved()
    const noexcept {
  return minted_ &&
         static_cast<const ExactDirectSparseStableFacetPositiveLookupReceipt&>(
             *this) == private_receipt_snapshot_ &&
         forest_stamp.session_instance_id != 0U &&
         forest_certified_at_entry && requested_key_canonical &&
         lookup_completed_without_mutation &&
         fingerprint_used_only_as_accelerator &&
         complete_key_comparison_authoritative &&
         !immutable_key_handle_binding_observed &&
         root_resolved_without_mutation && component_size == 0U &&
         !source_exactness_claimed && !public_status_claimed &&
         disposition == ExactDirectSparseStableFacetPositiveLookupDisposition::
                            unobserved;
}

bool ExactDirectSparseStableFacetPositiveLookupResult::certified_observed_for(
    const ExactDirectSparseStableFacetForestStamp& expected_stamp)
    const noexcept {
  return certified_observed() && forest_stamp == expected_stamp;
}

bool ExactDirectSparseStableFacetPositiveLookupResult::certified_unobserved_for(
    const ExactDirectSparseStableFacetForestStamp& expected_stamp)
    const noexcept {
  return certified_unobserved() && forest_stamp == expected_stamp;
}

ExactDirectSparseStableFacetForestPreparedBatch::
    ExactDirectSparseStableFacetForestPreparedBatch() noexcept = default;
ExactDirectSparseStableFacetForestPreparedBatch::
    ~ExactDirectSparseStableFacetForestPreparedBatch() = default;
ExactDirectSparseStableFacetForestPreparedBatch::
    ExactDirectSparseStableFacetForestPreparedBatch(
        ExactDirectSparseStableFacetForestPreparedBatch&&) noexcept = default;
ExactDirectSparseStableFacetForestPreparedBatch&
ExactDirectSparseStableFacetForestPreparedBatch::operator=(
    ExactDirectSparseStableFacetForestPreparedBatch&&) noexcept = default;
ExactDirectSparseStableFacetForestPreparedBatch::
    ExactDirectSparseStableFacetForestPreparedBatch(
        std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectSparseStableFacetForestPreparedBatch::valid() const noexcept {
  return impl_ != nullptr && !impl_->consumed && impl_->registry_counted &&
         impl_->pre_stamp.session_instance_id != 0U;
}

bool ExactDirectSparseStableFacetForestPreparedBatch::consumed() const
    noexcept {
  return impl_ == nullptr || impl_->consumed;
}

const ExactDirectSparseStableFacetForestStamp&
ExactDirectSparseStableFacetForestPreparedBatch::pre_stamp() const noexcept {
  static const ExactDirectSparseStableFacetForestStamp empty;
  return impl_ == nullptr ? empty : impl_->pre_stamp;
}

const contract::CanonicalId&
ExactDirectSparseStableFacetForestPreparedBatch::canonical_batch_digest()
    const noexcept {
  static const contract::CanonicalId empty;
  return impl_ == nullptr ? empty : impl_->batch_digest;
}

std::size_t
ExactDirectSparseStableFacetForestPreparedBatch::insertion_request_count()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->insertions.size();
}

std::size_t
ExactDirectSparseStableFacetForestPreparedBatch::union_request_count() const
    noexcept {
  return impl_ == nullptr ? 0U : impl_->unions.size();
}

std::size_t ExactDirectSparseStableFacetForestPreparedBatch::new_handle_count()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->new_handle_count;
}

std::size_t
ExactDirectSparseStableFacetForestPreparedBatch::compatible_repeat_count()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->compatible_repeat_count;
}

bool ExactDirectSparseStableFacetForestPreparationResult::certified_prepared()
    const noexcept {
  return ticket.has_value() && ticket->valid() &&
         pre_stamp.session_instance_id != 0U &&
         insertion_request_count == ticket->insertion_request_count() &&
         union_request_count == ticket->union_request_count() &&
         new_handle_count == ticket->new_handle_count() &&
         compatible_repeat_count == ticket->compatible_repeat_count() &&
         canonical_order_independent_staging &&
         all_commit_allocations_completed && !forest_logical_state_mutated &&
         !source_exactness_claimed && !public_status_claimed &&
         decision == ExactDirectSparseStableFacetForestPreparationDecision::
                         complete_canonical_staging;
}

bool ExactDirectSparseStableFacetForestCommitResult::certified_commit()
    const noexcept {
  std::size_t expected_epoch = 0U;
  return ticket_consumed && state_mutated && no_allocation_after_preparation &&
         immutable_handle_key_bindings_preserved &&
         post_stamp.session_instance_id == pre_stamp.session_instance_id &&
         checked_add(pre_stamp.epoch, 1U, expected_epoch) &&
         post_stamp.epoch == expected_epoch &&
         !source_exactness_claimed && !vertical_maps_complete &&
         !public_status_claimed &&
         decision == ExactDirectSparseStableFacetForestCommitDecision::
                         complete_atomic_sparse_commit;
}

bool ExactDirectSparseStableFacetForestCheckpoint::
    certified_honest_nonrestartable() const noexcept {
  try {
    return schema_version == direct_sparse_stable_facet_forest_schema_version &&
           stamp.session_instance_id != 0U &&
           process_local_session_identity_only && semantic_metadata_complete &&
           !sparse_entries_serialized && !immutable_keys_serialized &&
           !dsu_state_serialized && !checkpoint_restore_supported &&
           !restartable && !source_exactness_claimed &&
           !vertical_maps_complete && !public_status_claimed &&
           decision == ExactDirectSparseStableFacetForestCheckpointDecision::
                           complete_honest_nonrestartable_semantic_checkpoint &&
           checkpoint_digest == compute_checkpoint_digest(*this);
  } catch (...) {
    return false;
  }
}

ExactDirectSparseStableFacetForest::ExactDirectSparseStableFacetForest()
    noexcept = default;
ExactDirectSparseStableFacetForest::~ExactDirectSparseStableFacetForest() =
    default;
ExactDirectSparseStableFacetForest::ExactDirectSparseStableFacetForest(
    ExactDirectSparseStableFacetForest&&) noexcept = default;
ExactDirectSparseStableFacetForest&
ExactDirectSparseStableFacetForest::operator=(
    ExactDirectSparseStableFacetForest&&) noexcept = default;
ExactDirectSparseStableFacetForest::ExactDirectSparseStableFacetForest(
    std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectSparseStableFacetForest::certified_structure_only_forest()
    const noexcept {
  const bool handle_index_shape_valid =
      impl_ != nullptr &&
      ((impl_->handle_index_slots.empty() && impl_->entries.empty()) ||
       (power_of_two(impl_->handle_index_slots.size()) &&
        impl_->entries.size() <= impl_->handle_index_slots.size() / 2U)) &&
      impl_->handle_index_entry_count == impl_->entries.size();
  const bool positive_key_index_shape_valid =
      impl_ != nullptr &&
      ((impl_->positive_key_index_slots.empty() && impl_->entries.empty()) ||
       (power_of_two(impl_->positive_key_index_slots.size()) &&
        impl_->entries.size() <=
            impl_->positive_key_index_slots.size() / 2U)) &&
      impl_->positive_key_index_entry_count == impl_->entries.size();
  return impl_ != nullptr && impl_->initialized &&
         !impl_->poisoned && impl_->session_instance_id != 0U &&
         impl_->registry != nullptr &&
         handle_index_shape_valid && positive_key_index_shape_valid &&
         impl_->entries.size() <= impl_->budget.maximum_observed_handle_count &&
         impl_->entries.size() <= impl_->config.stable_facet_token_count &&
         impl_->component_count <= impl_->entries.size() &&
         impl_->committed_key_point_count <=
             impl_->budget.maximum_committed_key_point_count &&
         impl_->committed_operation_count <=
             impl_->budget.maximum_committed_operation_count &&
         impl_->committed_batch_count <=
             impl_->budget.maximum_committed_batch_count;
}

ExactDirectSparseStableFacetForestStamp
ExactDirectSparseStableFacetForest::current_stamp() const noexcept {
  return impl_ == nullptr ? ExactDirectSparseStableFacetForestStamp{}
                          : impl_->stamp();
}

std::size_t ExactDirectSparseStableFacetForest::outstanding_ticket_count()
    const noexcept {
  return impl_ == nullptr || impl_->registry == nullptr
             ? 0U
             : impl_->registry->outstanding_count;
}

std::span<const ExactDirectSparseStableFacetForestEntry>
ExactDirectSparseStableFacetForest::observed_entries() const noexcept {
  return impl_ == nullptr
             ? std::span<const ExactDirectSparseStableFacetForestEntry>{}
             : std::span<const ExactDirectSparseStableFacetForestEntry>{
                   impl_->entries};
}

std::size_t ExactDirectSparseStableFacetForest::
    materialized_handle_index_slot_count() const noexcept {
  return impl_ == nullptr ? 0U : impl_->handle_index_slots.size();
}

std::size_t ExactDirectSparseStableFacetForest::
    materialized_positive_key_index_slot_count() const noexcept {
  return impl_ == nullptr ? 0U : impl_->positive_key_index_slots.size();
}

ExactDirectSparseStableFacetLookupResult
ExactDirectSparseStableFacetForest::lookup(
    ExactDirectSparseStableFacetHandle handle) const noexcept {
  ExactDirectSparseStableFacetLookupResult result;
  result.requested_handle = handle;
  result.forest_certified_at_entry = certified_structure_only_forest();
  if (!result.forest_certified_at_entry) {
    return result;
  }
  result.root_resolved_without_mutation = true;
  if (handle >= impl_->config.stable_facet_token_count) {
    result.disposition =
        ExactDirectSparseStableFacetLookupDisposition::handle_out_of_namespace;
    return result;
  }
  const auto found = handle_index_row(
      impl_->entries, impl_->handle_index_slots, handle);
  if (!found.has_value()) {
    result.disposition =
        ExactDirectSparseStableFacetLookupDisposition::unobserved;
    return result;
  }
  const auto root = impl_->root_index(handle);
  if (!root.has_value()) {
    result.root_resolved_without_mutation = false;
    return result;
  }
  result.facet_key = impl_->entries[*found].facet_key;
  result.root_handle =
      impl_->entries[*root].stable_source_facet_token_index;
  result.component_size = impl_->entries[*root].component_size;
  result.immutable_handle_key_binding_observed = true;
  result.disposition = ExactDirectSparseStableFacetLookupDisposition::observed;
  return result;
}

ExactDirectSparseStableFacetPositiveLookupResult
ExactDirectSparseStableFacetForest::lookup_positive_full_key(
    const ExactDirectSparseFacetKey& key) const noexcept {
  ExactDirectSparseStableFacetPositiveLookupResult result;
  result.requested_key = key;
  result.forest_stamp = current_stamp();
  result.forest_certified_at_entry = certified_structure_only_forest();
  if (!result.forest_certified_at_entry) {
    return result;
  }
  result.requested_key_canonical = canonical_key(key);
  if (!result.requested_key_canonical) {
    result.lookup_completed_without_mutation = true;
    result.disposition =
        ExactDirectSparseStableFacetPositiveLookupDisposition::
            input_key_rejected;
    return result;
  }
  result.fingerprint_used_only_as_accelerator = true;
  result.complete_key_comparison_authoritative = true;
  const PositiveKeyIndexSearchResult found = positive_key_index_search(
      impl_->entries,
      impl_->positive_key_index_slots,
      key,
      impl_->config.positive_key_fingerprint_mask);
  result.slot_visit_count = found.slot_visit_count;
  result.full_key_comparison_count = found.full_key_comparison_count;
  if (found.disposition == PositiveKeyIndexSearchDisposition::contradiction) {
    result.disposition =
        ExactDirectSparseStableFacetPositiveLookupDisposition::contradiction;
    return result;
  }
  result.lookup_completed_without_mutation = true;
  result.root_resolved_without_mutation = true;
  if (found.disposition == PositiveKeyIndexSearchDisposition::unobserved) {
    result.disposition =
        ExactDirectSparseStableFacetPositiveLookupDisposition::unobserved;
    return ExactDirectSparseStableFacetPositiveLookupResult{
        static_cast<
            const ExactDirectSparseStableFacetPositiveLookupReceipt&>(
            result)};
  }
  if (found.row >= impl_->entries.size()) {
    result.lookup_completed_without_mutation = false;
    result.root_resolved_without_mutation = false;
    result.disposition =
        ExactDirectSparseStableFacetPositiveLookupDisposition::contradiction;
    return result;
  }
  const auto& entry = impl_->entries[found.row];
  const auto root = impl_->root_index(
      entry.stable_source_facet_token_index);
  if (!root.has_value()) {
    result.lookup_completed_without_mutation = false;
    result.root_resolved_without_mutation = false;
    result.disposition =
        ExactDirectSparseStableFacetPositiveLookupDisposition::contradiction;
    return result;
  }
  result.bound_handle = entry.stable_source_facet_token_index;
  result.root_handle =
      impl_->entries[*root].stable_source_facet_token_index;
  result.component_size = impl_->entries[*root].component_size;
  result.immutable_key_handle_binding_observed = true;
  result.disposition =
      ExactDirectSparseStableFacetPositiveLookupDisposition::observed;
  return ExactDirectSparseStableFacetPositiveLookupResult{
      static_cast<const ExactDirectSparseStableFacetPositiveLookupReceipt&>(
          result)};
}

ExactDirectSparseStableFacetForestPreparationResult
ExactDirectSparseStableFacetForest::prepare_batch(
    std::span<const ExactDirectSparseStableFacetInsertion> input_insertions,
    std::span<const ExactDirectSparseStableFacetUnion> input_unions) noexcept {
  ExactDirectSparseStableFacetForestPreparationResult output;
  output.pre_stamp = current_stamp();
  if (!certified_structure_only_forest()) {
    output.decision = ExactDirectSparseStableFacetForestPreparationDecision::
        no_forest_rejected;
    return output;
  }
  if (impl_->registry->outstanding_count >=
      impl_->budget.maximum_outstanding_ticket_count) {
    output.decision = ExactDirectSparseStableFacetForestPreparationDecision::
        no_outstanding_ticket_budget_exhausted;
    return output;
  }
  output.insertion_request_count = input_insertions.size();
  output.union_request_count = input_unions.size();
  if (input_insertions.size() >
          impl_->budget.maximum_batch_insertion_request_count ||
      input_unions.size() >
          impl_->budget.maximum_batch_union_request_count) {
    output.decision = ExactDirectSparseStableFacetForestPreparationDecision::
        no_budget_exhausted;
    return output;
  }
  std::size_t batch_operation_count = 0U;
  if (!checked_add(
          input_insertions.size(),
          input_unions.size(),
          batch_operation_count)) {
    output.decision = ExactDirectSparseStableFacetForestPreparationDecision::
        no_capacity_overflow;
    return output;
  }
  output.staging_entry_count = batch_operation_count;
  if (batch_operation_count > impl_->budget.maximum_batch_operation_count ||
      batch_operation_count > impl_->budget.maximum_staging_entry_count) {
    output.decision = ExactDirectSparseStableFacetForestPreparationDecision::
        no_budget_exhausted;
    return output;
  }
  std::size_t next_batch_count = 0U;
  std::size_t next_operation_count = 0U;
  if (!checked_add(
          impl_->committed_batch_count, 1U, next_batch_count) ||
      !checked_add(
          impl_->committed_operation_count,
          batch_operation_count,
          next_operation_count)) {
    output.decision = ExactDirectSparseStableFacetForestPreparationDecision::
        no_capacity_overflow;
    return output;
  }
  if (next_batch_count > impl_->budget.maximum_committed_batch_count ||
      next_operation_count >
          impl_->budget.maximum_committed_operation_count) {
    output.decision = ExactDirectSparseStableFacetForestPreparationDecision::
        no_budget_exhausted;
    return output;
  }

  // Validate every externally supplied key before any copy or comparison.
  // `insertion_less` delegates to `key_less`, whose iterator bounds are only
  // meaningful for a canonical fixed-capacity key.  In particular, neither a
  // hostile point_count nor two hostile keys sharing one handle may reach the
  // sorting comparator.
  for (const auto& insertion : input_insertions) {
    if (insertion.stable_source_facet_token_index >=
            impl_->config.stable_facet_token_count ||
        !canonical_key(insertion.facet_key) ||
        !checked_add(
            output.batch_key_point_count,
            insertion.facet_key.point_count,
            output.batch_key_point_count)) {
      output.decision =
          ExactDirectSparseStableFacetForestPreparationDecision::
              no_input_shape_rejected;
      return output;
    }
  }
  for (const auto& operation : input_unions) {
    if (operation.left_handle >=
            impl_->config.stable_facet_token_count ||
        operation.right_handle >=
            impl_->config.stable_facet_token_count) {
      output.decision =
          ExactDirectSparseStableFacetForestPreparationDecision::
              no_input_shape_rejected;
      return output;
    }
  }
  if (output.batch_key_point_count >
      impl_->budget.maximum_batch_key_point_count) {
    output.decision = ExactDirectSparseStableFacetForestPreparationDecision::
        no_budget_exhausted;
    return output;
  }

  try {
    std::vector<ExactDirectSparseStableFacetInsertion> insertions(
        input_insertions.begin(), input_insertions.end());
    std::vector<ExactDirectSparseStableFacetUnion> unions(
        input_unions.begin(), input_unions.end());
    // First certify the inverse key binding.  This sort is preparation-only;
    // the vector is restored to the historical handle/key canonical order
    // before digesting, so the logical batch/history digest remains v1.
    std::sort(
        insertions.begin(), insertions.end(), key_then_handle_insertion_less);
    for (std::size_t begin = 0U; begin < insertions.size();) {
      std::size_t end = begin;
      if (!checked_increment(end)) {
        output.decision =
            ExactDirectSparseStableFacetForestPreparationDecision::
                no_capacity_overflow;
        return output;
      }
      while (end < insertions.size() &&
             insertions[end].facet_key == insertions[begin].facet_key) {
        if (insertions[end].stable_source_facet_token_index !=
            insertions[begin].stable_source_facet_token_index) {
          output.decision =
              ExactDirectSparseStableFacetForestPreparationDecision::
                  contradiction_stable_handle_key_collision;
          return output;
        }
        if (!checked_increment(end)) {
          output.decision =
              ExactDirectSparseStableFacetForestPreparationDecision::
                  no_capacity_overflow;
          return output;
        }
      }
      const PositiveKeyIndexSearchResult existing_key =
          positive_key_index_search(
              impl_->entries,
              impl_->positive_key_index_slots,
              insertions[begin].facet_key,
              impl_->config.positive_key_fingerprint_mask);
      const auto existing_handle = handle_index_row(
          impl_->entries,
          impl_->handle_index_slots,
          insertions[begin].stable_source_facet_token_index);
      if (existing_key.disposition ==
              PositiveKeyIndexSearchDisposition::contradiction ||
          (existing_key.disposition ==
               PositiveKeyIndexSearchDisposition::observed &&
           (impl_->entries[existing_key.row]
                    .stable_source_facet_token_index !=
                insertions[begin].stable_source_facet_token_index ||
            !existing_handle.has_value() ||
            *existing_handle != existing_key.row))) {
        output.decision =
            ExactDirectSparseStableFacetForestPreparationDecision::
                contradiction_stable_handle_key_collision;
        return output;
      }
      begin = end;
    }
    std::sort(insertions.begin(), insertions.end(), insertion_less);
    for (auto& operation : unions) {
      if (operation.right_handle < operation.left_handle) {
        std::swap(operation.left_handle, operation.right_handle);
      }
    }
    std::sort(unions.begin(), unions.end(), union_less);

    std::size_t new_key_point_count = 0U;
    for (std::size_t begin = 0U; begin < insertions.size();) {
      std::size_t end = begin;
      if (!checked_increment(end)) {
        output.decision =
            ExactDirectSparseStableFacetForestPreparationDecision::
                no_capacity_overflow;
        return output;
      }
      while (end < insertions.size() &&
             insertions[end].stable_source_facet_token_index ==
                 insertions[begin].stable_source_facet_token_index) {
        if (insertions[end].facet_key != insertions[begin].facet_key) {
          output.decision =
              ExactDirectSparseStableFacetForestPreparationDecision::
                  contradiction_stable_handle_key_collision;
          return output;
        }
        if (!checked_increment(end)) {
          output.decision =
              ExactDirectSparseStableFacetForestPreparationDecision::
                  no_capacity_overflow;
          return output;
        }
      }
      const auto existing = handle_index_row(
          impl_->entries,
          impl_->handle_index_slots,
          insertions[begin].stable_source_facet_token_index);
      const bool exists = existing.has_value();
      if (exists &&
          impl_->entries[*existing].facet_key != insertions[begin].facet_key) {
        output.decision =
            ExactDirectSparseStableFacetForestPreparationDecision::
                contradiction_stable_handle_key_collision;
        return output;
      }
      const std::size_t group_size = end - begin;
      if (exists) {
        if (!checked_add(
                output.compatible_repeat_count,
                group_size,
                output.compatible_repeat_count)) {
          output.decision =
              ExactDirectSparseStableFacetForestPreparationDecision::
                  no_capacity_overflow;
          return output;
        }
      } else {
        if (!checked_increment(output.new_handle_count) ||
            !checked_add(
                new_key_point_count,
                insertions[begin].facet_key.point_count,
                new_key_point_count) ||
            !checked_add(
                output.compatible_repeat_count,
                group_size - 1U,
                output.compatible_repeat_count)) {
          output.decision =
              ExactDirectSparseStableFacetForestPreparationDecision::
                  no_capacity_overflow;
          return output;
        }
      }
      begin = end;
    }

    std::size_t next_observed_count = 0U;
    std::size_t next_key_point_count = 0U;
    if (!checked_add(
            impl_->entries.size(),
            output.new_handle_count,
            next_observed_count) ||
        !checked_add(
            impl_->committed_key_point_count,
            new_key_point_count,
            next_key_point_count)) {
      output.decision = ExactDirectSparseStableFacetForestPreparationDecision::
          no_capacity_overflow;
      return output;
    }
    if (next_observed_count >
            impl_->budget.maximum_observed_handle_count ||
        next_key_point_count >
            impl_->budget.maximum_committed_key_point_count) {
      output.decision = ExactDirectSparseStableFacetForestPreparationDecision::
          no_budget_exhausted;
      return output;
    }

    for (std::size_t index = 0U; index < unions.size(); ++index) {
      const auto& operation = unions[index];
      const auto handle_available = [&](ExactDirectSparseStableFacetHandle h) {
        return handle_index_row(
                   impl_->entries, impl_->handle_index_slots, h)
                   .has_value() ||
               insertion_span_contains_handle(insertions, h);
      };
      if (!handle_available(operation.left_handle) ||
          !handle_available(operation.right_handle)) {
        output.decision =
            ExactDirectSparseStableFacetForestPreparationDecision::
                no_union_unknown_handle_rejected;
        return output;
      }
      if (index != 0U && unions[index - 1U] == operation &&
          !checked_increment(output.duplicate_union_request_count)) {
        output.decision =
            ExactDirectSparseStableFacetForestPreparationDecision::
                no_capacity_overflow;
        return output;
      }
    }

    std::size_t target_entry_capacity = 0U;
    if (!geometric_entry_capacity(
            impl_->entries.capacity(),
            next_observed_count,
            impl_->budget.maximum_observed_handle_count,
            target_entry_capacity)) {
      output.decision = ExactDirectSparseStableFacetForestPreparationDecision::
          no_capacity_overflow;
      return output;
    }
    impl_->entries.reserve(target_entry_capacity);
    if (!impl_->ensure_handle_index_capacity(next_observed_count) ||
        !impl_->ensure_positive_key_index_capacity(next_observed_count)) {
      output.decision = ExactDirectSparseStableFacetForestPreparationDecision::
          no_capacity_overflow;
      return output;
    }
    auto prepared = std::make_unique<
        ExactDirectSparseStableFacetForestPreparedBatch::Impl>();
    prepared->registry = impl_->registry;
    prepared->pre_stamp = output.pre_stamp;
    prepared->insertions = std::move(insertions);
    prepared->unions = std::move(unions);
    prepared->batch_key_point_count = output.batch_key_point_count;
    prepared->new_handle_count = output.new_handle_count;
    prepared->new_key_point_count = new_key_point_count;
    prepared->compatible_repeat_count = output.compatible_repeat_count;
    prepared->duplicate_union_request_count =
        output.duplicate_union_request_count;
    prepared->batch_digest = canonical_batch_digest(
        impl_->config, prepared->insertions, prepared->unions);
    prepared->successor_history_digest = successor_digest(
        output.pre_stamp.semantic_history_digest, prepared->batch_digest);
    if (!checked_increment(impl_->registry->outstanding_count)) {
      output.decision =
          ExactDirectSparseStableFacetForestPreparationDecision::
              no_capacity_overflow;
      return output;
    }
    prepared->registry_counted = true;
    output.ticket.emplace(
        ExactDirectSparseStableFacetForestPreparedBatch{std::move(prepared)});
    output.canonical_order_independent_staging = true;
    output.all_commit_allocations_completed = true;
    output.decision = ExactDirectSparseStableFacetForestPreparationDecision::
        complete_canonical_staging;
    return output;
  } catch (const std::bad_alloc&) {
    output.decision = ExactDirectSparseStableFacetForestPreparationDecision::
        no_allocation_failed;
    return output;
  } catch (const std::length_error&) {
    output.decision = ExactDirectSparseStableFacetForestPreparationDecision::
        no_allocation_failed;
    return output;
  }
}

ExactDirectSparseStableFacetForestCommitResult
ExactDirectSparseStableFacetForest::commit(
    ExactDirectSparseStableFacetForestPreparedBatch&& ticket) noexcept {
  ExactDirectSparseStableFacetForestCommitResult output;
  output.pre_stamp = current_stamp();
  if (!certified_structure_only_forest()) {
    output.decision =
        ExactDirectSparseStableFacetForestCommitDecision::no_forest_rejected;
    return output;
  }
  if (ticket.impl_ == nullptr || ticket.impl_->consumed) {
    output.decision =
        ExactDirectSparseStableFacetForestCommitDecision::no_ticket_rejected;
    return output;
  }
  auto& prepared = *ticket.impl_;
  prepared.consume();
  output.ticket_consumed = true;
  if (prepared.pre_stamp.session_instance_id != impl_->session_instance_id) {
    output.decision = ExactDirectSparseStableFacetForestCommitDecision::
        no_foreign_ticket_rejected;
    return output;
  }
  if (prepared.pre_stamp != output.pre_stamp) {
    output.decision = ExactDirectSparseStableFacetForestCommitDecision::
        no_stale_or_sibling_ticket_rejected;
    return output;
  }
  std::size_t required_capacity = 0U;
  std::size_t post_key_point_count = 0U;
  std::size_t batch_operation_count = 0U;
  std::size_t post_operation_count = 0U;
  std::size_t post_batch_count = 0U;
  std::size_t post_epoch = 0U;
  std::size_t post_component_upper_bound = 0U;
  std::size_t effective_union_upper_bound = 0U;
  std::size_t required_handle_index_slots = 0U;
  std::size_t required_positive_key_index_slots = 0U;
  if (!checked_add(
          impl_->entries.size(),
          prepared.new_handle_count,
          required_capacity) ||
      required_capacity > impl_->entries.capacity() ||
      !checked_add(
          impl_->committed_key_point_count,
          prepared.new_key_point_count,
          post_key_point_count) ||
      !checked_add(
          prepared.insertions.size(),
          prepared.unions.size(),
          batch_operation_count) ||
      !checked_add(
          impl_->committed_operation_count,
          batch_operation_count,
          post_operation_count) ||
      !checked_add(
          impl_->committed_batch_count, 1U, post_batch_count) ||
      !checked_add(impl_->epoch, 1U, post_epoch) ||
      !checked_add(
          impl_->component_count,
          prepared.new_handle_count,
          post_component_upper_bound) ||
      !checked_add(
          impl_->committed_effective_union_count,
          prepared.unions.size(),
          effective_union_upper_bound) ||
      !required_handle_index_slot_count(
          required_capacity, required_handle_index_slots) ||
      required_handle_index_slots > impl_->handle_index_slots.size() ||
      !required_handle_index_slot_count(
          required_capacity, required_positive_key_index_slots) ||
      required_positive_key_index_slots >
          impl_->positive_key_index_slots.size() ||
      impl_->handle_index_entry_count != impl_->entries.size() ||
      impl_->positive_key_index_entry_count != impl_->entries.size()) {
    output.decision =
        ExactDirectSparseStableFacetForestCommitDecision::no_ticket_rejected;
    return output;
  }
  static_cast<void>(effective_union_upper_bound);

  const std::size_t pre_entry_count = impl_->entries.size();
  const auto rollback_new_rows_and_both_indexes = [&]() noexcept {
    impl_->entries.resize(pre_entry_count);
    const bool handle_restored = impl_->rebuild_current_handle_index();
    const bool key_restored = impl_->rebuild_current_positive_key_index();
    return handle_restored && key_restored;
  };
  for (std::size_t begin = 0U; begin < prepared.insertions.size();) {
    std::size_t end = begin;
    if (!checked_increment(end)) {
      output.decision =
          ExactDirectSparseStableFacetForestCommitDecision::no_ticket_rejected;
      return output;
    }
    while (end < prepared.insertions.size() &&
           prepared.insertions[end].stable_source_facet_token_index ==
               prepared.insertions[begin].stable_source_facet_token_index) {
      if (!checked_increment(end)) {
        output.decision = ExactDirectSparseStableFacetForestCommitDecision::
            no_ticket_rejected;
        return output;
      }
    }
    const auto& insertion = prepared.insertions[begin];
    if (!handle_index_row(
             impl_->entries,
             impl_->handle_index_slots,
             insertion.stable_source_facet_token_index)
             .has_value()) {
      impl_->entries.push_back(
          {insertion.stable_source_facet_token_index,
           insertion.facet_key,
           insertion.stable_source_facet_token_index,
           1U});
      if (!insert_handle_index_row(
              impl_->entries,
              impl_->handle_index_slots,
              impl_->entries.size() - 1U) ||
          !insert_positive_key_index_row(
              impl_->entries,
              impl_->positive_key_index_slots,
              impl_->entries.size() - 1U,
              impl_->config.positive_key_fingerprint_mask)) {
        if (!rollback_new_rows_and_both_indexes()) {
          impl_->poisoned = true;
        }
        output.inserted_handle_count = 0U;
        output.decision =
            ExactDirectSparseStableFacetForestCommitDecision::
                no_ticket_rejected;
        return output;
      }
      ++impl_->handle_index_entry_count;
      ++impl_->positive_key_index_entry_count;
      static_cast<void>(checked_increment(output.inserted_handle_count));
    }
    begin = end;
  }
  output.compatible_repeat_count = prepared.compatible_repeat_count;
  output.union_request_count = prepared.unions.size();
  std::size_t post_component_count = post_component_upper_bound;
  for (const auto& operation : prepared.unions) {
    if (impl_->apply_union(operation.left_handle, operation.right_handle)) {
      static_cast<void>(checked_increment(output.effective_union_count));
      --post_component_count;
    }
  }
  std::size_t post_effective_union_count = 0U;
  static_cast<void>(checked_add(
      impl_->committed_effective_union_count,
      output.effective_union_count,
      post_effective_union_count));
  impl_->component_count = post_component_count;
  impl_->committed_key_point_count = post_key_point_count;
  impl_->committed_operation_count = post_operation_count;
  impl_->committed_batch_count = post_batch_count;
  impl_->epoch = post_epoch;
  impl_->committed_effective_union_count = post_effective_union_count;
  impl_->semantic_history_digest = prepared.successor_history_digest;

  output.post_stamp = current_stamp();
  output.state_mutated = true;
  output.no_allocation_after_preparation = true;
  output.immutable_handle_key_bindings_preserved = true;
  output.decision = ExactDirectSparseStableFacetForestCommitDecision::
      complete_atomic_sparse_commit;
  return output;
}

ExactDirectSparseStableFacetForestCheckpoint
ExactDirectSparseStableFacetForest::checkpoint() const noexcept {
  ExactDirectSparseStableFacetForestCheckpoint output;
  if (!certified_structure_only_forest()) {
    return output;
  }
  output.stamp = current_stamp();
  output.process_local_session_identity_only = true;
  output.semantic_metadata_complete = true;
  output.decision = ExactDirectSparseStableFacetForestCheckpointDecision::
      complete_honest_nonrestartable_semantic_checkpoint;
  try {
    output.checkpoint_digest = compute_checkpoint_digest(output);
  } catch (...) {
    return {};
  }
  return output;
}

bool ExactDirectSparseStableFacetForestInitialization::certified_initialized()
    const noexcept {
  return forest.has_value() && forest->certified_structure_only_forest() &&
         sparse_empty_state_initialized &&
         namespace_capacity_not_materialized && !source_exactness_claimed &&
         !public_status_claimed &&
         decision == ExactDirectSparseStableFacetForestInitializationDecision::
                         complete_empty_sparse_structure;
}

ExactDirectSparseStableFacetForestInitialization
initialize_exact_direct_sparse_stable_facet_forest(
    const ExactDirectSparseStableFacetForestConfig& config,
    const ExactDirectSparseStableFacetForestBudget& budget) noexcept {
  ExactDirectSparseStableFacetForestInitialization output;
  if (config.source_identity_digest == contract::CanonicalId{}) {
    output.decision = ExactDirectSparseStableFacetForestInitializationDecision::
        no_configuration_rejected;
    return output;
  }
  if (budget.maximum_observed_handle_count >
      config.stable_facet_token_count) {
    output.decision = ExactDirectSparseStableFacetForestInitializationDecision::
        no_budget_rejected;
    return output;
  }
  const std::uint64_t session_id = allocate_session_instance_id();
  if (session_id == 0U) {
    output.decision = ExactDirectSparseStableFacetForestInitializationDecision::
        no_session_instance_id_exhausted;
    return output;
  }
  try {
    auto impl = std::make_unique<ExactDirectSparseStableFacetForest::Impl>();
    impl->config = config;
    impl->budget = budget;
    impl->session_instance_id = session_id;
    impl->registry = std::make_shared<PreparedTicketRegistry>();
    impl->semantic_history_digest = initial_digest(config);
    impl->initialized = true;
    output.forest.emplace(
        ExactDirectSparseStableFacetForest{std::move(impl)});
    output.sparse_empty_state_initialized = true;
    output.namespace_capacity_not_materialized =
        output.forest->observed_entries().empty() &&
        output.forest->materialized_handle_index_slot_count() == 0U &&
        output.forest->materialized_positive_key_index_slot_count() == 0U;
    output.decision = ExactDirectSparseStableFacetForestInitializationDecision::
        complete_empty_sparse_structure;
    return output;
  } catch (const std::bad_alloc&) {
    output.decision = ExactDirectSparseStableFacetForestInitializationDecision::
        no_allocation_failed;
    return output;
  }
}

}  // namespace morsehgp3d::hierarchy
