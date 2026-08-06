#pragma once

#include "morsehgp3d/hierarchy/direct_morse_forest_segment_sink.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

// Tranche 15L-a1: canonical big-endian byte codec for the 15I committed
// batch segments and the terminal final seal, toward the durable
// morsehgp3d-run.v1 header--footer archive (design:
// docs/research/DURABLE_RUN_ARCHIVE_15L_DESIGN.md).  The decoded payload is
// never a scientific authority: it feeds the future massive recertifier and
// the operator== differential against resident segments only.

inline constexpr std::uint32_t
    direct_morse_forest_durable_run_archive_schema_version = 1U;
inline constexpr std::string_view
    direct_morse_forest_durable_run_archive_backend = "reference_cpu";
inline constexpr std::string_view
    direct_morse_forest_durable_run_archive_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_morse_forest_durable_run_archive_mode =
        "canonical_bounded_forest_output_segment_codec_v1";
inline constexpr std::string_view
    direct_morse_forest_durable_run_archive_deployment_status =
        "architecture_only_codec_only_no_file_archive_yet_no_recertifier_"
        "no_massive_qualification";
inline constexpr std::string_view
    direct_morse_forest_durable_run_archive_public_status = "not_claimed";

// Bounded decode caps.  Record-count caps reuse the 15I physical limits;
// the byte and exact-text caps follow the 15K conventions.
struct ExactDirectMorseForestSegmentCodecLimits {
  std::size_t maximum_encoded_byte_count{64U * 1024U * 1024U};
  std::size_t maximum_exact_text_byte_count{1U * 1024U * 1024U};
  std::size_t maximum_total_exact_text_byte_count{16U * 1024U * 1024U};
  ExactDirectMorseForestSegmentLimits segment_limits{};
  std::size_t maximum_final_root_count{};

  friend bool operator==(
      const ExactDirectMorseForestSegmentCodecLimits&,
      const ExactDirectMorseForestSegmentCodecLimits&) = default;
};

enum class ExactDirectMorseForestSegmentCodecDecision : std::uint8_t {
  not_certified,
  no_schema_rejected,
  no_shape_rejected,
  no_encoded_byte_limit_exceeded,
  no_exact_text_limit_exceeded,
  no_total_exact_text_limit_exceeded,
  no_record_limit_exceeded,
  no_payload_digest_mismatch,
  no_truncation_or_suffix_rejected,
  no_capacity_overflow,
  no_allocation_failed,
  complete_canonical_segment_codec,
};

struct ExactDirectMorseForestEncodedSegment {
  std::vector<std::uint8_t> bytes;
  std::size_t encoded_byte_count{};
  contract::CanonicalId encoded_payload_digest{};
  ExactDirectMorseForestSegmentCodecDecision decision{
      ExactDirectMorseForestSegmentCodecDecision::not_certified};

  [[nodiscard]] bool certified_encoded() const noexcept;
};

struct ExactDirectMorseForestDecodedSegment {
  ExactDirectMorseForestBatchSegment segment{};
  std::size_t consumed_byte_count{};
  ExactDirectMorseForestSegmentCodecDecision decision{
      ExactDirectMorseForestSegmentCodecDecision::not_certified};

  [[nodiscard]] bool certified_decoded() const noexcept;
};

struct ExactDirectMorseForestEncodedFinalSeal {
  std::vector<std::uint8_t> bytes;
  std::size_t encoded_byte_count{};
  contract::CanonicalId encoded_payload_digest{};
  ExactDirectMorseForestSegmentCodecDecision decision{
      ExactDirectMorseForestSegmentCodecDecision::not_certified};

  [[nodiscard]] bool certified_encoded() const noexcept;
};

struct ExactDirectMorseForestDecodedFinalSeal {
  ExactDirectMorseForestFinalSeal seal{};
  std::size_t consumed_byte_count{};
  ExactDirectMorseForestSegmentCodecDecision decision{
      ExactDirectMorseForestSegmentCodecDecision::not_certified};

  [[nodiscard]] bool certified_decoded() const noexcept;
};

// Encoding is total on structurally certified inputs under the caps; both
// directions are deterministic and the decode of an encode is field-exact
// (operator==).  The byte stream carries a fixed 64-byte envelope (magic,
// codec schema, segment schema, payload length, payload SHA-256) followed by
// the canonical payload; trailing bytes beyond the declared length reject.
[[nodiscard]] ExactDirectMorseForestEncodedSegment
encode_exact_direct_morse_forest_batch_segment(
    const ExactDirectMorseForestBatchSegment& segment,
    const ExactDirectMorseForestSegmentCodecLimits& limits) noexcept;

[[nodiscard]] ExactDirectMorseForestDecodedSegment
decode_exact_direct_morse_forest_batch_segment(
    std::span<const std::uint8_t> bytes,
    const ExactDirectMorseForestSegmentCodecLimits& limits) noexcept;

[[nodiscard]] ExactDirectMorseForestEncodedFinalSeal
encode_exact_direct_morse_forest_final_seal(
    const ExactDirectMorseForestFinalSeal& seal,
    const ExactDirectMorseForestSegmentCodecLimits& limits) noexcept;

[[nodiscard]] ExactDirectMorseForestDecodedFinalSeal
decode_exact_direct_morse_forest_final_seal(
    std::span<const std::uint8_t> bytes,
    const ExactDirectMorseForestSegmentCodecLimits& limits) noexcept;

}  // namespace morsehgp3d::hierarchy
