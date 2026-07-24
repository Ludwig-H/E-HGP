#include "morsehgp3d/hierarchy/direct_morse_chunk_run.hpp"

#include "direct_sparse_facet_descent_batch_replay_internal.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <limits>
#include <mutex>
#include <new>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::array<std::uint8_t, 16U> chunk_run_magic{
    'M', 'H', '3', 'D', '1', '5', 'B', 'C',
    'H', 'U', 'N', 'K', 'R', 'U', 'N', 0U};
constexpr std::string_view application_digest_domain =
    "MorseHGP3D/phase15/direct-morse-chunk-run-application/v1";
constexpr std::string_view checkpoint_digest_domain =
    "MorseHGP3D/phase15/direct-morse-chunk-run-checkpoint/v1";

static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));

[[nodiscard]] std::size_t checked_size(std::uint64_t value) {
  if (value > std::numeric_limits<std::size_t>::max()) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run u64 does not fit size_t");
  }
  return static_cast<std::size_t>(value);
}

[[nodiscard]] std::uint64_t wire_size(std::size_t value) noexcept {
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::length_error("a Phase-15 chunk-run size overflows");
  }
  return left + right;
}

[[nodiscard]] std::uint64_t checked_add_u64(
    std::uint64_t left,
    std::uint64_t right) {
  if (right > std::numeric_limits<std::uint64_t>::max() - left) {
    throw std::length_error(
        "a Phase-15 chunk-run budget sum overflows");
  }
  return left + right;
}

[[nodiscard]] std::uint64_t checked_mul_u64(
    std::uint64_t left,
    std::uint64_t right) {
  if (left != 0U &&
      right > std::numeric_limits<std::uint64_t>::max() / left) {
    throw std::length_error(
        "a Phase-15 chunk-run budget product overflows");
  }
  return left * right;
}

class Writer {
 public:
  explicit Writer(std::size_t maximum_byte_count)
      : maximum_byte_count_(maximum_byte_count) {}

  void bytes(std::span<const std::uint8_t> input) {
    const std::size_t next = checked_add(data_.size(), input.size());
    if (next > maximum_byte_count_) {
      throw std::length_error(
          "a Phase-15 chunk-run wire exceeds its byte cap");
    }
    data_.insert(data_.end(), input.begin(), input.end());
  }

  void u8(std::uint8_t value) {
    const std::array<std::uint8_t, 1U> encoded{value};
    bytes(encoded);
  }

  void boolean(bool value) {
    u8(value ? std::uint8_t{1U} : std::uint8_t{0U});
  }

  void u32(std::uint32_t value) {
    std::array<std::uint8_t, 4U> encoded{};
    for (std::size_t index = 0U; index < encoded.size(); ++index) {
      const std::size_t shift =
          (encoded.size() - 1U - index) * 8U;
      encoded[index] =
          static_cast<std::uint8_t>(value >> shift);
    }
    bytes(encoded);
  }

  void u64(std::uint64_t value) {
    std::array<std::uint8_t, 8U> encoded{};
    for (std::size_t index = 0U; index < encoded.size(); ++index) {
      const std::size_t shift =
          (encoded.size() - 1U - index) * 8U;
      encoded[index] =
          static_cast<std::uint8_t>(value >> shift);
    }
    bytes(encoded);
  }

  void size(std::size_t value) {
    u64(wire_size(value));
  }

  void id(const contract::CanonicalId& value) {
    bytes(value.bytes());
  }

  void text(std::string_view value) {
    size(value.size());
    bytes(std::span<const std::uint8_t>{
        reinterpret_cast<const std::uint8_t*>(value.data()),
        value.size()});
  }

  [[nodiscard]] std::vector<std::uint8_t> finish() && {
    return std::move(data_);
  }

 private:
  std::size_t maximum_byte_count_{};
  std::vector<std::uint8_t> data_;
};

class Reader {
 public:
  explicit Reader(std::span<const std::uint8_t> bytes)
      : bytes_(bytes) {}

  [[nodiscard]] std::span<const std::uint8_t> bytes(
      std::size_t count) {
    if (count > bytes_.size() - position_) {
      throw std::invalid_argument(
          "a Phase-15 chunk-run wire is truncated");
    }
    const auto output = bytes_.subspan(position_, count);
    position_ += count;
    return output;
  }

  [[nodiscard]] std::uint8_t u8() {
    return bytes(1U).front();
  }

  [[nodiscard]] bool boolean() {
    const std::uint8_t value = u8();
    if (value > 1U) {
      throw std::invalid_argument(
          "a Phase-15 chunk-run boolean is not canonical");
    }
    return value != 0U;
  }

  [[nodiscard]] std::uint32_t u32() {
    const auto input = bytes(4U);
    std::uint32_t value = 0U;
    for (const std::uint8_t byte : input) {
      value = static_cast<std::uint32_t>(
          (value << 8U) | byte);
    }
    return value;
  }

  [[nodiscard]] std::uint64_t u64() {
    const auto input = bytes(8U);
    std::uint64_t value = 0U;
    for (const std::uint8_t byte : input) {
      value = (value << 8U) | byte;
    }
    return value;
  }

  [[nodiscard]] std::size_t size() {
    return checked_size(u64());
  }

  [[nodiscard]] contract::CanonicalId id() {
    std::array<std::uint8_t, contract::CanonicalId::byte_count>
        encoded{};
    const auto input = bytes(encoded.size());
    std::copy(input.begin(), input.end(), encoded.begin());
    return contract::CanonicalId{encoded};
  }

  [[nodiscard]] std::string text(
      std::size_t maximum_byte_count) {
    const std::size_t count = size();
    if (count == 0U || count > maximum_byte_count) {
      throw std::invalid_argument(
          "a Phase-15 chunk-run exact text exceeds its cap");
    }
    const auto input = bytes(count);
    return std::string{
        reinterpret_cast<const char*>(input.data()), input.size()};
  }

  [[nodiscard]] bool empty() const noexcept {
    return position_ == bytes_.size();
  }

 private:
  std::span<const std::uint8_t> bytes_;
  std::size_t position_{};
};

void hash_bytes(
    contract::CanonicalSha256Builder& builder,
    std::span<const std::uint8_t> bytes) {
  builder.update(bytes);
}

void hash_u8(
    contract::CanonicalSha256Builder& builder,
    std::uint8_t value) {
  const std::array<std::uint8_t, 1U> encoded{value};
  hash_bytes(builder, encoded);
}

void hash_bool(
    contract::CanonicalSha256Builder& builder,
    bool value) {
  hash_u8(builder, value ? std::uint8_t{1U} : std::uint8_t{0U});
}

void hash_u32(
    contract::CanonicalSha256Builder& builder,
    std::uint32_t value) {
  std::array<std::uint8_t, 4U> encoded{};
  for (std::size_t index = 0U; index < encoded.size(); ++index) {
    const std::size_t shift =
        (encoded.size() - 1U - index) * 8U;
    encoded[index] =
        static_cast<std::uint8_t>(value >> shift);
  }
  hash_bytes(builder, encoded);
}

void hash_u64(
    contract::CanonicalSha256Builder& builder,
    std::uint64_t value) {
  std::array<std::uint8_t, 8U> encoded{};
  for (std::size_t index = 0U; index < encoded.size(); ++index) {
    const std::size_t shift =
        (encoded.size() - 1U - index) * 8U;
    encoded[index] =
        static_cast<std::uint8_t>(value >> shift);
  }
  hash_bytes(builder, encoded);
}

void hash_size(
    contract::CanonicalSha256Builder& builder,
    std::size_t value) {
  hash_u64(builder, wire_size(value));
}

void hash_id(
    contract::CanonicalSha256Builder& builder,
    const contract::CanonicalId& value) {
  hash_bytes(builder, value.bytes());
}

void hash_text(
    contract::CanonicalSha256Builder& builder,
    std::string_view value) {
  hash_size(builder, value.size());
  builder.update(value);
}

void hash_level(
    contract::CanonicalSha256Builder& builder,
    const exact::ExactLevel& value) {
  hash_text(builder, value.canonical_key());
}

void encode_probe_budget(
    Writer& writer,
    const ExactDirectSparsePositiveFacetProbeBudget& budget) {
  writer.size(budget.maximum_slot_visit_count);
  writer.size(budget.maximum_component_parent_hop_count);
}

void encode_top_k_budget(
    Writer& writer,
    const spatial::ExactLbvhTopKBudget& budget) {
  writer.size(budget.maximum_node_visit_count);
  writer.size(budget.maximum_internal_node_expansion_count);
  writer.size(budget.maximum_exact_aabb_bound_evaluation_count);
  writer.size(budget.maximum_exact_point_distance_evaluation_count);
  writer.size(budget.maximum_frontier_entry_count);
  writer.size(budget.maximum_best_neighbor_entry_count);
  writer.size(budget.maximum_cutoff_shell_entry_count);
}

void encode_step_budget(
    Writer& writer,
    const ExactDirectSparseFacetDescentStepBudget& budget) {
  encode_probe_budget(writer, budget.source_locator_probe);
  encode_top_k_budget(writer, budget.top_k_query);
  encode_probe_budget(writer, budget.successor_locator_probe);
}

void encode_closure_budget(
    Writer& writer,
    const ExactDirectSparseFacetDescentClosureBudget& budget) {
  writer.size(budget.maximum_seed_count);
  writer.size(budget.maximum_node_count);
  writer.size(budget.maximum_step_call_count);
  writer.size(budget.maximum_memo_slot_count);
  encode_step_budget(writer, budget.step_budget);
}

void encode_execution_budget(
    Writer& writer,
    const ExactDirectSparseFacetDescentBatchExecutionBudget& budget) {
  writer.size(budget.maximum_selected_lane_count);
  writer.size(budget.maximum_selected_family_count);
  writer.size(budget.maximum_selected_arm_seed_count);
  writer.size(budget.maximum_selected_key_point_reference_count);
  writer.size(budget.maximum_resolved_key_count);
}

[[nodiscard]] ExactDirectSparsePositiveFacetProbeBudget
decode_probe_budget(Reader& reader) {
  return {
      reader.size(),
      reader.size(),
  };
}

[[nodiscard]] spatial::ExactLbvhTopKBudget decode_top_k_budget(
    Reader& reader) {
  return {
      reader.size(),
      reader.size(),
      reader.size(),
      reader.size(),
      reader.size(),
      reader.size(),
      reader.size(),
  };
}

[[nodiscard]] ExactDirectSparseFacetDescentStepBudget
decode_step_budget(Reader& reader) {
  ExactDirectSparseFacetDescentStepBudget budget;
  budget.source_locator_probe = decode_probe_budget(reader);
  budget.top_k_query = decode_top_k_budget(reader);
  budget.successor_locator_probe = decode_probe_budget(reader);
  return budget;
}

[[nodiscard]] ExactDirectSparseFacetDescentClosureBudget
decode_closure_budget(Reader& reader) {
  ExactDirectSparseFacetDescentClosureBudget budget;
  budget.maximum_seed_count = reader.size();
  budget.maximum_node_count = reader.size();
  budget.maximum_step_call_count = reader.size();
  budget.maximum_memo_slot_count = reader.size();
  budget.step_budget = decode_step_budget(reader);
  return budget;
}

[[nodiscard]] ExactDirectSparseFacetDescentBatchExecutionBudget
decode_execution_budget(Reader& reader) {
  return {
      reader.size(),
      reader.size(),
      reader.size(),
      reader.size(),
      reader.size(),
  };
}

void encode_byte_demand(
    Writer& writer,
    const ExactDirectMorseByteBudgetDemand& demand) {
  writer.u64(demand.used_bytes);
  writer.u64(demand.reserved_bytes);
}

void encode_byte_axis(
    Writer& writer,
    const ExactDirectMorseByteBudgetAxisSnapshot& axis) {
  writer.u64(axis.limit_bytes);
  writer.u64(axis.used_bytes);
  writer.u64(axis.reserved_bytes);
  writer.u64(axis.remaining_bytes);
  writer.boolean(axis.addition_overflow);
  writer.boolean(axis.within_budget);
}

void encode_budget_snapshot(
    Writer& writer,
    const ExactDirectMorseBudgetSnapshot& snapshot) {
  writer.u32(snapshot.schema_version);
  writer.u8(static_cast<std::uint8_t>(snapshot.boundary));
  writer.u64(snapshot.sequence_number);
  writer.u64(snapshot.effective_policy.device_limit_bytes);
  writer.u64(snapshot.effective_policy.host_limit_bytes);
  writer.u64(snapshot.effective_policy.scratch_limit_bytes);
  writer.u64(snapshot.effective_policy.output_limit_bytes);
  writer.u64(snapshot.effective_policy.time_limit_ns);
  encode_byte_demand(writer, snapshot.requested_demand.device);
  encode_byte_demand(writer, snapshot.requested_demand.host);
  writer.u64(snapshot.requested_demand.scratch.used_bytes);
  writer.u64(
      snapshot.requested_demand.scratch.temporary_reserved_bytes);
  writer.u64(
      snapshot.requested_demand.scratch.worst_merge_reserved_bytes);
  writer.u64(
      snapshot.requested_demand.scratch.checkpoint_reserved_bytes);
  writer.u64(
      snapshot.requested_demand.scratch.safety_margin_reserved_bytes);
  encode_byte_demand(writer, snapshot.requested_demand.output);
  writer.u64(
      snapshot.requested_demand.time.operation_reserved_ns);
  writer.u64(
      snapshot.requested_demand.time.checkpoint_reserved_ns);
  encode_byte_axis(writer, snapshot.device);
  encode_byte_axis(writer, snapshot.host);
  encode_byte_axis(writer, snapshot.scratch);
  encode_byte_axis(writer, snapshot.output);
  writer.u64(snapshot.time.limit_ns);
  writer.u64(snapshot.time.elapsed_ns);
  writer.u64(snapshot.time.operation_reserved_ns);
  writer.u64(snapshot.time.checkpoint_reserved_ns);
  writer.u64(snapshot.time.reserved_ns);
  writer.u64(snapshot.time.remaining_ns);
  writer.boolean(snapshot.time.clock_regressed);
  writer.boolean(snapshot.time.addition_overflow);
  writer.boolean(snapshot.time.within_budget);
  writer.u8(static_cast<std::uint8_t>(
      snapshot.first_refusal_reason));
  writer.u8(static_cast<std::uint8_t>(snapshot.decision));
  writer.id(snapshot.snapshot_digest);
  writer.boolean(snapshot.accounting_evaluation_complete);
  writer.boolean(snapshot.deterministic_refusal_order_applied);
  writer.boolean(
      snapshot.scratch_reserve_components_accounted_together);
  writer.boolean(snapshot.arithmetic_overflow_detected);
  writer.boolean(snapshot.monotonic_clock_certified);
  writer.boolean(snapshot.public_v2_projection_claimed);
  writer.boolean(snapshot.public_v2_time_reserve_representable);
  writer.boolean(snapshot.architecture_only);
  writer.boolean(snapshot.hierarchy_or_scientific_state_mutated);
  writer.boolean(snapshot.forbidden_global_structure_materialized);
  writer.boolean(snapshot.public_status_claimed);
}

[[nodiscard]] ExactDirectMorseByteBudgetDemand
decode_byte_demand(Reader& reader) {
  return {reader.u64(), reader.u64()};
}

[[nodiscard]] ExactDirectMorseByteBudgetAxisSnapshot
decode_byte_axis(Reader& reader) {
  ExactDirectMorseByteBudgetAxisSnapshot axis;
  axis.limit_bytes = reader.u64();
  axis.used_bytes = reader.u64();
  axis.reserved_bytes = reader.u64();
  axis.remaining_bytes = reader.u64();
  axis.addition_overflow = reader.boolean();
  axis.within_budget = reader.boolean();
  return axis;
}

[[nodiscard]] ExactDirectMorseBudgetSnapshot decode_budget_snapshot(
    Reader& reader) {
  ExactDirectMorseBudgetSnapshot snapshot;
  snapshot.schema_version = reader.u32();
  const std::uint8_t boundary = reader.u8();
  if (boundary <
          static_cast<std::uint8_t>(
              ExactDirectMorseBudgetBoundary::before_batch) ||
      boundary >
          static_cast<std::uint8_t>(
              ExactDirectMorseBudgetBoundary::final)) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run budget boundary is unknown");
  }
  snapshot.boundary =
      static_cast<ExactDirectMorseBudgetBoundary>(boundary);
  snapshot.sequence_number = reader.u64();
  snapshot.effective_policy.device_limit_bytes = reader.u64();
  snapshot.effective_policy.host_limit_bytes = reader.u64();
  snapshot.effective_policy.scratch_limit_bytes = reader.u64();
  snapshot.effective_policy.output_limit_bytes = reader.u64();
  snapshot.effective_policy.time_limit_ns = reader.u64();
  snapshot.requested_demand.device = decode_byte_demand(reader);
  snapshot.requested_demand.host = decode_byte_demand(reader);
  snapshot.requested_demand.scratch.used_bytes = reader.u64();
  snapshot.requested_demand.scratch.temporary_reserved_bytes =
      reader.u64();
  snapshot.requested_demand.scratch.worst_merge_reserved_bytes =
      reader.u64();
  snapshot.requested_demand.scratch.checkpoint_reserved_bytes =
      reader.u64();
  snapshot.requested_demand.scratch.safety_margin_reserved_bytes =
      reader.u64();
  snapshot.requested_demand.output = decode_byte_demand(reader);
  snapshot.requested_demand.time.operation_reserved_ns =
      reader.u64();
  snapshot.requested_demand.time.checkpoint_reserved_ns =
      reader.u64();
  snapshot.device = decode_byte_axis(reader);
  snapshot.host = decode_byte_axis(reader);
  snapshot.scratch = decode_byte_axis(reader);
  snapshot.output = decode_byte_axis(reader);
  snapshot.time.limit_ns = reader.u64();
  snapshot.time.elapsed_ns = reader.u64();
  snapshot.time.operation_reserved_ns = reader.u64();
  snapshot.time.checkpoint_reserved_ns = reader.u64();
  snapshot.time.reserved_ns = reader.u64();
  snapshot.time.remaining_ns = reader.u64();
  snapshot.time.clock_regressed = reader.boolean();
  snapshot.time.addition_overflow = reader.boolean();
  snapshot.time.within_budget = reader.boolean();
  const std::uint8_t refusal = reader.u8();
  if (refusal >
      static_cast<std::uint8_t>(
          ExactDirectMorseBudgetRefusalReason::time)) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run budget refusal is unknown");
  }
  snapshot.first_refusal_reason =
      static_cast<ExactDirectMorseBudgetRefusalReason>(refusal);
  const std::uint8_t decision = reader.u8();
  if (decision >
      static_cast<std::uint8_t>(
          ExactDirectMorseBudgetDecision::
              rejected_clock_regression)) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run budget decision is unknown");
  }
  snapshot.decision =
      static_cast<ExactDirectMorseBudgetDecision>(decision);
  snapshot.snapshot_digest = reader.id();
  snapshot.accounting_evaluation_complete = reader.boolean();
  snapshot.deterministic_refusal_order_applied = reader.boolean();
  snapshot.scratch_reserve_components_accounted_together =
      reader.boolean();
  snapshot.arithmetic_overflow_detected = reader.boolean();
  snapshot.monotonic_clock_certified = reader.boolean();
  snapshot.public_v2_projection_claimed = reader.boolean();
  snapshot.public_v2_time_reserve_representable =
      reader.boolean();
  snapshot.architecture_only = reader.boolean();
  snapshot.hierarchy_or_scientific_state_mutated =
      reader.boolean();
  snapshot.forbidden_global_structure_materialized =
      reader.boolean();
  snapshot.public_status_claimed = reader.boolean();
  return snapshot;
}

[[nodiscard]] bool canonical_facet_key(
    const ExactDirectSparseFacetKey& key) noexcept {
  if (key.point_count == 0U ||
      key.point_count > key.point_ids.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    if (index != 0U &&
        key.point_ids[index - 1U] >= key.point_ids[index]) {
      return false;
    }
  }
  return std::all_of(
      key.point_ids.begin() +
          static_cast<std::ptrdiff_t>(key.point_count),
      key.point_ids.end(),
      [](spatial::PointId id) { return id == 0U; });
}

void encode_facet_key(
    Writer& writer,
    const ExactDirectSparseFacetKey& key) {
  if (!canonical_facet_key(key)) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run facet key is not canonical");
  }
  writer.size(key.point_count);
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    writer.u64(key.point_ids[index]);
  }
}

[[nodiscard]] ExactDirectSparseFacetKey decode_facet_key(
    Reader& reader) {
  ExactDirectSparseFacetKey key;
  key.point_count = reader.size();
  if (key.point_count == 0U ||
      key.point_count > key.point_ids.size()) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run facet cardinality is invalid");
  }
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    key.point_ids[index] = reader.u64();
  }
  if (!canonical_facet_key(key)) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run facet key is not canonical");
  }
  return key;
}

void encode_locator_stamp(
    Writer& writer,
    const ExactDirectSparsePositiveFacetLocatorSnapshotStamp& stamp) {
  writer.u32(stamp.schema_version);
  writer.u64(stamp.external_authority_id);
  writer.size(stamp.committed_batch_count);
  writer.size(stamp.inserted_key_count);
  writer.size(stamp.component_union_count);
  writer.size(stamp.binding_count);
  writer.id(stamp.committed_history_digest);
}

[[nodiscard]] ExactDirectSparsePositiveFacetLocatorSnapshotStamp
decode_locator_stamp(Reader& reader) {
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp stamp;
  stamp.schema_version = reader.u32();
  stamp.external_authority_id = reader.u64();
  stamp.committed_batch_count = reader.size();
  stamp.inserted_key_count = reader.size();
  stamp.component_union_count = reader.size();
  stamp.binding_count = reader.size();
  stamp.committed_history_digest = reader.id();
  return stamp;
}

[[nodiscard]] bool known_traversal_order(
    spatial::LbvhTraversalOrder order) noexcept {
  return order == spatial::LbvhTraversalOrder::near_first ||
         order == spatial::LbvhTraversalOrder::far_first;
}

void require_complete_scientific_delta(
    const ExactDirectSparseFacetDescentBatchExecutionResult& result) {
  if (!result.complete_architecture_execution() ||
      !result.source_chunk_index.has_value() ||
      !known_traversal_order(result.traversal_order) ||
      result.closure_graph_persisted ||
      result.facet_coface_cell_gamma_or_delaunay_materialized ||
      result.scientific_commit_performed ||
      result.locator_state_mutated ||
      result.hierarchy_reduction_or_attachment_published ||
      result.gpu_execution_qualified ||
      result.sub_second_latency_claimed ||
      result.ten_million_point_capacity_claimed ||
      result.public_status_claimed) {
    throw std::invalid_argument(
        "a Phase-15 chunk run requires one complete compact 14D delta");
  }
}

[[nodiscard]] std::vector<std::uint8_t> encode_science(
    const ExactDirectSparseFacetDescentBatchExecutionResult& result,
    std::size_t maximum_byte_count) {
  require_complete_scientific_delta(result);
  Writer writer{maximum_byte_count};
  writer.u32(result.schema_version);
  encode_execution_budget(writer, result.requested_budget);
  encode_closure_budget(writer, result.requested_closure_budget);
  writer.u8(static_cast<std::uint8_t>(result.traversal_order));
  writer.size(result.source_batch_index);
  writer.boolean(result.source_chunk_index.has_value());
  writer.size(*result.source_chunk_index);
  writer.size(result.source_lane_begin_index);
  writer.size(result.source_lane_end_index);
  writer.size(result.source_family_begin_index);
  writer.size(result.source_family_end_index);
  writer.size(result.source_arm_seed_begin_index);
  writer.size(result.source_arm_seed_end_index);
  writer.size(result.source_facet_cardinality);
  writer.text(result.closed_batch_squared_level.canonical_key());
  writer.u64(result.locator_query_witness.external_authority_id);
  writer.u64(result.locator_query_witness.replay_token);
  encode_locator_stamp(writer, result.locator_snapshot_stamp);
  writer.size(result.required_selected_lane_count);
  writer.size(result.required_selected_family_count);
  writer.size(result.required_selected_arm_seed_count);
  writer.size(result.required_selected_key_point_reference_count);
  writer.size(result.required_resolved_key_count);
  writer.u8(static_cast<std::uint8_t>(
      result.closure_summary.disposition));
  writer.u8(static_cast<std::uint8_t>(
      result.closure_summary.decision));
  writer.boolean(result.closure_summary.complete_relative_positive);
  writer.size(result.resolved_keys.size());
  for (const auto& resolved : result.resolved_keys) {
    writer.size(resolved.resolved_key_index);
    encode_facet_key(writer, resolved.source_facet_key);
    writer.size(resolved.resolved_component_handle);
    writer.u64(
        resolved.resolved_binding_witness.external_authority_id);
    writer.u64(resolved.resolved_binding_witness.replay_token);
    writer.u8(static_cast<std::uint8_t>(
        resolved.closure_disposition));
    writer.boolean(
        resolved.source_projection_and_terminal_certified);
  }
  writer.size(result.arm_joins.size());
  for (const auto& join : result.arm_joins) {
    writer.size(join.arm_seed_index);
    writer.size(join.family_index);
    writer.size(join.lane_index);
    writer.size(join.resolved_key_index);
    writer.boolean(join.arm_identity_and_full_key_joined);
  }
  writer.u8(static_cast<std::uint8_t>(result.decision));
  writer.u8(static_cast<std::uint8_t>(result.scope));
  return std::move(writer).finish();
}

struct ScienceShape {
  std::size_t source_batch_index{};
  std::size_t source_chunk_index{};
  std::size_t resolved_key_count{};
  std::size_t arm_join_count{};
};

[[nodiscard]] ScienceShape validate_science(
    std::span<const std::uint8_t> science,
    std::size_t maximum_exact_text_byte_count) {
  Reader reader{science};
  if (reader.u32() !=
      direct_sparse_facet_descent_batch_executor_schema_version) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run science schema is unknown");
  }
  static_cast<void>(decode_execution_budget(reader));
  static_cast<void>(decode_closure_budget(reader));
  const auto traversal =
      static_cast<spatial::LbvhTraversalOrder>(reader.u8());
  if (!known_traversal_order(traversal)) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run traversal order is unknown");
  }
  ScienceShape shape;
  shape.source_batch_index = reader.size();
  if (!reader.boolean()) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run delta has no 14A chunk identity");
  }
  shape.source_chunk_index = reader.size();
  const std::size_t lane_begin = reader.size();
  const std::size_t lane_end = reader.size();
  const std::size_t family_begin = reader.size();
  const std::size_t family_end = reader.size();
  const std::size_t arm_begin = reader.size();
  const std::size_t arm_end = reader.size();
  if (lane_begin > lane_end || family_begin > family_end ||
      arm_begin > arm_end) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run batch interval is reversed");
  }
  const std::size_t facet_cardinality = reader.size();
  if (facet_cardinality >
      direct_sparse_positive_facet_maximum_point_count) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run facet cardinality exceeds K=10");
  }
  const std::string exact_level =
      reader.text(maximum_exact_text_byte_count);
  const exact::ExactLevel parsed_level{
      exact::ExactRational::parse_canonical(exact_level)};
  if (parsed_level.canonical_key() != exact_level) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run level is not canonical");
  }
  static_cast<void>(reader.u64());
  static_cast<void>(reader.u64());
  static_cast<void>(decode_locator_stamp(reader));
  const std::size_t required_lane_count = reader.size();
  const std::size_t required_family_count = reader.size();
  const std::size_t required_arm_count = reader.size();
  static_cast<void>(reader.size());
  const std::size_t required_resolved_count = reader.size();
  if (required_lane_count != lane_end - lane_begin ||
      required_family_count != family_end - family_begin ||
      required_arm_count != arm_end - arm_begin) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run required count contradicts its interval");
  }
  const std::uint8_t closure_disposition = reader.u8();
  if (closure_disposition !=
      static_cast<std::uint8_t>(
          ExactDirectSparseFacetDescentClosureDisposition::
              relative_positive)) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run closure is not relative-positive");
  }
  const std::uint8_t closure_decision = reader.u8();
  if (closure_decision !=
          static_cast<std::uint8_t>(
              ExactDirectSparseFacetDescentClosureDecision::
                  complete_empty_seed_set) &&
      closure_decision !=
          static_cast<std::uint8_t>(
              ExactDirectSparseFacetDescentClosureDecision::
                  complete_all_seeds_relative_positive)) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run closure decision is incomplete");
  }
  if (!reader.boolean()) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run closure is not complete");
  }
  shape.resolved_key_count = reader.size();
  if (shape.resolved_key_count != required_resolved_count) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run resolved-key count is inconsistent");
  }
  for (std::size_t index = 0U;
       index < shape.resolved_key_count;
       ++index) {
    if (reader.size() != index) {
      throw std::invalid_argument(
          "a Phase-15 chunk-run resolved-key index is not dense");
    }
    static_cast<void>(decode_facet_key(reader));
    static_cast<void>(reader.size());
    static_cast<void>(reader.u64());
    static_cast<void>(reader.u64());
    if (reader.u8() !=
        static_cast<std::uint8_t>(
            ExactDirectSparseFacetDescentClosureDisposition::
                relative_positive)) {
      throw std::invalid_argument(
          "a Phase-15 chunk-run resolved key is not positive");
    }
    if (!reader.boolean()) {
      throw std::invalid_argument(
          "a Phase-15 chunk-run resolved key is uncertified");
    }
  }
  shape.arm_join_count = reader.size();
  if (shape.arm_join_count != required_arm_count) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run arm-join count is inconsistent");
  }
  for (std::size_t index = 0U;
       index < shape.arm_join_count;
       ++index) {
    static_cast<void>(reader.size());
    static_cast<void>(reader.size());
    static_cast<void>(reader.size());
    if (reader.size() >= shape.resolved_key_count &&
        shape.resolved_key_count != 0U) {
      throw std::invalid_argument(
          "a Phase-15 chunk-run arm join has no resolved key");
    }
    if (shape.resolved_key_count == 0U) {
      throw std::invalid_argument(
          "a Phase-15 chunk-run nonempty arm join has no key");
    }
    if (!reader.boolean()) {
      throw std::invalid_argument(
          "a Phase-15 chunk-run arm join is uncertified");
    }
  }
  if (reader.u8() !=
          static_cast<std::uint8_t>(
              ExactDirectSparseFacetDescentBatchExecutionDecision::
                  complete_architecture_only_relative_positive_batch_delta) ||
      reader.u8() !=
          static_cast<std::uint8_t>(
              ExactDirectSparseFacetDescentBatchExecutionScope::
                  one_exact_14c_batch_compact_relative_positive_delta_before_hierarchy_commit_only) ||
      !reader.empty()) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run science payload is noncanonical");
  }
  return shape;
}

void encode_industrial_counters(
    Writer& writer,
    const ExactDirectMorseIndustrialCounters& counters) {
  writer.u64(counters.batch_count);
  writer.u64(counters.birth_count);
  writer.u64(counters.saddle_count);
  writer.u64(counters.arm_count);
  writer.u64(counters.key_point_reference_count);
  writer.u64(counters.node_count_upper_bound);
  writer.u64(counters.child_reference_count_upper_bound);
  writer.u64(counters.descent_node_reserve_count);
}

[[nodiscard]] ExactDirectMorseIndustrialCounters
decode_industrial_counters(Reader& reader) {
  ExactDirectMorseIndustrialCounters counters;
  counters.batch_count = reader.u64();
  counters.birth_count = reader.u64();
  counters.saddle_count = reader.u64();
  counters.arm_count = reader.u64();
  counters.key_point_reference_count = reader.u64();
  counters.node_count_upper_bound = reader.u64();
  counters.child_reference_count_upper_bound = reader.u64();
  counters.descent_node_reserve_count = reader.u64();
  return counters;
}

struct ParsedBatch {
  ScienceShape shape{};
  std::vector<std::uint8_t> science;
};

struct ParsedPayload {
  contract::CanonicalId application_contract_digest{};
  ExactDirectMorseIndustrialChunk chunk{};
  ExactDirectMorseBudgetSnapshot budget_snapshot{};
  std::vector<ParsedBatch> batches;
};

[[nodiscard]] std::vector<std::uint8_t> encode_payload(
    const contract::CanonicalId& application_contract_digest,
    const ExactDirectMorseIndustrialChunk& chunk,
    const ExactDirectMorseBudgetSnapshot& budget_snapshot,
    std::span<const ParsedBatch> batches,
    std::size_t maximum_payload_byte_count) {
  Writer writer{maximum_payload_byte_count};
  writer.bytes(chunk_run_magic);
  writer.u32(direct_morse_chunk_run_schema_version);
  writer.id(application_contract_digest);
  writer.size(chunk.chunk_index);
  writer.size(chunk.source_batch_begin_index);
  writer.size(chunk.source_batch_end_index);
  encode_industrial_counters(writer, chunk.counters);
  writer.u64(chunk.estimated_byte_count);
  writer.boolean(chunk.checkpoint_boundary_after_chunk);
  writer.boolean(chunk.external_run_boundary_after_chunk);
  encode_budget_snapshot(writer, budget_snapshot);
  writer.size(batches.size());
  for (const ParsedBatch& batch : batches) {
    writer.size(batch.science.size());
    writer.bytes(batch.science);
  }
  return std::move(writer).finish();
}

[[nodiscard]] ParsedPayload decode_payload(
    std::span<const std::uint8_t> bytes,
    const ExactDirectMorseChunkRunLimits& limits) {
  if (bytes.size() > limits.maximum_payload_byte_count) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run payload exceeds its cap");
  }
  Reader reader{bytes};
  if (!std::ranges::equal(reader.bytes(chunk_run_magic.size()),
                          chunk_run_magic)) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run magic is invalid");
  }
  if (reader.u32() != direct_morse_chunk_run_schema_version) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run schema is unknown");
  }
  ParsedPayload parsed;
  parsed.application_contract_digest = reader.id();
  parsed.chunk.chunk_index = reader.size();
  parsed.chunk.source_batch_begin_index = reader.size();
  parsed.chunk.source_batch_end_index = reader.size();
  if (parsed.chunk.source_batch_begin_index >
      parsed.chunk.source_batch_end_index) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run batch interval is reversed");
  }
  parsed.chunk.counters = decode_industrial_counters(reader);
  parsed.chunk.estimated_byte_count = reader.u64();
  parsed.chunk.checkpoint_boundary_after_chunk = reader.boolean();
  parsed.chunk.external_run_boundary_after_chunk = reader.boolean();
  parsed.budget_snapshot = decode_budget_snapshot(reader);
  const std::size_t batch_count = reader.size();
  if (batch_count > limits.maximum_batch_count_per_chunk ||
      batch_count !=
          parsed.chunk.source_batch_end_index -
              parsed.chunk.source_batch_begin_index ||
      parsed.chunk.counters.batch_count != wire_size(batch_count)) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run batch count is invalid");
  }
  parsed.batches.reserve(batch_count);
  std::size_t total_resolved_key_count = 0U;
  std::size_t total_arm_join_count = 0U;
  for (std::size_t index = 0U; index < batch_count; ++index) {
    const std::size_t science_byte_count = reader.size();
    if (science_byte_count == 0U ||
        science_byte_count >
            limits.maximum_science_byte_count_per_batch) {
      throw std::invalid_argument(
          "a Phase-15 chunk-run science segment exceeds its cap");
    }
    ParsedBatch batch;
    const auto science = reader.bytes(science_byte_count);
    batch.science.assign(science.begin(), science.end());
    batch.shape = validate_science(
        batch.science,
        limits.maximum_science_byte_count_per_batch);
    if (batch.shape.source_batch_index !=
            parsed.chunk.source_batch_begin_index + index ||
        batch.shape.source_chunk_index != parsed.chunk.chunk_index) {
      throw std::invalid_argument(
          "a Phase-15 chunk-run batch identity is not contiguous");
    }
    total_resolved_key_count = checked_add(
        total_resolved_key_count, batch.shape.resolved_key_count);
    total_arm_join_count = checked_add(
        total_arm_join_count, batch.shape.arm_join_count);
    if (total_resolved_key_count >
            limits.maximum_total_resolved_key_count_per_chunk ||
        total_arm_join_count >
            limits.maximum_total_arm_join_count_per_chunk) {
      throw std::invalid_argument(
          "a Phase-15 chunk-run scientific population exceeds its cap");
    }
    parsed.batches.push_back(std::move(batch));
  }
  if (!reader.empty()) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run payload has trailing bytes");
  }
  return parsed;
}

[[nodiscard]] bool valid_budget_snapshot(
    const ExactDirectMorseBudgetSnapshot& snapshot,
    std::uint64_t expected_sequence,
    const ExactDirectMorseEffectiveBudgetPolicy& expected_policy,
    const ExactDirectMorseBudgetDemand& expected_demand) {
  if (snapshot.schema_version != direct_morse_budget_schema_version ||
      snapshot.boundary !=
          ExactDirectMorseBudgetBoundary::before_run ||
      snapshot.sequence_number != expected_sequence ||
      snapshot.effective_policy != expected_policy ||
      snapshot.requested_demand != expected_demand) {
    return false;
  }
  ExactDirectMorseBudgetTracker verifier{
      expected_policy,
      snapshot.time.elapsed_ns,
      [] { return 0U; }};
  return snapshot == verifier.snapshot(
                         ExactDirectMorseBudgetBoundary::before_run,
                         expected_sequence,
                         expected_demand);
}

[[nodiscard]] contract::CanonicalId checkpoint_digest(
    const contract::CanonicalId& application_contract_digest,
    const contract::CanonicalId& source_checkpoint_digest,
    const ExactDirectMorseIndustrialChunk& chunk,
    std::span<const std::uint8_t> payload) {
  contract::CanonicalSha256Builder builder;
  hash_text(builder, checkpoint_digest_domain);
  hash_id(builder, application_contract_digest);
  hash_id(builder, source_checkpoint_digest);
  hash_size(builder, chunk.chunk_index);
  hash_size(builder, chunk.source_batch_begin_index);
  hash_size(builder, chunk.source_batch_end_index);
  hash_size(builder, payload.size());
  builder.update(payload);
  return builder.finalize();
}

void hash_execution_budget(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparseFacetDescentBatchExecutionBudget& budget) {
  hash_size(builder, budget.maximum_selected_lane_count);
  hash_size(builder, budget.maximum_selected_family_count);
  hash_size(builder, budget.maximum_selected_arm_seed_count);
  hash_size(
      builder, budget.maximum_selected_key_point_reference_count);
  hash_size(builder, budget.maximum_resolved_key_count);
}

void hash_effective_budget_policy(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectMorseEffectiveBudgetPolicy& policy) {
  hash_u64(builder, policy.device_limit_bytes);
  hash_u64(builder, policy.host_limit_bytes);
  hash_u64(builder, policy.scratch_limit_bytes);
  hash_u64(builder, policy.output_limit_bytes);
  hash_u64(builder, policy.time_limit_ns);
}

void hash_probe_budget(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparsePositiveFacetProbeBudget& budget) {
  hash_size(builder, budget.maximum_slot_visit_count);
  hash_size(builder, budget.maximum_component_parent_hop_count);
}

void hash_closure_budget(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparseFacetDescentClosureBudget& budget) {
  hash_size(builder, budget.maximum_seed_count);
  hash_size(builder, budget.maximum_node_count);
  hash_size(builder, budget.maximum_step_call_count);
  hash_size(builder, budget.maximum_memo_slot_count);
  hash_probe_budget(builder, budget.step_budget.source_locator_probe);
  hash_size(
      builder,
      budget.step_budget.top_k_query.maximum_node_visit_count);
  hash_size(
      builder,
      budget.step_budget.top_k_query
          .maximum_internal_node_expansion_count);
  hash_size(
      builder,
      budget.step_budget.top_k_query
          .maximum_exact_aabb_bound_evaluation_count);
  hash_size(
      builder,
      budget.step_budget.top_k_query
          .maximum_exact_point_distance_evaluation_count);
  hash_size(
      builder,
      budget.step_budget.top_k_query.maximum_frontier_entry_count);
  hash_size(
      builder,
      budget.step_budget.top_k_query
          .maximum_best_neighbor_entry_count);
  hash_size(
      builder,
      budget.step_budget.top_k_query
          .maximum_cutoff_shell_entry_count);
  hash_probe_budget(
      builder, budget.step_budget.successor_locator_probe);
}

void hash_industrial_config(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectMorseIndustrialPlanConfig& config) {
  hash_u8(builder, static_cast<std::uint8_t>(config.policy));
  const auto& memory = config.memory_model;
  hash_u64(builder, memory.fixed_chunk_bytes);
  hash_u64(builder, memory.checkpoint_boundary_bytes);
  hash_u64(builder, memory.external_run_boundary_bytes);
  hash_u64(builder, memory.bytes_per_batch);
  hash_u64(builder, memory.bytes_per_birth);
  hash_u64(builder, memory.bytes_per_saddle);
  hash_u64(builder, memory.bytes_per_arm);
  hash_u64(builder, memory.bytes_per_key_point_reference);
  hash_u64(builder, memory.bytes_per_node_upper_bound);
  hash_u64(builder, memory.bytes_per_child_reference_upper_bound);
  hash_u64(builder, memory.bytes_per_descent_node_reserve);
  hash_u64(builder, memory.descent_node_reserve_per_arm);
  const auto& chunk = config.chunk_budget;
  hash_u64(builder, chunk.maximum_bytes);
  hash_u64(builder, chunk.maximum_batch_count);
  hash_u64(builder, chunk.maximum_birth_count);
  hash_u64(builder, chunk.maximum_saddle_count);
  hash_u64(builder, chunk.maximum_arm_count);
  hash_u64(builder, chunk.maximum_descent_node_count);
}

void hash_plan_budget(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparseFacetDescentBatchPlanBudget& budget) {
  hash_size(builder, budget.maximum_source_chunk_count);
  hash_size(builder, budget.maximum_source_batch_count);
  hash_size(builder, budget.maximum_source_family_count);
  hash_size(builder, budget.maximum_source_arm_seed_count);
  hash_size(builder, budget.maximum_lane_count);
  hash_size(builder, budget.maximum_initial_seed_launch_count);
  hash_size(
      builder,
      budget
          .maximum_initial_seed_standalone_step_support_examination_count);
  hash_size(
      builder,
      budget.maximum_initial_seed_work_item_count_per_launch);
}

void hash_industrial_counters(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectMorseIndustrialCounters& counters) {
  hash_u64(builder, counters.batch_count);
  hash_u64(builder, counters.birth_count);
  hash_u64(builder, counters.saddle_count);
  hash_u64(builder, counters.arm_count);
  hash_u64(builder, counters.key_point_reference_count);
  hash_u64(builder, counters.node_count_upper_bound);
  hash_u64(builder, counters.child_reference_count_upper_bound);
  hash_u64(builder, counters.descent_node_reserve_count);
}

[[nodiscard]] contract::CanonicalId application_digest(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectMorseEventJournalResult& event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_arm_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& arm_journal,
    const ExactDirectMorseIndustrialPlanConfig& industrial_config,
    const ExactDirectSparseFacetDescentBatchPlanBudget& plan_budget,
    const ExactDirectSparseFacetDescentBatchPlanResult& source_plan,
    const ExactDirectSparseFacetDescentClosureConfig& closure_config,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectMorseChunkRunLimits& limits,
    const ExactDirectMorseEffectiveBudgetPolicy& budget_policy,
    std::span<const ExactDirectMorseChunkBatchReplayAuthority>
        authorities) {
  contract::CanonicalSha256Builder builder;
  hash_text(builder, application_digest_domain);
  hash_u32(builder, direct_morse_chunk_run_schema_version);
  hash_text(builder, direct_morse_chunk_run_backend);
  hash_text(builder, direct_morse_chunk_run_profile);
  hash_text(builder, direct_morse_chunk_run_mode);
  hash_text(builder, direct_morse_chunk_run_deployment_status);
  hash_text(builder, direct_morse_chunk_run_public_status);
  hash_u8(builder, static_cast<std::uint8_t>(traversal_order));
  hash_u64(builder, closure_config.memo_fingerprint_mask);
  hash_industrial_config(builder, industrial_config);
  hash_plan_budget(builder, plan_budget);
  hash_size(builder, limits.maximum_batch_count_per_chunk);
  hash_size(builder, limits.maximum_science_byte_count_per_batch);
  hash_size(
      builder, limits.maximum_total_resolved_key_count_per_chunk);
  hash_size(builder, limits.maximum_total_arm_join_count_per_chunk);
  hash_size(builder, limits.maximum_payload_byte_count);
  hash_u64(builder, limits.serialization_reserved_ns);
  hash_u64(builder, limits.checkpoint_reserved_ns);
  hash_u64(builder, limits.safety_margin_reserved_bytes);
  hash_size(builder, sizeof(ParsedBatch));
  hash_size(
      builder, atomic_linear_run_transition_fixed_wire_byte_count);
  hash_size(builder, atomic_linear_run_head_wire_byte_count);
  hash_effective_budget_policy(builder, budget_policy);
  hash_size(
      builder,
      trusted_arm_seed_budget
          .maximum_source_journal_replay_entry_count);
  hash_size(builder, trusted_arm_seed_budget.maximum_role_scan_count);
  hash_size(
      builder,
      trusted_arm_seed_budget.maximum_saddle_family_count);
  hash_size(builder, trusted_arm_seed_budget.maximum_arm_seed_count);

  hash_size(builder, cloud.size());
  for (std::size_t index = 0U; index < cloud.size(); ++index) {
    const auto point_id = static_cast<spatial::PointId>(index);
    for (const std::uint64_t bits :
         cloud.point(point_id).input_bits()) {
      hash_u64(builder, bits);
    }
    hash_size(builder, cloud.source_index(point_id));
  }

  hash_id(builder, event_journal.source_pair_semantic_digest);
  hash_id(builder, event_journal.source_higher_semantic_digest);
  hash_size(builder, event_journal.batches.size());
  for (const auto& batch : event_journal.batches) {
    hash_size(builder, batch.batch_index);
    hash_size(builder, batch.order);
    hash_level(builder, batch.squared_level);
    hash_size(builder, batch.role_record_offset);
    hash_size(builder, batch.role_record_count);
    hash_size(builder, batch.birth_role_count);
    hash_size(builder, batch.saddle_role_count);
  }

  hash_id(builder, arm_journal.source_pair_canonical_cloud_digest);
  hash_id(builder, arm_journal.source_higher_canonical_cloud_digest);
  hash_id(builder, arm_journal.source_pair_semantic_digest);
  hash_id(builder, arm_journal.source_higher_semantic_digest);
  hash_size(builder, arm_journal.families.size());
  for (const auto& family : arm_journal.families) {
    hash_size(builder, family.family_index);
    hash_size(builder, family.source_event_index);
    hash_size(builder, family.journal_event_projection_index);
    hash_size(builder, family.journal_role_record_index);
    hash_size(builder, family.journal_batch_index);
    hash_size(builder, family.order);
    hash_level(builder, family.critical_squared_level);
    hash_size(builder, family.arm_seed_offset);
    hash_size(builder, family.arm_seed_count);
    hash_id(builder, family.source_event_arm_identity_digest);
  }
  hash_size(builder, arm_journal.arm_seeds.size());
  for (const auto& seed : arm_journal.arm_seeds) {
    hash_size(builder, seed.arm_seed_index);
    hash_size(builder, seed.family_index);
    hash_u64(builder, seed.removed_support_point_id);
  }

  hash_size(
      builder, source_plan.source_industrial_plan.chunks.size());
  for (const auto& chunk :
       source_plan.source_industrial_plan.chunks) {
    hash_size(builder, chunk.chunk_index);
    hash_size(builder, chunk.source_batch_begin_index);
    hash_size(builder, chunk.source_batch_end_index);
    hash_industrial_counters(builder, chunk.counters);
    hash_u64(builder, chunk.estimated_byte_count);
    hash_bool(builder, chunk.checkpoint_boundary_after_chunk);
    hash_bool(builder, chunk.external_run_boundary_after_chunk);
  }
  hash_size(builder, source_plan.lanes.size());
  for (const auto& lane : source_plan.lanes) {
    hash_size(builder, lane.lane_index);
    hash_size(builder, lane.source_chunk_index);
    hash_size(builder, lane.source_batch_index);
    hash_size(builder, lane.candidate_family_begin_index);
    hash_size(builder, lane.candidate_family_end_index);
    hash_size(builder, lane.candidate_arm_seed_begin_index);
    hash_size(builder, lane.candidate_arm_seed_end_index);
    hash_size(builder, lane.facet_cardinality);
    hash_size(builder, lane.source_support_cardinality);
    hash_size(builder, lane.source_interior_cardinality);
    hash_size(builder, lane.matching_family_count);
    hash_size(builder, lane.matching_arm_seed_count);
    hash_size(builder, lane.initial_seed_work_item_count);
    hash_size(builder, lane.initial_seed_launch_count);
    hash_size(
        builder,
        lane
            .initial_seed_standalone_step_support_examination_count_upper_bound);
  }

  hash_size(builder, authorities.size());
  for (const auto& authority : authorities) {
    const auto& stamp = authority.strict_pre_batch_locator_stamp;
    hash_u32(builder, stamp.schema_version);
    hash_u64(builder, stamp.external_authority_id);
    hash_size(builder, stamp.committed_batch_count);
    hash_size(builder, stamp.inserted_key_count);
    hash_size(builder, stamp.component_union_count);
    hash_size(builder, stamp.binding_count);
    hash_id(builder, stamp.committed_history_digest);
    hash_u64(
        builder, authority.locator_query_witness.external_authority_id);
    hash_u64(builder, authority.locator_query_witness.replay_token);
    hash_execution_budget(builder, authority.execution_budget);
    hash_closure_budget(builder, authority.closure_budget);
    hash_u64(
        builder, authority.resource_envelope.device_reserved_bytes);
    hash_u64(
        builder, authority.resource_envelope.host_reserved_bytes);
    hash_u64(
        builder, authority.resource_envelope.scratch_reserved_bytes);
    hash_u64(
        builder, authority.resource_envelope.operation_reserved_ns);
  }
  return builder.finalize();
}

[[nodiscard]] std::vector<
    detail::ExactDirectSparseFacetDescentBatchReplayCursor>
build_batch_cursors(
    const ExactDirectMorseEventJournalResult& event_journal,
    const ExactDirectSaddleArmSeedJournalResult& arm_journal,
    const ExactDirectSparseFacetDescentBatchPlanResult& source_plan) {
  std::vector<
      detail::ExactDirectSparseFacetDescentBatchReplayCursor>
      cursors;
  cursors.reserve(event_journal.batches.size());
  std::size_t chunk_index = 0U;
  std::size_t lane_index = 0U;
  std::size_t family_index = 0U;
  std::size_t arm_seed_index = 0U;
  const auto& chunks = source_plan.source_industrial_plan.chunks;

  for (std::size_t batch_index = 0U;
       batch_index < event_journal.batches.size();
       ++batch_index) {
    while (chunk_index < chunks.size() &&
           batch_index >=
               chunks[chunk_index].source_batch_end_index) {
      ++chunk_index;
    }
    if (chunk_index >= chunks.size() ||
        batch_index <
            chunks[chunk_index].source_batch_begin_index) {
      throw std::invalid_argument(
          "a Phase-15 context cannot index its 14A chunks");
    }
    while (lane_index < source_plan.lanes.size() &&
           source_plan.lanes[lane_index].source_batch_index <
               batch_index) {
      ++lane_index;
    }
    while (family_index < arm_journal.families.size() &&
           arm_journal.families[family_index].journal_batch_index <
               batch_index) {
      const auto& family = arm_journal.families[family_index];
      if (family.arm_seed_offset != arm_seed_index) {
        throw std::invalid_argument(
            "a Phase-15 context found a noncontiguous arm family");
      }
      arm_seed_index = checked_add(
          arm_seed_index, family.arm_seed_count);
      ++family_index;
    }
    cursors.push_back({
        batch_index,
        chunk_index,
        lane_index,
        family_index,
        arm_seed_index,
    });
    while (lane_index < source_plan.lanes.size() &&
           source_plan.lanes[lane_index].source_batch_index ==
               batch_index) {
      ++lane_index;
    }
    while (family_index < arm_journal.families.size() &&
           arm_journal.families[family_index].journal_batch_index ==
               batch_index) {
      const auto& family = arm_journal.families[family_index];
      if (family.arm_seed_offset != arm_seed_index) {
        throw std::invalid_argument(
            "a Phase-15 context found a noncontiguous arm family");
      }
      arm_seed_index = checked_add(
          arm_seed_index, family.arm_seed_count);
      ++family_index;
    }
  }
  if (lane_index != source_plan.lanes.size() ||
      family_index != arm_journal.families.size() ||
      arm_seed_index != arm_journal.arm_seeds.size()) {
    throw std::invalid_argument(
        "a Phase-15 context did not index every 14C source record");
  }
  return cursors;
}

[[nodiscard]] bool valid_limits(
    const ExactDirectMorseChunkRunLimits& limits) noexcept {
  return limits.maximum_batch_count_per_chunk != 0U &&
         limits.maximum_science_byte_count_per_batch != 0U &&
         limits.maximum_payload_byte_count != 0U &&
         limits.serialization_reserved_ns != 0U &&
         limits.checkpoint_reserved_ns != 0U;
}

[[nodiscard]] bool valid_resource_envelopes(
    std::span<const ExactDirectMorseChunkBatchReplayAuthority>
        authorities) noexcept {
  return std::ranges::all_of(
      authorities,
      [](const ExactDirectMorseChunkBatchReplayAuthority& authority) {
        return authority.resource_envelope.operation_reserved_ns != 0U;
      });
}

[[nodiscard]] ExactDirectMorseBudgetDemand required_budget_demand(
    const ExactDirectMorseIndustrialChunk& chunk,
    std::span<const ExactDirectMorseChunkBatchReplayAuthority>
        authorities,
    const ExactDirectMorseChunkRunLimits& limits,
    const ExactDirectMorseChunkRunSessionUsage& session_usage,
    bool durable_publication,
    std::uint64_t sequence) {
  if (chunk.source_batch_begin_index >
          chunk.source_batch_end_index ||
      chunk.source_batch_end_index > authorities.size()) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run budget interval is invalid");
  }

  std::uint64_t maximum_device_bytes = 0U;
  std::uint64_t maximum_host_bytes = 0U;
  std::uint64_t maximum_scratch_bytes = 0U;
  std::uint64_t replay_time_ns = 0U;
  for (std::size_t batch_index =
           chunk.source_batch_begin_index;
       batch_index < chunk.source_batch_end_index;
       ++batch_index) {
    const auto& envelope =
        authorities[batch_index].resource_envelope;
    maximum_device_bytes =
        std::max(maximum_device_bytes, envelope.device_reserved_bytes);
    maximum_host_bytes =
        std::max(maximum_host_bytes, envelope.host_reserved_bytes);
    maximum_scratch_bytes =
        std::max(maximum_scratch_bytes, envelope.scratch_reserved_bytes);
    replay_time_ns = checked_add_u64(
        replay_time_ns, envelope.operation_reserved_ns);
  }

  const std::uint64_t payload_cap =
      wire_size(limits.maximum_payload_byte_count);
  const std::uint64_t science_cap =
      wire_size(limits.maximum_science_byte_count_per_batch);
  const std::uint64_t batch_count = wire_size(
      chunk.source_batch_end_index -
      chunk.source_batch_begin_index);
  const std::uint64_t batch_metadata_bytes = checked_mul_u64(
      batch_count, wire_size(sizeof(ParsedBatch)));

  // Logical peak of the application layer: retained science, one bounded
  // current science segment, the canonical output image, batch metadata and
  // the largest sequential resolver/replay envelope.  Allocator slack and
  // the generic store's own transition copy remain outside this bound.
  std::uint64_t host_bytes = checked_mul_u64(payload_cap, 2U);
  host_bytes = checked_add_u64(host_bytes, science_cap);
  host_bytes = checked_add_u64(host_bytes, batch_metadata_bytes);
  host_bytes = checked_add_u64(host_bytes, maximum_host_bytes);

  const std::uint64_t transition_bytes = checked_add_u64(
      wire_size(atomic_linear_run_transition_fixed_wire_byte_count),
      payload_cap);
  const std::uint64_t durable_bytes = checked_add_u64(
      transition_bytes,
      wire_size(atomic_linear_run_head_wire_byte_count));

  ExactDirectMorseBudgetDemand demand;
  demand.device.used_bytes = session_usage.device_used_bytes;
  demand.device.reserved_bytes = maximum_device_bytes;
  demand.host.used_bytes = session_usage.host_used_bytes;
  demand.host.reserved_bytes = host_bytes;
  demand.scratch.used_bytes = session_usage.scratch_used_bytes;
  demand.scratch.temporary_reserved_bytes = checked_add_u64(
      science_cap, maximum_scratch_bytes);
  demand.scratch.checkpoint_reserved_bytes =
      durable_publication ? durable_bytes : 0U;
  demand.scratch.safety_margin_reserved_bytes =
      limits.safety_margin_reserved_bytes;
  demand.output.used_bytes = session_usage.output_used_bytes;
  if (durable_publication) {
    demand.output.used_bytes = checked_add_u64(
        checked_add_u64(
            demand.output.used_bytes,
            checked_mul_u64(sequence, transition_bytes)),
        wire_size(atomic_linear_run_head_wire_byte_count));
    demand.output.reserved_bytes = durable_bytes;
  }
  demand.time.operation_reserved_ns = checked_add_u64(
      replay_time_ns, limits.serialization_reserved_ns);
  demand.time.checkpoint_reserved_ns =
      durable_publication ? limits.checkpoint_reserved_ns : 0U;
  return demand;
}

[[nodiscard]] ExactDirectMorseBudgetDemand
initial_store_budget_demand(
    const ExactDirectMorseChunkRunLimits& limits,
    const ExactDirectMorseChunkRunSessionUsage& session_usage) {
  ExactDirectMorseBudgetDemand demand;
  demand.device.used_bytes = session_usage.device_used_bytes;
  demand.host.used_bytes = session_usage.host_used_bytes;
  demand.scratch.used_bytes = session_usage.scratch_used_bytes;
  demand.scratch.checkpoint_reserved_bytes =
      wire_size(atomic_linear_run_head_wire_byte_count);
  demand.scratch.safety_margin_reserved_bytes =
      limits.safety_margin_reserved_bytes;
  demand.output.used_bytes = session_usage.output_used_bytes;
  demand.output.reserved_bytes =
      wire_size(atomic_linear_run_head_wire_byte_count);
  demand.time.checkpoint_reserved_ns =
      limits.checkpoint_reserved_ns;
  return demand;
}

[[nodiscard]] ExactDirectMorseBudgetDemand post_operation_demand(
    ExactDirectMorseBudgetDemand demand) noexcept {
  demand.time.operation_reserved_ns = 0U;
  return demand;
}

[[nodiscard]] bool completed_within_reserved_time(
    const ExactDirectMorseBudgetSnapshot& before,
    const ExactDirectMorseBudgetSnapshot& after,
    bool include_checkpoint_reserve) noexcept {
  const std::uint64_t operation_reserved_ns =
      before.requested_demand.time.operation_reserved_ns;
  const std::uint64_t checkpoint_reserved_ns =
      include_checkpoint_reserve
          ? before.requested_demand.time.checkpoint_reserved_ns
          : 0U;
  if (operation_reserved_ns >
      std::numeric_limits<std::uint64_t>::max() -
          checkpoint_reserved_ns) {
    return false;
  }
  const std::uint64_t total_reserved_ns =
      operation_reserved_ns + checkpoint_reserved_ns;
  return after.accepted() &&
         after.time.elapsed_ns >= before.time.elapsed_ns &&
         after.time.elapsed_ns - before.time.elapsed_ns <=
             total_reserved_ns;
}

}  // namespace

struct ExactDirectMorseChunkRunContext::Impl {
  const spatial::MortonLbvhIndex* index{};
  const spatial::CanonicalPointCloud* cloud{};
  const ExactDirectSupportTerminalFacade* source_facade{};
  const ExactDirectMorseEventJournalResult* source_event_journal{};
  ExactDirectSaddleArmSeedBudget trusted_arm_seed_budget{};
  const ExactDirectSaddleArmSeedJournalResult*
      source_arm_seed_journal{};
  ExactDirectMorseIndustrialPlanConfig industrial_config{};
  ExactDirectSparseFacetDescentBatchPlanBudget plan_budget{};
  ExactDirectSparseFacetDescentBatchPlanResult source_plan{};
  ExactDirectMorseChunkBatchLocatorResolverView locator_resolver;
  std::vector<ExactDirectMorseChunkBatchReplayAuthority>
      batch_authorities;
  ExactDirectMorseChunkRunLimits limits{};
  std::unique_ptr<ExactDirectMorseBudgetTracker> budget_tracker;
  ExactDirectMorseChunkRunSessionUsage session_usage{};
  std::mutex budget_mutex;
  std::mutex resource_gate_state_mutex;
  ExactDirectSparseFacetDescentClosureConfig closure_config{};
  spatial::LbvhTraversalOrder traversal_order{
      spatial::LbvhTraversalOrder::near_first};
  std::vector<
      detail::ExactDirectSparseFacetDescentBatchReplayCursor>
      batch_cursors;
  contract::CanonicalId application_contract_digest{};
  std::atomic_size_t arbitrary_batch_replay_count{};
  std::atomic_size_t publication_recertification_count{};
  std::atomic_size_t recovery_recertification_count{};
  std::atomic_size_t cleanup_recertification_count{};
  std::atomic_size_t rejected_recertification_count{};
  std::atomic_size_t maximum_chunk_resolved_key_count{};
  std::atomic_size_t maximum_chunk_arm_join_count{};
  std::atomic_size_t maximum_retained_batch_count{};
  std::atomic_size_t payload_pre_retention_refusal_count{};
  std::atomic_size_t maximum_live_external_locator_count{};
  std::atomic_size_t fresh_budget_evaluation_count{};
  std::atomic_size_t rejected_budget_evaluation_count{};

  struct ActiveBudgetGate {
    std::uint64_t sequence{};
    AtomicLinearRunRecertificationPhase phase{
        AtomicLinearRunRecertificationPhase::publication};
    ExactDirectMorseBudgetDemand demand{};
    ExactDirectMorseBudgetSnapshot before{};
    std::unique_lock<std::mutex> budget_lock;
  };
  std::optional<ActiveBudgetGate> active_budget_gate;

  static void observe_maximum(
      std::atomic_size_t& target,
      std::size_t observed) noexcept {
    std::size_t current = target.load(std::memory_order_relaxed);
    while (current < observed &&
           !target.compare_exchange_weak(
               current,
               observed,
               std::memory_order_relaxed,
               std::memory_order_relaxed)) {
    }
  }

  [[nodiscard]] ExactDirectMorseBudgetSnapshot evaluate_budget(
      ExactDirectMorseBudgetBoundary boundary,
      std::uint64_t sequence,
      const ExactDirectMorseBudgetDemand& demand) {
    fresh_budget_evaluation_count.fetch_add(
        1U, std::memory_order_relaxed);
    ExactDirectMorseBudgetSnapshot snapshot =
        budget_tracker->snapshot(boundary, sequence, demand);
    if (!snapshot.accepted()) {
      rejected_budget_evaluation_count.fetch_add(
          1U, std::memory_order_relaxed);
      throw std::length_error(
          "a Phase-15 fresh session budget rejected the operation");
    }
    return snapshot;
  }

  void finish_budget(
      std::uint64_t sequence,
      const ExactDirectMorseBudgetDemand& demand,
      const ExactDirectMorseBudgetSnapshot& before,
      bool include_checkpoint_reserve) {
    const ExactDirectMorseBudgetSnapshot after = evaluate_budget(
        ExactDirectMorseBudgetBoundary::checkpoint,
        sequence,
        post_operation_demand(demand));
    if (!completed_within_reserved_time(
            before, after, include_checkpoint_reserve)) {
      rejected_budget_evaluation_count.fetch_add(
          1U, std::memory_order_relaxed);
      throw std::length_error(
          "a Phase-15 operation exceeded its reserved time");
    }
  }

  [[nodiscard]] bool gate_resources(
      const AtomicLinearRunTransition& transition,
      AtomicLinearRunRecertificationPhase phase,
      AtomicLinearRunResourceGateBoundary boundary) noexcept {
    try {
      if (boundary ==
          AtomicLinearRunResourceGateBoundary::
              before_recertification) {
        if (transition.chunk_index >=
                source_plan.source_industrial_plan.chunks.size()) {
          return false;
        }
        const auto& chunk =
            source_plan.source_industrial_plan
                .chunks[checked_size(transition.chunk_index)];
        if (transition.batch_begin_index !=
                wire_size(chunk.source_batch_begin_index) ||
            transition.batch_end_index !=
                wire_size(chunk.source_batch_end_index)) {
          return false;
        }
        const ExactDirectMorseBudgetDemand demand =
            required_budget_demand(
                chunk,
                batch_authorities,
                limits,
                session_usage,
                phase ==
                    AtomicLinearRunRecertificationPhase::publication,
                transition.sequence);
        std::unique_lock state_lock{resource_gate_state_mutex};
        if (active_budget_gate.has_value()) {
          rejected_budget_evaluation_count.fetch_add(
              1U, std::memory_order_relaxed);
          return false;
        }
        std::unique_lock budget_lock{budget_mutex};
        const ExactDirectMorseBudgetSnapshot before =
            evaluate_budget(
                ExactDirectMorseBudgetBoundary::before_run,
                transition.sequence,
                demand);
        active_budget_gate.emplace(ActiveBudgetGate{
            transition.sequence,
            phase,
            demand,
            before,
            std::move(budget_lock)});
        return true;
      }

      std::optional<ActiveBudgetGate> completed;
      {
        std::unique_lock state_lock{resource_gate_state_mutex};
        if (!active_budget_gate.has_value() ||
            active_budget_gate->sequence != transition.sequence ||
            active_budget_gate->phase != phase) {
          rejected_budget_evaluation_count.fetch_add(
              1U, std::memory_order_relaxed);
          return false;
        }
        completed.emplace(std::move(*active_budget_gate));
        active_budget_gate.reset();
      }
      finish_budget(
          completed->sequence,
          completed->demand,
          completed->before,
          true);
      return true;
    } catch (...) {
      return false;
    }
  }

  [[nodiscard]] ExactDirectSparseFacetDescentBatchExecutionResult
  replay_batch(std::size_t batch_index) {
    if (batch_index >= batch_cursors.size()) {
      throw std::out_of_range(
          "a Phase-15 context batch index is out of range");
    }
    const auto& authority = batch_authorities[batch_index];
    std::shared_ptr<const ExactDirectSparsePositiveFacetLocator>
        locator = locator_resolver(
            batch_index, authority.strict_pre_batch_locator_stamp);
    if (!locator || !locator->certified_positive_locator() ||
        locator->snapshot_stamp() !=
            authority.strict_pre_batch_locator_stamp) {
      throw std::invalid_argument(
          "a Phase-15 locator resolver returned the wrong pre-batch prefix");
    }
    observe_maximum(maximum_live_external_locator_count, 1U);
    const auto before = locator->snapshot_stamp();
    const auto result =
        detail::replay_exact_direct_sparse_facet_descent_batch_at_cursor(
            *index,
            *cloud,
            *source_facade,
            *source_event_journal,
            *source_arm_seed_journal,
            source_plan,
            batch_cursors[batch_index],
            authority.locator_query_witness,
            *locator,
            authority.execution_budget,
            authority.closure_budget,
            closure_config,
            traversal_order);
    if (before != locator->snapshot_stamp() ||
        result.locator_snapshot_stamp !=
            authority.strict_pre_batch_locator_stamp) {
      throw std::invalid_argument(
          "a Phase-15 batch replay did not preserve its pre-batch locator");
    }
    arbitrary_batch_replay_count.fetch_add(
        1U, std::memory_order_relaxed);
    return result;
  }

  [[nodiscard]] AtomicLinearRunRecertification recertify(
      const AtomicLinearRunTransition& transition,
      AtomicLinearRunRecertificationPhase phase) {
    switch (phase) {
      case AtomicLinearRunRecertificationPhase::publication:
        publication_recertification_count.fetch_add(
            1U, std::memory_order_relaxed);
        break;
      case AtomicLinearRunRecertificationPhase::recovery:
        recovery_recertification_count.fetch_add(
            1U, std::memory_order_relaxed);
        break;
      case AtomicLinearRunRecertificationPhase::
          recovery_uncommitted_cleanup:
        cleanup_recertification_count.fetch_add(
            1U, std::memory_order_relaxed);
        break;
    }

    AtomicLinearRunRecertification certification;
    try {
      if (transition.chunk_index >=
              source_plan.source_industrial_plan.chunks.size()) {
        throw std::invalid_argument(
            "a Phase-15 transition chunk is outside its fresh plan");
      }
      const auto& expected_chunk =
          source_plan.source_industrial_plan
              .chunks[checked_size(transition.chunk_index)];
      if (transition.batch_begin_index !=
              wire_size(expected_chunk.source_batch_begin_index) ||
          transition.batch_end_index !=
              wire_size(expected_chunk.source_batch_end_index)) {
        throw std::invalid_argument(
            "a Phase-15 transition interval is not one complete 14A chunk");
      }
      ExactDirectMorseBudgetDemand persisted_demand =
          required_budget_demand(
              expected_chunk,
              batch_authorities,
              limits,
              {},
              true,
              transition.sequence);
      const ParsedPayload parsed =
          decode_payload(transition.payload, limits);
      if (parsed.budget_snapshot.requested_demand.output.used_bytes <
          persisted_demand.output.used_bytes) {
        throw std::invalid_argument(
            "a Phase-15 historical output usage omits its durable prefix");
      }
      persisted_demand.device.used_bytes =
          parsed.budget_snapshot.requested_demand.device.used_bytes;
      persisted_demand.host.used_bytes =
          parsed.budget_snapshot.requested_demand.host.used_bytes;
      persisted_demand.scratch.used_bytes =
          parsed.budget_snapshot.requested_demand.scratch.used_bytes;
      persisted_demand.output.used_bytes =
          parsed.budget_snapshot.requested_demand.output.used_bytes;
      observe_maximum(
          maximum_retained_batch_count, parsed.batches.size());
      std::size_t resolved_key_count = 0U;
      std::size_t arm_join_count = 0U;
      for (const ParsedBatch& batch : parsed.batches) {
        resolved_key_count = checked_add(
            resolved_key_count, batch.shape.resolved_key_count);
        arm_join_count = checked_add(
            arm_join_count, batch.shape.arm_join_count);
      }
      observe_maximum(
          maximum_chunk_resolved_key_count, resolved_key_count);
      observe_maximum(maximum_chunk_arm_join_count, arm_join_count);
      if (parsed.application_contract_digest !=
              application_contract_digest ||
          transition.chunk_index != parsed.chunk.chunk_index ||
          transition.batch_begin_index !=
              parsed.chunk.source_batch_begin_index ||
          transition.batch_end_index !=
              parsed.chunk.source_batch_end_index ||
          transition.budget_snapshot_digest !=
              parsed.budget_snapshot.snapshot_digest ||
          transition.sequence !=
              parsed.budget_snapshot.sequence_number ||
          !valid_budget_snapshot(
              parsed.budget_snapshot,
              transition.sequence,
              budget_tracker->policy(),
              persisted_demand) ||
          parsed.chunk != expected_chunk) {
        throw std::invalid_argument(
            "a Phase-15 transition does not match its fresh 14A chunk");
      }
      for (std::size_t index = 0U;
           index < parsed.batches.size();
           ++index) {
        const std::size_t batch_index =
            parsed.chunk.source_batch_begin_index + index;
        const auto fresh = replay_batch(batch_index);
        const auto fresh_science = encode_science(
            fresh, limits.maximum_science_byte_count_per_batch);
        if (fresh_science != parsed.batches[index].science) {
          throw std::invalid_argument(
              "a Phase-15 transition science differs from fresh 14D");
        }
      }
      const auto canonical_payload = encode_payload(
          parsed.application_contract_digest,
          parsed.chunk,
          parsed.budget_snapshot,
          parsed.batches,
          limits.maximum_payload_byte_count);
      if (canonical_payload != transition.payload ||
          transition.successor_checkpoint_digest !=
              checkpoint_digest(
                  application_contract_digest,
                  transition.source_checkpoint_digest,
                  parsed.chunk,
                  transition.payload)) {
        throw std::invalid_argument(
            "a Phase-15 transition checkpoint is not canonical");
      }
      certification.transition_recertified = true;
      certification.payload_is_canonical = true;
      certification.process_local_capability_absent = true;
    } catch (...) {
      rejected_recertification_count.fetch_add(
          1U, std::memory_order_relaxed);
    }
    return certification;
  }
};

ExactDirectMorseChunkRunContext::ExactDirectMorseChunkRunContext(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_arm_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_seed_journal,
    const ExactDirectMorseIndustrialPlanConfig& industrial_config,
    const ExactDirectSparseFacetDescentBatchPlanBudget& plan_budget,
    const ExactDirectSparseFacetDescentBatchPlanResult& observed_plan,
    ExactDirectMorseChunkBatchLocatorResolverView locator_resolver,
    std::span<const ExactDirectMorseChunkBatchReplayAuthority>
        batch_authorities,
    ExactDirectMorseChunkRunLimits limits,
    std::unique_ptr<ExactDirectMorseBudgetTracker> budget_tracker,
    ExactDirectMorseChunkRunSessionUsage session_usage,
    const ExactDirectSparseFacetDescentClosureConfig& closure_config,
    spatial::LbvhTraversalOrder traversal_order)
    : impl_(std::make_shared<Impl>()) {
  if (!index.validated_for(cloud) ||
      !locator_resolver ||
      !budget_tracker ||
      !known_traversal_order(traversal_order) ||
      !valid_limits(limits) ||
      !valid_resource_envelopes(batch_authorities) ||
      batch_authorities.size() !=
          source_event_journal.batches.size()) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run context authority is invalid");
  }
  impl_->index = &index;
  impl_->cloud = &cloud;
  impl_->source_facade = &source_facade;
  impl_->source_event_journal = &source_event_journal;
  impl_->trusted_arm_seed_budget = trusted_arm_seed_budget;
  impl_->source_arm_seed_journal = &source_arm_seed_journal;
  impl_->industrial_config = industrial_config;
  impl_->plan_budget = plan_budget;
  impl_->locator_resolver = locator_resolver;
  impl_->batch_authorities.assign(
      batch_authorities.begin(), batch_authorities.end());
  impl_->limits = limits;
  impl_->budget_tracker = std::move(budget_tracker);
  impl_->session_usage = session_usage;
  impl_->closure_config = closure_config;
  impl_->traversal_order = traversal_order;
  impl_->source_plan =
      build_exact_direct_sparse_facet_descent_batch_plan(
          cloud,
          source_facade,
          source_event_journal,
          trusted_arm_seed_budget,
          source_arm_seed_journal,
          industrial_config,
          plan_budget);
  if (!impl_->source_plan.complete_architecture_plan() ||
      impl_->source_plan != observed_plan) {
    throw std::invalid_argument(
        "a Phase-15 chunk-run context requires one fresh matching 14C plan");
  }
  for (const auto& chunk :
       impl_->source_plan.source_industrial_plan.chunks) {
    if (chunk.source_batch_end_index -
            chunk.source_batch_begin_index >
        limits.maximum_batch_count_per_chunk) {
      throw std::invalid_argument(
          "a Phase-15 14A chunk exceeds the durable batch cap");
    }
  }
  impl_->batch_cursors = build_batch_cursors(
      source_event_journal,
      source_arm_seed_journal,
      impl_->source_plan);
  impl_->application_contract_digest = application_digest(
      cloud,
      source_event_journal,
      trusted_arm_seed_budget,
      source_arm_seed_journal,
      industrial_config,
      plan_budget,
      impl_->source_plan,
      closure_config,
      traversal_order,
      limits,
      impl_->budget_tracker->policy(),
      impl_->batch_authorities);
}

ExactDirectMorseChunkRunContext::~ExactDirectMorseChunkRunContext() =
    default;

const contract::CanonicalId&
ExactDirectMorseChunkRunContext::application_contract_digest()
    const noexcept {
  return impl_->application_contract_digest;
}

AtomicLinearRunContract ExactDirectMorseChunkRunContext::run_contract(
    contract::CanonicalId initial_checkpoint_digest,
    contract::CanonicalId initial_output_chain_digest) const {
  AtomicLinearRunContract contract;
  contract.application_contract_digest =
      impl_->application_contract_digest;
  contract.initial_checkpoint_digest =
      std::move(initial_checkpoint_digest);
  contract.initial_output_chain_digest =
      std::move(initial_output_chain_digest);
  if (!impl_->source_plan.source_industrial_plan.chunks.empty()) {
    const auto& first =
        impl_->source_plan.source_industrial_plan.chunks.front();
    contract.initial_chunk_index = wire_size(first.chunk_index);
    contract.initial_batch_index =
        wire_size(first.source_batch_begin_index);
  }
  return contract;
}

AtomicLinearRunStore ExactDirectMorseChunkRunContext::create_new_store(
    const std::filesystem::path& dedicated_directory,
    contract::CanonicalId initial_checkpoint_digest,
    contract::CanonicalId initial_output_chain_digest,
    AtomicLinearRunStoreLimits store_limits) const {
  std::unique_lock budget_lock{impl_->budget_mutex};
  static_cast<void>(impl_->evaluate_budget(
      ExactDirectMorseBudgetBoundary::before_serialization,
      0U,
      initial_store_budget_demand(
          impl_->limits, impl_->session_usage)));
  const std::shared_ptr<Impl> shared = impl_;
  return AtomicLinearRunStore::create_new(
      dedicated_directory,
      run_contract(
          std::move(initial_checkpoint_digest),
          std::move(initial_output_chain_digest)),
      std::move(store_limits),
      [shared](
          const AtomicLinearRunTransition& transition,
          AtomicLinearRunRecertificationPhase phase) {
        return shared->recertify(transition, phase);
      },
      [shared](
          const AtomicLinearRunTransition& transition,
          AtomicLinearRunRecertificationPhase phase,
          AtomicLinearRunResourceGateBoundary boundary) {
        return shared->gate_resources(
            transition, phase, boundary);
      });
}

AtomicLinearRunStore
ExactDirectMorseChunkRunContext::open_existing_store(
    const std::filesystem::path& dedicated_directory,
    contract::CanonicalId initial_checkpoint_digest,
    contract::CanonicalId initial_output_chain_digest,
    AtomicLinearRunStoreLimits store_limits,
    std::optional<AtomicLinearRunExternalAnchor> expected_anchor) const {
  const std::shared_ptr<Impl> shared = impl_;
  return AtomicLinearRunStore::open_existing(
      dedicated_directory,
      run_contract(
          std::move(initial_checkpoint_digest),
          std::move(initial_output_chain_digest)),
      std::move(store_limits),
      [shared](
          const AtomicLinearRunTransition& transition,
          AtomicLinearRunRecertificationPhase phase) {
        return shared->recertify(transition, phase);
      },
      [shared](
          const AtomicLinearRunTransition& transition,
          AtomicLinearRunRecertificationPhase phase,
          AtomicLinearRunResourceGateBoundary boundary) {
        return shared->gate_resources(
            transition, phase, boundary);
      },
      std::move(expected_anchor));
}

AtomicLinearRunRecertification
ExactDirectMorseChunkRunContext::recertify_for_validation(
    const AtomicLinearRunTransition& transition,
    AtomicLinearRunRecertificationPhase phase) const {
  return impl_->recertify(transition, phase);
}

AtomicLinearRunChunkProposal
ExactDirectMorseChunkRunContext::prepare_chunk(
    const AtomicLinearRunTrustedState& trusted_state,
    std::size_t chunk_index) const {
  if (chunk_index >=
          impl_->source_plan.source_industrial_plan.chunks.size() ||
      trusted_state.next_chunk_index != wire_size(chunk_index)) {
    throw std::invalid_argument(
        "a Phase-15 chunk preparation is not the trusted next chunk");
  }
  const auto& chunk =
      impl_->source_plan.source_industrial_plan.chunks[chunk_index];
  if (trusted_state.next_batch_index !=
          wire_size(chunk.source_batch_begin_index)) {
    throw std::invalid_argument(
        "a Phase-15 chunk preparation cursor is invalid");
  }
  const ExactDirectMorseBudgetDemand budget_demand =
      required_budget_demand(
          chunk,
          impl_->batch_authorities,
          impl_->limits,
          impl_->session_usage,
          true,
          trusted_state.next_sequence);
  std::unique_lock budget_lock{impl_->budget_mutex};
  const ExactDirectMorseBudgetSnapshot budget_snapshot =
      impl_->evaluate_budget(
          ExactDirectMorseBudgetBoundary::before_run,
          trusted_state.next_sequence,
          budget_demand);

  const std::size_t batch_count =
      chunk.source_batch_end_index -
      chunk.source_batch_begin_index;
  std::size_t projected_payload_byte_count = 0U;
  try {
    projected_payload_byte_count =
        encode_payload(
            impl_->application_contract_digest,
            chunk,
            budget_snapshot,
            std::span<const ParsedBatch>{},
            impl_->limits.maximum_payload_byte_count)
            .size();
  } catch (const std::length_error&) {
    impl_->payload_pre_retention_refusal_count.fetch_add(
        1U, std::memory_order_relaxed);
    throw;
  }
  for (std::size_t index = 0U; index < batch_count; ++index) {
    projected_payload_byte_count = checked_add(
        projected_payload_byte_count, sizeof(std::uint64_t));
    if (projected_payload_byte_count >
        impl_->limits.maximum_payload_byte_count) {
      impl_->payload_pre_retention_refusal_count.fetch_add(
          1U, std::memory_order_relaxed);
      throw std::length_error(
          "a Phase-15 chunk payload lengths exceed their cap");
    }
  }

  std::vector<ParsedBatch> batches;
  batches.reserve(batch_count);
  std::size_t total_resolved_key_count = 0U;
  std::size_t total_arm_join_count = 0U;
  for (std::size_t batch_index =
           chunk.source_batch_begin_index;
       batch_index < chunk.source_batch_end_index;
       ++batch_index) {
    const auto fresh = impl_->replay_batch(batch_index);
    ParsedBatch batch;
    const std::size_t remaining_payload_byte_count =
        impl_->limits.maximum_payload_byte_count -
        projected_payload_byte_count;
    try {
      batch.science = encode_science(
          fresh,
          std::min(
              impl_->limits.maximum_science_byte_count_per_batch,
              remaining_payload_byte_count));
    } catch (const std::length_error&) {
      if (remaining_payload_byte_count <=
          impl_->limits.maximum_science_byte_count_per_batch) {
        impl_->payload_pre_retention_refusal_count.fetch_add(
            1U, std::memory_order_relaxed);
      }
      throw;
    }
    batch.shape = validate_science(
        batch.science,
        impl_->limits.maximum_science_byte_count_per_batch);
    projected_payload_byte_count = checked_add(
        projected_payload_byte_count, batch.science.size());
    if (projected_payload_byte_count >
        impl_->limits.maximum_payload_byte_count) {
      throw std::length_error(
          "a Phase-15 chunk science exceeds its payload cap");
    }
    total_resolved_key_count = checked_add(
        total_resolved_key_count, batch.shape.resolved_key_count);
    total_arm_join_count = checked_add(
        total_arm_join_count, batch.shape.arm_join_count);
    if (total_resolved_key_count >
            impl_->limits
                .maximum_total_resolved_key_count_per_chunk ||
        total_arm_join_count >
            impl_->limits.maximum_total_arm_join_count_per_chunk) {
      throw std::length_error(
          "a Phase-15 chunk preparation exceeds its science caps");
    }
    batches.push_back(std::move(batch));
    Impl::observe_maximum(
        impl_->maximum_retained_batch_count, batches.size());
  }
  Impl::observe_maximum(
      impl_->maximum_chunk_resolved_key_count,
      total_resolved_key_count);
  Impl::observe_maximum(
      impl_->maximum_chunk_arm_join_count,
      total_arm_join_count);
  AtomicLinearRunChunkProposal proposal;
  proposal.chunk_index = wire_size(chunk.chunk_index);
  proposal.batch_begin_index =
      wire_size(chunk.source_batch_begin_index);
  proposal.batch_end_index =
      wire_size(chunk.source_batch_end_index);
  proposal.budget_snapshot_digest = budget_snapshot.snapshot_digest;
  proposal.payload = encode_payload(
      impl_->application_contract_digest,
      chunk,
      budget_snapshot,
      batches,
      impl_->limits.maximum_payload_byte_count);
  if (proposal.payload.size() != projected_payload_byte_count) {
    throw std::logic_error(
        "a Phase-15 chunk payload preflight disagrees with encoding");
  }
  proposal.successor_checkpoint_digest = checkpoint_digest(
      impl_->application_contract_digest,
      trusted_state.checkpoint_digest,
      chunk,
      proposal.payload);
  impl_->finish_budget(
      trusted_state.next_sequence,
      budget_demand,
      budget_snapshot,
      false);
  return proposal;
}

AtomicLinearRunPublishResult
ExactDirectMorseChunkRunContext::publish_next_chunk(
    AtomicLinearRunStore& store,
    AtomicLinearRunPublishOptions options) const {
  const AtomicLinearRunTrustedState trusted_state =
      store.trusted_state();
  const std::size_t chunk_index =
      checked_size(trusted_state.next_chunk_index);
  return store.publish_next(
      prepare_chunk(trusted_state, chunk_index),
      options);
}

ExactDirectMorseChunkRunAudit
ExactDirectMorseChunkRunContext::audit() const noexcept {
  ExactDirectMorseChunkRunAudit audit;
  audit.source_plan_reconstruction_count = 1U;
  audit.indexed_batch_cursor_count = impl_->batch_cursors.size();
  audit.arbitrary_batch_replay_count =
      impl_->arbitrary_batch_replay_count.load(
          std::memory_order_relaxed);
  audit.publication_recertification_count =
      impl_->publication_recertification_count.load(
          std::memory_order_relaxed);
  audit.recovery_recertification_count =
      impl_->recovery_recertification_count.load(
          std::memory_order_relaxed);
  audit.cleanup_recertification_count =
      impl_->cleanup_recertification_count.load(
          std::memory_order_relaxed);
  audit.rejected_recertification_count =
      impl_->rejected_recertification_count.load(
          std::memory_order_relaxed);
  audit.maximum_chunk_resolved_key_count =
      impl_->maximum_chunk_resolved_key_count.load(
          std::memory_order_relaxed);
  audit.maximum_chunk_arm_join_count =
      impl_->maximum_chunk_arm_join_count.load(
          std::memory_order_relaxed);
  audit.maximum_retained_batch_count =
      impl_->maximum_retained_batch_count.load(
          std::memory_order_relaxed);
  audit.payload_pre_retention_refusal_count =
      impl_->payload_pre_retention_refusal_count.load(
          std::memory_order_relaxed);
  audit.maximum_live_external_locator_count =
      impl_->maximum_live_external_locator_count.load(
          std::memory_order_relaxed);
  audit.fresh_budget_evaluation_count =
      impl_->fresh_budget_evaluation_count.load(
          std::memory_order_relaxed);
  audit.rejected_budget_evaluation_count =
      impl_->rejected_budget_evaluation_count.load(
          std::memory_order_relaxed);
  return audit;
}

}  // namespace morsehgp3d::hierarchy
