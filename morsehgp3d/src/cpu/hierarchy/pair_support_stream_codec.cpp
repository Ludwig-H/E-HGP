#include "morsehgp3d/hierarchy/pair_support_stream_codec.hpp"

#include "morsehgp3d/contract/canonical_id.hpp"

#include <algorithm>
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

#if defined(__unix__) || defined(__APPLE__)
#include <cerrno>
#include <fcntl.h>
#include <system_error>
#include <sys/stat.h>
#include <unistd.h>
#endif

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

[[nodiscard]] bool checksum_input_fits_sha256(
    std::size_t wire_without_checksum_byte_count) noexcept {
  constexpr std::uint64_t maximum_sha256_byte_count =
      std::numeric_limits<std::uint64_t>::max() / 8U;
  if (chunk_checksum_domain.size() > maximum_sha256_byte_count) {
    return false;
  }
  return wire_without_checksum_byte_count <=
         maximum_sha256_byte_count - chunk_checksum_domain.size();
}

void require_checksum_input_fits_sha256(
    std::size_t wire_without_checksum_byte_count) {
  if (!checksum_input_fits_sha256(wire_without_checksum_byte_count)) {
    throw std::length_error(
        "the pair-support checksum input exceeds the SHA-256 bit-length limit");
  }
}

class ByteWriter {
 public:
  explicit ByteWriter(std::size_t maximum_byte_count)
      : maximum_byte_count_(maximum_byte_count) {}

  virtual ~ByteWriter() = default;

  ByteWriter(const ByteWriter&) = delete;
  ByteWriter& operator=(const ByteWriter&) = delete;

  void byte(std::uint8_t value) {
    bytes(std::span<const std::uint8_t>{&value, 1U});
  }

  void boolean(bool value) {
    byte(value ? std::uint8_t{1U} : std::uint8_t{0U});
  }

  void u32(std::uint32_t value) {
    std::array<std::uint8_t, 4U> encoded{};
    for (std::size_t index = 0U; index < 4U; ++index) {
      const std::size_t shift = (3U - index) * 8U;
      encoded[index] = static_cast<std::uint8_t>(value >> shift);
    }
    bytes(encoded);
  }

  void u64(std::uint64_t value) {
    std::array<std::uint8_t, 8U> encoded{};
    for (std::size_t index = 0U; index < 8U; ++index) {
      const std::size_t shift = (7U - index) * 8U;
      encoded[index] = static_cast<std::uint8_t>(value >> shift);
    }
    bytes(encoded);
  }

  void size(std::size_t value) {
    u64(checked_u64_for_encode(
        value, "a pair-support codec size does not fit uint64"));
  }

  void bytes(std::span<const std::uint8_t> values) {
    require_capacity(values.size());
    append(values);
    byte_count_ += values.size();
  }

  void identifier(const contract::CanonicalId& identifier) {
    bytes(identifier.bytes());
  }

  [[nodiscard]] std::size_t byte_count() const noexcept {
    return byte_count_;
  }

 protected:
  virtual void append(std::span<const std::uint8_t> values) = 0;

 private:
  void require_capacity(std::size_t additional_byte_count) const {
    if (byte_count_ > maximum_byte_count_ ||
        additional_byte_count > maximum_byte_count_ - byte_count_) {
      throw std::length_error(
          "the pair-support wire representation exceeds its byte limit");
    }
  }

  std::size_t maximum_byte_count_{};
  std::size_t byte_count_{};
};

class CountWriter final : public ByteWriter {
 public:
  explicit CountWriter(std::size_t maximum_byte_count)
      : ByteWriter(maximum_byte_count) {}

 private:
  void append(std::span<const std::uint8_t>) override {}
};

class VectorWriter final : public ByteWriter {
 public:
  VectorWriter(
      std::vector<std::uint8_t>& output,
      std::size_t maximum_byte_count,
      contract::CanonicalSha256Builder& checksum_builder)
      : ByteWriter(maximum_byte_count),
        output_(output),
        checksum_builder_(checksum_builder) {}

 private:
  void append(std::span<const std::uint8_t> values) override {
    checksum_builder_.update(values);
    output_.insert(output_.end(), values.begin(), values.end());
  }

  std::vector<std::uint8_t>& output_;
  contract::CanonicalSha256Builder& checksum_builder_;
};

[[nodiscard]] contract::CanonicalId wire_checksum(
    std::span<const std::uint8_t> wire_without_checksum) {
  require_checksum_input_fits_sha256(wire_without_checksum.size());
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

[[nodiscard]] bool add_within_limit(
    std::size_t value,
    std::size_t limit,
    std::size_t& total) noexcept {
  if (total > limit || value > limit - total) {
    return false;
  }
  total += value;
  return true;
}

[[nodiscard]] bool add_decimal_digit_upper_bound(
    const exact::BigInt& value,
    std::size_t limit,
    std::size_t& total) {
  if (value == 0) {
    return add_within_limit(1U, limit, total);
  }
  const exact::BigInt magnitude = value < 0 ? -value : value;
  const std::size_t most_significant_bit =
      boost::multiprecision::msb(magnitude);
  if (most_significant_bit == std::numeric_limits<std::size_t>::max()) {
    return false;
  }
  const std::size_t bit_count = most_significant_bit + 1U;

  // 30103 / 100000 is a strict upper approximation of log10(2).
  // Splitting the product avoids overflow; the remainder product is < 2^32.
  constexpr std::size_t numerator = 30103U;
  constexpr std::size_t denominator = 100000U;
  const std::size_t quotient = bit_count / denominator;
  const std::size_t remainder = bit_count % denominator;
  if (quotient > std::numeric_limits<std::size_t>::max() / numerator) {
    return false;
  }
  const std::size_t whole = quotient * numerator;
  const std::size_t remainder_product = remainder * numerator;
  const std::size_t fractional =
      remainder_product / denominator +
      (remainder_product % denominator == 0U ? 0U : 1U);
  if (whole > std::numeric_limits<std::size_t>::max() - fractional) {
    return false;
  }
  return add_within_limit(whole + fractional, limit, total);
}

[[nodiscard]] bool canonical_rational_allocation_is_bounded(
    const exact::ExactRational& value,
    std::size_t exact_text_byte_limit) {
  // The bit-length estimate is an upper bound but can differ by one decimal
  // digit for each integer.  Consequently canonical_key() is called only when
  // its result is guaranteed not to exceed the effective cap by more than two
  // bytes; the exact post-check still rejects every byte above the public cap.
  const std::size_t tolerated_limit =
      exact_text_byte_limit > std::numeric_limits<std::size_t>::max() - 2U
          ? exact_text_byte_limit
          : exact_text_byte_limit + 2U;
  std::size_t upper_byte_count = 0U;
  if (value.numerator() < 0 &&
      !add_within_limit(1U, tolerated_limit, upper_byte_count)) {
    return false;
  }
  if (!add_decimal_digit_upper_bound(
          value.numerator(), tolerated_limit, upper_byte_count) ||
      !add_within_limit(1U, tolerated_limit, upper_byte_count) ||
      !add_decimal_digit_upper_bound(
          value.denominator(), tolerated_limit, upper_byte_count)) {
    return false;
  }
  return true;
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

  void encode_exact_rational(const exact::ExactRational& value) {
    const std::size_t remaining_total_limit =
        limits_.maximum_total_exact_text_byte_count -
        total_exact_text_byte_count_;
    const std::size_t effective_limit = std::min(
        limits_.maximum_exact_text_byte_count, remaining_total_limit);
    if (!canonical_rational_allocation_is_bounded(value, effective_limit)) {
      throw std::length_error(
          "one exact text exceeds its pair-support codec limit");
    }
    encode_exact_text(value.canonical_key());
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
      encode_exact_rational(center.coordinate(axis));
    }
  }

  void encode_level(const exact::ExactLevel& level) {
    encode_exact_rational(level.rational());
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

class SpanByteReader {
 public:
  explicit SpanByteReader(std::span<const std::uint8_t> bytes)
      : bytes_(bytes) {}

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

  [[nodiscard]] std::string_view exact_text(std::size_t count) {
    if (count > bytes_.size() - position_) {
      fail_decode(DecodeDecision::truncated);
    }
    const std::span<const std::uint8_t> result =
        bytes_.subspan(position_, count);
    position_ += count;
    return std::string_view{
        reinterpret_cast<const char*>(result.data()), result.size()};
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

template <typename Reader>
class PayloadDecoder {
 public:
  PayloadDecoder(
      Reader& reader,
      const ExactPairSupportStreamCodecLimits& limits)
      : limits_(limits), reader_(reader) {}

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
    return reader_.exact_text(byte_count);
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
  Reader& reader_;
  std::size_t auxiliary_entry_count_{};
  std::size_t decoded_record_count_{};
  std::size_t point_id_reference_count_{};
  std::size_t total_exact_text_byte_count_{};
};

[[nodiscard]] std::size_t count_payload_bytes(
    const ExactPairSupportStreamChunk& chunk,
    const ExactPairSupportStreamCodecLimits& limits) {
  constexpr std::size_t envelope_overhead =
      chunk_header_byte_count + chunk_checksum_byte_count;
  if (limits.maximum_encoded_byte_count < envelope_overhead) {
    throw std::length_error(
        "the pair-support wire representation exceeds its byte limit");
  }
  CountWriter counter{limits.maximum_encoded_byte_count - envelope_overhead};
  PayloadEncoder payload_encoder{limits, counter};
  payload_encoder.encode(chunk);
  return counter.byte_count();
}

void encode_envelope_header(
    ByteWriter& writer,
    std::size_t payload_byte_count) {
  writer.bytes(chunk_magic);
  writer.u32(pair_support_stream_chunk_codec_version);
  writer.byte(chunk_wire_kind);
  writer.byte(chunk_wire_flags);
  writer.u64(checked_u64_for_encode(
      payload_byte_count,
      "the pair-support payload size does not fit uint64"));
}

void encode_counted_payload(
    ByteWriter& writer,
    const ExactPairSupportStreamChunk& chunk,
    const ExactPairSupportStreamCodecLimits& limits,
    std::size_t payload_byte_count) {
  const std::size_t payload_offset = writer.byte_count();
  PayloadEncoder payload_encoder{limits, writer};
  payload_encoder.encode(chunk);
  if (writer.byte_count() - payload_offset != payload_byte_count) {
    throw std::logic_error(
        "the counted pair-support payload changed during encoding");
  }
}

[[nodiscard]] bool equal_checksum(
    const contract::CanonicalId& expected,
    std::span<const std::uint8_t> observed) noexcept {
  if (observed.size() != contract::CanonicalId::byte_count) {
    return false;
  }
  std::uint8_t difference = 0U;
  for (std::size_t index = 0U; index < observed.size(); ++index) {
    difference = static_cast<std::uint8_t>(
        difference |
        static_cast<std::uint8_t>(
            expected.bytes()[index] ^ observed[index]));
  }
  return difference == 0U;
}

#if defined(__unix__) || defined(__APPLE__)

[[noreturn]] void throw_last_system_error(std::string_view message) {
  throw std::system_error(errno, std::generic_category(), std::string{message});
}

[[nodiscard]] off_t checked_file_offset(std::size_t offset) {
  if constexpr (
      std::numeric_limits<off_t>::max() <
      std::numeric_limits<std::size_t>::max()) {
    if (offset > static_cast<std::size_t>(
                     std::numeric_limits<off_t>::max())) {
      throw std::length_error(
          "a pair-support wire offset does not fit off_t");
    }
  }
  return static_cast<off_t>(offset);
}

void positional_write_all(
    int descriptor,
    std::span<const std::uint8_t> values,
    std::size_t offset) {
  std::size_t written = 0U;
  while (written < values.size()) {
    const std::size_t absolute_offset = checked_add_for_encode(
        offset,
        written,
        "a pair-support fd write offset overflows size_t");
    const ssize_t count = ::pwrite(
        descriptor,
        values.data() + written,
        values.size() - written,
        checked_file_offset(absolute_offset));
    if (count < 0) {
      if (errno == EINTR) {
        continue;
      }
      throw_last_system_error("cannot write the pair-support wire");
    }
    if (count == 0) {
      throw std::runtime_error(
          "a pair-support positional write made no progress");
    }
    written = checked_add_for_encode(
        written,
        static_cast<std::size_t>(count),
        "a pair-support fd write count overflows size_t");
  }
}

class BufferedFdWriter final : public ByteWriter {
 public:
  BufferedFdWriter(
      int descriptor,
      std::size_t maximum_byte_count,
      contract::CanonicalSha256Builder& checksum_builder)
      : ByteWriter(maximum_byte_count),
        descriptor_(descriptor),
        checksum_builder_(checksum_builder) {}

  ~BufferedFdWriter() override = default;

  void flush() {
    if (buffered_byte_count_ == 0U) {
      return;
    }
    positional_write_all(
        descriptor_,
        std::span<const std::uint8_t>{buffer_}.first(buffered_byte_count_),
        file_offset_);
    file_offset_ = checked_add_for_encode(
        file_offset_,
        buffered_byte_count_,
        "a pair-support fd writer offset overflows size_t");
    buffered_byte_count_ = 0U;
  }

  void append_unhashed_checksum(const contract::CanonicalId& checksum) {
    flush();
    positional_write_all(descriptor_, checksum.bytes(), file_offset_);
    file_offset_ = checked_add_for_encode(
        file_offset_,
        checksum.bytes().size(),
        "a pair-support fd checksum offset overflows size_t");
  }

  [[nodiscard]] std::size_t file_byte_count() const noexcept {
    return file_offset_ + buffered_byte_count_;
  }

 private:
  void append(std::span<const std::uint8_t> values) override {
    checksum_builder_.update(values);
    std::size_t copied = 0U;
    while (copied < values.size()) {
      if (buffered_byte_count_ == buffer_.size()) {
        flush();
      }
      const std::size_t count = std::min(
          buffer_.size() - buffered_byte_count_, values.size() - copied);
      std::copy_n(
          values.begin() + static_cast<std::ptrdiff_t>(copied),
          count,
          buffer_.begin() +
              static_cast<std::ptrdiff_t>(buffered_byte_count_));
      copied += count;
      buffered_byte_count_ += count;
    }
  }

  int descriptor_{};
  contract::CanonicalSha256Builder& checksum_builder_;
  std::array<std::uint8_t, pair_support_stream_fd_buffer_byte_count> buffer_{};
  std::size_t file_offset_{};
  std::size_t buffered_byte_count_{};
};

[[nodiscard]] bool fd_has_unsupported_status_flags(int flags) noexcept {
  bool unsupported = (flags & O_APPEND) != 0;
#ifdef O_DIRECT
  unsupported = unsupported || (flags & O_DIRECT) != 0;
#endif
  return unsupported;
}

[[nodiscard]] ssize_t positional_read_probe(int descriptor) noexcept {
  std::uint8_t byte{};
  for (;;) {
    const ssize_t count = ::pread(descriptor, &byte, 1U, off_t{0});
    if (count < 0 && errno == EINTR) {
      continue;
    }
    return count;
  }
}

[[nodiscard]] struct stat require_empty_output_descriptor(int descriptor) {
  const int flags = ::fcntl(descriptor, F_GETFL);
  if (flags < 0) {
    throw_last_system_error("cannot inspect the pair-support output fd");
  }
  if ((flags & O_ACCMODE) != O_RDWR ||
      fd_has_unsupported_status_flags(flags)) {
    throw std::invalid_argument(
        "the pair-support output fd must be O_RDWR without O_APPEND/O_DIRECT");
  }
  struct stat metadata {};
  if (::fstat(descriptor, &metadata) != 0) {
    throw_last_system_error("cannot inspect the pair-support output file");
  }
  if (!S_ISREG(metadata.st_mode) || metadata.st_size != 0) {
    throw std::invalid_argument(
        "the pair-support output fd must name an empty regular file");
  }
  if (positional_read_probe(descriptor) != 0) {
    throw std::invalid_argument(
        "the pair-support output fd must be seekable and empty");
  }
  return metadata;
}

[[nodiscard]] bool same_file_image_metadata(
    const struct stat& left,
    const struct stat& right) noexcept {
  return S_ISREG(right.st_mode) && left.st_dev == right.st_dev &&
         left.st_ino == right.st_ino && left.st_size == right.st_size;
}

[[nodiscard]] bool metadata_size_equals(
    const struct stat& metadata,
    std::size_t size) noexcept {
  return metadata.st_size >= 0 &&
         static_cast<std::uintmax_t>(metadata.st_size) == size;
}

[[nodiscard]] bool inspect_read_descriptor(
    int descriptor,
    struct stat& metadata) noexcept {
  const int flags = ::fcntl(descriptor, F_GETFL);
  if (flags < 0 || (flags & O_ACCMODE) == O_WRONLY ||
      fd_has_unsupported_status_flags(flags) ||
      ::fstat(descriptor, &metadata) != 0 ||
      !S_ISREG(metadata.st_mode) || metadata.st_size < 0) {
    return false;
  }
  return positional_read_probe(descriptor) >= 0;
}

void positional_read_exact_for_decode(
    int descriptor,
    std::span<std::uint8_t> output,
    std::size_t offset) {
  std::size_t read_count = 0U;
  while (read_count < output.size()) {
    const std::size_t absolute_offset = offset + read_count;
    if (absolute_offset < offset) {
      fail_decode(DecodeDecision::numeric_overflow);
    }
    off_t file_offset{};
    try {
      file_offset = checked_file_offset(absolute_offset);
    } catch (const std::length_error&) {
      fail_decode(DecodeDecision::numeric_overflow);
    }
    const ssize_t count = ::pread(
        descriptor,
        output.data() + read_count,
        output.size() - read_count,
        file_offset);
    if (count < 0) {
      if (errno == EINTR) {
        continue;
      }
      fail_decode(DecodeDecision::file_io_error);
    }
    if (count == 0) {
      fail_decode(DecodeDecision::file_changed);
    }
    read_count += static_cast<std::size_t>(count);
  }
}

class FdByteReader {
 public:
  FdByteReader(
      int descriptor,
      std::size_t file_offset,
      std::size_t byte_count,
      contract::CanonicalSha256Builder& checksum_builder,
      std::span<std::uint8_t> scratch)
      : descriptor_(descriptor),
        initial_file_offset_(file_offset),
        byte_count_(byte_count),
        checksum_builder_(checksum_builder),
        buffer_(scratch) {
    if (buffer_.size() != pair_support_stream_fd_buffer_byte_count) {
      throw std::logic_error(
          "the pair-support fd reader scratch must be exactly 64 KiB");
    }
  }

  [[nodiscard]] std::uint8_t byte() {
    std::uint8_t value{};
    read_into(std::span<std::uint8_t>{&value, 1U});
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
    std::array<std::uint8_t, 4U> encoded{};
    read_into(encoded);
    std::uint32_t value = 0U;
    for (const std::uint8_t byte_value : encoded) {
      value = static_cast<std::uint32_t>(
          (value << 8U) | static_cast<std::uint32_t>(byte_value));
    }
    return value;
  }

  [[nodiscard]] std::uint64_t u64() {
    std::array<std::uint8_t, 8U> encoded{};
    read_into(encoded);
    std::uint64_t value = 0U;
    for (const std::uint8_t byte_value : encoded) {
      value = (value << 8U) | static_cast<std::uint64_t>(byte_value);
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

  [[nodiscard]] contract::CanonicalId identifier() {
    std::array<std::uint8_t, contract::CanonicalId::byte_count> value{};
    read_into(value);
    return contract::CanonicalId{value};
  }

  [[nodiscard]] std::string_view exact_text(std::size_t count) {
    if (count > remaining()) {
      fail_decode(DecodeDecision::truncated);
    }
    exact_text_.resize(count);
    read_into(std::span<std::uint8_t>{
        reinterpret_cast<std::uint8_t*>(exact_text_.data()),
        exact_text_.size()});
    return exact_text_;
  }

  [[nodiscard]] bool empty() const noexcept {
    return position_ == byte_count_;
  }

  [[nodiscard]] std::size_t remaining() const noexcept {
    return byte_count_ - position_;
  }

 private:
  void refill() {
    if (position_ == byte_count_) {
      fail_decode(DecodeDecision::truncated);
    }
    const std::size_t count = std::min(buffer_.size(), remaining());
    positional_read_exact_for_decode(
        descriptor_,
        buffer_.first(count),
        initial_file_offset_ + position_);
    buffered_position_ = 0U;
    buffered_byte_count_ = count;
  }

  void read_into(std::span<std::uint8_t> output) {
    if (output.size() > remaining()) {
      fail_decode(DecodeDecision::truncated);
    }
    std::size_t copied = 0U;
    while (copied < output.size()) {
      if (buffered_position_ == buffered_byte_count_) {
        refill();
      }
      const std::size_t count = std::min(
          buffered_byte_count_ - buffered_position_,
          output.size() - copied);
      std::copy_n(
          buffer_.begin() +
              static_cast<std::ptrdiff_t>(buffered_position_),
          count,
          output.begin() + static_cast<std::ptrdiff_t>(copied));
      checksum_builder_.update(std::span<const std::uint8_t>{
          output.data() + copied, count});
      buffered_position_ += count;
      position_ += count;
      copied += count;
    }
  }

  int descriptor_{};
  std::size_t initial_file_offset_{};
  std::size_t byte_count_{};
  contract::CanonicalSha256Builder& checksum_builder_;
  std::size_t position_{};
  std::span<std::uint8_t> buffer_;
  std::size_t buffered_position_{};
  std::size_t buffered_byte_count_{};
  std::string exact_text_;
};

#endif

#undef MORSEHGP3D_FOR_EACH_PAIR_SUPPORT_AUDIT_FIELD

}  // namespace

std::vector<std::uint8_t> encode_exact_pair_support_stream_chunk(
    const ExactPairSupportStreamChunk& chunk,
    const ExactPairSupportStreamCodecLimits& limits) {
  require_finite_limits(limits);
  const std::size_t payload_byte_count =
      count_payload_bytes(chunk, limits);
  const std::size_t checksum_offset = checked_add_for_encode(
      chunk_header_byte_count,
      payload_byte_count,
      "the pair-support wire size overflows size_t");
  const std::size_t total_byte_count = checked_add_for_encode(
      checksum_offset,
      chunk_checksum_byte_count,
      "the pair-support wire size overflows size_t");
  if (total_byte_count > limits.maximum_encoded_byte_count) {
    throw std::length_error(
        "the pair-support wire representation exceeds its byte limit");
  }
  require_checksum_input_fits_sha256(checksum_offset);

  std::vector<std::uint8_t> encoded;
  encoded.reserve(total_byte_count);
  contract::CanonicalSha256Builder checksum_builder;
  checksum_builder.update(chunk_checksum_domain);
  VectorWriter writer{encoded, checksum_offset, checksum_builder};
  encode_envelope_header(writer, payload_byte_count);
  encode_counted_payload(
      writer, chunk, limits, payload_byte_count);
  if (writer.byte_count() != checksum_offset) {
    throw std::logic_error(
        "the pair-support wire size changed after counting");
  }
  const contract::CanonicalId checksum = checksum_builder.finalize();
  encoded.insert(
      encoded.end(), checksum.bytes().begin(), checksum.bytes().end());
  return encoded;
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
    SpanByteReader envelope{encoded};
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
    if (!checksum_input_fits_sha256(checksum_offset)) {
      fail_decode(DecodeDecision::numeric_overflow);
    }
    const contract::CanonicalId expected_checksum =
        wire_checksum(encoded.first(checksum_offset));
    const std::span<const std::uint8_t> observed_checksum =
        encoded.subspan(checksum_offset, chunk_checksum_byte_count);
    if (!equal_checksum(expected_checksum, observed_checksum)) {
      fail_decode(DecodeDecision::checksum_mismatch);
    }

    SpanByteReader payload_reader{encoded.subspan(
        chunk_header_byte_count, payload_byte_count)};
    PayloadDecoder decoder{payload_reader, limits};
    ExactPairSupportStreamChunk chunk = decoder.decode();
    return ExactPairSupportStreamDecodeResult{
        DecodeDecision::accepted, std::move(chunk)};
  } catch (const DecodeFailure& failure) {
    return ExactPairSupportStreamDecodeResult{
        failure.decision, std::nullopt};
  }
}

#if defined(__unix__) || defined(__APPLE__)

ExactPairSupportStreamFdWireReceipt
encode_exact_pair_support_stream_chunk_to_fd(
    int descriptor,
    const ExactPairSupportStreamChunk& chunk,
    const ExactPairSupportStreamCodecLimits& limits) {
  require_finite_limits(limits);
  const struct stat before = require_empty_output_descriptor(descriptor);
  const std::size_t payload_byte_count =
      count_payload_bytes(chunk, limits);
  const std::size_t checksum_offset = checked_add_for_encode(
      chunk_header_byte_count,
      payload_byte_count,
      "the pair-support wire size overflows size_t");
  const std::size_t total_byte_count = checked_add_for_encode(
      checksum_offset,
      chunk_checksum_byte_count,
      "the pair-support wire size overflows size_t");
  if (total_byte_count > limits.maximum_encoded_byte_count) {
    throw std::length_error(
        "the pair-support wire representation exceeds its byte limit");
  }
  require_checksum_input_fits_sha256(checksum_offset);

  contract::CanonicalSha256Builder checksum_builder;
  checksum_builder.update(chunk_checksum_domain);
  BufferedFdWriter writer{
      descriptor, checksum_offset, checksum_builder};
  encode_envelope_header(writer, payload_byte_count);
  encode_counted_payload(
      writer, chunk, limits, payload_byte_count);
  if (writer.byte_count() != checksum_offset) {
    throw std::logic_error(
        "the pair-support fd wire size changed after counting");
  }
  writer.flush();
  const contract::CanonicalId checksum = checksum_builder.finalize();
  writer.append_unhashed_checksum(checksum);
  if (writer.file_byte_count() != total_byte_count) {
    throw std::logic_error(
        "the pair-support fd writer produced an inconsistent size");
  }

  struct stat after {};
  if (::fstat(descriptor, &after) != 0) {
    throw_last_system_error("cannot inspect the encoded pair-support file");
  }
  if (!S_ISREG(after.st_mode) || before.st_dev != after.st_dev ||
      before.st_ino != after.st_ino ||
      !metadata_size_equals(after, total_byte_count)) {
    throw std::runtime_error(
        "the pair-support output file changed during encoding");
  }
  return ExactPairSupportStreamFdWireReceipt{
      total_byte_count, checksum};
}

namespace {

[[nodiscard]] ExactPairSupportStreamFdWireVerificationResult
verify_fd_wire_with_scratch(
    int descriptor,
    const ExactPairSupportStreamCodecLimits& limits,
    std::optional<ExactPairSupportStreamFdWireReceipt> expected_receipt,
    std::span<std::uint8_t> scratch) {
  require_finite_limits(limits);
  if (scratch.size() != pair_support_stream_fd_buffer_byte_count) {
    throw std::logic_error(
        "the pair-support fd verifier scratch must be exactly 64 KiB");
  }
  try {
    struct stat before {};
    if (!inspect_read_descriptor(descriptor, before)) {
      fail_decode(DecodeDecision::invalid_file_descriptor);
    }
    const std::uintmax_t unsigned_file_size =
        static_cast<std::uintmax_t>(before.st_size);
    if (unsigned_file_size > std::numeric_limits<std::size_t>::max()) {
      fail_decode(DecodeDecision::numeric_overflow);
    }
    const std::size_t file_size =
        static_cast<std::size_t>(unsigned_file_size);
    if (file_size > limits.maximum_encoded_byte_count) {
      fail_decode(DecodeDecision::encoded_byte_limit_exceeded);
    }
    if (file_size <
        chunk_header_byte_count + chunk_checksum_byte_count) {
      fail_decode(DecodeDecision::truncated);
    }

    std::array<std::uint8_t, chunk_header_byte_count> header{};
    positional_read_exact_for_decode(descriptor, header, 0U);
    SpanByteReader envelope{header};
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
    const std::size_t checksum_offset =
        chunk_header_byte_count + payload_byte_count;
    const std::size_t expected_total_byte_count =
        checksum_offset + chunk_checksum_byte_count;
    if (expected_total_byte_count > file_size) {
      fail_decode(DecodeDecision::payload_length_mismatch);
    }
    if (expected_total_byte_count < file_size) {
      fail_decode(DecodeDecision::trailing_bytes);
    }
    if (!checksum_input_fits_sha256(checksum_offset)) {
      fail_decode(DecodeDecision::numeric_overflow);
    }

    contract::CanonicalSha256Builder checksum_builder;
    checksum_builder.update(chunk_checksum_domain);
    std::size_t offset = 0U;
    while (offset < checksum_offset) {
      const std::size_t count =
          std::min(scratch.size(), checksum_offset - offset);
      positional_read_exact_for_decode(
          descriptor, scratch.first(count), offset);
      checksum_builder.update(
          std::span<const std::uint8_t>{scratch}.first(count));
      offset += count;
    }
    std::array<std::uint8_t, chunk_checksum_byte_count>
        observed_checksum_bytes{};
    positional_read_exact_for_decode(
        descriptor, observed_checksum_bytes, checksum_offset);

    struct stat after {};
    if (::fstat(descriptor, &after) != 0) {
      fail_decode(DecodeDecision::file_io_error);
    }
    if (!same_file_image_metadata(before, after)) {
      fail_decode(DecodeDecision::file_changed);
    }
    const contract::CanonicalId computed_checksum =
        checksum_builder.finalize();
    if (!equal_checksum(
            computed_checksum, observed_checksum_bytes)) {
      fail_decode(DecodeDecision::checksum_mismatch);
    }
    const contract::CanonicalId observed_checksum{
        observed_checksum_bytes};
    const ExactPairSupportStreamFdWireReceipt receipt{
        file_size, observed_checksum};
    if (expected_receipt.has_value() &&
        *expected_receipt != receipt) {
      fail_decode(DecodeDecision::receipt_mismatch);
    }
    return ExactPairSupportStreamFdWireVerificationResult{
        DecodeDecision::accepted, receipt};
  } catch (const DecodeFailure& failure) {
    return ExactPairSupportStreamFdWireVerificationResult{
        failure.decision, std::nullopt};
  }
}

}  // namespace

ExactPairSupportStreamFdWireVerificationResult
verify_exact_pair_support_stream_chunk_fd_wire(
    int descriptor,
    const ExactPairSupportStreamCodecLimits& limits,
    std::optional<ExactPairSupportStreamFdWireReceipt> expected_receipt) {
  std::array<std::uint8_t, pair_support_stream_fd_buffer_byte_count>
      scratch{};
  return verify_fd_wire_with_scratch(
      descriptor, limits, std::move(expected_receipt), scratch);
}

ExactPairSupportStreamDecodeResult
decode_exact_pair_support_stream_chunk_from_fd(
    int descriptor,
    const ExactPairSupportStreamCodecLimits& limits,
    std::optional<ExactPairSupportStreamFdWireReceipt> expected_receipt) {
  std::array<std::uint8_t, pair_support_stream_fd_buffer_byte_count>
      scratch{};
  const ExactPairSupportStreamFdWireVerificationResult first_verification =
      verify_fd_wire_with_scratch(
          descriptor,
          limits,
          std::move(expected_receipt),
          scratch);
  if (!first_verification.accepted()) {
    return ExactPairSupportStreamDecodeResult{
        first_verification.decision, std::nullopt};
  }
  const ExactPairSupportStreamFdWireReceipt receipt =
      *first_verification.receipt;
  const std::size_t payload_byte_count =
      receipt.encoded_byte_count -
      chunk_header_byte_count - chunk_checksum_byte_count;
  try {
    std::array<std::uint8_t, chunk_header_byte_count> parsed_header{};
    positional_read_exact_for_decode(
        descriptor, parsed_header, 0U);
    SpanByteReader envelope{parsed_header};
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
    if (envelope.u64() != payload_byte_count) {
      fail_decode(DecodeDecision::receipt_mismatch);
    }

    ExactPairSupportStreamChunk chunk;
    {
      // The parser borrows the same physical 64-KiB window as both checksum
      // passes.  It retains no resident I/O array of its own.
      contract::CanonicalSha256Builder parsed_checksum_builder;
      parsed_checksum_builder.update(chunk_checksum_domain);
      parsed_checksum_builder.update(parsed_header);
      FdByteReader payload_reader{
          descriptor,
          chunk_header_byte_count,
          payload_byte_count,
          parsed_checksum_builder,
          scratch};
      PayloadDecoder decoder{payload_reader, limits};
      chunk = decoder.decode();
      std::array<std::uint8_t, chunk_checksum_byte_count>
          parsed_checksum_bytes{};
      positional_read_exact_for_decode(
          descriptor,
          parsed_checksum_bytes,
          chunk_header_byte_count + payload_byte_count);
      const contract::CanonicalId parsed_checksum =
          parsed_checksum_builder.finalize();
      if (!equal_checksum(parsed_checksum, parsed_checksum_bytes)) {
        return ExactPairSupportStreamDecodeResult{
            DecodeDecision::checksum_mismatch, std::nullopt};
      }
      if (receipt.checksum !=
          contract::CanonicalId{parsed_checksum_bytes}) {
        return ExactPairSupportStreamDecodeResult{
            DecodeDecision::receipt_mismatch, std::nullopt};
      }
    }
    const ExactPairSupportStreamFdWireVerificationResult
        second_verification =
            verify_fd_wire_with_scratch(
                descriptor, limits, receipt, scratch);
    if (!second_verification.accepted()) {
      return ExactPairSupportStreamDecodeResult{
          second_verification.decision, std::nullopt};
    }
    return ExactPairSupportStreamDecodeResult{
        DecodeDecision::accepted, std::move(chunk)};
  } catch (const DecodeFailure& failure) {
    return ExactPairSupportStreamDecodeResult{
        failure.decision, std::nullopt};
  }
}

#endif

}  // namespace morsehgp3d::hierarchy
