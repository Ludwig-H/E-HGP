#include "morsehgp3d/hierarchy/direct_sparse_root_ledger.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <limits>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <unordered_set>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::size_t tombstone_slot = std::numeric_limits<std::size_t>::max();
constexpr std::string_view initial_digest_domain =
    "MorseHGP3D/phase15/direct-sparse-root-ledger/initial/v1/sha256/";
constexpr std::string_view preview_digest_domain =
    "MorseHGP3D/phase15/direct-sparse-root-ledger/forest-preview/v1/sha256/";
constexpr std::string_view transition_digest_domain =
    "MorseHGP3D/phase15/direct-sparse-root-ledger/transition/v1/sha256/";
constexpr std::string_view successor_digest_domain =
    "MorseHGP3D/phase15/direct-sparse-root-ledger/successor/v1/sha256/";
constexpr std::string_view checkpoint_digest_domain =
    "MorseHGP3D/phase15/direct-sparse-root-ledger/checkpoint/v1/sha256/";

std::atomic<std::uint64_t> next_session_instance_id{1U};

[[nodiscard]] std::optional<std::uint64_t> allocate_session_instance_id()
    noexcept {
  std::uint64_t candidate =
      next_session_instance_id.load(std::memory_order_relaxed);
  while (candidate != 0U) {
    const std::uint64_t successor =
        candidate == std::numeric_limits<std::uint64_t>::max()
            ? 0U
            : candidate + 1U;
    if (next_session_instance_id.compare_exchange_weak(
            candidate,
            successor,
            std::memory_order_relaxed,
            std::memory_order_relaxed)) {
      return candidate;
    }
  }
  return std::nullopt;
}

struct PreparedTicketRegistry {
  std::uint64_t session_instance_id{};
  std::size_t outstanding_count{};
};

struct PreparedTicketIdentity {};

struct StagedActiveBinding {
  ExactDirectSparseRootLedgerEntry entry{};
};

[[nodiscard]] bool checked_add(
    std::size_t left, std::size_t right, std::size_t& output) noexcept {
  if (left > std::numeric_limits<std::size_t>::max() - right) {
    return false;
  }
  output = left + right;
  return true;
}

[[nodiscard]] bool checked_increment(std::size_t& value) noexcept {
  if (value == std::numeric_limits<std::size_t>::max()) {
    return false;
  }
  ++value;
  return true;
}

[[nodiscard]] bool checked_multiply(
    std::size_t left, std::size_t right, std::size_t& output) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return false;
  }
  output = left * right;
  return true;
}

[[nodiscard]] bool valid_slice(
    std::size_t offset, std::size_t count, std::size_t extent) noexcept {
  return offset <= extent && count <= extent - offset;
}

void append_u8(contract::CanonicalSha256Builder& builder, std::uint8_t value) {
  const std::array<std::uint8_t, 1U> bytes{value};
  builder.update(bytes);
}

void append_u32(
    contract::CanonicalSha256Builder& builder, std::uint32_t value) {
  std::array<std::uint8_t, 4U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    bytes[index] = static_cast<std::uint8_t>(
        value >> ((bytes.size() - 1U - index) * 8U));
  }
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

void append_size(contract::CanonicalSha256Builder& builder, std::size_t value) {
  static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));
  append_u64(builder, static_cast<std::uint64_t>(value));
}

void append_id(
    contract::CanonicalSha256Builder& builder,
    const contract::CanonicalId& value) {
  builder.update(value.bytes());
}

void append_optional_root_id(
    contract::CanonicalSha256Builder& builder,
    const std::optional<ExactDirectSparseRootId>& value) {
  append_u8(builder, value.has_value() ? std::uint8_t{1U} : std::uint8_t{0U});
  if (value.has_value()) {
    append_u64(builder, *value);
  }
}

void append_forest_semantic_stamp(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparseStableFacetForestStamp& stamp) {
  append_u32(builder, stamp.schema_version);
  append_size(builder, stamp.stable_facet_token_count);
  append_size(builder, stamp.epoch);
  append_size(builder, stamp.committed_batch_count);
  append_size(builder, stamp.observed_handle_count);
  append_size(builder, stamp.component_count);
  append_size(builder, stamp.committed_key_point_count);
  append_size(builder, stamp.committed_operation_count);
  append_size(builder, stamp.committed_effective_union_count);
  append_id(builder, stamp.source_identity_digest);
  append_id(builder, stamp.semantic_history_digest);
}

void append_ledger_stamp(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparseRootLedgerStamp& stamp) {
  append_u32(builder, stamp.schema_version);
  append_u64(builder, stamp.session_instance_id);
  append_size(builder, stamp.epoch);
  append_size(builder, stamp.committed_batch_count);
  append_size(builder, stamp.active_root_count);
  append_size(builder, stamp.rooted_active_count);
  append_size(builder, stamp.history_entry_count);
  append_size(builder, stamp.coverage_node_count);
  append_size(builder, stamp.coverage_parent_reference_count);
  append_size(builder, stamp.coverage_point_delta_count);
  append_size(builder, stamp.prior_root_parent_reference_count);
  append_optional_root_id(builder, stamp.next_allocatable_root_id);
  append_id(builder, stamp.source_identity_digest);
  append_id(builder, stamp.semantic_history_digest);
  append_u64(builder, stamp.aligned_forest_stamp.session_instance_id);
  append_forest_semantic_stamp(builder, stamp.aligned_forest_stamp);
}

void append_ledger_semantic_stamp(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparseRootLedgerStamp& stamp) {
  append_u32(builder, stamp.schema_version);
  append_size(builder, stamp.epoch);
  append_size(builder, stamp.committed_batch_count);
  append_size(builder, stamp.active_root_count);
  append_size(builder, stamp.rooted_active_count);
  append_size(builder, stamp.history_entry_count);
  append_size(builder, stamp.coverage_node_count);
  append_size(builder, stamp.coverage_parent_reference_count);
  append_size(builder, stamp.coverage_point_delta_count);
  append_size(builder, stamp.prior_root_parent_reference_count);
  append_optional_root_id(builder, stamp.next_allocatable_root_id);
  append_id(builder, stamp.source_identity_digest);
  append_id(builder, stamp.semantic_history_digest);
  append_forest_semantic_stamp(builder, stamp.aligned_forest_stamp);
}

[[nodiscard]] std::uint64_t mix64(std::uint64_t value) noexcept {
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31U);
}

[[nodiscard]] std::uint64_t handle_fingerprint(
    ExactDirectSparseStableFacetHandle handle, std::uint64_t mask) noexcept {
  return mix64(static_cast<std::uint64_t>(handle)) & mask;
}

[[nodiscard]] std::uint64_t root_id_fingerprint(
    ExactDirectSparseRootId id, std::uint64_t mask) noexcept {
  return mix64(id ^ 0xd6e8feb86659fd93ULL) & mask;
}

[[nodiscard]] bool required_slot_count(
    std::size_t entry_count, std::size_t& output) noexcept {
  if (entry_count == 0U) {
    output = 0U;
    return true;
  }
  std::size_t doubled = 0U;
  if (!checked_add(entry_count, entry_count, doubled)) {
    return false;
  }
  std::size_t slots = 8U;
  while (slots < doubled) {
    if (slots > std::numeric_limits<std::size_t>::max() / 2U) {
      return false;
    }
    slots *= 2U;
  }
  output = slots;
  return true;
}

[[nodiscard]] bool strict_points(
    std::span<const spatial::PointId> points) noexcept {
  return std::adjacent_find(
             points.begin(), points.end(), std::greater_equal<>()) ==
         points.end();
}

[[nodiscard]] bool insert_slot(
    std::vector<std::size_t>& slots,
    std::uint64_t fingerprint,
    std::size_t row_index) noexcept {
  if (slots.empty() || row_index >= tombstone_slot - 1U) {
    return false;
  }
  const std::size_t mask = slots.size() - 1U;
  std::size_t slot = static_cast<std::size_t>(fingerprint) & mask;
  std::optional<std::size_t> first_tombstone;
  for (std::size_t visit = 0U; visit < slots.size(); ++visit) {
    if (slots[slot] == 0U) {
      slots[first_tombstone.value_or(slot)] = row_index + 1U;
      return true;
    }
    if (slots[slot] == tombstone_slot && !first_tombstone.has_value()) {
      first_tombstone = slot;
    }
    slot = (slot + 1U) & mask;
  }
  if (first_tombstone.has_value()) {
    slots[*first_tombstone] = row_index + 1U;
    return true;
  }
  return false;
}

template <class Key, class KeyForRow, class Fingerprint>
[[nodiscard]] std::optional<std::size_t> find_slot_row(
    const std::vector<ExactDirectSparseRootLedgerEntry>& rows,
    const std::vector<std::size_t>& slots,
    const Key& key,
    KeyForRow key_for_row,
    Fingerprint fingerprint) noexcept {
  if (slots.empty()) {
    return std::nullopt;
  }
  const std::size_t mask = slots.size() - 1U;
  std::size_t slot = static_cast<std::size_t>(fingerprint(key)) & mask;
  for (std::size_t visit = 0U; visit < slots.size(); ++visit) {
    const std::size_t encoded = slots[slot];
    if (encoded == 0U) {
      return std::nullopt;
    }
    if (encoded != tombstone_slot) {
      const std::size_t row = encoded - 1U;
      if (row >= rows.size()) {
        return std::nullopt;
      }
      const auto row_key = key_for_row(rows[row]);
      if (row_key.has_value() && *row_key == key) {
        return row;
      }
    }
    slot = (slot + 1U) & mask;
  }
  return std::nullopt;
}

[[nodiscard]] bool power_of_two(std::size_t value) noexcept {
  return value != 0U && (value & (value - 1U)) == 0U;
}

enum class ActiveRootIndexSearchDisposition : std::uint8_t {
  unobserved,
  observed,
  budget_exhausted,
  contradiction,
};

struct BoundedActiveRootIndexSearchResult {
  ActiveRootIndexSearchDisposition disposition{
      ActiveRootIndexSearchDisposition::contradiction};
  std::size_t row{};
  std::size_t slot_visit_count{};
  std::size_t exact_key_comparison_count{};
};

// Unlike find_slot_row(), this helper is used only by the additive bounded
// probe.  The historical lookup above deliberately remains byte-for-byte
// unchanged.  Fingerprints select a starting slot; equality of the complete
// forest-root handle remains authoritative under arbitrary collisions.
[[nodiscard]] BoundedActiveRootIndexSearchResult
active_root_index_search_bounded(
    std::span<const ExactDirectSparseRootLedgerEntry> rows,
    std::span<const std::size_t> slots,
    std::size_t active_root_count,
    ExactDirectSparseStableFacetHandle requested_handle,
    std::uint64_t fingerprint_mask,
    const ExactDirectSparseRootLedgerActiveRootProbeBudget& budget) noexcept {
  BoundedActiveRootIndexSearchResult result;
  if (slots.empty()) {
    result.disposition = active_root_count == 0U
                             ? ActiveRootIndexSearchDisposition::unobserved
                             : ActiveRootIndexSearchDisposition::contradiction;
    return result;
  }
  if (!power_of_two(slots.size()) || active_root_count > slots.size() ||
      active_root_count > rows.size()) {
    return result;
  }
  const std::size_t mask = slots.size() - 1U;
  std::size_t slot = static_cast<std::size_t>(handle_fingerprint(
                         requested_handle, fingerprint_mask)) &
                     mask;
  for (std::size_t probe = 0U; probe < slots.size(); ++probe) {
    if (result.slot_visit_count >=
        budget.maximum_root_index_slot_visit_count) {
      result.disposition =
          ActiveRootIndexSearchDisposition::budget_exhausted;
      return result;
    }
    ++result.slot_visit_count;
    const std::size_t encoded = slots[slot];
    if (encoded == 0U) {
      result.disposition = ActiveRootIndexSearchDisposition::unobserved;
      return result;
    }
    if (encoded != tombstone_slot) {
      const std::size_t row = encoded - 1U;
      if (row >= rows.size()) {
        return result;
      }
      ++result.exact_key_comparison_count;
      if (rows[row].forest_root_handle == requested_handle) {
        result.row = row;
        result.disposition = ActiveRootIndexSearchDisposition::observed;
        return result;
      }
    }
    slot = (slot + 1U) & mask;
  }

  // A table containing no empty slot is legal after accumulated tombstones.
  // Visiting the complete table with exact comparisons still proves absence.
  result.disposition = ActiveRootIndexSearchDisposition::unobserved;
  return result;
}

template <class Key, class KeyForRow, class Fingerprint>
[[nodiscard]] bool erase_slot(
    const std::vector<ExactDirectSparseRootLedgerEntry>& rows,
    std::vector<std::size_t>& slots,
    const Key& key,
    std::size_t expected_row,
    KeyForRow key_for_row,
    Fingerprint fingerprint) noexcept {
  if (slots.empty()) {
    return false;
  }
  const std::size_t mask = slots.size() - 1U;
  std::size_t slot = static_cast<std::size_t>(fingerprint(key)) & mask;
  for (std::size_t visit = 0U; visit < slots.size(); ++visit) {
    const std::size_t encoded = slots[slot];
    if (encoded == 0U) {
      return false;
    }
    if (encoded != tombstone_slot && encoded - 1U == expected_row &&
        expected_row < rows.size()) {
      const auto row_key = key_for_row(rows[expected_row]);
      if (row_key.has_value() && *row_key == key) {
        slots[slot] = tombstone_slot;
        return true;
      }
    }
    slot = (slot + 1U) & mask;
  }
  return false;
}

[[nodiscard]] contract::CanonicalId compute_preview_digest(
    const ExactDirectSparseStableFacetForestPreparedPreviewReceipt& receipt) {
  contract::CanonicalSha256Builder builder;
  builder.update(preview_digest_domain);
  append_forest_semantic_stamp(builder, receipt.pre_stamp);
  append_id(builder, receipt.canonical_batch_digest);
  append_size(builder, receipt.requested_handle_count);
  append_size(builder, receipt.shadow_node_count);
  append_size(builder, receipt.union_request_count);
  append_size(builder, receipt.expected_effective_union_count);
  append_size(builder, receipt.records.size());
  for (const auto& record : receipt.records) {
    append_size(builder, record.requested_handle);
    append_size(builder, record.post_root_handle);
    append_size(builder, record.post_component_size);
  }
  return builder.finalize();
}

[[nodiscard]] const ExactDirectSparseStableFacetForestPreparedPreviewRecord*
find_preview_record(
    std::span<const ExactDirectSparseStableFacetForestPreparedPreviewRecord>
        records,
    ExactDirectSparseStableFacetHandle handle) noexcept {
  const auto found = std::lower_bound(
      records.begin(),
      records.end(),
      handle,
      [](const auto& record, ExactDirectSparseStableFacetHandle value) {
        return record.requested_handle < value;
      });
  return found != records.end() && found->requested_handle == handle
             ? &*found
             : nullptr;
}

}  // namespace

struct ExactDirectSparseRootLedgerPreparedBatch::Impl {
  std::shared_ptr<PreparedTicketRegistry> registry;
  std::shared_ptr<const PreparedTicketIdentity> identity;
  ExactDirectSparseRootLedgerPreparationReceipt receipt{};
  contract::CanonicalId successor_history_digest{};
  ExactDirectSparseStableFacetForestPreparedBatch forest_ticket;
  ExactDirectSparseStableFacetForestPreparedPreview forest_preview;
  std::vector<ExactDirectSparseRootLedgerEntry> staged_entries;
  std::vector<ExactDirectSparseRootCoverageNode> staged_coverage_nodes;
  std::vector<ExactDirectSparseRootCoverageId> staged_parent_references;
  std::vector<spatial::PointId> staged_point_deltas;
  std::vector<ExactDirectSparseRootId> staged_prior_root_parents;
  std::vector<std::size_t> consumed_history_rows;
  std::vector<std::size_t> replacement_handle_slots;
  std::vector<std::size_t> replacement_root_id_slots;
  std::optional<ExactDirectSparseRootId> post_next_root_id;
  std::size_t post_rooted_active_count{};
  bool replace_handle_slots{false};
  bool replace_root_id_slots{false};
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
    // A terminal composite ticket must release the subordinate forest ticket
    // immediately.  In particular, a stale rejection happens before calling
    // forest.commit(); retaining that still-live ticket would keep the
    // forest's outstanding-ticket budget charged until wrapper destruction.
    // The immutable preview remains available for the post-commit comparison
    // performed by commit() after this call and owns no forest registry slot.
    forest_ticket = ExactDirectSparseStableFacetForestPreparedBatch{};
  }
};

struct ExactDirectSparseRootLedger::Impl {
  ExactDirectSparseRootLedgerConfig config{};
  ExactDirectSparseRootLedgerBudget budget{};
  std::uint64_t session_instance_id{};
  std::vector<ExactDirectSparseRootLedgerEntry> history;
  std::vector<ExactDirectSparseRootCoverageNode> coverage_nodes;
  std::vector<ExactDirectSparseRootCoverageId> coverage_parents;
  std::vector<spatial::PointId> coverage_deltas;
  std::vector<ExactDirectSparseRootId> prior_root_parents;
  std::vector<std::size_t> active_handle_slots;
  std::vector<std::size_t> active_root_id_slots;
  std::size_t active_root_count{};
  std::size_t rooted_active_count{};
  std::size_t epoch{};
  std::size_t committed_batch_count{};
  std::optional<ExactDirectSparseRootId> next_root_id;
  contract::CanonicalId source_identity_digest{};
  contract::CanonicalId semantic_history_digest{};
  ExactDirectSparseStableFacetForestStamp aligned_forest_stamp{};
  std::shared_ptr<PreparedTicketRegistry> registry;
  bool initialized{false};
  bool poisoned{false};

  [[nodiscard]] ExactDirectSparseRootLedgerStamp stamp() const noexcept {
    ExactDirectSparseRootLedgerStamp result;
    result.session_instance_id = session_instance_id;
    result.epoch = epoch;
    result.committed_batch_count = committed_batch_count;
    result.active_root_count = active_root_count;
    result.rooted_active_count = rooted_active_count;
    result.history_entry_count = history.size();
    result.coverage_node_count = coverage_nodes.size();
    result.coverage_parent_reference_count = coverage_parents.size();
    result.coverage_point_delta_count = coverage_deltas.size();
    result.prior_root_parent_reference_count = prior_root_parents.size();
    result.next_allocatable_root_id = next_root_id;
    result.source_identity_digest = source_identity_digest;
    result.semantic_history_digest = semantic_history_digest;
    result.aligned_forest_stamp = aligned_forest_stamp;
    return result;
  }

  [[nodiscard]] std::optional<std::size_t> handle_row(
      ExactDirectSparseStableFacetHandle handle) const noexcept {
    return find_slot_row(
        history,
        active_handle_slots,
        handle,
        [](const ExactDirectSparseRootLedgerEntry& row) {
          return std::optional<ExactDirectSparseStableFacetHandle>{
              row.forest_root_handle};
        },
        [this](ExactDirectSparseStableFacetHandle value) {
          return handle_fingerprint(
              value, config.forest_root_fingerprint_mask);
        });
  }

  [[nodiscard]] std::optional<std::size_t> root_id_row(
      ExactDirectSparseRootId id) const noexcept {
    return find_slot_row(
        history,
        active_root_id_slots,
        id,
        [](const ExactDirectSparseRootLedgerEntry& row) {
          return row.prior_root_id;
        },
        [this](ExactDirectSparseRootId value) {
          return root_id_fingerprint(value, config.root_id_fingerprint_mask);
        });
  }
};

static_assert(std::is_nothrow_move_constructible_v<
              ExactDirectSparseRootLedgerEntry>);
static_assert(std::is_nothrow_move_constructible_v<
              ExactDirectSparseRootCoverageNode>);

ExactDirectSparseRootLedgerPreparedBatch::
    ExactDirectSparseRootLedgerPreparedBatch() noexcept = default;
ExactDirectSparseRootLedgerPreparedBatch::~
    ExactDirectSparseRootLedgerPreparedBatch() = default;
ExactDirectSparseRootLedgerPreparedBatch::
    ExactDirectSparseRootLedgerPreparedBatch(
        ExactDirectSparseRootLedgerPreparedBatch&&) noexcept = default;
ExactDirectSparseRootLedgerPreparedBatch&
ExactDirectSparseRootLedgerPreparedBatch::operator=(
    ExactDirectSparseRootLedgerPreparedBatch&&) noexcept = default;
ExactDirectSparseRootLedgerPreparedBatch::
    ExactDirectSparseRootLedgerPreparedBatch(std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectSparseRootLedgerPreparedBatch::valid() const noexcept {
  return impl_ != nullptr && !impl_->consumed && impl_->registry != nullptr &&
         impl_->identity != nullptr && impl_->registry_counted &&
         impl_->receipt.decision ==
             ExactDirectSparseRootLedgerPreparationDecision::
                 complete_sealed_structure_only_transition &&
         impl_->forest_ticket.valid() && !impl_->forest_ticket.consumed() &&
         impl_->forest_preview.certified_for(
             impl_->forest_ticket, impl_->receipt.pre_stamp.aligned_forest_stamp);
}

bool ExactDirectSparseRootLedgerPreparedBatch::consumed() const noexcept {
  return impl_ == nullptr || impl_->consumed;
}

const ExactDirectSparseRootLedgerStamp&
ExactDirectSparseRootLedgerPreparedBatch::pre_stamp() const noexcept {
  static const ExactDirectSparseRootLedgerStamp empty;
  return impl_ != nullptr ? impl_->receipt.pre_stamp : empty;
}

const contract::CanonicalId&
ExactDirectSparseRootLedgerPreparedBatch::canonical_transition_digest()
    const noexcept {
  static const contract::CanonicalId empty;
  return impl_ != nullptr ? impl_->receipt.canonical_transition_digest : empty;
}

bool ExactDirectSparseRootLedgerPreparedBatch::certifies_receipt(
    const ExactDirectSparseRootLedgerPreparationReceipt& receipt)
    const noexcept {
  return valid() && receipt == impl_->receipt;
}

bool ExactDirectSparseRootLedgerPreparationResult::certified_prepared()
    const noexcept {
  return ticket.has_value() && ticket->certifies_receipt(
                                   static_cast<const
                                       ExactDirectSparseRootLedgerPreparationReceipt&>(
                                       *this));
}

ExactDirectSparseRootLedgerLookupResult::
    ExactDirectSparseRootLedgerLookupResult(
        const ExactDirectSparseRootLedgerLookupReceipt& receipt) noexcept
    : ExactDirectSparseRootLedgerLookupReceipt(receipt),
      private_receipt_snapshot_(receipt),
      minted_(receipt.disposition !=
              ExactDirectSparseRootLedgerLookupDisposition::not_certified) {}

bool ExactDirectSparseRootLedgerLookupResult::certified_observed()
    const noexcept {
  const bool request_matches_entry =
      (requested_key_kind ==
           ExactDirectSparseRootLedgerLookupKeyKind::forest_root_handle &&
       !requested_prior_root_id.has_value() &&
       requested_forest_root_handle == entry.forest_root_handle) ||
      (requested_key_kind ==
           ExactDirectSparseRootLedgerLookupKeyKind::prior_root_id &&
       requested_prior_root_id.has_value() &&
       entry.prior_root_id == requested_prior_root_id);
  return minted_ &&
         static_cast<const ExactDirectSparseRootLedgerLookupReceipt&>(*this) ==
             private_receipt_snapshot_ &&
         ledger_certified_at_entry && sparse_active_index_authoritative &&
         lookup_completed_without_mutation && entry.coverage_id != 0U &&
         request_matches_entry &&
         !source_exactness_claimed && !public_status_claimed &&
         ((disposition ==
               ExactDirectSparseRootLedgerLookupDisposition::observed_latent &&
           !entry.prior_root_id.has_value()) ||
          (disposition ==
               ExactDirectSparseRootLedgerLookupDisposition::observed_rooted &&
           entry.prior_root_id.has_value()));
}

bool ExactDirectSparseRootLedgerLookupResult::certified_unobserved()
    const noexcept {
  const bool request_shape =
      (requested_key_kind ==
           ExactDirectSparseRootLedgerLookupKeyKind::forest_root_handle &&
       !requested_prior_root_id.has_value()) ||
      (requested_key_kind ==
           ExactDirectSparseRootLedgerLookupKeyKind::prior_root_id &&
       requested_prior_root_id.has_value() && *requested_prior_root_id != 0U);
  return minted_ &&
         static_cast<const ExactDirectSparseRootLedgerLookupReceipt&>(*this) ==
             private_receipt_snapshot_ &&
         ledger_certified_at_entry && sparse_active_index_authoritative &&
         lookup_completed_without_mutation && entry.coverage_id == 0U &&
         request_shape &&
         !entry.prior_root_id.has_value() && !source_exactness_claimed &&
         !public_status_claimed &&
         disposition == ExactDirectSparseRootLedgerLookupDisposition::unobserved;
}

bool ExactDirectSparseRootLedgerLookupResult::certified_for(
    const ExactDirectSparseRootLedgerStamp& expected) const noexcept {
  return (certified_observed() || certified_unobserved()) &&
         ledger_stamp == expected;
}

ExactDirectSparseRootLedgerActiveRootProbeResult::
    ExactDirectSparseRootLedgerActiveRootProbeResult(
        const ExactDirectSparseRootLedgerActiveRootProbeReceipt& receipt)
    noexcept
    : ExactDirectSparseRootLedgerActiveRootProbeReceipt(receipt),
      private_receipt_snapshot_(receipt),
      minted_(
          receipt.disposition ==
              ExactDirectSparseRootLedgerActiveRootProbeDisposition::
                  unobserved ||
          receipt.disposition ==
              ExactDirectSparseRootLedgerActiveRootProbeDisposition::
                  observed_latent ||
          receipt.disposition ==
              ExactDirectSparseRootLedgerActiveRootProbeDisposition::
                  observed_rooted ||
          receipt.disposition ==
              ExactDirectSparseRootLedgerActiveRootProbeDisposition::
                  budget_exhausted ||
          receipt.disposition ==
              ExactDirectSparseRootLedgerActiveRootProbeDisposition::
                  contradiction) {}

bool ExactDirectSparseRootLedgerActiveRootProbeResult::certified_common()
    const noexcept {
  return minted_ &&
         static_cast<const
             ExactDirectSparseRootLedgerActiveRootProbeReceipt&>(*this) ==
             private_receipt_snapshot_ &&
         ledger_stamp.schema_version ==
             direct_sparse_root_ledger_schema_version &&
         ledger_stamp.session_instance_id != 0U && ledger_certified_at_entry &&
         sparse_active_root_index_authoritative &&
         fingerprint_used_only_as_accelerator &&
         exact_forest_root_handle_comparison_authoritative &&
         lookup_completed_without_mutation &&
         root_index_slot_visit_count <=
             requested_budget.maximum_root_index_slot_visit_count &&
         exact_key_comparison_count <= root_index_slot_visit_count &&
         !source_exactness_claimed && !public_status_claimed;
}

bool ExactDirectSparseRootLedgerActiveRootProbeResult::
    certified_observed_latent() const noexcept {
  return certified_common() &&
         exact_key_comparison_count != 0U &&
         entry.forest_root_handle == requested_forest_root_handle &&
         entry.history_index < ledger_stamp.history_entry_count &&
         entry.committed_batch_index < ledger_stamp.committed_batch_count &&
         entry.coverage_id != 0U &&
         entry.coverage_id <= ledger_stamp.coverage_node_count &&
         !entry.prior_root_id.has_value() &&
         disposition ==
             ExactDirectSparseRootLedgerActiveRootProbeDisposition::
                 observed_latent &&
         decision == ExactDirectSparseRootLedgerActiveRootProbeDecision::
                         complete_observed_latent;
}

bool ExactDirectSparseRootLedgerActiveRootProbeResult::
    certified_observed_rooted() const noexcept {
  return certified_common() &&
         exact_key_comparison_count != 0U &&
         entry.forest_root_handle == requested_forest_root_handle &&
         entry.history_index < ledger_stamp.history_entry_count &&
         entry.committed_batch_index < ledger_stamp.committed_batch_count &&
         entry.coverage_id != 0U &&
         entry.coverage_id <= ledger_stamp.coverage_node_count &&
         entry.prior_root_id.has_value() && *entry.prior_root_id != 0U &&
         disposition ==
             ExactDirectSparseRootLedgerActiveRootProbeDisposition::
                 observed_rooted &&
         decision == ExactDirectSparseRootLedgerActiveRootProbeDecision::
                         complete_observed_rooted;
}

bool ExactDirectSparseRootLedgerActiveRootProbeResult::certified_observed()
    const noexcept {
  return certified_observed_latent() || certified_observed_rooted();
}

bool ExactDirectSparseRootLedgerActiveRootProbeResult::certified_unobserved()
    const noexcept {
  return certified_common() &&
         entry == ExactDirectSparseRootLedgerEntry{} &&
         disposition ==
             ExactDirectSparseRootLedgerActiveRootProbeDisposition::
                 unobserved &&
         decision == ExactDirectSparseRootLedgerActiveRootProbeDecision::
                         complete_unobserved;
}

bool ExactDirectSparseRootLedgerActiveRootProbeResult::
    certified_budget_exhaustion() const noexcept {
  return certified_common() &&
         entry == ExactDirectSparseRootLedgerEntry{} &&
         root_index_slot_visit_count ==
             requested_budget.maximum_root_index_slot_visit_count &&
         disposition ==
             ExactDirectSparseRootLedgerActiveRootProbeDisposition::
                 budget_exhausted &&
         decision == ExactDirectSparseRootLedgerActiveRootProbeDecision::
                         no_root_index_slot_budget_exhausted;
}

bool ExactDirectSparseRootLedgerActiveRootProbeResult::
    certified_fail_closed_contradiction() const noexcept {
  return certified_common() &&
         entry == ExactDirectSparseRootLedgerEntry{} &&
         disposition ==
             ExactDirectSparseRootLedgerActiveRootProbeDisposition::
                 contradiction &&
         decision == ExactDirectSparseRootLedgerActiveRootProbeDecision::
                         contradiction_active_root_index;
}

bool ExactDirectSparseRootLedgerActiveRootProbeResult::certified_for(
    const ExactDirectSparseRootLedgerStamp& expected) const noexcept {
  return ledger_stamp == expected &&
         (certified_observed() || certified_unobserved() ||
          certified_budget_exhaustion() ||
          certified_fail_closed_contradiction());
}

bool ExactDirectSparseRootLedgerCommitResult::certified_commit()
    const noexcept {
  std::size_t expected_history_count = 0U;
  return checked_add(
             pre_stamp.history_entry_count,
             appended_history_entry_count,
             expected_history_count) &&
         forest_commit.certified_commit() && ticket_consumed && state_mutated &&
         no_allocation_after_preparation &&
         active_handle_and_root_id_bijections_preserved &&
         aligned_to_committed_forest_post_stamp &&
         external_transition_structure_only && !source_exactness_claimed &&
         !vertical_maps_complete && !durable_restart_supported &&
         !public_status_claimed && post_stamp.epoch == pre_stamp.epoch + 1U &&
         post_stamp.committed_batch_count ==
             pre_stamp.committed_batch_count + 1U &&
         post_stamp.history_entry_count == expected_history_count &&
         post_stamp.aligned_forest_stamp == forest_commit.post_stamp &&
         decision == ExactDirectSparseRootLedgerCommitDecision::
                         complete_atomic_forest_and_ledger_commit;
}

bool ExactDirectSparseRootCoverageMaterializationResult::
    certified_requested_only_materialization() const noexcept {
  if (decision != ExactDirectSparseRootCoverageMaterializationDecision::
                      complete_requested_roots_only ||
      records.size() != requested_root_count ||
      !only_requested_coverage_dags_traversed ||
      !canonical_sorted_unique_points || ledger_logical_state_mutated ||
      source_exactness_claimed || public_status_claimed) {
    return false;
  }
  std::size_t cursor = 0U;
  for (std::size_t index = 0U; index < records.size(); ++index) {
    const auto& record = records[index];
    if (record.coverage_id == 0U || record.point_offset != cursor ||
        !valid_slice(record.point_offset, record.point_count, point_ids.size()) ||
        (index != 0U && records[index - 1U].forest_root_handle >=
                            record.forest_root_handle)) {
      return false;
    }
    const auto points = std::span<const spatial::PointId>(point_ids).subspan(
        record.point_offset, record.point_count);
    if (!strict_points(points) ||
        !checked_add(cursor, record.point_count, cursor)) {
      return false;
    }
  }
  return cursor == point_ids.size();
}

bool ExactDirectSparseRootLedgerCheckpoint::certified_honest_nonrestartable()
    const noexcept {
  contract::CanonicalSha256Builder builder;
  builder.update(checkpoint_digest_domain);
  append_ledger_stamp(builder, stamp);
  const auto expected_digest = builder.finalize();
  return checkpoint_digest == expected_digest &&
         process_local_session_identity_only && semantic_metadata_complete &&
         !active_indexes_serialized && !history_entries_serialized &&
         !coverage_dag_serialized && !restore_supported && !restartable &&
         !source_exactness_claimed && !public_status_claimed &&
         decision == ExactDirectSparseRootLedgerCheckpointDecision::
                         complete_honest_nonrestartable_semantic_checkpoint;
}

ExactDirectSparseRootLedger::ExactDirectSparseRootLedger() noexcept = default;
ExactDirectSparseRootLedger::~ExactDirectSparseRootLedger() = default;
ExactDirectSparseRootLedger::ExactDirectSparseRootLedger(
    ExactDirectSparseRootLedger&&) noexcept = default;
ExactDirectSparseRootLedger& ExactDirectSparseRootLedger::operator=(
    ExactDirectSparseRootLedger&&) noexcept = default;
ExactDirectSparseRootLedger::ExactDirectSparseRootLedger(
    std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectSparseRootLedger::certified_structure_only_ledger()
    const noexcept {
  return impl_ != nullptr && impl_->initialized && !impl_->poisoned &&
         impl_->registry != nullptr &&
         impl_->registry->session_instance_id == impl_->session_instance_id &&
         impl_->session_instance_id != 0U &&
         impl_->source_identity_digest != contract::CanonicalId{} &&
         impl_->aligned_forest_stamp.source_identity_digest ==
             impl_->source_identity_digest &&
         impl_->active_root_count <= impl_->history.size() &&
         impl_->rooted_active_count <= impl_->active_root_count &&
         impl_->coverage_nodes.size() <= impl_->history.size() &&
         impl_->active_root_count <= impl_->budget.maximum_active_root_count &&
         impl_->history.size() <= impl_->budget.maximum_history_entry_count &&
         impl_->coverage_nodes.size() <=
             impl_->budget.maximum_coverage_node_count &&
         impl_->coverage_parents.size() <=
             impl_->budget.maximum_coverage_parent_reference_count &&
         impl_->coverage_deltas.size() <=
             impl_->budget.maximum_coverage_point_delta_count &&
         impl_->prior_root_parents.size() <=
             impl_->budget.maximum_prior_root_parent_reference_count;
}

ExactDirectSparseRootLedgerStamp ExactDirectSparseRootLedger::current_stamp()
    const noexcept {
  return impl_ != nullptr ? impl_->stamp() : ExactDirectSparseRootLedgerStamp{};
}

std::size_t ExactDirectSparseRootLedger::outstanding_ticket_count()
    const noexcept {
  return impl_ != nullptr && impl_->registry != nullptr
             ? impl_->registry->outstanding_count
             : 0U;
}

std::span<const ExactDirectSparseRootLedgerEntry>
ExactDirectSparseRootLedger::history_entries() const noexcept {
  return impl_ != nullptr
             ? std::span<const ExactDirectSparseRootLedgerEntry>(impl_->history)
             : std::span<const ExactDirectSparseRootLedgerEntry>{};
}

std::span<const ExactDirectSparseRootCoverageNode>
ExactDirectSparseRootLedger::coverage_nodes() const noexcept {
  return impl_ != nullptr
             ? std::span<const ExactDirectSparseRootCoverageNode>(
                   impl_->coverage_nodes)
             : std::span<const ExactDirectSparseRootCoverageNode>{};
}

std::span<const ExactDirectSparseRootCoverageId>
ExactDirectSparseRootLedger::coverage_parent_references() const noexcept {
  return impl_ != nullptr
             ? std::span<const ExactDirectSparseRootCoverageId>(
                   impl_->coverage_parents)
             : std::span<const ExactDirectSparseRootCoverageId>{};
}

std::span<const spatial::PointId>
ExactDirectSparseRootLedger::coverage_point_deltas() const noexcept {
  return impl_ != nullptr
             ? std::span<const spatial::PointId>(impl_->coverage_deltas)
             : std::span<const spatial::PointId>{};
}

std::span<const ExactDirectSparseRootId>
ExactDirectSparseRootLedger::prior_root_parent_references() const noexcept {
  return impl_ != nullptr
             ? std::span<const ExactDirectSparseRootId>(
                   impl_->prior_root_parents)
             : std::span<const ExactDirectSparseRootId>{};
}

std::size_t
ExactDirectSparseRootLedger::materialized_active_handle_index_slot_count()
    const noexcept {
  return impl_ != nullptr ? impl_->active_handle_slots.size() : 0U;
}

std::size_t
ExactDirectSparseRootLedger::materialized_active_root_id_index_slot_count()
    const noexcept {
  return impl_ != nullptr ? impl_->active_root_id_slots.size() : 0U;
}

ExactDirectSparseRootLedgerLookupResult
ExactDirectSparseRootLedger::lookup_active_root(
    ExactDirectSparseStableFacetHandle handle) const noexcept {
  ExactDirectSparseRootLedgerLookupReceipt receipt;
  receipt.requested_key_kind =
      ExactDirectSparseRootLedgerLookupKeyKind::forest_root_handle;
  receipt.requested_forest_root_handle = handle;
  if (!certified_structure_only_ledger()) {
    return ExactDirectSparseRootLedgerLookupResult(receipt);
  }
  receipt.ledger_stamp = impl_->stamp();
  receipt.ledger_certified_at_entry = true;
  receipt.sparse_active_index_authoritative = true;
  receipt.lookup_completed_without_mutation = true;
  const auto row = impl_->handle_row(handle);
  if (!row.has_value()) {
    receipt.disposition =
        ExactDirectSparseRootLedgerLookupDisposition::unobserved;
    return ExactDirectSparseRootLedgerLookupResult(receipt);
  }
  receipt.entry = impl_->history[*row];
  receipt.disposition = receipt.entry.prior_root_id.has_value()
                            ? ExactDirectSparseRootLedgerLookupDisposition::
                                  observed_rooted
                            : ExactDirectSparseRootLedgerLookupDisposition::
                                  observed_latent;
  return ExactDirectSparseRootLedgerLookupResult(receipt);
}

ExactDirectSparseRootLedgerLookupResult
ExactDirectSparseRootLedger::lookup_active_root_id(
    ExactDirectSparseRootId id) const noexcept {
  ExactDirectSparseRootLedgerLookupReceipt receipt;
  receipt.requested_key_kind =
      ExactDirectSparseRootLedgerLookupKeyKind::prior_root_id;
  receipt.requested_prior_root_id = id;
  if (!certified_structure_only_ledger() || id == 0U) {
    return ExactDirectSparseRootLedgerLookupResult(receipt);
  }
  receipt.ledger_stamp = impl_->stamp();
  receipt.ledger_certified_at_entry = true;
  receipt.sparse_active_index_authoritative = true;
  receipt.lookup_completed_without_mutation = true;
  const auto row = impl_->root_id_row(id);
  if (!row.has_value()) {
    receipt.disposition =
        ExactDirectSparseRootLedgerLookupDisposition::unobserved;
    return ExactDirectSparseRootLedgerLookupResult(receipt);
  }
  receipt.entry = impl_->history[*row];
  receipt.disposition =
      ExactDirectSparseRootLedgerLookupDisposition::observed_rooted;
  return ExactDirectSparseRootLedgerLookupResult(receipt);
}

ExactDirectSparseRootLedgerActiveRootProbeResult
ExactDirectSparseRootLedger::probe_active_root(
    ExactDirectSparseStableFacetHandle handle,
    const ExactDirectSparseRootLedgerActiveRootProbeBudget& budget)
    const noexcept {
  ExactDirectSparseRootLedgerActiveRootProbeReceipt receipt;
  receipt.requested_forest_root_handle = handle;
  receipt.requested_budget = budget;
  receipt.ledger_certified_at_entry = certified_structure_only_ledger();
  if (!receipt.ledger_certified_at_entry) {
    return ExactDirectSparseRootLedgerActiveRootProbeResult{};
  }
  receipt.ledger_stamp = impl_->stamp();
  receipt.sparse_active_root_index_authoritative = true;
  receipt.fingerprint_used_only_as_accelerator = true;
  receipt.exact_forest_root_handle_comparison_authoritative = true;
  receipt.lookup_completed_without_mutation = true;

  const auto found = active_root_index_search_bounded(
      impl_->history,
      impl_->active_handle_slots,
      impl_->active_root_count,
      handle,
      impl_->config.forest_root_fingerprint_mask,
      budget);
  receipt.root_index_slot_visit_count = found.slot_visit_count;
  receipt.exact_key_comparison_count = found.exact_key_comparison_count;
  if (found.disposition ==
      ActiveRootIndexSearchDisposition::budget_exhausted) {
    receipt.disposition =
        ExactDirectSparseRootLedgerActiveRootProbeDisposition::
            budget_exhausted;
    receipt.decision =
        ExactDirectSparseRootLedgerActiveRootProbeDecision::
            no_root_index_slot_budget_exhausted;
    return ExactDirectSparseRootLedgerActiveRootProbeResult(receipt);
  }
  if (found.disposition == ActiveRootIndexSearchDisposition::contradiction) {
    receipt.disposition =
        ExactDirectSparseRootLedgerActiveRootProbeDisposition::contradiction;
    receipt.decision =
        ExactDirectSparseRootLedgerActiveRootProbeDecision::
            contradiction_active_root_index;
    return ExactDirectSparseRootLedgerActiveRootProbeResult(receipt);
  }
  if (found.disposition == ActiveRootIndexSearchDisposition::unobserved) {
    receipt.disposition =
        ExactDirectSparseRootLedgerActiveRootProbeDisposition::unobserved;
    receipt.decision =
        ExactDirectSparseRootLedgerActiveRootProbeDecision::
            complete_unobserved;
    return ExactDirectSparseRootLedgerActiveRootProbeResult(receipt);
  }

  if (found.row >= impl_->history.size()) {
    receipt.disposition =
        ExactDirectSparseRootLedgerActiveRootProbeDisposition::contradiction;
    receipt.decision =
        ExactDirectSparseRootLedgerActiveRootProbeDecision::
            contradiction_active_root_index;
    return ExactDirectSparseRootLedgerActiveRootProbeResult(receipt);
  }
  const auto& entry = impl_->history[found.row];
  if (entry.history_index != found.row ||
      entry.forest_root_handle != handle || entry.coverage_id == 0U ||
      entry.coverage_id > impl_->coverage_nodes.size() ||
      impl_->coverage_nodes[entry.coverage_id - 1U].coverage_id !=
          entry.coverage_id ||
      entry.committed_batch_index >= impl_->committed_batch_count ||
      (entry.prior_root_id.has_value() && *entry.prior_root_id == 0U)) {
    receipt.disposition =
        ExactDirectSparseRootLedgerActiveRootProbeDisposition::contradiction;
    receipt.decision =
        ExactDirectSparseRootLedgerActiveRootProbeDecision::
            contradiction_active_root_index;
    return ExactDirectSparseRootLedgerActiveRootProbeResult(receipt);
  }
  receipt.entry = entry;
  if (entry.prior_root_id.has_value()) {
    receipt.disposition =
        ExactDirectSparseRootLedgerActiveRootProbeDisposition::observed_rooted;
    receipt.decision =
        ExactDirectSparseRootLedgerActiveRootProbeDecision::
            complete_observed_rooted;
  } else {
    receipt.disposition =
        ExactDirectSparseRootLedgerActiveRootProbeDisposition::observed_latent;
    receipt.decision =
        ExactDirectSparseRootLedgerActiveRootProbeDecision::
            complete_observed_latent;
  }
  return ExactDirectSparseRootLedgerActiveRootProbeResult(receipt);
}

ExactDirectSparseRootLedgerPreparationResult
ExactDirectSparseRootLedger::prepare_transition(
    const ExactDirectSparseStableFacetForest& forest,
    ExactDirectSparseStableFacetForestPreparedBatch&& forest_ticket,
    ExactDirectSparseStableFacetForestPreparedPreview&& forest_preview,
    std::span<const ExactDirectSparseRootLedgerGroupTransition> groups,
    std::span<const ExactDirectSparseStableFacetHandle> parent_root_handles,
    std::span<const spatial::PointId> group_point_deltas,
    std::span<const ExactDirectSparseRootLedgerStandaloneBirth>
        standalone_births,
    std::span<const spatial::PointId> standalone_birth_point_deltas) noexcept {
  ExactDirectSparseRootLedgerPreparationResult output;
  if (!certified_structure_only_ledger()) {
    output.decision =
        ExactDirectSparseRootLedgerPreparationDecision::no_ledger_rejected;
    return output;
  }
  output.pre_stamp = impl_->stamp();
  output.forest_batch_digest = forest_ticket.canonical_batch_digest();
  output.group_count = groups.size();
  output.standalone_birth_count = standalone_births.size();
  output.parent_root_reference_count = parent_root_handles.size();
  std::size_t total_point_deltas = 0U;
  if (!checked_add(
          group_point_deltas.size(),
          standalone_birth_point_deltas.size(),
          total_point_deltas)) {
    output.decision =
        ExactDirectSparseRootLedgerPreparationDecision::no_capacity_overflow;
    return output;
  }
  output.point_delta_count = total_point_deltas;
  output.preview_record_count = forest_preview.records().size();

  if (!forest.certified_structure_only_forest() ||
      forest.current_stamp() != impl_->aligned_forest_stamp ||
      !forest_ticket.valid() || forest_ticket.consumed() ||
      forest_ticket.pre_stamp() != impl_->aligned_forest_stamp) {
    output.decision = ExactDirectSparseRootLedgerPreparationDecision::
        no_forest_ticket_rejected;
    return output;
  }
  if (!forest_preview.certified_for(
          forest_ticket, impl_->aligned_forest_stamp)) {
    output.decision = ExactDirectSparseRootLedgerPreparationDecision::
        no_forest_preview_rejected;
    return output;
  }
  if (!forest_preview.receipt()
           .requested_handles_cover_all_ticket_touched_handles) {
    output.decision = ExactDirectSparseRootLedgerPreparationDecision::
        no_preview_exhaustiveness_rejected;
    return output;
  }
  if (impl_->registry->outstanding_count >=
      impl_->budget.maximum_outstanding_ticket_count) {
    output.decision = ExactDirectSparseRootLedgerPreparationDecision::
        no_outstanding_ticket_budget_exhausted;
    return output;
  }

  std::size_t output_count = 0U;
  std::size_t post_history_count = 0U;
  std::size_t maximum_post_coverage_count = 0U;
  std::size_t maximum_post_parent_count = 0U;
  std::size_t maximum_post_delta_count = 0U;
  std::size_t maximum_post_prior_root_parent_count = 0U;
  std::size_t staging_count = 0U;
  std::size_t required_preview_handle_capacity = 0U;
  std::size_t active_upper_bound = 0U;
  std::size_t handle_slot_upper_bound = 0U;
  std::size_t root_id_slot_upper_bound = 0U;
  std::size_t output_staging = 0U;
  std::size_t parent_staging = 0U;
  if (!checked_add(groups.size(), standalone_births.size(), output_count) ||
      output_count == 0U ||
      !checked_add(
          output_count,
          parent_root_handles.size(),
          required_preview_handle_capacity) ||
      !checked_add(
          impl_->active_root_count, output_count, active_upper_bound) ||
      !required_slot_count(active_upper_bound, handle_slot_upper_bound) ||
      !required_slot_count(active_upper_bound, root_id_slot_upper_bound) ||
      !checked_multiply(output_count, 4U, output_staging) ||
      !checked_multiply(
          parent_root_handles.size(), 6U, parent_staging) ||
      !checked_add(impl_->history.size(), output_count, post_history_count) ||
      !checked_add(
          impl_->coverage_nodes.size(),
          output_count,
          maximum_post_coverage_count) ||
      !checked_add(
          impl_->coverage_parents.size(),
          parent_root_handles.size(),
          maximum_post_parent_count) ||
      !checked_add(
          impl_->coverage_deltas.size(),
          total_point_deltas,
          maximum_post_delta_count) ||
      !checked_add(
          impl_->prior_root_parents.size(),
          parent_root_handles.size(),
          maximum_post_prior_root_parent_count) ||
      !checked_add(output_staging, parent_staging, staging_count) ||
      !checked_add(staging_count, total_point_deltas, staging_count) ||
      !checked_add(
          staging_count, forest_preview.records().size(), staging_count) ||
      (handle_slot_upper_bound > impl_->active_handle_slots.size() &&
       !checked_add(staging_count, handle_slot_upper_bound, staging_count)) ||
      (root_id_slot_upper_bound > impl_->active_root_id_slots.size() &&
       !checked_add(staging_count, root_id_slot_upper_bound, staging_count))) {
    output.decision =
        ExactDirectSparseRootLedgerPreparationDecision::no_capacity_overflow;
    return output;
  }

  // Every input-sized capacity is bounded before the first CSR scan, active
  // lookup, preview-record lookup or staging allocation.
  if (groups.size() > impl_->budget.maximum_group_count ||
      standalone_births.size() >
          impl_->budget.maximum_standalone_birth_count ||
      parent_root_handles.size() >
          impl_->budget.maximum_parent_root_reference_count ||
      total_point_deltas > impl_->budget.maximum_point_delta_count ||
      forest_preview.records().size() >
          impl_->budget.maximum_preview_record_count ||
      staging_count > impl_->budget.maximum_staging_entry_count ||
      post_history_count > impl_->budget.maximum_history_entry_count ||
      maximum_post_coverage_count >
          impl_->budget.maximum_coverage_node_count ||
      maximum_post_parent_count >
          impl_->budget.maximum_coverage_parent_reference_count ||
      maximum_post_delta_count >
          impl_->budget.maximum_coverage_point_delta_count ||
      maximum_post_prior_root_parent_count >
          impl_->budget.maximum_prior_root_parent_reference_count ||
      impl_->committed_batch_count >=
          impl_->budget.maximum_committed_batch_count) {
    output.decision =
        ExactDirectSparseRootLedgerPreparationDecision::no_budget_exhausted;
    return output;
  }

  try {
    auto prepared = std::make_unique<ExactDirectSparseRootLedgerPreparedBatch::
        Impl>();
    prepared->registry = impl_->registry;
    prepared->identity = std::make_shared<const PreparedTicketIdentity>();
    prepared->staged_entries.reserve(output_count);
    prepared->staged_coverage_nodes.reserve(output_count);
    prepared->staged_parent_references.reserve(parent_root_handles.size());
    prepared->staged_point_deltas.reserve(total_point_deltas);
    prepared->staged_prior_root_parents.reserve(parent_root_handles.size());
    prepared->consumed_history_rows.reserve(parent_root_handles.size());
    std::vector<ExactDirectSparseStableFacetHandle> required_preview_handles;
    required_preview_handles.reserve(required_preview_handle_capacity);
    std::vector<ExactDirectSparseStableFacetHandle> resulting_roots;
    resulting_roots.reserve(output_count);

    std::size_t parent_cursor = 0U;
    std::size_t group_delta_cursor = 0U;
    for (std::size_t index = 0U; index < groups.size(); ++index) {
      const auto& group = groups[index];
      if (group.source_group_index != index ||
          group.parent_root_count == 0U ||
          group.parent_root_offset != parent_cursor ||
          group.point_delta_offset != group_delta_cursor ||
          !valid_slice(
              group.parent_root_offset,
              group.parent_root_count,
              parent_root_handles.size()) ||
          !valid_slice(
              group.point_delta_offset,
              group.point_delta_count,
              group_point_deltas.size())) {
        output.decision = ExactDirectSparseRootLedgerPreparationDecision::
            no_input_shape_rejected;
        return output;
      }
      if (!checked_add(
              parent_cursor, group.parent_root_count, parent_cursor) ||
          !checked_add(
              group_delta_cursor,
              group.point_delta_count,
              group_delta_cursor)) {
        output.decision = ExactDirectSparseRootLedgerPreparationDecision::
            no_capacity_overflow;
        return output;
      }
      const auto parent_slice = parent_root_handles.subspan(
          group.parent_root_offset, group.parent_root_count);
      const auto delta_slice = group_point_deltas.subspan(
          group.point_delta_offset, group.point_delta_count);
      if (std::adjacent_find(
              parent_slice.begin(),
              parent_slice.end(),
              std::greater_equal<>()) != parent_slice.end() ||
          !strict_points(delta_slice)) {
        output.decision = ExactDirectSparseRootLedgerPreparationDecision::
            no_input_shape_rejected;
        return output;
      }
      required_preview_handles.push_back(group.representative_handle);
      required_preview_handles.insert(
          required_preview_handles.end(),
          parent_slice.begin(),
          parent_slice.end());
    }
    if (parent_cursor != parent_root_handles.size() ||
        group_delta_cursor != group_point_deltas.size()) {
      output.decision = ExactDirectSparseRootLedgerPreparationDecision::
          no_input_shape_rejected;
      return output;
    }
    std::size_t birth_delta_cursor = 0U;
    for (std::size_t index = 0U; index < standalone_births.size(); ++index) {
      const auto& birth = standalone_births[index];
      if (birth.facet_delta_count == 0U ||
          birth.point_delta_offset != birth_delta_cursor ||
          !valid_slice(
              birth.point_delta_offset,
              birth.point_delta_count,
              standalone_birth_point_deltas.size()) ||
          (index != 0U &&
           standalone_births[index - 1U].representative_handle >=
               birth.representative_handle) ||
          !strict_points(standalone_birth_point_deltas.subspan(
              birth.point_delta_offset, birth.point_delta_count))) {
        output.decision = ExactDirectSparseRootLedgerPreparationDecision::
            no_input_shape_rejected;
        return output;
      }
      if (!checked_add(
              birth_delta_cursor,
              birth.point_delta_count,
              birth_delta_cursor)) {
        output.decision = ExactDirectSparseRootLedgerPreparationDecision::
            no_capacity_overflow;
        return output;
      }
      required_preview_handles.push_back(birth.representative_handle);
    }
    if (birth_delta_cursor != standalone_birth_point_deltas.size()) {
      output.decision = ExactDirectSparseRootLedgerPreparationDecision::
          no_input_shape_rejected;
      return output;
    }
    std::sort(required_preview_handles.begin(), required_preview_handles.end());
    required_preview_handles.erase(
        std::unique(
            required_preview_handles.begin(), required_preview_handles.end()),
        required_preview_handles.end());
    const auto preview_records = forest_preview.records();
    for (const auto handle : required_preview_handles) {
      const auto record = find_preview_record(preview_records, handle);
      if (record == nullptr || record->post_component_size == 0U) {
        output.decision = ExactDirectSparseRootLedgerPreparationDecision::
            no_preview_exhaustiveness_rejected;
        return output;
      }
    }
    if (std::any_of(
            preview_records.begin(),
            preview_records.end(),
            [](const auto& record) {
              return record.post_component_size == 0U;
            })) {
      output.decision = ExactDirectSparseRootLedgerPreparationDecision::
          no_preview_exhaustiveness_rejected;
      return output;
    }

    std::optional<ExactDirectSparseRootId> next_root_id = impl_->next_root_id;
    const auto allocate_root_id = [&]()
        -> std::optional<ExactDirectSparseRootId> {
      if (!next_root_id.has_value()) {
        return std::nullopt;
      }
      const ExactDirectSparseRootId allocated = *next_root_id;
      if (allocated == std::numeric_limits<ExactDirectSparseRootId>::max()) {
        next_root_id.reset();
      } else {
        ++*next_root_id;
      }
      return allocated;
    };

    for (const auto& group : groups) {
      const auto anchor_record =
          find_preview_record(preview_records, group.representative_handle);
      if (anchor_record == nullptr) {
        output.decision = ExactDirectSparseRootLedgerPreparationDecision::
            no_preview_exhaustiveness_rejected;
        return output;
      }
      const auto parents = parent_root_handles.subspan(
          group.parent_root_offset, group.parent_root_count);
      std::vector<ExactDirectSparseRootCoverageId> parent_coverages;
      parent_coverages.reserve(parents.size());
      std::vector<ExactDirectSparseRootId> parent_root_ids;
      parent_root_ids.reserve(parents.size());
      std::optional<ExactDirectSparseRootId> preserved_root_id;
      std::size_t q_r = 0U;
      for (const auto parent_handle : parents) {
        const auto row = impl_->handle_row(parent_handle);
        const auto preview_record =
            find_preview_record(preview_records, parent_handle);
        if (!row.has_value() || preview_record == nullptr ||
            preview_record->post_root_handle !=
                anchor_record->post_root_handle) {
          output.decision = ExactDirectSparseRootLedgerPreparationDecision::
              contradiction_unknown_or_duplicate_parent_root;
          return output;
        }
        prepared->consumed_history_rows.push_back(*row);
        const auto& parent = impl_->history[*row];
        parent_coverages.push_back(parent.coverage_id);
        if (parent.prior_root_id.has_value()) {
          ++q_r;
          preserved_root_id = parent.prior_root_id;
          parent_root_ids.push_back(*parent.prior_root_id);
        }
      }
      std::sort(parent_coverages.begin(), parent_coverages.end());
      parent_coverages.erase(
          std::unique(parent_coverages.begin(), parent_coverages.end()),
          parent_coverages.end());
      std::sort(parent_root_ids.begin(), parent_root_ids.end());
      if (std::adjacent_find(parent_root_ids.begin(), parent_root_ids.end()) !=
          parent_root_ids.end()) {
        output.decision = ExactDirectSparseRootLedgerPreparationDecision::
            contradiction_unknown_or_duplicate_parent_root;
        return output;
      }
      if (parent_coverages.empty()) {
        output.decision = ExactDirectSparseRootLedgerPreparationDecision::
            contradiction_unknown_or_duplicate_parent_root;
        return output;
      }

      std::optional<ExactDirectSparseRootId> result_root_id;
      if (q_r == 1U) {
        result_root_id = preserved_root_id;
        ++output.q_r_one_count;
      } else {
        result_root_id = allocate_root_id();
        if (!result_root_id.has_value()) {
          output.decision = ExactDirectSparseRootLedgerPreparationDecision::
              contradiction_root_id_namespace_exhausted;
          return output;
        }
        ++output.allocated_root_id_count;
        if (q_r == 0U) {
          ++output.q_r_zero_count;
        } else {
          ++output.q_r_multiple_count;
        }
      }

      const auto delta = group_point_deltas.subspan(
          group.point_delta_offset, group.point_delta_count);
      ExactDirectSparseRootCoverageId coverage_id = 0U;
      const bool reuse_coverage =
          q_r == 1U && parent_coverages.size() == 1U && delta.empty();
      if (reuse_coverage) {
        coverage_id = parent_coverages.front();
        ++output.reused_coverage_count;
      } else {
        std::size_t coverage_index = 0U;
        if (!checked_add(
                impl_->coverage_nodes.size(),
                prepared->staged_coverage_nodes.size(),
                coverage_index) ||
            coverage_index == std::numeric_limits<std::size_t>::max()) {
          output.decision = ExactDirectSparseRootLedgerPreparationDecision::
              no_capacity_overflow;
          return output;
        }
        coverage_id = coverage_index + 1U;
        ExactDirectSparseRootCoverageNode node;
        node.coverage_id = coverage_id;
        node.parent_offset = impl_->coverage_parents.size() +
                             prepared->staged_parent_references.size();
        node.parent_count = parent_coverages.size();
        node.point_delta_offset = impl_->coverage_deltas.size() +
                                  prepared->staged_point_deltas.size();
        node.point_delta_count = delta.size();
        prepared->staged_coverage_nodes.push_back(node);
        prepared->staged_parent_references.insert(
            prepared->staged_parent_references.end(),
            parent_coverages.begin(),
            parent_coverages.end());
        prepared->staged_point_deltas.insert(
            prepared->staged_point_deltas.end(), delta.begin(), delta.end());
      }

      ExactDirectSparseRootLedgerEntry entry;
      entry.history_index =
          impl_->history.size() + prepared->staged_entries.size();
      entry.committed_batch_index = impl_->committed_batch_count;
      entry.forest_root_handle = anchor_record->post_root_handle;
      entry.source_group_index = group.source_group_index;
      entry.prior_root_id = result_root_id;
      entry.coverage_id = coverage_id;
      entry.prior_root_parent_offset = impl_->prior_root_parents.size() +
                                       prepared->staged_prior_root_parents.size();
      entry.prior_root_parent_count = parent_root_ids.size();
      entry.q_r = q_r;
      entry.parent_root_count = parents.size();
      entry.facet_delta_count = group.facet_delta_count;
      entry.point_delta_count = delta.size();
      entry.coverage_reused = reuse_coverage;
      entry.kind = ExactDirectSparseRootLedgerTransitionKind::quotient_group;
      prepared->staged_prior_root_parents.insert(
          prepared->staged_prior_root_parents.end(),
          parent_root_ids.begin(),
          parent_root_ids.end());
      prepared->staged_entries.push_back(entry);
      resulting_roots.push_back(entry.forest_root_handle);
    }

    std::sort(
        prepared->consumed_history_rows.begin(),
        prepared->consumed_history_rows.end());
    if (std::adjacent_find(
            prepared->consumed_history_rows.begin(),
            prepared->consumed_history_rows.end()) !=
        prepared->consumed_history_rows.end()) {
      output.decision = ExactDirectSparseRootLedgerPreparationDecision::
          contradiction_unknown_or_duplicate_parent_root;
      return output;
    }

    for (const auto& birth : standalone_births) {
      const auto record =
          find_preview_record(preview_records, birth.representative_handle);
      if (record == nullptr ||
          record->post_root_handle != birth.representative_handle ||
          record->post_component_size != 1U ||
          impl_->handle_row(record->post_root_handle).has_value()) {
        output.decision = ExactDirectSparseRootLedgerPreparationDecision::
            contradiction_resulting_root_collision;
        return output;
      }
      const auto delta = standalone_birth_point_deltas.subspan(
          birth.point_delta_offset, birth.point_delta_count);
      std::size_t coverage_index = 0U;
      if (!checked_add(
              impl_->coverage_nodes.size(),
              prepared->staged_coverage_nodes.size(),
              coverage_index) ||
          coverage_index == std::numeric_limits<std::size_t>::max()) {
        output.decision = ExactDirectSparseRootLedgerPreparationDecision::
            no_capacity_overflow;
        return output;
      }
      ExactDirectSparseRootCoverageNode node;
      node.coverage_id = coverage_index + 1U;
      node.parent_offset = impl_->coverage_parents.size() +
                           prepared->staged_parent_references.size();
      node.point_delta_offset = impl_->coverage_deltas.size() +
                                prepared->staged_point_deltas.size();
      node.point_delta_count = delta.size();
      prepared->staged_coverage_nodes.push_back(node);
      prepared->staged_point_deltas.insert(
          prepared->staged_point_deltas.end(), delta.begin(), delta.end());

      ExactDirectSparseRootLedgerEntry entry;
      entry.history_index =
          impl_->history.size() + prepared->staged_entries.size();
      entry.committed_batch_index = impl_->committed_batch_count;
      entry.forest_root_handle = record->post_root_handle;
      entry.coverage_id = node.coverage_id;
      entry.facet_delta_count = birth.facet_delta_count;
      entry.point_delta_count = delta.size();
      entry.kind =
          ExactDirectSparseRootLedgerTransitionKind::standalone_direct_birth;
      entry.prior_root_parent_offset = impl_->prior_root_parents.size() +
                                       prepared->staged_prior_root_parents.size();
      prepared->staged_entries.push_back(entry);
      resulting_roots.push_back(entry.forest_root_handle);
    }

    std::sort(resulting_roots.begin(), resulting_roots.end());
    if (std::adjacent_find(resulting_roots.begin(), resulting_roots.end()) !=
        resulting_roots.end()) {
      output.decision = ExactDirectSparseRootLedgerPreparationDecision::
          contradiction_resulting_root_collision;
      return output;
    }
    for (const auto& record : preview_records) {
      const bool post_root_is_staged = std::binary_search(
          resulting_roots.begin(),
          resulting_roots.end(),
          record.post_root_handle);
      const auto pre_lookup = forest.lookup(record.requested_handle);
      if (pre_lookup.certified_observed()) {
        const auto active_row = impl_->handle_row(pre_lookup.root_handle);
        if (!active_row.has_value()) {
          output.decision = ExactDirectSparseRootLedgerPreparationDecision::
              no_preview_exhaustiveness_rejected;
          return output;
        }
        const bool pre_root_consumed = std::binary_search(
            prepared->consumed_history_rows.begin(),
            prepared->consumed_history_rows.end(),
            *active_row);
        if ((pre_root_consumed && !post_root_is_staged) ||
            (!pre_root_consumed &&
             record.post_root_handle != pre_lookup.root_handle)) {
          output.decision = ExactDirectSparseRootLedgerPreparationDecision::
              no_preview_exhaustiveness_rejected;
          return output;
        }
      } else if (!pre_lookup.certified_unobserved() ||
                 !post_root_is_staged) {
        output.decision = ExactDirectSparseRootLedgerPreparationDecision::
            no_preview_exhaustiveness_rejected;
        return output;
      }
    }
    for (const auto& entry : prepared->staged_entries) {
      const auto existing = impl_->handle_row(entry.forest_root_handle);
      if (existing.has_value() &&
          !std::binary_search(
              prepared->consumed_history_rows.begin(),
              prepared->consumed_history_rows.end(),
              *existing)) {
        output.decision = ExactDirectSparseRootLedgerPreparationDecision::
            contradiction_resulting_root_collision;
        return output;
      }
    }

    std::size_t post_active_count = 0U;
    if (prepared->consumed_history_rows.size() > impl_->active_root_count ||
        !checked_add(
            impl_->active_root_count - prepared->consumed_history_rows.size(),
            prepared->staged_entries.size(),
            post_active_count) ||
        post_active_count > impl_->budget.maximum_active_root_count) {
      output.decision =
          ExactDirectSparseRootLedgerPreparationDecision::no_budget_exhausted;
      return output;
    }
    output.post_active_root_count = post_active_count;
    std::size_t consumed_rooted = 0U;
    for (const std::size_t row : prepared->consumed_history_rows) {
      if (impl_->history[row].prior_root_id.has_value()) {
        ++consumed_rooted;
      }
    }
    if (consumed_rooted > impl_->rooted_active_count) {
      output.decision = ExactDirectSparseRootLedgerPreparationDecision::
          contradiction_unknown_or_duplicate_parent_root;
      return output;
    }
    std::size_t staged_rooted = 0U;
    for (const auto& entry : prepared->staged_entries) {
      if (entry.prior_root_id.has_value()) {
        ++staged_rooted;
      }
    }
    if (!checked_add(
            impl_->rooted_active_count - consumed_rooted,
            staged_rooted,
            prepared->post_rooted_active_count)) {
      output.decision = ExactDirectSparseRootLedgerPreparationDecision::
          no_capacity_overflow;
      return output;
    }
    prepared->post_next_root_id = next_root_id;

    std::size_t required_handle_slots = 0U;
    std::size_t required_root_slots = 0U;
    if (!required_slot_count(post_active_count, required_handle_slots) ||
        !required_slot_count(
            prepared->post_rooted_active_count, required_root_slots)) {
      output.decision = ExactDirectSparseRootLedgerPreparationDecision::
          no_capacity_overflow;
      return output;
    }
    const auto build_replacement = [&](bool root_index) -> bool {
      auto& replacement = root_index ? prepared->replacement_root_id_slots
                                     : prepared->replacement_handle_slots;
      const std::size_t slots = root_index ? required_root_slots
                                           : required_handle_slots;
      replacement.assign(slots, 0U);
      const auto& current_slots =
          root_index ? impl_->active_root_id_slots
                     : impl_->active_handle_slots;
      for (const std::size_t encoded : current_slots) {
        if (encoded == 0U || encoded == tombstone_slot) {
          continue;
        }
        const std::size_t row = encoded - 1U;
        if (row >= impl_->history.size()) {
          return false;
        }
        if (std::binary_search(
                prepared->consumed_history_rows.begin(),
                prepared->consumed_history_rows.end(),
                row)) {
          continue;
        }
        const auto& entry = impl_->history[row];
        if (root_index && !entry.prior_root_id.has_value()) {
          continue;
        }
        const std::uint64_t fingerprint =
            root_index
                ? root_id_fingerprint(
                      *entry.prior_root_id,
                      impl_->config.root_id_fingerprint_mask)
                : handle_fingerprint(
                      entry.forest_root_handle,
                      impl_->config.forest_root_fingerprint_mask);
        if (!insert_slot(replacement, fingerprint, row)) {
          return false;
        }
      }
      for (const auto& entry : prepared->staged_entries) {
        if (root_index && !entry.prior_root_id.has_value()) {
          continue;
        }
        const std::uint64_t fingerprint =
            root_index
                ? root_id_fingerprint(
                      *entry.prior_root_id,
                      impl_->config.root_id_fingerprint_mask)
                : handle_fingerprint(
                      entry.forest_root_handle,
                      impl_->config.forest_root_fingerprint_mask);
        if (!insert_slot(replacement, fingerprint, entry.history_index)) {
          return false;
        }
      }
      return true;
    };
    if (required_handle_slots > impl_->active_handle_slots.size()) {
      prepared->replace_handle_slots = true;
      if (!build_replacement(false)) {
        output.decision = ExactDirectSparseRootLedgerPreparationDecision::
            contradiction_resulting_root_collision;
        return output;
      }
    }
    if (required_root_slots > impl_->active_root_id_slots.size()) {
      prepared->replace_root_id_slots = true;
      if (!build_replacement(true)) {
        output.decision = ExactDirectSparseRootLedgerPreparationDecision::
            contradiction_resulting_root_collision;
        return output;
      }
    }

    impl_->history.reserve(post_history_count);
    impl_->coverage_nodes.reserve(
        impl_->coverage_nodes.size() + prepared->staged_coverage_nodes.size());
    impl_->coverage_parents.reserve(
        impl_->coverage_parents.size() +
        prepared->staged_parent_references.size());
    impl_->coverage_deltas.reserve(
        impl_->coverage_deltas.size() + prepared->staged_point_deltas.size());
    impl_->prior_root_parents.reserve(
        impl_->prior_root_parents.size() +
        prepared->staged_prior_root_parents.size());

    output.staged_coverage_node_count =
        prepared->staged_coverage_nodes.size();
    output.staged_coverage_parent_reference_count =
        prepared->staged_parent_references.size();
    output.staged_coverage_point_delta_count =
        prepared->staged_point_deltas.size();
    output.staged_prior_root_parent_reference_count =
        prepared->staged_prior_root_parents.size();
    output.preview_receipt_digest = compute_preview_digest(
        forest_preview.receipt());

    contract::CanonicalSha256Builder transition_builder;
    transition_builder.update(transition_digest_domain);
    append_ledger_semantic_stamp(transition_builder, output.pre_stamp);
    append_id(transition_builder, output.forest_batch_digest);
    append_id(transition_builder, output.preview_receipt_digest);
    append_size(transition_builder, prepared->staged_entries.size());
    for (const auto& entry : prepared->staged_entries) {
      append_size(transition_builder, entry.forest_root_handle);
      append_u8(
          transition_builder,
          entry.source_group_index.has_value() ? std::uint8_t{1U}
                                               : std::uint8_t{0U});
      if (entry.source_group_index.has_value()) {
        append_size(transition_builder, *entry.source_group_index);
      }
      append_optional_root_id(transition_builder, entry.prior_root_id);
      append_size(transition_builder, entry.coverage_id);
      append_size(transition_builder, entry.prior_root_parent_offset);
      append_size(transition_builder, entry.prior_root_parent_count);
      append_size(transition_builder, entry.q_r);
      append_size(transition_builder, entry.parent_root_count);
      append_size(transition_builder, entry.facet_delta_count);
      append_size(transition_builder, entry.point_delta_count);
      append_u8(transition_builder, entry.coverage_reused ? 1U : 0U);
      append_u8(
          transition_builder, static_cast<std::uint8_t>(entry.kind));
    }
    append_size(
        transition_builder, prepared->staged_parent_references.size());
    for (const auto id : prepared->staged_parent_references) {
      append_size(transition_builder, id);
    }
    append_size(transition_builder, prepared->staged_point_deltas.size());
    for (const auto point : prepared->staged_point_deltas) {
      append_u64(transition_builder, point);
    }
    append_size(
        transition_builder, prepared->staged_prior_root_parents.size());
    for (const auto root_id : prepared->staged_prior_root_parents) {
      append_u64(transition_builder, root_id);
    }
    output.canonical_transition_digest = transition_builder.finalize();

    contract::CanonicalSha256Builder successor_builder;
    successor_builder.update(successor_digest_domain);
    append_id(successor_builder, impl_->semantic_history_digest);
    append_id(successor_builder, output.canonical_transition_digest);
    prepared->successor_history_digest = successor_builder.finalize();

    output.forest_ticket_identity_bound_by_private_preview = true;
    output.all_previewed_parents_share_declared_post_root = true;
    output.all_fallible_work_completed_before_commit = true;
    output.forest_or_ledger_logical_state_mutated = false;
    output.physical_capacity_may_have_changed = true;
    output.external_transition_structure_only = true;
    output.frozen_scientific_authority_replayed = false;
    output.source_exactness_claimed = false;
    output.vertical_maps_complete = false;
    output.durable_restart_supported = false;
    output.public_status_claimed = false;
    output.decision = ExactDirectSparseRootLedgerPreparationDecision::
        complete_sealed_structure_only_transition;
    prepared->receipt = static_cast<const
        ExactDirectSparseRootLedgerPreparationReceipt&>(output);
    prepared->forest_ticket = std::move(forest_ticket);
    prepared->forest_preview = std::move(forest_preview);
    ++impl_->registry->outstanding_count;
    prepared->registry_counted = true;
    ExactDirectSparseRootLedgerPreparedBatch sealed_ticket(
        std::move(prepared));
    output.ticket.emplace(std::move(sealed_ticket));
    return output;
  } catch (const std::bad_alloc&) {
    output.decision =
        ExactDirectSparseRootLedgerPreparationDecision::no_allocation_failed;
    return output;
  } catch (const std::length_error&) {
    output.decision =
        ExactDirectSparseRootLedgerPreparationDecision::no_capacity_overflow;
    return output;
  } catch (...) {
    output.decision = ExactDirectSparseRootLedgerPreparationDecision::
        no_input_shape_rejected;
    return output;
  }
}

ExactDirectSparseRootLedgerCommitResult ExactDirectSparseRootLedger::commit(
    ExactDirectSparseRootLedgerPreparedBatch&& ticket,
    ExactDirectSparseStableFacetForest& forest) noexcept {
  ExactDirectSparseRootLedgerCommitResult output;
  if (!certified_structure_only_ledger()) {
    output.decision =
        ExactDirectSparseRootLedgerCommitDecision::no_ledger_rejected;
    return output;
  }
  output.pre_stamp = impl_->stamp();
  if (!ticket.valid() || ticket.impl_ == nullptr) {
    output.decision =
        ExactDirectSparseRootLedgerCommitDecision::no_ticket_rejected;
    return output;
  }
  auto& prepared = *ticket.impl_;
  if (prepared.registry != impl_->registry || prepared.identity == nullptr ||
      prepared.registry->session_instance_id != impl_->session_instance_id) {
    output.decision =
        ExactDirectSparseRootLedgerCommitDecision::no_foreign_ticket_rejected;
    return output;
  }
  if (prepared.receipt.pre_stamp != output.pre_stamp ||
      forest.current_stamp() != impl_->aligned_forest_stamp ||
      !prepared.forest_preview.certified_for(
          prepared.forest_ticket, impl_->aligned_forest_stamp)) {
    prepared.consume();
    output.ticket_consumed = true;
    output.decision = ExactDirectSparseRootLedgerCommitDecision::
        no_stale_or_sibling_ticket_rejected;
    return output;
  }

  const std::size_t expected_inserted_handle_count =
      prepared.forest_ticket.new_handle_count();
  const auto& expected_preview_receipt = prepared.forest_preview.receipt();
  output.forest_commit =
      forest.commit(std::move(prepared.forest_ticket));
  prepared.consume();
  output.ticket_consumed = true;
  if (!output.forest_commit.certified_commit()) {
    output.decision = ExactDirectSparseRootLedgerCommitDecision::
        no_forest_commit_rejected;
    return output;
  }
  bool forest_post_matches_preview =
      output.forest_commit.inserted_handle_count ==
          expected_inserted_handle_count &&
      output.forest_commit.union_request_count ==
          expected_preview_receipt.union_request_count &&
      output.forest_commit.effective_union_count ==
          expected_preview_receipt.expected_effective_union_count;
  for (const auto& record : expected_preview_receipt.records) {
    const auto observed = forest.lookup(record.requested_handle);
    if (!observed.certified_observed() ||
        observed.root_handle != record.post_root_handle ||
        observed.component_size != record.post_component_size) {
      forest_post_matches_preview = false;
      break;
    }
  }
  if (!forest_post_matches_preview) {
    // The forest has already committed and has no rollback image.  Poisoning
    // is therefore the only honest closed failure: this ledger can no longer
    // certify alignment or accept another transition.
    impl_->poisoned = true;
    output.state_mutated = true;
    output.decision = ExactDirectSparseRootLedgerCommitDecision::
        no_forest_commit_rejected;
    return output;
  }

  // All following pushes address capacity reserved by preparation and contain
  // only nothrow scalar records.  No allocation or validation remains.
  if (!prepared.replace_handle_slots) {
    for (const std::size_t row : prepared.consumed_history_rows) {
      const auto& entry = impl_->history[row];
      const bool erased = erase_slot(
          impl_->history,
          impl_->active_handle_slots,
          entry.forest_root_handle,
          row,
          [](const ExactDirectSparseRootLedgerEntry& candidate) {
            return std::optional<ExactDirectSparseStableFacetHandle>{
                candidate.forest_root_handle};
          },
          [this](ExactDirectSparseStableFacetHandle value) {
            return handle_fingerprint(
                value, impl_->config.forest_root_fingerprint_mask);
          });
      if (!erased) {
        impl_->poisoned = true;
      }
    }
  }
  if (!prepared.replace_root_id_slots) {
    for (const std::size_t row : prepared.consumed_history_rows) {
      const auto& entry = impl_->history[row];
      if (!entry.prior_root_id.has_value()) {
        continue;
      }
      const bool erased = erase_slot(
          impl_->history,
          impl_->active_root_id_slots,
          *entry.prior_root_id,
          row,
          [](const ExactDirectSparseRootLedgerEntry& candidate) {
            return candidate.prior_root_id;
          },
          [this](ExactDirectSparseRootId value) {
            return root_id_fingerprint(
                value, impl_->config.root_id_fingerprint_mask);
          });
      if (!erased) {
        impl_->poisoned = true;
      }
    }
  }

  impl_->coverage_parents.insert(
      impl_->coverage_parents.end(),
      prepared.staged_parent_references.begin(),
      prepared.staged_parent_references.end());
  impl_->coverage_deltas.insert(
      impl_->coverage_deltas.end(),
      prepared.staged_point_deltas.begin(),
      prepared.staged_point_deltas.end());
  impl_->prior_root_parents.insert(
      impl_->prior_root_parents.end(),
      prepared.staged_prior_root_parents.begin(),
      prepared.staged_prior_root_parents.end());
  impl_->coverage_nodes.insert(
      impl_->coverage_nodes.end(),
      prepared.staged_coverage_nodes.begin(),
      prepared.staged_coverage_nodes.end());
  impl_->history.insert(
      impl_->history.end(),
      prepared.staged_entries.begin(),
      prepared.staged_entries.end());

  if (prepared.replace_handle_slots) {
    impl_->active_handle_slots.swap(prepared.replacement_handle_slots);
  } else {
    for (const auto& entry : prepared.staged_entries) {
      if (!insert_slot(
              impl_->active_handle_slots,
              handle_fingerprint(
                  entry.forest_root_handle,
                  impl_->config.forest_root_fingerprint_mask),
              entry.history_index)) {
        impl_->poisoned = true;
      }
    }
  }
  if (prepared.replace_root_id_slots) {
    impl_->active_root_id_slots.swap(prepared.replacement_root_id_slots);
  } else {
    for (const auto& entry : prepared.staged_entries) {
      if (entry.prior_root_id.has_value() &&
          !insert_slot(
              impl_->active_root_id_slots,
              root_id_fingerprint(
                  *entry.prior_root_id,
                  impl_->config.root_id_fingerprint_mask),
              entry.history_index)) {
        impl_->poisoned = true;
      }
    }
  }

  impl_->active_root_count = prepared.receipt.post_active_root_count;
  impl_->rooted_active_count = prepared.post_rooted_active_count;
  impl_->next_root_id = prepared.post_next_root_id;
  impl_->semantic_history_digest = prepared.successor_history_digest;
  impl_->aligned_forest_stamp = output.forest_commit.post_stamp;
  ++impl_->epoch;
  ++impl_->committed_batch_count;

  output.appended_history_entry_count = prepared.staged_entries.size();
  output.appended_coverage_node_count =
      prepared.staged_coverage_nodes.size();
  output.allocated_root_id_count = prepared.receipt.allocated_root_id_count;
  output.preserved_root_id_count = prepared.receipt.q_r_one_count;
  output.latent_birth_count = prepared.receipt.standalone_birth_count;
  output.state_mutated = true;
  output.no_allocation_after_preparation = true;
  output.active_handle_and_root_id_bijections_preserved = !impl_->poisoned;
  output.aligned_to_committed_forest_post_stamp = true;
  output.external_transition_structure_only = true;
  output.post_stamp = impl_->stamp();
  output.decision = impl_->poisoned
                        ? ExactDirectSparseRootLedgerCommitDecision::
                              no_ledger_rejected
                        : ExactDirectSparseRootLedgerCommitDecision::
                              complete_atomic_forest_and_ledger_commit;
  return output;
}

ExactDirectSparseRootCoverageMaterializationResult
ExactDirectSparseRootLedger::materialize_active_coverages(
    std::span<const ExactDirectSparseStableFacetHandle> requested_handles,
    const ExactDirectSparseRootCoverageMaterializationBudget& budget)
    const noexcept {
  ExactDirectSparseRootCoverageMaterializationResult output;
  if (!certified_structure_only_ledger()) {
    output.decision = ExactDirectSparseRootCoverageMaterializationDecision::
        no_ledger_rejected;
    return output;
  }
  output.ledger_stamp = impl_->stamp();
  output.requested_root_count = requested_handles.size();
  if (requested_handles.size() > budget.maximum_requested_root_count) {
    output.decision = ExactDirectSparseRootCoverageMaterializationDecision::
        no_budget_exhausted;
    return output;
  }
  if (std::adjacent_find(
          requested_handles.begin(),
          requested_handles.end(),
          std::greater_equal<>()) != requested_handles.end()) {
    output.decision = ExactDirectSparseRootCoverageMaterializationDecision::
        no_requested_root_shape_rejected;
    return output;
  }
  try {
    output.records.reserve(requested_handles.size());
    for (const auto handle : requested_handles) {
      const auto row = impl_->handle_row(handle);
      if (!row.has_value()) {
        output.records.clear();
        output.point_ids.clear();
        output.decision = ExactDirectSparseRootCoverageMaterializationDecision::
            no_requested_root_unobserved_rejected;
        return output;
      }
      const auto& binding = impl_->history[*row];
      std::vector<ExactDirectSparseRootCoverageId> stack;
      std::unordered_set<ExactDirectSparseRootCoverageId> visited;
      std::vector<spatial::PointId> points;
      if (budget.maximum_scratch_entry_count == 0U) {
        output.records.clear();
        output.decision = ExactDirectSparseRootCoverageMaterializationDecision::
            no_budget_exhausted;
        return output;
      }
      stack.push_back(binding.coverage_id);
      output.scratch_entry_count =
          std::max(output.scratch_entry_count, std::size_t{1U});
      while (!stack.empty()) {
        const auto coverage_id = stack.back();
        stack.pop_back();
        if (visited.find(coverage_id) != visited.end()) {
          continue;
        }
        std::size_t scratch_after_visit = 0U;
        if (!checked_add(visited.size(), 1U, scratch_after_visit) ||
            !checked_add(
                scratch_after_visit, stack.size(), scratch_after_visit) ||
            !checked_add(
                scratch_after_visit, points.size(), scratch_after_visit) ||
            scratch_after_visit > budget.maximum_scratch_entry_count) {
          output.records.clear();
          output.point_ids.clear();
          output.decision = ExactDirectSparseRootCoverageMaterializationDecision::
              no_budget_exhausted;
          return output;
        }
        visited.insert(coverage_id);
        output.scratch_entry_count =
            std::max(output.scratch_entry_count, scratch_after_visit);
        if (coverage_id == 0U || coverage_id > impl_->coverage_nodes.size()) {
          output.records.clear();
          output.point_ids.clear();
          output.decision = ExactDirectSparseRootCoverageMaterializationDecision::
              contradiction_coverage_dag;
          return output;
        }
        if (!checked_increment(output.reachable_coverage_node_count) ||
            output.reachable_coverage_node_count >
                budget.maximum_reachable_coverage_node_count) {
          output.records.clear();
          output.point_ids.clear();
          output.decision = ExactDirectSparseRootCoverageMaterializationDecision::
              no_budget_exhausted;
          return output;
        }
        const auto& node = impl_->coverage_nodes[coverage_id - 1U];
        if (node.coverage_id != coverage_id ||
            !valid_slice(
                node.parent_offset,
                node.parent_count,
                impl_->coverage_parents.size()) ||
            !valid_slice(
                node.point_delta_offset,
                node.point_delta_count,
                impl_->coverage_deltas.size())) {
          output.records.clear();
          output.point_ids.clear();
          output.decision = ExactDirectSparseRootCoverageMaterializationDecision::
              contradiction_coverage_dag;
          return output;
        }
        const auto parents =
            std::span<const ExactDirectSparseRootCoverageId>(
                impl_->coverage_parents)
                .subspan(node.parent_offset, node.parent_count);
        std::size_t post_parent_scan_count = 0U;
        std::size_t post_stack_size = 0U;
        std::size_t scratch_after_parents = 0U;
        if (!checked_add(
                output.parent_reference_scan_count,
                parents.size(),
                post_parent_scan_count) ||
            post_parent_scan_count >
                budget.maximum_parent_reference_scan_count ||
            !checked_add(stack.size(), parents.size(), post_stack_size) ||
            !checked_add(
                visited.size(), post_stack_size, scratch_after_parents) ||
            !checked_add(
                scratch_after_parents,
                points.size(),
                scratch_after_parents) ||
            scratch_after_parents > budget.maximum_scratch_entry_count) {
          output.records.clear();
          output.point_ids.clear();
          output.decision = ExactDirectSparseRootCoverageMaterializationDecision::
              no_budget_exhausted;
          return output;
        }
        for (const auto parent : parents) {
          if (parent == 0U || parent >= coverage_id) {
            output.records.clear();
            output.point_ids.clear();
            output.decision = ExactDirectSparseRootCoverageMaterializationDecision::
                contradiction_coverage_dag;
            return output;
          }
        }
        output.parent_reference_scan_count = post_parent_scan_count;
        stack.insert(stack.end(), parents.begin(), parents.end());
        output.scratch_entry_count =
            std::max(output.scratch_entry_count, scratch_after_parents);
        const auto deltas = std::span<const spatial::PointId>(
                                impl_->coverage_deltas)
                                .subspan(
                                    node.point_delta_offset,
                                    node.point_delta_count);
        if (!strict_points(deltas)) {
          output.records.clear();
          output.point_ids.clear();
          output.decision = ExactDirectSparseRootCoverageMaterializationDecision::
              contradiction_coverage_dag;
          return output;
        }
        std::size_t post_delta_scan_count = 0U;
        std::size_t post_point_size = 0U;
        std::size_t scratch_after_deltas = 0U;
        if (!checked_add(
                output.point_delta_scan_count,
                deltas.size(),
                post_delta_scan_count) ||
            post_delta_scan_count > budget.maximum_point_delta_scan_count ||
            !checked_add(points.size(), deltas.size(), post_point_size) ||
            !checked_add(
                visited.size(), stack.size(), scratch_after_deltas) ||
            !checked_add(
                scratch_after_deltas,
                post_point_size,
                scratch_after_deltas) ||
            scratch_after_deltas > budget.maximum_scratch_entry_count) {
          output.records.clear();
          output.point_ids.clear();
          output.decision = ExactDirectSparseRootCoverageMaterializationDecision::
              no_budget_exhausted;
          return output;
        }
        output.point_delta_scan_count = post_delta_scan_count;
        points.insert(points.end(), deltas.begin(), deltas.end());
        output.scratch_entry_count =
            std::max(output.scratch_entry_count, scratch_after_deltas);
      }
      std::sort(points.begin(), points.end());
      points.erase(std::unique(points.begin(), points.end()), points.end());
      std::size_t post_output_count = 0U;
      if (!checked_add(
              output.point_ids.size(), points.size(), post_output_count)) {
        output.records.clear();
        output.point_ids.clear();
        output.decision = ExactDirectSparseRootCoverageMaterializationDecision::
            no_capacity_overflow;
        return output;
      }
      if (post_output_count > budget.maximum_output_point_count) {
        output.records.clear();
        output.point_ids.clear();
        output.decision = ExactDirectSparseRootCoverageMaterializationDecision::
            no_budget_exhausted;
        return output;
      }
      output.records.push_back({
          handle,
          binding.prior_root_id,
          binding.coverage_id,
          output.point_ids.size(),
          points.size()});
      output.point_ids.insert(
          output.point_ids.end(), points.begin(), points.end());
    }
    output.only_requested_coverage_dags_traversed = true;
    output.canonical_sorted_unique_points = true;
    output.ledger_logical_state_mutated = false;
    output.decision = ExactDirectSparseRootCoverageMaterializationDecision::
        complete_requested_roots_only;
    return output;
  } catch (const std::bad_alloc&) {
    output.records.clear();
    output.point_ids.clear();
    output.decision = ExactDirectSparseRootCoverageMaterializationDecision::
        no_allocation_failed;
    return output;
  } catch (const std::length_error&) {
    output.records.clear();
    output.point_ids.clear();
    output.decision = ExactDirectSparseRootCoverageMaterializationDecision::
        no_capacity_overflow;
    return output;
  } catch (...) {
    output.records.clear();
    output.point_ids.clear();
    output.decision = ExactDirectSparseRootCoverageMaterializationDecision::
        contradiction_coverage_dag;
    return output;
  }
}

ExactDirectSparseRootLedgerCheckpoint ExactDirectSparseRootLedger::checkpoint()
    const noexcept {
  ExactDirectSparseRootLedgerCheckpoint output;
  if (!certified_structure_only_ledger()) {
    return output;
  }
  output.stamp = impl_->stamp();
  contract::CanonicalSha256Builder builder;
  builder.update(checkpoint_digest_domain);
  append_ledger_stamp(builder, output.stamp);
  output.checkpoint_digest = builder.finalize();
  output.process_local_session_identity_only = true;
  output.semantic_metadata_complete = true;
  output.decision = ExactDirectSparseRootLedgerCheckpointDecision::
      complete_honest_nonrestartable_semantic_checkpoint;
  return output;
}

bool ExactDirectSparseRootLedgerInitialization::certified_initialized()
    const noexcept {
  return ledger.has_value() && ledger->certified_structure_only_ledger() &&
         sparse_empty_state_initialized &&
         forest_handle_namespace_not_materialized && !source_exactness_claimed &&
         !public_status_claimed &&
         decision == ExactDirectSparseRootLedgerInitializationDecision::
                         complete_empty_sparse_ledger;
}

ExactDirectSparseRootLedgerInitialization
initialize_exact_direct_sparse_root_ledger(
    const ExactDirectSparseRootLedgerConfig& config,
    const ExactDirectSparseRootLedgerBudget& budget,
    const ExactDirectSparseStableFacetForest& forest) noexcept {
  ExactDirectSparseRootLedgerInitialization output;
  if (config.first_allocated_root_id == 0U) {
    output.decision = ExactDirectSparseRootLedgerInitializationDecision::
        no_configuration_rejected;
    return output;
  }
  if (budget.maximum_outstanding_ticket_count == 0U) {
    output.decision =
        ExactDirectSparseRootLedgerInitializationDecision::no_budget_rejected;
    return output;
  }
  const auto forest_stamp = forest.current_stamp();
  if (!forest.certified_structure_only_forest() ||
      forest_stamp.source_identity_digest == contract::CanonicalId{} ||
      forest_stamp.epoch != 0U || forest_stamp.committed_batch_count != 0U ||
      forest_stamp.observed_handle_count != 0U ||
      forest_stamp.component_count != 0U ||
      forest_stamp.committed_key_point_count != 0U ||
      forest_stamp.committed_operation_count != 0U ||
      forest_stamp.committed_effective_union_count != 0U ||
      forest.outstanding_ticket_count() != 0U) {
    output.decision =
        ExactDirectSparseRootLedgerInitializationDecision::no_forest_rejected;
    return output;
  }
  try {
    const auto session_id = allocate_session_instance_id();
    if (!session_id.has_value()) {
      output.decision = ExactDirectSparseRootLedgerInitializationDecision::
          no_session_instance_id_exhausted;
      return output;
    }
    auto impl = std::make_unique<ExactDirectSparseRootLedger::Impl>();
    impl->config = config;
    impl->budget = budget;
    impl->session_instance_id = *session_id;
    impl->next_root_id = config.first_allocated_root_id;
    impl->source_identity_digest = forest_stamp.source_identity_digest;
    impl->aligned_forest_stamp = forest_stamp;
    impl->registry = std::make_shared<PreparedTicketRegistry>();
    impl->registry->session_instance_id = *session_id;
    contract::CanonicalSha256Builder builder;
    builder.update(initial_digest_domain);
    append_u32(builder, direct_sparse_root_ledger_schema_version);
    append_id(builder, impl->source_identity_digest);
    append_u64(builder, config.first_allocated_root_id);
    append_forest_semantic_stamp(builder, impl->aligned_forest_stamp);
    impl->semantic_history_digest = builder.finalize();
    impl->initialized = true;
    output.ledger.emplace(ExactDirectSparseRootLedger(std::move(impl)));
    output.sparse_empty_state_initialized = true;
    output.forest_handle_namespace_not_materialized = true;
    output.decision = ExactDirectSparseRootLedgerInitializationDecision::
        complete_empty_sparse_ledger;
    return output;
  } catch (const std::bad_alloc&) {
    output.decision = ExactDirectSparseRootLedgerInitializationDecision::
        no_allocation_failed;
    return output;
  } catch (...) {
    output.decision =
        ExactDirectSparseRootLedgerInitializationDecision::no_forest_rejected;
    return output;
  }
}

}  // namespace morsehgp3d::hierarchy
