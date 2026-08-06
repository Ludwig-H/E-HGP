#include "morsehgp3d/hierarchy/direct_morse_forest_durable_run_archive.hpp"

#include "morsehgp3d/contract/canonical_id.hpp"

#include <array>
#include <cerrno>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace morsehgp3d::hierarchy {

namespace {

constexpr std::array<std::uint8_t, 8U> segment_magic{
    'M', 'H', '3', 'D', 'S', 'E', 'G', '1'};
constexpr std::array<std::uint8_t, 8U> seal_magic{
    'M', 'H', '3', 'D', 'S', 'E', 'A', 'L'};
constexpr std::size_t envelope_byte_count = 64U;
constexpr std::size_t envelope_reserved_byte_count = 8U;

struct CodecFailure {
  ExactDirectMorseForestSegmentCodecDecision decision{
      ExactDirectMorseForestSegmentCodecDecision::no_shape_rejected};
};

[[noreturn]] void fail(
    ExactDirectMorseForestSegmentCodecDecision decision) {
  throw CodecFailure{decision};
}

// -------------------------------------------------------------------------
// Canonical big-endian writer.  Every append enforces the byte cap; every
// exact text enforces the per-text and cumulative caps.
// -------------------------------------------------------------------------

struct Writer {
  const ExactDirectMorseForestSegmentCodecLimits& limits;
  std::vector<std::uint8_t> bytes;
  std::size_t total_exact_text_byte_count{};

  explicit Writer(const ExactDirectMorseForestSegmentCodecLimits& codec_limits)
      : limits(codec_limits) {}

  void raw(const std::uint8_t* data, std::size_t count) {
    if (count > limits.maximum_encoded_byte_count ||
        bytes.size() > limits.maximum_encoded_byte_count - count) {
      fail(ExactDirectMorseForestSegmentCodecDecision::
               no_encoded_byte_limit_exceeded);
    }
    bytes.insert(bytes.end(), data, data + count);
  }

  void u8(std::uint8_t value) { raw(&value, 1U); }

  void u32(std::uint32_t value) {
    std::array<std::uint8_t, 4U> out{};
    for (std::size_t index = 0U; index < 4U; ++index) {
      out[index] =
          static_cast<std::uint8_t>((value >> (8U * (3U - index))) & 0xFFU);
    }
    raw(out.data(), out.size());
  }

  void u64(std::uint64_t value) {
    std::array<std::uint8_t, 8U> out{};
    for (std::size_t index = 0U; index < 8U; ++index) {
      out[index] =
          static_cast<std::uint8_t>((value >> (8U * (7U - index))) & 0xFFU);
    }
    raw(out.data(), out.size());
  }

  void size(std::size_t value) {
    if constexpr (sizeof(std::size_t) > sizeof(std::uint64_t)) {
      if (value > std::numeric_limits<std::uint64_t>::max()) {
        fail(ExactDirectMorseForestSegmentCodecDecision::
                 no_capacity_overflow);
      }
    }
    u64(static_cast<std::uint64_t>(value));
  }

  void boolean(bool value) { u8(value ? 1U : 0U); }

  void id(const contract::CanonicalId& value) {
    raw(value.bytes().data(), contract::CanonicalId::byte_count);
  }

  void text(const std::string& value) {
    if (value.size() > limits.maximum_exact_text_byte_count) {
      fail(ExactDirectMorseForestSegmentCodecDecision::
               no_exact_text_limit_exceeded);
    }
    if (total_exact_text_byte_count >
            limits.maximum_total_exact_text_byte_count ||
        value.size() > limits.maximum_total_exact_text_byte_count -
                           total_exact_text_byte_count) {
      fail(ExactDirectMorseForestSegmentCodecDecision::
               no_total_exact_text_limit_exceeded);
    }
    total_exact_text_byte_count += value.size();
    size(value.size());
    raw(reinterpret_cast<const std::uint8_t*>(value.data()), value.size());
  }

  void optional_u64(const std::optional<std::uint64_t>& value) {
    boolean(value.has_value());
    u64(value.value_or(0U));
  }

  void optional_size(const std::optional<std::size_t>& value) {
    boolean(value.has_value());
    size(value.value_or(0U));
  }

  void level(const exact::ExactLevel& value) {
    text(value.canonical_key());
  }

  void center(const exact::ExactCenter3& value) {
    const auto record = value.to_record();
    text(record.x_numerator);
    text(record.y_numerator);
    text(record.z_numerator);
    text(record.denominator);
  }

  void facet_key(const ExactDirectSparseFacetKey& value) {
    if (value.point_count > value.point_ids.size()) {
      fail(ExactDirectMorseForestSegmentCodecDecision::no_shape_rejected);
    }
    for (std::size_t index = value.point_count;
         index < value.point_ids.size();
         ++index) {
      if (value.point_ids[index] != 0U) {
        fail(ExactDirectMorseForestSegmentCodecDecision::no_shape_rejected);
      }
    }
    size(value.point_count);
    for (const auto point_id : value.point_ids) {
      u64(point_id);
    }
  }

  void witness(const ExactDirectSparseFacetWitness& value) {
    u64(value.external_authority_id);
    u64(value.replay_token);
  }

  void locator_stamp(
      const ExactDirectSparsePositiveFacetLocatorSnapshotStamp& value) {
    u32(value.schema_version);
    u64(value.external_authority_id);
    size(value.committed_batch_count);
    size(value.inserted_key_count);
    size(value.component_union_count);
    size(value.binding_count);
    id(value.committed_history_digest);
  }

  void cursor(const ExactDirectMorseForestSegmentCursor& value) {
    size(value.segment_count);
    size(value.implicit_order_one_prefix_count);
    size(value.birth_record_count);
    size(value.arm_root_binding_count);
    size(value.saddle_record_count);
    size(value.atomic_group_count);
    size(value.child_reference_count);
    size(value.batch_record_count);
    size(value.node_count);
    size(value.logical_output_entry_count);
    id(value.chain_digest);
  }

  void counters(const ExactDirectMorseForestCounters& value) {
    size(value.birth_record_count);
    size(value.latent_higher_order_birth_count);
    size(value.order_one_birth_node_count);
    size(value.arm_root_binding_count);
    size(value.saddle_record_count);
    size(value.atomic_group_count);
    size(value.reduced_birth_group_count);
    size(value.continuation_group_count);
    size(value.multifusion_group_count);
    size(value.child_reference_count);
    size(value.batch_record_count);
    size(value.node_count);
    size(value.final_root_count);
    size(value.locator_union_count);
    size(value.closure_call_count);
    size(value.quotient_call_count);
    size(value.distinct_strict_arm_count);
    size(value.duplicate_strict_arm_reference_count);
    size(value.aggregate_closure_node_count);
    size(value.aggregate_closure_step_call_count);
    size(value.maximum_batch_arm_count);
    size(value.maximum_batch_carrier_arity);
    size(value.maximum_batch_merge_arity);
  }
};

// -------------------------------------------------------------------------
// Bounded canonical reader.  Every read is bounds-checked; every exact text
// is re-parsed and re-rendered so only canonical spellings decode.
// -------------------------------------------------------------------------

struct Reader {
  const ExactDirectMorseForestSegmentCodecLimits& limits;
  std::span<const std::uint8_t> bytes;
  std::size_t cursor{};
  std::size_t total_exact_text_byte_count{};

  Reader(
      const ExactDirectMorseForestSegmentCodecLimits& codec_limits,
      std::span<const std::uint8_t> payload)
      : limits(codec_limits), bytes(payload) {}

  const std::uint8_t* take(std::size_t count) {
    if (count > bytes.size() || cursor > bytes.size() - count) {
      fail(ExactDirectMorseForestSegmentCodecDecision::
               no_truncation_or_suffix_rejected);
    }
    const std::uint8_t* out = bytes.data() + cursor;
    cursor += count;
    return out;
  }

  std::uint8_t u8() { return *take(1U); }

  std::uint32_t u32() {
    const std::uint8_t* data = take(4U);
    std::uint32_t value = 0U;
    for (std::size_t index = 0U; index < 4U; ++index) {
      value = (value << 8U) | data[index];
    }
    return value;
  }

  std::uint64_t u64() {
    const std::uint8_t* data = take(8U);
    std::uint64_t value = 0U;
    for (std::size_t index = 0U; index < 8U; ++index) {
      value = (value << 8U) | data[index];
    }
    return value;
  }

  std::size_t size_value() {
    const std::uint64_t value = u64();
    if constexpr (sizeof(std::size_t) < sizeof(std::uint64_t)) {
      if (value > std::numeric_limits<std::size_t>::max()) {
        fail(ExactDirectMorseForestSegmentCodecDecision::
                 no_capacity_overflow);
      }
    }
    return static_cast<std::size_t>(value);
  }

  bool boolean() {
    const std::uint8_t value = u8();
    if (value > 1U) {
      fail(ExactDirectMorseForestSegmentCodecDecision::no_shape_rejected);
    }
    return value != 0U;
  }

  contract::CanonicalId id() {
    const std::uint8_t* data = take(contract::CanonicalId::byte_count);
    std::array<std::uint8_t, contract::CanonicalId::byte_count> out{};
    std::memcpy(out.data(), data, out.size());
    return contract::CanonicalId{out};
  }

  std::string text() {
    const std::size_t length = size_value();
    if (length > limits.maximum_exact_text_byte_count) {
      fail(ExactDirectMorseForestSegmentCodecDecision::
               no_exact_text_limit_exceeded);
    }
    if (total_exact_text_byte_count >
            limits.maximum_total_exact_text_byte_count ||
        length > limits.maximum_total_exact_text_byte_count -
                     total_exact_text_byte_count) {
      fail(ExactDirectMorseForestSegmentCodecDecision::
               no_total_exact_text_limit_exceeded);
    }
    total_exact_text_byte_count += length;
    const std::uint8_t* data = take(length);
    return std::string{
        reinterpret_cast<const char*>(data), length};
  }

  std::optional<std::uint64_t> optional_u64() {
    const bool present = boolean();
    const std::uint64_t value = u64();
    if (!present && value != 0U) {
      fail(ExactDirectMorseForestSegmentCodecDecision::no_shape_rejected);
    }
    return present ? std::optional<std::uint64_t>{value} : std::nullopt;
  }

  std::optional<std::size_t> optional_size() {
    const bool present = boolean();
    const std::size_t value = size_value();
    if (!present && value != 0U) {
      fail(ExactDirectMorseForestSegmentCodecDecision::no_shape_rejected);
    }
    return present ? std::optional<std::size_t>{value} : std::nullopt;
  }

  exact::ExactLevel level() {
    const std::string spelled = text();
    try {
      exact::ExactRational value =
          exact::ExactRational::parse_canonical(spelled);
      if (value.canonical_key() != spelled) {
        fail(ExactDirectMorseForestSegmentCodecDecision::no_shape_rejected);
      }
      return exact::ExactLevel{std::move(value)};
    } catch (const CodecFailure&) {
      throw;
    } catch (...) {
      fail(ExactDirectMorseForestSegmentCodecDecision::no_shape_rejected);
    }
  }

  exact::ExactCenter3 center() {
    const std::string x = text();
    const std::string y = text();
    const std::string z = text();
    const std::string denominator = text();
    try {
      exact::ExactCenter3 value{
          exact::parse_canonical_integer(x),
          exact::parse_canonical_integer(y),
          exact::parse_canonical_integer(z),
          exact::parse_canonical_positive_integer(denominator)};
      const auto record = value.to_record();
      if (record.x_numerator != x || record.y_numerator != y ||
          record.z_numerator != z || record.denominator != denominator) {
        fail(ExactDirectMorseForestSegmentCodecDecision::no_shape_rejected);
      }
      return value;
    } catch (const CodecFailure&) {
      throw;
    } catch (...) {
      fail(ExactDirectMorseForestSegmentCodecDecision::no_shape_rejected);
    }
  }

  ExactDirectSparseFacetKey facet_key() {
    ExactDirectSparseFacetKey value{};
    value.point_count = size_value();
    if (value.point_count > value.point_ids.size()) {
      fail(ExactDirectMorseForestSegmentCodecDecision::no_shape_rejected);
    }
    for (std::size_t index = 0U; index < value.point_ids.size(); ++index) {
      value.point_ids[index] = u64();
      if (index >= value.point_count && value.point_ids[index] != 0U) {
        fail(ExactDirectMorseForestSegmentCodecDecision::no_shape_rejected);
      }
    }
    return value;
  }

  ExactDirectSparseFacetWitness witness() {
    ExactDirectSparseFacetWitness value{};
    value.external_authority_id = u64();
    value.replay_token = u64();
    return value;
  }

  ExactDirectSparsePositiveFacetLocatorSnapshotStamp locator_stamp() {
    ExactDirectSparsePositiveFacetLocatorSnapshotStamp value{};
    value.schema_version = u32();
    value.external_authority_id = u64();
    value.committed_batch_count = size_value();
    value.inserted_key_count = size_value();
    value.component_union_count = size_value();
    value.binding_count = size_value();
    value.committed_history_digest = id();
    return value;
  }

  ExactDirectMorseForestSegmentCursor cursor_value() {
    ExactDirectMorseForestSegmentCursor value{};
    value.segment_count = size_value();
    value.implicit_order_one_prefix_count = size_value();
    value.birth_record_count = size_value();
    value.arm_root_binding_count = size_value();
    value.saddle_record_count = size_value();
    value.atomic_group_count = size_value();
    value.child_reference_count = size_value();
    value.batch_record_count = size_value();
    value.node_count = size_value();
    value.logical_output_entry_count = size_value();
    value.chain_digest = id();
    return value;
  }

  ExactDirectMorseForestCounters counters() {
    ExactDirectMorseForestCounters value{};
    value.birth_record_count = size_value();
    value.latent_higher_order_birth_count = size_value();
    value.order_one_birth_node_count = size_value();
    value.arm_root_binding_count = size_value();
    value.saddle_record_count = size_value();
    value.atomic_group_count = size_value();
    value.reduced_birth_group_count = size_value();
    value.continuation_group_count = size_value();
    value.multifusion_group_count = size_value();
    value.child_reference_count = size_value();
    value.batch_record_count = size_value();
    value.node_count = size_value();
    value.final_root_count = size_value();
    value.locator_union_count = size_value();
    value.closure_call_count = size_value();
    value.quotient_call_count = size_value();
    value.distinct_strict_arm_count = size_value();
    value.duplicate_strict_arm_reference_count = size_value();
    value.aggregate_closure_node_count = size_value();
    value.aggregate_closure_step_call_count = size_value();
    value.maximum_batch_arm_count = size_value();
    value.maximum_batch_carrier_arity = size_value();
    value.maximum_batch_merge_arity = size_value();
    return value;
  }
};

// -------------------------------------------------------------------------
// Per-record writers and readers, field for field in declaration order.
// -------------------------------------------------------------------------

void write_batch(Writer& out, const ExactDirectMorseForestBatch& value) {
  out.size(value.batch_index);
  out.size(value.source_journal_batch_index);
  out.size(value.order);
  out.level(value.squared_level);
  out.size(value.birth_record_offset);
  out.size(value.birth_record_count);
  out.size(value.saddle_record_offset);
  out.size(value.saddle_record_count);
  out.size(value.atomic_group_offset);
  out.size(value.atomic_group_count);
  out.size(value.strict_pre_batch_carrier_count);
  out.size(value.strict_pre_batch_reduced_root_count);
  out.size(value.closed_post_batch_carrier_count);
  out.size(value.closed_post_batch_reduced_root_count);
  out.locator_stamp(value.strict_pre_batch_stamp);
  out.locator_stamp(value.committed_batch_stamp);
  out.boolean(value.strict_arms_resolved_before_mutation);
  out.boolean(value.quotient_resolved_before_mutation);
  out.boolean(value.unions_then_births_committed_atomically);
}

[[nodiscard]] ExactDirectMorseForestBatch read_batch(Reader& in) {
  ExactDirectMorseForestBatch value{};
  value.batch_index = in.size_value();
  value.source_journal_batch_index = in.size_value();
  value.order = in.size_value();
  value.squared_level = in.level();
  value.birth_record_offset = in.size_value();
  value.birth_record_count = in.size_value();
  value.saddle_record_offset = in.size_value();
  value.saddle_record_count = in.size_value();
  value.atomic_group_offset = in.size_value();
  value.atomic_group_count = in.size_value();
  value.strict_pre_batch_carrier_count = in.size_value();
  value.strict_pre_batch_reduced_root_count = in.size_value();
  value.closed_post_batch_carrier_count = in.size_value();
  value.closed_post_batch_reduced_root_count = in.size_value();
  value.strict_pre_batch_stamp = in.locator_stamp();
  value.committed_batch_stamp = in.locator_stamp();
  value.strict_arms_resolved_before_mutation = in.boolean();
  value.quotient_resolved_before_mutation = in.boolean();
  value.unions_then_births_committed_atomically = in.boolean();
  return value;
}

void write_birth(
    Writer& out, const ExactDirectMorseForestBirthRecord& value) {
  out.size(value.birth_record_index);
  out.size(value.source_event_projection_index);
  out.size(value.source_journal_batch_index);
  out.size(value.order);
  out.facet_key(value.facet_key);
  out.size(value.component_handle);
  out.optional_u64(value.order_one_birth_node_id);
  out.witness(value.binding_witness);
}

[[nodiscard]] ExactDirectMorseForestBirthRecord read_birth(Reader& in) {
  ExactDirectMorseForestBirthRecord value{};
  value.birth_record_index = in.size_value();
  value.source_event_projection_index = in.size_value();
  value.source_journal_batch_index = in.size_value();
  value.order = in.size_value();
  value.facet_key = in.facet_key();
  value.component_handle = in.size_value();
  value.order_one_birth_node_id = in.optional_u64();
  value.binding_witness = in.witness();
  return value;
}

void write_binding(
    Writer& out, const ExactDirectMorseForestArmRootBinding& value) {
  out.size(value.binding_index);
  out.size(value.source_arm_seed_index);
  out.size(value.source_family_index);
  out.facet_key(value.strict_arm_key);
  out.size(value.frozen_carrier_component_handle);
  out.optional_u64(value.prior_reduced_root_node_id);
  out.size(value.terminal_birth_record_index);
  out.facet_key(value.terminal_birth_facet_key);
  out.witness(value.terminal_birth_binding_witness);
  out.u64(value.removed_support_point_id);
  out.center(value.terminal_birth_exact_center);
  out.level(value.terminal_birth_exact_squared_level);
}

[[nodiscard]] ExactDirectMorseForestArmRootBinding read_binding(Reader& in) {
  ExactDirectMorseForestArmRootBinding value{};
  value.binding_index = in.size_value();
  value.source_arm_seed_index = in.size_value();
  value.source_family_index = in.size_value();
  value.strict_arm_key = in.facet_key();
  value.frozen_carrier_component_handle = in.size_value();
  value.prior_reduced_root_node_id = in.optional_u64();
  value.terminal_birth_record_index = in.size_value();
  value.terminal_birth_facet_key = in.facet_key();
  value.terminal_birth_binding_witness = in.witness();
  value.removed_support_point_id = in.u64();
  value.terminal_birth_exact_center = in.center();
  value.terminal_birth_exact_squared_level = in.level();
  return value;
}

void write_saddle(
    Writer& out, const ExactDirectMorseForestSaddleRecord& value) {
  out.size(value.saddle_record_index);
  out.size(value.source_family_index);
  out.size(value.source_event_index);
  out.size(value.source_journal_batch_index);
  out.size(value.arm_binding_offset);
  out.size(value.arm_binding_count);
  out.size(value.distinct_frozen_carrier_count);
  out.size(value.distinct_latent_carrier_count);
  out.size(value.distinct_prior_reduced_root_count);
  out.size(value.atomic_group_index);
  out.size(value.journal_event_projection_index);
  out.id(value.source_event_arm_identity_digest);
}

[[nodiscard]] ExactDirectMorseForestSaddleRecord read_saddle(Reader& in) {
  ExactDirectMorseForestSaddleRecord value{};
  value.saddle_record_index = in.size_value();
  value.source_family_index = in.size_value();
  value.source_event_index = in.size_value();
  value.source_journal_batch_index = in.size_value();
  value.arm_binding_offset = in.size_value();
  value.arm_binding_count = in.size_value();
  value.distinct_frozen_carrier_count = in.size_value();
  value.distinct_latent_carrier_count = in.size_value();
  value.distinct_prior_reduced_root_count = in.size_value();
  value.atomic_group_index = in.size_value();
  value.journal_event_projection_index = in.size_value();
  value.source_event_arm_identity_digest = in.id();
  return value;
}

void write_group(
    Writer& out, const ExactDirectMorseForestAtomicGroup& value) {
  out.size(value.atomic_group_index);
  out.size(value.batch_index);
  out.size(value.saddle_record_offset);
  out.size(value.saddle_record_count);
  out.size(value.frozen_carrier_count);
  out.size(value.latent_carrier_count);
  out.size(value.prior_reduced_root_count);
  out.size(value.child_offset);
  out.size(value.child_count);
  out.optional_u64(value.created_node_id);
  out.u64(value.resulting_root_node_id);
  out.u8(static_cast<std::uint8_t>(value.kind));
}

[[nodiscard]] ExactDirectMorseForestAtomicGroup read_group(Reader& in) {
  ExactDirectMorseForestAtomicGroup value{};
  value.atomic_group_index = in.size_value();
  value.batch_index = in.size_value();
  value.saddle_record_offset = in.size_value();
  value.saddle_record_count = in.size_value();
  value.frozen_carrier_count = in.size_value();
  value.latent_carrier_count = in.size_value();
  value.prior_reduced_root_count = in.size_value();
  value.child_offset = in.size_value();
  value.child_count = in.size_value();
  value.created_node_id = in.optional_u64();
  value.resulting_root_node_id = in.u64();
  const std::uint8_t kind = in.u8();
  if (kind >
      static_cast<std::uint8_t>(
          ExactDirectMorseForestAtomicGroupKind::multifusion)) {
    fail(ExactDirectMorseForestSegmentCodecDecision::no_shape_rejected);
  }
  value.kind = static_cast<ExactDirectMorseForestAtomicGroupKind>(kind);
  return value;
}

void write_node(Writer& out, const ExactDirectMorseForestNode& value) {
  out.u64(value.node_id);
  out.size(value.order);
  out.level(value.squared_level);
  out.u8(static_cast<std::uint8_t>(value.kind));
  out.size(value.child_offset);
  out.size(value.child_count);
  out.optional_size(value.birth_record_index);
  out.optional_size(value.atomic_group_index);
}

[[nodiscard]] ExactDirectMorseForestNode read_node(Reader& in) {
  ExactDirectMorseForestNode value{};
  value.node_id = in.u64();
  value.order = in.size_value();
  value.squared_level = in.level();
  const std::uint8_t kind = in.u8();
  if (kind >
      static_cast<std::uint8_t>(
          ExactDirectMorseForestNodeKind::multifusion)) {
    fail(ExactDirectMorseForestSegmentCodecDecision::no_shape_rejected);
  }
  value.kind = static_cast<ExactDirectMorseForestNodeKind>(kind);
  value.child_offset = in.size_value();
  value.child_count = in.size_value();
  value.birth_record_index = in.optional_size();
  value.atomic_group_index = in.optional_size();
  return value;
}

void write_final_root(
    Writer& out, const ExactDirectMorseForestFinalRoot& value) {
  out.size(value.final_root_index);
  out.size(value.order);
  out.size(value.component_handle);
  out.u64(value.root_node_id);
}

[[nodiscard]] ExactDirectMorseForestFinalRoot read_final_root(Reader& in) {
  ExactDirectMorseForestFinalRoot value{};
  value.final_root_index = in.size_value();
  value.order = in.size_value();
  value.component_handle = in.size_value();
  value.root_node_id = in.u64();
  return value;
}

// -------------------------------------------------------------------------
// Envelope assembly and validation.
// -------------------------------------------------------------------------

[[nodiscard]] contract::CanonicalId payload_sha256(
    std::span<const std::uint8_t> payload) {
  contract::CanonicalSha256Builder builder;
  builder.update(std::string_view{
      reinterpret_cast<const char*>(payload.data()), payload.size()});
  return builder.finalize();
}

[[nodiscard]] std::vector<std::uint8_t> assemble(
    const std::array<std::uint8_t, 8U>& magic,
    const ExactDirectMorseForestSegmentCodecLimits& limits,
    std::vector<std::uint8_t>&& payload,
    contract::CanonicalId& digest_out) {
  Writer envelope{limits};
  envelope.raw(magic.data(), magic.size());
  envelope.u32(direct_morse_forest_durable_run_archive_schema_version);
  envelope.u32(direct_morse_forest_output_segment_schema_version);
  envelope.size(payload.size());
  digest_out = payload_sha256(payload);
  envelope.id(digest_out);
  const std::array<std::uint8_t, envelope_reserved_byte_count> reserved{};
  envelope.raw(reserved.data(), reserved.size());
  if (envelope.bytes.size() != envelope_byte_count) {
    fail(ExactDirectMorseForestSegmentCodecDecision::no_shape_rejected);
  }
  if (payload.size() >
      limits.maximum_encoded_byte_count - envelope.bytes.size()) {
    fail(ExactDirectMorseForestSegmentCodecDecision::
             no_encoded_byte_limit_exceeded);
  }
  std::vector<std::uint8_t> out = std::move(envelope.bytes);
  out.insert(out.end(), payload.begin(), payload.end());
  return out;
}

[[nodiscard]] std::span<const std::uint8_t> open_envelope(
    const std::array<std::uint8_t, 8U>& magic,
    const ExactDirectMorseForestSegmentCodecLimits& limits,
    std::span<const std::uint8_t> bytes) {
  if (bytes.size() > limits.maximum_encoded_byte_count) {
    fail(ExactDirectMorseForestSegmentCodecDecision::
             no_encoded_byte_limit_exceeded);
  }
  if (bytes.size() < envelope_byte_count) {
    fail(ExactDirectMorseForestSegmentCodecDecision::
             no_truncation_or_suffix_rejected);
  }
  Reader envelope{limits, bytes.first(envelope_byte_count)};
  const std::uint8_t* observed_magic = envelope.take(magic.size());
  if (std::memcmp(observed_magic, magic.data(), magic.size()) != 0) {
    fail(ExactDirectMorseForestSegmentCodecDecision::no_schema_rejected);
  }
  if (envelope.u32() !=
          direct_morse_forest_durable_run_archive_schema_version ||
      envelope.u32() != direct_morse_forest_output_segment_schema_version) {
    fail(ExactDirectMorseForestSegmentCodecDecision::no_schema_rejected);
  }
  const std::size_t declared_length = envelope.size_value();
  const contract::CanonicalId declared_digest = envelope.id();
  for (std::size_t index = 0U; index < envelope_reserved_byte_count;
       ++index) {
    if (*envelope.take(1U) != 0U) {
      fail(ExactDirectMorseForestSegmentCodecDecision::no_schema_rejected);
    }
  }
  if (bytes.size() - envelope_byte_count != declared_length) {
    fail(ExactDirectMorseForestSegmentCodecDecision::
             no_truncation_or_suffix_rejected);
  }
  const auto payload = bytes.subspan(envelope_byte_count);
  if (payload_sha256(payload) != declared_digest) {
    fail(ExactDirectMorseForestSegmentCodecDecision::
             no_payload_digest_mismatch);
  }
  return payload;
}

template <class Result, class Body>
[[nodiscard]] Result run_codec(Body&& body) noexcept {
  Result output;
  try {
    body(output);
    output.decision = ExactDirectMorseForestSegmentCodecDecision::
        complete_canonical_segment_codec;
  } catch (const CodecFailure& failure) {
    output = Result{};
    output.decision = failure.decision;
  } catch (const std::bad_alloc&) {
    output = Result{};
    output.decision =
        ExactDirectMorseForestSegmentCodecDecision::no_allocation_failed;
  } catch (...) {
    output = Result{};
    output.decision =
        ExactDirectMorseForestSegmentCodecDecision::no_shape_rejected;
  }
  return output;
}

void require_segment_limits(
    const ExactDirectMorseForestBatchSegment& segment,
    const ExactDirectMorseForestSegmentCodecLimits& limits) {
  const auto& caps = limits.segment_limits;
  if (segment.birth_records.size() > caps.maximum_birth_record_count ||
      segment.arm_root_bindings.size() >
          caps.maximum_arm_root_binding_count ||
      segment.saddle_records.size() > caps.maximum_saddle_record_count ||
      segment.atomic_groups.size() > caps.maximum_atomic_group_count ||
      segment.child_node_ids.size() > caps.maximum_child_reference_count ||
      segment.nodes.size() > caps.maximum_node_count) {
    fail(ExactDirectMorseForestSegmentCodecDecision::
             no_record_limit_exceeded);
  }
}

}  // namespace

bool ExactDirectMorseForestEncodedSegment::certified_encoded()
    const noexcept {
  return decision == ExactDirectMorseForestSegmentCodecDecision::
                         complete_canonical_segment_codec &&
         bytes.size() == encoded_byte_count &&
         encoded_byte_count >= envelope_byte_count;
}

bool ExactDirectMorseForestDecodedSegment::certified_decoded()
    const noexcept {
  return decision == ExactDirectMorseForestSegmentCodecDecision::
                         complete_canonical_segment_codec &&
         consumed_byte_count >= envelope_byte_count;
}

bool ExactDirectMorseForestEncodedFinalSeal::certified_encoded()
    const noexcept {
  return decision == ExactDirectMorseForestSegmentCodecDecision::
                         complete_canonical_segment_codec &&
         bytes.size() == encoded_byte_count &&
         encoded_byte_count >= envelope_byte_count;
}

bool ExactDirectMorseForestDecodedFinalSeal::certified_decoded()
    const noexcept {
  return decision == ExactDirectMorseForestSegmentCodecDecision::
                         complete_canonical_segment_codec &&
         consumed_byte_count >= envelope_byte_count;
}

ExactDirectMorseForestEncodedSegment
encode_exact_direct_morse_forest_batch_segment(
    const ExactDirectMorseForestBatchSegment& segment,
    const ExactDirectMorseForestSegmentCodecLimits& limits) noexcept {
  return run_codec<ExactDirectMorseForestEncodedSegment>(
      [&](ExactDirectMorseForestEncodedSegment& output) {
        if (segment.schema_version !=
                direct_morse_forest_output_segment_schema_version ||
            !segment.certified_structure()) {
          fail(ExactDirectMorseForestSegmentCodecDecision::
                   no_schema_rejected);
        }
        require_segment_limits(segment, limits);
        Writer payload{limits};
        payload.u32(segment.schema_version);
        payload.cursor(segment.begin_cursor);
        payload.cursor(segment.end_cursor);
        payload.id(segment.payload_digest);
        write_batch(payload, segment.batch);
        payload.size(segment.birth_records.size());
        for (const auto& record : segment.birth_records) {
          write_birth(payload, record);
        }
        payload.size(segment.arm_root_bindings.size());
        for (const auto& record : segment.arm_root_bindings) {
          write_binding(payload, record);
        }
        payload.size(segment.saddle_records.size());
        for (const auto& record : segment.saddle_records) {
          write_saddle(payload, record);
        }
        payload.size(segment.atomic_groups.size());
        for (const auto& record : segment.atomic_groups) {
          write_group(payload, record);
        }
        payload.size(segment.child_node_ids.size());
        for (const auto node_id : segment.child_node_ids) {
          payload.u64(node_id);
        }
        payload.size(segment.nodes.size());
        for (const auto& record : segment.nodes) {
          write_node(payload, record);
        }
        payload.boolean(segment.canonical_singleton_prefix_implicit);
        payload.boolean(segment.derived_cache_only);
        payload.boolean(segment.forbidden_global_structure_materialized);
        payload.boolean(segment.public_status_claimed);
        output.bytes = assemble(
            segment_magic,
            limits,
            std::move(payload.bytes),
            output.encoded_payload_digest);
        output.encoded_byte_count = output.bytes.size();
      });
}

ExactDirectMorseForestDecodedSegment
decode_exact_direct_morse_forest_batch_segment(
    std::span<const std::uint8_t> bytes,
    const ExactDirectMorseForestSegmentCodecLimits& limits) noexcept {
  return run_codec<ExactDirectMorseForestDecodedSegment>(
      [&](ExactDirectMorseForestDecodedSegment& output) {
        const auto payload = open_envelope(segment_magic, limits, bytes);
        Reader in{limits, payload};
        ExactDirectMorseForestBatchSegment segment{};
        segment.schema_version = in.u32();
        if (segment.schema_version !=
            direct_morse_forest_output_segment_schema_version) {
          fail(ExactDirectMorseForestSegmentCodecDecision::
                   no_schema_rejected);
        }
        segment.begin_cursor = in.cursor_value();
        segment.end_cursor = in.cursor_value();
        segment.payload_digest = in.id();
        segment.batch = read_batch(in);
        const auto read_count = [&](std::size_t cap) {
          const std::size_t count = in.size_value();
          if (count > cap) {
            fail(ExactDirectMorseForestSegmentCodecDecision::
                     no_record_limit_exceeded);
          }
          return count;
        };
        const std::size_t birth_count = read_count(
            limits.segment_limits.maximum_birth_record_count);
        segment.birth_records.reserve(birth_count);
        for (std::size_t index = 0U; index < birth_count; ++index) {
          segment.birth_records.push_back(read_birth(in));
        }
        const std::size_t binding_count = read_count(
            limits.segment_limits.maximum_arm_root_binding_count);
        segment.arm_root_bindings.reserve(binding_count);
        for (std::size_t index = 0U; index < binding_count; ++index) {
          segment.arm_root_bindings.push_back(read_binding(in));
        }
        const std::size_t saddle_count = read_count(
            limits.segment_limits.maximum_saddle_record_count);
        segment.saddle_records.reserve(saddle_count);
        for (std::size_t index = 0U; index < saddle_count; ++index) {
          segment.saddle_records.push_back(read_saddle(in));
        }
        const std::size_t group_count = read_count(
            limits.segment_limits.maximum_atomic_group_count);
        segment.atomic_groups.reserve(group_count);
        for (std::size_t index = 0U; index < group_count; ++index) {
          segment.atomic_groups.push_back(read_group(in));
        }
        const std::size_t child_count = read_count(
            limits.segment_limits.maximum_child_reference_count);
        segment.child_node_ids.reserve(child_count);
        for (std::size_t index = 0U; index < child_count; ++index) {
          segment.child_node_ids.push_back(in.u64());
        }
        const std::size_t node_count =
            read_count(limits.segment_limits.maximum_node_count);
        segment.nodes.reserve(node_count);
        for (std::size_t index = 0U; index < node_count; ++index) {
          segment.nodes.push_back(read_node(in));
        }
        segment.canonical_singleton_prefix_implicit = in.boolean();
        segment.derived_cache_only = in.boolean();
        segment.forbidden_global_structure_materialized = in.boolean();
        segment.public_status_claimed = in.boolean();
        if (in.cursor != payload.size()) {
          fail(ExactDirectMorseForestSegmentCodecDecision::
                   no_truncation_or_suffix_rejected);
        }
        if (!segment.certified_structure()) {
          fail(ExactDirectMorseForestSegmentCodecDecision::
                   no_shape_rejected);
        }
        output.segment = std::move(segment);
        output.consumed_byte_count = bytes.size();
      });
}

ExactDirectMorseForestEncodedFinalSeal
encode_exact_direct_morse_forest_final_seal(
    const ExactDirectMorseForestFinalSeal& seal,
    const ExactDirectMorseForestSegmentCodecLimits& limits) noexcept {
  return run_codec<ExactDirectMorseForestEncodedFinalSeal>(
      [&](ExactDirectMorseForestEncodedFinalSeal& output) {
        if (seal.schema_version !=
            direct_morse_forest_output_segment_schema_version) {
          fail(ExactDirectMorseForestSegmentCodecDecision::
                   no_schema_rejected);
        }
        if (seal.final_roots.size() > limits.maximum_final_root_count) {
          fail(ExactDirectMorseForestSegmentCodecDecision::
                   no_record_limit_exceeded);
        }
        Writer payload{limits};
        payload.u32(seal.schema_version);
        payload.size(seal.point_count);
        payload.size(seal.effective_maximum_order);
        payload.id(seal.source_higher_canonical_cloud_digest);
        payload.cursor(seal.final_cursor);
        payload.locator_stamp(seal.final_locator_stamp);
        payload.counters(seal.counters);
        payload.size(seal.final_roots.size());
        for (const auto& record : seal.final_roots) {
          write_final_root(payload, record);
        }
        payload.size(seal.logical_output_entry_count);
        payload.boolean(seal.all_source_batches_committed);
        payload.boolean(seal.every_segment_acknowledged);
        payload.boolean(seal.derived_cache_only);
        payload.boolean(seal.conditional_exact_h0_only);
        payload.boolean(seal.forbidden_global_structure_materialized);
        payload.boolean(seal.public_status_claimed);
        output.bytes = assemble(
            seal_magic,
            limits,
            std::move(payload.bytes),
            output.encoded_payload_digest);
        output.encoded_byte_count = output.bytes.size();
      });
}

ExactDirectMorseForestDecodedFinalSeal
decode_exact_direct_morse_forest_final_seal(
    std::span<const std::uint8_t> bytes,
    const ExactDirectMorseForestSegmentCodecLimits& limits) noexcept {
  return run_codec<ExactDirectMorseForestDecodedFinalSeal>(
      [&](ExactDirectMorseForestDecodedFinalSeal& output) {
        const auto payload = open_envelope(seal_magic, limits, bytes);
        Reader in{limits, payload};
        ExactDirectMorseForestFinalSeal seal{};
        seal.schema_version = in.u32();
        if (seal.schema_version !=
            direct_morse_forest_output_segment_schema_version) {
          fail(ExactDirectMorseForestSegmentCodecDecision::
                   no_schema_rejected);
        }
        seal.point_count = in.size_value();
        seal.effective_maximum_order = in.size_value();
        seal.source_higher_canonical_cloud_digest = in.id();
        seal.final_cursor = in.cursor_value();
        seal.final_locator_stamp = in.locator_stamp();
        seal.counters = in.counters();
        const std::size_t root_count = in.size_value();
        if (root_count > limits.maximum_final_root_count) {
          fail(ExactDirectMorseForestSegmentCodecDecision::
                   no_record_limit_exceeded);
        }
        seal.final_roots.reserve(root_count);
        for (std::size_t index = 0U; index < root_count; ++index) {
          seal.final_roots.push_back(read_final_root(in));
        }
        seal.logical_output_entry_count = in.size_value();
        seal.all_source_batches_committed = in.boolean();
        seal.every_segment_acknowledged = in.boolean();
        seal.derived_cache_only = in.boolean();
        seal.conditional_exact_h0_only = in.boolean();
        seal.forbidden_global_structure_materialized = in.boolean();
        seal.public_status_claimed = in.boolean();
        if (in.cursor != payload.size()) {
          fail(ExactDirectMorseForestSegmentCodecDecision::
                   no_truncation_or_suffix_rejected);
        }
        output.seal = std::move(seal);
        output.consumed_byte_count = bytes.size();
      });
}

// ===========================================================================
// 15L-a2: the morsehgp3d-run.v1 durable file archive.
// ===========================================================================

namespace {

constexpr std::array<std::uint8_t, 8U> run_magic{
    'M', 'H', '3', 'D', 'R', 'U', 'N', '1'};
constexpr std::array<std::uint8_t, 8U> end_magic{
    'M', 'H', '3', 'D', 'E', 'N', 'D', '1'};
constexpr std::size_t run_header_byte_count = 128U;
constexpr std::uint8_t frame_region_tag = 0x01U;
constexpr std::uint8_t footer_region_tag = 0x02U;
constexpr std::string_view temporary_suffix = ".tmp";

struct ArchiveFailure {
  ExactDirectMorseForestRunArchiveDecision decision{
      ExactDirectMorseForestRunArchiveDecision::no_io_failed};
};

[[noreturn]] void archive_fail(
    ExactDirectMorseForestRunArchiveDecision decision) {
  throw ArchiveFailure{decision};
}

struct UniqueFd {
  int fd{-1};
  UniqueFd() noexcept = default;
  explicit UniqueFd(int value) noexcept : fd(value) {}
  UniqueFd(const UniqueFd&) = delete;
  UniqueFd& operator=(const UniqueFd&) = delete;
  UniqueFd(UniqueFd&& other) noexcept : fd(other.fd) { other.fd = -1; }
  UniqueFd& operator=(UniqueFd&& other) noexcept {
    if (this != &other) {
      reset();
      fd = other.fd;
      other.fd = -1;
    }
    return *this;
  }
  ~UniqueFd() { reset(); }
  void reset() noexcept {
    if (fd >= 0) {
      static_cast<void>(::close(fd));
      fd = -1;
    }
  }
  [[nodiscard]] bool valid() const noexcept { return fd >= 0; }
};

[[nodiscard]] bool plain_file_name(const std::string& name) noexcept {
  return !name.empty() && name.find('/') == std::string::npos &&
         name != "." && name != ".." &&
         name.find('\0') == std::string::npos;
}

void write_all_bytes(int fd, std::span<const std::uint8_t> bytes) {
  std::size_t written = 0U;
  while (written < bytes.size()) {
    const ::ssize_t step = ::write(
        fd, bytes.data() + written, bytes.size() - written);
    if (step < 0) {
      if (errno == EINTR) {
        continue;
      }
      archive_fail(ExactDirectMorseForestRunArchiveDecision::no_io_failed);
    }
    written += static_cast<std::size_t>(step);
  }
}

[[nodiscard]] std::size_t read_some_bytes(
    int fd, std::uint8_t* data, std::size_t count) {
  std::size_t consumed = 0U;
  while (consumed < count) {
    const ::ssize_t step = ::read(fd, data + consumed, count - consumed);
    if (step < 0) {
      if (errno == EINTR) {
        continue;
      }
      archive_fail(ExactDirectMorseForestRunArchiveDecision::no_io_failed);
    }
    if (step == 0) {
      break;
    }
    consumed += static_cast<std::size_t>(step);
  }
  return consumed;
}

void append_u64_bytes(std::vector<std::uint8_t>& out, std::uint64_t value) {
  for (std::size_t index = 0U; index < 8U; ++index) {
    out.push_back(
        static_cast<std::uint8_t>((value >> (8U * (7U - index))) & 0xFFU));
  }
}

[[nodiscard]] std::uint64_t parse_u64_bytes(const std::uint8_t* data) {
  std::uint64_t value = 0U;
  for (std::size_t index = 0U; index < 8U; ++index) {
    value = (value << 8U) | data[index];
  }
  return value;
}

[[nodiscard]] std::vector<std::uint8_t> encode_run_header(
    const ExactDirectMorseForestRunArchiveIdentity& identity) {
  std::vector<std::uint8_t> out;
  out.reserve(run_header_byte_count);
  out.insert(out.end(), run_magic.begin(), run_magic.end());
  for (const std::uint32_t schema :
       {direct_morse_forest_durable_run_archive_schema_version,
        direct_morse_forest_output_segment_schema_version}) {
    for (std::size_t index = 0U; index < 4U; ++index) {
      out.push_back(
          static_cast<std::uint8_t>((schema >> (8U * (3U - index))) & 0xFFU));
    }
  }
  for (const contract::CanonicalId* digest :
       {&identity.manifest_digest,
        &identity.source_identity_digest,
        &identity.source_higher_canonical_cloud_digest}) {
    out.insert(
        out.end(), digest->bytes().begin(), digest->bytes().end());
  }
  append_u64_bytes(out, static_cast<std::uint64_t>(identity.point_count));
  append_u64_bytes(
      out, static_cast<std::uint64_t>(identity.effective_maximum_order));
  if (out.size() != run_header_byte_count) {
    archive_fail(
        ExactDirectMorseForestRunArchiveDecision::no_identity_rejected);
  }
  return out;
}

// The incremental whole-archive digest absorbs every byte in file order.
struct ArchiveDigest {
  contract::CanonicalSha256Builder builder;
  std::size_t byte_count{};

  void absorb(std::span<const std::uint8_t> bytes) {
    builder.update(std::string_view{
        reinterpret_cast<const char*>(bytes.data()), bytes.size()});
    byte_count += bytes.size();
  }
};

}  // namespace

bool ExactDirectMorseForestRunArchiveWriteResult::certified_published()
    const noexcept {
  return decision ==
             ExactDirectMorseForestRunArchiveDecision::
                 complete_atomic_run_archive &&
         final_published && temporary_removed;
}

bool ExactDirectMorseForestRunArchiveLoadResult::certified_loaded()
    const noexcept {
  return decision ==
             ExactDirectMorseForestRunArchiveDecision::
                 complete_atomic_run_archive &&
         !validated_prefix_only;
}

bool ExactDirectMorseForestRunArchiveRecoveryResult::certified_recovered()
    const noexcept {
  return decision == ExactDirectMorseForestRunArchiveRecoveryDecision::
                         absent ||
         decision == ExactDirectMorseForestRunArchiveRecoveryDecision::
                         torn_temporary_discarded ||
         decision == ExactDirectMorseForestRunArchiveRecoveryDecision::
                         valid_final_present;
}

struct ExactDirectMorseForestRunArchiveWriter::Impl {
  UniqueFd directory;
  UniqueFd temporary;
  std::string temporary_name;
  std::string final_name;
  ExactDirectMorseForestRunArchiveIdentity identity{};
  ExactDirectMorseForestSegmentCodecLimits limits{};
  ArchiveDigest digest;
  ExactDirectMorseForestSegmentCursor chain_cursor{};
  bool chain_seeded{false};
  std::size_t appended_segment_count{};
  bool finalized{false};

  void write_and_absorb(std::span<const std::uint8_t> bytes) {
    write_all_bytes(temporary.fd, bytes);
    digest.absorb(bytes);
  }

  void abandon() noexcept {
    if (temporary.valid()) {
      temporary.reset();
      if (directory.valid()) {
        static_cast<void>(
            ::unlinkat(directory.fd, temporary_name.c_str(), 0));
        static_cast<void>(::fsync(directory.fd));
      }
    }
  }
};

ExactDirectMorseForestRunArchiveWriter::
    ExactDirectMorseForestRunArchiveWriter() noexcept = default;

ExactDirectMorseForestRunArchiveWriter::
    ~ExactDirectMorseForestRunArchiveWriter() {
  if (impl_ != nullptr && !impl_->finalized) {
    impl_->abandon();
  }
}

ExactDirectMorseForestRunArchiveWriter::ExactDirectMorseForestRunArchiveWriter(
    ExactDirectMorseForestRunArchiveWriter&&) noexcept = default;
ExactDirectMorseForestRunArchiveWriter&
ExactDirectMorseForestRunArchiveWriter::operator=(
    ExactDirectMorseForestRunArchiveWriter&&) noexcept = default;

bool ExactDirectMorseForestRunArchiveWriter::open() const noexcept {
  return impl_ != nullptr && impl_->temporary.valid() && !impl_->finalized;
}

void ExactDirectMorseForestRunArchiveWriter::abandon() noexcept {
  if (impl_ != nullptr && !impl_->finalized) {
    impl_->abandon();
  }
}

ExactDirectMorseForestRunArchiveWriter
ExactDirectMorseForestRunArchiveWriter::open_new(
    const std::string& directory_path,
    const std::string& final_file_name,
    const ExactDirectMorseForestRunArchiveIdentity& identity,
    const ExactDirectMorseForestSegmentCodecLimits& limits,
    ExactDirectMorseForestRunArchiveDecision& decision) noexcept {
  ExactDirectMorseForestRunArchiveWriter writer;
  decision = ExactDirectMorseForestRunArchiveDecision::not_certified;
  try {
    if (!plain_file_name(final_file_name)) {
      decision =
          ExactDirectMorseForestRunArchiveDecision::no_directory_rejected;
      return writer;
    }
    auto impl = std::make_unique<Impl>();
    impl->identity = identity;
    impl->limits = limits;
    impl->final_name = final_file_name;
    impl->temporary_name =
        final_file_name + std::string{temporary_suffix};
    impl->directory = UniqueFd{::open(
        directory_path.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC)};
    if (!impl->directory.valid()) {
      decision =
          ExactDirectMorseForestRunArchiveDecision::no_directory_rejected;
      return writer;
    }
    struct ::stat existing {};
    if (::fstatat(
            impl->directory.fd,
            final_file_name.c_str(),
            &existing,
            AT_SYMLINK_NOFOLLOW) == 0) {
      decision = ExactDirectMorseForestRunArchiveDecision::
          no_existing_final_rejected;
      return writer;
    }
    impl->temporary = UniqueFd{::openat(
        impl->directory.fd,
        impl->temporary_name.c_str(),
        O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC,
        0644)};
    if (!impl->temporary.valid()) {
      decision = ExactDirectMorseForestRunArchiveDecision::
          no_existing_final_rejected;
      return writer;
    }
    impl->write_and_absorb(encode_run_header(identity));
    writer.impl_ = std::move(impl);
    decision = ExactDirectMorseForestRunArchiveDecision::
        complete_atomic_run_archive;
    return writer;
  } catch (const ArchiveFailure& failure) {
    decision = failure.decision;
    return writer;
  } catch (const std::bad_alloc&) {
    decision =
        ExactDirectMorseForestRunArchiveDecision::no_allocation_failed;
    return writer;
  } catch (...) {
    decision = ExactDirectMorseForestRunArchiveDecision::no_io_failed;
    return writer;
  }
}

ExactDirectMorseForestRunArchiveDecision
ExactDirectMorseForestRunArchiveWriter::append_segment(
    const ExactDirectMorseForestBatchSegment& segment) noexcept {
  if (!open()) {
    return ExactDirectMorseForestRunArchiveDecision::no_io_failed;
  }
  try {
    if (impl_->chain_seeded) {
      if (!(segment.begin_cursor == impl_->chain_cursor)) {
        return ExactDirectMorseForestRunArchiveDecision::no_chain_rejected;
      }
    }
    const auto encoded = encode_exact_direct_morse_forest_batch_segment(
        segment, impl_->limits);
    if (!encoded.certified_encoded()) {
      return ExactDirectMorseForestRunArchiveDecision::no_codec_rejected;
    }
    std::vector<std::uint8_t> frame;
    frame.reserve(1U + 8U + 8U + encoded.bytes.size() + 32U);
    frame.push_back(frame_region_tag);
    append_u64_bytes(
        frame,
        static_cast<std::uint64_t>(8U + encoded.bytes.size() + 32U));
    append_u64_bytes(
        frame, static_cast<std::uint64_t>(impl_->appended_segment_count));
    frame.insert(frame.end(), encoded.bytes.begin(), encoded.bytes.end());
    frame.insert(
        frame.end(),
        segment.end_cursor.chain_digest.bytes().begin(),
        segment.end_cursor.chain_digest.bytes().end());
    impl_->write_and_absorb(frame);
    impl_->chain_cursor = segment.end_cursor;
    impl_->chain_seeded = true;
    ++impl_->appended_segment_count;
    return ExactDirectMorseForestRunArchiveDecision::
        complete_atomic_run_archive;
  } catch (const ArchiveFailure& failure) {
    return failure.decision;
  } catch (const std::bad_alloc&) {
    return ExactDirectMorseForestRunArchiveDecision::no_allocation_failed;
  } catch (...) {
    return ExactDirectMorseForestRunArchiveDecision::no_io_failed;
  }
}

ExactDirectMorseForestRunArchiveWriteResult
ExactDirectMorseForestRunArchiveWriter::finalize(
    const ExactDirectMorseForestFinalSeal& seal) noexcept {
  ExactDirectMorseForestRunArchiveWriteResult output;
  if (!open()) {
    output.decision = ExactDirectMorseForestRunArchiveDecision::no_io_failed;
    return output;
  }
  try {
    output.appended_segment_count = impl_->appended_segment_count;
    if (impl_->chain_seeded &&
        !(seal.final_cursor == impl_->chain_cursor)) {
      output.decision =
          ExactDirectMorseForestRunArchiveDecision::no_seal_rejected;
      return output;
    }
    const auto encoded_seal = encode_exact_direct_morse_forest_final_seal(
        seal, impl_->limits);
    if (!encoded_seal.certified_encoded()) {
      output.decision =
          ExactDirectMorseForestRunArchiveDecision::no_codec_rejected;
      return output;
    }
    std::vector<std::uint8_t> footer;
    footer.push_back(footer_region_tag);
    footer.insert(footer.end(), end_magic.begin(), end_magic.end());
    append_u64_bytes(
        footer,
        static_cast<std::uint64_t>(impl_->appended_segment_count));
    footer.insert(
        footer.end(),
        impl_->identity.terminal_source_chain_digest.bytes().begin(),
        impl_->identity.terminal_source_chain_digest.bytes().end());
    append_u64_bytes(
        footer, static_cast<std::uint64_t>(encoded_seal.bytes.size()));
    footer.insert(
        footer.end(), encoded_seal.bytes.begin(), encoded_seal.bytes.end());
    impl_->write_and_absorb(footer);
    output.archive_digest = impl_->digest.builder.finalize();
    std::vector<std::uint8_t> tail;
    tail.insert(
        tail.end(),
        output.archive_digest.bytes().begin(),
        output.archive_digest.bytes().end());
    tail.insert(tail.end(), end_magic.begin(), end_magic.end());
    write_all_bytes(impl_->temporary.fd, tail);
    output.published_byte_count = impl_->digest.byte_count + tail.size();
    if (::fdatasync(impl_->temporary.fd) != 0) {
      archive_fail(ExactDirectMorseForestRunArchiveDecision::no_io_failed);
    }
    if (::linkat(
            impl_->directory.fd,
            impl_->temporary_name.c_str(),
            impl_->directory.fd,
            impl_->final_name.c_str(),
            0) != 0) {
      archive_fail(ExactDirectMorseForestRunArchiveDecision::no_io_failed);
    }
    output.final_published = true;
    if (::unlinkat(
            impl_->directory.fd, impl_->temporary_name.c_str(), 0) != 0) {
      archive_fail(ExactDirectMorseForestRunArchiveDecision::no_io_failed);
    }
    output.temporary_removed = true;
    if (::fsync(impl_->directory.fd) != 0) {
      archive_fail(ExactDirectMorseForestRunArchiveDecision::no_io_failed);
    }
    impl_->temporary.reset();
    impl_->finalized = true;
    output.decision = ExactDirectMorseForestRunArchiveDecision::
        complete_atomic_run_archive;
    return output;
  } catch (const ArchiveFailure& failure) {
    output.decision = failure.decision;
    return output;
  } catch (const std::bad_alloc&) {
    output.decision =
        ExactDirectMorseForestRunArchiveDecision::no_allocation_failed;
    return output;
  } catch (...) {
    output.decision = ExactDirectMorseForestRunArchiveDecision::no_io_failed;
    return output;
  }
}

namespace {

// Shared bounded validation pass.  With a consumer it delivers each decoded
// segment; without one it only validates.  Every consumed byte feeds the
// running archive digest until the sealed digest bytes themselves.
[[nodiscard]] ExactDirectMorseForestRunArchiveLoadResult
validate_run_archive(
    int directory_fd,
    const std::string& final_file_name,
    const ExactDirectMorseForestRunArchiveIdentity& expected_identity,
    const ExactDirectMorseForestSegmentCodecLimits& limits,
    const ExactDirectMorseForestRunArchiveConsumerView* consumer) {
  ExactDirectMorseForestRunArchiveLoadResult output;
  UniqueFd file{::openat(
      directory_fd, final_file_name.c_str(), O_RDONLY | O_CLOEXEC)};
  if (!file.valid()) {
    output.decision =
        ExactDirectMorseForestRunArchiveDecision::no_io_failed;
    return output;
  }
  ArchiveDigest digest;
  const auto read_exact = [&](std::uint8_t* data,
                              std::size_t count,
                              bool absorb) {
    if (read_some_bytes(file.fd, data, count) != count) {
      archive_fail(
          ExactDirectMorseForestRunArchiveDecision::no_archive_digest_rejected);
    }
    if (absorb) {
      digest.absorb(std::span<const std::uint8_t>{data, count});
    }
  };
  std::array<std::uint8_t, run_header_byte_count> header{};
  read_exact(header.data(), header.size(), true);
  const auto expected_header = encode_run_header(expected_identity);
  if (!std::equal(
          header.begin(), header.end(), expected_header.begin())) {
    output.decision =
        ExactDirectMorseForestRunArchiveDecision::no_identity_rejected;
    return output;
  }
  output.identity = expected_identity;
  ExactDirectMorseForestSegmentCursor chain_cursor{};
  bool chain_seeded = false;
  std::vector<std::uint8_t> frame;
  for (;;) {
    std::uint8_t tag = 0U;
    read_exact(&tag, 1U, true);
    if (tag == footer_region_tag) {
      break;
    }
    if (tag != frame_region_tag) {
      output.validated_prefix_only = true;
      output.decision =
          ExactDirectMorseForestRunArchiveDecision::no_chain_rejected;
      return output;
    }
    std::array<std::uint8_t, 8U> length_bytes{};
    read_exact(length_bytes.data(), length_bytes.size(), true);
    const std::uint64_t frame_length = parse_u64_bytes(length_bytes.data());
    if (frame_length < 8U + envelope_byte_count + 32U ||
        frame_length >
            8U + limits.maximum_encoded_byte_count + 32U) {
      output.validated_prefix_only = true;
      output.decision =
          ExactDirectMorseForestRunArchiveDecision::no_limit_exceeded;
      return output;
    }
    frame.assign(static_cast<std::size_t>(frame_length), 0U);
    read_exact(frame.data(), frame.size(), true);
    const std::uint64_t segment_index = parse_u64_bytes(frame.data());
    if (segment_index != output.delivered_segment_count) {
      output.validated_prefix_only = true;
      output.decision = ExactDirectMorseForestRunArchiveDecision::
          no_segment_order_rejected;
      return output;
    }
    const std::span<const std::uint8_t> encoded{
        frame.data() + 8U, frame.size() - 8U - 32U};
    const auto decoded = decode_exact_direct_morse_forest_batch_segment(
        encoded, limits);
    if (!decoded.certified_decoded()) {
      output.validated_prefix_only = true;
      output.decision =
          ExactDirectMorseForestRunArchiveDecision::no_codec_rejected;
      return output;
    }
    std::array<std::uint8_t, 32U> chain_bytes{};
    std::memcpy(
        chain_bytes.data(), frame.data() + frame.size() - 32U, 32U);
    if (contract::CanonicalId{chain_bytes} !=
            decoded.segment.end_cursor.chain_digest ||
        (chain_seeded &&
         !(decoded.segment.begin_cursor == chain_cursor))) {
      output.validated_prefix_only = true;
      output.decision =
          ExactDirectMorseForestRunArchiveDecision::no_chain_rejected;
      return output;
    }
    chain_cursor = decoded.segment.end_cursor;
    chain_seeded = true;
    if (consumer != nullptr) {
      if (consumer->state == nullptr || consumer->consume == nullptr ||
          !consumer->consume(consumer->state, decoded.segment)) {
        output.validated_prefix_only = true;
        output.decision = ExactDirectMorseForestRunArchiveDecision::
            no_consumer_rejected;
        return output;
      }
    }
    ++output.delivered_segment_count;
  }
  std::array<std::uint8_t, 8U> observed_end_magic{};
  read_exact(observed_end_magic.data(), observed_end_magic.size(), true);
  if (!std::equal(
          observed_end_magic.begin(),
          observed_end_magic.end(),
          end_magic.begin())) {
    output.decision =
        ExactDirectMorseForestRunArchiveDecision::no_seal_rejected;
    return output;
  }
  std::array<std::uint8_t, 8U> count_bytes{};
  read_exact(count_bytes.data(), count_bytes.size(), true);
  std::array<std::uint8_t, 32U> terminal_chain_bytes{};
  read_exact(terminal_chain_bytes.data(), terminal_chain_bytes.size(), true);
  std::array<std::uint8_t, 8U> seal_length_bytes{};
  read_exact(seal_length_bytes.data(), seal_length_bytes.size(), true);
  const std::uint64_t seal_length = parse_u64_bytes(seal_length_bytes.data());
  if (parse_u64_bytes(count_bytes.data()) !=
          output.delivered_segment_count ||
      contract::CanonicalId{terminal_chain_bytes} !=
          expected_identity.terminal_source_chain_digest) {
    output.decision =
        ExactDirectMorseForestRunArchiveDecision::no_seal_rejected;
    return output;
  }
  if (seal_length < envelope_byte_count ||
      seal_length > limits.maximum_encoded_byte_count) {
    output.decision =
        ExactDirectMorseForestRunArchiveDecision::no_limit_exceeded;
    return output;
  }
  frame.assign(static_cast<std::size_t>(seal_length), 0U);
  read_exact(frame.data(), frame.size(), true);
  const auto decoded_seal = decode_exact_direct_morse_forest_final_seal(
      frame, limits);
  if (!decoded_seal.certified_decoded() ||
      (chain_seeded &&
       !(decoded_seal.seal.final_cursor == chain_cursor))) {
    output.decision =
        ExactDirectMorseForestRunArchiveDecision::no_seal_rejected;
    return output;
  }
  output.final_seal = decoded_seal.seal;
  output.validated_byte_count = digest.byte_count;
  output.archive_digest = digest.builder.finalize();
  std::array<std::uint8_t, 32U> declared_digest{};
  read_exact(declared_digest.data(), declared_digest.size(), false);
  std::array<std::uint8_t, 8U> trailing_magic{};
  read_exact(trailing_magic.data(), trailing_magic.size(), false);
  std::uint8_t overflow = 0U;
  if (contract::CanonicalId{declared_digest} != output.archive_digest ||
      !std::equal(
          trailing_magic.begin(),
          trailing_magic.end(),
          end_magic.begin()) ||
      read_some_bytes(file.fd, &overflow, 1U) != 0U) {
    output.decision = ExactDirectMorseForestRunArchiveDecision::
        no_archive_digest_rejected;
    return output;
  }
  output.decision =
      ExactDirectMorseForestRunArchiveDecision::complete_atomic_run_archive;
  return output;
}

}  // namespace

ExactDirectMorseForestRunArchiveLoadResult
load_exact_direct_morse_forest_run_archive(
    const std::string& directory_path,
    const std::string& final_file_name,
    const ExactDirectMorseForestRunArchiveIdentity& expected_identity,
    const ExactDirectMorseForestSegmentCodecLimits& limits,
    const ExactDirectMorseForestRunArchiveConsumerView& consumer) noexcept {
  ExactDirectMorseForestRunArchiveLoadResult output;
  try {
    if (!plain_file_name(final_file_name)) {
      output.decision =
          ExactDirectMorseForestRunArchiveDecision::no_directory_rejected;
      return output;
    }
    UniqueFd directory{::open(
        directory_path.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC)};
    if (!directory.valid()) {
      output.decision =
          ExactDirectMorseForestRunArchiveDecision::no_directory_rejected;
      return output;
    }
    return validate_run_archive(
        directory.fd, final_file_name, expected_identity, limits, &consumer);
  } catch (const ArchiveFailure& failure) {
    output.decision = failure.decision;
    return output;
  } catch (const std::bad_alloc&) {
    output.decision =
        ExactDirectMorseForestRunArchiveDecision::no_allocation_failed;
    return output;
  } catch (...) {
    output.decision = ExactDirectMorseForestRunArchiveDecision::no_io_failed;
    return output;
  }
}

ExactDirectMorseForestRunArchiveRecoveryResult
recover_exact_direct_morse_forest_run_archive_directory(
    const std::string& directory_path,
    const std::string& final_file_name,
    const ExactDirectMorseForestRunArchiveIdentity& expected_identity,
    const ExactDirectMorseForestSegmentCodecLimits& limits) noexcept {
  ExactDirectMorseForestRunArchiveRecoveryResult output;
  try {
    if (!plain_file_name(final_file_name)) {
      output.decision = ExactDirectMorseForestRunArchiveRecoveryDecision::
          no_directory_rejected;
      return output;
    }
    UniqueFd directory{::open(
        directory_path.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC)};
    if (!directory.valid()) {
      output.decision = ExactDirectMorseForestRunArchiveRecoveryDecision::
          no_directory_rejected;
      return output;
    }
    const std::string temporary_name =
        final_file_name + std::string{temporary_suffix};
    struct ::stat state {};
    if (::fstatat(
            directory.fd,
            temporary_name.c_str(),
            &state,
            AT_SYMLINK_NOFOLLOW) == 0) {
      if (::unlinkat(directory.fd, temporary_name.c_str(), 0) != 0 ||
          ::fsync(directory.fd) != 0) {
        output.decision =
            ExactDirectMorseForestRunArchiveRecoveryDecision::no_io_failed;
        return output;
      }
      output.discarded_temporary_count = 1U;
    }
    if (::fstatat(
            directory.fd,
            final_file_name.c_str(),
            &state,
            AT_SYMLINK_NOFOLLOW) != 0) {
      output.decision = output.discarded_temporary_count != 0U
          ? ExactDirectMorseForestRunArchiveRecoveryDecision::
                torn_temporary_discarded
          : ExactDirectMorseForestRunArchiveRecoveryDecision::absent;
      return output;
    }
    output.final_present = true;
    const auto validation = validate_run_archive(
        directory.fd, final_file_name, expected_identity, limits, nullptr);
    output.decision = validation.certified_loaded()
        ? ExactDirectMorseForestRunArchiveRecoveryDecision::
              valid_final_present
        : ExactDirectMorseForestRunArchiveRecoveryDecision::
              invalid_final_present;
    return output;
  } catch (const ArchiveFailure&) {
    output.decision = ExactDirectMorseForestRunArchiveRecoveryDecision::
        no_io_failed;
    return output;
  } catch (...) {
    output.decision = ExactDirectMorseForestRunArchiveRecoveryDecision::
        no_io_failed;
    return output;
  }
}

}  // namespace morsehgp3d::hierarchy
