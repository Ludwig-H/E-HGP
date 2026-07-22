#pragma once

#include "morsehgp3d/hierarchy/pair_support_stream.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t pair_support_stream_chunk_codec_version = 1U;
inline constexpr std::size_t pair_support_stream_fd_buffer_byte_count =
    64U * 1024U;
// A pair-supported 3D event uses rational centers and squared levels derived
// from binary64 input.  This cap is deliberately generous for that path while
// preventing attacker-sized decimal conversions before an exact parse.
inline constexpr std::size_t
    pair_support_stream_default_maximum_exact_text_byte_count = 2048U;

// All limits are caller-owned and deliberately finite.  The defaults bound one
// independently replayable chunk; they are not product-SLO claims.  A value of
// std::numeric_limits<std::size_t>::max() is rejected as an unbounded sentinel.
struct ExactPairSupportStreamCodecLimits {
  std::size_t maximum_encoded_byte_count{64U * 1024U * 1024U};
  std::size_t maximum_frontier_entry_count{1'000'000U};
  // Sum of witness-frontier entries, strict receipts and a deferred node.
  std::size_t maximum_auxiliary_entry_count{1'000'000U};
  // Sum of events and relevant extra-shell diagnostics.  record_order is
  // independently required not to exceed this same cap.
  std::size_t maximum_record_count{1'000'000U};
  // Support ids, interior ids and canonical extra-shell witnesses.
  std::size_t maximum_point_id_reference_count{16'000'000U};
  std::size_t maximum_exact_text_byte_count{
      pair_support_stream_default_maximum_exact_text_byte_count};
  std::size_t maximum_total_exact_text_byte_count{16U * 1024U * 1024U};

  friend bool operator==(
      const ExactPairSupportStreamCodecLimits&,
      const ExactPairSupportStreamCodecLimits&) = default;
};

enum class ExactPairSupportStreamDecodeDecision : std::uint8_t {
  accepted,
  encoded_byte_limit_exceeded,
  truncated,
  invalid_magic,
  unsupported_version,
  unsupported_kind,
  unsupported_flags,
  payload_length_mismatch,
  checksum_mismatch,
  numeric_overflow,
  count_limit_exceeded,
  frontier_entry_limit_exceeded,
  auxiliary_entry_limit_exceeded,
  point_id_reference_limit_exceeded,
  exact_text_limit_exceeded,
  invalid_boolean,
  invalid_enumeration,
  noncanonical_exact_text,
  trailing_bytes,
  invalid_file_descriptor,
  file_io_error,
  file_changed,
  receipt_mismatch,
};

struct ExactPairSupportStreamDecodeResult {
  ExactPairSupportStreamDecodeDecision decision{
      ExactPairSupportStreamDecodeDecision::truncated};
  std::optional<ExactPairSupportStreamChunk> chunk;

  [[nodiscard]] bool accepted() const noexcept {
    return decision == ExactPairSupportStreamDecodeDecision::accepted &&
           chunk.has_value();
  }
};

#if defined(__unix__) || defined(__APPLE__)

// Receipt for an immutable v1 wire image.  The descriptor itself is borrowed;
// none of the fd entry points changes its current file position.
struct ExactPairSupportStreamFdWireReceipt {
  std::size_t encoded_byte_count{};
  contract::CanonicalId checksum{};

  friend bool operator==(
      const ExactPairSupportStreamFdWireReceipt&,
      const ExactPairSupportStreamFdWireReceipt&) = default;
};

struct ExactPairSupportStreamFdWireVerificationResult {
  ExactPairSupportStreamDecodeDecision decision{
      ExactPairSupportStreamDecodeDecision::invalid_file_descriptor};
  std::optional<ExactPairSupportStreamFdWireReceipt> receipt;

  [[nodiscard]] bool accepted() const noexcept {
    return decision == ExactPairSupportStreamDecodeDecision::accepted &&
           receipt.has_value();
  }
};

#endif

// Throws std::invalid_argument for an unbounded limit configuration or a
// non-canonical in-memory enum/bool, and std::length_error when the supplied
// chunk exceeds one of the caller's limits.  No native structure layout or
// padding is copied into the wire representation.
[[nodiscard]] std::vector<std::uint8_t>
encode_exact_pair_support_stream_chunk(
    const ExactPairSupportStreamChunk& chunk,
    const ExactPairSupportStreamCodecLimits& limits);

// Hostile bytes fail closed through decision and an empty optional.  Expected
// parse failures do not escape as exceptions; allocation failures and invalid
// (effectively unbounded) caller limit configurations may still throw.
[[nodiscard]] ExactPairSupportStreamDecodeResult
decode_exact_pair_support_stream_chunk(
    std::span<const std::uint8_t> encoded,
    const ExactPairSupportStreamCodecLimits& limits);

#if defined(__unix__) || defined(__APPLE__)

// Encodes at offset zero of one borrowed, empty, seekable regular O_RDWR file.
// O_APPEND and O_DIRECT are rejected.  The function uses only positional I/O,
// leaves the descriptor position unchanged, and returns the exact size/digest
// expected on disk.  A failed call may leave an unpublished partial file.
[[nodiscard]] ExactPairSupportStreamFdWireReceipt
encode_exact_pair_support_stream_chunk_to_fd(
    int descriptor,
    const ExactPairSupportStreamChunk& chunk,
    const ExactPairSupportStreamCodecLimits& limits);

// Authenticates the complete regular-file image without allocating it.  When
// supplied, expected_receipt also prevents accepting a different valid image.
[[nodiscard]] ExactPairSupportStreamFdWireVerificationResult
verify_exact_pair_support_stream_chunk_fd_wire(
    int descriptor,
    const ExactPairSupportStreamCodecLimits& limits,
    std::optional<ExactPairSupportStreamFdWireReceipt> expected_receipt =
        std::nullopt);

// Performs a full checksum pass before semantic allocations, parses through a
// fixed-size fd buffer, then authenticates the same receipt a second time.
[[nodiscard]] ExactPairSupportStreamDecodeResult
decode_exact_pair_support_stream_chunk_from_fd(
    int descriptor,
    const ExactPairSupportStreamCodecLimits& limits,
    std::optional<ExactPairSupportStreamFdWireReceipt> expected_receipt =
        std::nullopt);

#endif

}  // namespace morsehgp3d::hierarchy
