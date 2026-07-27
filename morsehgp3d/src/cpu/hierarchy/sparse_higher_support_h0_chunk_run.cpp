#include "morsehgp3d/hierarchy/sparse_higher_support_h0_chunk_run.hpp"

#include "morsehgp3d/exact/integer.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <exception>
#include <limits>
#include <mutex>
#include <new>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::array<std::uint8_t, 16U> payload_magic{
    'M', 'H', '3', 'D', '1', '4', 'A', 'B',
    '-', 'H', 'I', 'G', 'H', 'H', '0', 0U};
constexpr std::string_view application_domain =
    "MorseHGP3D/phase14AB/higher-support-H0/application/v1";
constexpr std::string_view budget_snapshot_domain =
    "MorseHGP3D/phase14AB/higher-support-H0/fixed-budget/v1";

static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));
static_assert(sizeof(spatial::PointId) <= sizeof(std::uint64_t));

[[nodiscard]] std::size_t checked_size(std::uint64_t value) {
  if (value > std::numeric_limits<std::size_t>::max()) {
    throw std::invalid_argument("a Phase-14AB index does not fit size_t");
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

void hash_text(
    contract::CanonicalSha256Builder& builder,
    std::string_view value) {
  hash_size(builder, value.size());
  builder.update(value);
}

void hash_id(
    contract::CanonicalSha256Builder& builder,
    const contract::CanonicalId& value) {
  builder.update(value.bytes());
}

[[nodiscard]] std::size_t decimal_byte_upper_bound(
    const exact::BigInt& value) {
  if (value == 0) {
    return 1U;
  }
  const exact::BigInt magnitude = value < 0 ? -value : value;
  const std::size_t most_significant_bit =
      boost::multiprecision::msb(magnitude);
  if (most_significant_bit ==
      std::numeric_limits<std::size_t>::max()) {
    throw std::length_error(
        "a Phase-14AB exact integer bit length overflows size_t");
  }
  const std::size_t bit_count = most_significant_bit + 1U;
  // 30103 / 100000 is a strict upper approximation of log10(2).
  constexpr std::size_t numerator = 30103U;
  constexpr std::size_t denominator = 100000U;
  const std::size_t quotient = bit_count / denominator;
  const std::size_t remainder = bit_count % denominator;
  if (quotient > std::numeric_limits<std::size_t>::max() / numerator) {
    throw std::length_error(
        "a Phase-14AB decimal bound overflows size_t");
  }
  const std::size_t whole = quotient * numerator;
  const std::size_t remainder_product = remainder * numerator;
  const std::size_t fractional =
      remainder_product / denominator +
      (remainder_product % denominator == 0U ? 0U : 1U);
  return checked_add(
      checked_add(
          whole, fractional,
          "a Phase-14AB decimal bound overflows size_t"),
      value < 0 ? 1U : 0U,
      "a Phase-14AB signed decimal bound overflows size_t");
}

class Writer {
 public:
  explicit Writer(
      const ExactSparseHigherSupportH0ChunkRunLimits& limits)
      : limits_(limits) {}

  void bytes(std::span<const std::uint8_t> input) {
    const std::size_t next = checked_add(
        data_.size(), input.size(),
        "a Phase-14AB payload size overflows size_t");
    if (next > limits_.maximum_payload_byte_count) {
      throw std::length_error("a Phase-14AB payload exceeds its cap");
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
    // This rational preflight is deliberately conservative.  Its gap from
    // log10(2) can accumulate for gigantic integers, but conversion is
    // attempted only when the resulting string is bounded by cap + 1; the
    // exact canonical length is then enforced.
    if (decimal_byte_upper_bound(value) >
        limits_.maximum_exact_integer_byte_count + 1U) {
      throw std::length_error(
          "a Phase-14AB exact integer exceeds its allocation cap");
    }
    const std::string encoded = exact::canonical_integer_string(value);
    if (encoded.empty() ||
        encoded.size() > limits_.maximum_exact_integer_byte_count) {
      throw std::length_error(
          "a Phase-14AB exact integer exceeds its encoded cap");
    }
    exact_integer_byte_count_ = checked_add(
        exact_integer_byte_count_, encoded.size(),
        "a Phase-14AB exact-integer population overflows size_t");
    if (exact_integer_byte_count_ >
        limits_.maximum_total_exact_integer_byte_count_per_chunk) {
      throw std::length_error(
          "a Phase-14AB exact-integer population exceeds its cap");
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
  const ExactSparseHigherSupportH0ChunkRunLimits& limits_;
  std::size_t exact_integer_byte_count_{};
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

void encode_support(
    Writer& writer,
    std::uint8_t support_size,
    const std::array<spatial::PointId, 4U>& support_ids) {
  writer.u8(support_size);
  for (std::size_t index = 0U; index < support_ids.size(); ++index) {
    writer.u64(static_cast<std::uint64_t>(support_ids[index]));
  }
}

void encode_event(Writer& writer, const ExactHigherSupportEvent& event) {
  encode_support(writer, event.support_size, event.support_ids);
  encode_center(writer, event.center);
  encode_level(writer, event.squared_level);
  encode_point_ids(writer, event.interior_ids);
  writer.size(event.closed_rank);
  writer.size(event.exterior_count);
}

void encode_diagnostic(
    Writer& writer,
    const ExactHigherSupportExtraShellDiagnostic& diagnostic) {
  encode_support(writer, diagnostic.support_size, diagnostic.support_ids);
  encode_center(writer, diagnostic.center);
  encode_level(writer, diagnostic.squared_level);
  encode_point_ids(writer, diagnostic.interior_ids);
  writer.size(diagnostic.shell_count);
  writer.u64(static_cast<std::uint64_t>(
      diagnostic.canonical_extra_shell_witness_id));
  writer.size(diagnostic.minimum_possible_closed_rank);
  writer.size(diagnostic.observed_closed_rank);
  writer.size(diagnostic.exterior_count);
}

[[nodiscard]] std::vector<std::uint8_t> encode_payload(
    const contract::CanonicalId& application_digest,
    const ExactHigherSupportCheckpointManifest& manifest,
    const ExactHigherSupportTerminalSegment& segment,
    const ExactSparseHigherSupportH0ChunkRunLimits& limits) {
  Writer writer{limits};
  writer.bytes(payload_magic);
  writer.u32(sparse_higher_support_h0_chunk_run_schema_version);
  writer.id(application_digest);
  writer.id(manifest.semantic_digest);
  writer.u64(segment.chunk_sequence);
  writer.size(segment.first_output_record_index);
  writer.id(segment.source_checkpoint_digest);
  writer.id(segment.next_checkpoint_digest);
  writer.id(segment.previous_output_chain_digest);
  writer.id(segment.output_chain_digest);
  writer.u8(static_cast<std::uint8_t>(segment.status));
  writer.u8(static_cast<std::uint8_t>(segment.stop_reason));
  writer.size(segment.events.size());
  writer.size(segment.relevant_extra_shell_diagnostics.size());
  writer.size(segment.emitted_record_count());
  writer.size(segment.destroyed_prune_certificate_count);
  writer.size(segment.destroyed_rank_receipt_count);
  writer.size(segment.emitted_point_id_reference_count);
  for (const ExactHigherSupportEvent& event : segment.events) {
    encode_event(writer, event);
  }
  for (const ExactHigherSupportExtraShellDiagnostic& diagnostic :
       segment.relevant_extra_shell_diagnostics) {
    encode_diagnostic(writer, diagnostic);
  }
  return std::move(writer).finish();
}

[[nodiscard]] contract::CanonicalId budget_snapshot_digest(
    const contract::CanonicalId& application_digest,
    std::size_t source_chunk_count,
    std::size_t successor_chunk_count) {
  contract::CanonicalSha256Builder builder;
  hash_text(builder, budget_snapshot_domain);
  hash_id(builder, application_digest);
  hash_size(builder, source_chunk_count);
  hash_size(builder, successor_chunk_count);
  return builder.finalize();
}

[[nodiscard]] bool finite_positive(std::size_t value) noexcept {
  return value != 0U && value != std::numeric_limits<std::size_t>::max();
}

[[nodiscard]] bool valid_limits(
    const ExactHigherSupportCheckpointManifest& manifest,
    const ExactHigherSupportStreamBudget& budget,
    const ExactSparseDirectH0CandidateRunLimits& projection_limits,
    const ExactSparseHigherSupportH0ChunkRunLimits& limits) noexcept {
  const std::size_t canonical_root_count = manifest.point_count >= 4U ? 2U : 1U;
  return manifest.schema_version == higher_support_checkpoint_schema_version &&
      manifest.traversal_version == higher_support_traversal_version &&
      manifest.point_count >= 3U &&
      finite_positive(budget.maximum_work_unit_count) &&
      budget.maximum_frontier_entry_count >= canonical_root_count &&
      budget.maximum_frontier_entry_count !=
          std::numeric_limits<std::size_t>::max() &&
      finite_positive(budget.maximum_auxiliary_frontier_entry_count) &&
      finite_positive(budget.maximum_emitted_record_count) &&
      finite_positive(budget.maximum_emitted_point_id_reference_count) &&
      finite_positive(budget.maximum_prune_receipt_count) &&
      finite_positive(budget.maximum_global_closed_ball_query_count) &&
      finite_positive(budget.maximum_point_classification_count) &&
      finite_positive(projection_limits.maximum_candidate_count) &&
      finite_positive(projection_limits.maximum_diagnostic_count) &&
      finite_positive(
          projection_limits.maximum_point_id_reference_count) &&
      projection_limits.maximum_candidate_count >=
          budget.maximum_emitted_record_count &&
      projection_limits.maximum_diagnostic_count >=
          budget.maximum_emitted_record_count &&
      projection_limits.maximum_point_id_reference_count >=
          budget.maximum_emitted_point_id_reference_count &&
      finite_positive(limits.maximum_chunk_count) &&
      finite_positive(limits.maximum_exact_integer_byte_count) &&
      finite_positive(
          limits.maximum_total_exact_integer_byte_count_per_chunk) &&
      limits.maximum_total_exact_integer_byte_count_per_chunk >=
          limits.maximum_exact_integer_byte_count &&
      finite_positive(limits.maximum_payload_byte_count) &&
      limits.maximum_payload_byte_count >=
          sparse_higher_support_h0_chunk_run_fixed_payload_byte_count;
}

[[nodiscard]] contract::CanonicalId application_digest(
    const ExactHigherSupportCheckpointManifest& manifest,
    const ExactHigherSupportStreamBudget& budget,
    const ExactSparseDirectH0CandidateRunLimits& projection_limits,
    const ExactSparseHigherSupportH0ChunkRunLimits& limits) {
  contract::CanonicalSha256Builder builder;
  hash_text(builder, application_domain);
  hash_u32(builder, sparse_higher_support_h0_chunk_run_schema_version);
  hash_u32(builder, sparse_direct_h0_candidate_merge_schema_version);
  hash_u32(builder, manifest.schema_version);
  hash_u32(builder, manifest.traversal_version);
  hash_size(builder, manifest.point_count);
  hash_size(builder, manifest.lbvh_node_count);
  hash_size(builder, manifest.lbvh_leaf_count);
  hash_size(builder, manifest.requested_maximum_order);
  hash_size(builder, manifest.effective_maximum_order);
  hash_size(builder, manifest.maximum_relevant_closed_rank);
  hash_id(builder, manifest.canonical_cloud_digest);
  hash_id(builder, manifest.lbvh_digest);
  hash_id(builder, manifest.semantic_digest);
  hash_size(builder, budget.maximum_work_unit_count);
  hash_size(builder, budget.maximum_frontier_entry_count);
  hash_size(builder, budget.maximum_auxiliary_frontier_entry_count);
  hash_size(builder, budget.maximum_emitted_record_count);
  hash_size(builder, budget.maximum_emitted_point_id_reference_count);
  hash_size(builder, budget.maximum_prune_receipt_count);
  hash_size(builder, budget.maximum_global_closed_ball_query_count);
  hash_size(builder, budget.maximum_point_classification_count);
  hash_size(builder, projection_limits.maximum_candidate_count);
  hash_size(builder, projection_limits.maximum_diagnostic_count);
  hash_size(builder, projection_limits.maximum_point_id_reference_count);
  hash_size(builder, limits.maximum_chunk_count);
  hash_size(builder, limits.maximum_exact_integer_byte_count);
  hash_size(
      builder, limits.maximum_total_exact_integer_byte_count_per_chunk);
  hash_size(builder, limits.maximum_payload_byte_count);
  return builder.finalize();
}

}  // namespace

struct ExactSparseHigherSupportH0ChunkRunContext::Impl {
  const ExactHigherSupportAuthorityContext* authority{};
  ExactHigherSupportStreamBudget chunk_budget{};
  ExactSparseDirectH0CandidateRunLimits projection_limits{};
  ExactSparseHigherSupportH0ChunkRunLimits limits{};
  contract::CanonicalId application_contract_digest{};
  contract::CanonicalId initial_checkpoint_digest{};
  contract::CanonicalId initial_output_chain_digest{};
  AtomicLinearRunStoreBinding store_binding;

  struct ReplayCache {
    std::optional<ExactHigherSupportTerminalSession> session;
    std::optional<ExactHigherSupportUnsealedDrain> pending_segment;
    std::size_t chunk_count{};
    bool reopen_required{false};
  };

  mutable ReplayCache producer_cache;
  mutable ReplayCache verifier_cache;
  mutable std::mutex producer_mutex;
  mutable std::mutex verifier_mutex;
  mutable std::atomic_size_t producer_root_reconstruction_count{};
  mutable std::atomic_size_t verifier_root_reconstruction_count{};
  mutable std::atomic_size_t producer_chunk_replay_count{};
  mutable std::atomic_size_t verifier_chunk_replay_count{};
  mutable std::atomic_size_t prepared_chunk_count{};
  mutable std::atomic_size_t durably_published_chunk_count{};
  mutable std::atomic_size_t publication_recertification_count{};
  mutable std::atomic_size_t recovery_recertification_count{};
  mutable std::atomic_size_t cleanup_recertification_count{};
  mutable std::atomic_size_t rejected_recertification_count{};
  mutable std::atomic_size_t maximum_retained_candidate_count{};
  mutable std::atomic_size_t maximum_retained_diagnostic_count{};
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
    cache.pending_segment.reset();
    cache.session.reset();
    cache.session.emplace(*authority, chunk_budget, limits.maximum_chunk_count);
    cache.chunk_count = 0U;
    cache.reopen_required = false;
    (verifier ? verifier_root_reconstruction_count
              : producer_root_reconstruction_count)
        .fetch_add(1U, std::memory_order_relaxed);
  }

  void observe_segment(
      const ExactHigherSupportTerminalSegment& segment) const noexcept {
    observe_maximum(
        maximum_retained_candidate_count, segment.events.size());
    observe_maximum(
        maximum_retained_diagnostic_count,
        segment.relevant_extra_shell_diagnostics.size());
    observe_maximum(
        maximum_retained_reference_count,
        segment.emitted_point_id_reference_count);
  }

  void replay_to(
      ReplayCache& cache,
      std::size_t target,
      bool verifier) const {
    if (target > limits.maximum_chunk_count) {
      throw std::length_error(
          "a Phase-14AB replay target exceeds its chunk cap");
    }
    if (!cache.session.has_value() || cache.chunk_count > target ||
        cache.pending_segment.has_value()) {
      reset_cache(cache, verifier);
    }
    while (cache.chunk_count < target) {
      if (cache.session->trusted_checkpoint().locally_complete()) {
        throw std::invalid_argument(
            "a Phase-14AB replay target lies beyond terminality");
      }
      ExactHigherSupportUnsealedDrain drained =
          cache.session->drain_next_unsealed_segment();
      if (drained.status() !=
              ExactHigherSupportUnsealedDrainStatus::segment_ready ||
          !drained.segment_available()) {
        throw std::invalid_argument(
            "a Phase-14AB replay target lies beyond its bounded producer");
      }
      observe_segment(*drained.segment());
      static_cast<void>(
          project_exact_sparse_direct_h0_higher_candidate_run(
              std::move(drained), projection_limits));
      ++cache.chunk_count;
      (verifier ? verifier_chunk_replay_count
                : producer_chunk_replay_count)
          .fetch_add(1U, std::memory_order_relaxed);
    }
    if (cache.session->chunk_count() != cache.chunk_count ||
        cache.session->released_segment_count() != cache.chunk_count ||
        cache.session->resident_segment_count() != 0U ||
        cache.session->unconsumed_segment_outstanding()) {
      throw std::logic_error(
          "a Phase-14AB replay cache lost its linear chunk cursor");
    }
  }

  [[nodiscard]] AtomicLinearRunChunkProposal prepare_locked(
      const AtomicLinearRunTrustedState& trusted_state) const {
    if (producer_cache.reopen_required) {
      throw std::logic_error(
          "an indeterminate Phase-14AB producer requires a fresh reopen");
    }
    if (trusted_state.next_sequence != trusted_state.next_chunk_index ||
        trusted_state.next_sequence != trusted_state.next_batch_index) {
      throw std::invalid_argument(
          "a Phase-14AB trusted cursor is not dense and canonical");
    }
    const std::size_t source = checked_size(trusted_state.next_chunk_index);
    if (source >= limits.maximum_chunk_count) {
      throw std::invalid_argument(
          "a Phase-14AB chunk-capped producer has no successor transition");
    }

    if (producer_cache.pending_segment.has_value()) {
      const ExactHigherSupportTerminalSegment* pending =
          producer_cache.pending_segment->segment();
      if (pending == nullptr || pending->chunk_sequence != source ||
          pending->source_checkpoint_digest !=
              trusted_state.checkpoint_digest) {
        reset_cache(producer_cache, false);
      }
    }
    if (!producer_cache.pending_segment.has_value()) {
      try {
        replay_to(producer_cache, source, false);
        if (producer_cache.session->trusted_checkpoint().locally_complete()) {
          throw std::invalid_argument(
              "a terminal Phase-14AB producer has no successor transition");
        }
        ExactHigherSupportUnsealedDrain drained =
            producer_cache.session->drain_next_unsealed_segment();
        if (drained.status() !=
                ExactHigherSupportUnsealedDrainStatus::segment_ready ||
            !drained.segment_available()) {
          throw std::invalid_argument(
              "a Phase-14AB producer did not return its next bounded segment");
        }
        producer_cache.pending_segment.emplace(std::move(drained));
        ++producer_cache.chunk_count;
        producer_chunk_replay_count.fetch_add(
            1U, std::memory_order_relaxed);
      } catch (...) {
        if (!producer_cache.pending_segment.has_value()) {
          producer_cache.session.reset();
          producer_cache.chunk_count = 0U;
        }
        throw;
      }
    }

    const ExactHigherSupportCheckpointManifest* manifest =
        producer_cache.pending_segment->manifest();
    const ExactHigherSupportTerminalSegment* segment =
        producer_cache.pending_segment->segment();
    if (manifest == nullptr || segment == nullptr ||
        *manifest != authority->manifest() ||
        segment->chunk_sequence != source ||
        segment->source_checkpoint_digest != trusted_state.checkpoint_digest ||
        producer_cache.chunk_count != source + 1U) {
      throw std::logic_error(
          "a Phase-14AB pending producer segment has invalid provenance");
    }
    observe_segment(*segment);

    AtomicLinearRunChunkProposal proposal;
    proposal.chunk_index = wire_size(source);
    proposal.batch_begin_index = wire_size(source);
    proposal.batch_end_index = wire_size(source + 1U);
    proposal.successor_checkpoint_digest =
        segment->next_checkpoint_digest;
    proposal.budget_snapshot_digest = budget_snapshot_digest(
        application_contract_digest, source, source + 1U);
    proposal.payload = encode_payload(
        application_contract_digest, *manifest, *segment, limits);
    prepared_chunk_count.fetch_add(1U, std::memory_order_relaxed);
    return proposal;
  }

  [[nodiscard]] AtomicLinearRunRecertification recertify_with_cache(
      const AtomicLinearRunTransition& transition,
      ReplayCache& cache,
      bool verifier) const {
    const std::size_t source = checked_size(transition.batch_begin_index);
    const std::size_t successor = checked_size(transition.batch_end_index);
    if (transition.sequence != transition.chunk_index ||
        transition.chunk_index != transition.batch_begin_index ||
        successor != source + 1U || source >= limits.maximum_chunk_count) {
      throw std::invalid_argument(
          "a Phase-14AB transition shape is not one dense chunk");
    }

    replay_to(cache, source, verifier);
    if (cache.session->trusted_checkpoint().locally_complete()) {
      throw std::invalid_argument(
          "a Phase-14AB transition follows a terminal chunk");
    }
    ExactHigherSupportUnsealedDrain drained =
        cache.session->drain_next_unsealed_segment();
    if (drained.status() !=
            ExactHigherSupportUnsealedDrainStatus::segment_ready ||
        !drained.segment_available() || drained.manifest() == nullptr) {
      throw std::invalid_argument(
          "a Phase-14AB transition has no freshly replayed segment");
    }
    ++cache.chunk_count;
    (verifier ? verifier_chunk_replay_count
              : producer_chunk_replay_count)
        .fetch_add(1U, std::memory_order_relaxed);
    const ExactHigherSupportTerminalSegment& segment = *drained.segment();
    observe_segment(segment);
    const std::vector<std::uint8_t> canonical = encode_payload(
        application_contract_digest, *drained.manifest(), segment, limits);
    const contract::CanonicalId expected_budget = budget_snapshot_digest(
        application_contract_digest, source, successor);
    if (segment.chunk_sequence != source ||
        segment.source_checkpoint_digest !=
            transition.source_checkpoint_digest ||
        segment.next_checkpoint_digest !=
            transition.successor_checkpoint_digest ||
        transition.budget_snapshot_digest != expected_budget ||
        transition.payload != canonical) {
      throw std::invalid_argument(
          "a Phase-14AB transition differs from fresh higher-support replay");
    }

    const bool session_complete =
        cache.session->trusted_checkpoint().locally_complete();
    if (session_complete !=
        (segment.status == ExactHigherSupportStreamStatus::complete)) {
      throw std::logic_error(
          "a Phase-14AB terminal segment disagrees with its successor checkpoint");
    }
    ExactSparseDirectH0CandidateRun run =
        project_exact_sparse_direct_h0_higher_candidate_run(
            std::move(drained), projection_limits);
    const ExactSparseDirectH0HigherSourceContract* source_contract =
        run.higher_source_contract
            ? &*run.higher_source_contract
            : nullptr;
    if (source_contract == nullptr ||
        source_contract->chunk_sequence != source ||
        source_contract->source_checkpoint_digest !=
            transition.source_checkpoint_digest ||
        source_contract->next_checkpoint_digest !=
            transition.successor_checkpoint_digest) {
      throw std::logic_error(
          "a Phase-14AB common H0 projection lost its fresh source contract");
    }

    auto projection = std::make_shared<
        ExactSparseHigherSupportH0RecertifiedChunkProjection>();
    projection->source_chunk_count = source;
    projection->successor_chunk_count = successor;
    projection->run = std::move(run);
    projection->cumulative_audit_after =
        cache.session->trusted_checkpoint().cumulative_audit;
    projection->session_complete = session_complete;
    projection->run_chunk_limit_reached =
        !session_complete && successor == limits.maximum_chunk_count;
    return AtomicLinearRunRecertification{
        true, true, true, std::move(projection), false};
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

    try {
      if (phase == AtomicLinearRunRecertificationPhase::
                       recovery_uncommitted_cleanup) {
        ReplayCache scratch;
        return recertify_with_cache(transition, scratch, true);
      }
      std::lock_guard lock{verifier_mutex};
      try {
        return recertify_with_cache(
            transition, verifier_cache, true);
      } catch (...) {
        verifier_cache.pending_segment.reset();
        verifier_cache.session.reset();
        verifier_cache.chunk_count = 0U;
        throw;
      }
    } catch (const std::bad_alloc&) {
      rejected_recertification_count.fetch_add(
          1U, std::memory_order_relaxed);
      return AtomicLinearRunRecertification{
          false, false, false, {}, true};
    } catch (...) {
      rejected_recertification_count.fetch_add(
          1U, std::memory_order_relaxed);
      return {};
    }
  }
};

static_assert(
    std::is_nothrow_move_assignable_v<ExactSparseDirectH0CandidateRun>,
    "Phase-14AB accepted projections require a nothrow common-run move");

ExactSparseHigherSupportH0ChunkRunContext::
    ExactSparseHigherSupportH0ChunkRunContext(
        const ExactHigherSupportAuthorityContext& authority,
        ExactHigherSupportStreamBudget chunk_budget,
        ExactSparseDirectH0CandidateRunLimits projection_limits,
        ExactSparseHigherSupportH0ChunkRunLimits limits)
    : impl_(std::make_shared<Impl>()) {
  const ExactHigherSupportCheckpointManifest& manifest =
      authority.manifest();
  if (!valid_limits(manifest, chunk_budget, projection_limits, limits)) {
    throw std::invalid_argument(
        "a Phase-14AB context contract is invalid or unbounded");
  }
  const ExactHigherSupportCheckpoint initial =
      make_initial_exact_higher_support_checkpoint(authority);
  if (!verify_exact_higher_support_checkpoint(authority, initial)
           .local_integrity_verified) {
    throw std::logic_error(
        "the Phase-14AB canonical initial checkpoint failed verification");
  }
  impl_->authority = &authority;
  impl_->chunk_budget = chunk_budget;
  impl_->projection_limits = projection_limits;
  impl_->limits = limits;
  impl_->application_contract_digest = application_digest(
      manifest, chunk_budget, projection_limits, limits);
  impl_->initial_checkpoint_digest = initial.checkpoint_digest;
  impl_->initial_output_chain_digest = initial.output_chain_digest;
}

ExactSparseHigherSupportH0ChunkRunContext::
    ~ExactSparseHigherSupportH0ChunkRunContext() = default;

const contract::CanonicalId&
ExactSparseHigherSupportH0ChunkRunContext::application_contract_digest()
    const noexcept {
  return impl_->application_contract_digest;
}

const contract::CanonicalId&
ExactSparseHigherSupportH0ChunkRunContext::initial_checkpoint_digest()
    const noexcept {
  return impl_->initial_checkpoint_digest;
}

AtomicLinearRunContract
ExactSparseHigherSupportH0ChunkRunContext::run_contract() const {
  AtomicLinearRunContract result;
  result.application_contract_digest =
      impl_->application_contract_digest;
  result.initial_checkpoint_digest = impl_->initial_checkpoint_digest;
  result.initial_output_chain_digest =
      impl_->initial_output_chain_digest;
  result.initial_chunk_index = 0U;
  result.initial_batch_index = 0U;
  return result;
}

AtomicLinearRunStore
ExactSparseHigherSupportH0ChunkRunContext::create_new_store(
    const std::filesystem::path& dedicated_directory,
    AtomicLinearRunStoreLimits store_limits) const {
  if (store_limits.maximum_batch_span != 1U) {
    throw std::invalid_argument(
        "a Phase-14AB store must use a unit chunk span");
  }
  const std::shared_ptr<Impl> shared = impl_;
  return AtomicLinearRunStore::create_new_bound(
      dedicated_directory, run_contract(), std::move(store_limits),
      [shared](const AtomicLinearRunTransition& transition,
               AtomicLinearRunRecertificationPhase phase) {
        return shared->recertify(transition, phase);
      },
      [](const AtomicLinearRunTransition&,
         AtomicLinearRunRecertificationPhase,
         AtomicLinearRunResourceGateBoundary) { return true; },
      impl_->store_binding);
}

AtomicLinearRunStore
ExactSparseHigherSupportH0ChunkRunContext::open_existing_store(
    const std::filesystem::path& dedicated_directory,
    AtomicLinearRunStoreLimits store_limits,
    std::optional<AtomicLinearRunExternalAnchor> expected_anchor) const {
  return open_existing_store(
      dedicated_directory, std::move(store_limits),
      std::move(expected_anchor), {});
}

AtomicLinearRunStore
ExactSparseHigherSupportH0ChunkRunContext::open_existing_store(
    const std::filesystem::path& dedicated_directory,
    AtomicLinearRunStoreLimits store_limits,
    std::optional<AtomicLinearRunExternalAnchor> expected_anchor,
    ExactSparseHigherSupportH0RecertifiedChunkVisitor
        committed_prefix_visitor) const {
  if (store_limits.maximum_batch_span != 1U) {
    throw std::invalid_argument(
        "a Phase-14AB store must use a unit chunk span");
  }
  AtomicLinearRunCommittedPrefixVisitor visitor;
  if (committed_prefix_visitor) {
    visitor = [typed = std::move(committed_prefix_visitor)](
                  const AtomicLinearRunTransition&,
                  const AtomicLinearRunAcceptedProjection* projection) {
      const auto* accepted = dynamic_cast<const
          ExactSparseHigherSupportH0RecertifiedChunkProjection*>(
              projection);
      if (accepted == nullptr) {
        throw std::logic_error(
            "a Phase-14AB recovery projection has the wrong type");
      }
      typed(*accepted);
    };
  }
  const std::shared_ptr<Impl> shared = impl_;
  return AtomicLinearRunStore::open_existing_bound(
      dedicated_directory, run_contract(), std::move(store_limits),
      [shared](const AtomicLinearRunTransition& transition,
               AtomicLinearRunRecertificationPhase phase) {
        return shared->recertify(transition, phase);
      },
      [](const AtomicLinearRunTransition&,
         AtomicLinearRunRecertificationPhase,
         AtomicLinearRunResourceGateBoundary) { return true; },
      std::move(expected_anchor), std::move(visitor),
      impl_->store_binding);
}

AtomicLinearRunPublishResult
ExactSparseHigherSupportH0ChunkRunContext::publish_next_chunk(
    AtomicLinearRunStore& store,
    AtomicLinearRunPublishOptions options) const {
  std::lock_guard lock{impl_->producer_mutex};
  if (!store.bound_to(impl_->store_binding)) {
    throw std::invalid_argument(
        "a Phase-14AB producer requires a store created or reopened by "
        "the same context");
  }
  if (impl_->producer_cache.reopen_required) {
    const AtomicLinearRunStoreStatus& status = store.status();
    if (!status.opened_existing_run ||
        !status.authoritative_head_certified ||
        !status.linear_prefix_replayed ||
        status.failed_closed_reopen_required) {
      throw std::logic_error(
          "an indeterminate Phase-14AB producer requires a certified "
          "existing-store reopen under the same context");
    }
    impl_->reset_cache(impl_->producer_cache, false);
  }
  AtomicLinearRunPublishResult result = store.publish_next(
      impl_->prepare_locked(store.trusted_state()), options);
  if (result.decision ==
      AtomicLinearRunPublishDecision::durably_published) {
    if (!impl_->producer_cache.pending_segment.has_value() ||
        !impl_->producer_cache.pending_segment->segment_available()) {
      std::terminate();
    }
    impl_->producer_cache.pending_segment->consume();
    impl_->producer_cache.pending_segment.reset();
    impl_->durably_published_chunk_count.fetch_add(
        1U, std::memory_order_relaxed);
  } else if (
      result.decision == AtomicLinearRunPublishDecision::
                             indeterminate_io_failure_reopen_required) {
    impl_->producer_cache.reopen_required = true;
  }
  return result;
}

AtomicLinearRunRecertification
ExactSparseHigherSupportH0ChunkRunContext::recertify_for_validation(
    const AtomicLinearRunTransition& transition,
    AtomicLinearRunRecertificationPhase phase) const {
  return impl_->recertify(transition, phase);
}

ExactSparseHigherSupportH0ChunkRunAudit
ExactSparseHigherSupportH0ChunkRunContext::audit() const noexcept {
  ExactSparseHigherSupportH0ChunkRunAudit result;
  result.producer_root_reconstruction_count =
      impl_->producer_root_reconstruction_count.load(
          std::memory_order_relaxed);
  result.verifier_root_reconstruction_count =
      impl_->verifier_root_reconstruction_count.load(
          std::memory_order_relaxed);
  result.producer_chunk_replay_count =
      impl_->producer_chunk_replay_count.load(std::memory_order_relaxed);
  result.verifier_chunk_replay_count =
      impl_->verifier_chunk_replay_count.load(std::memory_order_relaxed);
  result.prepared_chunk_count =
      impl_->prepared_chunk_count.load(std::memory_order_relaxed);
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
  result.maximum_retained_candidate_count =
      impl_->maximum_retained_candidate_count.load(
          std::memory_order_relaxed);
  result.maximum_retained_diagnostic_count =
      impl_->maximum_retained_diagnostic_count.load(
          std::memory_order_relaxed);
  result.maximum_retained_point_id_reference_count =
      impl_->maximum_retained_reference_count.load(
          std::memory_order_relaxed);
  result.maximum_pending_producer_segment_count = 1U;
  result.maximum_persistent_session_cache_count = 2U;
  return result;
}

}  // namespace morsehgp3d::hierarchy
