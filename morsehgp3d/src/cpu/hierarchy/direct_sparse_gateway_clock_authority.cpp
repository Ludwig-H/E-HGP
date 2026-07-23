#include "morsehgp3d/hierarchy/direct_sparse_gateway_clock_authority.hpp"

#include "morsehgp3d/contract/canonical_id.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::size_t unassigned_chronology_index =
    std::numeric_limits<std::size_t>::max();

[[nodiscard]] bool checked_add(
    std::size_t left,
    std::size_t right,
    std::size_t& result) noexcept {
  if (left > std::numeric_limits<std::size_t>::max() - right) {
    return false;
  }
  result = left + right;
  return true;
}

[[nodiscard]] bool checked_multiply(
    std::size_t left,
    std::size_t right,
    std::size_t& result) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return false;
  }
  result = left * right;
  return true;
}

[[nodiscard]] bool fits_u64(std::size_t value) noexcept {
  if constexpr (sizeof(std::size_t) > sizeof(std::uint64_t)) {
    return value <= static_cast<std::size_t>(
                        std::numeric_limits<std::uint64_t>::max());
  }
  return true;
}

void append_u32(
    contract::CanonicalSha256Builder& builder,
    std::uint32_t value) {
  std::array<std::uint8_t, 4U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    const std::size_t shift = (bytes.size() - 1U - index) * 8U;
    bytes[index] = static_cast<std::uint8_t>(value >> shift);
  }
  builder.update(bytes);
}

void append_u64(
    contract::CanonicalSha256Builder& builder,
    std::uint64_t value) {
  std::array<std::uint8_t, 8U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    const std::size_t shift = (bytes.size() - 1U - index) * 8U;
    bytes[index] = static_cast<std::uint8_t>(value >> shift);
  }
  builder.update(bytes);
}

[[nodiscard]] bool append_size(
    contract::CanonicalSha256Builder& builder,
    std::size_t value) {
  if (!fits_u64(value)) {
    return false;
  }
  append_u64(builder, static_cast<std::uint64_t>(value));
  return true;
}

void append_id(
    contract::CanonicalSha256Builder& builder,
    const contract::CanonicalId& value) {
  builder.update(value.bytes());
}

[[nodiscard]] bool append_locator_stamp(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparsePositiveFacetLocatorSnapshotStamp& stamp) {
  append_u32(builder, stamp.schema_version);
  append_u64(builder, stamp.external_authority_id);
  if (!append_size(builder, stamp.committed_batch_count) ||
      !append_size(builder, stamp.inserted_key_count) ||
      !append_size(builder, stamp.component_union_count) ||
      !append_size(builder, stamp.binding_count)) {
    return false;
  }
  append_id(builder, stamp.committed_history_digest);
  return true;
}

[[nodiscard]] bool locator_stamp_encoding_is_valid(
    const ExactDirectSparsePositiveFacetLocatorSnapshotStamp& stamp) noexcept {
  return stamp.schema_version ==
             direct_sparse_positive_facet_locator_schema_version &&
         stamp.external_authority_id != 0U &&
         fits_u64(stamp.committed_batch_count) &&
         fits_u64(stamp.inserted_key_count) &&
         fits_u64(stamp.component_union_count) &&
         fits_u64(stamp.binding_count);
}

[[nodiscard]] bool compute_initial_chain_digest(
    std::uint64_t authority_id,
    std::uint64_t session_id,
    const ExactDirectSparsePositiveFacetLocatorSnapshotStamp&
        opening_locator_stamp,
    std::size_t source_batch_count,
    const contract::CanonicalId& source_scientific_identity_digest,
    contract::CanonicalId& digest) {
  if (!fits_u64(source_batch_count)) {
    return false;
  }
  contract::CanonicalSha256Builder builder;
  builder.update(direct_sparse_gateway_clock_authority_initial_digest_domain);
  append_u32(builder, direct_sparse_gateway_clock_authority_schema_version);
  append_u64(builder, authority_id);
  append_u64(builder, session_id);
  if (!append_locator_stamp(builder, opening_locator_stamp)) {
    return false;
  }
  if (!append_size(builder, source_batch_count)) {
    return false;
  }
  append_id(builder, source_scientific_identity_digest);
  digest = builder.finalize();
  return true;
}

[[nodiscard]] bool compute_capture_chain_digest(
    std::uint64_t authority_id,
    std::uint64_t session_id,
    std::size_t source_batch_count,
    std::size_t chronological_index,
    std::size_t source_batch_index,
    const ExactDirectSparsePositiveFacetLocatorSnapshotStamp& stamp,
    const contract::CanonicalId& previous_digest,
    contract::CanonicalId& digest) {
  if (!fits_u64(source_batch_count) || !fits_u64(chronological_index) ||
      !fits_u64(source_batch_index) ||
      !locator_stamp_encoding_is_valid(stamp)) {
    return false;
  }
  contract::CanonicalSha256Builder builder;
  builder.update(direct_sparse_gateway_clock_authority_capture_digest_domain);
  append_u32(builder, direct_sparse_gateway_clock_authority_schema_version);
  append_u64(builder, authority_id);
  append_u64(builder, session_id);
  if (!append_size(builder, source_batch_count)) {
    return false;
  }
  append_id(builder, previous_digest);
  if (!append_size(builder, chronological_index) ||
      !append_size(builder, source_batch_index) ||
      !append_locator_stamp(builder, stamp)) {
    return false;
  }
  digest = builder.finalize();
  return true;
}

[[nodiscard]] bool compute_seal_digest(
    std::uint64_t authority_id,
    std::uint64_t session_id,
    std::size_t source_batch_count,
    std::size_t capture_record_count,
    const contract::CanonicalId& capture_chain_digest,
    const contract::CanonicalId& source_identity_digest,
    const ExactDirectSparsePositiveFacetLocatorSnapshotStamp& final_stamp,
    const contract::CanonicalId& certificate_digest,
    contract::CanonicalId& digest) {
  if (!fits_u64(source_batch_count) || !fits_u64(capture_record_count) ||
      !locator_stamp_encoding_is_valid(final_stamp)) {
    return false;
  }
  contract::CanonicalSha256Builder builder;
  builder.update(direct_sparse_gateway_clock_authority_seal_digest_domain);
  append_u32(builder, direct_sparse_gateway_clock_authority_schema_version);
  append_u64(builder, authority_id);
  append_u64(builder, session_id);
  if (!append_size(builder, source_batch_count) ||
      !append_size(builder, capture_record_count)) {
    return false;
  }
  append_id(builder, capture_chain_digest);
  append_id(builder, source_identity_digest);
  if (!append_locator_stamp(builder, final_stamp)) {
    return false;
  }
  append_id(builder, certificate_digest);
  digest = builder.finalize();
  return true;
}

}  // namespace

bool ExactDirectSparseGatewayClockAuthorityCaptureResult::certified_capture()
    const noexcept {
  return schema_version ==
             direct_sparse_gateway_clock_authority_schema_version &&
         locator_stamp_at_entry == locator_stamp_at_exit &&
         committed_record.chronological_index ==
             required_chronological_index &&
         committed_record.source_batch_index ==
             requested_source_batch_index &&
         committed_record.locator_snapshot_stamp == locator_stamp_at_entry &&
         authority_initialized &&
         source_batch_in_range_and_previously_absent &&
         capture_capacity_preflight_certified &&
         locator_certified_and_entry_exit_stamp_equal &&
         locator_prefix_chronology_certified &&
         canonical_chain_transition_freshly_computed &&
         chronological_index_dense && source_batch_snapshot_committed &&
         authority_state_mutated &&
         decision ==
             ExactDirectSparseGatewayClockAuthorityCaptureDecision::
                 complete_certified_source_batch_snapshot_capture;
}

bool ExactDirectSparseGatewayClockAuthorityCaptureResult::
    certified_atomic_failure() const noexcept {
  return schema_version ==
             direct_sparse_gateway_clock_authority_schema_version &&
         decision !=
             ExactDirectSparseGatewayClockAuthorityCaptureDecision::
                 not_certified &&
         decision !=
             ExactDirectSparseGatewayClockAuthorityCaptureDecision::
                 complete_certified_source_batch_snapshot_capture &&
         !source_batch_snapshot_committed && !authority_state_mutated;
}

bool ExactDirectSparseGatewayClockAuthorityCaptureResult::certified_outcome()
    const noexcept {
  return certified_capture() || certified_atomic_failure();
}

bool ExactDirectSparseGatewayClockAuthoritySealResult::certified_seal()
    const noexcept {
  return schema_version ==
             direct_sparse_gateway_clock_authority_schema_version &&
         required_source_batch_count <=
             requested_budget.maximum_source_batch_count &&
         required_capture_record_scan_count <=
             requested_budget.maximum_capture_record_scan_count &&
         required_certificate_boundary_count <=
             requested_budget.maximum_certificate_boundary_count &&
         required_temporary_scratch_byte_count <=
             requested_budget.maximum_temporary_scratch_byte_count &&
         capture_record_scan_count == required_capture_record_scan_count &&
         source_identity_result.certified_identity() &&
         source_identity_result.requested_budget ==
             requested_budget.source_identity_budget &&
         certificate_digest_result.certified_digest() &&
         certificate_digest_result.requested_budget ==
             requested_budget.certificate_digest_budget &&
         locator_stamp_at_entry == locator_stamp_at_exit &&
         seal_budget_preflight_certified &&
         every_source_batch_present_exactly_once &&
         source_identity_freshly_computed &&
         boundaries_reindexed_by_source_from_chronological_records &&
         final_locator_extends_capture_chronology &&
         certificate_digest_freshly_computed &&
         locator_certified_and_entry_exit_stamp_equal &&
         exactly_one_certificate_committed && authority_state_mutated &&
         decision ==
             ExactDirectSparseGatewayClockAuthoritySealDecision::
                 complete_certified_single_clock_certificate_seal;
}

bool ExactDirectSparseGatewayClockAuthoritySealResult::
    certified_atomic_failure() const noexcept {
  return schema_version ==
             direct_sparse_gateway_clock_authority_schema_version &&
         decision !=
             ExactDirectSparseGatewayClockAuthoritySealDecision::
                 not_certified &&
         decision !=
             ExactDirectSparseGatewayClockAuthoritySealDecision::
                 complete_certified_single_clock_certificate_seal &&
         !exactly_one_certificate_committed && !authority_state_mutated;
}

bool ExactDirectSparseGatewayClockAuthoritySealResult::certified_outcome()
    const noexcept {
  return certified_seal() || certified_atomic_failure();
}

bool ExactDirectSparseGatewayClockAuthorityJournal::
    certified_initialized_authority() const noexcept {
  return schema_version_ ==
             direct_sparse_gateway_clock_authority_schema_version &&
         authority_id_ != 0U && session_id_ != 0U &&
         expected_locator_authority_id_ != 0U &&
         locator_stamp_encoding_is_valid(opening_locator_stamp_) &&
         opening_locator_stamp_.external_authority_id ==
             expected_locator_authority_id_ &&
         source_batch_count_ <= budget_.maximum_source_batch_count &&
         source_batch_count_ <= budget_.maximum_capture_record_count &&
         source_batch_count_ <=
             budget_.maximum_source_chronology_entry_count &&
         required_initial_arena_byte_count_ <=
             budget_.maximum_initial_arena_byte_count &&
         capture_records_.size() <= source_batch_count_ &&
         capture_records_.capacity() >= source_batch_count_ &&
         source_chronology_indices_.size() == source_batch_count_ &&
         initialization_budget_preflight_certified_ &&
         dense_source_index_initialized_ &&
         opening_locator_stamp_certified_ &&
         initialization_decision_ ==
             ExactDirectSparseGatewayClockAuthorityInitializationDecision::
                 complete_certified_empty_authority_journal;
}

ExactDirectSparseGatewayClockAuthorityJournal::
    ExactDirectSparseGatewayClockAuthorityJournal(
        ExactDirectSparseGatewayClockAuthorityJournal&& other) noexcept {
  *this = std::move(other);
}

ExactDirectSparseGatewayClockAuthorityJournal&
ExactDirectSparseGatewayClockAuthorityJournal::operator=(
    ExactDirectSparseGatewayClockAuthorityJournal&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  schema_version_ = other.schema_version_;
  authority_id_ = other.authority_id_;
  session_id_ = other.session_id_;
  expected_locator_authority_id_ =
      other.expected_locator_authority_id_;
  opening_locator_stamp_ = other.opening_locator_stamp_;
  source_batch_count_ = other.source_batch_count_;
  source_scientific_identity_digest_ =
      other.source_scientific_identity_digest_;
  budget_ = other.budget_;
  required_initial_arena_byte_count_ =
      other.required_initial_arena_byte_count_;
  capture_records_ = std::move(other.capture_records_);
  source_chronology_indices_ =
      std::move(other.source_chronology_indices_);
  initial_chain_digest_ = other.initial_chain_digest_;
  current_capture_chain_digest_ =
      other.current_capture_chain_digest_;
  seal_digest_ = other.seal_digest_;
  sealed_certificate_ = std::move(other.sealed_certificate_);
  committed_certificate_count_ =
      other.committed_certificate_count_;
  initialization_budget_preflight_certified_ =
      other.initialization_budget_preflight_certified_;
  dense_source_index_initialized_ =
      other.dense_source_index_initialized_;
  opening_locator_stamp_certified_ =
      other.opening_locator_stamp_certified_;
  certificate_present_ = other.certificate_present_;
  initialization_decision_ = other.initialization_decision_;
  other.invalidate_after_move();
  return *this;
}

void ExactDirectSparseGatewayClockAuthorityJournal::invalidate_after_move()
    noexcept {
  schema_version_ = 0U;
  authority_id_ = 0U;
  session_id_ = 0U;
  expected_locator_authority_id_ = 0U;
  opening_locator_stamp_ = {};
  source_batch_count_ = 0U;
  source_scientific_identity_digest_ = {};
  budget_ = {};
  required_initial_arena_byte_count_ = 0U;
  capture_records_.clear();
  source_chronology_indices_.clear();
  initial_chain_digest_ = {};
  current_capture_chain_digest_ = {};
  seal_digest_ = {};
  sealed_certificate_ = {};
  committed_certificate_count_ = 0U;
  initialization_budget_preflight_certified_ = false;
  dense_source_index_initialized_ = false;
  opening_locator_stamp_certified_ = false;
  certificate_present_ = false;
  initialization_decision_ =
      ExactDirectSparseGatewayClockAuthorityInitializationDecision::
          not_certified;
}

bool ExactDirectSparseGatewayClockAuthorityJournal::certified_sealed_once()
    const noexcept {
  return certified_initialized_authority() && certificate_present_ &&
         committed_certificate_count_ == 1U &&
         capture_records_.size() == source_batch_count_ &&
         sealed_certificate_.schema_version ==
             direct_sparse_gateway_clock_schema_version &&
         sealed_certificate_.authority_id == authority_id_ &&
         sealed_certificate_.replay_token == session_id_ &&
         sealed_certificate_.source_scientific_identity_digest ==
             source_scientific_identity_digest_ &&
         sealed_certificate_.final_locator_stamp.external_authority_id ==
             expected_locator_authority_id_ &&
         sealed_certificate_.boundaries.size() == source_batch_count_ &&
         sealed_certificate_.digest_present;
}

ExactDirectSparseGatewayClockAuthorityJournal
build_exact_direct_sparse_gateway_clock_authority_journal(
    std::uint64_t authority_id,
    std::uint64_t session_id,
    const ExactDirectSparseGatewayCandidateScientificIdentityResult&
        source_identity,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseGatewayClockAuthorityJournalBudget& budget) {
  ExactDirectSparseGatewayClockAuthorityJournal result;
  result.authority_id_ = authority_id;
  result.session_id_ = session_id;
  const std::size_t source_batch_count =
      source_identity.required_batch_count;
  result.source_batch_count_ = source_batch_count;
  result.source_scientific_identity_digest_ =
      source_identity.scientific_identity_digest;
  result.budget_ = budget;

  if (authority_id == 0U || session_id == 0U) {
    result.initialization_decision_ =
        ExactDirectSparseGatewayClockAuthorityInitializationDecision::
            no_authority_identifier_rejected;
    return result;
  }
  if (!source_identity.certified_identity()) {
    result.initialization_decision_ =
        ExactDirectSparseGatewayClockAuthorityInitializationDecision::
            no_authority_source_identity_rejected;
    return result;
  }
  if (!locator.certified_positive_locator()) {
    result.initialization_decision_ =
        ExactDirectSparseGatewayClockAuthorityInitializationDecision::
            no_authority_locator_rejected;
    return result;
  }
  const auto opening_locator_stamp_at_entry = locator.snapshot_stamp();
  const auto opening_locator_stamp_at_exit = locator.snapshot_stamp();
  if (opening_locator_stamp_at_entry != opening_locator_stamp_at_exit ||
      !locator_stamp_encoding_is_valid(opening_locator_stamp_at_entry)) {
    result.initialization_decision_ =
        ExactDirectSparseGatewayClockAuthorityInitializationDecision::
            no_authority_locator_rejected;
    return result;
  }
  result.opening_locator_stamp_ = opening_locator_stamp_at_entry;
  result.expected_locator_authority_id_ =
      opening_locator_stamp_at_entry.external_authority_id;
  result.opening_locator_stamp_certified_ = true;

  std::size_t capture_bytes = 0U;
  std::size_t source_index_bytes = 0U;
  if (!fits_u64(source_batch_count) ||
      !checked_multiply(
          source_batch_count,
          sizeof(ExactDirectSparseGatewayClockAuthorityCaptureRecord),
          capture_bytes) ||
      !checked_multiply(
          source_batch_count, sizeof(std::size_t), source_index_bytes) ||
      !checked_add(
          capture_bytes,
          source_index_bytes,
          result.required_initial_arena_byte_count_)) {
    result.initialization_decision_ =
        ExactDirectSparseGatewayClockAuthorityInitializationDecision::
            no_authority_capacity_overflow;
    return result;
  }
  if (source_batch_count > budget.maximum_source_batch_count ||
      source_batch_count > budget.maximum_capture_record_count ||
      source_batch_count >
          budget.maximum_source_chronology_entry_count ||
      result.required_initial_arena_byte_count_ >
          budget.maximum_initial_arena_byte_count) {
    result.initialization_decision_ =
        ExactDirectSparseGatewayClockAuthorityInitializationDecision::
            no_authority_budget_exhausted;
    return result;
  }
  result.initialization_budget_preflight_certified_ = true;

  std::vector<ExactDirectSparseGatewayClockAuthorityCaptureRecord>
      capture_records;
  std::vector<std::size_t> source_chronology_indices;
  if (source_batch_count > capture_records.max_size() ||
      source_batch_count > source_chronology_indices.max_size()) {
    result.initialization_decision_ =
        ExactDirectSparseGatewayClockAuthorityInitializationDecision::
            no_authority_capacity_overflow;
    return result;
  }
  try {
    capture_records.reserve(source_batch_count);
    source_chronology_indices.assign(
        source_batch_count, unassigned_chronology_index);
  } catch (const std::length_error&) {
    result.initialization_decision_ =
        ExactDirectSparseGatewayClockAuthorityInitializationDecision::
            no_authority_capacity_overflow;
    return result;
  } catch (const std::bad_alloc&) {
    result.initialization_decision_ =
        ExactDirectSparseGatewayClockAuthorityInitializationDecision::
            no_authority_allocation_failed;
    return result;
  }

  if (!compute_initial_chain_digest(
          authority_id,
          session_id,
          result.opening_locator_stamp_,
          source_batch_count,
          source_identity.scientific_identity_digest,
          result.initial_chain_digest_)) {
    result.initialization_decision_ =
        ExactDirectSparseGatewayClockAuthorityInitializationDecision::
            no_authority_capacity_overflow;
    return result;
  }
  result.current_capture_chain_digest_ = result.initial_chain_digest_;
  result.capture_records_ = std::move(capture_records);
  result.source_chronology_indices_ =
      std::move(source_chronology_indices);
  result.dense_source_index_initialized_ = true;
  result.initialization_decision_ =
      ExactDirectSparseGatewayClockAuthorityInitializationDecision::
          complete_certified_empty_authority_journal;
  return result;
}

ExactDirectSparseGatewayClockAuthorityCaptureResult
ExactDirectSparseGatewayClockAuthorityJournal::capture_source_batch(
    std::size_t source_batch_index,
    const ExactDirectSparsePositiveFacetLocator& locator) {
  ExactDirectSparseGatewayClockAuthorityCaptureResult result;
  result.requested_source_batch_index = source_batch_index;
  result.required_chronological_index = capture_records_.size();
  result.authority_initialized = certified_initialized_authority();
  if (!result.authority_initialized) {
    result.decision =
        ExactDirectSparseGatewayClockAuthorityCaptureDecision::
            no_capture_authority_not_initialized;
    return result;
  }
  if (certificate_present_ || committed_certificate_count_ != 0U) {
    result.decision =
        ExactDirectSparseGatewayClockAuthorityCaptureDecision::
            no_capture_authority_already_sealed;
    return result;
  }
  if (source_batch_index >= source_batch_count_) {
    result.decision =
        ExactDirectSparseGatewayClockAuthorityCaptureDecision::
            no_capture_source_batch_out_of_range;
    return result;
  }
  if (source_chronology_indices_[source_batch_index] !=
      unassigned_chronology_index) {
    result.decision =
        ExactDirectSparseGatewayClockAuthorityCaptureDecision::
            no_capture_source_batch_duplicate;
    return result;
  }
  result.source_batch_in_range_and_previously_absent = true;

  if (capture_records_.size() >= source_batch_count_ ||
      capture_records_.size() >= budget_.maximum_capture_record_count ||
      capture_records_.capacity() <= capture_records_.size()) {
    result.decision =
        ExactDirectSparseGatewayClockAuthorityCaptureDecision::
            no_capture_capacity_exhausted;
    return result;
  }
  result.capture_capacity_preflight_certified = true;

  if (!locator.certified_positive_locator()) {
    result.decision =
        ExactDirectSparseGatewayClockAuthorityCaptureDecision::
            no_capture_locator_not_certified;
    return result;
  }
  result.locator_stamp_at_entry = locator.snapshot_stamp();
  if (!locator_stamp_encoding_is_valid(result.locator_stamp_at_entry) ||
      result.locator_stamp_at_entry.external_authority_id !=
          expected_locator_authority_id_) {
    result.locator_stamp_at_exit = locator.snapshot_stamp();
    result.decision =
        ExactDirectSparseGatewayClockAuthorityCaptureDecision::
            no_capture_stamp_encoding_rejected;
    return result;
  }
  if (capture_records_.empty()) {
    if (result.locator_stamp_at_entry != opening_locator_stamp_) {
      result.locator_stamp_at_exit = locator.snapshot_stamp();
      result.decision =
          ExactDirectSparseGatewayClockAuthorityCaptureDecision::
              no_capture_locator_chronology_rejected;
      return result;
    }
  } else {
    const auto& previous_stamp =
        capture_records_.back().locator_snapshot_stamp;
    if (result.locator_stamp_at_entry.committed_batch_count <
            previous_stamp.committed_batch_count ||
        (result.locator_stamp_at_entry.committed_batch_count ==
             previous_stamp.committed_batch_count &&
         result.locator_stamp_at_entry != previous_stamp)) {
      result.locator_stamp_at_exit = locator.snapshot_stamp();
      result.decision =
          ExactDirectSparseGatewayClockAuthorityCaptureDecision::
              no_capture_locator_chronology_rejected;
      return result;
    }
  }
  result.locator_prefix_chronology_certified = true;

  ExactDirectSparseGatewayClockAuthorityCaptureRecord record;
  record.chronological_index = capture_records_.size();
  record.source_batch_index = source_batch_index;
  record.locator_snapshot_stamp = result.locator_stamp_at_entry;
  record.previous_chain_digest = current_capture_chain_digest_;
  if (!compute_capture_chain_digest(
          authority_id_,
          session_id_,
          source_batch_count_,
          record.chronological_index,
          record.source_batch_index,
          record.locator_snapshot_stamp,
          record.previous_chain_digest,
          record.chain_digest)) {
    result.locator_stamp_at_exit = locator.snapshot_stamp();
    result.decision =
        ExactDirectSparseGatewayClockAuthorityCaptureDecision::
            no_capture_stamp_encoding_rejected;
    return result;
  }
  result.canonical_chain_transition_freshly_computed = true;
  result.locator_stamp_at_exit = locator.snapshot_stamp();
  if (result.locator_stamp_at_entry != result.locator_stamp_at_exit) {
    result.decision =
        ExactDirectSparseGatewayClockAuthorityCaptureDecision::
            no_capture_frozen_locator_rejected;
    return result;
  }
  result.locator_certified_and_entry_exit_stamp_equal = true;
  result.chronological_index_dense =
      record.chronological_index == capture_records_.size();

  capture_records_.push_back(record);
  source_chronology_indices_[source_batch_index] =
      record.chronological_index;
  current_capture_chain_digest_ = record.chain_digest;
  result.committed_record = record;
  result.source_batch_snapshot_committed = true;
  result.authority_state_mutated = true;
  result.decision =
      ExactDirectSparseGatewayClockAuthorityCaptureDecision::
          complete_certified_source_batch_snapshot_capture;
  return result;
}

ExactDirectSparseGatewayClockAuthoritySealResult
ExactDirectSparseGatewayClockAuthorityJournal::seal_clock_certificate(
    const ExactDirectSparseGatewayCandidateJournalResult& source_journal,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseGatewayClockAuthoritySealBudget& budget) {
  ExactDirectSparseGatewayClockAuthoritySealResult result;
  result.requested_budget = budget;
  result.required_source_batch_count = source_batch_count_;
  result.required_capture_record_scan_count = source_batch_count_;
  result.required_certificate_boundary_count = source_batch_count_;

  const auto fail =
      [&](ExactDirectSparseGatewayClockAuthoritySealDecision decision) {
        result.locator_stamp_at_exit = locator.snapshot_stamp();
        result.decision = decision;
        result.authority_state_mutated = false;
        result.exactly_one_certificate_committed = false;
        return result;
      };

  if (!certified_initialized_authority()) {
    return fail(
        ExactDirectSparseGatewayClockAuthoritySealDecision::
            no_seal_authority_not_initialized);
  }
  if (certificate_present_ || committed_certificate_count_ != 0U) {
    return fail(
        ExactDirectSparseGatewayClockAuthoritySealDecision::
            no_seal_already_sealed);
  }
  if (capture_records_.size() != source_batch_count_ ||
      source_journal.batches.size() != source_batch_count_) {
    return fail(
        ExactDirectSparseGatewayClockAuthoritySealDecision::
            no_seal_incomplete_or_foreign_source_batches);
  }

  if (!fits_u64(source_batch_count_) ||
      !checked_multiply(
          source_batch_count_,
          sizeof(ExactDirectSparseGatewayClockBoundary),
          result.required_temporary_scratch_byte_count)) {
    return fail(
        ExactDirectSparseGatewayClockAuthoritySealDecision::
            no_seal_capacity_overflow);
  }
  if (source_batch_count_ > budget.maximum_source_batch_count ||
      result.required_capture_record_scan_count >
          budget.maximum_capture_record_scan_count ||
      result.required_certificate_boundary_count >
          budget.maximum_certificate_boundary_count ||
      result.required_temporary_scratch_byte_count >
          budget.maximum_temporary_scratch_byte_count) {
    return fail(
        ExactDirectSparseGatewayClockAuthoritySealDecision::
            no_seal_budget_exhausted);
  }
  result.seal_budget_preflight_certified = true;

  if (!locator.certified_positive_locator()) {
    return fail(
        ExactDirectSparseGatewayClockAuthoritySealDecision::
            no_seal_frozen_locator_rejected);
  }
  result.locator_stamp_at_entry = locator.snapshot_stamp();
  if (!capture_records_.empty()) {
    const auto& last_capture_stamp =
        capture_records_.back().locator_snapshot_stamp;
    if (result.locator_stamp_at_entry.committed_batch_count <
            last_capture_stamp.committed_batch_count ||
        (result.locator_stamp_at_entry.committed_batch_count ==
             last_capture_stamp.committed_batch_count &&
         result.locator_stamp_at_entry != last_capture_stamp)) {
      return fail(
          ExactDirectSparseGatewayClockAuthoritySealDecision::
              no_seal_authority_history_rejected);
    }
  } else if (result.locator_stamp_at_entry != opening_locator_stamp_) {
    return fail(
        ExactDirectSparseGatewayClockAuthoritySealDecision::
            no_seal_authority_history_rejected);
  }
  result.final_locator_extends_capture_chronology = true;
  if (!locator_stamp_encoding_is_valid(result.locator_stamp_at_entry) ||
      result.locator_stamp_at_entry.external_authority_id !=
          expected_locator_authority_id_) {
    return fail(
        ExactDirectSparseGatewayClockAuthoritySealDecision::
            no_seal_frozen_locator_rejected);
  }

  result.source_identity_result =
      compute_exact_direct_sparse_gateway_candidate_scientific_identity(
          source_journal, budget.source_identity_budget);
  if (!result.source_identity_result.certified_identity()) {
    return fail(
        ExactDirectSparseGatewayClockAuthoritySealDecision::
            no_seal_source_identity_rejected);
  }
  if (result.source_identity_result.scientific_identity_digest !=
      source_scientific_identity_digest_) {
    return fail(
        ExactDirectSparseGatewayClockAuthoritySealDecision::
            no_seal_source_identity_rejected);
  }
  result.source_identity_freshly_computed = true;

  ExactDirectSparseGatewayClockCertificate certificate;
  certificate.authority_id = authority_id_;
  certificate.replay_token = session_id_;
  certificate.source_scientific_identity_digest =
      result.source_identity_result.scientific_identity_digest;
  certificate.final_locator_stamp = result.locator_stamp_at_entry;
  if (source_batch_count_ > certificate.boundaries.max_size()) {
    return fail(
        ExactDirectSparseGatewayClockAuthoritySealDecision::
            no_seal_capacity_overflow);
  }
  try {
    certificate.boundaries.reserve(source_batch_count_);
  } catch (const std::length_error&) {
    return fail(
        ExactDirectSparseGatewayClockAuthoritySealDecision::
            no_seal_capacity_overflow);
  } catch (const std::bad_alloc&) {
    return fail(
        ExactDirectSparseGatewayClockAuthoritySealDecision::
            no_seal_allocation_failed);
  }

  for (std::size_t source_index = 0U; source_index < source_batch_count_;
       ++source_index) {
    ++result.capture_record_scan_count;
    const std::size_t chronological_index =
        source_chronology_indices_[source_index];
    if (chronological_index >= capture_records_.size()) {
      return fail(
          ExactDirectSparseGatewayClockAuthoritySealDecision::
              no_seal_authority_history_rejected);
    }
    const auto& record = capture_records_[chronological_index];
    if (record.chronological_index != chronological_index ||
        record.source_batch_index != source_index) {
      return fail(
          ExactDirectSparseGatewayClockAuthoritySealDecision::
              no_seal_authority_history_rejected);
    }
    certificate.boundaries.push_back(
        {source_index,
         record.locator_snapshot_stamp.committed_batch_count,
         record.locator_snapshot_stamp});
  }
  result.every_source_batch_present_exactly_once = true;
  result.boundaries_reindexed_by_source_from_chronological_records = true;

  result.certificate_digest_result =
      compute_exact_direct_sparse_gateway_clock_certificate_digest(
          certificate, budget.certificate_digest_budget);
  if (!result.certificate_digest_result.certified_digest()) {
    return fail(
        ExactDirectSparseGatewayClockAuthoritySealDecision::
            no_seal_certificate_digest_rejected);
  }
  certificate.certificate_digest =
      result.certificate_digest_result.certificate_digest;
  certificate.digest_present = true;
  result.certificate_digest_freshly_computed = true;

  if (!compute_seal_digest(
          authority_id_,
          session_id_,
          source_batch_count_,
          capture_records_.size(),
          current_capture_chain_digest_,
          certificate.source_scientific_identity_digest,
          certificate.final_locator_stamp,
          certificate.certificate_digest,
          result.seal_digest)) {
    return fail(
        ExactDirectSparseGatewayClockAuthoritySealDecision::
            no_seal_capacity_overflow);
  }

  result.locator_stamp_at_exit = locator.snapshot_stamp();
  if (result.locator_stamp_at_entry != result.locator_stamp_at_exit ||
      certificate.final_locator_stamp != result.locator_stamp_at_exit ||
      (source_batch_count_ == 0U &&
       certificate.final_locator_stamp != opening_locator_stamp_)) {
    result.decision =
        ExactDirectSparseGatewayClockAuthoritySealDecision::
            no_seal_frozen_locator_rejected;
    return result;
  }
  result.locator_certified_and_entry_exit_stamp_equal = true;

  sealed_certificate_ = std::move(certificate);
  seal_digest_ = result.seal_digest;
  certificate_present_ = true;
  committed_certificate_count_ = 1U;
  result.exactly_one_certificate_committed = true;
  result.authority_state_mutated = true;
  result.decision =
      ExactDirectSparseGatewayClockAuthoritySealDecision::
          complete_certified_single_clock_certificate_seal;
  return result;
}

bool ExactDirectSparseGatewayClockAuthorityVerification::
    certified_external_clock_binding() const noexcept {
  std::size_t expected_scratch_bytes = 0U;
  const bool arithmetic_valid =
      checked_multiply(
          required_source_presence_entry_count,
          sizeof(std::size_t),
          expected_scratch_bytes);
  return schema_version ==
             direct_sparse_gateway_clock_authority_schema_version &&
         arithmetic_valid &&
         required_temporary_scratch_byte_count == expected_scratch_bytes &&
         required_capture_record_count <=
             requested_budget.maximum_capture_record_count &&
         required_capture_record_scan_count <=
             requested_budget.maximum_capture_record_scan_count &&
         required_source_presence_entry_count <=
             requested_budget.maximum_source_presence_entry_count &&
         required_source_presence_scan_count <=
             requested_budget.maximum_source_presence_scan_count &&
         required_temporary_scratch_byte_count <=
             requested_budget.maximum_temporary_scratch_byte_count &&
         capture_record_scan_count == required_capture_record_scan_count &&
         source_presence_scan_count == required_source_presence_scan_count &&
         authority_certificate_digest_result.certified_digest() &&
         authority_certificate_digest_result.requested_budget ==
             requested_budget.clock_verification_budget
                 .certificate_digest_budget &&
         clock_verification.requested_budget ==
             requested_budget.clock_verification_budget &&
         clock_verification.certified_conditional_clock_binding() &&
         replay_budget_preflight_certified &&
         external_seal_anchor_matched &&
         initialization_state_freshly_replayed &&
         opening_locator_stamp_bound_and_freshly_replayed &&
         chronological_indices_dense &&
         locator_prefix_chronology_nondecreasing_and_equal_prefix_stamp_stable &&
         source_batches_present_exactly_once_in_arbitrary_capture_order &&
         every_capture_chain_transition_freshly_replayed &&
         every_boundary_matches_captured_stamp_by_source &&
         final_locator_stamp_freshly_matched &&
         certificate_digest_freshly_matched &&
         single_seal_digest_freshly_replayed &&
         conditional_clock_certificate_freshly_verified &&
         !journal_and_locator_inputs_mutated && in_memory_replay_only &&
         !crash_durable && external_clock_authority_replayed &&
         !conditional_on_caller_clock_authority_replay &&
         conditional_on_caller_strict_pre_lot_orchestration &&
         !external_freeze_synchronization_replayed &&
         conditional_on_caller_external_freeze_synchronization &&
         !forbidden_global_structure_materialized &&
         !public_status_claimed && partial_refinement_only &&
         decision ==
             ExactDirectSparseGatewayClockAuthorityVerificationDecision::
                 complete_external_clock_authority_replayed &&
         scope ==
             ExactDirectSparseGatewayClockAuthorityVerificationScope::
                 in_memory_authority_captured_source_batches_to_frozen_locator_prefixes_and_single_10_11_clock_certificate_only &&
         result_certified;
}

ExactDirectSparseGatewayClockAuthorityVerification
verify_exact_direct_sparse_gateway_clock_authority_journal(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseGatewayCandidateBudget& trusted_source_budget,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectSparseGatewayCandidateJournalResult& observed_source,
    std::size_t trusted_component_handle_count,
    const ExactDirectSparsePositiveFacetLocatorBudget& trusted_locator_budget,
    const ExactDirectSparsePositiveFacetLocatorConfig& trusted_locator_config,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseGatewayClockAuthorityExternalSealAnchor&
        external_seal_anchor,
    const ExactDirectSparseGatewayClockAuthorityJournalBudget&
        trusted_authority_budget,
    const ExactDirectSparseGatewayClockAuthorityJournal& observed_authority,
    const ExactDirectSparseGatewayClockAuthorityVerificationBudget& budget) {
  const std::uint64_t trusted_authority_id =
      external_seal_anchor.authority_id;
  const std::uint64_t trusted_session_id =
      external_seal_anchor.session_id;
  ExactDirectSparseGatewayClockAuthorityVerification result;
  result.requested_budget = budget;
  result.locator_stamp_at_entry = locator.snapshot_stamp();
  result.scope =
      ExactDirectSparseGatewayClockAuthorityVerificationScope::
          in_memory_authority_captured_source_batches_to_frozen_locator_prefixes_and_single_10_11_clock_certificate_only;

  const std::size_t authority_record_count_at_entry =
      observed_authority.capture_records().size();
  const std::size_t authority_certificate_count_at_entry =
      observed_authority.committed_certificate_count();
  const contract::CanonicalId authority_chain_digest_at_entry =
      observed_authority.current_capture_chain_digest();
  const contract::CanonicalId authority_seal_digest_at_entry =
      observed_authority.seal_digest();

  const auto fail =
      [&](ExactDirectSparseGatewayClockAuthorityVerificationDecision
              decision) {
        result.locator_stamp_at_exit = locator.snapshot_stamp();
        result.decision = decision;
        result.external_clock_authority_replayed = false;
        result.conditional_on_caller_clock_authority_replay = true;
        result.result_certified = false;
        return result;
      };

  result.required_capture_record_count =
      observed_authority.capture_records().size();
  result.required_capture_record_scan_count =
      result.required_capture_record_count;
  result.required_source_presence_entry_count =
      observed_authority.source_batch_count();
  result.required_source_presence_scan_count =
      observed_authority.source_batch_count();
  if (!fits_u64(result.required_capture_record_count) ||
      !fits_u64(result.required_source_presence_entry_count) ||
      !checked_multiply(
          result.required_source_presence_entry_count,
          sizeof(std::size_t),
          result.required_temporary_scratch_byte_count)) {
    return fail(
        ExactDirectSparseGatewayClockAuthorityVerificationDecision::
            no_authority_replay_capacity_overflow);
  }
  if (result.required_capture_record_count >
          budget.maximum_capture_record_count ||
      result.required_capture_record_scan_count >
          budget.maximum_capture_record_scan_count ||
      result.required_source_presence_entry_count >
          budget.maximum_source_presence_entry_count ||
      result.required_source_presence_scan_count >
          budget.maximum_source_presence_scan_count ||
      result.required_temporary_scratch_byte_count >
          budget.maximum_temporary_scratch_byte_count) {
    return fail(
        ExactDirectSparseGatewayClockAuthorityVerificationDecision::
            no_authority_replay_budget_exhausted);
  }
  result.replay_budget_preflight_certified = true;

  if (trusted_authority_id == 0U || trusted_session_id == 0U ||
      trusted_locator_config.external_authority_id == 0U ||
      observed_authority.authority_id() != trusted_authority_id ||
      observed_authority.session_id() != trusted_session_id ||
      observed_authority.expected_locator_authority_id() !=
          trusted_locator_config.external_authority_id ||
      observed_authority.budget() != trusted_authority_budget) {
    return fail(
        ExactDirectSparseGatewayClockAuthorityVerificationDecision::
            no_authority_replay_identifier_rejected);
  }

  if (!observed_authority.certified_initialized_authority() ||
      !observed_authority.certified_sealed_once() ||
      result.required_capture_record_count !=
          result.required_source_presence_entry_count ||
      observed_authority.source_chronology_indices().size() !=
          result.required_source_presence_entry_count ||
      observed_source.batches.size() !=
          result.required_source_presence_entry_count) {
    return fail(
        ExactDirectSparseGatewayClockAuthorityVerificationDecision::
            no_authority_replay_initialization_rejected);
  }
  if (observed_authority.opening_locator_stamp().external_authority_id !=
          trusted_locator_config.external_authority_id ||
      observed_authority.opening_locator_stamp().committed_batch_count >
          result.locator_stamp_at_entry.committed_batch_count ||
      (result.required_capture_record_count == 0U &&
       observed_authority.opening_locator_stamp() !=
           observed_authority.sealed_certificate()
               .final_locator_stamp) ||
      (result.required_capture_record_count != 0U &&
       observed_authority.capture_records()
               .front()
               .locator_snapshot_stamp !=
           observed_authority.opening_locator_stamp())) {
    return fail(
        ExactDirectSparseGatewayClockAuthorityVerificationDecision::
            no_authority_replay_initialization_rejected);
  }

  if (!compute_initial_chain_digest(
          trusted_authority_id,
          trusted_session_id,
          observed_authority.opening_locator_stamp(),
          result.required_source_presence_entry_count,
          observed_authority.source_scientific_identity_digest(),
          result.replayed_initial_chain_digest) ||
      result.replayed_initial_chain_digest !=
          observed_authority.initial_chain_digest()) {
    return fail(
        ExactDirectSparseGatewayClockAuthorityVerificationDecision::
            no_authority_replay_chain_rejected);
  }
  result.initialization_state_freshly_replayed = true;

  std::vector<std::size_t> source_to_chronology;
  if (result.required_source_presence_entry_count >
      source_to_chronology.max_size()) {
    return fail(
        ExactDirectSparseGatewayClockAuthorityVerificationDecision::
            no_authority_replay_capacity_overflow);
  }
  try {
    source_to_chronology.assign(
        result.required_source_presence_entry_count,
        unassigned_chronology_index);
  } catch (const std::length_error&) {
    return fail(
        ExactDirectSparseGatewayClockAuthorityVerificationDecision::
            no_authority_replay_capacity_overflow);
  } catch (const std::bad_alloc&) {
    return fail(
        ExactDirectSparseGatewayClockAuthorityVerificationDecision::
            no_authority_replay_allocation_failed);
  }

  contract::CanonicalId running_digest =
      result.replayed_initial_chain_digest;
  for (std::size_t chronology_index = 0U;
       chronology_index < observed_authority.capture_records().size();
       ++chronology_index) {
    ++result.capture_record_scan_count;
    const auto& record =
        observed_authority.capture_records()[chronology_index];
    const bool chronological_stamp_valid =
        chronology_index == 0U ||
        (record.locator_snapshot_stamp.committed_batch_count >
             observed_authority.capture_records()[chronology_index - 1U]
                 .locator_snapshot_stamp.committed_batch_count) ||
        (record.locator_snapshot_stamp.committed_batch_count ==
             observed_authority.capture_records()[chronology_index - 1U]
                 .locator_snapshot_stamp.committed_batch_count &&
         record.locator_snapshot_stamp ==
             observed_authority.capture_records()[chronology_index - 1U]
                 .locator_snapshot_stamp);
    if (record.chronological_index != chronology_index ||
        record.source_batch_index >=
            result.required_source_presence_entry_count ||
        source_to_chronology[record.source_batch_index] !=
            unassigned_chronology_index ||
        record.previous_chain_digest != running_digest ||
        record.locator_snapshot_stamp.external_authority_id !=
            observed_authority.expected_locator_authority_id() ||
        !chronological_stamp_valid ||
        record.locator_snapshot_stamp.committed_batch_count >
            result.locator_stamp_at_entry.committed_batch_count) {
      return fail(
          ExactDirectSparseGatewayClockAuthorityVerificationDecision::
              no_authority_replay_chain_rejected);
    }
    contract::CanonicalId expected_digest;
    if (!compute_capture_chain_digest(
            trusted_authority_id,
            trusted_session_id,
            result.required_source_presence_entry_count,
            chronology_index,
            record.source_batch_index,
            record.locator_snapshot_stamp,
            running_digest,
            expected_digest) ||
        expected_digest != record.chain_digest) {
      return fail(
          ExactDirectSparseGatewayClockAuthorityVerificationDecision::
              no_authority_replay_chain_rejected);
    }
    source_to_chronology[record.source_batch_index] = chronology_index;
    running_digest = expected_digest;
  }
  result.replayed_capture_chain_digest = running_digest;
  if (running_digest !=
      observed_authority.current_capture_chain_digest()) {
    return fail(
        ExactDirectSparseGatewayClockAuthorityVerificationDecision::
            no_authority_replay_chain_rejected);
  }
  result.chronological_indices_dense = true;
  result
      .locator_prefix_chronology_nondecreasing_and_equal_prefix_stamp_stable =
      true;
  result.every_capture_chain_transition_freshly_replayed = true;

  const auto& certificate = observed_authority.sealed_certificate();
  if (certificate.boundaries.size() !=
      result.required_source_presence_entry_count) {
    return fail(
        ExactDirectSparseGatewayClockAuthorityVerificationDecision::
            no_authority_replay_source_coverage_rejected);
  }
  for (std::size_t source_index = 0U;
       source_index < result.required_source_presence_entry_count;
       ++source_index) {
    ++result.source_presence_scan_count;
    const std::size_t chronology_index =
        source_to_chronology[source_index];
    if (chronology_index == unassigned_chronology_index ||
        observed_authority.source_chronology_indices()[source_index] !=
            chronology_index) {
      return fail(
          ExactDirectSparseGatewayClockAuthorityVerificationDecision::
              no_authority_replay_source_coverage_rejected);
    }
    const auto& record =
        observed_authority.capture_records()[chronology_index];
    const auto& boundary = certificate.boundaries[source_index];
    if (boundary.source_batch_index != source_index ||
        boundary.strict_pre_locator_prefix_count !=
            record.locator_snapshot_stamp.committed_batch_count ||
        boundary.historical_locator_stamp !=
            record.locator_snapshot_stamp) {
      return fail(
          ExactDirectSparseGatewayClockAuthorityVerificationDecision::
              no_authority_replay_source_coverage_rejected);
    }
  }
  result.source_batches_present_exactly_once_in_arbitrary_capture_order =
      true;
  result.every_boundary_matches_captured_stamp_by_source = true;

  result.authority_certificate_digest_result =
      compute_exact_direct_sparse_gateway_clock_certificate_digest(
          certificate,
          budget.clock_verification_budget.certificate_digest_budget);
  if (!result.authority_certificate_digest_result.certified_digest() ||
      result.authority_certificate_digest_result.certificate_digest !=
          certificate.certificate_digest ||
      certificate.source_scientific_identity_digest !=
          observed_authority.source_scientific_identity_digest()) {
    return fail(
        ExactDirectSparseGatewayClockAuthorityVerificationDecision::
            no_authority_replay_certificate_rejected);
  }
  result.certificate_digest_freshly_matched = true;

  if (!compute_seal_digest(
          trusted_authority_id,
          trusted_session_id,
          result.required_source_presence_entry_count,
          result.required_capture_record_count,
          result.replayed_capture_chain_digest,
          certificate.source_scientific_identity_digest,
          certificate.final_locator_stamp,
          certificate.certificate_digest,
          result.replayed_seal_digest) ||
      result.replayed_seal_digest != observed_authority.seal_digest() ||
      result.replayed_seal_digest !=
          external_seal_anchor.expected_seal_digest) {
    return fail(
        ExactDirectSparseGatewayClockAuthorityVerificationDecision::
            no_authority_replay_certificate_rejected);
  }
  result.single_seal_digest_freshly_replayed = true;
  result.external_seal_anchor_matched = true;

  if (certificate.final_locator_stamp != result.locator_stamp_at_entry) {
    return fail(
        ExactDirectSparseGatewayClockAuthorityVerificationDecision::
            no_authority_replay_frozen_snapshot_rejected);
  }
  result.final_locator_stamp_freshly_matched = true;

  const ExactDirectSparseGatewayExternalClockAnchor anchor{
      trusted_authority_id,
      trusted_session_id,
      certificate.certificate_digest};
  try {
    result.clock_verification =
        verify_exact_direct_sparse_gateway_clock_certificate(
            index,
            cloud,
            source_facade,
            source_journal,
            source_arm_budget,
            source_arm_journal,
            source_incidence_budget,
            source_incidence_journal,
            trusted_source_budget,
            traversal_order,
            observed_source,
            trusted_component_handle_count,
            trusted_locator_budget,
            trusted_locator_config,
            locator,
            anchor,
            certificate,
            budget.clock_verification_budget);
  } catch (const std::length_error&) {
    return fail(
        ExactDirectSparseGatewayClockAuthorityVerificationDecision::
            no_authority_replay_capacity_overflow);
  } catch (const std::bad_alloc&) {
    return fail(
        ExactDirectSparseGatewayClockAuthorityVerificationDecision::
            no_authority_replay_allocation_failed);
  }
  if (!result.clock_verification.certified_conditional_clock_binding() ||
      result.clock_verification.external_clock_authority_replayed ||
      !result.clock_verification
           .conditional_on_caller_clock_authority_replay ||
      result.clock_verification.source_identity_result
              .scientific_identity_digest !=
          observed_authority.source_scientific_identity_digest()) {
    return fail(
        ExactDirectSparseGatewayClockAuthorityVerificationDecision::
            no_authority_replay_clock_rejected);
  }
  result.conditional_clock_certificate_freshly_verified = true;
  result.opening_locator_stamp_bound_and_freshly_replayed = true;

  result.locator_stamp_at_exit = locator.snapshot_stamp();
  if (result.locator_stamp_at_entry != result.locator_stamp_at_exit ||
      certificate.final_locator_stamp != result.locator_stamp_at_exit ||
      observed_authority.capture_records().size() !=
          authority_record_count_at_entry ||
      observed_authority.committed_certificate_count() !=
          authority_certificate_count_at_entry ||
      observed_authority.current_capture_chain_digest() !=
          authority_chain_digest_at_entry ||
      observed_authority.seal_digest() != authority_seal_digest_at_entry) {
    result.decision =
        ExactDirectSparseGatewayClockAuthorityVerificationDecision::
            no_authority_replay_frozen_snapshot_rejected;
    return result;
  }
  result.journal_and_locator_inputs_mutated = false;

  // The nested CLOCK verifier remains conditional by construction.  Only the
  // outer replay of the owning authority journal discharges that premise.
  result.external_clock_authority_replayed = true;
  result.conditional_on_caller_clock_authority_replay = false;
  result.decision =
      ExactDirectSparseGatewayClockAuthorityVerificationDecision::
          complete_external_clock_authority_replayed;
  result.result_certified = true;
  return result;
}

}  // namespace morsehgp3d::hierarchy
