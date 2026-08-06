#include "morsehgp3d/hierarchy/direct_morse_forest_durable_run_archive.hpp"

#include "morsehgp3d/contract/canonical_id.hpp"

#include <array>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

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

}  // namespace morsehgp3d::hierarchy
