#pragma once

#include "morsehgp3d/hierarchy/direct_morse_forest_segment_sink.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
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

// -------------------------------------------------------------------------
// 15L-a2: the morsehgp3d-run.v1 durable file archive.  One file binds the
// manifest digest, the terminal source chain digest and the complete 15I
// segment chain under a header--footer seal; publication is atomic
// (temporary file, fdatasync, no-replace link, directory fsync, following
// the sealed pair_support_stream_durable precedent) and an interrupted run
// leaves either no final file or a complete one -- never a partial
// publication.
// -------------------------------------------------------------------------

struct ExactDirectMorseForestRunArchiveIdentity {
  contract::CanonicalId manifest_digest{};
  contract::CanonicalId source_identity_digest{};
  contract::CanonicalId source_higher_canonical_cloud_digest{};
  contract::CanonicalId terminal_source_chain_digest{};
  std::size_t point_count{};
  std::size_t effective_maximum_order{};

  friend bool operator==(
      const ExactDirectMorseForestRunArchiveIdentity&,
      const ExactDirectMorseForestRunArchiveIdentity&) = default;
};

enum class ExactDirectMorseForestRunArchiveDecision : std::uint8_t {
  not_certified,
  no_identity_rejected,
  no_directory_rejected,
  no_existing_final_rejected,
  no_io_failed,
  no_codec_rejected,
  no_chain_rejected,
  no_segment_order_rejected,
  no_seal_rejected,
  no_archive_digest_rejected,
  no_limit_exceeded,
  no_consumer_rejected,
  no_allocation_failed,
  complete_atomic_run_archive,
};

struct ExactDirectMorseForestRunArchiveWriteResult {
  std::size_t appended_segment_count{};
  std::size_t published_byte_count{};
  contract::CanonicalId archive_digest{};
  bool temporary_removed{false};
  bool final_published{false};
  ExactDirectMorseForestRunArchiveDecision decision{
      ExactDirectMorseForestRunArchiveDecision::not_certified};

  [[nodiscard]] bool certified_published() const noexcept;
};

// Streaming writer.  Segments must arrive in exact 15I chain order: the
// first begin cursor seeds the chain and every subsequent begin cursor must
// equal the previous end cursor, chain digest included.  finalize() seals
// the footer, synchronizes and atomically publishes; abandon() (or
// destruction before finalize) removes the temporary and never publishes.
class ExactDirectMorseForestRunArchiveWriter final {
 public:
  ExactDirectMorseForestRunArchiveWriter() noexcept;
  ~ExactDirectMorseForestRunArchiveWriter();
  ExactDirectMorseForestRunArchiveWriter(
      ExactDirectMorseForestRunArchiveWriter&&) noexcept;
  ExactDirectMorseForestRunArchiveWriter& operator=(
      ExactDirectMorseForestRunArchiveWriter&&) noexcept;
  ExactDirectMorseForestRunArchiveWriter(
      const ExactDirectMorseForestRunArchiveWriter&) = delete;
  ExactDirectMorseForestRunArchiveWriter& operator=(
      const ExactDirectMorseForestRunArchiveWriter&) = delete;

  [[nodiscard]] static ExactDirectMorseForestRunArchiveWriter open_new(
      const std::string& directory_path,
      const std::string& final_file_name,
      const ExactDirectMorseForestRunArchiveIdentity& identity,
      const ExactDirectMorseForestSegmentCodecLimits& limits,
      ExactDirectMorseForestRunArchiveDecision& decision) noexcept;

  [[nodiscard]] bool open() const noexcept;
  [[nodiscard]] ExactDirectMorseForestRunArchiveDecision append_segment(
      const ExactDirectMorseForestBatchSegment& segment) noexcept;
  [[nodiscard]] ExactDirectMorseForestRunArchiveWriteResult finalize(
      const ExactDirectMorseForestFinalSeal& seal) noexcept;
  void abandon() noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

struct ExactDirectMorseForestRunArchiveLoadResult {
  ExactDirectMorseForestRunArchiveIdentity identity{};
  ExactDirectMorseForestFinalSeal final_seal{};
  std::size_t delivered_segment_count{};
  std::size_t validated_byte_count{};
  contract::CanonicalId archive_digest{};
  bool validated_prefix_only{false};
  ExactDirectMorseForestRunArchiveDecision decision{
      ExactDirectMorseForestRunArchiveDecision::not_certified};

  [[nodiscard]] bool certified_loaded() const noexcept;
};

// Synchronous non-retaining consumer, one decoded segment resident at a
// time.  A false return stops the load fail-closed as no_consumer_rejected.
struct ExactDirectMorseForestRunArchiveConsumerView {
  using ConsumeFunction = bool (*)(
      void*, const ExactDirectMorseForestBatchSegment&) noexcept;

  void* state{};
  ConsumeFunction consume{};
};

// Bounded loader.  Validates the header against the expected identity
// (anti-rollback: the terminal source chain digest must match), streams the
// frames one at a time under the codec limits, revalidates each sealed
// digest and the cursor chain, then the footer, the final seal and the
// whole-archive digest.  Any divergence fails closed before or at the
// faulty frame; already-delivered segments are reported as a validated
// prefix, never as a complete run.
[[nodiscard]] ExactDirectMorseForestRunArchiveLoadResult
load_exact_direct_morse_forest_run_archive(
    const std::string& directory_path,
    const std::string& final_file_name,
    const ExactDirectMorseForestRunArchiveIdentity& expected_identity,
    const ExactDirectMorseForestSegmentCodecLimits& limits,
    const ExactDirectMorseForestRunArchiveConsumerView& consumer) noexcept;

enum class ExactDirectMorseForestRunArchiveRecoveryDecision : std::uint8_t {
  not_certified,
  no_directory_rejected,
  no_io_failed,
  absent,
  torn_temporary_discarded,
  valid_final_present,
  invalid_final_present,
};

struct ExactDirectMorseForestRunArchiveRecoveryResult {
  std::size_t discarded_temporary_count{};
  bool final_present{false};
  ExactDirectMorseForestRunArchiveRecoveryDecision decision{
      ExactDirectMorseForestRunArchiveRecoveryDecision::not_certified};

  [[nodiscard]] bool certified_recovered() const noexcept;
};

// Interruption recovery.  Discards every matching temporary fail-closed (a
// temporary is never promoted), then classifies the final file by a full
// bounded validation pass without a consumer.
[[nodiscard]] ExactDirectMorseForestRunArchiveRecoveryResult
recover_exact_direct_morse_forest_run_archive_directory(
    const std::string& directory_path,
    const std::string& final_file_name,
    const ExactDirectMorseForestRunArchiveIdentity& expected_identity,
    const ExactDirectMorseForestSegmentCodecLimits& limits) noexcept;

}  // namespace morsehgp3d::hierarchy
