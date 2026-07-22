#include "morsehgp3d/hierarchy/pair_support_stream_codec.hpp"

#include "morsehgp3d/contract/canonical_id.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using DecodeDecision = ExactPairSupportStreamDecodeDecision;

constexpr std::array<std::uint8_t, 16U> chunk_magic{
    'M', 'H', 'G', 'P', '3', 'D', '-', 'P',
    'A', 'I', 'R', '-', 'C', 'H', 'N', 'K'};
constexpr std::uint8_t chunk_wire_kind = 1U;
constexpr std::uint8_t chunk_wire_flags = 0U;
constexpr std::size_t chunk_header_byte_count =
    chunk_magic.size() + 4U + 1U + 1U + 8U;
constexpr std::size_t chunk_checksum_byte_count =
    contract::CanonicalId::byte_count;
constexpr std::string_view chunk_checksum_domain =
    "MorseHGP3D/phase9/pair-support/chunk-wire/v1/sha256/";

#define MORSEHGP3D_FOR_EACH_PAIR_SUPPORT_AUDIT_FIELD(APPLY) \
  APPLY(total_pair_count);                                      \
  APPLY(work_unit_count);                                       \
  APPLY(support_product_visit_count);                           \
  APPLY(support_product_expansion_count);                       \
  APPLY(self_product_expansion_count);                          \
  APPLY(cross_product_expansion_count);                         \
  APPLY(diagonal_leaf_discard_count);                           \
  APPLY(diagonal_product_rank_search_skip_count);               \
  APPLY(rank_prune_search_count);                               \
  APPLY(witness_node_visit_count);                              \
  APPLY(exact_phi_aabb_bound_count);                            \
  APPLY(exact_anchor_ball_minimum_aabb_bound_count);            \
  APPLY(certified_anchor_noninterior_subtree_count);            \
  APPLY(certified_anchor_noninterior_point_count);              \
  APPLY(certified_anchor_shell_tangent_subtree_count);          \
  APPLY(equality_or_positive_bound_descent_count);              \
  APPLY(strict_interior_witness_subtree_count);                 \
  APPLY(strict_interior_witness_point_count);                   \
  APPLY(rank_pruned_product_count);                             \
  APPLY(rank_pruned_pair_count);                                \
  APPLY(leaf_pair_classification_count);                        \
  APPLY(global_closed_ball_query_count);                        \
  APPLY(point_classification_count);                            \
  APPLY(closed_ball_node_visit_count);                          \
  APPLY(exact_closed_ball_minimum_aabb_bound_count);            \
  APPLY(exact_closed_ball_maximum_aabb_bound_count);            \
  APPLY(closed_ball_bulk_interior_subtree_count);               \
  APPLY(closed_ball_bulk_interior_point_count);                 \
  APPLY(closed_ball_bulk_exterior_subtree_count);               \
  APPLY(closed_ball_bulk_exterior_point_count);                 \
  APPLY(early_closed_rank_rejection_count);                     \
  APPLY(exact_point_distance_evaluation_count);                 \
  APPLY(accepted_event_count);                                  \
  APPLY(relevant_extra_shell_diagnostic_count);                 \
  APPLY(emitted_point_id_reference_count);                      \
  APPLY(above_rank_pair_count);                                 \
  APPLY(maximum_frontier_entry_count);                          \
  APPLY(maximum_witness_frontier_entry_count);                  \
  APPLY(maximum_closed_ball_frontier_entry_count);              \
  APPLY(remaining_frontier_pair_count);                         \
  APPLY(resolved_pair_count)

[[nodiscard]] bool is_unbounded_limit(std::size_t value) noexcept {
  return value == std::numeric_limits<std::size_t>::max();
}

void require_finite_limits(
    const ExactPairSupportStreamCodecLimits& limits) {
  if (is_unbounded_limit(limits.maximum_encoded_byte_count) ||
      is_unbounded_limit(limits.maximum_frontier_entry_count) ||
      is_unbounded_limit(limits.maximum_auxiliary_entry_count) ||
      is_unbounded_limit(limits.maximum_record_count) ||
      is_unbounded_limit(limits.maximum_point_id_reference_count) ||
      is_unbounded_limit(limits.maximum_exact_text_byte_count) ||
      is_unbounded_limit(limits.maximum_total_exact_text_byte_count)) {
    throw std::invalid_argument(
        "pair-support codec limits must be finite, not SIZE_MAX");
  }
}

[[nodiscard]] std::size_t checked_add_for_encode(
    std::size_t left,
    std::size_t right,
    std::string_view message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::length_error(std::string{message});
  }
  return left + right;
}

[[nodiscard]] std::uint64_t checked_u64_for_encode(
    std::size_t value,
    std::string_view message) {
  if constexpr (
      std::numeric_limits<std::size_t>::max() >
      std::numeric_limits<std::uint64_t>::max()) {
    if (value > std::numeric_limits<std::uint64_t>::max()) {
      throw std::length_error(std::string{message});
    }
  }
  return static_cast<std::uint64_t>(value);
}

class ByteWriter {
 public:
  ByteWriter(
      std::size_t maximum_byte_count,
      std::size_t reserved_trailing_byte_count = 0U)
      : maximum_byte_count_(maximum_byte_count),
        reserved_trailing_byte_count_(reserved_trailing_byte_count) {
    if (reserved_trailing_byte_count_ > maximum_byte_count_) {
      throw std::length_error(
          "the pair-support wire representation exceeds its byte limit");
    }
  }

  void byte(std::uint8_t value) {
    require_capacity(1U);
    bytes_.push_back(value);
  }

  void boolean(bool value) {
    byte(value ? std::uint8_t{1U} : std::uint8_t{0U});
  }

  void u32(std::uint32_t value) {
    for (std::size_t index = 0U; index < 4U; ++index) {
      const std::size_t shift = (3U - index) * 8U;
      byte(static_cast<std::uint8_t>(value >> shift));
    }
  }

  void u64(std::uint64_t value) {
    for (std::size_t index = 0U; index < 8U; ++index) {
      const std::size_t shift = (7U - index) * 8U;
      byte(static_cast<std::uint8_t>(value >> shift));
    }
  }

  void size(std::size_t value) {
    u64(checked_u64_for_encode(
        value, "a pair-support codec size does not fit uint64"));
  }

  void bytes(std::span<const std::uint8_t> values) {
    require_capacity(values.size());
    bytes_.insert(bytes_.end(), values.begin(), values.end());
  }

  void identifier(const contract::CanonicalId& identifier) {
    bytes(identifier.bytes());
  }

  [[nodiscard]] std::size_t byte_count() const noexcept {
    return bytes_.size();
  }

  void overwrite_u64(std::size_t offset, std::uint64_t value) {
    if (offset > bytes_.size() || bytes_.size() - offset < 8U) {
      throw std::logic_error(
          "a pair-support wire uint64 backpatch is out of range");
    }
    for (std::size_t index = 0U; index < 8U; ++index) {
      const std::size_t shift = (7U - index) * 8U;
      bytes_[offset + index] =
          static_cast<std::uint8_t>(value >> shift);
    }
  }

  void append_reserved_identifier(
      const contract::CanonicalId& identifier) {
    if (reserved_trailing_byte_count_ !=
        contract::CanonicalId::byte_count) {
      throw std::logic_error(
          "the pair-support checksum reservation is inconsistent");
    }
    reserved_trailing_byte_count_ = 0U;
    bytes(identifier.bytes());
  }

  [[nodiscard]] const std::vector<std::uint8_t>& data() const noexcept {
    return bytes_;
  }

  [[nodiscard]] std::vector<std::uint8_t> release() && {
    return std::move(bytes_);
  }

 private:
  void require_capacity(std::size_t additional_byte_count) const {
    if (reserved_trailing_byte_count_ > maximum_byte_count_ ||
        bytes_.size() >
            maximum_byte_count_ - reserved_trailing_byte_count_ ||
        additional_byte_count >
            maximum_byte_count_ - reserved_trailing_byte_count_ -
                bytes_.size()) {
      throw std::length_error(
          "the pair-support wire representation exceeds its byte limit");
    }
  }

  std::size_t maximum_byte_count_{};
  std::size_t reserved_trailing_byte_count_{};
  std::vector<std::uint8_t> bytes_;
};

[[nodiscard]] contract::CanonicalId wire_checksum(
    std::span<const std::uint8_t> wire_without_checksum) {
  contract::CanonicalSha256Builder builder;
  builder.update(chunk_checksum_domain);
  builder.update(wire_without_checksum);
  return builder.finalize();
}

[[nodiscard]] std::uint8_t encode_status(
    ExactPairSupportStreamStatus status) {
  switch (status) {
    case ExactPairSupportStreamStatus::complete:
      return 1U;
    case ExactPairSupportStreamStatus::budget_exhausted:
      return 2U;
  }
  throw std::invalid_argument("invalid pair-support stream status");
}

[[nodiscard]] std::uint8_t encode_stop_reason(
    ExactPairSupportStopReason reason) {
  switch (reason) {
    case ExactPairSupportStopReason::none:
      return 0U;
    case ExactPairSupportStopReason::work_unit_limit:
      return 1U;
    case ExactPairSupportStopReason::frontier_entry_limit:
      return 2U;
    case ExactPairSupportStopReason::auxiliary_frontier_entry_limit:
      return 3U;
    case ExactPairSupportStopReason::emitted_record_limit:
      return 4U;
    case ExactPairSupportStopReason::emitted_point_id_reference_limit:
      return 5U;
    case ExactPairSupportStopReason::global_closed_ball_query_limit:
      return 6U;
    case ExactPairSupportStopReason::point_classification_limit:
      return 7U;
  }
  throw std::invalid_argument("invalid pair-support stop reason");
}

[[nodiscard]] std::uint8_t encode_pending_stage(
    ExactPairSupportPendingStage stage) {
  switch (stage) {
    case ExactPairSupportPendingStage::rank_search:
      return 1U;
    case ExactPairSupportPendingStage::expand_product:
      return 2U;
    case ExactPairSupportPendingStage::classify_leaf:
      return 3U;
  }
  throw std::invalid_argument("invalid pair-support pending stage");
}

[[nodiscard]] std::uint8_t encode_record_kind(
    ExactPairSupportStreamChunk::RecordKind kind) {
  switch (kind) {
    case ExactPairSupportStreamChunk::RecordKind::event:
      return 1U;
    case ExactPairSupportStreamChunk::RecordKind::
        relevant_extra_shell_diagnostic:
      return 2U;
  }
  throw std::invalid_argument("invalid pair-support record kind");
}

class PayloadEncoder {
 public:
  PayloadEncoder(
      const ExactPairSupportStreamCodecLimits& limits,
      ByteWriter& writer)
      : limits_(limits), writer_(writer) {}

  void encode(const ExactPairSupportStreamChunk& chunk) {
    encode_manifest(chunk.manifest);
    encode_budget(chunk.budget);
    writer_.u64(chunk.chunk_sequence);
    writer_.size(chunk.first_output_record_index);
    writer_.identifier(chunk.source_checkpoint_digest);
    writer_.identifier(chunk.previous_output_chain_digest);
    writer_.identifier(chunk.output_chain_digest);
    writer_.byte(encode_status(chunk.status));
    writer_.byte(encode_stop_reason(chunk.stop_reason));

    const std::size_t record_count = checked_add_for_encode(
        chunk.events.size(),
        chunk.relevant_extra_shell_diagnostics.size(),
        "the pair-support record count overflows size_t");
    require_count(
        record_count,
        limits_.maximum_record_count,
        "the pair-support record count exceeds its codec limit");
    require_count(
        chunk.record_order.size(),
        limits_.maximum_record_count,
        "the pair-support record-order count exceeds its codec limit");

    writer_.size(chunk.events.size());
    for (const ExactPairSupportEvent& event : chunk.events) {
      encode_event(event);
    }
    writer_.size(chunk.relevant_extra_shell_diagnostics.size());
    for (const ExactPairSupportExtraShellDiagnostic& diagnostic :
         chunk.relevant_extra_shell_diagnostics) {
      encode_diagnostic(diagnostic);
    }
    writer_.size(chunk.record_order.size());
    for (const ExactPairSupportStreamChunk::RecordKind kind :
         chunk.record_order) {
      writer_.byte(encode_record_kind(kind));
    }
    encode_audit(chunk.cumulative_audit_before);
    encode_audit(chunk.cumulative_audit_after);
    encode_checkpoint(chunk.next_checkpoint);
    writer_.boolean(chunk.candidate_prepared);
    writer_.boolean(chunk.no_forbidden_global_structure_materialized);
    writer_.boolean(chunk.hierarchy_reduction_performed);
  }

 private:
  static void require_count(
      std::size_t count,
      std::size_t maximum,
      std::string_view message) {
    if (count > maximum) {
      throw std::length_error(std::string{message});
    }
  }

  void charge_point_id_references(std::size_t count) {
    point_id_reference_count_ = checked_add_for_encode(
        point_id_reference_count_,
        count,
        "the pair-support point-id reference count overflows size_t");
    require_count(
        point_id_reference_count_,
        limits_.maximum_point_id_reference_count,
        "the pair-support point-id reference count exceeds its codec limit");
  }

  void charge_auxiliary_entries(std::size_t count) {
    auxiliary_entry_count_ = checked_add_for_encode(
        auxiliary_entry_count_,
        count,
        "the pair-support auxiliary entry count overflows size_t");
    require_count(
        auxiliary_entry_count_,
        limits_.maximum_auxiliary_entry_count,
        "the pair-support auxiliary entry count exceeds its codec limit");
  }

  void encode_exact_text(std::string_view text) {
    require_count(
        text.size(),
        limits_.maximum_exact_text_byte_count,
        "one exact text exceeds its pair-support codec limit");
    total_exact_text_byte_count_ = checked_add_for_encode(
        total_exact_text_byte_count_,
        text.size(),
        "the total exact text size overflows size_t");
    require_count(
        total_exact_text_byte_count_,
        limits_.maximum_total_exact_text_byte_count,
        "the total exact text exceeds its pair-support codec limit");
    writer_.size(text.size());
    writer_.bytes(std::span<const std::uint8_t>{
        reinterpret_cast<const std::uint8_t*>(text.data()), text.size()});
  }

  void encode_manifest(
      const ExactPairSupportCheckpointManifest& manifest) {
    writer_.u32(manifest.schema_version);
    writer_.u32(manifest.traversal_version);
    writer_.size(manifest.point_count);
    writer_.size(manifest.lbvh_node_count);
    writer_.size(manifest.lbvh_leaf_count);
    writer_.size(manifest.requested_maximum_order);
    writer_.size(manifest.effective_maximum_order);
    writer_.size(manifest.maximum_relevant_closed_rank);
    writer_.identifier(manifest.canonical_cloud_digest);
    writer_.identifier(manifest.lbvh_digest);
    writer_.identifier(manifest.semantic_digest);
  }

  void encode_budget(const ExactPairSupportStreamBudget& budget) {
    writer_.size(budget.maximum_work_unit_count);
    writer_.size(budget.maximum_frontier_entry_count);
    writer_.size(budget.maximum_auxiliary_frontier_entry_count);
    writer_.size(budget.maximum_emitted_record_count);
    writer_.size(budget.maximum_emitted_point_id_reference_count);
    writer_.size(budget.maximum_global_closed_ball_query_count);
    writer_.size(budget.maximum_point_classification_count);
  }

  void encode_frontier_entry(
      const ExactPairSupportFrontierEntry& entry) {
    writer_.u64(entry.first_node_index);
    writer_.u64(entry.second_node_index);
    writer_.u64(entry.first_leaf_begin);
    writer_.u64(entry.first_leaf_end);
    writer_.u64(entry.second_leaf_begin);
    writer_.u64(entry.second_leaf_end);
    if (entry.self_product > 1U) {
      throw std::invalid_argument(
          "pair-support self_product must be encoded as boolean 0 or 1");
    }
    writer_.byte(entry.self_product);
  }

  void encode_witness_entry(
      const ExactPairSupportWitnessNodeEntry& entry) {
    writer_.u64(entry.node_index);
    writer_.u64(entry.leaf_begin);
    writer_.u64(entry.leaf_end);
  }

  void encode_center(const exact::ExactCenter3& center) {
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      encode_exact_text(center.coordinate(axis).canonical_key());
    }
  }

  void encode_level(const exact::ExactLevel& level) {
    encode_exact_text(level.canonical_key());
  }

  void encode_event(const ExactPairSupportEvent& event) {
    charge_point_id_references(
        checked_add_for_encode(
            2U,
            event.interior_ids.size(),
            "one pair-support event point-id count overflows size_t"));
    for (const spatial::PointId point_id : event.support_ids) {
      writer_.u64(point_id);
    }
    encode_center(event.center);
    encode_level(event.squared_level);
    writer_.size(event.interior_ids.size());
    for (const spatial::PointId point_id : event.interior_ids) {
      writer_.u64(point_id);
    }
    writer_.size(event.closed_rank);
    writer_.size(event.exterior_count);
  }

  void encode_diagnostic(
      const ExactPairSupportExtraShellDiagnostic& diagnostic) {
    const std::size_t fixed_reference_count = 3U;
    charge_point_id_references(checked_add_for_encode(
        fixed_reference_count,
        diagnostic.interior_ids.size(),
        "one pair-support diagnostic point-id count overflows size_t"));
    for (const spatial::PointId point_id : diagnostic.support_ids) {
      writer_.u64(point_id);
    }
    encode_center(diagnostic.center);
    encode_level(diagnostic.squared_level);
    writer_.size(diagnostic.interior_ids.size());
    for (const spatial::PointId point_id : diagnostic.interior_ids) {
      writer_.u64(point_id);
    }
    writer_.size(diagnostic.shell_count);
    writer_.u64(diagnostic.canonical_extra_shell_witness_id);
    writer_.size(diagnostic.minimum_possible_closed_rank);
    writer_.size(diagnostic.observed_closed_rank);
    writer_.size(diagnostic.exterior_count);
  }

  void encode_audit(const ExactPairSupportStreamAudit& audit) {
#define MORSEHGP3D_ENCODE_AUDIT_FIELD(field) writer_.size(audit.field)
    MORSEHGP3D_FOR_EACH_PAIR_SUPPORT_AUDIT_FIELD(
        MORSEHGP3D_ENCODE_AUDIT_FIELD);
#undef MORSEHGP3D_ENCODE_AUDIT_FIELD
    writer_.boolean(audit.pair_partition_accounting_certified);
  }

  void encode_checkpoint(const ExactPairSupportCheckpoint& checkpoint) {
    encode_manifest(checkpoint.manifest);
    writer_.u64(checkpoint.next_chunk_sequence);
    writer_.size(checkpoint.output_record_count);
    writer_.identifier(checkpoint.output_chain_digest);
    require_count(
        checkpoint.frontier.size(),
        limits_.maximum_frontier_entry_count,
        "the pair-support frontier exceeds its codec limit");
    writer_.size(checkpoint.frontier.size());
    for (const ExactPairSupportFrontierEntry& entry : checkpoint.frontier) {
      encode_frontier_entry(entry);
    }
    writer_.boolean(checkpoint.pending_product.has_value());
    if (checkpoint.pending_product.has_value()) {
      encode_pending_product(*checkpoint.pending_product);
    }
    encode_audit(checkpoint.cumulative_audit);
    writer_.identifier(checkpoint.checkpoint_digest);
  }

  void encode_pending_product(
      const ExactPairSupportPendingProduct& pending) {
    encode_frontier_entry(pending.product);
    writer_.byte(encode_pending_stage(pending.stage));
    writer_.boolean(pending.rank_search_started);
    charge_auxiliary_entries(pending.witness_frontier.size());
    writer_.size(pending.witness_frontier.size());
    for (const ExactPairSupportWitnessNodeEntry& entry :
         pending.witness_frontier) {
      encode_witness_entry(entry);
    }
    charge_auxiliary_entries(pending.strict_witness_receipts.size());
    writer_.size(pending.strict_witness_receipts.size());
    for (const ExactPairSupportWitnessNodeEntry& entry :
         pending.strict_witness_receipts) {
      encode_witness_entry(entry);
    }
    writer_.boolean(pending.deferred_expansion_node.has_value());
    if (pending.deferred_expansion_node.has_value()) {
      charge_auxiliary_entries(1U);
      encode_witness_entry(*pending.deferred_expansion_node);
    }
    writer_.size(pending.strict_witness_point_count);
  }

  const ExactPairSupportStreamCodecLimits& limits_;
  ByteWriter& writer_;
  std::size_t auxiliary_entry_count_{};
  std::size_t point_id_reference_count_{};
  std::size_t total_exact_text_byte_count_{};
};

struct DecodeFailure {
  DecodeDecision decision;
};

[[noreturn]] void fail_decode(DecodeDecision decision) {
  throw DecodeFailure{decision};
}

class ByteReader {
 public:
  explicit ByteReader(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}

  [[nodiscard]] std::uint8_t byte() {
    if (position_ == bytes_.size()) {
      fail_decode(DecodeDecision::truncated);
    }
    const std::uint8_t value = bytes_[position_];
    ++position_;
    return value;
  }

  [[nodiscard]] bool boolean() {
    const std::uint8_t value = byte();
    if (value > 1U) {
      fail_decode(DecodeDecision::invalid_boolean);
    }
    return value == 1U;
  }

  [[nodiscard]] std::uint32_t u32() {
    std::uint32_t value = 0U;
    for (std::size_t index = 0U; index < 4U; ++index) {
      value = static_cast<std::uint32_t>(
          (value << 8U) | static_cast<std::uint32_t>(byte()));
    }
    return value;
  }

  [[nodiscard]] std::uint64_t u64() {
    std::uint64_t value = 0U;
    for (std::size_t index = 0U; index < 8U; ++index) {
      value = (value << 8U) | static_cast<std::uint64_t>(byte());
    }
    return value;
  }

  [[nodiscard]] std::size_t size() {
    const std::uint64_t value = u64();
    if constexpr (
        std::numeric_limits<std::size_t>::max() <
        std::numeric_limits<std::uint64_t>::max()) {
      if (value > std::numeric_limits<std::size_t>::max()) {
        fail_decode(DecodeDecision::numeric_overflow);
      }
    }
    return static_cast<std::size_t>(value);
  }

  [[nodiscard]] std::span<const std::uint8_t> bytes(std::size_t count) {
    if (count > bytes_.size() - position_) {
      fail_decode(DecodeDecision::truncated);
    }
    const std::span<const std::uint8_t> result =
        bytes_.subspan(position_, count);
    position_ += count;
    return result;
  }

  [[nodiscard]] contract::CanonicalId identifier() {
    std::array<std::uint8_t, contract::CanonicalId::byte_count> value{};
    for (std::uint8_t& byte_value : value) {
      byte_value = byte();
    }
    return contract::CanonicalId{value};
  }

  [[nodiscard]] bool empty() const noexcept {
    return position_ == bytes_.size();
  }

  [[nodiscard]] std::size_t remaining() const noexcept {
    return bytes_.size() - position_;
  }

 private:
  std::span<const std::uint8_t> bytes_;
  std::size_t position_{};
};

[[nodiscard]] ExactPairSupportStreamStatus decode_status(
    std::uint8_t value) {
  switch (value) {
    case 1U:
      return ExactPairSupportStreamStatus::complete;
    case 2U:
      return ExactPairSupportStreamStatus::budget_exhausted;
    default:
      fail_decode(DecodeDecision::invalid_enumeration);
  }
}

[[nodiscard]] ExactPairSupportStopReason decode_stop_reason(
    std::uint8_t value) {
  switch (value) {
    case 0U:
      return ExactPairSupportStopReason::none;
    case 1U:
      return ExactPairSupportStopReason::work_unit_limit;
    case 2U:
      return ExactPairSupportStopReason::frontier_entry_limit;
    case 3U:
      return ExactPairSupportStopReason::auxiliary_frontier_entry_limit;
    case 4U:
      return ExactPairSupportStopReason::emitted_record_limit;
    case 5U:
      return ExactPairSupportStopReason::emitted_point_id_reference_limit;
    case 6U:
      return ExactPairSupportStopReason::global_closed_ball_query_limit;
    case 7U:
      return ExactPairSupportStopReason::point_classification_limit;
    default:
      fail_decode(DecodeDecision::invalid_enumeration);
  }
}

[[nodiscard]] ExactPairSupportPendingStage decode_pending_stage(
    std::uint8_t value) {
  switch (value) {
    case 1U:
      return ExactPairSupportPendingStage::rank_search;
    case 2U:
      return ExactPairSupportPendingStage::expand_product;
    case 3U:
      return ExactPairSupportPendingStage::classify_leaf;
    default:
      fail_decode(DecodeDecision::invalid_enumeration);
  }
}

[[nodiscard]] ExactPairSupportStreamChunk::RecordKind decode_record_kind(
    std::uint8_t value) {
  switch (value) {
    case 1U:
      return ExactPairSupportStreamChunk::RecordKind::event;
    case 2U:
      return ExactPairSupportStreamChunk::RecordKind::
          relevant_extra_shell_diagnostic;
    default:
      fail_decode(DecodeDecision::invalid_enumeration);
  }
}

class PayloadDecoder {
 public:
  PayloadDecoder(
      std::span<const std::uint8_t> payload,
      const ExactPairSupportStreamCodecLimits& limits)
      : limits_(limits), reader_(payload) {}

  [[nodiscard]] ExactPairSupportStreamChunk decode() {
    ExactPairSupportStreamChunk chunk;
    chunk.manifest = decode_manifest();
    chunk.budget = decode_budget();
    chunk.chunk_sequence = reader_.u64();
    chunk.first_output_record_index = reader_.size();
    chunk.source_checkpoint_digest = reader_.identifier();
    chunk.previous_output_chain_digest = reader_.identifier();
    chunk.output_chain_digest = reader_.identifier();
    chunk.status = decode_status(reader_.byte());
    chunk.stop_reason = decode_stop_reason(reader_.byte());

    const std::size_t event_count = bounded_count(
        limits_.maximum_record_count,
        DecodeDecision::count_limit_exceeded);
    require_feasible_count(event_count, 84U);
    chunk.events.reserve(event_count);
    for (std::size_t index = 0U; index < event_count; ++index) {
      chunk.events.push_back(decode_event());
    }

    const std::size_t diagnostic_count = reader_.size();
    charge_bounded_count(
        decoded_record_count_,
        event_count,
        limits_.maximum_record_count,
        DecodeDecision::count_limit_exceeded);
    charge_bounded_count(
        decoded_record_count_,
        diagnostic_count,
        limits_.maximum_record_count,
        DecodeDecision::count_limit_exceeded);
    require_feasible_count(diagnostic_count, 108U);
    chunk.relevant_extra_shell_diagnostics.reserve(diagnostic_count);
    for (std::size_t index = 0U; index < diagnostic_count; ++index) {
      chunk.relevant_extra_shell_diagnostics.push_back(
          decode_diagnostic());
    }

    const std::size_t record_order_count = bounded_count(
        limits_.maximum_record_count,
        DecodeDecision::count_limit_exceeded);
    require_feasible_count(record_order_count, 1U);
    chunk.record_order.reserve(record_order_count);
    for (std::size_t index = 0U; index < record_order_count; ++index) {
      chunk.record_order.push_back(decode_record_kind(reader_.byte()));
    }
    chunk.cumulative_audit_before = decode_audit();
    chunk.cumulative_audit_after = decode_audit();
    chunk.next_checkpoint = decode_checkpoint();
    chunk.candidate_prepared = reader_.boolean();
    chunk.no_forbidden_global_structure_materialized = reader_.boolean();
    chunk.hierarchy_reduction_performed = reader_.boolean();
    if (!reader_.empty()) {
      fail_decode(DecodeDecision::trailing_bytes);
    }
    return chunk;
  }

 private:
  [[nodiscard]] static std::size_t checked_add_for_decode(
      std::size_t left,
      std::size_t right) {
    if (right > std::numeric_limits<std::size_t>::max() - left) {
      fail_decode(DecodeDecision::numeric_overflow);
    }
    return left + right;
  }

  static void charge_bounded_count(
      std::size_t& aggregate,
      std::size_t count,
      std::size_t maximum,
      DecodeDecision decision) {
    if (count > maximum - aggregate) {
      fail_decode(decision);
    }
    aggregate += count;
  }

  [[nodiscard]] std::size_t bounded_count(
      std::size_t maximum,
      DecodeDecision decision) {
    const std::size_t count = reader_.size();
    if (count > maximum) {
      fail_decode(decision);
    }
    return count;
  }

  void require_feasible_count(
      std::size_t count,
      std::size_t minimum_encoded_byte_count_per_entry) const {
    if (minimum_encoded_byte_count_per_entry == 0U ||
        count > reader_.remaining() /
                    minimum_encoded_byte_count_per_entry) {
      fail_decode(DecodeDecision::truncated);
    }
  }

  [[nodiscard]] std::string_view decode_exact_text() {
    const std::size_t byte_count = reader_.size();
    if (byte_count > limits_.maximum_exact_text_byte_count ||
        byte_count > limits_.maximum_total_exact_text_byte_count -
                         total_exact_text_byte_count_) {
      fail_decode(DecodeDecision::exact_text_limit_exceeded);
    }
    total_exact_text_byte_count_ += byte_count;
    const std::span<const std::uint8_t> bytes = reader_.bytes(byte_count);
    return std::string_view{
        reinterpret_cast<const char*>(bytes.data()), bytes.size()};
  }

  [[nodiscard]] exact::ExactRational decode_exact_rational() {
    const std::string_view text = decode_exact_text();
    try {
      return exact::ExactRational::parse_canonical(text);
    } catch (const std::invalid_argument&) {
      fail_decode(DecodeDecision::noncanonical_exact_text);
    } catch (const std::domain_error&) {
      fail_decode(DecodeDecision::noncanonical_exact_text);
    }
  }

  [[nodiscard]] exact::ExactCenter3 decode_center() {
    std::array<exact::ExactRational, 3U> coordinates;
    for (exact::ExactRational& coordinate : coordinates) {
      coordinate = decode_exact_rational();
    }
    try {
      return exact::ExactCenter3{coordinates};
    } catch (const std::invalid_argument&) {
      fail_decode(DecodeDecision::noncanonical_exact_text);
    } catch (const std::domain_error&) {
      fail_decode(DecodeDecision::noncanonical_exact_text);
    }
  }

  [[nodiscard]] exact::ExactLevel decode_level() {
    try {
      return exact::ExactLevel{decode_exact_rational()};
    } catch (const std::invalid_argument&) {
      fail_decode(DecodeDecision::noncanonical_exact_text);
    } catch (const std::domain_error&) {
      fail_decode(DecodeDecision::noncanonical_exact_text);
    }
  }

  [[nodiscard]] ExactPairSupportCheckpointManifest decode_manifest() {
    ExactPairSupportCheckpointManifest manifest;
    manifest.schema_version = reader_.u32();
    manifest.traversal_version = reader_.u32();
    manifest.point_count = reader_.size();
    manifest.lbvh_node_count = reader_.size();
    manifest.lbvh_leaf_count = reader_.size();
    manifest.requested_maximum_order = reader_.size();
    manifest.effective_maximum_order = reader_.size();
    manifest.maximum_relevant_closed_rank = reader_.size();
    manifest.canonical_cloud_digest = reader_.identifier();
    manifest.lbvh_digest = reader_.identifier();
    manifest.semantic_digest = reader_.identifier();
    return manifest;
  }

  [[nodiscard]] ExactPairSupportStreamBudget decode_budget() {
    ExactPairSupportStreamBudget budget;
    budget.maximum_work_unit_count = reader_.size();
    budget.maximum_frontier_entry_count = reader_.size();
    budget.maximum_auxiliary_frontier_entry_count = reader_.size();
    budget.maximum_emitted_record_count = reader_.size();
    budget.maximum_emitted_point_id_reference_count = reader_.size();
    budget.maximum_global_closed_ball_query_count = reader_.size();
    budget.maximum_point_classification_count = reader_.size();
    return budget;
  }

  [[nodiscard]] ExactPairSupportFrontierEntry decode_frontier_entry() {
    ExactPairSupportFrontierEntry entry;
    entry.first_node_index = reader_.u64();
    entry.second_node_index = reader_.u64();
    entry.first_leaf_begin = reader_.u64();
    entry.first_leaf_end = reader_.u64();
    entry.second_leaf_begin = reader_.u64();
    entry.second_leaf_end = reader_.u64();
    entry.self_product = reader_.boolean() ? 1U : 0U;
    return entry;
  }

  [[nodiscard]] ExactPairSupportWitnessNodeEntry decode_witness_entry() {
    ExactPairSupportWitnessNodeEntry entry;
    entry.node_index = reader_.u64();
    entry.leaf_begin = reader_.u64();
    entry.leaf_end = reader_.u64();
    return entry;
  }

  [[nodiscard]] ExactPairSupportEvent decode_event() {
    ExactPairSupportEvent event;
    charge_bounded_count(
        point_id_reference_count_,
        2U,
        limits_.maximum_point_id_reference_count,
        DecodeDecision::point_id_reference_limit_exceeded);
    for (spatial::PointId& point_id : event.support_ids) {
      point_id = reader_.u64();
    }
    event.center = decode_center();
    event.squared_level = decode_level();
    const std::size_t interior_count = reader_.size();
    charge_bounded_count(
        point_id_reference_count_,
        interior_count,
        limits_.maximum_point_id_reference_count,
        DecodeDecision::point_id_reference_limit_exceeded);
    require_feasible_count(interior_count, 8U);
    event.interior_ids.reserve(interior_count);
    for (std::size_t index = 0U; index < interior_count; ++index) {
      event.interior_ids.push_back(reader_.u64());
    }
    event.closed_rank = reader_.size();
    event.exterior_count = reader_.size();
    return event;
  }

  [[nodiscard]] ExactPairSupportExtraShellDiagnostic decode_diagnostic() {
    ExactPairSupportExtraShellDiagnostic diagnostic;
    charge_bounded_count(
        point_id_reference_count_,
        3U,
        limits_.maximum_point_id_reference_count,
        DecodeDecision::point_id_reference_limit_exceeded);
    for (spatial::PointId& point_id : diagnostic.support_ids) {
      point_id = reader_.u64();
    }
    diagnostic.center = decode_center();
    diagnostic.squared_level = decode_level();
    const std::size_t interior_count = reader_.size();
    charge_bounded_count(
        point_id_reference_count_,
        interior_count,
        limits_.maximum_point_id_reference_count,
        DecodeDecision::point_id_reference_limit_exceeded);
    require_feasible_count(interior_count, 8U);
    diagnostic.interior_ids.reserve(interior_count);
    for (std::size_t index = 0U; index < interior_count; ++index) {
      diagnostic.interior_ids.push_back(reader_.u64());
    }
    diagnostic.shell_count = reader_.size();
    diagnostic.canonical_extra_shell_witness_id = reader_.u64();
    diagnostic.minimum_possible_closed_rank = reader_.size();
    diagnostic.observed_closed_rank = reader_.size();
    diagnostic.exterior_count = reader_.size();
    return diagnostic;
  }

  [[nodiscard]] ExactPairSupportStreamAudit decode_audit() {
    ExactPairSupportStreamAudit audit;
#define MORSEHGP3D_DECODE_AUDIT_FIELD(field) audit.field = reader_.size()
    MORSEHGP3D_FOR_EACH_PAIR_SUPPORT_AUDIT_FIELD(
        MORSEHGP3D_DECODE_AUDIT_FIELD);
#undef MORSEHGP3D_DECODE_AUDIT_FIELD
    audit.pair_partition_accounting_certified = reader_.boolean();
    return audit;
  }

  [[nodiscard]] ExactPairSupportCheckpoint decode_checkpoint() {
    ExactPairSupportCheckpoint checkpoint;
    checkpoint.manifest = decode_manifest();
    checkpoint.next_chunk_sequence = reader_.u64();
    checkpoint.output_record_count = reader_.size();
    checkpoint.output_chain_digest = reader_.identifier();
    const std::size_t frontier_count = bounded_count(
        limits_.maximum_frontier_entry_count,
        DecodeDecision::frontier_entry_limit_exceeded);
    require_feasible_count(frontier_count, 49U);
    checkpoint.frontier.reserve(frontier_count);
    for (std::size_t index = 0U; index < frontier_count; ++index) {
      checkpoint.frontier.push_back(decode_frontier_entry());
    }
    if (reader_.boolean()) {
      checkpoint.pending_product = decode_pending_product();
    }
    checkpoint.cumulative_audit = decode_audit();
    checkpoint.checkpoint_digest = reader_.identifier();
    return checkpoint;
  }

  [[nodiscard]] ExactPairSupportPendingProduct decode_pending_product() {
    ExactPairSupportPendingProduct pending;
    pending.product = decode_frontier_entry();
    pending.stage = decode_pending_stage(reader_.byte());
    pending.rank_search_started = reader_.boolean();

    const std::size_t witness_count = reader_.size();
    charge_bounded_count(
        auxiliary_entry_count_,
        witness_count,
        limits_.maximum_auxiliary_entry_count,
        DecodeDecision::auxiliary_entry_limit_exceeded);
    require_feasible_count(witness_count, 24U);
    pending.witness_frontier.reserve(witness_count);
    for (std::size_t index = 0U; index < witness_count; ++index) {
      pending.witness_frontier.push_back(decode_witness_entry());
    }

    const std::size_t receipt_count = reader_.size();
    charge_bounded_count(
        auxiliary_entry_count_,
        receipt_count,
        limits_.maximum_auxiliary_entry_count,
        DecodeDecision::auxiliary_entry_limit_exceeded);
    require_feasible_count(receipt_count, 24U);
    pending.strict_witness_receipts.reserve(receipt_count);
    for (std::size_t index = 0U; index < receipt_count; ++index) {
      pending.strict_witness_receipts.push_back(decode_witness_entry());
    }
    if (reader_.boolean()) {
      charge_bounded_count(
          auxiliary_entry_count_,
          1U,
          limits_.maximum_auxiliary_entry_count,
          DecodeDecision::auxiliary_entry_limit_exceeded);
      pending.deferred_expansion_node = decode_witness_entry();
    }
    pending.strict_witness_point_count = reader_.size();
    return pending;
  }

  const ExactPairSupportStreamCodecLimits& limits_;
  ByteReader reader_;
  std::size_t auxiliary_entry_count_{};
  std::size_t decoded_record_count_{};
  std::size_t point_id_reference_count_{};
  std::size_t total_exact_text_byte_count_{};
};

#undef MORSEHGP3D_FOR_EACH_PAIR_SUPPORT_AUDIT_FIELD

}  // namespace

std::vector<std::uint8_t> encode_exact_pair_support_stream_chunk(
    const ExactPairSupportStreamChunk& chunk,
    const ExactPairSupportStreamCodecLimits& limits) {
  require_finite_limits(limits);
  constexpr std::size_t minimum_wire_byte_count =
      chunk_header_byte_count + chunk_checksum_byte_count;
  if (limits.maximum_encoded_byte_count < minimum_wire_byte_count) {
    throw std::length_error(
        "the pair-support wire representation exceeds its byte limit");
  }

  ByteWriter wire{
      limits.maximum_encoded_byte_count, chunk_checksum_byte_count};
  wire.bytes(chunk_magic);
  wire.u32(pair_support_stream_chunk_codec_version);
  wire.byte(chunk_wire_kind);
  wire.byte(chunk_wire_flags);
  const std::size_t payload_length_offset = wire.byte_count();
  wire.u64(0U);
  const std::size_t payload_offset = wire.byte_count();
  PayloadEncoder payload_encoder{limits, wire};
  payload_encoder.encode(chunk);
  const std::size_t payload_byte_count = wire.byte_count() - payload_offset;
  wire.overwrite_u64(
      payload_length_offset,
      checked_u64_for_encode(
          payload_byte_count,
          "the pair-support payload size does not fit uint64"));
  const contract::CanonicalId checksum = wire_checksum(wire.data());
  wire.append_reserved_identifier(checksum);
  return std::move(wire).release();
}

ExactPairSupportStreamDecodeResult decode_exact_pair_support_stream_chunk(
    std::span<const std::uint8_t> encoded,
    const ExactPairSupportStreamCodecLimits& limits) {
  require_finite_limits(limits);
  if (encoded.size() > limits.maximum_encoded_byte_count) {
    return ExactPairSupportStreamDecodeResult{
        DecodeDecision::encoded_byte_limit_exceeded, std::nullopt};
  }

  try {
    if (encoded.size() <
        chunk_header_byte_count + chunk_checksum_byte_count) {
      fail_decode(DecodeDecision::truncated);
    }
    ByteReader envelope{encoded};
    for (const std::uint8_t expected : chunk_magic) {
      if (envelope.byte() != expected) {
        fail_decode(DecodeDecision::invalid_magic);
      }
    }
    if (envelope.u32() != pair_support_stream_chunk_codec_version) {
      fail_decode(DecodeDecision::unsupported_version);
    }
    if (envelope.byte() != chunk_wire_kind) {
      fail_decode(DecodeDecision::unsupported_kind);
    }
    if (envelope.byte() != chunk_wire_flags) {
      fail_decode(DecodeDecision::unsupported_flags);
    }
    const std::uint64_t payload_byte_count_u64 = envelope.u64();
    if constexpr (
        std::numeric_limits<std::size_t>::max() <
        std::numeric_limits<std::uint64_t>::max()) {
      if (payload_byte_count_u64 >
          std::numeric_limits<std::size_t>::max()) {
        fail_decode(DecodeDecision::numeric_overflow);
      }
    }
    const std::size_t payload_byte_count =
        static_cast<std::size_t>(payload_byte_count_u64);
    const std::size_t maximum_payload_byte_count =
        std::numeric_limits<std::size_t>::max() -
        chunk_header_byte_count - chunk_checksum_byte_count;
    if (payload_byte_count > maximum_payload_byte_count) {
      fail_decode(DecodeDecision::numeric_overflow);
    }
    const std::size_t expected_total_byte_count =
        chunk_header_byte_count + payload_byte_count +
        chunk_checksum_byte_count;
    if (expected_total_byte_count > encoded.size()) {
      fail_decode(DecodeDecision::payload_length_mismatch);
    }
    if (expected_total_byte_count < encoded.size()) {
      fail_decode(DecodeDecision::trailing_bytes);
    }

    const std::size_t checksum_offset =
        chunk_header_byte_count + payload_byte_count;
    const contract::CanonicalId expected_checksum =
        wire_checksum(encoded.first(checksum_offset));
    const std::span<const std::uint8_t> observed_checksum =
        encoded.subspan(checksum_offset, chunk_checksum_byte_count);
    std::uint8_t checksum_difference = 0U;
    for (std::size_t index = 0U;
         index < chunk_checksum_byte_count;
         ++index) {
      checksum_difference = static_cast<std::uint8_t>(
          checksum_difference |
          static_cast<std::uint8_t>(
              expected_checksum.bytes()[index] ^ observed_checksum[index]));
    }
    if (checksum_difference != 0U) {
      fail_decode(DecodeDecision::checksum_mismatch);
    }

    PayloadDecoder decoder{
        encoded.subspan(chunk_header_byte_count, payload_byte_count),
        limits};
    ExactPairSupportStreamChunk chunk = decoder.decode();
    return ExactPairSupportStreamDecodeResult{
        DecodeDecision::accepted, std::move(chunk)};
  } catch (const DecodeFailure& failure) {
    return ExactPairSupportStreamDecodeResult{
        failure.decision, std::nullopt};
  }
}

}  // namespace morsehgp3d::hierarchy
