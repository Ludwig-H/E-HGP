#include "morsehgp3d/hierarchy/sparse_anchored_pair_chunk_run.hpp"

#include "morsehgp3d/exact/integer.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <limits>
#include <mutex>
#include <new>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::array<std::uint8_t, 16U> payload_magic{
    'M', 'H', '3', 'D', '1', '4', 'V', '-',
    'P', 'A', 'I', 'R', 'R', 'U', 'N', 0U};
constexpr std::string_view application_domain =
    "MorseHGP3D/phase14V/sparse-anchored-pair/application/v1";
constexpr std::string_view initial_checkpoint_domain =
    "MorseHGP3D/phase14V/sparse-anchored-pair/checkpoint/initial/v1";
constexpr std::string_view checkpoint_domain =
    "MorseHGP3D/phase14V/sparse-anchored-pair/checkpoint/transition/v1";

static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));
static_assert(sizeof(spatial::PointId) <= sizeof(std::uint64_t));

[[nodiscard]] std::size_t checked_size(std::uint64_t value) {
  if (value > std::numeric_limits<std::size_t>::max()) {
    throw std::invalid_argument("a Phase-14V index does not fit size_t");
  }
  return static_cast<std::size_t>(value);
}

[[nodiscard]] std::uint64_t wire_size(std::size_t value) noexcept {
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
}

void hash_u8(
    contract::CanonicalSha256Builder& builder,
    std::uint8_t value) {
  const std::array<std::uint8_t, 1U> bytes{value};
  builder.update(bytes);
}

void hash_u32(
    contract::CanonicalSha256Builder& builder,
    std::uint32_t value) {
  std::array<std::uint8_t, 4U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    bytes[index] = static_cast<std::uint8_t>(
        value >> ((bytes.size() - 1U - index) * 8U));
  }
  builder.update(bytes);
}

void hash_u64(
    contract::CanonicalSha256Builder& builder,
    std::uint64_t value) {
  std::array<std::uint8_t, 8U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    bytes[index] = static_cast<std::uint8_t>(
        value >> ((bytes.size() - 1U - index) * 8U));
  }
  builder.update(bytes);
}

void hash_size(
    contract::CanonicalSha256Builder& builder,
    std::size_t value) {
  hash_u64(builder, wire_size(value));
}

void hash_bool(
    contract::CanonicalSha256Builder& builder,
    bool value) {
  hash_u8(builder, value ? std::uint8_t{1U} : std::uint8_t{0U});
}

void hash_text(
    contract::CanonicalSha256Builder& builder,
    std::string_view text) {
  hash_size(builder, text.size());
  builder.update(text);
}

void hash_id(
    contract::CanonicalSha256Builder& builder,
    const contract::CanonicalId& value) {
  builder.update(value.bytes());
}

class Writer {
 public:
  explicit Writer(const ExactSparseAnchoredPairChunkRunLimits& limits)
      : limits_(limits) {}

  void bytes(std::span<const std::uint8_t> input) {
    const std::size_t next = checked_add(
        data_.size(), input.size(), "a Phase-14V payload size overflows");
    if (next > limits_.maximum_payload_byte_count) {
      throw std::length_error("a Phase-14V payload exceeds its cap");
    }
    data_.insert(data_.end(), input.begin(), input.end());
  }

  void u8(std::uint8_t value) {
    const std::array<std::uint8_t, 1U> encoded{value};
    bytes(encoded);
  }

  void boolean(bool value) { u8(value ? 1U : 0U); }

  void u32(std::uint32_t value) {
    std::array<std::uint8_t, 4U> encoded{};
    for (std::size_t index = 0U; index < encoded.size(); ++index) {
      encoded[index] = static_cast<std::uint8_t>(
          value >> ((encoded.size() - 1U - index) * 8U));
    }
    bytes(encoded);
  }

  void u64(std::uint64_t value) {
    std::array<std::uint8_t, 8U> encoded{};
    for (std::size_t index = 0U; index < encoded.size(); ++index) {
      encoded[index] = static_cast<std::uint8_t>(
          value >> ((encoded.size() - 1U - index) * 8U));
    }
    bytes(encoded);
  }

  void size(std::size_t value) { u64(wire_size(value)); }

  void id(const contract::CanonicalId& value) { bytes(value.bytes()); }

  void integer(const exact::BigInt& value) {
    std::string encoded = exact::canonical_integer_string(value);
    if (encoded.empty() ||
        encoded.size() > limits_.maximum_exact_integer_byte_count) {
      throw std::length_error(
          "a Phase-14V exact integer exceeds its cap");
    }
    exact_byte_count_ = checked_add(
        exact_byte_count_, encoded.size(),
        "a Phase-14V exact-text count overflows");
    if (exact_byte_count_ >
        limits_.maximum_total_exact_integer_byte_count_per_chunk) {
      throw std::length_error(
          "a Phase-14V exact-text population exceeds its cap");
    }
    size(encoded.size());
    bytes(std::span<const std::uint8_t>{
        reinterpret_cast<const std::uint8_t*>(encoded.data()),
        encoded.size()});
  }

  [[nodiscard]] std::vector<std::uint8_t> finish() && {
    return std::move(data_);
  }

 private:
  const ExactSparseAnchoredPairChunkRunLimits& limits_;
  std::size_t exact_byte_count_{};
  std::vector<std::uint8_t> data_;
};

void encode_center(Writer& writer, const exact::ExactCenter3& center) {
  writer.integer(center.numerator(0U));
  writer.integer(center.numerator(1U));
  writer.integer(center.numerator(2U));
  writer.integer(center.denominator());
}

void encode_level(Writer& writer, const exact::ExactLevel& level) {
  writer.integer(level.numerator());
  writer.integer(level.denominator());
}

void encode_point_ids(
    Writer& writer,
    std::span<const spatial::PointId> point_ids) {
  writer.size(point_ids.size());
  for (const spatial::PointId point_id : point_ids) {
    writer.u64(static_cast<std::uint64_t>(point_id));
  }
}

void encode_record(
    Writer& writer,
    const ExactSparseAnchoredPairRecord& record) {
  std::visit(
      [&writer](const auto& value) {
        using Value = std::remove_cvref_t<decltype(value)>;
        if constexpr (std::is_same_v<Value, ExactPairSupportEvent>) {
          writer.u8(1U);
          writer.u64(static_cast<std::uint64_t>(value.support_ids[0]));
          writer.u64(static_cast<std::uint64_t>(value.support_ids[1]));
          encode_center(writer, value.center);
          encode_level(writer, value.squared_level);
          encode_point_ids(writer, value.interior_ids);
          writer.size(value.closed_rank);
          writer.size(value.exterior_count);
        } else {
          writer.u8(2U);
          writer.u64(static_cast<std::uint64_t>(value.support_ids[0]));
          writer.u64(static_cast<std::uint64_t>(value.support_ids[1]));
          encode_center(writer, value.center);
          encode_level(writer, value.squared_level);
          encode_point_ids(writer, value.interior_ids);
          writer.size(value.shell_count);
          writer.u64(static_cast<std::uint64_t>(
              value.canonical_extra_shell_witness_id));
          writer.size(value.minimum_possible_closed_rank);
          writer.size(value.observed_closed_rank);
          writer.size(value.exterior_count);
        }
      },
      record);
}

struct ReplayChunk {
  std::size_t source_advance_count{};
  std::size_t successor_advance_count{};
  ExactSparseAnchoredPairRecordSegment segment;
  ExactSparseAnchoredPairSessionAudit audit{};
  ExactSparseAnchoredPairSessionStepKind last_step_kind{
      ExactSparseAnchoredPairSessionStepKind::budget_exhausted};
  ExactSparseAnchoredPairSessionStopReason last_stop_reason{
      ExactSparseAnchoredPairSessionStopReason::none};
  bool session_complete{false};
  bool total_capacity_exhausted{false};
  bool run_advance_limit_reached{false};
};

[[nodiscard]] std::vector<std::uint8_t> encode_payload(
    const contract::CanonicalId& application_digest,
    const ReplayChunk& chunk,
    const ExactSparseAnchoredPairChunkRunLimits& limits) {
  Writer writer{limits};
  writer.bytes(payload_magic);
  writer.u32(sparse_anchored_pair_chunk_run_schema_version);
  writer.id(application_digest);
  writer.size(chunk.source_advance_count);
  writer.size(chunk.successor_advance_count);
  writer.size(chunk.segment.first_output_record_index);
  writer.size(chunk.segment.first_output_point_id_reference_index);
  writer.size(chunk.segment.event_count);
  writer.size(chunk.segment.relevant_extra_shell_diagnostic_count);
  writer.size(chunk.segment.point_id_reference_count);
  writer.u8(static_cast<std::uint8_t>(chunk.last_step_kind));
  writer.u8(static_cast<std::uint8_t>(chunk.last_stop_reason));
  writer.boolean(chunk.session_complete);
  writer.boolean(chunk.total_capacity_exhausted);
  writer.boolean(chunk.run_advance_limit_reached);
  writer.size(chunk.audit.advance_call_count);
  writer.size(chunk.audit.emitted_record_count);
  writer.size(chunk.audit.emitted_point_id_reference_count);
  writer.size(chunk.audit.accepted_event_count);
  writer.size(chunk.audit.relevant_extra_shell_diagnostic_count);
  writer.size(chunk.segment.records.size());
  for (const ExactSparseAnchoredPairRecord& record : chunk.segment.records) {
    encode_record(writer, record);
  }
  return std::move(writer).finish();
}

[[nodiscard]] contract::CanonicalId checkpoint_digest(
    const contract::CanonicalId& application_digest,
    const contract::CanonicalId& source_checkpoint_digest,
    std::uint64_t chunk_index,
    const ReplayChunk& chunk,
    std::span<const std::uint8_t> payload) {
  contract::CanonicalSha256Builder builder;
  hash_text(builder, checkpoint_domain);
  hash_id(builder, application_digest);
  hash_id(builder, source_checkpoint_digest);
  hash_u64(builder, chunk_index);
  hash_size(builder, chunk.source_advance_count);
  hash_size(builder, chunk.successor_advance_count);
  hash_bool(builder, chunk.session_complete);
  hash_bool(builder, chunk.total_capacity_exhausted);
  hash_bool(builder, chunk.run_advance_limit_reached);
  hash_size(builder, payload.size());
  builder.update(payload);
  return builder.finalize();
}

[[nodiscard]] bool valid_limits(
    std::size_t maximum_closed_rank,
    const ExactSparseAnchoredPairSessionAdvanceBudget& budget,
    const ExactSparseAnchoredPairChunkRunLimits& limits) noexcept {
  return maximum_closed_rank >= 2U &&
      maximum_closed_rank <=
          exact_anchored_pair_candidate_maximum_closed_rank &&
      budget.candidate_cursor.maximum_schedule_advance_count > 0U &&
      budget.candidate_cursor.maximum_orientation_check_count > 0U &&
      budget.candidate_cursor
              .maximum_grouped_traversal_node_visit_count > 0U &&
      budget.candidate_cursor
              .maximum_grouped_traversal_exact_predicate_count > 0U &&
      budget.classifier.maximum_node_visit_count > 0U &&
      budget.maximum_emitted_record_count > 0U &&
      budget.maximum_emitted_point_id_reference_count >=
          maximum_closed_rank + 1U &&
      limits.maximum_advance_count_per_chunk > 0U &&
      limits.maximum_cumulative_advance_count > 0U &&
      limits.maximum_record_count_per_chunk > 0U &&
      limits.maximum_point_id_reference_count_per_chunk >=
          maximum_closed_rank + 1U &&
      limits.maximum_exact_integer_byte_count > 0U &&
      limits.maximum_total_exact_integer_byte_count_per_chunk >=
          limits.maximum_exact_integer_byte_count &&
      limits.maximum_payload_byte_count >= 256U;
}

[[nodiscard]] contract::CanonicalId application_digest(
    const ExactPairSupportCheckpointManifest& manifest,
    std::size_t maximum_closed_rank,
    const ExactMortonGroupedAnchoredPairScheduleConfig& config,
    const ExactSparseAnchoredPairSessionTotalCapacity& capacity,
    const ExactSparseAnchoredPairSessionAdvanceBudget& budget,
    const ExactSparseAnchoredPairChunkRunLimits& limits) {
  contract::CanonicalSha256Builder builder;
  hash_text(builder, application_domain);
  hash_u32(builder, sparse_anchored_pair_chunk_run_schema_version);
  hash_text(builder, sparse_anchored_pair_chunk_run_backend);
  hash_text(builder, sparse_anchored_pair_chunk_run_profile);
  hash_text(builder, sparse_anchored_pair_chunk_run_mode);
  hash_text(builder, sparse_anchored_pair_chunk_run_deployment_status);
  hash_text(builder, sparse_anchored_pair_chunk_run_public_status);
  hash_id(builder, manifest.canonical_cloud_digest);
  hash_id(builder, manifest.lbvh_digest);
  hash_id(builder, manifest.semantic_digest);
  hash_size(builder, manifest.point_count);
  hash_size(builder, manifest.lbvh_node_count);
  hash_size(builder, manifest.lbvh_leaf_count);
  hash_size(builder, maximum_closed_rank);
  hash_size(builder, config.maximum_anchor_count_per_group);
  hash_size(builder, config.proposed_witness_pool_size);
  hash_bool(builder, config.use_triangular_block_pair_schedule);
  hash_bool(
      builder, config.use_symmetric_inconclusive_cross_block_splitting);
  hash_bool(builder, config.prioritize_cross_blocks);
  hash_bool(builder, config.use_witness_subtree_first_for_triangular_blocks);
  hash_bool(builder, config.use_floating_witness_order_for_triangular_blocks);
  hash_size(builder, capacity.maximum_schedule_advance_count);
  hash_size(builder, capacity.maximum_orientation_check_count);
  hash_size(builder, capacity.maximum_grouped_traversal_node_visit_count);
  hash_size(
      builder, capacity.maximum_grouped_traversal_exact_predicate_count);
  hash_size(builder, capacity.maximum_admitted_candidate_count);
  hash_size(builder, capacity.maximum_classification_node_visit_count);
  hash_size(builder, capacity.maximum_output_record_count);
  hash_size(builder, capacity.maximum_output_point_id_reference_count);
  hash_size(builder, budget.candidate_cursor.maximum_schedule_advance_count);
  hash_size(
      builder, budget.candidate_cursor.maximum_orientation_check_count);
  hash_size(
      builder,
      budget.candidate_cursor.maximum_grouped_traversal_node_visit_count);
  hash_size(
      builder,
      budget.candidate_cursor.maximum_grouped_traversal_exact_predicate_count);
  hash_size(builder, budget.classifier.maximum_node_visit_count);
  hash_size(builder, budget.maximum_emitted_record_count);
  hash_size(builder, budget.maximum_emitted_point_id_reference_count);
  hash_size(builder, limits.maximum_advance_count_per_chunk);
  hash_size(builder, limits.maximum_cumulative_advance_count);
  hash_size(builder, limits.maximum_record_count_per_chunk);
  hash_size(builder, limits.maximum_point_id_reference_count_per_chunk);
  hash_size(builder, limits.maximum_exact_integer_byte_count);
  hash_size(
      builder, limits.maximum_total_exact_integer_byte_count_per_chunk);
  hash_size(builder, limits.maximum_payload_byte_count);
  return builder.finalize();
}

}  // namespace

struct ExactSparseAnchoredPairChunkRunContext::Impl {
  const ExactPairSupportAuthorityContext* authority{};
  std::size_t maximum_closed_rank{};
  ExactMortonGroupedAnchoredPairScheduleConfig schedule_config{};
  ExactSparseAnchoredPairSessionTotalCapacity total_capacity{};
  ExactSparseAnchoredPairSessionAdvanceBudget advance_budget{};
  ExactSparseAnchoredPairChunkRunLimits limits{};
  contract::CanonicalId application_contract_digest{};
  contract::CanonicalId initial_checkpoint_digest{};

  struct ReplayCache {
    std::optional<ExactSparseAnchoredPairSession> session;
    std::size_t advance_count{};
  };

  mutable ReplayCache producer_cache;
  mutable ReplayCache verifier_cache;
  mutable std::mutex producer_mutex;
  mutable std::mutex verifier_mutex;
  mutable std::atomic_size_t producer_root_reconstruction_count{};
  mutable std::atomic_size_t verifier_root_reconstruction_count{};
  mutable std::atomic_size_t producer_advance_count{};
  mutable std::atomic_size_t verifier_advance_count{};
  mutable std::atomic_size_t prepared_chunk_count{};
  mutable std::atomic_size_t durably_published_chunk_count{};
  mutable std::atomic_size_t publication_recertification_count{};
  mutable std::atomic_size_t recovery_recertification_count{};
  mutable std::atomic_size_t cleanup_recertification_count{};
  mutable std::atomic_size_t rejected_recertification_count{};
  mutable std::atomic_size_t maximum_retained_record_count{};
  mutable std::atomic_size_t maximum_retained_reference_count{};

  static void observe_maximum(
      std::atomic_size_t& target,
      std::size_t value) noexcept {
    std::size_t current = target.load(std::memory_order_relaxed);
    while (current < value &&
           !target.compare_exchange_weak(
               current, value, std::memory_order_relaxed,
               std::memory_order_relaxed)) {
    }
  }

  void reset_cache(ReplayCache& cache, bool verifier) const {
    cache.session.reset();
    cache.session.emplace(ExactSparseAnchoredPairSession::start(
        authority->index(), authority->cloud(), maximum_closed_rank,
        schedule_config, total_capacity));
    cache.advance_count = 0U;
    (verifier ? verifier_root_reconstruction_count
              : producer_root_reconstruction_count)
        .fetch_add(1U, std::memory_order_relaxed);
  }

  void discard_released_segment(ReplayCache& cache) const {
    ExactSparseAnchoredPairRecordSegment discarded =
        cache.session->release_unsealed_record_segment();
    if (discarded.record_count() > limits.maximum_record_count_per_chunk ||
        discarded.point_id_reference_count >
            limits.maximum_point_id_reference_count_per_chunk) {
      throw std::length_error(
          "a Phase-14V replay step exceeds the segment caps");
    }
  }

  void replay_to(
      ReplayCache& cache,
      std::size_t target,
      bool verifier) const {
    if (target > limits.maximum_cumulative_advance_count) {
      throw std::length_error(
          "a Phase-14V replay target exceeds its cumulative cap");
    }
    if (!cache.session.has_value() || cache.advance_count > target) {
      reset_cache(cache, verifier);
    }
    while (cache.advance_count < target) {
      if (cache.session->complete() ||
          cache.session->total_capacity_exhausted() ||
          cache.session->poisoned()) {
        throw std::invalid_argument(
            "a Phase-14V replay target lies beyond its terminal session");
      }
      static_cast<void>(cache.session->advance(
          authority->index(), authority->cloud(), advance_budget));
      ++cache.advance_count;
      discard_released_segment(cache);
      (verifier ? verifier_advance_count : producer_advance_count)
          .fetch_add(1U, std::memory_order_relaxed);
    }
  }

  [[nodiscard]] ReplayChunk build_chunk(
      ReplayCache& cache,
      std::size_t source_advance_count,
      bool verifier) const {
    replay_to(cache, source_advance_count, verifier);
    if (cache.session->complete() ||
        cache.session->total_capacity_exhausted() ||
        cache.session->poisoned() ||
        source_advance_count >= limits.maximum_cumulative_advance_count) {
      throw std::invalid_argument(
          "a Phase-14V terminal checkpoint has no successor chunk");
    }

    ReplayChunk chunk;
    chunk.source_advance_count = source_advance_count;
    chunk.successor_advance_count = source_advance_count;
    chunk.segment.first_output_record_index =
        cache.session->audit().emitted_record_count;
    chunk.segment.first_output_point_id_reference_index =
        cache.session->audit().emitted_point_id_reference_count;

    for (std::size_t local = 0U;
         local < limits.maximum_advance_count_per_chunk &&
         chunk.successor_advance_count <
             limits.maximum_cumulative_advance_count;
         ++local) {
      const std::size_t remaining_records =
          limits.maximum_record_count_per_chunk -
          chunk.segment.record_count();
      const std::size_t remaining_references =
          limits.maximum_point_id_reference_count_per_chunk -
          chunk.segment.point_id_reference_count;
      if (remaining_records == 0U ||
          remaining_references < maximum_closed_rank + 1U) {
        break;
      }

      ExactSparseAnchoredPairSessionStep step =
          cache.session->advance(
              authority->index(), authority->cloud(), advance_budget);
      ++cache.advance_count;
      ++chunk.successor_advance_count;
      (verifier ? verifier_advance_count : producer_advance_count)
          .fetch_add(1U, std::memory_order_relaxed);
      chunk.last_step_kind = step.kind();
      chunk.last_stop_reason = step.stop_reason();

      ExactSparseAnchoredPairRecordSegment released =
          cache.session->release_unsealed_record_segment();
      if (released.first_output_record_index !=
              chunk.segment.first_output_record_index +
                  chunk.segment.record_count() ||
          released.first_output_point_id_reference_index !=
              chunk.segment.first_output_point_id_reference_index +
                  chunk.segment.point_id_reference_count ||
          released.record_count() > remaining_records ||
          released.point_id_reference_count > remaining_references) {
        throw std::logic_error(
            "a Phase-14V released segment is not a bounded continuation");
      }
      chunk.segment.event_count = checked_add(
          chunk.segment.event_count, released.event_count,
          "a Phase-14V event count overflows");
      chunk.segment.relevant_extra_shell_diagnostic_count = checked_add(
          chunk.segment.relevant_extra_shell_diagnostic_count,
          released.relevant_extra_shell_diagnostic_count,
          "a Phase-14V diagnostic count overflows");
      chunk.segment.point_id_reference_count = checked_add(
          chunk.segment.point_id_reference_count,
          released.point_id_reference_count,
          "a Phase-14V point-reference count overflows");
      for (ExactSparseAnchoredPairRecord& record : released.records) {
        chunk.segment.records.push_back(std::move(record));
      }

      if (cache.session->complete() ||
          cache.session->total_capacity_exhausted()) {
        break;
      }
    }

    if (chunk.successor_advance_count == source_advance_count) {
      throw std::length_error(
          "a Phase-14V chunk cannot make one bounded advance");
    }
    chunk.audit = cache.session->audit();
    chunk.session_complete = cache.session->complete();
    chunk.total_capacity_exhausted =
        cache.session->total_capacity_exhausted();
    chunk.run_advance_limit_reached =
        !chunk.session_complete && !chunk.total_capacity_exhausted &&
        chunk.successor_advance_count ==
            limits.maximum_cumulative_advance_count;
    observe_maximum(
        maximum_retained_record_count, chunk.segment.record_count());
    observe_maximum(
        maximum_retained_reference_count,
        chunk.segment.point_id_reference_count);
    return chunk;
  }

  [[nodiscard]] AtomicLinearRunRecertification recertify(
      const AtomicLinearRunTransition& transition,
      AtomicLinearRunRecertificationPhase phase) const {
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
      const std::size_t source = checked_size(
          transition.batch_begin_index);
      const std::size_t successor = checked_size(
          transition.batch_end_index);
      if (transition.chunk_index != transition.sequence ||
          successor <= source ||
          successor - source > limits.maximum_advance_count_per_chunk) {
        throw std::invalid_argument(
            "a Phase-14V transition shape is invalid");
      }

      ReplayChunk expected;
      if (phase == AtomicLinearRunRecertificationPhase::
                       recovery_uncommitted_cleanup) {
        ReplayCache scratch;
        expected = build_chunk(scratch, source, true);
      } else {
        std::lock_guard lock{verifier_mutex};
        expected = build_chunk(verifier_cache, source, true);
      }
      const std::vector<std::uint8_t> canonical = encode_payload(
          application_contract_digest, expected, limits);
      if (expected.successor_advance_count != successor ||
          canonical != transition.payload ||
          transition.successor_checkpoint_digest != checkpoint_digest(
              application_contract_digest,
              transition.source_checkpoint_digest,
              transition.chunk_index,
              expected,
              canonical)) {
        throw std::invalid_argument(
            "a Phase-14V transition differs from fresh P8l replay");
      }

      auto projection = std::make_shared<
          ExactSparseAnchoredPairRecertifiedChunkProjection>();
      projection->source_advance_count = expected.source_advance_count;
      projection->successor_advance_count =
          expected.successor_advance_count;
      projection->record_segment = std::move(expected.segment);
      projection->cumulative_audit = expected.audit;
      projection->last_step_kind = expected.last_step_kind;
      projection->last_stop_reason = expected.last_stop_reason;
      projection->session_complete = expected.session_complete;
      projection->total_capacity_exhausted =
          expected.total_capacity_exhausted;
      projection->run_advance_limit_reached =
          expected.run_advance_limit_reached;
      certification = AtomicLinearRunRecertification{
          true, true, true, std::move(projection), false};
    } catch (const std::bad_alloc&) {
      certification.operational_resource_failure = true;
      rejected_recertification_count.fetch_add(
          1U, std::memory_order_relaxed);
    } catch (...) {
      rejected_recertification_count.fetch_add(
          1U, std::memory_order_relaxed);
    }
    return certification;
  }
};

ExactSparseAnchoredPairChunkRunContext::
    ExactSparseAnchoredPairChunkRunContext(
        const ExactPairSupportAuthorityContext& authority,
        std::size_t maximum_closed_rank,
        ExactMortonGroupedAnchoredPairScheduleConfig schedule_config,
        ExactSparseAnchoredPairSessionTotalCapacity total_capacity,
        ExactSparseAnchoredPairSessionAdvanceBudget advance_budget,
        ExactSparseAnchoredPairChunkRunLimits limits)
    : impl_(std::make_shared<Impl>()) {
  const ExactPairSupportCheckpointManifest& manifest = authority.manifest();
  if (manifest.maximum_relevant_closed_rank != maximum_closed_rank ||
      !valid_limits(maximum_closed_rank, advance_budget, limits)) {
    throw std::invalid_argument(
        "a Phase-14V context contract is invalid");
  }
  impl_->authority = &authority;
  impl_->maximum_closed_rank = maximum_closed_rank;
  impl_->schedule_config = schedule_config;
  impl_->total_capacity = total_capacity;
  impl_->advance_budget = advance_budget;
  impl_->limits = limits;
  impl_->application_contract_digest = application_digest(
      manifest, maximum_closed_rank, schedule_config, total_capacity,
      advance_budget, limits);
  contract::CanonicalSha256Builder builder;
  hash_text(builder, initial_checkpoint_domain);
  hash_id(builder, impl_->application_contract_digest);
  hash_size(builder, 0U);
  hash_bool(builder, false);
  impl_->initial_checkpoint_digest = builder.finalize();
}

ExactSparseAnchoredPairChunkRunContext::
    ~ExactSparseAnchoredPairChunkRunContext() = default;

const contract::CanonicalId&
ExactSparseAnchoredPairChunkRunContext::application_contract_digest()
    const noexcept {
  return impl_->application_contract_digest;
}

const contract::CanonicalId&
ExactSparseAnchoredPairChunkRunContext::initial_checkpoint_digest()
    const noexcept {
  return impl_->initial_checkpoint_digest;
}

AtomicLinearRunContract ExactSparseAnchoredPairChunkRunContext::run_contract(
    contract::CanonicalId initial_output_chain_digest) const {
  AtomicLinearRunContract contract_value;
  contract_value.application_contract_digest =
      impl_->application_contract_digest;
  contract_value.initial_checkpoint_digest =
      impl_->initial_checkpoint_digest;
  contract_value.initial_output_chain_digest =
      std::move(initial_output_chain_digest);
  contract_value.initial_chunk_index = 0U;
  contract_value.initial_batch_index = 0U;
  return contract_value;
}

AtomicLinearRunStore
ExactSparseAnchoredPairChunkRunContext::create_new_store(
    const std::filesystem::path& dedicated_directory,
    contract::CanonicalId initial_output_chain_digest,
    AtomicLinearRunStoreLimits store_limits) const {
  const std::shared_ptr<Impl> shared = impl_;
  return AtomicLinearRunStore::create_new(
      dedicated_directory,
      run_contract(std::move(initial_output_chain_digest)),
      std::move(store_limits),
      [shared](const AtomicLinearRunTransition& transition,
               AtomicLinearRunRecertificationPhase phase) {
        return shared->recertify(transition, phase);
      },
      [](const AtomicLinearRunTransition&,
         AtomicLinearRunRecertificationPhase,
         AtomicLinearRunResourceGateBoundary) { return true; });
}

AtomicLinearRunStore
ExactSparseAnchoredPairChunkRunContext::open_existing_store(
    const std::filesystem::path& dedicated_directory,
    contract::CanonicalId initial_output_chain_digest,
    AtomicLinearRunStoreLimits store_limits,
    std::optional<AtomicLinearRunExternalAnchor> expected_anchor) const {
  return open_existing_store(
      dedicated_directory, std::move(initial_output_chain_digest),
      std::move(store_limits), std::move(expected_anchor), {});
}

AtomicLinearRunStore
ExactSparseAnchoredPairChunkRunContext::open_existing_store(
    const std::filesystem::path& dedicated_directory,
    contract::CanonicalId initial_output_chain_digest,
    AtomicLinearRunStoreLimits store_limits,
    std::optional<AtomicLinearRunExternalAnchor> expected_anchor,
    ExactSparseAnchoredPairRecertifiedChunkVisitor
        committed_prefix_visitor) const {
  AtomicLinearRunCommittedPrefixVisitor visitor;
  if (committed_prefix_visitor) {
    visitor = [typed = std::move(committed_prefix_visitor)](
                  const AtomicLinearRunTransition&,
                  const AtomicLinearRunAcceptedProjection* projection) {
      const auto* accepted = dynamic_cast<const
          ExactSparseAnchoredPairRecertifiedChunkProjection*>(projection);
      if (accepted == nullptr) {
        throw std::logic_error(
            "a Phase-14V recovery projection has the wrong type");
      }
      typed(*accepted);
    };
  }
  const std::shared_ptr<Impl> shared = impl_;
  return AtomicLinearRunStore::open_existing(
      dedicated_directory,
      run_contract(std::move(initial_output_chain_digest)),
      std::move(store_limits),
      [shared](const AtomicLinearRunTransition& transition,
               AtomicLinearRunRecertificationPhase phase) {
        return shared->recertify(transition, phase);
      },
      [](const AtomicLinearRunTransition&,
         AtomicLinearRunRecertificationPhase,
         AtomicLinearRunResourceGateBoundary) { return true; },
      std::move(expected_anchor), std::move(visitor));
}

AtomicLinearRunChunkProposal
ExactSparseAnchoredPairChunkRunContext::prepare_next_chunk(
    const AtomicLinearRunTrustedState& trusted_state) const {
  if (trusted_state.next_chunk_index != trusted_state.next_sequence ||
      trusted_state.next_batch_index >
          impl_->limits.maximum_cumulative_advance_count) {
    throw std::invalid_argument(
        "a Phase-14V trusted cursor is not canonical");
  }
  const std::size_t source = checked_size(trusted_state.next_batch_index);
  std::lock_guard lock{impl_->producer_mutex};
  ReplayChunk chunk = impl_->build_chunk(
      impl_->producer_cache, source, false);
  AtomicLinearRunChunkProposal proposal;
  proposal.chunk_index = trusted_state.next_chunk_index;
  proposal.batch_begin_index = wire_size(chunk.source_advance_count);
  proposal.batch_end_index = wire_size(chunk.successor_advance_count);
  proposal.payload = encode_payload(
      impl_->application_contract_digest, chunk, impl_->limits);
  proposal.successor_checkpoint_digest = checkpoint_digest(
      impl_->application_contract_digest,
      trusted_state.checkpoint_digest,
      proposal.chunk_index,
      chunk,
      proposal.payload);
  contract::CanonicalSha256Builder budget_builder;
  hash_text(budget_builder, "MorseHGP3D/phase14V/fixed-advance-budget/v1");
  hash_id(budget_builder, impl_->application_contract_digest);
  hash_size(budget_builder, chunk.source_advance_count);
  hash_size(budget_builder, chunk.successor_advance_count);
  proposal.budget_snapshot_digest = budget_builder.finalize();
  impl_->prepared_chunk_count.fetch_add(1U, std::memory_order_relaxed);
  return proposal;
}

AtomicLinearRunPublishResult
ExactSparseAnchoredPairChunkRunContext::publish_next_chunk(
    AtomicLinearRunStore& store,
    AtomicLinearRunPublishOptions options) const {
  AtomicLinearRunPublishResult result = store.publish_next(
      prepare_next_chunk(store.trusted_state()), options);
  if (result.decision ==
      AtomicLinearRunPublishDecision::durably_published) {
    impl_->durably_published_chunk_count.fetch_add(
        1U, std::memory_order_relaxed);
  }
  return result;
}

AtomicLinearRunRecertification
ExactSparseAnchoredPairChunkRunContext::recertify_for_validation(
    const AtomicLinearRunTransition& transition,
    AtomicLinearRunRecertificationPhase phase) const {
  return impl_->recertify(transition, phase);
}

ExactSparseAnchoredPairChunkRunAudit
ExactSparseAnchoredPairChunkRunContext::audit() const noexcept {
  ExactSparseAnchoredPairChunkRunAudit result;
  result.producer_root_reconstruction_count =
      impl_->producer_root_reconstruction_count.load(
          std::memory_order_relaxed);
  result.verifier_root_reconstruction_count =
      impl_->verifier_root_reconstruction_count.load(
          std::memory_order_relaxed);
  result.producer_advance_count = impl_->producer_advance_count.load(
      std::memory_order_relaxed);
  result.verifier_advance_count = impl_->verifier_advance_count.load(
      std::memory_order_relaxed);
  result.prepared_chunk_count = impl_->prepared_chunk_count.load(
      std::memory_order_relaxed);
  result.durably_published_chunk_count =
      impl_->durably_published_chunk_count.load(
          std::memory_order_relaxed);
  result.publication_recertification_count =
      impl_->publication_recertification_count.load(
          std::memory_order_relaxed);
  result.recovery_recertification_count =
      impl_->recovery_recertification_count.load(
          std::memory_order_relaxed);
  result.cleanup_recertification_count =
      impl_->cleanup_recertification_count.load(
          std::memory_order_relaxed);
  result.rejected_recertification_count =
      impl_->rejected_recertification_count.load(
          std::memory_order_relaxed);
  result.maximum_retained_record_count =
      impl_->maximum_retained_record_count.load(
          std::memory_order_relaxed);
  result.maximum_retained_point_id_reference_count =
      impl_->maximum_retained_reference_count.load(
          std::memory_order_relaxed);
  result.maximum_live_session_cache_count = 2U;
  return result;
}

}  // namespace morsehgp3d::hierarchy
