#include "morsehgp3d/hierarchy/direct_sparse_gateway_clock.hpp"

#include "morsehgp3d/contract/canonical_id.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <boost/multiprecision/integer.hpp>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::size_t maximum_sha256_input_byte_count =
    static_cast<std::size_t>(
        std::numeric_limits<std::uint64_t>::max() / UINT64_C(8));
constexpr std::size_t certificate_fixed_payload_byte_count = 136U;
constexpr std::size_t certificate_boundary_payload_byte_count = 92U;

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

[[nodiscard]] bool sha256_input_size_is_valid(
    std::string_view domain,
    std::size_t payload_byte_count) noexcept {
  return domain.size() <= maximum_sha256_input_byte_count &&
         payload_byte_count <= maximum_sha256_input_byte_count - domain.size();
}

class MeasureSink {
 public:
  explicit MeasureSink(
      std::size_t maximum_exact_level_integer_bit_count) noexcept
      : maximum_exact_level_integer_bit_count_(
            maximum_exact_level_integer_bit_count) {}

  [[nodiscard]] bool append_byte(std::uint8_t) noexcept {
    return add_payload_bytes(1U);
  }

  [[nodiscard]] bool append_u32(std::uint32_t) noexcept {
    return add_payload_bytes(4U);
  }

  [[nodiscard]] bool append_u64(std::uint64_t) noexcept {
    return add_payload_bytes(8U);
  }

  [[nodiscard]] bool append_size(std::size_t value) noexcept {
    if (!fits_u64(value)) {
      capacity_overflow_ = true;
      return false;
    }
    return append_u64(static_cast<std::uint64_t>(value));
  }

  [[nodiscard]] bool append_id(const contract::CanonicalId&) noexcept {
    return add_payload_bytes(contract::CanonicalId::byte_count);
  }

  [[nodiscard]] bool append_level(const exact::ExactLevel& level) {
    const std::size_t numerator_bits = integer_bit_count(level.numerator());
    const std::size_t denominator_bits =
        integer_bit_count(level.denominator());
    maximum_observed_integer_bit_count_ = std::max(
        maximum_observed_integer_bit_count_,
        std::max(numerator_bits, denominator_bits));
    if (numerator_bits > maximum_exact_level_integer_bit_count_ ||
        denominator_bits > maximum_exact_level_integer_bit_count_) {
      bit_budget_exhausted_ = true;
      return false;
    }

    const std::string numerator = level.numerator_string();
    const std::string denominator = level.denominator_string();
    if (!fits_u64(numerator.size()) || !fits_u64(denominator.size())) {
      capacity_overflow_ = true;
      return false;
    }
    std::size_t added_decimal_bytes = 0U;
    if (!checked_add(
            numerator.size(), denominator.size(), added_decimal_bytes) ||
        !checked_add(
            exact_level_decimal_byte_count_,
            added_decimal_bytes,
            exact_level_decimal_byte_count_)) {
      capacity_overflow_ = true;
      return false;
    }
    return append_u64(static_cast<std::uint64_t>(numerator.size())) &&
           add_payload_bytes(numerator.size()) &&
           append_u64(static_cast<std::uint64_t>(denominator.size())) &&
           add_payload_bytes(denominator.size());
  }

  [[nodiscard]] std::size_t payload_byte_count() const noexcept {
    return payload_byte_count_;
  }
  [[nodiscard]] std::size_t exact_level_decimal_byte_count() const noexcept {
    return exact_level_decimal_byte_count_;
  }
  [[nodiscard]] std::size_t maximum_observed_integer_bit_count()
      const noexcept {
    return maximum_observed_integer_bit_count_;
  }
  [[nodiscard]] bool capacity_overflow() const noexcept {
    return capacity_overflow_;
  }
  [[nodiscard]] bool budget_exhausted() const noexcept {
    return bit_budget_exhausted_;
  }

 private:
  [[nodiscard]] static std::size_t integer_bit_count(
      const exact::BigInt& value) noexcept {
    if (value == 0) {
      return 1U;
    }
    return static_cast<std::size_t>(boost::multiprecision::msb(value)) + 1U;
  }

  [[nodiscard]] bool add_payload_bytes(std::size_t count) noexcept {
    if (!checked_add(payload_byte_count_, count, payload_byte_count_)) {
      capacity_overflow_ = true;
      return false;
    }
    return true;
  }

  std::size_t maximum_exact_level_integer_bit_count_{};
  std::size_t payload_byte_count_{};
  std::size_t exact_level_decimal_byte_count_{};
  std::size_t maximum_observed_integer_bit_count_{};
  bool capacity_overflow_{false};
  bool bit_budget_exhausted_{false};
};

class HashSink {
 public:
  explicit HashSink(contract::CanonicalSha256Builder& builder) noexcept
      : builder_(builder) {}

  [[nodiscard]] bool append_byte(std::uint8_t value) {
    const std::array<std::uint8_t, 1U> bytes{value};
    builder_.update(bytes);
    return true;
  }

  [[nodiscard]] bool append_u32(std::uint32_t value) {
    std::array<std::uint8_t, 4U> bytes{};
    for (std::size_t index = 0U; index < bytes.size(); ++index) {
      const std::size_t shift = (bytes.size() - 1U - index) * 8U;
      bytes[index] = static_cast<std::uint8_t>(value >> shift);
    }
    builder_.update(bytes);
    return true;
  }

  [[nodiscard]] bool append_u64(std::uint64_t value) {
    std::array<std::uint8_t, 8U> bytes{};
    for (std::size_t index = 0U; index < bytes.size(); ++index) {
      const std::size_t shift = (bytes.size() - 1U - index) * 8U;
      bytes[index] = static_cast<std::uint8_t>(value >> shift);
    }
    builder_.update(bytes);
    return true;
  }

  [[nodiscard]] bool append_size(std::size_t value) {
    if (!fits_u64(value)) {
      return false;
    }
    return append_u64(static_cast<std::uint64_t>(value));
  }

  [[nodiscard]] bool append_id(const contract::CanonicalId& value) {
    builder_.update(value.bytes());
    return true;
  }

  [[nodiscard]] bool append_level(const exact::ExactLevel& level) {
    const std::string numerator = level.numerator_string();
    const std::string denominator = level.denominator_string();
    return append_size(numerator.size()) && append_text(numerator) &&
           append_size(denominator.size()) && append_text(denominator);
  }

 private:
  [[nodiscard]] bool append_text(std::string_view value) {
    builder_.update(value);
    return true;
  }

  contract::CanonicalSha256Builder& builder_;
};

[[nodiscard]] bool append_traversal_order_tag(
    auto& sink,
    spatial::LbvhTraversalOrder order) {
  switch (order) {
    case spatial::LbvhTraversalOrder::near_first:
      return sink.append_byte(1U);
    case spatial::LbvhTraversalOrder::far_first:
      return sink.append_byte(2U);
  }
  return false;
}

[[nodiscard]] bool append_deletion_source_tag(
    auto& sink,
    ExactDirectSparseGatewayDeletionSource source) {
  switch (source) {
    case ExactDirectSparseGatewayDeletionSource::unspecified:
      return sink.append_byte(0U);
    case ExactDirectSparseGatewayDeletionSource::strict_arm_seed:
      return sink.append_byte(1U);
    case ExactDirectSparseGatewayDeletionSource::equal_level_facet_seed:
      return sink.append_byte(2U);
  }
  return false;
}

[[nodiscard]] bool append_level_relation_tag(
    auto& sink,
    ExactDirectSparseGatewayLevelRelation relation) {
  switch (relation) {
    case ExactDirectSparseGatewayLevelRelation::unspecified:
      return sink.append_byte(0U);
    case ExactDirectSparseGatewayLevelRelation::
        first_incidence_strictly_below_saddle:
      return sink.append_byte(1U);
    case ExactDirectSparseGatewayLevelRelation::
        first_incidence_equal_to_saddle:
      return sink.append_byte(2U);
  }
  return false;
}

[[nodiscard]] bool append_bool(auto& sink, bool value) {
  return sink.append_byte(value ? 1U : 0U);
}

[[nodiscard]] bool append_facet_key(
    auto& sink,
    const ExactDirectSparseFacetKey& key) {
  if (!sink.append_size(key.point_count)) {
    return false;
  }
  for (const spatial::PointId point_id : key.point_ids) {
    if (!sink.append_u64(static_cast<std::uint64_t>(point_id))) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool append_first_incidence_audit(
    auto& sink,
    const ExactDirectSparseFirstIncidenceAudit& audit) {
  const std::array<std::size_t, 18U> counters{
      audit.eligible_coface_point_count,
      audit.source_support_enumeration_count,
      audit.node_visit_count,
      audit.internal_node_expansion_count,
      audit.exact_aabb_bound_evaluation_count,
      audit.exact_point_evaluation_count,
      audit.excluded_facet_point_count,
      audit.coface_support_enumeration_count,
      audit.candidate_point_classification_count,
      audit.inside_or_boundary_source_ball_point_count,
      audit.outside_source_ball_point_count,
      audit.pruned_node_count,
      audit.pruned_eligible_point_count,
      audit.peak_frontier_entry_count,
      audit.peak_cominimizer_entry_count,
      audit.incumbent_improvement_count,
      audit.equal_incumbent_observation_count,
      audit.provisional_cominimizer_overflow_count,
  };
  for (const std::size_t counter : counters) {
    if (!sink.append_size(counter)) {
      return false;
    }
  }
  return append_bool(sink, audit.traversal_complete);
}

// This is the single canonical source-identity serializer used by the
// measurement and SHA passes.
[[nodiscard]] bool append_gateway_candidate_scientific_identity(
    auto& sink,
    const ExactDirectSparseGatewayCandidateJournalResult& journal) {
  if (!sink.append_u32(journal.schema_version) ||
      !append_traversal_order_tag(sink, journal.traversal_order) ||
      !sink.append_size(journal.point_count) ||
      !sink.append_size(journal.source_direct_event_count) ||
      !sink.append_id(journal.source_pair_canonical_cloud_digest) ||
      !sink.append_id(journal.source_higher_canonical_cloud_digest) ||
      !sink.append_id(journal.source_pair_semantic_digest) ||
      !sink.append_id(journal.source_higher_semantic_digest)) {
    return false;
  }

  if (!sink.append_byte(1U) ||
      !sink.append_size(journal.deletion_projections.size())) {
    return false;
  }
  for (const auto& projection : journal.deletion_projections) {
    if (!sink.append_size(projection.deletion_projection_index) ||
        !sink.append_size(projection.source_family_index) ||
        !append_deletion_source_tag(sink, projection.source) ||
        !sink.append_size(projection.source_deletion_index) ||
        !sink.append_size(projection.source_event_index) ||
        !sink.append_size(projection.source_order) ||
        !sink.append_u64(
            static_cast<std::uint64_t>(projection.removed_point_id)) ||
        !sink.append_size(projection.facet_token_index) ||
        !sink.append_level(projection.saddle_squared_level) ||
        !append_level_relation_tag(sink, projection.level_relation) ||
        !append_bool(
            sink, projection.removed_point_is_first_incidence_cominimizer)) {
      return false;
    }
  }

  if (!sink.append_byte(2U) || !sink.append_size(journal.facet_tokens.size())) {
    return false;
  }
  for (const auto& token : journal.facet_tokens) {
    if (!sink.append_size(token.facet_token_index) ||
        !append_facet_key(sink, token.source_facet_key) ||
        !sink.append_level(token.source_miniball_squared_level) ||
        !sink.append_level(token.first_incidence_squared_level) ||
        !append_first_incidence_audit(sink, token.first_incidence_audit) ||
        !sink.append_size(token.deletion_projection_offset) ||
        !sink.append_size(token.deletion_projection_count) ||
        !sink.append_size(token.gateway_candidate_offset) ||
        !sink.append_size(token.gateway_candidate_count) ||
        !sink.append_size(token.batch_index)) {
      return false;
    }
  }

  if (!sink.append_byte(3U) ||
      !sink.append_size(journal.gateway_candidates.size())) {
    return false;
  }
  for (const auto& candidate : journal.gateway_candidates) {
    if (!sink.append_size(candidate.gateway_candidate_index) ||
        !sink.append_size(candidate.facet_token_index) ||
        !sink.append_u64(static_cast<std::uint64_t>(candidate.added_point_id))) {
      return false;
    }
    for (const spatial::PointId point_id :
         candidate.positive_support_point_ids) {
      if (!sink.append_u64(static_cast<std::uint64_t>(point_id))) {
        return false;
      }
    }
    if (!sink.append_size(candidate.positive_support_point_count) ||
        !append_bool(sink, candidate.added_point_in_source_closed_ball) ||
        !append_bool(
            sink, candidate.added_point_in_selected_positive_support)) {
      return false;
    }
  }

  if (!sink.append_byte(4U) || !sink.append_size(journal.batches.size())) {
    return false;
  }
  for (const auto& batch : journal.batches) {
    if (!sink.append_size(batch.batch_index) ||
        !sink.append_size(batch.facet_cardinality) ||
        !sink.append_level(batch.first_incidence_squared_level) ||
        !sink.append_size(batch.facet_token_index_offset) ||
        !sink.append_size(batch.facet_token_index_count)) {
      return false;
    }
  }

  if (!sink.append_byte(5U) ||
      !sink.append_size(journal.batch_facet_token_indices.size())) {
    return false;
  }
  for (const std::size_t token_index :
       journal.batch_facet_token_indices) {
    if (!sink.append_size(token_index)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool append_locator_stamp(
    auto& sink,
    const ExactDirectSparsePositiveFacetLocatorSnapshotStamp& stamp) {
  return sink.append_u32(stamp.schema_version) &&
         sink.append_u64(stamp.external_authority_id) &&
         sink.append_size(stamp.committed_batch_count) &&
         sink.append_size(stamp.inserted_key_count) &&
         sink.append_size(stamp.component_union_count) &&
         sink.append_size(stamp.binding_count) &&
         sink.append_id(stamp.committed_history_digest);
}

// This is the single canonical certificate serializer.
[[nodiscard]] bool append_gateway_clock_certificate_payload(
    auto& sink,
    const ExactDirectSparseGatewayClockCertificate& certificate) {
  if (!sink.append_u32(certificate.schema_version) ||
      !sink.append_u64(certificate.authority_id) ||
      !sink.append_u64(certificate.replay_token) ||
      !sink.append_id(certificate.source_scientific_identity_digest) ||
      !append_locator_stamp(sink, certificate.final_locator_stamp) ||
      !sink.append_size(certificate.boundaries.size())) {
    return false;
  }
  for (const auto& boundary : certificate.boundaries) {
    if (!sink.append_size(boundary.source_batch_index) ||
        !sink.append_size(boundary.strict_pre_locator_prefix_count) ||
        !append_locator_stamp(sink, boundary.historical_locator_stamp)) {
      return false;
    }
  }
  return true;
}

struct PrefixSourceEntry {
  std::size_t prefix{};
  std::size_t source_batch_index{};
};

[[nodiscard]] bool prefix_source_less(
    const PrefixSourceEntry& left,
    const PrefixSourceEntry& right) noexcept {
  return left.prefix < right.prefix ||
         (left.prefix == right.prefix &&
          left.source_batch_index < right.source_batch_index);
}

class BoundedPrefixSourceComparator {
 public:
  explicit BoundedPrefixSourceComparator(std::size_t maximum_count) noexcept
      : maximum_count_(maximum_count) {}

  [[nodiscard]] bool less(
      const PrefixSourceEntry& left,
      const PrefixSourceEntry& right,
      bool& result) noexcept {
    if (comparison_count_ >= maximum_count_) {
      exhausted_ = true;
      return false;
    }
    ++comparison_count_;
    result = prefix_source_less(left, right);
    return true;
  }

  [[nodiscard]] std::size_t comparison_count() const noexcept {
    return comparison_count_;
  }
  [[nodiscard]] bool exhausted() const noexcept { return exhausted_; }

 private:
  std::size_t maximum_count_{};
  std::size_t comparison_count_{};
  bool exhausted_{false};
};

[[nodiscard]] bool sift_down_max_heap(
    std::span<PrefixSourceEntry> entries,
    std::size_t root,
    std::size_t heap_size,
    BoundedPrefixSourceComparator& comparator) noexcept {
  while (root < heap_size / 2U) {
    const std::size_t left_child = root * 2U + 1U;
    std::size_t greatest_child = left_child;
    const std::size_t right_child = left_child + 1U;
    if (right_child < heap_size) {
      bool left_is_less = false;
      if (!comparator.less(
              entries[left_child], entries[right_child], left_is_less)) {
        return false;
      }
      if (left_is_less) {
        greatest_child = right_child;
      }
    }
    bool root_is_less = false;
    if (!comparator.less(entries[root], entries[greatest_child], root_is_less)) {
      return false;
    }
    if (!root_is_less) {
      return true;
    }
    std::swap(entries[root], entries[greatest_child]);
    root = greatest_child;
  }
  return true;
}

[[nodiscard]] bool bounded_heapsort(
    std::span<PrefixSourceEntry> entries,
    BoundedPrefixSourceComparator& comparator) noexcept {
  if (entries.size() < 2U) {
    return true;
  }
  for (std::size_t index = entries.size() / 2U; index > 0U; --index) {
    if (!sift_down_max_heap(entries, index - 1U, entries.size(), comparator)) {
      return false;
    }
  }
  for (std::size_t heap_size = entries.size(); heap_size > 1U; --heap_size) {
    std::swap(entries[0U], entries[heap_size - 1U]);
    if (!sift_down_max_heap(entries, 0U, heap_size - 1U, comparator)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool sum_arena_record_count(
    const ExactDirectSparseGatewayCandidateJournalResult& journal,
    std::size_t& count) noexcept {
  count = 0U;
  return checked_add(count, journal.deletion_projections.size(), count) &&
         checked_add(count, journal.facet_tokens.size(), count) &&
         checked_add(count, journal.gateway_candidates.size(), count) &&
         checked_add(count, journal.batches.size(), count) &&
         checked_add(
             count, journal.batch_facet_token_indices.size(), count);
}

[[nodiscard]] bool scientific_identity_payload_size(
    std::size_t deletion_projection_count,
    std::size_t facet_token_count,
    std::size_t gateway_candidate_count,
    std::size_t batch_count,
    std::size_t batch_facet_token_index_count,
    std::size_t exact_level_decimal_byte_count,
    std::size_t& payload_byte_count) noexcept {
  payload_byte_count = 194U;
  const std::array<std::pair<std::size_t, std::size_t>, 5U> terms{{
      {deletion_projection_count, 75U},
      {facet_token_count, 313U},
      {gateway_candidate_count, 66U},
      {batch_count, 48U},
      {batch_facet_token_index_count, 8U},
  }};
  for (const auto& [count, record_byte_count] : terms) {
    std::size_t term = 0U;
    if (!checked_multiply(count, record_byte_count, term) ||
        !checked_add(payload_byte_count, term, payload_byte_count)) {
      return false;
    }
  }
  return checked_add(
      payload_byte_count,
      exact_level_decimal_byte_count,
      payload_byte_count);
}

[[nodiscard]] bool source_verification_is_complete(
    const ExactDirectSparseGatewayCandidateVerification& verification)
    noexcept {
  return verification.observed_storage_within_budget &&
         verification.source_incidence_journal_freshly_replayed &&
         verification.deletion_projections_freshly_replayed &&
         verification.facet_tokens_freshly_replayed &&
         verification.gateway_candidates_freshly_replayed &&
         verification.batches_freshly_replayed &&
         verification.counters_and_result_facts_freshly_replayed &&
         verification.no_forbidden_global_structure_materialized &&
         verification.fresh_replay_certified && verification.result_certified;
}

[[nodiscard]] bool locator_structure_verification_is_complete(
    const ExactDirectSparsePositiveFacetLocatorStructuralVerification&
        verification,
    const ExactDirectSparsePositiveFacetLocatorStructuralVerificationBudget&
        budget) noexcept {
  return verification.requested_budget == budget &&
         verification.required_table_slot_count <=
             budget.maximum_table_slot_count &&
         verification.required_key_point_count <=
             budget.maximum_key_point_count &&
         verification.required_component_parent_count <=
             budget.maximum_component_parent_count &&
         verification.required_union_record_count <=
             budget.maximum_union_record_count &&
         verification.required_batch_record_count <=
             budget.maximum_batch_record_count &&
         verification.required_binding_scratch_entry_count <=
             budget.maximum_binding_scratch_entry_count &&
         verification.required_key_point_scratch_entry_count <=
             budget.maximum_key_point_scratch_entry_count &&
         verification.required_table_slot_scratch_entry_count <=
             budget.maximum_table_slot_scratch_entry_count &&
         verification.required_component_parent_scratch_entry_count <=
             budget.maximum_component_parent_scratch_entry_count &&
         verification.required_temporary_scratch_byte_count <=
             budget.maximum_temporary_scratch_byte_count &&
         verification.fingerprint_search_slot_visit_count <=
             budget.maximum_fingerprint_search_slot_visit_count &&
         verification.insertion_chronology_slot_visit_count <=
             budget.maximum_insertion_chronology_slot_visit_count &&
         verification.union_parent_hop_count <=
             budget.maximum_union_parent_hop_count &&
         verification.table_slot_scan_count ==
             verification.required_table_slot_count &&
         verification.key_point_scan_count ==
             verification.required_key_point_count &&
         verification.union_record_scan_count ==
             verification.required_union_record_count &&
         verification.batch_record_scan_count ==
             verification.required_batch_record_count &&
         verification.required_key_point_scratch_entry_count ==
             verification.required_key_point_count &&
         verification.required_table_slot_scratch_entry_count ==
             verification.required_table_slot_count &&
         verification.required_component_parent_scratch_entry_count ==
             verification.required_component_parent_count &&
         verification.trusted_construction_parameters_certified &&
         verification.capacity_requirements_certified &&
         verification.scratch_requirement_arithmetic_certified &&
         verification.budget_preflight_certified &&
         !verification.fingerprint_search_budget_exhausted &&
         !verification.insertion_chronology_budget_exhausted &&
         !verification.union_parent_hop_budget_exhausted &&
         !verification.budget_exhausted &&
         !verification.structure_contract_rejected &&
         verification.flat_table_and_key_arena_certified &&
         verification.every_fingerprint_recomputed_and_full_key_located &&
         verification.committed_slot_insertion_chronology_freshly_replayed &&
         verification.dense_handle_dsu_replay_certified &&
         verification.union_witness_structure_certified &&
         verification.historical_batch_assertions_and_counters_well_formed &&
         verification.committed_history_digest_freshly_replayed &&
         verification.internal_fact_fields_match_contract &&
         verification.decision_and_scope_certified &&
         !verification.external_authority_replayed_by_locator &&
         verification.bounded_temporary_scratch_without_second_durable_output &&
         verification.fresh_durable_structure_verification_certified &&
         verification.result_certified &&
         verification.decision ==
             ExactDirectSparsePositiveFacetLocatorStructuralVerificationDecision::
                 complete_certified_durable_structure_verification;
}

}  // namespace

bool ExactDirectSparseGatewayCandidateScientificIdentityResult::
    certified_identity() const noexcept {
  std::size_t expected_arena_scan_count = 0U;
  std::size_t expected_payload_byte_count = 0U;
  const bool arithmetic_valid =
      checked_add(
          required_deletion_projection_count,
          required_facet_token_count,
          expected_arena_scan_count) &&
      checked_add(
          expected_arena_scan_count,
          required_gateway_candidate_count,
          expected_arena_scan_count) &&
      checked_add(
          expected_arena_scan_count,
          required_batch_count,
          expected_arena_scan_count) &&
      checked_add(
          expected_arena_scan_count,
          required_batch_facet_token_index_count,
          expected_arena_scan_count) &&
      scientific_identity_payload_size(
          required_deletion_projection_count,
          required_facet_token_count,
          required_gateway_candidate_count,
          required_batch_count,
          required_batch_facet_token_index_count,
          required_exact_level_decimal_byte_count,
          expected_payload_byte_count);
  return schema_version == direct_sparse_gateway_clock_schema_version &&
         arithmetic_valid &&
         arena_record_scan_count == expected_arena_scan_count &&
         required_digest_payload_byte_count == expected_payload_byte_count &&
         required_deletion_projection_count <=
             requested_budget.maximum_deletion_projection_count &&
         required_facet_token_count <=
             requested_budget.maximum_facet_token_count &&
         required_gateway_candidate_count <=
             requested_budget.maximum_gateway_candidate_count &&
         required_batch_count <= requested_budget.maximum_batch_count &&
         required_batch_facet_token_index_count <=
             requested_budget.maximum_batch_facet_token_index_count &&
         required_maximum_single_exact_level_integer_bit_count <=
             requested_budget.maximum_single_exact_level_integer_bit_count &&
         required_exact_level_decimal_byte_count <=
             requested_budget.maximum_exact_level_decimal_byte_count &&
         required_digest_payload_byte_count <=
             requested_budget.maximum_digest_payload_byte_count &&
         sha256_input_size_is_valid(
             direct_sparse_gateway_candidate_scientific_identity_domain,
             required_digest_payload_byte_count) &&
         population_budget_preflight_certified &&
         exact_level_integer_bits_within_budget &&
         exact_level_decimal_bytes_within_budget &&
         digest_payload_bytes_within_budget &&
         canonical_encoding_freshly_replayed &&
         all_five_scientific_arenas_bound && digest_present &&
         decision ==
             ExactDirectSparseGatewayCandidateScientificIdentityDecision::
                 complete_certified_scientific_identity;
}

bool ExactDirectSparseGatewayClockCertificateDigestResult::certified_digest()
    const noexcept {
  std::size_t boundary_bytes = 0U;
  std::size_t expected_payload_byte_count = 0U;
  const bool arithmetic_valid =
      checked_multiply(
          required_boundary_count,
          certificate_boundary_payload_byte_count,
          boundary_bytes) &&
      checked_add(
          certificate_fixed_payload_byte_count,
          boundary_bytes,
          expected_payload_byte_count);
  return schema_version == direct_sparse_gateway_clock_schema_version &&
         arithmetic_valid &&
         required_digest_payload_byte_count == expected_payload_byte_count &&
         boundary_scan_count == required_boundary_count &&
         required_boundary_count <= requested_budget.maximum_boundary_count &&
         required_digest_payload_byte_count <=
             requested_budget.maximum_digest_payload_byte_count &&
         sha256_input_size_is_valid(
             direct_sparse_gateway_clock_certificate_digest_domain,
             required_digest_payload_byte_count) &&
         budget_preflight_certified && canonical_encoding_freshly_replayed &&
         digest_present &&
         decision ==
             ExactDirectSparseGatewayClockCertificateDigestDecision::
                 complete_certified_certificate_digest;
}

bool ExactDirectSparseGatewayClockVerification::
    certified_conditional_clock_binding() const noexcept {
  std::size_t expected_boundary_scan_count = 0U;
  std::size_t sort_scratch_bytes = 0U;
  std::size_t prefix_scratch_bytes = 0U;
  std::size_t expected_scratch_bytes = 0U;
  const bool arithmetic_valid =
      checked_multiply(
          required_boundary_count, 2U, expected_boundary_scan_count) &&
      checked_multiply(
          required_boundary_count,
          sizeof(PrefixSourceEntry),
          sort_scratch_bytes) &&
      checked_multiply(
          required_boundary_count,
          sizeof(std::size_t),
          prefix_scratch_bytes) &&
      checked_add(
          sort_scratch_bytes,
          prefix_scratch_bytes,
          expected_scratch_bytes);
  const auto& prefix_budget = requested_budget.prefix_stamp_sweep_budget;
  std::size_t prefix_internal_scratch_bytes = 0U;
  const bool prefix_scratch_arithmetic_valid = checked_multiply(
      prefix_stamp_sweep_counters.binding_record_replay_count,
      sizeof(std::size_t),
      prefix_internal_scratch_bytes);
  const bool prefix_counters_within_budget =
      prefix_stamp_sweep_counters.prefix_request_scan_count <=
          prefix_budget.maximum_prefix_request_count &&
      prefix_stamp_sweep_counters.batch_record_scan_count <=
          prefix_budget.maximum_batch_record_scan_count &&
      prefix_stamp_sweep_counters.table_slot_scan_count <=
          prefix_budget.maximum_table_slot_scan_count &&
      prefix_stamp_sweep_counters.union_record_replay_count <=
          prefix_budget.maximum_union_record_replay_count &&
      prefix_stamp_sweep_counters.binding_record_replay_count <=
          prefix_budget.maximum_binding_record_replay_count &&
      prefix_stamp_sweep_counters.binding_record_replay_count <=
          prefix_budget.maximum_binding_slot_index_scratch_count &&
      prefix_stamp_sweep_counters.key_point_replay_count <=
          prefix_budget.maximum_key_point_replay_count &&
      prefix_scratch_arithmetic_valid &&
      prefix_internal_scratch_bytes <=
          prefix_budget.maximum_temporary_scratch_byte_count;
  return schema_version == direct_sparse_gateway_clock_schema_version &&
         arithmetic_valid &&
         required_boundary_scan_count == expected_boundary_scan_count &&
         boundary_scan_count == required_boundary_scan_count &&
         required_sort_scratch_entry_count == required_boundary_count &&
         required_prefix_scratch_entry_count == required_boundary_count &&
         required_temporary_scratch_byte_count == expected_scratch_bytes &&
         required_boundary_count <= requested_budget.maximum_boundary_count &&
         required_boundary_scan_count <=
             requested_budget.maximum_boundary_scan_count &&
         required_sort_scratch_entry_count <=
             requested_budget.maximum_sort_scratch_entry_count &&
         required_prefix_scratch_entry_count <=
             requested_budget.maximum_prefix_scratch_entry_count &&
         required_temporary_scratch_byte_count <=
             requested_budget.maximum_temporary_scratch_byte_count &&
         sort_comparison_count <=
             requested_budget.maximum_sort_comparison_count &&
         source_verification_is_complete(source_journal_verification) &&
         source_identity_result.certified_identity() &&
         source_identity_result.required_batch_count ==
             required_boundary_count &&
         source_identity_result.requested_budget ==
             requested_budget.source_identity_budget &&
         locator_structure_verification_is_complete(
             locator_structure_verification,
             requested_budget.locator_structure_budget) &&
         locator_structure_verification.required_batch_record_count ==
             locator_stamp_at_entry.committed_batch_count &&
         locator_structure_verification.required_binding_scratch_entry_count ==
             locator_stamp_at_entry.inserted_key_count &&
         locator_structure_verification.required_union_record_count ==
             locator_stamp_at_entry.component_union_count &&
         certificate_digest_result.certified_digest() &&
         certificate_digest_result.required_boundary_count ==
             required_boundary_count &&
         certificate_digest_result.requested_budget ==
             requested_budget.certificate_digest_budget &&
         locator_stamp_at_entry == locator_stamp_at_exit &&
         prefix_stamp_sweep_counters.prefix_request_scan_count ==
             required_boundary_count &&
         prefix_stamp_sweep_counters.emitted_stamp_count ==
             required_boundary_count &&
         prefix_stamp_sweep_counters.locator_snapshot_check_count == 2U &&
         prefix_counters_within_budget &&
         boundary_and_scratch_budget_preflight_certified &&
         external_anchor_non_null_and_payload_matched &&
         source_journal_freshly_replayed &&
         locator_structure_freshly_replayed &&
         source_scientific_identity_freshly_replayed &&
         certificate_digest_freshly_replayed &&
         boundaries_dense_and_prefixes_in_history &&
         boundaries_sorted_by_prefix_without_source_monotonicity_assumption &&
         every_historical_stamp_freshly_replayed &&
         final_locator_stamp_matches_entry_and_exit &&
         !source_and_locator_inputs_mutated &&
         !external_clock_authority_replayed &&
         conditional_on_caller_clock_authority_replay &&
         !forbidden_global_structure_materialized && !public_status_claimed &&
         partial_refinement_only &&
         decision ==
             ExactDirectSparseGatewayClockVerificationDecision::
                 complete_conditional_source_batch_locator_clock_certificate &&
         scope ==
             ExactDirectSparseGatewayClockScope::
                 source_gateway_candidate_batches_to_strict_pre_locator_commit_prefixes_relative_to_caller_clock_authority_only &&
         result_certified;
}

ExactDirectSparseGatewayCandidateScientificIdentityResult
compute_exact_direct_sparse_gateway_candidate_scientific_identity(
    const ExactDirectSparseGatewayCandidateJournalResult& journal,
    const ExactDirectSparseGatewayCandidateScientificIdentityBudget& budget) {
  ExactDirectSparseGatewayCandidateScientificIdentityResult result;
  result.requested_budget = budget;
  result.required_deletion_projection_count =
      journal.deletion_projections.size();
  result.required_facet_token_count = journal.facet_tokens.size();
  result.required_gateway_candidate_count = journal.gateway_candidates.size();
  result.required_batch_count = journal.batches.size();
  result.required_batch_facet_token_index_count =
      journal.batch_facet_token_indices.size();

  if (!fits_u64(result.required_deletion_projection_count) ||
      !fits_u64(result.required_facet_token_count) ||
      !fits_u64(result.required_gateway_candidate_count) ||
      !fits_u64(result.required_batch_count) ||
      !fits_u64(result.required_batch_facet_token_index_count) ||
      !sum_arena_record_count(journal, result.arena_record_scan_count)) {
    result.decision =
        ExactDirectSparseGatewayCandidateScientificIdentityDecision::
            no_identity_capacity_overflow;
    return result;
  }
  if (result.required_deletion_projection_count >
          budget.maximum_deletion_projection_count ||
      result.required_facet_token_count > budget.maximum_facet_token_count ||
      result.required_gateway_candidate_count >
          budget.maximum_gateway_candidate_count ||
      result.required_batch_count > budget.maximum_batch_count ||
      result.required_batch_facet_token_index_count >
          budget.maximum_batch_facet_token_index_count) {
    result.decision =
        ExactDirectSparseGatewayCandidateScientificIdentityDecision::
            no_identity_budget_exhausted;
    return result;
  }
  result.population_budget_preflight_certified = true;

  MeasureSink measurement{
      budget.maximum_single_exact_level_integer_bit_count};
  if (!append_gateway_candidate_scientific_identity(measurement, journal)) {
    result.required_maximum_single_exact_level_integer_bit_count =
        measurement.maximum_observed_integer_bit_count();
    result.required_exact_level_decimal_byte_count =
        measurement.exact_level_decimal_byte_count();
    result.required_digest_payload_byte_count =
        measurement.payload_byte_count();
    if (measurement.capacity_overflow()) {
      result.decision =
          ExactDirectSparseGatewayCandidateScientificIdentityDecision::
              no_identity_capacity_overflow;
    } else if (measurement.budget_exhausted()) {
      result.decision =
          ExactDirectSparseGatewayCandidateScientificIdentityDecision::
              no_identity_budget_exhausted;
    } else {
      result.decision =
          ExactDirectSparseGatewayCandidateScientificIdentityDecision::
              no_identity_encoding_rejected;
    }
    return result;
  }
  result.required_maximum_single_exact_level_integer_bit_count =
      measurement.maximum_observed_integer_bit_count();
  result.required_exact_level_decimal_byte_count =
      measurement.exact_level_decimal_byte_count();
  result.required_digest_payload_byte_count = measurement.payload_byte_count();
  result.exact_level_integer_bits_within_budget = true;
  result.exact_level_decimal_bytes_within_budget =
      result.required_exact_level_decimal_byte_count <=
      budget.maximum_exact_level_decimal_byte_count;
  result.digest_payload_bytes_within_budget =
      result.required_digest_payload_byte_count <=
      budget.maximum_digest_payload_byte_count;
  if (!result.exact_level_decimal_bytes_within_budget ||
      !result.digest_payload_bytes_within_budget) {
    result.decision =
        ExactDirectSparseGatewayCandidateScientificIdentityDecision::
            no_identity_budget_exhausted;
    return result;
  }

  if (!sha256_input_size_is_valid(
          direct_sparse_gateway_candidate_scientific_identity_domain,
          result.required_digest_payload_byte_count)) {
    result.digest_payload_bytes_within_budget = false;
    result.decision =
        ExactDirectSparseGatewayCandidateScientificIdentityDecision::
            no_identity_capacity_overflow;
    return result;
  }

  contract::CanonicalSha256Builder builder;
  builder.update(direct_sparse_gateway_candidate_scientific_identity_domain);
  HashSink hash_sink{builder};
  if (!append_gateway_candidate_scientific_identity(hash_sink, journal)) {
    result.decision =
        ExactDirectSparseGatewayCandidateScientificIdentityDecision::
            no_identity_encoding_rejected;
    return result;
  }
  result.scientific_identity_digest = builder.finalize();
  result.canonical_encoding_freshly_replayed = true;
  result.all_five_scientific_arenas_bound = true;
  result.digest_present = true;
  result.decision =
      ExactDirectSparseGatewayCandidateScientificIdentityDecision::
          complete_certified_scientific_identity;
  return result;
}

ExactDirectSparseGatewayClockCertificateDigestResult
compute_exact_direct_sparse_gateway_clock_certificate_digest(
    const ExactDirectSparseGatewayClockCertificate& certificate,
    const ExactDirectSparseGatewayClockCertificateDigestBudget& budget) {
  ExactDirectSparseGatewayClockCertificateDigestResult result;
  result.requested_budget = budget;
  result.required_boundary_count = certificate.boundaries.size();
  std::size_t boundary_bytes = 0U;
  if (!fits_u64(result.required_boundary_count) ||
      !checked_multiply(
          result.required_boundary_count,
          certificate_boundary_payload_byte_count,
          boundary_bytes) ||
      !checked_add(
          certificate_fixed_payload_byte_count,
          boundary_bytes,
          result.required_digest_payload_byte_count) ||
      !sha256_input_size_is_valid(
          direct_sparse_gateway_clock_certificate_digest_domain,
          result.required_digest_payload_byte_count)) {
    result.decision =
        ExactDirectSparseGatewayClockCertificateDigestDecision::
            no_digest_capacity_overflow;
    return result;
  }
  if (result.required_boundary_count > budget.maximum_boundary_count ||
      result.required_digest_payload_byte_count >
          budget.maximum_digest_payload_byte_count) {
    result.decision =
        ExactDirectSparseGatewayClockCertificateDigestDecision::
            no_digest_budget_exhausted;
    return result;
  }
  result.budget_preflight_certified = true;

  contract::CanonicalSha256Builder builder;
  builder.update(direct_sparse_gateway_clock_certificate_digest_domain);
  HashSink hash_sink{builder};
  if (!append_gateway_clock_certificate_payload(hash_sink, certificate)) {
    result.decision =
        ExactDirectSparseGatewayClockCertificateDigestDecision::
            no_digest_capacity_overflow;
    return result;
  }
  result.boundary_scan_count = result.required_boundary_count;
  result.certificate_digest = builder.finalize();
  result.canonical_encoding_freshly_replayed = true;
  result.digest_present = true;
  result.decision =
      ExactDirectSparseGatewayClockCertificateDigestDecision::
          complete_certified_certificate_digest;
  return result;
}

ExactDirectSparseGatewayClockVerification
verify_exact_direct_sparse_gateway_clock_certificate(
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
    const ExactDirectSparseGatewayExternalClockAnchor& external_anchor,
    const ExactDirectSparseGatewayClockCertificate& certificate,
    const ExactDirectSparseGatewayClockVerificationBudget& budget) {
  ExactDirectSparseGatewayClockVerification result;
  result.requested_budget = budget;
  result.locator_stamp_at_entry = locator.snapshot_stamp();
  result.external_clock_authority_replayed = false;
  result.conditional_on_caller_clock_authority_replay = true;
  result.forbidden_global_structure_materialized = false;
  result.public_status_claimed = false;
  result.partial_refinement_only = true;
  result.scope =
      ExactDirectSparseGatewayClockScope::
          source_gateway_candidate_batches_to_strict_pre_locator_commit_prefixes_relative_to_caller_clock_authority_only;

  const auto fail = [&](ExactDirectSparseGatewayClockVerificationDecision
                            decision) {
    result.locator_stamp_at_exit = locator.snapshot_stamp();
    result.decision = decision;
    result.result_certified = false;
    return result;
  };

  result.required_boundary_count = certificate.boundaries.size();
  result.required_sort_scratch_entry_count =
      result.required_boundary_count;
  result.required_prefix_scratch_entry_count =
      result.required_boundary_count;
  std::size_t sort_scratch_bytes = 0U;
  std::size_t prefix_scratch_bytes = 0U;
  if (!fits_u64(result.required_boundary_count) ||
      !checked_multiply(
          result.required_boundary_count,
          2U,
          result.required_boundary_scan_count) ||
      !checked_multiply(
          result.required_sort_scratch_entry_count,
          sizeof(PrefixSourceEntry),
          sort_scratch_bytes) ||
      !checked_multiply(
          result.required_prefix_scratch_entry_count,
          sizeof(std::size_t),
          prefix_scratch_bytes) ||
      !checked_add(
          sort_scratch_bytes,
          prefix_scratch_bytes,
          result.required_temporary_scratch_byte_count)) {
    return fail(
        ExactDirectSparseGatewayClockVerificationDecision::
            no_clock_capacity_overflow);
  }
  if (result.required_boundary_count > budget.maximum_boundary_count ||
      result.required_boundary_scan_count >
          budget.maximum_boundary_scan_count ||
      result.required_sort_scratch_entry_count >
          budget.maximum_sort_scratch_entry_count ||
      result.required_prefix_scratch_entry_count >
          budget.maximum_prefix_scratch_entry_count ||
      result.required_temporary_scratch_byte_count >
          budget.maximum_temporary_scratch_byte_count) {
    return fail(
        ExactDirectSparseGatewayClockVerificationDecision::
            no_clock_budget_exhausted);
  }
  result.boundary_and_scratch_budget_preflight_certified = true;

  if (external_anchor.authority_id == 0U ||
      external_anchor.replay_token == 0U ||
      certificate.schema_version != direct_sparse_gateway_clock_schema_version ||
      certificate.authority_id == 0U || certificate.replay_token == 0U ||
      certificate.authority_id != external_anchor.authority_id ||
      certificate.replay_token != external_anchor.replay_token ||
      !certificate.digest_present) {
    return fail(
        ExactDirectSparseGatewayClockVerificationDecision::
            no_clock_anchor_rejected);
  }
  result.external_anchor_non_null_and_payload_matched = true;

  result.source_journal_verification =
      verify_exact_direct_sparse_gateway_candidate_journal(
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
          observed_source);
  if (!result.source_journal_verification.result_certified) {
    return fail(
        ExactDirectSparseGatewayClockVerificationDecision::
            no_clock_source_journal_rejected);
  }
  result.source_journal_freshly_replayed = true;

  result.locator_structure_verification =
      verify_exact_direct_sparse_positive_facet_locator_structure(
          trusted_component_handle_count,
          trusted_locator_budget,
          trusted_locator_config,
          budget.locator_structure_budget,
          locator.state_view());
  if (!result.locator_structure_verification.result_certified) {
    return fail(
        ExactDirectSparseGatewayClockVerificationDecision::
            no_clock_locator_structure_rejected);
  }
  result.locator_structure_freshly_replayed = true;

  result.source_identity_result =
      compute_exact_direct_sparse_gateway_candidate_scientific_identity(
          observed_source, budget.source_identity_budget);
  if (!result.source_identity_result.certified_identity() ||
      result.source_identity_result.scientific_identity_digest !=
          certificate.source_scientific_identity_digest) {
    return fail(
        ExactDirectSparseGatewayClockVerificationDecision::
            no_clock_source_identity_rejected);
  }
  result.source_scientific_identity_freshly_replayed = true;

  result.certificate_digest_result =
      compute_exact_direct_sparse_gateway_clock_certificate_digest(
          certificate, budget.certificate_digest_budget);
  if (!result.certificate_digest_result.certified_digest() ||
      result.certificate_digest_result.certificate_digest !=
          certificate.certificate_digest ||
      result.certificate_digest_result.certificate_digest !=
          external_anchor.expected_certificate_digest) {
    return fail(
        ExactDirectSparseGatewayClockVerificationDecision::
            no_clock_certificate_digest_rejected);
  }
  result.certificate_digest_freshly_replayed = true;

  if (result.required_boundary_count != observed_source.batches.size() ||
      certificate.final_locator_stamp != result.locator_stamp_at_entry) {
    return fail(
        ExactDirectSparseGatewayClockVerificationDecision::
            no_clock_certificate_shape_rejected);
  }

  std::vector<PrefixSourceEntry> sorted_boundaries;
  sorted_boundaries.reserve(result.required_sort_scratch_entry_count);
  for (std::size_t source_index = 0U;
       source_index < certificate.boundaries.size();
       ++source_index) {
    const auto& boundary = certificate.boundaries[source_index];
    ++result.boundary_scan_count;
    if (boundary.source_batch_index != source_index ||
        boundary.strict_pre_locator_prefix_count >
            result.locator_stamp_at_entry.committed_batch_count) {
      sorted_boundaries.clear();
      return fail(
          ExactDirectSparseGatewayClockVerificationDecision::
              no_clock_certificate_shape_rejected);
    }
    sorted_boundaries.push_back(
        {boundary.strict_pre_locator_prefix_count, source_index});
  }
  result.boundaries_dense_and_prefixes_in_history = true;

  BoundedPrefixSourceComparator comparator{
      budget.maximum_sort_comparison_count};
  if (!bounded_heapsort(sorted_boundaries, comparator)) {
    result.sort_comparison_count = comparator.comparison_count();
    sorted_boundaries.clear();
    return fail(
        ExactDirectSparseGatewayClockVerificationDecision::
            no_clock_sort_budget_exhausted);
  }
  result.sort_comparison_count = comparator.comparison_count();
  result.boundaries_sorted_by_prefix_without_source_monotonicity_assumption =
      true;

  std::vector<std::size_t> sorted_prefixes;
  sorted_prefixes.reserve(result.required_prefix_scratch_entry_count);
  for (const auto& entry : sorted_boundaries) {
    sorted_prefixes.push_back(entry.prefix);
  }

  const auto prefix_stamp_sweep =
      build_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
          sorted_prefixes, locator, budget.prefix_stamp_sweep_budget);
  result.prefix_stamp_sweep_counters = prefix_stamp_sweep.counters;
  if (!prefix_stamp_sweep.certified_partial_refinement() ||
      !prefix_stamp_sweep.certified_outcome() ||
      prefix_stamp_sweep.prefix_stamps.size() !=
          result.required_boundary_count ||
      prefix_stamp_sweep.locator_snapshot_stamp !=
          result.locator_stamp_at_entry) {
    sorted_prefixes.clear();
    sorted_boundaries.clear();
    return fail(
        ExactDirectSparseGatewayClockVerificationDecision::
            no_clock_prefix_stamp_replay_rejected);
  }

  for (std::size_t sorted_index = 0U;
       sorted_index < sorted_boundaries.size();
       ++sorted_index) {
    ++result.boundary_scan_count;
    const std::size_t source_index =
        sorted_boundaries[sorted_index].source_batch_index;
    if (prefix_stamp_sweep.prefix_stamps[sorted_index] !=
        certificate.boundaries[source_index].historical_locator_stamp) {
      sorted_prefixes.clear();
      sorted_boundaries.clear();
      return fail(
          ExactDirectSparseGatewayClockVerificationDecision::
              no_clock_prefix_stamp_replay_rejected);
    }
  }
  result.every_historical_stamp_freshly_replayed = true;

  sorted_prefixes.clear();
  sorted_boundaries.clear();
  result.locator_stamp_at_exit = locator.snapshot_stamp();
  if (certificate.final_locator_stamp != result.locator_stamp_at_entry ||
      certificate.final_locator_stamp != result.locator_stamp_at_exit) {
    result.decision =
        ExactDirectSparseGatewayClockVerificationDecision::
            no_clock_frozen_snapshot_rejected;
    return result;
  }
  result.final_locator_stamp_matches_entry_and_exit = true;
  // Both scientific inputs are const and this kernel exposes no write path.
  // This fact does not claim to detect an unsynchronized concurrent writer;
  // callers must externally freeze the journal and locator during the call.
  result.source_and_locator_inputs_mutated = false;
  result.decision =
      ExactDirectSparseGatewayClockVerificationDecision::
          complete_conditional_source_batch_locator_clock_certificate;
  result.result_certified = true;
  return result;
}

}  // namespace morsehgp3d::hierarchy
