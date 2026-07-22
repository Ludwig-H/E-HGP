#include "morsehgp3d/hierarchy/pair_support_stream_codec.hpp"

#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/exact/center.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::contract::CanonicalId;
using morsehgp3d::contract::CanonicalSha256Builder;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactCenter3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::hierarchy::ExactPairSupportCheckpointManifest;
using morsehgp3d::hierarchy::ExactPairSupportExtraShellDiagnostic;
using morsehgp3d::hierarchy::ExactPairSupportFrontierEntry;
using morsehgp3d::hierarchy::ExactPairSupportPendingProduct;
using morsehgp3d::hierarchy::ExactPairSupportPendingStage;
using morsehgp3d::hierarchy::ExactPairSupportStreamAudit;
using morsehgp3d::hierarchy::ExactPairSupportStreamBudget;
using morsehgp3d::hierarchy::ExactPairSupportStreamChunk;
using morsehgp3d::hierarchy::ExactPairSupportStreamCodecLimits;
using morsehgp3d::hierarchy::ExactPairSupportStreamDecodeDecision;
using morsehgp3d::hierarchy::ExactPairSupportStreamDecodeResult;
using morsehgp3d::hierarchy::ExactPairSupportStreamStatus;
using morsehgp3d::hierarchy::ExactPairSupportStopReason;
using morsehgp3d::hierarchy::ExactPairSupportWitnessNodeEntry;
using morsehgp3d::hierarchy::decode_exact_pair_support_stream_chunk;
using morsehgp3d::hierarchy::encode_exact_pair_support_stream_chunk;

constexpr std::size_t wire_header_byte_count = 30U;
constexpr std::size_t wire_checksum_byte_count = CanonicalId::byte_count;
constexpr std::size_t wire_payload_length_offset = 22U;
constexpr std::size_t payload_status_offset = 320U;
constexpr std::size_t payload_event_count_offset = 322U;
constexpr std::size_t payload_first_exact_text_length_offset = 346U;
constexpr std::string_view wire_checksum_domain =
    "MorseHGP3D/phase9/pair-support/chunk-wire/v1/sha256/";

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Exception, typename Function>
void check_throws(Function&& function, const std::string& message) {
  try {
    std::forward<Function>(function)();
  } catch (const Exception&) {
    return;
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: " << message << " (unexpected exception: "
              << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] CanonicalId identifier(std::uint8_t seed) {
  std::array<std::uint8_t, CanonicalId::byte_count> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    bytes[index] = static_cast<std::uint8_t>(
        static_cast<unsigned int>(seed) +
        static_cast<unsigned int>(index));
  }
  return CanonicalId{bytes};
}

[[nodiscard]] ExactCenter3 center(
    const ExactRational& x,
    const ExactRational& y,
    const ExactRational& z) {
  return ExactCenter3{std::array<ExactRational, 3U>{x, y, z}};
}

[[nodiscard]] ExactPairSupportStreamAudit audit_from(std::size_t first) {
  ExactPairSupportStreamAudit audit;
  const std::array members{
      &ExactPairSupportStreamAudit::total_pair_count,
      &ExactPairSupportStreamAudit::work_unit_count,
      &ExactPairSupportStreamAudit::support_product_visit_count,
      &ExactPairSupportStreamAudit::support_product_expansion_count,
      &ExactPairSupportStreamAudit::self_product_expansion_count,
      &ExactPairSupportStreamAudit::cross_product_expansion_count,
      &ExactPairSupportStreamAudit::diagonal_leaf_discard_count,
      &ExactPairSupportStreamAudit::
          diagonal_product_rank_search_skip_count,
      &ExactPairSupportStreamAudit::rank_prune_search_count,
      &ExactPairSupportStreamAudit::witness_node_visit_count,
      &ExactPairSupportStreamAudit::exact_phi_aabb_bound_count,
      &ExactPairSupportStreamAudit::
          exact_anchor_ball_minimum_aabb_bound_count,
      &ExactPairSupportStreamAudit::
          certified_anchor_noninterior_subtree_count,
      &ExactPairSupportStreamAudit::
          certified_anchor_noninterior_point_count,
      &ExactPairSupportStreamAudit::
          certified_anchor_shell_tangent_subtree_count,
      &ExactPairSupportStreamAudit::
          equality_or_positive_bound_descent_count,
      &ExactPairSupportStreamAudit::
          strict_interior_witness_subtree_count,
      &ExactPairSupportStreamAudit::strict_interior_witness_point_count,
      &ExactPairSupportStreamAudit::rank_pruned_product_count,
      &ExactPairSupportStreamAudit::rank_pruned_pair_count,
      &ExactPairSupportStreamAudit::leaf_pair_classification_count,
      &ExactPairSupportStreamAudit::global_closed_ball_query_count,
      &ExactPairSupportStreamAudit::point_classification_count,
      &ExactPairSupportStreamAudit::closed_ball_node_visit_count,
      &ExactPairSupportStreamAudit::
          exact_closed_ball_minimum_aabb_bound_count,
      &ExactPairSupportStreamAudit::
          exact_closed_ball_maximum_aabb_bound_count,
      &ExactPairSupportStreamAudit::
          closed_ball_bulk_interior_subtree_count,
      &ExactPairSupportStreamAudit::
          closed_ball_bulk_interior_point_count,
      &ExactPairSupportStreamAudit::
          closed_ball_bulk_exterior_subtree_count,
      &ExactPairSupportStreamAudit::
          closed_ball_bulk_exterior_point_count,
      &ExactPairSupportStreamAudit::early_closed_rank_rejection_count,
      &ExactPairSupportStreamAudit::
          exact_point_distance_evaluation_count,
      &ExactPairSupportStreamAudit::accepted_event_count,
      &ExactPairSupportStreamAudit::
          relevant_extra_shell_diagnostic_count,
      &ExactPairSupportStreamAudit::emitted_point_id_reference_count,
      &ExactPairSupportStreamAudit::above_rank_pair_count,
      &ExactPairSupportStreamAudit::maximum_frontier_entry_count,
      &ExactPairSupportStreamAudit::
          maximum_witness_frontier_entry_count,
      &ExactPairSupportStreamAudit::
          maximum_closed_ball_frontier_entry_count,
      &ExactPairSupportStreamAudit::remaining_frontier_pair_count,
      &ExactPairSupportStreamAudit::resolved_pair_count};
  std::size_t value = first;
  for (const auto member : members) {
    audit.*member = value;
    ++value;
  }
  audit.pair_partition_accounting_certified = true;
  return audit;
}

[[nodiscard]] ExactPairSupportCheckpointManifest manifest() {
  return ExactPairSupportCheckpointManifest{
      1U,
      1U,
      19U,
      37U,
      19U,
      10U,
      10U,
      11U,
      identifier(1U),
      identifier(33U),
      identifier(65U)};
}

[[nodiscard]] ExactPairSupportStreamChunk fixture_chunk() {
  ExactPairSupportStreamChunk chunk;
  chunk.manifest = manifest();
  chunk.budget = ExactPairSupportStreamBudget{
      101U, 102U, 103U, 104U, 105U, 106U, 107U};
  chunk.chunk_sequence = 7U;
  chunk.first_output_record_index = 9U;
  chunk.source_checkpoint_digest = identifier(97U);
  chunk.previous_output_chain_digest = identifier(129U);
  chunk.output_chain_digest = identifier(161U);
  chunk.status = ExactPairSupportStreamStatus::budget_exhausted;
  chunk.stop_reason = ExactPairSupportStopReason::work_unit_limit;

  chunk.events.push_back(morsehgp3d::hierarchy::ExactPairSupportEvent{
      {2U, 11U},
      center(
          ExactRational{BigInt{1}, BigInt{2}},
          ExactRational{BigInt{-2}, BigInt{3}},
          ExactRational{BigInt{5}, BigInt{7}}),
      ExactLevel{BigInt{9}, BigInt{4}},
      {3U, 5U},
      6U,
      13U});
  chunk.relevant_extra_shell_diagnostics.push_back(
      ExactPairSupportExtraShellDiagnostic{
          {4U, 17U},
          center(
              ExactRational{BigInt{-1}, BigInt{5}},
              ExactRational{BigInt{2}, BigInt{9}},
              ExactRational{BigInt{7}, BigInt{11}}),
          ExactLevel{BigInt{25}, BigInt{9}},
          {8U},
          5U,
          12U,
          4U,
          7U,
          12U});
  chunk.record_order = {
      ExactPairSupportStreamChunk::RecordKind::event,
      ExactPairSupportStreamChunk::RecordKind::
          relevant_extra_shell_diagnostic};
  chunk.cumulative_audit_before = audit_from(200U);
  chunk.cumulative_audit_after = audit_from(300U);

  chunk.next_checkpoint.manifest = manifest();
  chunk.next_checkpoint.next_chunk_sequence = 8U;
  chunk.next_checkpoint.output_record_count = 11U;
  chunk.next_checkpoint.output_chain_digest = identifier(193U);
  chunk.next_checkpoint.frontier = {
      ExactPairSupportFrontierEntry{
          0x0102030405060708ULL,
          0x1112131415161718ULL,
          0x2122232425262728ULL,
          0x3132333435363738ULL,
          0x4142434445464748ULL,
          0x5152535455565758ULL,
          1U},
      ExactPairSupportFrontierEntry{7U, 8U, 9U, 10U, 11U, 12U, 0U}};
  ExactPairSupportPendingProduct pending;
  pending.product =
      ExactPairSupportFrontierEntry{13U, 14U, 15U, 16U, 17U, 18U, 0U};
  pending.stage = ExactPairSupportPendingStage::classify_leaf;
  pending.rank_search_started = true;
  pending.witness_frontier = {
      ExactPairSupportWitnessNodeEntry{19U, 20U, 21U},
      ExactPairSupportWitnessNodeEntry{22U, 23U, 24U}};
  pending.strict_witness_receipts = {
      ExactPairSupportWitnessNodeEntry{25U, 26U, 27U}};
  pending.deferred_expansion_node =
      ExactPairSupportWitnessNodeEntry{28U, 29U, 30U};
  pending.strict_witness_point_count = 31U;
  chunk.next_checkpoint.pending_product = std::move(pending);
  chunk.next_checkpoint.cumulative_audit = audit_from(400U);
  chunk.next_checkpoint.checkpoint_digest = identifier(211U);
  chunk.candidate_prepared = true;
  chunk.no_forbidden_global_structure_materialized = true;
  chunk.hierarchy_reduction_performed = false;
  return chunk;
}

[[nodiscard]] ExactPairSupportStreamCodecLimits fixture_limits() {
  return ExactPairSupportStreamCodecLimits{
      1U * 1024U * 1024U,
      8U,
      8U,
      8U,
      16U,
      128U,
      1024U};
}

[[nodiscard]] std::uint64_t read_u64(
    const std::vector<std::uint8_t>& bytes,
    std::size_t offset) {
  std::uint64_t value = 0U;
  for (std::size_t index = 0U; index < 8U; ++index) {
    value = (value << 8U) |
            static_cast<std::uint64_t>(bytes[offset + index]);
  }
  return value;
}

void write_u64(
    std::vector<std::uint8_t>& bytes,
    std::size_t offset,
    std::uint64_t value) {
  for (std::size_t index = 0U; index < 8U; ++index) {
    const std::size_t shift = (7U - index) * 8U;
    bytes[offset + index] = static_cast<std::uint8_t>(value >> shift);
  }
}

void append_u64(std::vector<std::uint8_t>& bytes, std::uint64_t value) {
  for (std::size_t index = 0U; index < 8U; ++index) {
    const std::size_t shift = (7U - index) * 8U;
    bytes.push_back(static_cast<std::uint8_t>(value >> shift));
  }
}

void rehash_wire(std::vector<std::uint8_t>& wire) {
  check(
      wire.size() >= wire_checksum_byte_count,
      "the hostile-test wire is large enough for a checksum");
  if (wire.size() < wire_checksum_byte_count) {
    return;
  }
  const std::size_t checksum_offset =
      wire.size() - wire_checksum_byte_count;
  CanonicalSha256Builder builder;
  builder.update(wire_checksum_domain);
  builder.update(std::span<const std::uint8_t>{wire}.first(checksum_offset));
  const CanonicalId checksum = builder.finalize();
  for (std::size_t index = 0U;
       index < wire_checksum_byte_count;
       ++index) {
    wire[checksum_offset + index] = checksum.bytes()[index];
  }
}

[[nodiscard]] std::size_t find_bytes(
    const std::vector<std::uint8_t>& haystack,
    const std::vector<std::uint8_t>& needle) {
  const auto found = std::search(
      haystack.begin(), haystack.end(), needle.begin(), needle.end());
  if (found == haystack.end()) {
    throw std::logic_error("the expected codec fixture bytes were not found");
  }
  return static_cast<std::size_t>(found - haystack.begin());
}

void check_decision(
    const std::vector<std::uint8_t>& encoded,
    const ExactPairSupportStreamCodecLimits& limits,
    ExactPairSupportStreamDecodeDecision expected,
    const std::string& message) {
  const ExactPairSupportStreamDecodeResult result =
      decode_exact_pair_support_stream_chunk(encoded, limits);
  check(
      result.decision == expected && !result.chunk.has_value() &&
          !result.accepted(),
      message);
}

void test_roundtrip_and_determinism() {
  const ExactPairSupportStreamChunk chunk = fixture_chunk();
  const ExactPairSupportStreamCodecLimits limits = fixture_limits();
  const std::vector<std::uint8_t> first =
      encode_exact_pair_support_stream_chunk(chunk, limits);
  const std::vector<std::uint8_t> second =
      encode_exact_pair_support_stream_chunk(chunk, limits);
  check(first == second, "chunk encoding is byte-for-byte deterministic");
  check(
      first.size() <= limits.maximum_encoded_byte_count,
      "chunk encoding respects the external byte cap");
  const ExactPairSupportStreamDecodeResult decoded =
      decode_exact_pair_support_stream_chunk(first, limits);
  check(
      decoded.accepted() && decoded.chunk == chunk,
      "every chunk and checkpoint field survives the canonical roundtrip");
  if (decoded.chunk.has_value()) {
    const std::vector<std::uint8_t> reencoded =
        encode_exact_pair_support_stream_chunk(*decoded.chunk, limits);
    check(
        reencoded == first,
        "accepted wire bytes have a unique canonical re-encoding");
  }

  CanonicalSha256Builder golden_builder;
  golden_builder.update(std::span<const std::uint8_t>{first});
  const std::string golden = golden_builder.finalize().to_lower_hex();
  check(
      golden ==
          "10c0559055cb857dca14fb6a6e16f6f12fb6c8680b808421cf8af20dbe56777b",
      "the complete canonical wire has a stable golden SHA-256 (observed " +
          golden + ")");
}

void test_hostile_envelope_and_checksum() {
  const ExactPairSupportStreamCodecLimits limits = fixture_limits();
  const std::vector<std::uint8_t> valid =
      encode_exact_pair_support_stream_chunk(fixture_chunk(), limits);

  for (std::size_t prefix = 0U; prefix < valid.size(); ++prefix) {
    const ExactPairSupportStreamDecodeResult result =
        decode_exact_pair_support_stream_chunk(
            std::span<const std::uint8_t>{valid}.first(prefix), limits);
    check(
        !result.accepted() && !result.chunk.has_value(),
        "every strict wire prefix fails closed");
  }

  std::vector<std::uint8_t> hostile = valid;
  hostile[0] ^= 1U;
  check_decision(
      hostile,
      limits,
      ExactPairSupportStreamDecodeDecision::invalid_magic,
      "an invalid magic fails before payload parsing");

  hostile = valid;
  hostile[19] = 2U;
  check_decision(
      hostile,
      limits,
      ExactPairSupportStreamDecodeDecision::unsupported_version,
      "an unknown wire version fails closed");

  hostile = valid;
  hostile[20] = 2U;
  check_decision(
      hostile,
      limits,
      ExactPairSupportStreamDecodeDecision::unsupported_kind,
      "an unknown envelope kind fails closed");

  hostile = valid;
  hostile[21] = 1U;
  check_decision(
      hostile,
      limits,
      ExactPairSupportStreamDecodeDecision::unsupported_flags,
      "nonzero unknown envelope flags fail closed");

  hostile = valid;
  write_u64(
      hostile,
      wire_payload_length_offset,
      read_u64(hostile, wire_payload_length_offset) + 1U);
  check_decision(
      hostile,
      limits,
      ExactPairSupportStreamDecodeDecision::payload_length_mismatch,
      "a forged payload length cannot expose a partial payload");

  hostile = valid;
  hostile.back() ^= 1U;
  check_decision(
      hostile,
      limits,
      ExactPairSupportStreamDecodeDecision::checksum_mismatch,
      "a corrupted checksum fails closed");

  hostile = valid;
  hostile[wire_header_byte_count + 17U] ^= 1U;
  check_decision(
      hostile,
      limits,
      ExactPairSupportStreamDecodeDecision::checksum_mismatch,
      "a corrupted payload fails before any nested allocation");

  hostile = valid;
  hostile.push_back(0U);
  check_decision(
      hostile,
      limits,
      ExactPairSupportStreamDecodeDecision::trailing_bytes,
      "wire bytes after the checksum are rejected");

  hostile = valid;
  const std::size_t checksum_offset =
      hostile.size() - wire_checksum_byte_count;
  hostile.insert(
      hostile.begin() + static_cast<std::ptrdiff_t>(checksum_offset), 0U);
  write_u64(
      hostile,
      wire_payload_length_offset,
      read_u64(hostile, wire_payload_length_offset) + 1U);
  rehash_wire(hostile);
  check_decision(
      hostile,
      limits,
      ExactPairSupportStreamDecodeDecision::trailing_bytes,
      "a checksummed trailing payload byte is rejected by exact EOF");
}

void test_hostile_payload_and_limits() {
  const ExactPairSupportStreamCodecLimits limits = fixture_limits();
  const ExactPairSupportStreamChunk chunk = fixture_chunk();
  const std::vector<std::uint8_t> valid =
      encode_exact_pair_support_stream_chunk(chunk, limits);
  std::vector<std::uint8_t> hostile = valid;

  hostile[wire_header_byte_count + payload_status_offset] = 0xffU;
  rehash_wire(hostile);
  check_decision(
      hostile,
      limits,
      ExactPairSupportStreamDecodeDecision::invalid_enumeration,
      "an unknown payload enum fails closed after checksum validation");

  hostile = valid;
  write_u64(
      hostile,
      wire_header_byte_count + payload_event_count_offset,
      std::numeric_limits<std::uint64_t>::max());
  rehash_wire(hostile);
  check_decision(
      hostile,
      limits,
      ExactPairSupportStreamDecodeDecision::count_limit_exceeded,
      "UINT64_MAX record counts are rejected before reserve");

  hostile = valid;
  write_u64(
      hostile,
      wire_header_byte_count + payload_event_count_offset,
      1000U);
  rehash_wire(hostile);
  ExactPairSupportStreamCodecLimits permissive_count_limits = limits;
  permissive_count_limits.maximum_record_count = 1000U;
  check_decision(
      hostile,
      permissive_count_limits,
      ExactPairSupportStreamDecodeDecision::truncated,
      "a count inside its configured cap but impossible in the remaining payload is rejected before reserve");

  hostile = valid;
  write_u64(
      hostile,
      wire_header_byte_count + payload_first_exact_text_length_offset,
      std::numeric_limits<std::uint64_t>::max());
  rehash_wire(hostile);
  check_decision(
      hostile,
      limits,
      ExactPairSupportStreamDecodeDecision::exact_text_limit_exceeded,
      "UINT64_MAX exact lengths are rejected before BigInt parsing");

  hostile = valid;
  const std::vector<std::uint8_t> noncanonical_text{
      static_cast<std::uint8_t>('-'),
      static_cast<std::uint8_t>('2'),
      static_cast<std::uint8_t>('/'),
      static_cast<std::uint8_t>('3')};
  const std::size_t text_offset = find_bytes(hostile, noncanonical_text);
  hostile[text_offset] = static_cast<std::uint8_t>('+');
  rehash_wire(hostile);
  check_decision(
      hostile,
      limits,
      ExactPairSupportStreamDecodeDecision::noncanonical_exact_text,
      "a checksummed noncanonical rational is rejected");

  hostile = valid;
  std::vector<std::uint8_t> frontier_wire;
  const ExactPairSupportFrontierEntry& frontier =
      chunk.next_checkpoint.frontier.front();
  append_u64(frontier_wire, frontier.first_node_index);
  append_u64(frontier_wire, frontier.second_node_index);
  append_u64(frontier_wire, frontier.first_leaf_begin);
  append_u64(frontier_wire, frontier.first_leaf_end);
  append_u64(frontier_wire, frontier.second_leaf_begin);
  append_u64(frontier_wire, frontier.second_leaf_end);
  frontier_wire.push_back(1U);
  const std::size_t frontier_offset = find_bytes(hostile, frontier_wire);
  hostile[frontier_offset + frontier_wire.size() - 1U] = 2U;
  rehash_wire(hostile);
  check_decision(
      hostile,
      limits,
      ExactPairSupportStreamDecodeDecision::invalid_boolean,
      "a checksummed non-boolean byte is rejected");

  ExactPairSupportStreamCodecLimits constrained = limits;
  constrained.maximum_encoded_byte_count = valid.size() - 1U;
  check_decision(
      valid,
      constrained,
      ExactPairSupportStreamDecodeDecision::encoded_byte_limit_exceeded,
      "the global encoded-byte cap is checked before the envelope");

  constrained = limits;
  constrained.maximum_record_count = 1U;
  check_decision(
      valid,
      constrained,
      ExactPairSupportStreamDecodeDecision::count_limit_exceeded,
      "the aggregate record cap is checked before vector reserve");

  constrained = limits;
  constrained.maximum_point_id_reference_count = 7U;
  check_decision(
      valid,
      constrained,
      ExactPairSupportStreamDecodeDecision::
          point_id_reference_limit_exceeded,
      "the aggregate PointId-reference cap is enforced");

  constrained = limits;
  constrained.maximum_exact_text_byte_count = 2U;
  check_decision(
      valid,
      constrained,
      ExactPairSupportStreamDecodeDecision::exact_text_limit_exceeded,
      "the per-value exact-text cap precedes BigInt parsing");

  constrained = limits;
  constrained.maximum_total_exact_text_byte_count = 3U;
  check_decision(
      valid,
      constrained,
      ExactPairSupportStreamDecodeDecision::exact_text_limit_exceeded,
      "the aggregate exact-text cap is enforced incrementally");

  constrained = limits;
  constrained.maximum_frontier_entry_count = 1U;
  check_decision(
      valid,
      constrained,
      ExactPairSupportStreamDecodeDecision::frontier_entry_limit_exceeded,
      "the checkpoint frontier cap precedes reserve");

  constrained = limits;
  constrained.maximum_auxiliary_entry_count = 3U;
  check_decision(
      valid,
      constrained,
      ExactPairSupportStreamDecodeDecision::auxiliary_entry_limit_exceeded,
      "all pending-product auxiliary entries share one cap");
}

void test_encode_rejects_invalid_or_unbounded_inputs() {
  const ExactPairSupportStreamChunk chunk = fixture_chunk();
  const ExactPairSupportStreamCodecLimits limits = fixture_limits();
  const std::vector<std::uint8_t> expected_wire =
      encode_exact_pair_support_stream_chunk(chunk, limits);

  ExactPairSupportStreamCodecLimits constrained = limits;
  constrained.maximum_encoded_byte_count = expected_wire.size();
  check(
      encode_exact_pair_support_stream_chunk(chunk, constrained) ==
          expected_wire,
      "encoding accepts an exact total wire-byte cap");

  constrained.maximum_encoded_byte_count = expected_wire.size() - 1U;
  check_throws<std::length_error>(
      [&] {
        static_cast<void>(
            encode_exact_pair_support_stream_chunk(chunk, constrained));
      },
      "encoding rejects a cap one byte below the complete wire");

  constrained.maximum_encoded_byte_count =
      wire_header_byte_count + wire_checksum_byte_count - 1U;
  check_throws<std::length_error>(
      [&] {
        static_cast<void>(
            encode_exact_pair_support_stream_chunk(chunk, constrained));
      },
      "encoding rejects a 61-byte cap before reserved-tail arithmetic");

  constrained = limits;
  constrained.maximum_record_count = 1U;
  check_throws<std::length_error>(
      [&] {
        static_cast<void>(
            encode_exact_pair_support_stream_chunk(chunk, constrained));
      },
      "encoding enforces the aggregate record cap");

  constrained = limits;
  constrained.maximum_auxiliary_entry_count = 3U;
  check_throws<std::length_error>(
      [&] {
        static_cast<void>(
            encode_exact_pair_support_stream_chunk(chunk, constrained));
      },
      "encoding enforces the aggregate auxiliary cap");

  constrained = limits;
  constrained.maximum_frontier_entry_count = 1U;
  check_throws<std::length_error>(
      [&] {
        static_cast<void>(
            encode_exact_pair_support_stream_chunk(chunk, constrained));
      },
      "encoding enforces the checkpoint frontier cap");

  constrained = limits;
  constrained.maximum_point_id_reference_count = 7U;
  check_throws<std::length_error>(
      [&] {
        static_cast<void>(
            encode_exact_pair_support_stream_chunk(chunk, constrained));
      },
      "encoding enforces the aggregate PointId-reference cap");

  constrained = limits;
  constrained.maximum_exact_text_byte_count = 2U;
  check_throws<std::length_error>(
      [&] {
        static_cast<void>(
            encode_exact_pair_support_stream_chunk(chunk, constrained));
      },
      "encoding enforces the per-value exact-text cap");

  constrained = limits;
  constrained.maximum_total_exact_text_byte_count = 3U;
  check_throws<std::length_error>(
      [&] {
        static_cast<void>(
            encode_exact_pair_support_stream_chunk(chunk, constrained));
      },
      "encoding enforces the aggregate exact-text cap");

  ExactPairSupportStreamChunk invalid = chunk;
  invalid.next_checkpoint.frontier.front().self_product = 2U;
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(
            encode_exact_pair_support_stream_chunk(invalid, limits));
      },
      "encoding rejects an in-memory non-boolean frontier tag");

  invalid = chunk;
  invalid.status = static_cast<ExactPairSupportStreamStatus>(0xffU);
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(
            encode_exact_pair_support_stream_chunk(invalid, limits));
      },
      "encoding rejects an in-memory unknown enum");

  ExactPairSupportStreamCodecLimits unbounded = limits;
  unbounded.maximum_record_count =
      std::numeric_limits<std::size_t>::max();
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(
            encode_exact_pair_support_stream_chunk(chunk, unbounded));
      },
      "SIZE_MAX is rejected as an unbounded encoder configuration");
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(decode_exact_pair_support_stream_chunk(
            std::span<const std::uint8_t>{}, unbounded));
      },
      "SIZE_MAX is rejected as an unbounded decoder configuration");
}

}  // namespace

int main() {
  test_roundtrip_and_determinism();
  test_hostile_envelope_and_checksum();
  test_hostile_payload_and_limits();
  test_encode_rejects_invalid_or_unbounded_inputs();
  if (failures != 0) {
    std::cerr << failures << " pair-support codec test(s) failed\n";
    return 1;
  }
  std::cout << "pair-support bounded canonical codec tests passed\n";
  return 0;
}
