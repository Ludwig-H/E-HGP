#include "morsehgp3d/hierarchy/pair_support_stream_codec.hpp"

#include "morsehgp3d/contract/canonical_id.hpp"
#include "morsehgp3d/exact/center.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
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
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace {

using morsehgp3d::contract::CanonicalId;
using morsehgp3d::contract::CanonicalSha256Builder;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::CircumcenterKind;
using morsehgp3d::exact::ExactCenter3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::exact::circumcenter;
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
using morsehgp3d::hierarchy::
    pair_support_stream_default_maximum_exact_text_byte_count;

#if defined(__unix__) || defined(__APPLE__)
using morsehgp3d::hierarchy::ExactPairSupportStreamFdWireReceipt;
using morsehgp3d::hierarchy::decode_exact_pair_support_stream_chunk_from_fd;
using morsehgp3d::hierarchy::encode_exact_pair_support_stream_chunk_to_fd;
using morsehgp3d::hierarchy::pair_support_stream_fd_buffer_byte_count;
using morsehgp3d::hierarchy::verify_exact_pair_support_stream_chunk_fd_wire;
#endif

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

#if defined(__unix__) || defined(__APPLE__)

class UniqueFd {
 public:
  explicit UniqueFd(int descriptor = -1) noexcept
      : descriptor_(descriptor) {}

  ~UniqueFd() {
    if (descriptor_ >= 0) {
      static_cast<void>(::close(descriptor_));
    }
  }

  UniqueFd(const UniqueFd&) = delete;
  UniqueFd& operator=(const UniqueFd&) = delete;

  UniqueFd(UniqueFd&& other) noexcept
      : descriptor_(std::exchange(other.descriptor_, -1)) {}

  [[nodiscard]] int get() const noexcept { return descriptor_; }

 private:
  int descriptor_{};
};

[[nodiscard]] UniqueFd temporary_fd() {
  std::array<char, 38U> path{
      '/', 't', 'm', 'p', '/', 'm', 'o', 'r', 's', 'e', 'h', 'g', 'p',
      '3', 'd', '-', 'c', 'o', 'd', 'e', 'c', '-', 'X', 'X', 'X', 'X',
      'X', 'X', '\0'};
  const int descriptor = ::mkstemp(path.data());
  if (descriptor < 0) {
    throw std::runtime_error("cannot create a codec test file");
  }
  if (::unlink(path.data()) != 0) {
    const int saved_errno = errno;
    static_cast<void>(::close(descriptor));
    throw std::runtime_error(
        "cannot unlink a codec test file: " +
        std::to_string(saved_errno));
  }
  return UniqueFd{descriptor};
}

void positional_write(
    int descriptor,
    std::span<const std::uint8_t> bytes,
    std::size_t offset = 0U) {
  std::size_t written = 0U;
  while (written < bytes.size()) {
    const ssize_t count = ::pwrite(
        descriptor,
        bytes.data() + written,
        bytes.size() - written,
        static_cast<off_t>(offset + written));
    if (count < 0 && errno == EINTR) {
      continue;
    }
    if (count <= 0) {
      throw std::runtime_error("cannot write a codec test file");
    }
    written += static_cast<std::size_t>(count);
  }
}

[[nodiscard]] std::vector<std::uint8_t> positional_read(
    int descriptor,
    std::size_t byte_count) {
  std::vector<std::uint8_t> bytes(byte_count);
  std::size_t read_count = 0U;
  while (read_count < bytes.size()) {
    const ssize_t count = ::pread(
        descriptor,
        bytes.data() + read_count,
        bytes.size() - read_count,
        static_cast<off_t>(read_count));
    if (count < 0 && errno == EINTR) {
      continue;
    }
    if (count <= 0) {
      throw std::runtime_error("cannot read a codec test file");
    }
    read_count += static_cast<std::size_t>(count);
  }
  return bytes;
}

#endif

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

void test_exact_text_default_and_bit_length_preflight() {
  check(
      ExactPairSupportStreamCodecLimits{}
              .maximum_exact_text_byte_count ==
          pair_support_stream_default_maximum_exact_text_byte_count &&
          pair_support_stream_default_maximum_exact_text_byte_count == 2048U,
      "the pair-path exact-text default is the public 2048-byte bound");

  ExactPairSupportStreamChunk chunk = fixture_chunk();
  chunk.events.front().center = center(
      ExactRational{BigInt{1} << 512U},
      ExactRational{BigInt{1}},
      ExactRational{BigInt{1}});
  ExactPairSupportStreamCodecLimits limits = fixture_limits();
  limits.maximum_exact_text_byte_count = 32U;
  check_throws<std::length_error>(
      [&] {
        static_cast<void>(
            encode_exact_pair_support_stream_chunk(chunk, limits));
      },
      "bit length rejects an oversized exact key before decimal materialization");

  chunk = fixture_chunk();
  chunk.events.front().center = center(
      ExactRational{BigInt{123}},
      ExactRational{BigInt{1}},
      ExactRational{BigInt{1}});
  limits.maximum_exact_text_byte_count = 4U;
  check_throws<std::length_error>(
      [&] {
        static_cast<void>(
            encode_exact_pair_support_stream_chunk(chunk, limits));
      },
      "the at-most-two-byte preflight tolerance never weakens the exact cap");

  const double smallest = std::numeric_limits<double>::denorm_min();
  const double largest = std::numeric_limits<double>::max();
  const auto extreme_pair = circumcenter(
      CertifiedPoint3::from_binary64(smallest, smallest, smallest),
      CertifiedPoint3::from_binary64(largest, largest, largest));
  check(
      extreme_pair.kind() == CircumcenterKind::unique &&
          extreme_pair.center().has_value() &&
          extreme_pair.squared_level().has_value(),
      "finite binary64 extremes define one exact pair center and level");
  if (extreme_pair.center().has_value() &&
      extreme_pair.squared_level().has_value()) {
    std::size_t maximum_center_text_byte_count = 0U;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      maximum_center_text_byte_count = std::max(
          maximum_center_text_byte_count,
          extreme_pair.center()->coordinate(axis).canonical_key().size());
    }
    const std::size_t level_text_byte_count =
        extreme_pair.squared_level()->canonical_key().size();
    check(
        maximum_center_text_byte_count <= 958U &&
            level_text_byte_count <= 1914U &&
            level_text_byte_count <
                pair_support_stream_default_maximum_exact_text_byte_count,
        "binary64 pair extremes respect the proved 958/1914-byte text bounds");

    chunk = fixture_chunk();
    chunk.events.front().center = *extreme_pair.center();
    chunk.events.front().squared_level = *extreme_pair.squared_level();
    check(
        !encode_exact_pair_support_stream_chunk(
             chunk, ExactPairSupportStreamCodecLimits{})
             .empty(),
        "the default 2048-byte exact cap accepts a binary64 extreme pair");
  }
}

#if defined(__unix__) || defined(__APPLE__)

void test_fd_roundtrip_receipt_and_position() {
  const ExactPairSupportStreamChunk chunk = fixture_chunk();
  const ExactPairSupportStreamCodecLimits limits = fixture_limits();
  const std::vector<std::uint8_t> vector_wire =
      encode_exact_pair_support_stream_chunk(chunk, limits);
  UniqueFd file = temporary_fd();
  check(
      ::lseek(file.get(), 37, SEEK_SET) == 37,
      "the fd fixture position is initialized away from zero");
  const ExactPairSupportStreamFdWireReceipt receipt =
      encode_exact_pair_support_stream_chunk_to_fd(
          file.get(), chunk, limits);
  check(
      ::lseek(file.get(), 0, SEEK_CUR) == 37,
      "fd encoding leaves the borrowed descriptor position unchanged");
  check(
      receipt.encoded_byte_count == vector_wire.size() &&
          positional_read(file.get(), vector_wire.size()) == vector_wire,
      "fd and vector encoders produce the identical v1 golden wire");
  std::array<std::uint8_t, CanonicalId::byte_count> expected_checksum{};
  std::copy_n(
      vector_wire.end() -
          static_cast<std::ptrdiff_t>(CanonicalId::byte_count),
      CanonicalId::byte_count,
      expected_checksum.begin());
  check(
      receipt.checksum == CanonicalId{expected_checksum},
      "the fd receipt carries the exact trailing wire checksum");

  const auto verified = verify_exact_pair_support_stream_chunk_fd_wire(
      file.get(), limits, receipt);
  check(
      verified.accepted() && verified.receipt == receipt &&
          ::lseek(file.get(), 0, SEEK_CUR) == 37,
      "fd verification authenticates the receipt without moving position");
  const ExactPairSupportStreamDecodeResult decoded =
      decode_exact_pair_support_stream_chunk_from_fd(
          file.get(), limits, receipt);
  check(
      decoded.accepted() && decoded.chunk == chunk &&
          ::lseek(file.get(), 0, SEEK_CUR) == 37,
      "fd decode matches span decode and preserves descriptor position");

#if defined(__linux__)
  const std::string proc_descriptor_path =
      "/proc/self/fd/" + std::to_string(file.get());
  UniqueFd read_only{
      ::open(proc_descriptor_path.c_str(), O_RDONLY | O_CLOEXEC)};
  check(
      read_only.get() >= 0 &&
          verify_exact_pair_support_stream_chunk_fd_wire(
              read_only.get(), limits, receipt)
              .accepted() &&
          decode_exact_pair_support_stream_chunk_from_fd(
              read_only.get(), limits, receipt)
              .accepted(),
      "fd verification and decoding accept a borrowed O_RDONLY descriptor");
#endif

  ExactPairSupportStreamFdWireReceipt wrong_receipt = receipt;
  ++wrong_receipt.encoded_byte_count;
  const auto mismatch = verify_exact_pair_support_stream_chunk_fd_wire(
      file.get(), limits, wrong_receipt);
  check(
      mismatch.decision ==
              ExactPairSupportStreamDecodeDecision::receipt_mismatch &&
          !mismatch.receipt.has_value(),
      "a different otherwise well-formed receipt is rejected");

  ExactPairSupportStreamCodecLimits exact_cap = limits;
  exact_cap.maximum_encoded_byte_count = vector_wire.size();
  check(
      verify_exact_pair_support_stream_chunk_fd_wire(
          file.get(), exact_cap, receipt)
          .accepted(),
      "fd verification accepts the exact total byte cap");
  exact_cap.maximum_encoded_byte_count = vector_wire.size() - 1U;
  check(
      verify_exact_pair_support_stream_chunk_fd_wire(
          file.get(), exact_cap)
              .decision == ExactPairSupportStreamDecodeDecision::
                               encoded_byte_limit_exceeded,
      "fd verification rejects a cap one byte below the wire");

  UniqueFd exact_output = temporary_fd();
  exact_cap.maximum_encoded_byte_count = vector_wire.size();
  check(
      encode_exact_pair_support_stream_chunk_to_fd(
          exact_output.get(), chunk, exact_cap) == receipt,
      "fd encoding accepts an exact total byte cap");
  UniqueFd short_output = temporary_fd();
  exact_cap.maximum_encoded_byte_count = vector_wire.size() - 1U;
  check_throws<std::length_error>(
      [&] {
        static_cast<void>(encode_exact_pair_support_stream_chunk_to_fd(
            short_output.get(), chunk, exact_cap));
      },
      "fd encoding rejects a cap one byte below the wire before writing");
  struct stat short_metadata {};
  check(
      ::fstat(short_output.get(), &short_metadata) == 0 &&
          short_metadata.st_size == 0,
      "a counting-pass cap rejection leaves the empty output untouched");
}

void test_fd_buffer_boundary() {
  ExactPairSupportStreamChunk chunk = fixture_chunk();
  const std::size_t interior_count =
      pair_support_stream_fd_buffer_byte_count / sizeof(std::uint64_t) +
      257U;
  chunk.events.front().interior_ids.clear();
  chunk.events.front().interior_ids.reserve(interior_count);
  for (std::size_t index = 0U; index < interior_count; ++index) {
    chunk.events.front().interior_ids.push_back(
        static_cast<std::uint64_t>(index + 1000U));
  }
  ExactPairSupportStreamCodecLimits limits = fixture_limits();
  limits.maximum_point_id_reference_count = interior_count + 16U;
  const std::vector<std::uint8_t> vector_wire =
      encode_exact_pair_support_stream_chunk(chunk, limits);
  check(
      vector_wire.size() > pair_support_stream_fd_buffer_byte_count,
      "the large fd fixture crosses the public 64 KiB buffer boundary");
  UniqueFd file = temporary_fd();
  const ExactPairSupportStreamFdWireReceipt receipt =
      encode_exact_pair_support_stream_chunk_to_fd(
          file.get(), chunk, limits);
  const ExactPairSupportStreamDecodeResult decoded =
      decode_exact_pair_support_stream_chunk_from_fd(
          file.get(), limits, receipt);
  check(
      positional_read(file.get(), vector_wire.size()) == vector_wire &&
          decoded.accepted() && decoded.chunk == chunk,
      "buffered fd encode/decode is exact across multiple 64 KiB windows");
}

void test_hostile_fds_and_file_images() {
  const ExactPairSupportStreamChunk chunk = fixture_chunk();
  const ExactPairSupportStreamCodecLimits limits = fixture_limits();

  UniqueFd corrupted = temporary_fd();
  const ExactPairSupportStreamFdWireReceipt corrupted_receipt =
      encode_exact_pair_support_stream_chunk_to_fd(
          corrupted.get(), chunk, limits);
  std::uint8_t changed{};
  check(
      ::pread(
          corrupted.get(),
          &changed,
          1U,
          static_cast<off_t>(wire_header_byte_count + 17U)) == 1,
      "the checksum fixture byte can be read");
  changed ^= 1U;
  positional_write(
      corrupted.get(),
      std::span<const std::uint8_t>{&changed, 1U},
      wire_header_byte_count + 17U);
  check(
      verify_exact_pair_support_stream_chunk_fd_wire(
          corrupted.get(), limits, corrupted_receipt)
              .decision ==
          ExactPairSupportStreamDecodeDecision::checksum_mismatch,
      "fd verification rejects a corrupted payload checksum");
  check(
      decode_exact_pair_support_stream_chunk_from_fd(
          corrupted.get(), limits)
              .decision ==
          ExactPairSupportStreamDecodeDecision::checksum_mismatch,
      "fd decode performs checksum authentication before semantic parsing");

  UniqueFd truncated = temporary_fd();
  const ExactPairSupportStreamFdWireReceipt truncated_receipt =
      encode_exact_pair_support_stream_chunk_to_fd(
          truncated.get(), chunk, limits);
  check(
      ::ftruncate(
          truncated.get(),
          static_cast<off_t>(truncated_receipt.encoded_byte_count - 1U)) ==
          0,
      "the truncation fixture is shortened by one byte");
  check(
      verify_exact_pair_support_stream_chunk_fd_wire(
          truncated.get(), limits)
              .decision == ExactPairSupportStreamDecodeDecision::
                               payload_length_mismatch,
      "fd verification rejects a truncated declared payload");

  int pipe_descriptors[2]{-1, -1};
  check(::pipe(pipe_descriptors) == 0, "the hostile pipe fixture is created");
  if (pipe_descriptors[0] >= 0 && pipe_descriptors[1] >= 0) {
    UniqueFd pipe_read{pipe_descriptors[0]};
    UniqueFd pipe_write{pipe_descriptors[1]};
    check(
        verify_exact_pair_support_stream_chunk_fd_wire(
            pipe_read.get(), limits)
                .decision == ExactPairSupportStreamDecodeDecision::
                                 invalid_file_descriptor,
        "a non-seekable non-regular source fd is rejected");
    check_throws<std::invalid_argument>(
        [&] {
          static_cast<void>(encode_exact_pair_support_stream_chunk_to_fd(
              pipe_write.get(), chunk, limits));
        },
        "a non-regular output fd is rejected");
  }

  UniqueFd append_output = temporary_fd();
  const int current_flags = ::fcntl(append_output.get(), F_GETFL);
  check(
      current_flags >= 0 &&
          ::fcntl(
              append_output.get(), F_SETFL, current_flags | O_APPEND) == 0,
      "the O_APPEND hostile fixture is configured");
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(encode_exact_pair_support_stream_chunk_to_fd(
            append_output.get(), chunk, limits));
      },
      "O_APPEND output is rejected before positional writes");

#ifdef O_DIRECT
  UniqueFd direct_output = temporary_fd();
  const int direct_flags = ::fcntl(direct_output.get(), F_GETFL);
  if (direct_flags >= 0 &&
      ::fcntl(
          direct_output.get(), F_SETFL, direct_flags | O_DIRECT) == 0) {
    check_throws<std::invalid_argument>(
        [&] {
          static_cast<void>(encode_exact_pair_support_stream_chunk_to_fd(
              direct_output.get(), chunk, limits));
        },
        "O_DIRECT output is rejected before unaligned buffered I/O");
  }
#endif

  UniqueFd nonempty_output = temporary_fd();
  const std::uint8_t sentinel = 0x5aU;
  positional_write(
      nonempty_output.get(),
      std::span<const std::uint8_t>{&sentinel, 1U});
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(encode_exact_pair_support_stream_chunk_to_fd(
            nonempty_output.get(), chunk, limits));
      },
      "a nonempty output fd is rejected without overwriting its sentinel");
  check(
      positional_read(nonempty_output.get(), 1U).front() == sentinel,
      "the rejected nonempty fd keeps its original byte");
}

#endif

}  // namespace

int main() {
  test_roundtrip_and_determinism();
  test_hostile_envelope_and_checksum();
  test_hostile_payload_and_limits();
  test_encode_rejects_invalid_or_unbounded_inputs();
  test_exact_text_default_and_bit_length_preflight();
#if defined(__unix__) || defined(__APPLE__)
  test_fd_roundtrip_receipt_and_position();
  test_fd_buffer_boundary();
  test_hostile_fds_and_file_images();
#endif
  if (failures != 0) {
    std::cerr << failures << " pair-support codec test(s) failed\n";
    return 1;
  }
  std::cout << "pair-support bounded canonical codec tests passed\n";
  return 0;
}
