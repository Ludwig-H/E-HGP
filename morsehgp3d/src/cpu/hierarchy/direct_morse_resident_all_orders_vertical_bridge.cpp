#include "morsehgp3d/hierarchy/direct_morse_resident_all_orders_vertical_bridge.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <boost/multiprecision/cpp_int.hpp>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

std::atomic<std::uint64_t> next_all_orders_bridge_session_instance_id{1U};

[[nodiscard]] std::uint64_t allocate_bridge_session_instance_id() noexcept {
  std::uint64_t candidate = next_all_orders_bridge_session_instance_id.load(
      std::memory_order_relaxed);
  while (candidate != 0U) {
    const std::uint64_t successor = candidate + 1U;
    if (next_all_orders_bridge_session_instance_id.compare_exchange_weak(
            candidate,
            successor,
            std::memory_order_relaxed,
            std::memory_order_relaxed)) {
      return candidate;
    }
  }
  return 0U;
}

[[nodiscard]] bool checked_add(
    std::size_t left, std::size_t right, std::size_t& output) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  output = left + right;
  return true;
}

[[nodiscard]] bool checked_twice(
    std::size_t value, std::size_t& output) noexcept {
  if (value > std::numeric_limits<std::size_t>::max() / 2U) {
    return false;
  }
  output = 2U * value;
  return true;
}

[[nodiscard]] bool valid_slice(
    std::size_t offset,
    std::size_t count,
    std::size_t arena_size) noexcept {
  return offset <= arena_size && count <= arena_size - offset;
}

[[nodiscard]] bool facet_key_less(
    const ExactDirectSparseFacetKey& left,
    const ExactDirectSparseFacetKey& right) noexcept {
  if (left.point_count != right.point_count) {
    return left.point_count < right.point_count;
  }
  return std::lexicographical_compare(
      left.point_ids.begin(),
      left.point_ids.end(),
      right.point_ids.begin(),
      right.point_ids.end());
}

[[nodiscard]] bool canonical_facet_key(
    const ExactDirectSparseFacetKey& key,
    std::size_t expected_point_count,
    std::size_t cloud_point_count) noexcept {
  if (expected_point_count == 0U ||
      expected_point_count > direct_sparse_positive_facet_maximum_point_count ||
      key.point_count != expected_point_count) {
    return false;
  }
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    if (key.point_ids[index] >= cloud_point_count ||
        (index != 0U &&
         key.point_ids[index - 1U] >= key.point_ids[index])) {
      return false;
    }
  }
  return std::all_of(
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(key.point_count),
      key.point_ids.end(),
      [](spatial::PointId point_id) { return point_id == 0U; });
}

[[nodiscard]] ExactDirectSparseFacetKey erase_source_point(
    const ExactDirectSparseFacetKey& source,
    std::size_t erased_index) noexcept {
  ExactDirectSparseFacetKey output;
  if (source.point_count <= 1U || erased_index >= source.point_count) {
    return output;
  }
  output.point_count = source.point_count - 1U;
  std::size_t output_index = 0U;
  for (std::size_t source_index = 0U;
       source_index < source.point_count;
       ++source_index) {
    if (source_index == erased_index) {
      continue;
    }
    output.point_ids[output_index] = source.point_ids[source_index];
    ++output_index;
  }
  return output;
}

[[nodiscard]] bool token_less(
    const ExactFrozenIncidenceToken& left,
    const ExactFrozenIncidenceToken& right) noexcept {
  if (left.kind != right.kind) {
    return static_cast<std::uint8_t>(left.kind) <
           static_cast<std::uint8_t>(right.kind);
  }
  return left.token_id < right.token_id;
}

[[nodiscard]] const ExactFrozenIncidenceTokenBinding* find_token_binding(
    const ExactFrozenIncidenceQuotientResult& quotient,
    const ExactFrozenIncidenceToken& token) noexcept {
  const auto iterator = std::lower_bound(
      quotient.token_bindings.begin(),
      quotient.token_bindings.end(),
      token,
      [](const ExactFrozenIncidenceTokenBinding& binding,
         const ExactFrozenIncidenceToken& value) {
        return token_less(binding.token, value);
      });
  if (iterator == quotient.token_bindings.end() || iterator->token != token) {
    return nullptr;
  }
  return &*iterator;
}

[[nodiscard]] const ExactDirectMorseResidentAllOrdersVerticalRootWitness*
find_root_witness(
    const std::vector<
        ExactDirectMorseResidentAllOrdersVerticalRootWitness>& witnesses,
    ExactFrozenIncidencePriorRootId source_root_id,
    std::size_t* index = nullptr) noexcept {
  const auto iterator = std::lower_bound(
      witnesses.begin(),
      witnesses.end(),
      source_root_id,
      [](const ExactDirectMorseResidentAllOrdersVerticalRootWitness& witness,
         ExactFrozenIncidencePriorRootId value) {
        return witness.source_root_id < value;
      });
  if (iterator == witnesses.end() ||
      iterator->source_root_id != source_root_id) {
    return nullptr;
  }
  if (index != nullptr) {
    *index = static_cast<std::size_t>(iterator - witnesses.begin());
  }
  return &*iterator;
}

[[nodiscard]] const ExactDirectMorseUnifiedResidentRootCoverage*
find_root_coverage(
    const std::vector<ExactDirectMorseUnifiedResidentRootCoverage>& roots,
    ExactFrozenIncidencePriorRootId root_id) noexcept {
  const auto iterator = std::lower_bound(
      roots.begin(),
      roots.end(),
      root_id,
      [](const ExactDirectMorseUnifiedResidentRootCoverage& root,
         ExactFrozenIncidencePriorRootId value) {
        return root.root_id < value;
      });
  if (iterator == roots.end() || iterator->root_id != root_id) {
    return nullptr;
  }
  return &*iterator;
}

[[nodiscard]] bool source_root_witnesses_strictly_ordered(
    const std::vector<
        ExactDirectMorseResidentAllOrdersVerticalRootWitness>& witnesses)
    noexcept {
  for (std::size_t index = 0U; index < witnesses.size(); ++index) {
    if (!witnesses[index].certified_conditional_root_witness() ||
        (index != 0U &&
         witnesses[index - 1U].source_root_id >=
             witnesses[index].source_root_id)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool normalized_source_facts_consistent(
    ExactDirectMorseUnifiedResidentSourceKind source_kind,
    bool normalized_direct_source_session,
    ExactDirectNormalizedH0ResidentRetractionMode retraction_mode,
    bool normalized_horizontal_incidence_reduction_certified) noexcept {
  if (source_kind ==
      ExactDirectMorseUnifiedResidentSourceKind::successive_incidence_star) {
    return !normalized_direct_source_session &&
           retraction_mode ==
               ExactDirectNormalizedH0ResidentRetractionMode::
                   not_applicable_successive_incidence_star &&
           !normalized_horizontal_incidence_reduction_certified;
  }
  return source_kind ==
             ExactDirectMorseUnifiedResidentSourceKind::
                 normalized_direct_h0_candidate_source &&
         normalized_direct_source_session &&
         retraction_mode !=
             ExactDirectNormalizedH0ResidentRetractionMode::
                 not_applicable_successive_incidence_star &&
         (!normalized_horizontal_incidence_reduction_certified ||
          retraction_mode ==
              ExactDirectNormalizedH0ResidentRetractionMode::
                  certified_horizontal_incidence_complete_rank_window_and_sparse_strict_facet_closure);
}

[[nodiscard]] bool normalized_incidence_complete_v7_capability(
    ExactDirectMorseUnifiedResidentSourceKind source_kind,
    bool normalized_direct_source_session,
    ExactDirectNormalizedH0ResidentRetractionMode retraction_mode,
    bool normalized_horizontal_incidence_reduction_certified) noexcept {
  return source_kind ==
             ExactDirectMorseUnifiedResidentSourceKind::
                 normalized_direct_h0_candidate_source &&
         normalized_direct_source_session &&
         retraction_mode ==
             ExactDirectNormalizedH0ResidentRetractionMode::
                 certified_horizontal_incidence_complete_rank_window_and_sparse_strict_facet_closure &&
         normalized_horizontal_incidence_reduction_certified;
}

struct TargetProbePayload {
  ExactFrozenIncidencePriorRootId target_root_id{};
  ExactDirectSparseFacetKey canonical_target_key{};
  ExactDirectSparseFacetWitness source_binding_witness{};
  bool sparse_target_closure_used{false};
};

enum class TargetProbeStatus : std::uint8_t {
  complete,
  budget_exhausted,
  locator_rejected,
  target_not_rooted,
};

struct GroupAccumulator {
  bool initialized{false};
  ExactFrozenIncidencePriorRootId target_root_id{};
  ExactDirectSparseFacetKey canonical_target_key{};
  ExactDirectSparseFacetWitness canonical_source_binding_witness{};
};

[[nodiscard]] bool add_group_candidate(
    GroupAccumulator& accumulator,
    const ExactDirectSparseFacetKey& key,
    const TargetProbePayload& probe) noexcept {
  if (!accumulator.initialized) {
    accumulator.initialized = true;
    accumulator.target_root_id = probe.target_root_id;
    accumulator.canonical_target_key = key;
    accumulator.canonical_source_binding_witness =
        probe.source_binding_witness;
    return true;
  }
  if (accumulator.target_root_id != probe.target_root_id) {
    return false;
  }
  if (key == accumulator.canonical_target_key) {
    return accumulator.canonical_source_binding_witness ==
           probe.source_binding_witness;
  }
  if (facet_key_less(key, accumulator.canonical_target_key)) {
    accumulator.canonical_target_key = key;
    accumulator.canonical_source_binding_witness =
        probe.source_binding_witness;
  }
  return true;
}

void append_u8(
    contract::CanonicalSha256Builder& builder, std::uint8_t value) {
  const std::array<std::uint8_t, 1U> bytes{value};
  builder.update(bytes);
}

void append_u32(
    contract::CanonicalSha256Builder& builder, std::uint32_t value) {
  std::array<std::uint8_t, 4U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    bytes[index] = static_cast<std::uint8_t>(
        value >> ((bytes.size() - 1U - index) * 8U));
  }
  builder.update(bytes);
}

void append_u64(
    contract::CanonicalSha256Builder& builder, std::uint64_t value) {
  std::array<std::uint8_t, 8U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    bytes[index] = static_cast<std::uint8_t>(
        value >> ((bytes.size() - 1U - index) * 8U));
  }
  builder.update(bytes);
}

void append_size(
    contract::CanonicalSha256Builder& builder, std::size_t value) {
  static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));
  append_u64(builder, static_cast<std::uint64_t>(value));
}

void append_id(
    contract::CanonicalSha256Builder& builder,
    const contract::CanonicalId& value) {
  builder.update(value.bytes());
}

[[nodiscard]] bool append_nonnegative_bigint(
    contract::CanonicalSha256Builder& builder,
    const exact::BigInt& value) noexcept {
  try {
    if (value < 0) {
      return false;
    }
    if (value == 0) {
      append_size(builder, 0U);
      return true;
    }
    const std::size_t bit_count =
        static_cast<std::size_t>(boost::multiprecision::msb(value)) + 1U;
    const std::size_t byte_count = (bit_count + 7U) / 8U;
    append_size(builder, byte_count);
    for (std::size_t byte_index = 0U;
         byte_index < byte_count;
         ++byte_index) {
      const std::size_t low_bit = 8U * (byte_count - 1U - byte_index);
      std::uint8_t byte = 0U;
      for (std::size_t bit = 0U; bit < 8U; ++bit) {
        if (boost::multiprecision::bit_test(value, low_bit + bit)) {
          byte |= static_cast<std::uint8_t>(std::uint8_t{1U} << bit);
        }
      }
      append_u8(builder, byte);
    }
    return true;
  } catch (...) {
    return false;
  }
}

[[nodiscard]] bool append_level(
    contract::CanonicalSha256Builder& builder,
    const exact::ExactLevel& level) noexcept {
  return append_nonnegative_bigint(builder, level.numerator()) &&
         append_nonnegative_bigint(builder, level.denominator());
}

void append_locator_stamp(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparsePositiveFacetLocatorSnapshotStamp& stamp) {
  append_u32(builder, stamp.schema_version);
  append_u64(builder, stamp.external_authority_id);
  append_size(builder, stamp.committed_batch_count);
  append_size(builder, stamp.inserted_key_count);
  append_size(builder, stamp.component_union_count);
  append_size(builder, stamp.binding_count);
  append_id(builder, stamp.committed_history_digest);
}

void append_snapshot_identity(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectMorseUnifiedSnapshotIdentity& identity) {
  append_u32(builder, identity.schema_version);
  append_u64(builder, identity.session_authority_id);
  append_u64(builder, identity.locator_instance_id);
  append_size(builder, identity.epoch);
  append_size(builder, identity.batch_cursor);
  append_id(builder, identity.source_pair_canonical_cloud_digest);
  append_id(builder, identity.source_higher_canonical_cloud_digest);
  append_id(builder, identity.source_pair_semantic_digest);
  append_id(builder, identity.source_higher_semantic_digest);
  append_locator_stamp(builder, identity.locator_stamp);
}

struct AuthenticHigherDirectMembershipCounts {
  std::size_t saddle_count{};
  std::size_t birth_count{};
};

struct AuthenticTerminalComponentSource {
  std::size_t direct_reference_index{};
  std::size_t role_record_index{};
  std::size_t event_projection_index{};
  ExactDirectSparseFacetKey complete_source_facet_key{};
};

[[nodiscard]] bool inspect_authentic_higher_direct_membership(
    const ExactDirectSparseUnifiedLevelPlanResult& plan,
    const ExactDirectMorseUnifiedResidentAuthorityBundle& bundle,
    AuthenticHigherDirectMembershipCounts& counts) noexcept {
  counts = {};
  if (bundle.source_batch_index >= plan.batches.size()) {
    return false;
  }
  const auto& batch = plan.batches[bundle.source_batch_index];
  const auto& frozen = bundle.frozen_batch;
  if (batch.batch_index != bundle.source_batch_index ||
      batch.future_snapshot_index != bundle.source_future_snapshot_index ||
      batch.squared_level != bundle.squared_level ||
      batch.order != bundle.order || bundle.order < 3U ||
      !valid_slice(
          batch.direct_reference_offset,
          batch.direct_reference_count,
          plan.direct_references.size()) ||
      !valid_slice(
          batch.residual_reference_offset,
          batch.residual_reference_count,
          plan.residual_references.size()) ||
      frozen.source_batch_index != bundle.source_batch_index ||
      frozen.source_future_snapshot_index !=
          bundle.source_future_snapshot_index ||
      frozen.squared_level != bundle.squared_level ||
      frozen.order != bundle.order) {
    return false;
  }
  for (std::size_t local = 0U;
       local < batch.direct_reference_count;
       ++local) {
    const std::size_t direct_index = batch.direct_reference_offset + local;
    const auto& direct = plan.direct_references[direct_index];
    if (direct.direct_reference_index != direct_index ||
        direct.source_role_record_index >= plan.source_role_record_count ||
        direct.source_event_projection_index >=
            plan.source_event_projection_count) {
      return false;
    }
    if (direct.role == ExactDirectMorseH0Role::birth) {
      if (direct.source_incidence_family_index.has_value() ||
          !direct.direct_birth_facet_token_index.has_value() ||
          *direct.direct_birth_facet_token_index >= plan.facet_tokens.size()) {
        return false;
      }
      ++counts.birth_count;
    } else if (direct.role == ExactDirectMorseH0Role::saddle) {
      if (!direct.source_incidence_family_index.has_value() ||
          *direct.source_incidence_family_index >=
              plan.source_incidence_family_count ||
          direct.direct_birth_facet_token_index.has_value()) {
        return false;
      }
      ++counts.saddle_count;
    } else {
      return false;
    }
  }
  std::size_t expected_hyperedge_count = 0U;
  if (!checked_add(
          counts.saddle_count,
          batch.residual_reference_count,
          expected_hyperedge_count) ||
      frozen.counters.batch_direct_reference_scan_count !=
          batch.direct_reference_count ||
      frozen.counters.deferred_direct_birth_count != counts.birth_count ||
      frozen.counters.direct_saddle_hyperedge_count != counts.saddle_count ||
      frozen.counters.residual_hyperedge_count !=
          batch.residual_reference_count ||
      frozen.counters.hyperedge_count != expected_hyperedge_count ||
      frozen.counters.token_reference_count !=
          frozen.quotient_token_references.size() ||
      frozen.counters.group_count != frozen.quotient.groups.size() ||
      frozen.quotient_hyperedges.size() != expected_hyperedge_count ||
      frozen.provenance.size() != expected_hyperedge_count ||
      frozen.quotient.hyperedge_bindings.size() != expected_hyperedge_count ||
      frozen.incidence_facet_token_indices.size() !=
          frozen.quotient_token_references.size() ||
      frozen.action_plan.groups.size() != frozen.quotient.groups.size() ||
      frozen.coverage_deltas.size() != frozen.quotient.groups.size()) {
    return false;
  }
  for (std::size_t hyperedge_index = 0U;
       hyperedge_index < expected_hyperedge_count;
       ++hyperedge_index) {
    const auto& hyperedge = frozen.quotient_hyperedges[hyperedge_index];
    const auto& provenance = frozen.provenance[hyperedge_index];
    const auto& binding =
        frozen.quotient.hyperedge_bindings[hyperedge_index];
    const auto expected_kind =
        hyperedge_index < counts.saddle_count
            ? ExactFrozenIncidenceHyperedgeProvenanceKind::direct_family
            : ExactFrozenIncidenceHyperedgeProvenanceKind::residual_incidence;
    if (hyperedge.hyperedge_index != hyperedge_index ||
        !valid_slice(
            hyperedge.token_reference_offset,
            hyperedge.token_reference_count,
            frozen.quotient_token_references.size()) ||
        provenance.source_hyperedge_index != hyperedge_index ||
        provenance.kind != expected_kind ||
        binding.source_hyperedge_index != hyperedge_index ||
        binding.source_token_reference_offset !=
            hyperedge.token_reference_offset ||
        binding.source_token_reference_count !=
            hyperedge.token_reference_count ||
        binding.group_index >= frozen.quotient.groups.size()) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool inspect_authentic_terminal_component_source(
    const ExactDirectSparseUnifiedLevelPlanResult& plan,
    const ExactDirectMorseUnifiedResidentAuthorityBundle& bundle,
    const AuthenticHigherDirectMembershipCounts& counts,
    AuthenticTerminalComponentSource& source) noexcept {
  source = {};
  if (bundle.source_batch_index >= plan.batches.size() ||
      bundle.order < 3U || bundle.order != plan.point_count ||
      counts.birth_count != 1U || counts.saddle_count != 0U) {
    return false;
  }
  const auto& batch = plan.batches[bundle.source_batch_index];
  const auto& frozen = bundle.frozen_batch;
  if (batch.direct_reference_count != 1U ||
      batch.residual_reference_count != 0U ||
      frozen.counters.hyperedge_count != 0U ||
      frozen.counters.token_reference_count != 0U ||
      frozen.counters.group_count != 0U ||
      !frozen.quotient_hyperedges.empty() ||
      !frozen.quotient_token_references.empty() ||
      !frozen.quotient.groups.empty() ||
      !frozen.action_plan.groups.empty() ||
      !frozen.coverage_deltas.empty() ||
      !bundle.facet_resolutions.empty()) {
    return false;
  }
  const std::size_t direct_index = batch.direct_reference_offset;
  if (direct_index >= plan.direct_references.size()) {
    return false;
  }
  const auto& direct = plan.direct_references[direct_index];
  if (direct.direct_reference_index != direct_index ||
      direct.role != ExactDirectMorseH0Role::birth ||
      direct.source_incidence_family_index.has_value() ||
      !direct.direct_birth_facet_token_index.has_value() ||
      *direct.direct_birth_facet_token_index >= plan.facet_tokens.size()) {
    return false;
  }
  const auto& token =
      plan.facet_tokens[*direct.direct_birth_facet_token_index];
  if (token.facet_token_index != *direct.direct_birth_facet_token_index ||
      token.direct_birth_reference_count != 1U ||
      !canonical_facet_key(
          token.facet_key, bundle.order, plan.point_count)) {
    return false;
  }
  for (std::size_t index = 0U; index < plan.point_count; ++index) {
    if (token.facet_key.point_ids[index] !=
        static_cast<spatial::PointId>(index)) {
      return false;
    }
  }
  source.direct_reference_index = direct_index;
  source.role_record_index = direct.source_role_record_index;
  source.event_projection_index = direct.source_event_projection_index;
  source.complete_source_facet_key = token.facet_key;
  return true;
}

[[nodiscard]] bool populate_authentic_higher_saddle_membership(
    const ExactDirectSparseUnifiedLevelPlanResult& plan,
    const ExactDirectMorseUnifiedResidentAuthorityBundle& bundle,
    const AuthenticHigherDirectMembershipCounts& counts,
    std::vector<ExactDirectMorseResidentDirectSaddleGroupBinding>& bindings) {
  const auto& batch = plan.batches[bundle.source_batch_index];
  const auto& frozen = bundle.frozen_batch;
  bindings.clear();
  bindings.reserve(counts.saddle_count);
  std::size_t saddle_index = 0U;
  for (std::size_t local = 0U;
       local < batch.direct_reference_count;
       ++local) {
    const auto& direct = plan.direct_references[
        batch.direct_reference_offset + local];
    if (direct.role != ExactDirectMorseH0Role::saddle) {
      continue;
    }
    const auto& quotient_binding =
        frozen.quotient.hyperedge_bindings[saddle_index];
    bindings.push_back(ExactDirectMorseResidentDirectSaddleGroupBinding{
        bindings.size(),
        direct.direct_reference_index,
        direct.source_role_record_index,
        direct.source_event_projection_index,
        *direct.source_incidence_family_index,
        saddle_index,
        quotient_binding.group_index,
        quotient_binding.group_index,
        0U});
    ++saddle_index;
  }
  return saddle_index == counts.saddle_count &&
         bindings.size() == counts.saddle_count;
}

[[nodiscard]] bool count_higher_plan_saddles(
    const ExactDirectSparseUnifiedLevelPlanResult& plan,
    std::size_t& count) noexcept {
  count = 0U;
  for (const auto& batch : plan.batches) {
    if (batch.order < 3U) {
      continue;
    }
    if (!valid_slice(
            batch.direct_reference_offset,
            batch.direct_reference_count,
            plan.direct_references.size())) {
      return false;
    }
    for (std::size_t local = 0U;
         local < batch.direct_reference_count;
         ++local) {
      if (plan.direct_references[batch.direct_reference_offset + local].role ==
          ExactDirectMorseH0Role::saddle) {
        if (count == std::numeric_limits<std::size_t>::max()) {
          return false;
        }
        ++count;
      }
    }
  }
  return true;
}

[[nodiscard]] contract::CanonicalId compute_higher_membership_digest(
    const ExactDirectMorseResidentAllOrdersVerticalBatchRecord& record)
    noexcept {
  try {
    contract::CanonicalSha256Builder builder;
    builder.update(
        "MorseHGP3D/direct_morse_resident_higher_o4_membership/v4");
    append_u32(builder, record.schema_version);
    append_snapshot_identity(builder, record.resident_pre_batch_identity);
    append_locator_stamp(builder, record.live_post_resident_locator_stamp);
    append_size(builder, record.source_batch_index);
    if (!append_level(builder, record.squared_level)) {
      return {};
    }
    append_size(builder, record.source_order);
    append_size(builder, record.target_order);
    append_size(builder, record.frozen_hyperedge_count);
    append_size(builder, record.frozen_token_reference_count);
    append_size(builder, record.frozen_quotient_group_count);
    append_size(builder, record.frozen_direct_saddle_hyperedge_count);
    append_size(builder, record.frozen_residual_hyperedge_count);
    append_size(builder, record.group_images.size());
    for (const auto& image : record.group_images) {
      append_size(builder, image.owner_group_index);
      append_u64(builder, image.resident_resultant_source_root_id);
      append_u64(builder, image.resolved_target_root_id);
    }
    append_size(builder, record.direct_saddle_group_bindings.size());
    for (const auto& binding : record.direct_saddle_group_bindings) {
      append_size(builder, binding.binding_index);
      append_size(builder, binding.source_direct_reference_index);
      append_size(builder, binding.source_role_record_index);
      append_size(builder, binding.source_event_projection_index);
      append_size(builder, binding.source_incidence_family_index);
      append_size(builder, binding.source_hyperedge_index);
      append_size(builder, binding.owner_group_index);
      append_size(builder, binding.group_image_index);
      append_u64(builder, binding.resident_resultant_root_id);
    }
    append_u8(builder, record.terminal_component_image.has_value() ? 1U : 0U);
    if (record.terminal_component_image.has_value()) {
      const auto& image = *record.terminal_component_image;
      append_size(builder, image.source_batch_index);
      append_size(builder, image.source_direct_reference_index);
      append_size(builder, image.source_role_record_index);
      append_size(builder, image.source_event_projection_index);
      if (!append_level(builder, image.squared_level)) {
        return {};
      }
      append_size(builder, image.source_order);
      append_size(builder, image.target_order);
      append_size(builder, image.complete_source_facet_key.point_count);
      for (std::size_t index = 0U;
           index < image.complete_source_facet_key.point_count;
           ++index) {
        append_size(
            builder, image.complete_source_facet_key.point_ids[index]);
      }
      append_size(builder, image.canonical_target_facet_key.point_count);
      for (std::size_t index = 0U;
           index < image.canonical_target_facet_key.point_count;
           ++index) {
        append_size(
            builder, image.canonical_target_facet_key.point_ids[index]);
      }
      append_u64(
          builder, image.target_source_binding_witness.external_authority_id);
      append_u64(builder, image.target_source_binding_witness.replay_token);
      append_u64(builder, image.resolved_target_root_id);
      append_size(builder, image.target_deletion_probe_count);
      append_size(builder, image.sparse_target_closure_count);
    }
    return builder.finalize();
  } catch (...) {
    return {};
  }
}

}  // namespace

contract::CanonicalId
canonical_exact_direct_morse_resident_higher_o4_membership_digest(
    const ExactDirectMorseResidentAllOrdersVerticalBatchRecord& record)
    noexcept {
  return compute_higher_membership_digest(record);
}

struct ExactDirectMorseResidentAllOrdersVerticalBridge::Impl {
  struct Seal {
    std::uint64_t bridge_session_instance_id{};
    std::uint64_t owned_k2_k1_bridge_session_instance_id{};
    std::uint64_t resident_session_authority_id{};
    contract::CanonicalId canonical_cloud_digest{};
    contract::CanonicalId resident_higher_canonical_cloud_digest{};
  };

  struct TicketRegistry {
    std::size_t live_ticket_count{};
  };

  ExactDirectMorseResidentK2K1ClosedCutBridge owned_bridge{};
  ExactDirectMorseResidentAllOrdersVerticalBridgeBudget budget{};
  std::shared_ptr<const Seal> seal;
  std::shared_ptr<TicketRegistry> ticket_registry;
  ExactDirectMorseResidentAllOrdersVerticalBridgeStamp committed_stamp{};
  std::vector<ExactDirectMorseResidentAllOrdersVerticalRootWitness>
      source_root_target_witnesses;
  std::vector<ExactDirectMorseResidentAllOrdersVerticalBatchRecord>
      committed_higher_batches;
  std::optional<ExactDirectMorseResidentAllOrdersVerticalFinalSeal>
      final_vertical_seal;
  std::size_t required_k2_batch_count{};
  std::size_t required_higher_batch_count{};
  bool initialized{false};

  [[nodiscard]] bool structurally_ready() const noexcept {
    if (!initialized || seal == nullptr ||
        seal->bridge_session_instance_id == 0U ||
        seal->owned_k2_k1_bridge_session_instance_id == 0U ||
        seal->resident_session_authority_id == 0U ||
        seal->canonical_cloud_digest == contract::CanonicalId{} ||
        seal->resident_higher_canonical_cloud_digest ==
            contract::CanonicalId{} ||
        ticket_registry == nullptr || !owned_bridge.ready() ||
        committed_stamp.schema_version !=
            direct_morse_resident_all_orders_vertical_bridge_schema_version ||
        committed_stamp.bridge_session_instance_id !=
            seal->bridge_session_instance_id ||
        committed_stamp.owned_k2_k1_bridge_session_instance_id !=
            seal->owned_k2_k1_bridge_session_instance_id ||
        committed_stamp.resident_session_authority_id !=
            seal->resident_session_authority_id ||
        committed_stamp.canonical_cloud_digest !=
            seal->canonical_cloud_digest ||
        committed_stamp.resident_higher_canonical_cloud_digest !=
            seal->resident_higher_canonical_cloud_digest ||
        committed_stamp.resident_batch_cursor !=
            owned_bridge.resident_locator()
                .snapshot_stamp()
                .committed_batch_count ||
        committed_stamp.resident_epoch !=
            committed_stamp.resident_batch_cursor ||
        committed_stamp.committed_k2_batch_count !=
            owned_bridge.committed_k2_batches().size() ||
        committed_stamp.committed_higher_batch_count !=
            committed_higher_batches.size() ||
        committed_stamp.persistent_source_root_witness_count !=
            source_root_target_witnesses.size() ||
        committed_higher_batches.size() >
            budget.maximum_committed_higher_batch_count ||
        committed_stamp.committed_higher_group_count >
            budget.maximum_committed_higher_group_count ||
        source_root_target_witnesses.size() >
            budget.maximum_persistent_source_root_witness_count ||
        !source_root_witnesses_strictly_ordered(
            source_root_target_witnesses)) {
      return false;
    }
    const auto owned_stamp = owned_bridge.current_stamp();
    if (committed_stamp.committed_k2_group_count !=
            owned_stamp.committed_k2_group_count ||
        committed_stamp.committed_k2_direct_saddle_group_binding_count !=
            owned_stamp
                .committed_k2_direct_saddle_group_binding_count ||
        committed_stamp.committed_k2_direct_birth_k1_binding_count !=
            owned_stamp.committed_k2_direct_birth_k1_binding_count) {
      return false;
    }
    std::size_t group_count = 0U;
    std::size_t higher_saddle_binding_count = 0U;
    std::size_t terminal_component_image_count = 0U;
    for (const auto& record : committed_higher_batches) {
      if (!record.certified_conditional_higher_batch() ||
          !checked_add(group_count, record.group_images.size(), group_count) ||
          !checked_add(
              higher_saddle_binding_count,
              record.direct_saddle_group_bindings.size(),
              higher_saddle_binding_count) ||
          !checked_add(
              terminal_component_image_count,
              record.terminal_component_image.has_value() ? 1U : 0U,
              terminal_component_image_count)) {
        return false;
      }
    }
    if (group_count != committed_stamp.committed_higher_group_count ||
        terminal_component_image_count !=
            committed_stamp.committed_terminal_component_image_count ||
        terminal_component_image_count > 1U ||
        higher_saddle_binding_count !=
            committed_stamp
                .committed_higher_direct_saddle_group_binding_count ||
        higher_saddle_binding_count >
            budget
                .maximum_committed_higher_direct_saddle_group_binding_count ||
        !normalized_source_facts_consistent(
            owned_bridge.resident_source_kind(),
            owned_bridge.resident_normalized_direct_source_session(),
            owned_bridge.resident_normalized_h0_retraction_mode(),
            owned_bridge
                .resident_normalized_horizontal_incidence_reduction_certified())) {
      return false;
    }
    if (!final_vertical_seal.has_value()) {
      return true;
    }
    const auto& final = *final_vertical_seal;
    return final.certified_final_vertical_seal() &&
           final.sealed_stamp == committed_stamp &&
           final.terminal_locator_stamp ==
               owned_bridge.resident_locator().snapshot_stamp() &&
           final.resident_source_kind == owned_bridge.resident_source_kind() &&
           final.resident_normalized_direct_source_session ==
               owned_bridge.resident_normalized_direct_source_session() &&
           final.resident_normalized_h0_retraction_mode ==
               owned_bridge.resident_normalized_h0_retraction_mode() &&
           final.resident_normalized_horizontal_incidence_reduction_certified ==
               owned_bridge
                   .resident_normalized_horizontal_incidence_reduction_certified() &&
           final.k1_source_forest_digest ==
               owned_bridge.owned_k1_source_forest_digest() &&
           final.distinct_k1_level_count ==
               owned_bridge.owned_k1_distinct_level_count() &&
           owned_bridge.owned_k1_terminal_complete() &&
           final.normalized_incidence_complete_v7_source_capability ==
               normalized_incidence_complete_v7_capability(
                   owned_bridge.resident_source_kind(),
                   owned_bridge.resident_normalized_direct_source_session(),
                   owned_bridge.resident_normalized_h0_retraction_mode(),
                   owned_bridge
                       .resident_normalized_horizontal_incidence_reduction_certified());
  }
};

struct ExactDirectMorseResidentAllOrdersVerticalPreparedBatch::Impl {
  ~Impl() noexcept {
    if (!owns_ticket_slot) {
      return;
    }
    if (ticket_registry == nullptr ||
        ticket_registry->live_ticket_count == 0U) {
      std::terminate();
    }
    --ticket_registry->live_ticket_count;
  }

  std::shared_ptr<const ExactDirectMorseResidentAllOrdersVerticalBridge::Impl::
                      Seal>
      seal;
  std::shared_ptr<
      ExactDirectMorseResidentAllOrdersVerticalBridge::Impl::TicketRegistry>
      ticket_registry;
  std::optional<ExactDirectMorseResidentK2K1ClosedCutPreparedBatch>
      owned_bridge_ticket;
  ExactDirectMorseUnifiedSnapshotIdentity resident_identity{};
  std::optional<ExactDirectMorseResidentAllOrdersVerticalBatchRecord>
      higher_record;
  std::vector<ExactDirectMorseUnifiedResidentGroupRecord>
      expected_resident_groups;
  std::vector<std::size_t> prior_witness_reprobe_indices;
  std::vector<std::size_t> continuation_witness_indices;
  std::size_t resident_group_record_count_before{};
  std::size_t new_source_root_witness_count{};
  std::uint64_t next_query_replay_token{};
  bool owns_ticket_slot{false};
  bool consumed{false};
};

bool ExactDirectMorseResidentAllOrdersVerticalRootWitness::
    certified_conditional_root_witness() const noexcept {
  return source_root_id != 0U && source_order >= 3U &&
         target_order + 1U == source_order &&
         canonical_target_facet_key.point_count == target_order &&
         target_source_binding_witness.external_authority_id != 0U &&
         target_source_binding_witness.replay_token != 0U &&
         last_resolved_target_root_id != 0U &&
         first_source_batch_index <= last_verified_source_batch_index &&
         live_reprobe_count != 0U && canonical_target_facet_selected &&
         source_binding_witness_live_reprobed &&
         target_root_was_live_and_rooted &&
         !external_target_authority_replayed;
}

bool ExactDirectMorseResidentAllOrdersVerticalGroupImage::
    certified_conditional_group_image() const noexcept {
  return resident_resultant_source_root_id != 0U &&
         resolved_target_root_id != 0U &&
         canonical_target_facet_key.point_count >= 2U &&
         target_source_binding_witness.external_authority_id != 0U &&
         target_source_binding_witness.replay_token != 0U &&
         sparse_target_closure_count <= projected_target_facet_probe_count &&
         every_prior_source_root_witness_present_and_live_reprobed &&
         every_latent_or_equal_source_key_projected_to_all_deletions &&
         every_target_deletion_live_positive_and_rooted &&
         every_locator_miss_resolved_by_certified_sparse_target_closure &&
         one_live_target_root_for_complete_group &&
         resultant_source_root_bound_after_resident_commit;
}

bool ExactDirectMorseResidentTerminalComponentImage::
    certified_conditional_terminal_component_image() const noexcept {
  if (source_order < 3U || target_order + 1U != source_order ||
      !canonical_facet_key(
          complete_source_facet_key, source_order, source_order) ||
      !canonical_facet_key(
          canonical_target_facet_key, target_order, source_order) ||
      target_source_binding_witness.external_authority_id == 0U ||
      target_source_binding_witness.replay_token == 0U ||
      resolved_target_root_id == 0U ||
      target_deletion_probe_count != source_order ||
      sparse_target_closure_count != 0U ||
      !source_order_equals_cloud_point_count ||
      !exactly_one_terminal_birth_reference ||
      !frozen_terminal_batch_has_no_hyperedge_or_group ||
      !every_codimension_one_deletion_probed ||
      !every_target_deletion_live_positive_and_rooted ||
      !every_target_deletion_direct_positive_hit ||
      !one_live_target_root_for_complete_terminal_component ||
      !pre_batch_locator_snapshot_immutable_during_all_probes ||
      !resident_terminal_batch_committed_without_synthetic_group ||
      global_facet_coface_or_gamma_catalog_materialized ||
      ordinary_or_higher_order_delaunay_materialized ||
      public_status_claimed) {
    return false;
  }
  for (std::size_t index = 0U; index < source_order; ++index) {
    if (complete_source_facet_key.point_ids[index] !=
        static_cast<spatial::PointId>(index)) {
      return false;
    }
  }
  return true;
}

bool ExactDirectMorseResidentAllOrdersVerticalBatchRecord::
    certified_conditional_higher_batch() const noexcept {
  if (schema_version !=
          direct_morse_resident_all_orders_vertical_bridge_schema_version ||
      source_order < 3U || target_order + 1U != source_order ||
      resident_pre_batch_identity.batch_cursor != source_batch_index ||
      resident_pre_batch_identity.epoch != source_batch_index ||
      resident_pre_batch_identity.session_authority_id == 0U ||
      resident_pre_batch_identity.locator_instance_id == 0U ||
      resident_pre_batch_identity.locator_stamp.committed_batch_count !=
          source_batch_index ||
      live_post_resident_locator_stamp.external_authority_id !=
          resident_pre_batch_identity.session_authority_id ||
      live_post_resident_locator_stamp.committed_batch_count !=
          source_batch_index + 1U ||
      frozen_hyperedge_count !=
          frozen_direct_saddle_hyperedge_count +
              frozen_residual_hyperedge_count ||
      frozen_quotient_group_count != group_images.size() ||
      frozen_direct_saddle_hyperedge_count !=
          direct_saddle_group_bindings.size() ||
      !exact_product_order_target_batch_already_committed ||
      !pre_batch_locator_snapshot_immutable_during_all_probes ||
      !every_higher_group_has_one_inductive_target_image ||
      !every_direct_saddle_bound_exactly_once ||
      !bindings_replayed_against_frozen_hyperedge_quotient ||
      !all_residual_hyperedges_consumed_in_same_quotient ||
      !binding_group_images_crosschecked ||
      !o4_membership_digest_canonical ||
      o4_membership_digest == contract::CanonicalId{} ||
      !actual_resident_group_suffix_compared_field_by_field ||
      !resident_batch_committed ||
      !post_resident_commit_publication_allocation_free ||
      all_naturality_squares_replayed || vertical_maps_complete ||
      global_morse_obligation_replayed || full_pi0_membership_claimed ||
      m1_replayed ||
      global_facet_coface_or_gamma_catalog_materialized ||
      ordinary_or_higher_order_delaunay_materialized ||
      pair_matrix_materialized || public_status_claimed) {
    return false;
  }
  // The historical lower bound on facet-resolution scans holds for every
  // ordinary batch; only the terminal singleton-component batch legitimately
  // performs zero scans while still probing its full deletion shell.
  if (!terminal_component_image.has_value() &&
      source_facet_resolution_scan_count <
          projected_target_facet_probe_count / source_order) {
    return false;
  }
  std::size_t expected_prior_probe_count = 0U;
  std::size_t expected_target_probe_count = 0U;
  std::size_t expected_sparse_target_closure_count = 0U;
  for (std::size_t group_index = 0U;
       group_index < group_images.size();
       ++group_index) {
    const auto& image = group_images[group_index];
    if (image.owner_group_index != group_index ||
        !image.certified_conditional_group_image() ||
        !checked_add(
            expected_prior_probe_count,
            image.prior_root_witness_probe_count,
            expected_prior_probe_count) ||
        !checked_add(
            expected_target_probe_count,
            image.projected_target_facet_probe_count,
            expected_target_probe_count) ||
        !checked_add(
            expected_sparse_target_closure_count,
            image.sparse_target_closure_count,
            expected_sparse_target_closure_count)) {
      return false;
    }
  }
  for (std::size_t binding_index = 0U;
       binding_index < direct_saddle_group_bindings.size();
       ++binding_index) {
    const auto& binding = direct_saddle_group_bindings[binding_index];
    if (binding.binding_index != binding_index ||
        binding.source_hyperedge_index != binding_index ||
        binding.owner_group_index >= group_images.size() ||
        binding.group_image_index != binding.owner_group_index ||
        !binding.certified_conditional_saddle_group_binding() ||
        binding.resident_resultant_root_id !=
            group_images[binding.owner_group_index]
                .resident_resultant_source_root_id ||
        (binding_index != 0U &&
         direct_saddle_group_bindings[binding_index - 1U]
                 .source_direct_reference_index >=
             binding.source_direct_reference_index)) {
      return false;
    }
  }
  if (terminal_component_image.has_value()) {
    const auto& image = *terminal_component_image;
    if (!group_images.empty() || !direct_saddle_group_bindings.empty() ||
        frozen_hyperedge_count != 0U || frozen_token_reference_count != 0U ||
        frozen_quotient_group_count != 0U ||
        frozen_direct_saddle_hyperedge_count != 0U ||
        frozen_residual_hyperedge_count != 0U ||
        source_facet_resolution_scan_count != 0U ||
        image.source_batch_index != source_batch_index ||
        image.squared_level != squared_level ||
        image.source_order != source_order ||
        image.target_order != target_order ||
        !image.certified_conditional_terminal_component_image() ||
        !checked_add(
            expected_target_probe_count,
            image.target_deletion_probe_count,
            expected_target_probe_count) ||
        !checked_add(
            expected_sparse_target_closure_count,
            image.sparse_target_closure_count,
            expected_sparse_target_closure_count)) {
      return false;
    }
  }
  return expected_prior_probe_count == prior_root_witness_probe_count &&
         expected_target_probe_count == projected_target_facet_probe_count &&
         expected_sparse_target_closure_count ==
             sparse_target_closure_count &&
         sparse_target_closure_count <= projected_target_facet_probe_count &&
         canonical_exact_direct_morse_resident_higher_o4_membership_digest(
             *this) == o4_membership_digest;
}

bool ExactDirectMorseResidentAllOrdersVerticalFinalSeal::
    certified_final_vertical_seal() const noexcept {
  std::size_t expected_resident_group_count = 0U;
  std::size_t expected_final_target_reprobe_count = 0U;
  if (!checked_add(
          sealed_k2_group_count,
          sealed_higher_group_count,
          expected_resident_group_count) ||
      !checked_add(
          sealed_final_root_witness_probe_count,
          sealed_terminal_final_target_reprobe_count,
          expected_final_target_reprobe_count)) {
    return false;
  }
  const bool token_range_certified =
      expected_final_target_reprobe_count == 0U
          ? final_root_witness_query_replay_token_begin == 0U &&
                final_root_witness_query_replay_token_end == 0U
          : final_root_witness_query_replay_token_begin != 0U &&
                final_root_witness_query_replay_token_end >=
                    final_root_witness_query_replay_token_begin &&
                final_root_witness_query_replay_token_end -
                        final_root_witness_query_replay_token_begin ==
                    expected_final_target_reprobe_count - 1U;
  const bool source_facts_certified = normalized_source_facts_consistent(
      resident_source_kind,
      resident_normalized_direct_source_session,
      resident_normalized_h0_retraction_mode,
      resident_normalized_horizontal_incidence_reduction_certified);
  return schema_version ==
             direct_morse_resident_all_orders_vertical_bridge_schema_version &&
         sealed_stamp.schema_version ==
             direct_morse_resident_all_orders_vertical_bridge_schema_version &&
         sealed_stamp.bridge_session_instance_id != 0U &&
         sealed_stamp.resident_session_authority_id != 0U &&
         sealed_stamp.resident_batch_cursor == required_resident_batch_count &&
         sealed_stamp.committed_k2_batch_count == required_k2_batch_count &&
         sealed_stamp.committed_higher_batch_count ==
             required_higher_batch_count &&
         sealed_stamp.committed_k2_group_count == sealed_k2_group_count &&
         sealed_stamp.committed_higher_group_count ==
             sealed_higher_group_count &&
         sealed_stamp.committed_terminal_component_image_count ==
             sealed_terminal_component_image_count &&
         sealed_terminal_component_image_count <= 1U &&
         sealed_terminal_final_target_reprobe_count ==
             sealed_terminal_component_image_count &&
         (sealed_terminal_component_image_count == 0U
              ? sealed_terminal_target_deletion_probe_count == 0U &&
                    sealed_terminal_sparse_target_closure_count == 0U
              : sealed_terminal_target_deletion_probe_count >= 3U &&
                    sealed_terminal_target_deletion_probe_count <=
                        direct_sparse_positive_facet_maximum_point_count &&
                    sealed_terminal_sparse_target_closure_count == 0U) &&
         sealed_stamp.persistent_source_root_witness_count ==
             sealed_persistent_source_root_witness_count &&
         expected_resident_group_count == required_resident_group_count &&
         sealed_expected_projected_target_facet_probe_count ==
             sealed_projected_target_facet_probe_count &&
         sealed_sparse_target_closure_count <=
             sealed_projected_target_facet_probe_count &&
         sealed_final_root_witness_probe_count ==
             sealed_persistent_source_root_witness_count &&
         terminal_locator_stamp.external_authority_id ==
             sealed_stamp.resident_session_authority_id &&
         terminal_locator_stamp.committed_batch_count ==
             required_resident_batch_count &&
         terminal_locator_stamp.component_union_count ==
             sealed_terminal_locator_union_count &&
         sealed_terminal_locator_batch_count == required_resident_batch_count &&
         terminal_k1_session_instance_id != 0U &&
         pre_terminal_k1_level_cursor <= terminal_k1_level_cursor &&
         terminal_k1_level_cursor == distinct_k1_level_count &&
         sealed_target_only_k1_level_count ==
             terminal_k1_level_cursor - pre_terminal_k1_level_cursor &&
         k1_source_forest_digest != contract::CanonicalId{} &&
         terminal_k1_history_digest != contract::CanonicalId{} &&
         token_range_certified && source_facts_certified &&
         normalized_incidence_complete_v7_source_capability ==
             normalized_incidence_complete_v7_capability(
                 resident_source_kind,
                 resident_normalized_direct_source_session,
                 resident_normalized_h0_retraction_mode,
                 resident_normalized_horizontal_incidence_reduction_certified) &&
         owned_k1_terminal_advance_receipt_live_verified &&
         every_target_only_k1_level_consumed &&
         owned_k1_terminal_cursor_complete &&
         resident_cursor_exhausted && no_outstanding_prepared_ticket &&
         every_required_plan_batch_replayed_in_exact_product_order &&
         every_k2_batch_and_group_replayed_to_sealed_k1 &&
         every_higher_group_bound_to_one_live_adjacent_order_root &&
         every_terminal_component_bound_to_one_live_adjacent_order_root &&
         every_nonroot_source_key_projected_to_all_codimension_one_deletions &&
         every_resident_group_covered_exactly_once &&
         every_persistent_target_binding_reprobed_at_terminal_locator &&
         terminal_locator_monotone_union_history_owned_and_exhausted &&
         every_intermediate_target_fusion_composed_by_terminal_root_reprobe &&
         all_naturality_squares_replayed && vertical_maps_complete &&
         !global_morse_obligation_replayed && !m1_replayed &&
         !global_facet_coface_or_gamma_catalog_materialized &&
         !ordinary_or_higher_order_delaunay_materialized &&
         !pair_matrix_materialized && !public_status_claimed;
}

bool ExactDirectMorseResidentAllOrdersVerticalSealResult::
    certified_final_vertical_seal() const noexcept {
  const bool first_publication =
      final_seal_published && !existing_final_seal_returned_without_mutation;
  const bool idempotent_read =
      !final_seal_published && existing_final_seal_returned_without_mutation;
  return seal.has_value() && seal->certified_final_vertical_seal() &&
         (first_publication || idempotent_read) &&
         !no_partial_seal_published_on_failure &&
         decision ==
             ExactDirectMorseResidentAllOrdersVerticalSealDecision::
                 complete_certified_final_vertical_seal;
}

ExactDirectMorseResidentAllOrdersVerticalPreparedBatch::
    ExactDirectMorseResidentAllOrdersVerticalPreparedBatch() noexcept =
    default;
ExactDirectMorseResidentAllOrdersVerticalPreparedBatch::
    ~ExactDirectMorseResidentAllOrdersVerticalPreparedBatch() = default;
ExactDirectMorseResidentAllOrdersVerticalPreparedBatch::
    ExactDirectMorseResidentAllOrdersVerticalPreparedBatch(
        ExactDirectMorseResidentAllOrdersVerticalPreparedBatch&&) noexcept =
    default;
ExactDirectMorseResidentAllOrdersVerticalPreparedBatch&
ExactDirectMorseResidentAllOrdersVerticalPreparedBatch::operator=(
    ExactDirectMorseResidentAllOrdersVerticalPreparedBatch&&) noexcept =
    default;
ExactDirectMorseResidentAllOrdersVerticalPreparedBatch::
    ExactDirectMorseResidentAllOrdersVerticalPreparedBatch(
        std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectMorseResidentAllOrdersVerticalPreparedBatch::valid() const
    noexcept {
  return impl_ != nullptr && impl_->seal != nullptr &&
         impl_->owned_bridge_ticket.has_value() &&
         impl_->owned_bridge_ticket->valid() && !impl_->consumed &&
         (!impl_->higher_record.has_value() ||
          (impl_->higher_record->source_order >= 3U &&
           impl_->higher_record->resident_pre_batch_identity ==
               impl_->resident_identity &&
           !impl_->higher_record->resident_batch_committed));
}

bool ExactDirectMorseResidentAllOrdersVerticalPreparedBatch::
    higher_order_vertical_batch() const noexcept {
  return impl_ != nullptr && impl_->higher_record.has_value();
}

const ExactDirectMorseUnifiedSnapshotIdentity&
ExactDirectMorseResidentAllOrdersVerticalPreparedBatch::
    resident_pre_batch_identity() const noexcept {
  static const ExactDirectMorseUnifiedSnapshotIdentity empty{};
  return impl_ == nullptr ? empty : impl_->resident_identity;
}

const ExactDirectMorseUnifiedResidentAuthorityBundle&
ExactDirectMorseResidentAllOrdersVerticalPreparedBatch::
    resident_authority_bundle() const noexcept {
  static const ExactDirectMorseUnifiedResidentAuthorityBundle empty{};
  return impl_ == nullptr || !impl_->owned_bridge_ticket.has_value()
             ? empty
             : impl_->owned_bridge_ticket->resident_authority_bundle();
}

const ExactDirectMorseResidentAllOrdersVerticalBatchRecord*
ExactDirectMorseResidentAllOrdersVerticalPreparedBatch::
    conditional_batch_record() const noexcept {
  return impl_ == nullptr || !impl_->higher_record.has_value()
             ? nullptr
             : &*impl_->higher_record;
}

bool ExactDirectMorseResidentAllOrdersVerticalPreparationResult::
    certified_prepared_batch() const noexcept {
  const bool transit =
      decision ==
              ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                  complete_prepared_nonhigher_transit_batch &&
      nonhigher_transit_only &&
      !conditional_higher_group_images_prepared;
  const bool higher =
      decision ==
              ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                  complete_prepared_conditional_higher_vertical_batch &&
      !nonhigher_transit_only && conditional_higher_group_images_prepared;
  return ticket.has_value() && ticket->valid() && (transit || higher) &&
         no_resident_or_outer_vertical_scientific_state_mutated_on_failure;
}

bool ExactDirectMorseResidentAllOrdersVerticalCommitResult::
    certified_committed_batch() const noexcept {
  const bool transit =
      decision ==
              ExactDirectMorseResidentAllOrdersVerticalCommitDecision::
                  complete_committed_nonhigher_transit_batch &&
      !outer_vertical_state_mutated &&
      !post_resident_commit_publication_allocation_free;
  const bool higher =
      decision ==
              ExactDirectMorseResidentAllOrdersVerticalCommitDecision::
                  complete_committed_conditional_higher_vertical_batch &&
      outer_vertical_state_mutated &&
      post_resident_commit_publication_allocation_free;
  return ticket_consumed && owned_bridge_commit.certified_committed_batch() &&
         !no_outer_vertical_state_mutated_on_owned_bridge_rejection &&
         (transit || higher);
}

namespace {

[[nodiscard]] TargetProbeStatus probe_target_key(
    const ExactDirectMorseResidentAllOrdersVerticalBridge::Impl& impl,
    const ExactDirectSparseFacetKey& key,
    std::size_t target_order,
    const exact::ExactLevel& closed_squared_level,
    bool allow_sparse_target_closure,
    const std::optional<ExactDirectSparseFacetWitness>&
        required_source_binding_witness,
    const ExactDirectSparsePositiveFacetLocatorSnapshotStamp& expected_stamp,
    std::uint64_t& next_query_replay_token,
    TargetProbePayload& output) noexcept {
  const auto& locator = impl.owned_bridge.resident_locator();
  const auto& plan = impl.owned_bridge.resident_plan();
  const auto& components = impl.owned_bridge.resident_component_states();
  const auto& roots = impl.owned_bridge.resident_root_coverages();
  if (!canonical_facet_key(key, target_order, plan.point_count) ||
      locator.snapshot_stamp() != expected_stamp ||
      next_query_replay_token == 0U ||
      next_query_replay_token > impl.budget.maximum_query_replay_token) {
    return next_query_replay_token == 0U ||
                   next_query_replay_token >
                       impl.budget.maximum_query_replay_token
               ? TargetProbeStatus::budget_exhausted
               : TargetProbeStatus::locator_rejected;
  }
  const ExactDirectSparseFacetWitness query_witness{
      impl.seal->resident_session_authority_id,
      next_query_replay_token};
  next_query_replay_token =
      next_query_replay_token == std::numeric_limits<std::uint64_t>::max()
          ? 0U
          : next_query_replay_token + 1U;
  const auto probe = locator.probe_positive_facet(
      key, query_witness, impl.budget.target_probe);
  const auto verification = verify_exact_direct_sparse_positive_facet_probe(
      locator,
      key,
      query_witness,
      impl.budget.target_probe,
      probe);
  if (!verification.result_certified ||
      locator.snapshot_stamp() != expected_stamp) {
    return probe.certified_budget_exhaustion()
               ? TargetProbeStatus::budget_exhausted
               : TargetProbeStatus::locator_rejected;
  }

  std::size_t handle = 0U;
  ExactDirectSparseFacetKey canonical_target_key{};
  ExactDirectSparseFacetWitness source_binding_witness{};
  bool sparse_target_closure_used = false;
  if (probe.certified_positive_hit()) {
    if (!probe.component_handle_present ||
        !probe.source_binding_witness_present ||
        (required_source_binding_witness.has_value() &&
         probe.source_binding_witness != *required_source_binding_witness)) {
      return TargetProbeStatus::locator_rejected;
    }
    handle = probe.component_handle;
    canonical_target_key = key;
    source_binding_witness = probe.source_binding_witness;
  } else {
    if (!allow_sparse_target_closure ||
        required_source_binding_witness.has_value() ||
        !probe.certified_unresolved_miss()) {
      return probe.certified_budget_exhaustion()
                 ? TargetProbeStatus::budget_exhausted
                 : TargetProbeStatus::locator_rejected;
    }
    const std::array canonical_distinct_keys{key};
    const auto closure =
        impl.owned_bridge.build_resident_sparse_facet_descent_closure(
            canonical_distinct_keys,
            closed_squared_level,
            query_witness,
            impl.budget.target_closure,
            impl.budget.target_closure_config,
            impl.budget.target_closure_traversal_order);
    if (!closure.certified_complete_relative_positive_closure()) {
      return closure.certified_budget_exhaustion()
                 ? TargetProbeStatus::budget_exhausted
                 : TargetProbeStatus::locator_rejected;
    }
    if (closure.locator_snapshot_stamp != expected_stamp ||
        closure.closed_batch_squared_level != closed_squared_level ||
        closure.locator_query_witness != query_witness ||
        closure.common_facet_cardinality != target_order ||
        closure.seed_projections.size() != 1U || closure.nodes.empty()) {
      return TargetProbeStatus::locator_rejected;
    }
    const auto& projection = closure.seed_projections.front();
    if (projection.seed_index != 0U ||
        projection.source_facet_key != key ||
        projection.closure_disposition !=
            ExactDirectSparseFacetDescentClosureDisposition::relative_positive ||
        projection.root_node_index >= closure.nodes.size() ||
        projection.terminal_node_index >= closure.nodes.size() ||
        closure.nodes[projection.root_node_index].facet_key != key) {
      return TargetProbeStatus::locator_rejected;
    }
    const auto& terminal = closure.nodes[projection.terminal_node_index];
    if (terminal.kind !=
            ExactDirectSparseFacetDescentNodeKind::positive_locator_terminal ||
        terminal.closure_disposition !=
            ExactDirectSparseFacetDescentClosureDisposition::relative_positive ||
        !terminal.resolved_component_handle.has_value() ||
        !terminal.resolved_binding_witness.has_value() ||
        !canonical_facet_key(
            terminal.facet_key, target_order, plan.point_count)) {
      return TargetProbeStatus::locator_rejected;
    }
    handle = *terminal.resolved_component_handle;
    canonical_target_key = terminal.facet_key;
    source_binding_witness = *terminal.resolved_binding_witness;
    sparse_target_closure_used = true;
  }
  if (source_binding_witness.external_authority_id !=
          impl.seal->resident_session_authority_id ||
      source_binding_witness.replay_token == 0U ||
      locator.snapshot_stamp() != expected_stamp) {
    return TargetProbeStatus::locator_rejected;
  }
  if (handle >= components.size() || handle >= plan.facet_tokens.size()) {
    return TargetProbeStatus::target_not_rooted;
  }
  const auto& component = components[handle];
  if (component.component_handle != handle ||
      component.parent_handle != handle || !component.active ||
      !component.root_id.has_value() || *component.root_id == 0U ||
      plan.facet_tokens[handle].facet_key.point_count != target_order ||
      find_root_coverage(roots, *component.root_id) == nullptr) {
    return TargetProbeStatus::target_not_rooted;
  }
  output.target_root_id = *component.root_id;
  output.canonical_target_key = canonical_target_key;
  output.source_binding_witness = source_binding_witness;
  output.sparse_target_closure_used = sparse_target_closure_used;
  return TargetProbeStatus::complete;
}

[[nodiscard]] bool resident_group_matches_expected(
    const ExactDirectMorseUnifiedResidentGroupRecord& actual,
    const ExactDirectMorseUnifiedResidentGroupRecord& expected) noexcept {
  return actual.group_record_index == expected.group_record_index &&
         actual.batch_index == expected.batch_index &&
         actual.owner_group_index == expected.owner_group_index &&
         actual.squared_level == expected.squared_level &&
         actual.order == expected.order && actual.q_r == expected.q_r &&
         actual.action == expected.action &&
         actual.resultant_root_id != 0U &&
         actual.child_root_ids == expected.child_root_ids &&
         actual.coverage_delta_points == expected.coverage_delta_points &&
         actual.empty_coverage_delta == expected.empty_coverage_delta;
}

}  // namespace

ExactDirectMorseResidentAllOrdersVerticalBridge::
    ExactDirectMorseResidentAllOrdersVerticalBridge() noexcept = default;
ExactDirectMorseResidentAllOrdersVerticalBridge::
    ~ExactDirectMorseResidentAllOrdersVerticalBridge() = default;
ExactDirectMorseResidentAllOrdersVerticalBridge::
    ExactDirectMorseResidentAllOrdersVerticalBridge(
        ExactDirectMorseResidentAllOrdersVerticalBridge&&) noexcept = default;
ExactDirectMorseResidentAllOrdersVerticalBridge&
ExactDirectMorseResidentAllOrdersVerticalBridge::operator=(
    ExactDirectMorseResidentAllOrdersVerticalBridge&&) noexcept = default;
ExactDirectMorseResidentAllOrdersVerticalBridge::
    ExactDirectMorseResidentAllOrdersVerticalBridge(
        std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectMorseResidentAllOrdersVerticalBridge::ready() const noexcept {
  return impl_ != nullptr && impl_->structurally_ready();
}

bool ExactDirectMorseResidentAllOrdersVerticalBridge::resident_complete() const
    noexcept {
  return ready() && impl_->owned_bridge.resident_complete();
}

bool ExactDirectMorseResidentAllOrdersVerticalBridge::
    conditional_all_adjacent_order_group_images_complete() const noexcept {
  if (!resident_complete() ||
      impl_->committed_stamp.committed_k2_batch_count !=
          impl_->required_k2_batch_count ||
      impl_->committed_stamp.committed_higher_batch_count !=
          impl_->required_higher_batch_count) {
    return false;
  }
  std::array<bool, direct_sparse_positive_facet_maximum_point_count + 1U>
      planned_source_order{};
  std::array<bool, direct_sparse_positive_facet_maximum_point_count + 1U>
      imaged_source_order{};
  for (const auto& batch : impl_->owned_bridge.resident_plan().batches) {
    if (batch.order < 2U ||
        batch.order > direct_sparse_positive_facet_maximum_point_count) {
      return false;
    }
    planned_source_order[batch.order] = true;
  }
  for (const auto& batch : impl_->owned_bridge.committed_k2_batches()) {
    imaged_source_order[2U] =
        imaged_source_order[2U] || !batch.group_images.empty();
  }
  for (const auto& batch : impl_->committed_higher_batches) {
    if (batch.source_order < 3U ||
        batch.source_order > direct_sparse_positive_facet_maximum_point_count) {
      return false;
    }
    imaged_source_order[batch.source_order] =
        imaged_source_order[batch.source_order] || !batch.group_images.empty();
  }
  for (std::size_t order = 2U;
       order <= direct_sparse_positive_facet_maximum_point_count;
       ++order) {
    if (planned_source_order[order] && !imaged_source_order[order]) {
      return false;
    }
  }
  return true;
}

bool ExactDirectMorseResidentAllOrdersVerticalBridge::
    conditional_all_adjacent_order_component_images_complete() const
    noexcept {
  if (!resident_complete() ||
      impl_->committed_stamp.committed_k2_batch_count !=
          impl_->required_k2_batch_count ||
      impl_->committed_stamp.committed_higher_batch_count !=
          impl_->required_higher_batch_count) {
    return false;
  }
  std::array<bool, direct_sparse_positive_facet_maximum_point_count + 1U>
      planned_source_order{};
  std::array<bool, direct_sparse_positive_facet_maximum_point_count + 1U>
      imaged_source_order{};
  const std::size_t point_count = impl_->owned_bridge.resident_plan().point_count;
  for (const auto& batch : impl_->owned_bridge.resident_plan().batches) {
    if (batch.order < 2U ||
        batch.order > direct_sparse_positive_facet_maximum_point_count) {
      return false;
    }
    planned_source_order[batch.order] = true;
  }
  for (const auto& batch : impl_->owned_bridge.committed_k2_batches()) {
    imaged_source_order[2U] =
        imaged_source_order[2U] || !batch.group_images.empty();
  }
  std::size_t terminal_component_image_count = 0U;
  for (const auto& batch : impl_->committed_higher_batches) {
    if (batch.source_order < 3U ||
        batch.source_order > direct_sparse_positive_facet_maximum_point_count) {
      return false;
    }
    imaged_source_order[batch.source_order] =
        imaged_source_order[batch.source_order] || !batch.group_images.empty();
    if (batch.terminal_component_image.has_value()) {
      if (batch.source_order != point_count ||
          !batch.terminal_component_image
               ->certified_conditional_terminal_component_image() ||
          terminal_component_image_count ==
              std::numeric_limits<std::size_t>::max()) {
        return false;
      }
      ++terminal_component_image_count;
      imaged_source_order[batch.source_order] = true;
    }
  }
  if (terminal_component_image_count !=
          impl_->committed_stamp.committed_terminal_component_image_count ||
      terminal_component_image_count > 1U) {
    return false;
  }
  for (std::size_t order = 2U;
       order <= direct_sparse_positive_facet_maximum_point_count;
       ++order) {
    if (planned_source_order[order] && !imaged_source_order[order]) {
      return false;
    }
  }
  return true;
}

ExactDirectMorseUnifiedResidentSourceKind
ExactDirectMorseResidentAllOrdersVerticalBridge::resident_source_kind() const
    noexcept {
  return impl_ == nullptr
             ? ExactDirectMorseUnifiedResidentSourceKind::
                   successive_incidence_star
             : impl_->owned_bridge.resident_source_kind();
}

bool ExactDirectMorseResidentAllOrdersVerticalBridge::
    resident_normalized_direct_source_session() const noexcept {
  return impl_ != nullptr &&
         impl_->owned_bridge.resident_normalized_direct_source_session();
}

ExactDirectNormalizedH0ResidentRetractionMode
ExactDirectMorseResidentAllOrdersVerticalBridge::
    resident_normalized_h0_retraction_mode() const noexcept {
  return impl_ == nullptr
             ? ExactDirectNormalizedH0ResidentRetractionMode::
                   not_applicable_successive_incidence_star
             : impl_->owned_bridge.resident_normalized_h0_retraction_mode();
}

bool ExactDirectMorseResidentAllOrdersVerticalBridge::
    resident_normalized_horizontal_incidence_reduction_certified()
        const noexcept {
  return impl_ != nullptr &&
         impl_->owned_bridge
             .resident_normalized_horizontal_incidence_reduction_certified();
}

bool ExactDirectMorseResidentAllOrdersVerticalBridge::
    normalized_incidence_complete_v7_source_capability() const noexcept {
  return final_vertical_sealed() &&
         normalized_incidence_complete_v7_capability(
             impl_->owned_bridge.resident_source_kind(),
             impl_->owned_bridge.resident_normalized_direct_source_session(),
             impl_->owned_bridge.resident_normalized_h0_retraction_mode(),
             impl_->owned_bridge
                 .resident_normalized_horizontal_incidence_reduction_certified()) &&
         impl_->final_vertical_seal
             ->normalized_incidence_complete_v7_source_capability;
}

ExactDirectK1BoruvkaClosedCutSessionStamp
ExactDirectMorseResidentAllOrdersVerticalBridge::owned_k1_current_stamp()
    const {
  if (!ready()) {
    throw std::logic_error("the all-orders resident bridge is not ready");
  }
  return impl_->owned_bridge.owned_k1_current_stamp();
}

contract::CanonicalId
ExactDirectMorseResidentAllOrdersVerticalBridge::
    owned_k1_source_forest_digest() const noexcept {
  return impl_ == nullptr ? contract::CanonicalId{}
                          : impl_->owned_bridge.owned_k1_source_forest_digest();
}

bool ExactDirectMorseResidentAllOrdersVerticalBridge::
    verify_owned_k1_source_forest_digest(
        const contract::CanonicalId& digest) const noexcept {
  return impl_ != nullptr &&
         impl_->owned_bridge.verify_owned_k1_source_forest_digest(digest);
}

std::size_t ExactDirectMorseResidentAllOrdersVerticalBridge::
    owned_k1_distinct_level_count() const noexcept {
  return impl_ == nullptr
             ? 0U
             : impl_->owned_bridge.owned_k1_distinct_level_count();
}

bool ExactDirectMorseResidentAllOrdersVerticalBridge::
    owned_k1_terminal_complete() const noexcept {
  return impl_ != nullptr && impl_->owned_bridge.owned_k1_terminal_complete();
}

ExactDirectMorseResidentAllOrdersVerticalBridgeStamp
ExactDirectMorseResidentAllOrdersVerticalBridge::current_stamp() const {
  if (!ready()) {
    throw std::logic_error("the all-orders resident bridge is not ready");
  }
  return impl_->committed_stamp;
}

bool ExactDirectMorseResidentAllOrdersVerticalBridge::verify_stamp(
    const ExactDirectMorseResidentAllOrdersVerticalBridgeStamp& stamp) const
    noexcept {
  return ready() && stamp == impl_->committed_stamp;
}

bool ExactDirectMorseResidentAllOrdersVerticalBridge::final_vertical_sealed()
    const noexcept {
  return ready() && impl_->final_vertical_seal.has_value() &&
         impl_->final_vertical_seal->certified_final_vertical_seal();
}

const ExactDirectMorseResidentAllOrdersVerticalFinalSeal*
ExactDirectMorseResidentAllOrdersVerticalBridge::final_vertical_seal() const
    noexcept {
  return impl_ == nullptr || !impl_->final_vertical_seal.has_value()
             ? nullptr
             : &*impl_->final_vertical_seal;
}

bool ExactDirectMorseResidentAllOrdersVerticalBridge::
    verify_final_vertical_seal(
        const ExactDirectMorseResidentAllOrdersVerticalFinalSeal& seal) const
    noexcept {
  return final_vertical_sealed() && seal == *impl_->final_vertical_seal;
}

const std::vector<ExactDirectMorseResidentAllOrdersVerticalRootWitness>&
ExactDirectMorseResidentAllOrdersVerticalBridge::
    source_root_target_witnesses() const noexcept {
  static const std::vector<
      ExactDirectMorseResidentAllOrdersVerticalRootWitness>
      empty;
  return impl_ == nullptr ? empty : impl_->source_root_target_witnesses;
}

const std::vector<ExactDirectMorseResidentAllOrdersVerticalBatchRecord>&
ExactDirectMorseResidentAllOrdersVerticalBridge::committed_higher_batches()
    const noexcept {
  static const std::vector<ExactDirectMorseResidentAllOrdersVerticalBatchRecord>
      empty;
  return impl_ == nullptr ? empty : impl_->committed_higher_batches;
}

const std::vector<ExactDirectMorseResidentK2K1ClosedCutBatchRecord>&
ExactDirectMorseResidentAllOrdersVerticalBridge::committed_k2_batches() const
    noexcept {
  static const std::vector<ExactDirectMorseResidentK2K1ClosedCutBatchRecord>
      empty;
  return impl_ == nullptr ? empty : impl_->owned_bridge.committed_k2_batches();
}

const ExactDirectSparseUnifiedLevelPlanResult&
ExactDirectMorseResidentAllOrdersVerticalBridge::resident_plan() const noexcept {
  static const ExactDirectSparseUnifiedLevelPlanResult empty{};
  return impl_ == nullptr ? empty : impl_->owned_bridge.resident_plan();
}

const ExactDirectSparsePositiveFacetLocator&
ExactDirectMorseResidentAllOrdersVerticalBridge::resident_locator() const
    noexcept {
  static const ExactDirectSparsePositiveFacetLocator empty{};
  return impl_ == nullptr ? empty : impl_->owned_bridge.resident_locator();
}

const std::vector<ExactDirectMorseUnifiedResidentComponentState>&
ExactDirectMorseResidentAllOrdersVerticalBridge::resident_component_states()
    const noexcept {
  static const std::vector<ExactDirectMorseUnifiedResidentComponentState> empty;
  return impl_ == nullptr ? empty
                          : impl_->owned_bridge.resident_component_states();
}

const std::vector<ExactDirectMorseUnifiedResidentRootCoverage>&
ExactDirectMorseResidentAllOrdersVerticalBridge::resident_root_coverages() const
    noexcept {
  static const std::vector<ExactDirectMorseUnifiedResidentRootCoverage> empty;
  return impl_ == nullptr ? empty
                          : impl_->owned_bridge.resident_root_coverages();
}

const std::vector<ExactDirectMorseUnifiedResidentGroupRecord>&
ExactDirectMorseResidentAllOrdersVerticalBridge::resident_group_records() const
    noexcept {
  static const std::vector<ExactDirectMorseUnifiedResidentGroupRecord> empty;
  return impl_ == nullptr ? empty
                          : impl_->owned_bridge.resident_group_records();
}

ExactDirectMorseResidentAllOrdersVerticalPreparationResult
ExactDirectMorseResidentAllOrdersVerticalBridge::prepare_next() {
  ExactDirectMorseResidentAllOrdersVerticalPreparationResult output;
  output.no_resident_or_outer_vertical_scientific_state_mutated_on_failure =
      true;
  const auto reject = [&output](
                          ExactDirectMorseResidentAllOrdersVerticalPreparationDecision
                              decision) {
    return ExactDirectMorseResidentAllOrdersVerticalPreparationResult{
        std::nullopt,
        output.requirements,
        output
            .no_resident_or_outer_vertical_scientific_state_mutated_on_failure,
        output.owned_k1_lower_cut_may_have_advanced_without_outer_publication,
        false,
        false,
        decision};
  };
  if (!ready() || impl_->final_vertical_seal.has_value()) {
    return reject(
        ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
            no_bridge_not_ready);
  }

  auto owned_preparation = impl_->owned_bridge.prepare_next();
  output.owned_k1_lower_cut_may_have_advanced_without_outer_publication =
      owned_preparation.k1_lower_cut_may_have_advanced_without_vertical_publication;
  if (!owned_preparation.certified_prepared_batch() ||
      !owned_preparation.ticket.has_value()) {
    return reject(
        ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
            no_owned_bridge_preparation_rejected);
  }
  const auto& bundle =
      owned_preparation.ticket->resident_authority_bundle();
  const auto locator_stamp = impl_->owned_bridge.resident_locator().snapshot_stamp();
  if (!bundle.certified_strict_pre_batch_bundle() ||
      bundle.identity.session_authority_id !=
          impl_->seal->resident_session_authority_id ||
      bundle.identity.locator_instance_id == 0U ||
      (impl_->committed_stamp.resident_locator_instance_id != 0U &&
       bundle.identity.locator_instance_id !=
           impl_->committed_stamp.resident_locator_instance_id) ||
      bundle.identity.batch_cursor !=
          impl_->committed_stamp.resident_batch_cursor ||
      bundle.identity.epoch != impl_->committed_stamp.resident_epoch ||
      bundle.identity.locator_stamp != locator_stamp ||
      bundle.identity.source_pair_canonical_cloud_digest !=
          impl_->seal->canonical_cloud_digest ||
      bundle.identity.source_higher_canonical_cloud_digest !=
          impl_->seal->resident_higher_canonical_cloud_digest ||
      bundle.source_batch_index != bundle.identity.batch_cursor) {
    return reject(
        ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
            no_resident_pre_batch_identity_rejected);
  }

  output.requirements.resident_batch_count =
      impl_->owned_bridge.resident_plan().batches.size();
  output.requirements.resident_k2_batch_count = impl_->required_k2_batch_count;
  output.requirements.resident_higher_batch_count =
      impl_->required_higher_batch_count;
  if (!count_higher_plan_saddles(
          impl_->owned_bridge.resident_plan(),
          output.requirements.resident_higher_direct_saddle_reference_count)) {
    return reject(
        ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
            no_resident_pre_batch_identity_rejected);
  }
  output.requirements.persistent_source_root_witness_count_before =
      impl_->source_root_target_witnesses.size();

  try {
    auto prepared = std::make_unique<
        ExactDirectMorseResidentAllOrdersVerticalPreparedBatch::Impl>();
    prepared->seal = impl_->seal;
    prepared->ticket_registry = impl_->ticket_registry;
    prepared->resident_identity = bundle.identity;
    prepared->resident_group_record_count_before =
        impl_->owned_bridge.resident_group_records().size();
    prepared->next_query_replay_token =
        impl_->committed_stamp.next_query_replay_token;
    prepared->owned_bridge_ticket.emplace(
        std::move(*owned_preparation.ticket));

    if (bundle.order < 3U) {
      if (impl_->ticket_registry->live_ticket_count ==
          std::numeric_limits<std::size_t>::max()) {
        return reject(
            ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                no_preparation_budget_exhausted);
      }
      ++impl_->ticket_registry->live_ticket_count;
      prepared->owns_ticket_slot = true;
      output.ticket.emplace(
          ExactDirectMorseResidentAllOrdersVerticalPreparedBatch{
              std::move(prepared)});
      output.nonhigher_transit_only = true;
      output.decision =
          ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
              complete_prepared_nonhigher_transit_batch;
      return output;
    }

    const auto& plan = impl_->owned_bridge.resident_plan();
    const auto& frozen = bundle.frozen_batch;
    const auto& actions = frozen.action_plan;
    const auto& quotient = frozen.quotient;
    AuthenticHigherDirectMembershipCounts direct_counts;
    const std::size_t group_count = actions.groups.size();
    if (!inspect_authentic_higher_direct_membership(
            plan, bundle, direct_counts)) {
      return reject(
          ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
            no_frozen_group_shape_rejected);
    }
    AuthenticTerminalComponentSource terminal_source;
    const bool terminal_component_batch =
        bundle.order == plan.point_count && group_count == 0U;
    if (terminal_component_batch &&
        (impl_->committed_stamp.committed_terminal_component_image_count !=
             0U ||
         !inspect_authentic_terminal_component_source(
             plan, bundle, direct_counts, terminal_source))) {
      return reject(
          ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
              no_frozen_group_shape_rejected);
    }
    if (quotient.groups.size() != group_count ||
        frozen.coverage_deltas.size() != group_count ||
        bundle.order > direct_sparse_positive_facet_maximum_point_count ||
        bundle.order > impl_->owned_bridge.resident_plan().point_count ||
        impl_->committed_higher_batches.size() >=
            impl_->budget.maximum_committed_higher_batch_count ||
        group_count > impl_->budget.maximum_prepared_higher_group_count ||
        direct_counts.saddle_count >
            impl_->budget
                .maximum_prepared_higher_direct_saddle_group_binding_count) {
      return reject(
          ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
              no_preparation_budget_exhausted);
    }

    std::size_t next_committed_group_count = 0U;
    std::size_t next_committed_saddle_binding_count = 0U;
    std::size_t prior_root_probe_count = 0U;
    std::size_t group_child_count = 0U;
    std::size_t group_delta_point_count = 0U;
    std::size_t new_root_witness_count = 0U;
    if (!checked_add(
            impl_->committed_stamp.committed_higher_group_count,
            group_count,
            next_committed_group_count) ||
        next_committed_group_count >
            impl_->budget.maximum_committed_higher_group_count ||
        !checked_add(
            impl_->committed_stamp
                .committed_higher_direct_saddle_group_binding_count,
            direct_counts.saddle_count,
            next_committed_saddle_binding_count) ||
        next_committed_saddle_binding_count >
            impl_->budget
                .maximum_committed_higher_direct_saddle_group_binding_count) {
      return reject(
          ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
              no_preparation_budget_exhausted);
    }
    for (std::size_t group_index = 0U;
         group_index < group_count;
         ++group_index) {
      const auto& action_group = actions.groups[group_index];
      const auto& quotient_group = quotient.groups[group_index];
      const auto& delta = frozen.coverage_deltas[group_index];
      if (action_group.group_index != group_index ||
          quotient_group.group_index != group_index ||
          delta.coverage_delta_record_index != group_index ||
          delta.owner_group_index != group_index ||
          action_group.q_r != action_group.prior_root_count ||
          action_group.q_r != delta.q_r ||
          action_group.action != delta.action ||
          !valid_slice(
              action_group.prior_root_offset,
              action_group.prior_root_count,
              actions.prior_root_ids.size()) ||
          !valid_slice(
              quotient_group.token_offset,
              quotient_group.token_count,
              quotient.group_tokens.size()) ||
          !valid_slice(
              delta.point_reference_offset,
              delta.point_reference_count,
              frozen.coverage_delta_points.size()) ||
          !checked_add(
              prior_root_probe_count,
              action_group.prior_root_count,
              prior_root_probe_count) ||
          !checked_add(
              group_child_count,
              action_group.prior_root_count,
              group_child_count) ||
          !checked_add(
              group_delta_point_count,
              delta.point_reference_count,
              group_delta_point_count)) {
        return reject(
            ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                no_frozen_group_shape_rejected);
      }
      if (action_group.q_r != 1U) {
        if (new_root_witness_count ==
            std::numeric_limits<std::size_t>::max()) {
          return reject(
              ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                  no_preparation_budget_exhausted);
        }
        ++new_root_witness_count;
      }
    }

    std::size_t source_resolution_scan_count = 0U;
    std::size_t projected_target_probe_count = 0U;
    if (!checked_twice(
            bundle.facet_resolutions.size(),
            source_resolution_scan_count)) {
      return reject(
          ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
              no_preparation_budget_exhausted);
    }
    for (const auto& resolution : bundle.facet_resolutions) {
      if (resolution.facet_token_index >=
              impl_->owned_bridge.resident_plan().facet_tokens.size() ||
          (resolution.token.kind ==
           ExactFrozenIncidenceTokenKind::rooted_carrier) !=
              resolution.prior_root_id.has_value()) {
        return reject(
            ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                no_frozen_group_shape_rejected);
      }
      if (resolution.token.kind ==
          ExactFrozenIncidenceTokenKind::rooted_carrier) {
        continue;
      }
      const auto* binding = find_token_binding(quotient, resolution.token);
      const auto& source_key = impl_->owned_bridge.resident_plan()
                                   .facet_tokens[resolution.facet_token_index]
                                   .facet_key;
      if (binding == nullptr || binding->group_index >= group_count ||
          !canonical_facet_key(
              source_key,
              bundle.order,
              impl_->owned_bridge.resident_plan().point_count) ||
          !checked_add(
              projected_target_probe_count,
              bundle.order,
              projected_target_probe_count)) {
        return reject(
            ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
              no_frozen_group_shape_rejected);
      }
    }
    if (terminal_component_batch &&
        !checked_add(
            projected_target_probe_count,
            bundle.order,
            projected_target_probe_count)) {
      return reject(
          ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
              no_preparation_budget_exhausted);
    }

    std::size_t persistent_witness_count_after = 0U;
    std::size_t total_query_count = 0U;
    if (!checked_add(
            impl_->source_root_target_witnesses.size(),
            new_root_witness_count,
            persistent_witness_count_after) ||
        !checked_add(
            prior_root_probe_count,
            projected_target_probe_count,
            total_query_count) ||
        persistent_witness_count_after >
            impl_->budget.maximum_persistent_source_root_witness_count ||
        prior_root_probe_count >
            impl_->budget.maximum_prior_root_witness_probe_count ||
        source_resolution_scan_count >
            impl_->budget.maximum_source_facet_resolution_scan_count ||
        projected_target_probe_count >
            impl_->budget.maximum_projected_target_facet_probe_count ||
        projected_target_probe_count >
            impl_->budget.maximum_sparse_target_closure_count ||
        group_child_count >
            impl_->budget.maximum_expected_group_child_root_reference_count ||
        group_delta_point_count >
            impl_->budget
                .maximum_expected_group_coverage_delta_point_reference_count ||
        (total_query_count != 0U &&
         (prepared->next_query_replay_token == 0U ||
          prepared->next_query_replay_token >
              impl_->budget.maximum_query_replay_token ||
          total_query_count - 1U >
              impl_->budget.maximum_query_replay_token -
                  prepared->next_query_replay_token))) {
      return reject(
          ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
              no_preparation_budget_exhausted);
    }

    output.requirements.prepared_higher_group_count = group_count;
    output.requirements.prepared_higher_direct_saddle_group_binding_count =
        direct_counts.saddle_count;
    output.requirements.persistent_source_root_witness_count_after =
        persistent_witness_count_after;
    output.requirements.prior_root_witness_probe_count =
        prior_root_probe_count;
    output.requirements.source_facet_resolution_scan_count =
        source_resolution_scan_count;
    output.requirements.projected_target_facet_probe_count =
        projected_target_probe_count;
    output.requirements.expected_group_child_root_reference_count =
        group_child_count;
    output.requirements
        .expected_group_coverage_delta_point_reference_count =
        group_delta_point_count;

    prepared->higher_record.emplace();
    auto& record = *prepared->higher_record;
    record.resident_pre_batch_identity = bundle.identity;
    record.source_batch_index = bundle.source_batch_index;
    record.squared_level = bundle.squared_level;
    record.source_order = bundle.order;
    record.target_order = bundle.order - 1U;
    record.frozen_hyperedge_count = frozen.counters.hyperedge_count;
    record.frozen_token_reference_count =
        frozen.counters.token_reference_count;
    record.frozen_quotient_group_count = frozen.counters.group_count;
    record.frozen_direct_saddle_hyperedge_count =
        frozen.counters.direct_saddle_hyperedge_count;
    record.frozen_residual_hyperedge_count =
        frozen.counters.residual_hyperedge_count;
    record.prior_root_witness_probe_count = prior_root_probe_count;
    record.source_facet_resolution_scan_count =
        source_resolution_scan_count;
    record.projected_target_facet_probe_count =
        projected_target_probe_count;
    record.group_images.resize(group_count);
    if (!populate_authentic_higher_saddle_membership(
            plan,
            bundle,
            direct_counts,
            record.direct_saddle_group_bindings)) {
      return reject(
          ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
              no_frozen_group_shape_rejected);
    }
    record.every_direct_saddle_bound_exactly_once = true;
    record.bindings_replayed_against_frozen_hyperedge_quotient = true;
    record.all_residual_hyperedges_consumed_in_same_quotient = true;
    record.exact_product_order_target_batch_already_committed = true;
    prepared->expected_resident_groups.resize(group_count);
    prepared->continuation_witness_indices.assign(
        group_count, std::numeric_limits<std::size_t>::max());
    prepared->prior_witness_reprobe_indices.reserve(prior_root_probe_count);
    prepared->new_source_root_witness_count = new_root_witness_count;
    std::vector<GroupAccumulator> accumulators(group_count);

    if (terminal_component_batch) {
      record.terminal_component_image.emplace();
      auto& image = *record.terminal_component_image;
      image.source_batch_index = bundle.source_batch_index;
      image.source_direct_reference_index =
          terminal_source.direct_reference_index;
      image.source_role_record_index = terminal_source.role_record_index;
      image.source_event_projection_index =
          terminal_source.event_projection_index;
      image.squared_level = bundle.squared_level;
      image.source_order = bundle.order;
      image.target_order = record.target_order;
      image.complete_source_facet_key =
          terminal_source.complete_source_facet_key;
      GroupAccumulator terminal_accumulator;
      for (std::size_t erased_index = 0U;
           erased_index < image.complete_source_facet_key.point_count;
           ++erased_index) {
        const auto target_key = erase_source_point(
            image.complete_source_facet_key, erased_index);
        TargetProbePayload probe;
        const auto status = probe_target_key(
            *impl_,
            target_key,
            record.target_order,
            record.squared_level,
            false,
            std::nullopt,
            locator_stamp,
            prepared->next_query_replay_token,
            probe);
        if (status != TargetProbeStatus::complete) {
          return reject(
              status == TargetProbeStatus::budget_exhausted
                  ? ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                        no_preparation_budget_exhausted
                  : status == TargetProbeStatus::target_not_rooted
                        ? ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                              no_target_component_not_rooted
                        : ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                              no_target_facet_locator_probe_rejected);
        }
        if (!add_group_candidate(
                terminal_accumulator, probe.canonical_target_key, probe)) {
          return reject(
              ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                  no_group_target_root_conflict);
        }
        ++image.target_deletion_probe_count;
      }
      if (!terminal_accumulator.initialized ||
          image.target_deletion_probe_count != bundle.order) {
        return reject(
            ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                no_frozen_group_shape_rejected);
      }
      image.canonical_target_facet_key =
          terminal_accumulator.canonical_target_key;
      image.target_source_binding_witness =
          terminal_accumulator.canonical_source_binding_witness;
      image.resolved_target_root_id = terminal_accumulator.target_root_id;
      image.source_order_equals_cloud_point_count = true;
      image.exactly_one_terminal_birth_reference = true;
      image.frozen_terminal_batch_has_no_hyperedge_or_group = true;
      image.every_codimension_one_deletion_probed = true;
      image.every_target_deletion_live_positive_and_rooted = true;
      image.every_target_deletion_direct_positive_hit = true;
      image.one_live_target_root_for_complete_terminal_component = true;
      record.sparse_target_closure_count =
          image.sparse_target_closure_count;
      output.requirements.sparse_target_closure_count =
          image.sparse_target_closure_count;
    }

    for (std::size_t group_index = 0U;
         group_index < group_count;
         ++group_index) {
      const auto& action_group = actions.groups[group_index];
      const auto& delta = frozen.coverage_deltas[group_index];
      auto& image = record.group_images[group_index];
      image.owner_group_index = group_index;
      image.q_r = action_group.q_r;
      image.action = action_group.action;
      auto& expected = prepared->expected_resident_groups[group_index];
      expected.group_record_index =
          prepared->resident_group_record_count_before + group_index;
      expected.batch_index = bundle.source_batch_index;
      expected.owner_group_index = group_index;
      expected.squared_level = bundle.squared_level;
      expected.order = bundle.order;
      expected.q_r = action_group.q_r;
      expected.action = action_group.action;
      expected.resultant_root_id = 0U;
      const auto child_begin = actions.prior_root_ids.begin() +
          static_cast<std::ptrdiff_t>(action_group.prior_root_offset);
      expected.child_root_ids.assign(
          child_begin,
          child_begin +
              static_cast<std::ptrdiff_t>(action_group.prior_root_count));
      expected.coverage_delta_points.reserve(delta.point_reference_count);
      for (std::size_t local = 0U;
           local < delta.point_reference_count;
           ++local) {
        const auto& point = frozen.coverage_delta_points[
            delta.point_reference_offset + local];
        if (point.owner_group_index != group_index) {
          return reject(
              ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                  no_frozen_group_shape_rejected);
        }
        expected.coverage_delta_points.push_back(point.point_id);
      }
      expected.empty_coverage_delta = delta.point_reference_count == 0U;

      for (std::size_t local = 0U;
           local < action_group.prior_root_count;
           ++local) {
        const auto source_root_id = actions.prior_root_ids[
            action_group.prior_root_offset + local];
        std::size_t witness_index = 0U;
        const auto* witness = find_root_witness(
            impl_->source_root_target_witnesses,
            source_root_id,
            &witness_index);
        if (witness == nullptr ||
            !witness->certified_conditional_root_witness() ||
            witness->source_order != bundle.order ||
            witness->target_order != record.target_order) {
          return reject(
              ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                  no_prior_source_root_witness);
        }
        TargetProbePayload probe;
        const auto status = probe_target_key(
            *impl_,
            witness->canonical_target_facet_key,
            record.target_order,
            record.squared_level,
            false,
            witness->target_source_binding_witness,
            locator_stamp,
            prepared->next_query_replay_token,
            probe);
        if (status != TargetProbeStatus::complete) {
          return reject(
              status == TargetProbeStatus::budget_exhausted
                  ? ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                        no_preparation_budget_exhausted
                  : status == TargetProbeStatus::target_not_rooted
                        ? ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                              no_target_component_not_rooted
                        : ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                              no_target_facet_locator_probe_rejected);
        }
        if (!add_group_candidate(
                accumulators[group_index],
                probe.canonical_target_key,
                probe)) {
          return reject(
              ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                  no_group_target_root_conflict);
        }
        prepared->prior_witness_reprobe_indices.push_back(witness_index);
        ++image.prior_root_witness_probe_count;
        if (action_group.q_r == 1U) {
          prepared->continuation_witness_indices[group_index] =
              witness_index;
        }
      }
      image.every_prior_source_root_witness_present_and_live_reprobed = true;
    }

    for (const auto& resolution : bundle.facet_resolutions) {
      if (resolution.token.kind ==
          ExactFrozenIncidenceTokenKind::rooted_carrier) {
        continue;
      }
      const auto* binding = find_token_binding(quotient, resolution.token);
      if (binding == nullptr || binding->group_index >= group_count) {
        return reject(
            ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                no_frozen_group_shape_rejected);
      }
      auto& image = record.group_images[binding->group_index];
      const auto& source_key = impl_->owned_bridge.resident_plan()
                                   .facet_tokens[resolution.facet_token_index]
                                   .facet_key;
      ++image.nonroot_source_facet_resolution_count;
      for (std::size_t erased_index = 0U;
           erased_index < source_key.point_count;
           ++erased_index) {
        const auto target_key = erase_source_point(source_key, erased_index);
        TargetProbePayload probe;
        const auto status = probe_target_key(
            *impl_,
            target_key,
            record.target_order,
            record.squared_level,
            true,
            std::nullopt,
            locator_stamp,
            prepared->next_query_replay_token,
            probe);
        if (status != TargetProbeStatus::complete) {
          return reject(
              status == TargetProbeStatus::budget_exhausted
                  ? ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                        no_preparation_budget_exhausted
                  : status == TargetProbeStatus::target_not_rooted
                        ? ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                              no_target_component_not_rooted
                        : ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                              no_target_facet_locator_probe_rejected);
        }
        if (!add_group_candidate(
                accumulators[binding->group_index],
                probe.canonical_target_key,
                probe)) {
          return reject(
              ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                  no_group_target_root_conflict);
        }
        if (probe.sparse_target_closure_used) {
          if (record.sparse_target_closure_count >=
                  impl_->budget.maximum_sparse_target_closure_count ||
              image.sparse_target_closure_count ==
                  std::numeric_limits<std::size_t>::max()) {
            return reject(
                ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                    no_preparation_budget_exhausted);
          }
          ++record.sparse_target_closure_count;
          ++image.sparse_target_closure_count;
          output.requirements.sparse_target_closure_count =
              record.sparse_target_closure_count;
        }
        ++image.projected_target_facet_probe_count;
      }
    }

    for (std::size_t group_index = 0U;
         group_index < group_count;
         ++group_index) {
      auto& image = record.group_images[group_index];
      const auto& accumulator = accumulators[group_index];
      std::size_t expected_deletion_count = 0U;
      if (image.nonroot_source_facet_resolution_count != 0U &&
          image.nonroot_source_facet_resolution_count >
              std::numeric_limits<std::size_t>::max() /
                  bundle.order) {
        return reject(
            ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                no_preparation_budget_exhausted);
      }
      expected_deletion_count =
          image.nonroot_source_facet_resolution_count * bundle.order;
      if (!accumulator.initialized ||
          image.projected_target_facet_probe_count !=
              expected_deletion_count) {
        return reject(
            ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
                no_frozen_group_shape_rejected);
      }
      image.resolved_target_root_id = accumulator.target_root_id;
      image.canonical_target_facet_key =
          accumulator.canonical_target_key;
      image.target_source_binding_witness =
          accumulator.canonical_source_binding_witness;
      image.every_latent_or_equal_source_key_projected_to_all_deletions =
          true;
      image.every_target_deletion_live_positive_and_rooted = true;
      image.every_locator_miss_resolved_by_certified_sparse_target_closure =
          true;
      image.one_live_target_root_for_complete_group = true;
    }
    if (impl_->owned_bridge.resident_locator().snapshot_stamp() !=
        locator_stamp) {
      return reject(
          ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
              no_resident_pre_batch_identity_rejected);
    }
    record.pre_batch_locator_snapshot_immutable_during_all_probes = true;
    if (record.terminal_component_image.has_value()) {
      record.terminal_component_image
          ->pre_batch_locator_snapshot_immutable_during_all_probes = true;
    }
    if (impl_->ticket_registry->live_ticket_count ==
        std::numeric_limits<std::size_t>::max()) {
      return reject(
          ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
              no_preparation_budget_exhausted);
    }
    ++impl_->ticket_registry->live_ticket_count;
    prepared->owns_ticket_slot = true;
    output.ticket.emplace(
        ExactDirectMorseResidentAllOrdersVerticalPreparedBatch{
            std::move(prepared)});
    output.conditional_higher_group_images_prepared = true;
    output.decision =
        ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
            complete_prepared_conditional_higher_vertical_batch;
    return output;
  } catch (const std::bad_alloc&) {
    return reject(
        ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
            no_allocation_failed);
  } catch (const std::length_error&) {
    return reject(
        ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
            no_preparation_budget_exhausted);
  } catch (const std::exception&) {
    return reject(
        ExactDirectMorseResidentAllOrdersVerticalPreparationDecision::
            no_frozen_group_shape_rejected);
  }
}

ExactDirectMorseResidentAllOrdersVerticalCommitResult
ExactDirectMorseResidentAllOrdersVerticalBridge::commit(
    ExactDirectMorseResidentAllOrdersVerticalPreparedBatch&& ticket) noexcept {
  static_assert(std::is_nothrow_move_constructible_v<exact::ExactLevel>);
  static_assert(std::is_nothrow_move_assignable_v<exact::ExactLevel>);
  static_assert(std::is_nothrow_move_constructible_v<
                ExactDirectMorseResidentAllOrdersVerticalBatchRecord>);
  static_assert(std::is_nothrow_move_constructible_v<
                ExactDirectMorseResidentAllOrdersVerticalRootWitness>);

  ExactDirectMorseResidentAllOrdersVerticalCommitResult output;
  auto prepared = std::move(ticket.impl_);
  if (prepared == nullptr || prepared->consumed ||
      !prepared->owned_bridge_ticket.has_value()) {
    output.ticket_consumed = true;
    output.decision =
        ExactDirectMorseResidentAllOrdersVerticalCommitDecision::
            no_ticket_already_consumed;
    return output;
  }
  prepared->consumed = true;
  output.ticket_consumed = true;
  if (prepared->owns_ticket_slot) {
    if (prepared->ticket_registry == nullptr ||
        prepared->ticket_registry->live_ticket_count == 0U) {
      std::terminate();
    }
    --prepared->ticket_registry->live_ticket_count;
    prepared->owns_ticket_slot = false;
  }
  if (!ready() || impl_->final_vertical_seal.has_value() ||
      prepared->seal.get() != impl_->seal.get()) {
    output.decision =
        ExactDirectMorseResidentAllOrdersVerticalCommitDecision::
            no_foreign_ticket_rejected;
    return output;
  }

  const bool higher_batch = prepared->higher_record.has_value();
  output.owned_bridge_commit = impl_->owned_bridge.commit(
      std::move(*prepared->owned_bridge_ticket));
  prepared->owned_bridge_ticket.reset();
  if (!output.owned_bridge_commit.certified_committed_batch()) {
    output.no_outer_vertical_state_mutated_on_owned_bridge_rejection = true;
    output.committed_resident_batch_cursor =
        impl_->committed_stamp.resident_batch_cursor;
    output.committed_higher_batch_count =
        impl_->committed_stamp.committed_higher_batch_count;
    output.committed_higher_group_count =
        impl_->committed_stamp.committed_higher_group_count;
    output.committed_higher_direct_saddle_group_binding_count =
        impl_->committed_stamp
            .committed_higher_direct_saddle_group_binding_count;
    output.persistent_source_root_witness_count =
        impl_->committed_stamp.persistent_source_root_witness_count;
    output.decision =
        ExactDirectMorseResidentAllOrdersVerticalCommitDecision::
            no_owned_bridge_commit_rejected_without_outer_vertical_mutation;
    return output;
  }

  if (output.owned_bridge_commit.committed_resident_batch_cursor !=
          prepared->resident_identity.batch_cursor + 1U ||
      output.owned_bridge_commit.resident_commit.committed_epoch !=
          prepared->resident_identity.epoch + 1U ||
      impl_->owned_bridge.resident_locator()
              .snapshot_stamp()
              .committed_batch_count !=
          prepared->resident_identity.batch_cursor + 1U) {
    std::terminate();
  }

  if (higher_batch) {
    auto& record = *prepared->higher_record;
    const auto& resident_groups = impl_->owned_bridge.resident_group_records();
    if (prepared->resident_group_record_count_before >
            resident_groups.size() ||
        resident_groups.size() -
                prepared->resident_group_record_count_before !=
            prepared->expected_resident_groups.size() ||
        prepared->expected_resident_groups.size() !=
            record.group_images.size() ||
        impl_->committed_higher_batches.size() >=
            impl_->committed_higher_batches.capacity() ||
        impl_->source_root_target_witnesses.size() +
                prepared->new_source_root_witness_count >
            impl_->source_root_target_witnesses.capacity() ||
        impl_->source_root_target_witnesses.size() +
                prepared->new_source_root_witness_count >
            impl_->budget.maximum_persistent_source_root_witness_count) {
      std::terminate();
    }
    for (std::size_t group_index = 0U;
         group_index < record.group_images.size();
         ++group_index) {
      const auto& actual = resident_groups[
          prepared->resident_group_record_count_before + group_index];
      const auto& expected = prepared->expected_resident_groups[group_index];
      if (!resident_group_matches_expected(actual, expected)) {
        std::terminate();
      }
      record.group_images[group_index].resident_resultant_source_root_id =
          actual.resultant_root_id;
      record.group_images[group_index]
          .resultant_source_root_bound_after_resident_commit = true;
    }
    for (auto& binding : record.direct_saddle_group_bindings) {
      if (binding.owner_group_index >= record.group_images.size() ||
          binding.group_image_index != binding.owner_group_index) {
        std::terminate();
      }
      binding.resident_resultant_root_id =
          record.group_images[binding.owner_group_index]
              .resident_resultant_source_root_id;
      if (!binding.certified_conditional_saddle_group_binding()) {
        std::terminate();
      }
    }
    record.binding_group_images_crosschecked = true;

    for (const std::size_t witness_index :
         prepared->prior_witness_reprobe_indices) {
      if (witness_index >= impl_->source_root_target_witnesses.size() ||
          impl_->source_root_target_witnesses[witness_index]
                  .live_reprobe_count ==
              std::numeric_limits<std::size_t>::max()) {
        std::terminate();
      }
      ++impl_->source_root_target_witnesses[witness_index]
            .live_reprobe_count;
      impl_->source_root_target_witnesses[witness_index]
          .source_binding_witness_live_reprobed = true;
      impl_->source_root_target_witnesses[witness_index]
          .target_root_was_live_and_rooted = true;
    }

    std::size_t appended_witness_count = 0U;
    for (std::size_t group_index = 0U;
         group_index < record.group_images.size();
         ++group_index) {
      const auto& actual = resident_groups[
          prepared->resident_group_record_count_before + group_index];
      const auto& expected = prepared->expected_resident_groups[group_index];
      const auto& image = record.group_images[group_index];
      if (expected.q_r == 1U) {
        const std::size_t witness_index =
            prepared->continuation_witness_indices[group_index];
        if (expected.child_root_ids.size() != 1U ||
            actual.resultant_root_id != expected.child_root_ids.front() ||
            witness_index >= impl_->source_root_target_witnesses.size()) {
          std::terminate();
        }
        auto& witness =
            impl_->source_root_target_witnesses[witness_index];
        if (witness.source_root_id != actual.resultant_root_id ||
            witness.source_order != record.source_order ||
            witness.target_order != record.target_order) {
          std::terminate();
        }
        witness.canonical_target_facet_key =
            image.canonical_target_facet_key;
        witness.target_source_binding_witness =
            image.target_source_binding_witness;
        witness.last_resolved_target_root_id = image.resolved_target_root_id;
        witness.last_verified_source_batch_index = record.source_batch_index;
        witness.canonical_target_facet_selected = true;
        witness.source_binding_witness_live_reprobed = true;
        witness.target_root_was_live_and_rooted = true;
      } else {
        if (actual.resultant_root_id == 0U ||
            (!impl_->source_root_target_witnesses.empty() &&
             impl_->source_root_target_witnesses.back().source_root_id >=
                 actual.resultant_root_id)) {
          std::terminate();
        }
        impl_->source_root_target_witnesses.emplace_back(
            ExactDirectMorseResidentAllOrdersVerticalRootWitness{
                actual.resultant_root_id,
                record.source_order,
                record.target_order,
                image.canonical_target_facet_key,
                image.target_source_binding_witness,
                image.resolved_target_root_id,
                record.source_batch_index,
                record.source_batch_index,
                1U,
                true,
                true,
                true,
                false});
        ++appended_witness_count;
      }
    }
    if (appended_witness_count !=
        prepared->new_source_root_witness_count) {
      std::terminate();
    }

    if (record.terminal_component_image.has_value()) {
      if (!record.group_images.empty() ||
          prepared->new_source_root_witness_count != 0U ||
          resident_groups.size() !=
              prepared->resident_group_record_count_before) {
        std::terminate();
      }
      record.terminal_component_image
          ->resident_terminal_batch_committed_without_synthetic_group = true;
    }

    record.live_post_resident_locator_stamp =
        impl_->owned_bridge.resident_locator().snapshot_stamp();
    record.every_higher_group_has_one_inductive_target_image = true;
    record.actual_resident_group_suffix_compared_field_by_field = true;
    record.resident_batch_committed = true;
    record.post_resident_commit_publication_allocation_free = true;
    record.complete_campaign_exhaustion_observed =
        impl_->owned_bridge.resident_complete();
    record.o4_membership_digest =
        canonical_exact_direct_morse_resident_higher_o4_membership_digest(
            record);
    record.o4_membership_digest_canonical =
        record.o4_membership_digest != contract::CanonicalId{};
    if (!record.certified_conditional_higher_batch()) {
      std::terminate();
    }
    impl_->committed_higher_batches.emplace_back(std::move(record));
    ++impl_->committed_stamp.committed_higher_batch_count;
    impl_->committed_stamp.committed_higher_group_count +=
        impl_->committed_higher_batches.back().group_images.size();
    impl_->committed_stamp.committed_terminal_component_image_count +=
        impl_->committed_higher_batches.back()
                .terminal_component_image.has_value()
            ? 1U
            : 0U;
    impl_->committed_stamp
        .committed_higher_direct_saddle_group_binding_count +=
        impl_->committed_higher_batches.back()
            .direct_saddle_group_bindings.size();
    impl_->committed_stamp.persistent_source_root_witness_count =
        impl_->source_root_target_witnesses.size();
    impl_->committed_stamp.next_query_replay_token =
        prepared->next_query_replay_token;
    output.outer_vertical_state_mutated = true;
    output.post_resident_commit_publication_allocation_free = true;
    output.decision =
        ExactDirectMorseResidentAllOrdersVerticalCommitDecision::
            complete_committed_conditional_higher_vertical_batch;
  } else {
    output.decision =
        ExactDirectMorseResidentAllOrdersVerticalCommitDecision::
            complete_committed_nonhigher_transit_batch;
  }

  if (impl_->committed_stamp.resident_locator_instance_id == 0U) {
    impl_->committed_stamp.resident_locator_instance_id =
        prepared->resident_identity.locator_instance_id;
  } else if (impl_->committed_stamp.resident_locator_instance_id !=
             prepared->resident_identity.locator_instance_id) {
    std::terminate();
  }
  impl_->committed_stamp.resident_batch_cursor =
      output.owned_bridge_commit.committed_resident_batch_cursor;
  impl_->committed_stamp.resident_epoch =
      output.owned_bridge_commit.resident_commit.committed_epoch;
  impl_->committed_stamp.committed_k2_batch_count =
      output.owned_bridge_commit.committed_k2_batch_count;
  impl_->committed_stamp.committed_k2_group_count =
      output.owned_bridge_commit.committed_k2_group_count;
  impl_->committed_stamp.committed_k2_direct_saddle_group_binding_count =
      output.owned_bridge_commit
          .committed_k2_direct_saddle_group_binding_count;
  impl_->committed_stamp.committed_k2_direct_birth_k1_binding_count =
      output.owned_bridge_commit.committed_k2_direct_birth_k1_binding_count;
  output.committed_resident_batch_cursor =
      impl_->committed_stamp.resident_batch_cursor;
  output.committed_higher_batch_count =
      impl_->committed_stamp.committed_higher_batch_count;
  output.committed_higher_group_count =
      impl_->committed_stamp.committed_higher_group_count;
  output.committed_higher_direct_saddle_group_binding_count =
      impl_->committed_stamp
          .committed_higher_direct_saddle_group_binding_count;
  output.persistent_source_root_witness_count =
      impl_->committed_stamp.persistent_source_root_witness_count;
  if (!impl_->structurally_ready()) {
    std::terminate();
  }
  return output;
}

ExactDirectMorseResidentAllOrdersVerticalSealResult
ExactDirectMorseResidentAllOrdersVerticalBridge::seal() noexcept {
  ExactDirectMorseResidentAllOrdersVerticalSealResult output;
  const auto reject = [&output](
                          ExactDirectMorseResidentAllOrdersVerticalSealDecision
                              decision) {
    output.seal.reset();
    output.final_seal_published = false;
    output.existing_final_seal_returned_without_mutation = false;
    output.no_partial_seal_published_on_failure = true;
    output.decision = decision;
    return std::move(output);
  };
  if (!ready()) {
    return reject(
        ExactDirectMorseResidentAllOrdersVerticalSealDecision::
            no_bridge_not_ready);
  }
  if (impl_->final_vertical_seal.has_value()) {
    output.seal = impl_->final_vertical_seal;
    output.final_seal_published = false;
    output.existing_final_seal_returned_without_mutation = true;
    output.decision =
        ExactDirectMorseResidentAllOrdersVerticalSealDecision::
            complete_certified_final_vertical_seal;
    return output;
  }
  if (!impl_->owned_bridge.resident_complete() ||
      impl_->committed_stamp.resident_batch_cursor !=
          impl_->owned_bridge.resident_plan().batches.size()) {
    return reject(
        ExactDirectMorseResidentAllOrdersVerticalSealDecision::
            no_resident_not_exhausted);
  }
  if (impl_->ticket_registry == nullptr ||
      impl_->ticket_registry->live_ticket_count != 0U) {
    return reject(
        ExactDirectMorseResidentAllOrdersVerticalSealDecision::
            no_outstanding_prepared_ticket);
  }
  if (!conditional_all_adjacent_order_component_images_complete()) {
    return reject(
        ExactDirectMorseResidentAllOrdersVerticalSealDecision::
            no_adjacent_source_order_component_coverage);
  }

  const auto& plan = impl_->owned_bridge.resident_plan();
  const auto& locator = impl_->owned_bridge.resident_locator();
  const auto& k2_batches = impl_->owned_bridge.committed_k2_batches();
  const auto& higher_batches = impl_->committed_higher_batches;
  const auto terminal_locator_stamp = locator.snapshot_stamp();
  if (!locator.certified_positive_locator() ||
      terminal_locator_stamp.external_authority_id !=
          impl_->committed_stamp.resident_session_authority_id ||
      terminal_locator_stamp.committed_batch_count != plan.batches.size() ||
      terminal_locator_stamp.component_union_count !=
          locator.committed_unions().size() ||
      locator.committed_batches().size() != plan.batches.size()) {
    return reject(
        ExactDirectMorseResidentAllOrdersVerticalSealDecision::
            no_plan_batch_replay_mismatch);
  }
  for (std::size_t batch_index = 0U;
       batch_index < locator.committed_batches().size();
       ++batch_index) {
    const auto& locator_batch = locator.committed_batches()[batch_index];
    if (locator_batch.committed_batch_index != batch_index ||
        !locator_batch.input_shape_certified ||
        !locator_batch.input_witness_structure_certified ||
        !locator_batch.strict_pre_batch_snapshot_certified ||
        !locator_batch.sequential_atomic_commit_certified) {
      return reject(
          ExactDirectMorseResidentAllOrdersVerticalSealDecision::
              no_plan_batch_replay_mismatch);
    }
  }
  for (std::size_t union_index = 0U;
       union_index < locator.committed_unions().size();
       ++union_index) {
    const auto& committed_union = locator.committed_unions()[union_index];
    if (committed_union.committed_union_index != union_index ||
        committed_union.left_handle >= locator.component_parents().size() ||
        committed_union.right_handle >= locator.component_parents().size() ||
        committed_union.witness.external_authority_id !=
            terminal_locator_stamp.external_authority_id ||
        committed_union.witness.replay_token == 0U) {
      return reject(
          ExactDirectMorseResidentAllOrdersVerticalSealDecision::
              no_plan_batch_replay_mismatch);
    }
  }
  std::size_t k2_cursor = 0U;
  std::size_t higher_cursor = 0U;
  std::size_t required_k2_count = 0U;
  std::size_t required_higher_count = 0U;
  for (std::size_t batch_index = 0U;
       batch_index < plan.batches.size();
       ++batch_index) {
    const auto& planned = plan.batches[batch_index];
    if (planned.batch_index != batch_index) {
      return reject(
          ExactDirectMorseResidentAllOrdersVerticalSealDecision::
              no_plan_batch_replay_mismatch);
    }
    if (planned.order == 2U) {
      if (k2_cursor >= k2_batches.size()) {
        return reject(
            ExactDirectMorseResidentAllOrdersVerticalSealDecision::
                no_plan_batch_replay_mismatch);
      }
      const auto& observed = k2_batches[k2_cursor];
      if (!observed.certified_conditional_k2_to_k1_batch() ||
          observed.source_batch_index != batch_index ||
          observed.order != planned.order ||
          observed.squared_level != planned.squared_level) {
        return reject(
            ExactDirectMorseResidentAllOrdersVerticalSealDecision::
                no_plan_batch_replay_mismatch);
      }
      ++k2_cursor;
      ++required_k2_count;
    } else if (planned.order >= 3U) {
      if (higher_cursor >= higher_batches.size()) {
        return reject(
            ExactDirectMorseResidentAllOrdersVerticalSealDecision::
                no_plan_batch_replay_mismatch);
      }
      const auto& observed = higher_batches[higher_cursor];
      if (!observed.certified_conditional_higher_batch() ||
          observed.source_batch_index != batch_index ||
          observed.source_order != planned.order ||
          observed.target_order + 1U != planned.order ||
          observed.squared_level != planned.squared_level) {
        return reject(
            ExactDirectMorseResidentAllOrdersVerticalSealDecision::
                no_plan_batch_replay_mismatch);
      }
      ++higher_cursor;
      ++required_higher_count;
    } else {
      return reject(
          ExactDirectMorseResidentAllOrdersVerticalSealDecision::
              no_plan_batch_replay_mismatch);
    }
  }
  if (k2_cursor != k2_batches.size() ||
      higher_cursor != higher_batches.size() ||
      required_k2_count != impl_->required_k2_batch_count ||
      required_higher_count != impl_->required_higher_batch_count ||
      required_k2_count !=
          impl_->committed_stamp.committed_k2_batch_count ||
      required_higher_count !=
          impl_->committed_stamp.committed_higher_batch_count) {
    return reject(
        ExactDirectMorseResidentAllOrdersVerticalSealDecision::
            no_plan_batch_replay_mismatch);
  }

  std::size_t sealed_k2_group_count = 0U;
  for (const auto& batch : k2_batches) {
    if (!checked_add(
            sealed_k2_group_count,
            batch.group_images.size(),
            sealed_k2_group_count)) {
      return reject(
          ExactDirectMorseResidentAllOrdersVerticalSealDecision::
              no_group_or_deletion_coverage_mismatch);
    }
  }
  std::size_t sealed_higher_group_count = 0U;
  std::size_t sealed_nonroot_resolution_count = 0U;
  std::size_t sealed_expected_target_probe_count = 0U;
  std::size_t sealed_target_probe_count = 0U;
  std::size_t sealed_sparse_target_closure_count = 0U;
  std::size_t sealed_terminal_component_image_count = 0U;
  std::size_t sealed_terminal_target_deletion_probe_count = 0U;
  std::size_t sealed_terminal_sparse_target_closure_count = 0U;
  for (const auto& batch : higher_batches) {
    if (!checked_add(
            sealed_higher_group_count,
            batch.group_images.size(),
            sealed_higher_group_count)) {
      return reject(
          ExactDirectMorseResidentAllOrdersVerticalSealDecision::
              no_group_or_deletion_coverage_mismatch);
    }
    std::size_t observed_batch_sparse_target_closure_count = 0U;
    for (const auto& group : batch.group_images) {
      std::size_t exact_group_deletion_count = 0U;
      if (group.nonroot_source_facet_resolution_count != 0U &&
          group.nonroot_source_facet_resolution_count >
              std::numeric_limits<std::size_t>::max() /
                  batch.source_order) {
        return reject(
            ExactDirectMorseResidentAllOrdersVerticalSealDecision::
                no_group_or_deletion_coverage_mismatch);
      }
      exact_group_deletion_count =
          group.nonroot_source_facet_resolution_count * batch.source_order;
      if (group.projected_target_facet_probe_count !=
              exact_group_deletion_count ||
          !checked_add(
              sealed_nonroot_resolution_count,
              group.nonroot_source_facet_resolution_count,
              sealed_nonroot_resolution_count) ||
          !checked_add(
              sealed_expected_target_probe_count,
              exact_group_deletion_count,
              sealed_expected_target_probe_count) ||
          !checked_add(
              sealed_target_probe_count,
              group.projected_target_facet_probe_count,
              sealed_target_probe_count) ||
          !checked_add(
              observed_batch_sparse_target_closure_count,
              group.sparse_target_closure_count,
              observed_batch_sparse_target_closure_count) ||
          !checked_add(
              sealed_sparse_target_closure_count,
              group.sparse_target_closure_count,
          sealed_sparse_target_closure_count)) {
        return reject(
            ExactDirectMorseResidentAllOrdersVerticalSealDecision::
                no_group_or_deletion_coverage_mismatch);
      }
    }
    if (batch.terminal_component_image.has_value()) {
      const auto& image = *batch.terminal_component_image;
      if (!image.certified_conditional_terminal_component_image() ||
          !checked_add(
              sealed_terminal_component_image_count,
              1U,
              sealed_terminal_component_image_count) ||
          !checked_add(
              sealed_terminal_target_deletion_probe_count,
              image.target_deletion_probe_count,
              sealed_terminal_target_deletion_probe_count) ||
          !checked_add(
              sealed_terminal_sparse_target_closure_count,
              image.sparse_target_closure_count,
              sealed_terminal_sparse_target_closure_count) ||
          !checked_add(
              sealed_expected_target_probe_count,
              image.target_deletion_probe_count,
              sealed_expected_target_probe_count) ||
          !checked_add(
              sealed_target_probe_count,
              image.target_deletion_probe_count,
              sealed_target_probe_count) ||
          !checked_add(
              observed_batch_sparse_target_closure_count,
              image.sparse_target_closure_count,
              observed_batch_sparse_target_closure_count) ||
          !checked_add(
              sealed_sparse_target_closure_count,
              image.sparse_target_closure_count,
              sealed_sparse_target_closure_count)) {
        return reject(
            ExactDirectMorseResidentAllOrdersVerticalSealDecision::
                no_group_or_deletion_coverage_mismatch);
      }
    }
    if (observed_batch_sparse_target_closure_count !=
        batch.sparse_target_closure_count) {
      return reject(
          ExactDirectMorseResidentAllOrdersVerticalSealDecision::
              no_group_or_deletion_coverage_mismatch);
    }
  }

  const auto& resident_groups = impl_->owned_bridge.resident_group_records();
  if (sealed_k2_group_count !=
          impl_->committed_stamp.committed_k2_group_count ||
      sealed_higher_group_count !=
          impl_->committed_stamp.committed_higher_group_count ||
      sealed_terminal_component_image_count !=
          impl_->committed_stamp.committed_terminal_component_image_count ||
      sealed_terminal_component_image_count > 1U ||
      sealed_k2_group_count > resident_groups.size() ||
      sealed_higher_group_count !=
          resident_groups.size() - sealed_k2_group_count) {
    return reject(
        ExactDirectMorseResidentAllOrdersVerticalSealDecision::
            no_group_or_deletion_coverage_mismatch);
  }
  for (std::size_t group_index = 0U;
       group_index < resident_groups.size();
       ++group_index) {
    const auto& group = resident_groups[group_index];
    if (group.group_record_index != group_index ||
        group.batch_index >= plan.batches.size() ||
        group.order != plan.batches[group.batch_index].order ||
        group.squared_level !=
            plan.batches[group.batch_index].squared_level ||
        group.order < 2U || group.resultant_root_id == 0U) {
      return reject(
          ExactDirectMorseResidentAllOrdersVerticalSealDecision::
              no_group_or_deletion_coverage_mismatch);
    }
  }

  // K2 batches close K1 only through their own source levels.  Drain the
  // privately owned exact K1 forest after resident exhaustion so equality
  // levels strictly after the last K2 batch are part of the vertical history.
  const auto terminal_k1 =
      impl_->owned_bridge.seal_owned_k1_terminal_target_history();
  if (!terminal_k1.certified_terminal_target_history()) {
    return reject(
        ExactDirectMorseResidentAllOrdersVerticalSealDecision::
            no_owned_k1_terminal_target_history_rejected);
  }

  // A source root may never occur in another source-order group after one or
  // more target-only batches have fused its target component.  Re-probing its
  // persistent original binding at the exhausted locator resolves through the
  // locator's current DSU parent chain.  Combined with the exact chronological
  // locator batch/union history checked above, this composes every such
  // intermediate target fusion without retaining a global vertical atlas.
  const std::size_t final_witness_probe_count =
      impl_->source_root_target_witnesses.size();
  std::size_t final_target_reprobe_count = 0U;
  if (!checked_add(
          final_witness_probe_count,
          sealed_terminal_component_image_count,
          final_target_reprobe_count) ||
      final_target_reprobe_count >
          impl_->budget.maximum_final_root_witness_probe_count ||
      final_witness_probe_count >
          impl_->budget.maximum_persistent_source_root_witness_count) {
    return reject(
        ExactDirectMorseResidentAllOrdersVerticalSealDecision::
            no_terminal_target_witness_reprobe_rejected);
  }
  std::uint64_t terminal_query_token =
      impl_->committed_stamp.next_query_replay_token;
  std::uint64_t terminal_query_token_begin = 0U;
  std::uint64_t terminal_query_token_end = 0U;
  if (final_target_reprobe_count != 0U) {
    if (terminal_query_token == 0U ||
        terminal_query_token > impl_->budget.maximum_query_replay_token ||
        final_target_reprobe_count - 1U >
            impl_->budget.maximum_query_replay_token - terminal_query_token) {
      return reject(
          ExactDirectMorseResidentAllOrdersVerticalSealDecision::
              no_terminal_target_witness_reprobe_rejected);
    }
    terminal_query_token_begin = terminal_query_token;
    terminal_query_token_end =
        terminal_query_token + final_target_reprobe_count - 1U;
  }
  for (const auto& witness : impl_->source_root_target_witnesses) {
    TargetProbePayload terminal_probe;
    const TargetProbeStatus status = probe_target_key(
        *impl_,
        witness.canonical_target_facet_key,
        witness.target_order,
        plan.batches.empty() ? exact::ExactLevel{}
                             : plan.batches.back().squared_level,
        false,
        witness.target_source_binding_witness,
        terminal_locator_stamp,
        terminal_query_token,
        terminal_probe);
    if (status != TargetProbeStatus::complete ||
        terminal_probe.target_root_id == 0U ||
        terminal_probe.source_binding_witness !=
            witness.target_source_binding_witness) {
      return reject(
          ExactDirectMorseResidentAllOrdersVerticalSealDecision::
              no_terminal_target_witness_reprobe_rejected);
    }
  }
  for (const auto& batch : higher_batches) {
    if (!batch.terminal_component_image.has_value()) {
      continue;
    }
    const auto& image = *batch.terminal_component_image;
    TargetProbePayload terminal_probe;
    const TargetProbeStatus status = probe_target_key(
        *impl_,
        image.canonical_target_facet_key,
        image.target_order,
        image.squared_level,
        false,
        image.target_source_binding_witness,
        terminal_locator_stamp,
        terminal_query_token,
        terminal_probe);
    if (status != TargetProbeStatus::complete ||
        terminal_probe.target_root_id == 0U ||
        terminal_probe.source_binding_witness !=
            image.target_source_binding_witness) {
      return reject(
          ExactDirectMorseResidentAllOrdersVerticalSealDecision::
              no_terminal_target_witness_reprobe_rejected);
    }
  }

  const auto source_kind = impl_->owned_bridge.resident_source_kind();
  const bool normalized_direct_source_session =
      impl_->owned_bridge.resident_normalized_direct_source_session();
  const auto normalized_retraction_mode =
      impl_->owned_bridge.resident_normalized_h0_retraction_mode();
  const bool normalized_horizontal_incidence_reduction_certified =
      impl_->owned_bridge
          .resident_normalized_horizontal_incidence_reduction_certified();
  if (!normalized_source_facts_consistent(
          source_kind,
          normalized_direct_source_session,
          normalized_retraction_mode,
          normalized_horizontal_incidence_reduction_certified)) {
    return reject(
        ExactDirectMorseResidentAllOrdersVerticalSealDecision::
            no_group_or_deletion_coverage_mismatch);
  }

  ExactDirectMorseResidentAllOrdersVerticalFinalSeal final;
  final.sealed_stamp = impl_->committed_stamp;
  final.required_resident_batch_count = plan.batches.size();
  final.required_k2_batch_count = required_k2_count;
  final.required_higher_batch_count = required_higher_count;
  final.required_resident_group_count = resident_groups.size();
  final.sealed_k2_group_count = sealed_k2_group_count;
  final.sealed_higher_group_count = sealed_higher_group_count;
  final.sealed_terminal_component_image_count =
      sealed_terminal_component_image_count;
  final.sealed_terminal_target_deletion_probe_count =
      sealed_terminal_target_deletion_probe_count;
  final.sealed_terminal_sparse_target_closure_count =
      sealed_terminal_sparse_target_closure_count;
  final.sealed_terminal_final_target_reprobe_count =
      sealed_terminal_component_image_count;
  final.sealed_nonroot_source_facet_resolution_count =
      sealed_nonroot_resolution_count;
  final.sealed_expected_projected_target_facet_probe_count =
      sealed_expected_target_probe_count;
  final.sealed_projected_target_facet_probe_count =
      sealed_target_probe_count;
  final.sealed_sparse_target_closure_count =
      sealed_sparse_target_closure_count;
  final.sealed_persistent_source_root_witness_count =
      final_witness_probe_count;
  final.sealed_final_root_witness_probe_count = final_witness_probe_count;
  final.sealed_terminal_locator_union_count =
      locator.committed_unions().size();
  final.sealed_terminal_locator_batch_count =
      locator.committed_batches().size();
  final.final_root_witness_query_replay_token_begin =
      terminal_query_token_begin;
  final.final_root_witness_query_replay_token_end = terminal_query_token_end;
  final.terminal_k1_session_instance_id =
      terminal_k1.k1_session_instance_id;
  final.pre_terminal_k1_level_cursor = terminal_k1.pre_k1_level_cursor;
  final.terminal_k1_level_cursor = terminal_k1.terminal_k1_level_cursor;
  final.distinct_k1_level_count = terminal_k1.distinct_k1_level_count;
  final.sealed_target_only_k1_level_count =
      terminal_k1.consumed_target_only_k1_level_count;
  final.k1_source_forest_digest = terminal_k1.k1_source_forest_digest;
  final.terminal_k1_history_digest =
      terminal_k1.terminal_k1_history_digest;
  final.terminal_locator_stamp = terminal_locator_stamp;
  final.resident_source_kind = source_kind;
  final.resident_normalized_h0_retraction_mode = normalized_retraction_mode;
  final.resident_normalized_direct_source_session =
      normalized_direct_source_session;
  final.resident_normalized_horizontal_incidence_reduction_certified =
      normalized_horizontal_incidence_reduction_certified;
  final.normalized_incidence_complete_v7_source_capability =
      normalized_incidence_complete_v7_capability(
          source_kind,
          normalized_direct_source_session,
          normalized_retraction_mode,
          normalized_horizontal_incidence_reduction_certified);
  final.owned_k1_terminal_advance_receipt_live_verified =
      terminal_k1.live_k1_advance_receipt_verified;
  final.every_target_only_k1_level_consumed =
      terminal_k1.every_remaining_k1_equal_level_batch_consumed;
  final.owned_k1_terminal_cursor_complete =
      terminal_k1.terminal_k1_cursor_complete;
  final.resident_cursor_exhausted = true;
  final.no_outstanding_prepared_ticket = true;
  final.every_required_plan_batch_replayed_in_exact_product_order = true;
  final.every_k2_batch_and_group_replayed_to_sealed_k1 = true;
  final.every_higher_group_bound_to_one_live_adjacent_order_root = true;
  final.every_terminal_component_bound_to_one_live_adjacent_order_root = true;
  final.every_nonroot_source_key_projected_to_all_codimension_one_deletions =
      true;
  final.every_resident_group_covered_exactly_once = true;
  final.every_persistent_target_binding_reprobed_at_terminal_locator = true;
  final.terminal_locator_monotone_union_history_owned_and_exhausted = true;
  final.every_intermediate_target_fusion_composed_by_terminal_root_reprobe =
      true;
  final.all_naturality_squares_replayed = true;
  final.vertical_maps_complete = true;
  if (!final.certified_final_vertical_seal()) {
    return reject(
        ExactDirectMorseResidentAllOrdersVerticalSealDecision::
            no_group_or_deletion_coverage_mismatch);
  }
  impl_->final_vertical_seal.emplace(final);
  output.seal.emplace(final);
  output.final_seal_published = true;
  output.existing_final_seal_returned_without_mutation = false;
  output.decision =
      ExactDirectMorseResidentAllOrdersVerticalSealDecision::
          complete_certified_final_vertical_seal;
  if (!impl_->structurally_ready()) {
    std::terminate();
  }
  return output;
}

bool ExactDirectMorseResidentAllOrdersVerticalInitialization::
    certified_ready_bridge() const noexcept {
  return schema_version ==
             direct_morse_resident_all_orders_vertical_bridge_schema_version &&
         owned_k2_k1_bridge_consumed &&
         process_local_bridge_capability_issued &&
         exact_initial_resident_cursor_required &&
         persistent_batch_and_root_witness_arenas_preallocated &&
         !incidence_complete_reduction &&
         !all_naturality_squares_replayed && !vertical_maps_complete &&
         !global_facet_coface_or_gamma_catalog_materialized &&
         !ordinary_or_higher_order_delaunay_materialized &&
         !pair_matrix_materialized && !public_status_claimed &&
         decision ==
             ExactDirectMorseResidentAllOrdersVerticalInitializationDecision::
                 complete_certified_bounded_conditional_bridge &&
         bridge.ready();
}

ExactDirectMorseResidentAllOrdersVerticalInitialization
initialize_exact_direct_morse_resident_all_orders_vertical_bridge(
    ExactDirectMorseResidentK2K1ClosedCutBridge&& owned_bridge,
    const ExactDirectMorseResidentAllOrdersVerticalBridgeBudget& budget) {
  ExactDirectMorseResidentAllOrdersVerticalInitialization output;
  output.requested_budget = budget;
  if (!owned_bridge.ready()) {
    output.decision =
        ExactDirectMorseResidentAllOrdersVerticalInitializationDecision::
            no_owned_k2_k1_bridge_rejected;
    return output;
  }
  const auto owned_stamp = owned_bridge.current_stamp();
  if (owned_stamp.resident_batch_cursor != 0U ||
      owned_stamp.resident_epoch != 0U ||
      owned_stamp.committed_k2_batch_count != 0U ||
      owned_stamp.committed_k2_group_count != 0U ||
      owned_stamp.committed_k2_direct_saddle_group_binding_count != 0U ||
      owned_stamp.committed_k2_direct_birth_k1_binding_count != 0U ||
      owned_bridge.resident_locator()
              .snapshot_stamp()
              .committed_batch_count != 0U ||
      !owned_bridge.resident_group_records().empty() ||
      !owned_bridge.committed_k2_batches().empty()) {
    output.decision =
        ExactDirectMorseResidentAllOrdersVerticalInitializationDecision::
            no_initial_resident_cursor_required;
    return output;
  }
  output.exact_initial_resident_cursor_required = true;

  output.requirements.resident_batch_count =
      owned_bridge.resident_plan().batches.size();
  for (std::size_t batch_index = 0U;
       batch_index < owned_bridge.resident_plan().batches.size();
       ++batch_index) {
    const auto& batch = owned_bridge.resident_plan().batches[batch_index];
    if (batch.batch_index != batch_index || batch.order < 2U) {
      output.decision =
          ExactDirectMorseResidentAllOrdersVerticalInitializationDecision::
              no_owned_k2_k1_bridge_rejected;
      return output;
    }
    if (batch.order == 2U) {
      ++output.requirements.resident_k2_batch_count;
    } else {
      ++output.requirements.resident_higher_batch_count;
    }
  }
  if (!count_higher_plan_saddles(
          owned_bridge.resident_plan(),
          output.requirements.resident_higher_direct_saddle_reference_count)) {
    output.decision =
        ExactDirectMorseResidentAllOrdersVerticalInitializationDecision::
            no_owned_k2_k1_bridge_rejected;
    return output;
  }
  std::size_t maximum_batch_saddle_count = 0U;
  for (const auto& batch : owned_bridge.resident_plan().batches) {
    if (batch.order < 3U) {
      continue;
    }
    std::size_t batch_saddle_count = 0U;
    for (std::size_t local = 0U;
         local < batch.direct_reference_count;
         ++local) {
      if (owned_bridge.resident_plan()
              .direct_references[batch.direct_reference_offset + local]
              .role == ExactDirectMorseH0Role::saddle) {
        ++batch_saddle_count;
      }
    }
    maximum_batch_saddle_count =
        std::max(maximum_batch_saddle_count, batch_saddle_count);
  }
  if (output.requirements.resident_higher_batch_count >
          budget.maximum_committed_higher_batch_count ||
      output.requirements.resident_higher_direct_saddle_reference_count >
          budget
              .maximum_committed_higher_direct_saddle_group_binding_count ||
      maximum_batch_saddle_count >
          budget.maximum_prepared_higher_direct_saddle_group_binding_count ||
      (output.requirements.resident_higher_batch_count != 0U &&
       (budget.maximum_committed_higher_group_count == 0U ||
        budget.maximum_prepared_higher_group_count == 0U ||
        budget.maximum_persistent_source_root_witness_count == 0U ||
        budget.maximum_prior_root_witness_probe_count == 0U ||
        budget.maximum_final_root_witness_probe_count == 0U ||
        budget.maximum_source_facet_resolution_scan_count == 0U ||
        budget.maximum_projected_target_facet_probe_count == 0U ||
        budget.maximum_sparse_target_closure_count == 0U ||
        budget.maximum_expected_group_child_root_reference_count == 0U ||
        budget
                .maximum_expected_group_coverage_delta_point_reference_count ==
            0U ||
        budget.maximum_query_replay_token == 0U ||
        budget.target_probe.maximum_slot_visit_count == 0U ||
        budget.target_closure.maximum_seed_count < 1U ||
        budget.target_closure.maximum_node_count < 1U ||
        budget.target_closure.maximum_step_call_count < 1U ||
        budget.target_closure.maximum_memo_slot_count < 3U ||
        budget.target_closure.step_budget.source_locator_probe
                .maximum_slot_visit_count == 0U ||
        budget.target_closure.step_budget.top_k_query
                .maximum_node_visit_count == 0U ||
        budget.target_closure.step_budget.successor_locator_probe
                .maximum_slot_visit_count == 0U ||
        (budget.target_closure_traversal_order !=
             spatial::LbvhTraversalOrder::near_first &&
         budget.target_closure_traversal_order !=
             spatial::LbvhTraversalOrder::far_first)))) {
    output.decision =
        ExactDirectMorseResidentAllOrdersVerticalInitializationDecision::
            no_budget_rejected;
    return output;
  }

  try {
    const std::uint64_t session_id = allocate_bridge_session_instance_id();
    if (session_id == 0U) {
      output.decision =
          ExactDirectMorseResidentAllOrdersVerticalInitializationDecision::
              no_session_instance_id_exhausted;
      return output;
    }
    auto impl =
        std::make_unique<ExactDirectMorseResidentAllOrdersVerticalBridge::Impl>();
    impl->budget = budget;
    impl->required_k2_batch_count =
        output.requirements.resident_k2_batch_count;
    impl->required_higher_batch_count =
        output.requirements.resident_higher_batch_count;
    impl->committed_higher_batches.reserve(
        budget.maximum_committed_higher_batch_count);
    impl->source_root_target_witnesses.reserve(
        budget.maximum_persistent_source_root_witness_count);
    output.persistent_batch_and_root_witness_arenas_preallocated = true;
    impl->seal = std::make_shared<const
        ExactDirectMorseResidentAllOrdersVerticalBridge::Impl::Seal>(
        ExactDirectMorseResidentAllOrdersVerticalBridge::Impl::Seal{
            session_id,
            owned_stamp.bridge_session_instance_id,
            owned_stamp.resident_session_authority_id,
            owned_stamp.canonical_cloud_digest,
            owned_stamp.resident_higher_canonical_cloud_digest});
    impl->ticket_registry = std::make_shared<
        ExactDirectMorseResidentAllOrdersVerticalBridge::Impl::TicketRegistry>();
    output.process_local_bridge_capability_issued = true;
    impl->committed_stamp.schema_version =
        direct_morse_resident_all_orders_vertical_bridge_schema_version;
    impl->committed_stamp.bridge_session_instance_id = session_id;
    impl->committed_stamp.owned_k2_k1_bridge_session_instance_id =
        owned_stamp.bridge_session_instance_id;
    impl->committed_stamp.resident_session_authority_id =
        owned_stamp.resident_session_authority_id;
    impl->committed_stamp.next_query_replay_token = 1U;
    impl->committed_stamp.canonical_cloud_digest =
        owned_stamp.canonical_cloud_digest;
    impl->committed_stamp.resident_higher_canonical_cloud_digest =
        owned_stamp.resident_higher_canonical_cloud_digest;
    output.canonical_cloud_digest = owned_stamp.canonical_cloud_digest;
    output.resident_higher_canonical_cloud_digest =
        owned_stamp.resident_higher_canonical_cloud_digest;
    impl->owned_bridge = std::move(owned_bridge);
    output.owned_k2_k1_bridge_consumed = true;
    impl->initialized = true;
    if (!impl->structurally_ready()) {
      output.decision =
          ExactDirectMorseResidentAllOrdersVerticalInitializationDecision::
              no_owned_k2_k1_bridge_rejected;
      return output;
    }
    output.bridge = ExactDirectMorseResidentAllOrdersVerticalBridge{
        std::move(impl)};
    output.decision =
        ExactDirectMorseResidentAllOrdersVerticalInitializationDecision::
            complete_certified_bounded_conditional_bridge;
    return output;
  } catch (const std::bad_alloc&) {
    output.decision =
        ExactDirectMorseResidentAllOrdersVerticalInitializationDecision::
            no_allocation_failed;
    return output;
  } catch (const std::exception&) {
    output.decision =
        ExactDirectMorseResidentAllOrdersVerticalInitializationDecision::
            no_owned_k2_k1_bridge_rejected;
    return output;
  }
}

}  // namespace morsehgp3d::hierarchy
