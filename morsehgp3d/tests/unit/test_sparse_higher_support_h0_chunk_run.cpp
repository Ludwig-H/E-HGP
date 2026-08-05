#include "morsehgp3d/hierarchy/sparse_higher_support_h0_chunk_run.hpp"

#include "morsehgp3d/exact/point.hpp"

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::contract::CanonicalId;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::exact::parse_canonical_integer;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;

void require(bool condition, std::string_view message) {
  if (!condition) {
    throw std::runtime_error(std::string{message});
  }
}

class TemporaryWorkspace {
 public:
  TemporaryWorkspace() {
    parent_ = std::filesystem::temp_directory_path();
    std::string pattern =
        (parent_ / "morsehgp3d-higher-h0-chunk-XXXXXX").string();
    std::vector<char> writable(pattern.begin(), pattern.end());
    writable.push_back('\0');
    char* const created = ::mkdtemp(writable.data());
    if (created == nullptr) {
      throw std::system_error(
          errno, std::generic_category(), "mkdtemp failed");
    }
    root_ = std::filesystem::path{created};
  }

  ~TemporaryWorkspace() {
    if (!root_.empty() && root_.parent_path() == parent_ &&
        root_.filename().string().starts_with(
            "morsehgp3d-higher-h0-chunk-")) {
      std::error_code ignored;
      static_cast<void>(std::filesystem::remove_all(root_, ignored));
    }
  }

  TemporaryWorkspace(const TemporaryWorkspace&) = delete;
  TemporaryWorkspace& operator=(const TemporaryWorkspace&) = delete;

  [[nodiscard]] std::filesystem::path make_run_directory(
      std::string_view leaf = "run") const {
    const std::filesystem::path directory =
        root_ / std::string{leaf};
    if (!std::filesystem::create_directory(directory)) {
      throw std::runtime_error("cannot create the Phase-14AB run directory");
    }
    return directory;
  }

 private:
  std::filesystem::path parent_;
  std::filesystem::path root_;
};

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] CanonicalPointCloud regular_tetrahedron() {
  const std::array<CertifiedPoint3, 4U> points{
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0)};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud e5_cloud() {
  const std::array<CertifiedPoint3, 5U> points{
      point(-2.0, -1.0, 0.0),
      point(-2.0, 1.0, 0.0),
      point(0.0, 0.0, 0.0),
      point(3.0, 2.0, 0.0),
      point(4.0, -1.0, 0.0)};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactHigherSupportStreamBudget chunk_budget() {
  return {
      1U,
      64U,
      64U,
      1U,
      32U,
      16U,
      16U,
      64U};
}

[[nodiscard]] ExactSparseDirectH0CandidateRunLimits projection_limits() {
  return {8U, 8U, 128U};
}

[[nodiscard]] ExactSparseHigherSupportH0ChunkRunLimits chunk_run_limits() {
  return {256U, 256U, 16U * 1024U, 64U * 1024U};
}

[[nodiscard]] AtomicLinearRunStoreLimits store_limits() {
  constexpr std::size_t transition_count = 256U;
  constexpr std::size_t payload_bytes = 64U * 1024U;
  constexpr std::size_t encoded_bytes =
      atomic_linear_run_transition_fixed_wire_byte_count + payload_bytes;
  return {
      transition_count,
      payload_bytes,
      encoded_bytes,
      transition_count * encoded_bytes,
      1U};
}

[[nodiscard]] ExactRational decimal_rational_with_integer_bytes(
    std::size_t byte_count,
    bool negative) {
  require(
      byte_count >= (negative ? 2U : 1U),
      "the exact decimal boundary fixture is too short");
  std::string numerator;
  numerator.reserve(byte_count);
  if (negative) {
    numerator.push_back('-');
  }
  numerator.push_back('1');
  numerator.append(byte_count - numerator.size(), '0');
  return ExactRational{
      parse_canonical_integer(numerator), BigInt{3}};
}

[[nodiscard]] ExactSparseHigherSupportH0ChunkRunLimits
rational_boundary_limits(std::size_t exact_integer_byte_count) {
  return {
      256U,
      exact_integer_byte_count,
      exact_integer_byte_count + 1U,
      exact_integer_byte_count + 18U};
}

struct ExpectedSequence {
  std::vector<ExactSparseDirectH0CandidateRun> runs;
  std::vector<ExactHigherSupportStreamAudit> cumulative_audits;
  std::vector<ExactHigherSupportStreamStatus> statuses;
  std::size_t empty_segment_count{};
  std::size_t event_segment_count{};
};

[[nodiscard]] ExpectedSequence build_expected_sequence(
    const ExactHigherSupportAuthorityContext& authority) {
  ExpectedSequence result;
  ExactHigherSupportTerminalSession session{
      authority,
      chunk_budget(),
      chunk_run_limits().maximum_chunk_count};
  while (!session.trusted_checkpoint().locally_complete()) {
    auto drained = session.drain_next_unsealed_segment();
    require(
        drained.status() ==
                ExactHigherSupportUnsealedDrainStatus::segment_ready &&
            drained.segment_available(),
        "the Phase-14AA reference drain did not return one segment");
    const ExactHigherSupportTerminalSegment& segment = *drained.segment();
    result.empty_segment_count +=
        segment.emitted_record_count() == 0U ? 1U : 0U;
    result.event_segment_count += segment.events.empty() ? 0U : 1U;
    result.statuses.push_back(segment.status);
    result.runs.push_back(
        project_exact_sparse_direct_h0_higher_candidate_run(
            std::move(drained), projection_limits()));
    result.cumulative_audits.push_back(
        session.trusted_checkpoint().cumulative_audit);
  }
  require(
      session.chunk_count() == result.runs.size() &&
          session.released_segment_count() == result.runs.size() &&
          session.resident_segment_count() == 0U &&
          !session.unconsumed_segment_outstanding(),
      "the Phase-14AA reference retained a drained segment");
  return result;
}

[[nodiscard]] const ExactSparseHigherSupportH0RecertifiedChunkProjection&
accepted_projection(const AtomicLinearRunPublishResult& publication) {
  const auto* projection = dynamic_cast<const
      ExactSparseHigherSupportH0RecertifiedChunkProjection*>(
          publication.recertification.accepted_projection.get());
  if (projection == nullptr) {
    throw std::runtime_error(
        "a durable Phase-14AB publication returned no typed projection");
  }
  return *projection;
}

void append_projection(
    const ExactSparseHigherSupportH0RecertifiedChunkProjection& projection,
    std::vector<ExactSparseDirectH0CandidateRun>& runs,
    std::vector<ExactHigherSupportStreamAudit>& audits) {
  require(
      projection.source_chunk_count == runs.size() &&
          projection.successor_chunk_count == runs.size() + 1U,
      "a Phase-14AB projection lost its dense chunk cursor");
  require(
      projection.run.schema_version ==
              sparse_direct_h0_candidate_merge_schema_version &&
          projection.run.contains_higher_candidates &&
          !projection.run.contains_pair_candidates &&
          projection.run
              .candidates_strictly_sorted_by_terminal_facade_key &&
          projection.run.diagnostics_preserved &&
          !projection.run.global_event_indices_assigned &&
          !projection.run.hierarchy_reduction_performed &&
          !projection.run.complete_h0_authority_claimed &&
          projection.run.higher_source_contract.has_value(),
      "a Phase-14AB projection overstated or lost its common-H0 scope");
  runs.push_back(projection.run);
  audits.push_back(projection.cumulative_audit_after);
}

struct TransitionCapture {
  std::optional<AtomicLinearRunTransition> transition;
  bool wrong_projection_type{false};
};

AtomicLinearRunPreHeadCommitOutcome capture_transition(
    const AtomicLinearRunTransition& transition,
    const AtomicLinearRunAcceptedProjection* projection,
    void* opaque) noexcept {
  auto& capture = *static_cast<TransitionCapture*>(opaque);
  if (dynamic_cast<const
          ExactSparseHigherSupportH0RecertifiedChunkProjection*>(
          projection) == nullptr) {
    capture.wrong_projection_type = true;
    return AtomicLinearRunPreHeadCommitOutcome::rejected_atomically;
  }
  try {
    capture.transition = transition;
  } catch (...) {
    return AtomicLinearRunPreHeadCommitOutcome::indeterminate;
  }
  return AtomicLinearRunPreHeadCommitOutcome::committed;
}

struct UnlinkSynchronizedHeadTemporary {
  std::filesystem::path path;
  bool unlink_succeeded{false};
};

void unlink_synchronized_head_temporary(
    AtomicLinearRunPublishStage stage,
    void* opaque) noexcept {
  if (stage != AtomicLinearRunPublishStage::
                   head_temporary_file_synchronized_and_reread) {
    return;
  }
  auto& state =
      *static_cast<UnlinkSynchronizedHeadTemporary*>(opaque);
  state.unlink_succeeded = ::unlink(state.path.c_str()) == 0;
}

struct CorruptWrittenHeadTemporary {
  std::filesystem::path path;
  bool corruption_succeeded{false};
};

void corrupt_written_head_temporary(
    AtomicLinearRunPublishStage stage,
    void* opaque) noexcept {
  if (stage !=
      AtomicLinearRunPublishStage::head_temporary_file_written) {
    return;
  }
  auto& state = *static_cast<CorruptWrittenHeadTemporary*>(opaque);
  const int descriptor = ::open(
      state.path.c_str(), O_WRONLY | O_CLOEXEC | O_NOFOLLOW);
  if (descriptor < 0) {
    return;
  }
  constexpr std::uint8_t corrupt_magic = UINT8_C(0xff);
  const bool write_succeeded =
      ::pwrite(descriptor, &corrupt_magic, 1U, 0) == 1;
  const bool close_succeeded = ::close(descriptor) == 0;
  state.corruption_succeeded = write_succeeded && close_succeeded;
}

[[nodiscard]] std::filesystem::path first_transition_final(
    const std::filesystem::path& directory) {
  return directory / "linear-run-00000000000000000000.run";
}

[[nodiscard]] bool first_transition_publication_clean(
    const std::filesystem::path& directory) {
  return !std::filesystem::exists(first_transition_final(directory)) &&
      !std::filesystem::exists(
          directory / ".linear-run-00000000000000000000.tmp") &&
      !std::filesystem::exists(directory / ".HEAD.tmp");
}

void require_bounded_memory_audit(
    const ExactSparseHigherSupportH0ChunkRunAudit& audit) {
  require(
      audit.maximum_retained_candidate_count <=
              projection_limits().maximum_candidate_count &&
          audit.maximum_retained_diagnostic_count <=
              projection_limits().maximum_diagnostic_count &&
          audit.maximum_retained_point_id_reference_count <=
              projection_limits().maximum_point_id_reference_count &&
          audit.maximum_pending_producer_segment_count == 1U &&
          audit.retained_transition_history_count == 0U &&
          audit.context_owned_global_support_count == 0U &&
          audit.context_owned_global_facet_or_coface_count == 0U &&
          audit.context_owned_cell_gamma_or_delaunay_count == 0U &&
          audit.maximum_persistent_session_cache_count == 2U,
      "the Phase-14AB context exceeded its bounded-memory contract");
}

void require_store_scope(const AtomicLinearRunStoreStatus& status) {
  require(
      status.writer_lock_acquired &&
          status.authoritative_head_certified &&
          status.linear_prefix_replayed &&
          !status.failed_closed_reopen_required &&
          !status.process_local_ticket_serialized &&
          status.retained_transition_history_count == 0U &&
          status.global_gamma_cell_count == 0U &&
          status.higher_order_delaunay_cell_count == 0U,
      "the atomic run store materialized forbidden Phase-14AB state");
}

void test_general_rational_wire_budgets_are_syntax_only() {
  require(
      sparse_higher_support_triangle_exact_integer_byte_count == 6973U &&
          sparse_higher_support_tetrahedron_exact_integer_byte_count ==
              9503U,
      "the higher-support conservative decimal budgets drifted");

  constexpr std::array<std::size_t, 2U> byte_counts{
      sparse_higher_support_triangle_exact_integer_byte_count,
      sparse_higher_support_tetrahedron_exact_integer_byte_count};
  constexpr std::array<std::uint8_t, 2U> support_sizes{3U, 4U};
  const CanonicalPointCloud cloud = regular_tetrahedron();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportAuthorityContext authority{index, cloud, 10U};

  for (std::size_t case_index = 0U;
       case_index < byte_counts.size(); ++case_index) {
    const std::size_t byte_count = byte_counts[case_index];
    const std::uint8_t support_size = support_sizes[case_index];
    const ExactRational value = decimal_rational_with_integer_bytes(
        byte_count, support_size == 4U);
    ExactSparseHigherSupportH0ChunkRunContext exact_context{
        authority,
        chunk_budget(),
        projection_limits(),
        rational_boundary_limits(byte_count)};
    const ExactSparseHigherSupportH0RationalWireRoundTrip round_trip =
        exact_context.round_trip_exact_rational_wire_for_validation(
            support_size, value);
    require(
        round_trip.support_size == support_size &&
            round_trip.value == value &&
            round_trip.numerator_byte_count == byte_count &&
            round_trip.denominator_byte_count == 1U &&
            round_trip.canonical_rational_field_byte_count ==
                byte_count + 2U &&
            round_trip.encoded_wire_byte_count == byte_count + 18U &&
            value.denominator() == 3,
        "an exact higher-support decimal boundary did not round-trip canonically");

    ExactSparseHigherSupportH0ChunkRunLimits below =
        rational_boundary_limits(byte_count);
    below.maximum_exact_integer_byte_count = byte_count - 1U;
    ExactSparseHigherSupportH0ChunkRunContext below_context{
        authority, chunk_budget(), projection_limits(), below};
    const ExactSparseHigherSupportH0ChunkRunAudit audit_before =
        below_context.audit();
    bool rejected = false;
    try {
      static_cast<void>(
          below_context.round_trip_exact_rational_wire_for_validation(
              support_size, value));
    } catch (const std::length_error&) {
      rejected = true;
    }
    require(
        rejected && below_context.audit() == audit_before,
        "a cap-minus-one syntax-only rational validation was not rejected");
  }
}

void test_non_dyadic_1105_over_242_durable_round_trip() {
  const CanonicalPointCloud cloud = e5_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportAuthorityContext authority{index, cloud, 10U};
  const ExactLevel target{BigInt{1105}, BigInt{242}};
  TemporaryWorkspace workspace;
  const std::filesystem::path run_directory =
      workspace.make_run_directory();
  AtomicLinearRunExternalAnchor terminal_anchor;
  std::size_t published_target_count = 0U;

  {
    ExactSparseHigherSupportH0ChunkRunContext context{
        authority,
        chunk_budget(),
        projection_limits(),
        chunk_run_limits()};
    auto store = context.create_new_store(run_directory, store_limits());
    bool complete = false;
    while (!complete) {
      const AtomicLinearRunPublishResult publication =
          context.publish_next_chunk(store);
      require(
          publication.decision ==
                  AtomicLinearRunPublishDecision::durably_published &&
              publication.trusted_state_advanced,
          "the E5 non-dyadic fixture did not publish durably");
      const auto& projection = accepted_projection(publication);
      for (const ExactSparseDirectH0Candidate& candidate :
           projection.run.candidates) {
        if (candidate.support_size == 3U &&
            candidate.squared_level == target) {
          ++published_target_count;
        }
      }
      complete = projection.session_complete;
      terminal_anchor = publication.current_anchor;
    }
    require_store_scope(store.status());
  }

  std::size_t recovered_target_count = 0U;
  {
    ExactSparseHigherSupportH0ChunkRunContext context{
        authority,
        chunk_budget(),
        projection_limits(),
        chunk_run_limits()};
    auto store = context.open_existing_store(
        run_directory,
        store_limits(),
        terminal_anchor,
        [&](const ExactSparseHigherSupportH0RecertifiedChunkProjection&
                projection) {
          for (const ExactSparseDirectH0Candidate& candidate :
               projection.run.candidates) {
            if (candidate.support_size == 3U &&
                candidate.squared_level == target) {
              require(
                  candidate.squared_level.numerator_string() == "1105" &&
                      candidate.squared_level.denominator_string() == "242",
                  "the durable E5 level was changed or made dyadic");
              ++recovered_target_count;
            }
          }
        });
    require_store_scope(store.status());
  }
  require(
      published_target_count == 1U && recovered_target_count == 1U,
      "the permanent 1105/242 higher-support event did not survive HEAD reopen");
}

void test_context_binding_payload_floor_and_chunk_cap() {
  constexpr std::size_t requested_maximum_order = 10U;
  const CanonicalPointCloud cloud = regular_tetrahedron();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportAuthorityContext authority{
      index, cloud, requested_maximum_order};

  ExactSparseHigherSupportH0ChunkRunLimits below_floor =
      chunk_run_limits();
  below_floor.maximum_payload_byte_count =
      sparse_higher_support_h0_chunk_run_fixed_payload_byte_count - 1U;
  bool below_floor_rejected = false;
  try {
    ExactSparseHigherSupportH0ChunkRunContext invalid{
        authority, chunk_budget(), projection_limits(), below_floor};
    static_cast<void>(invalid.audit());
  } catch (const std::invalid_argument&) {
    below_floor_rejected = true;
  }
  ExactSparseHigherSupportH0ChunkRunLimits exact_floor =
      chunk_run_limits();
  exact_floor.maximum_payload_byte_count =
      sparse_higher_support_h0_chunk_run_fixed_payload_byte_count;
  ExactSparseHigherSupportH0ChunkRunContext floor_context{
      authority, chunk_budget(), projection_limits(), exact_floor};
  require(
      sparse_higher_support_h0_chunk_run_fixed_payload_byte_count == 278U &&
          below_floor_rejected &&
          floor_context.audit().prepared_chunk_count == 0U,
      "the Phase-14AB context accepted less than its fixed payload header");

  ExactSparseHigherSupportH0ChunkRunContext owner{
      authority,
      chunk_budget(),
      projection_limits(),
      chunk_run_limits()};
  ExactSparseHigherSupportH0ChunkRunContext foreign{
      authority,
      chunk_budget(),
      projection_limits(),
      chunk_run_limits()};
  TemporaryWorkspace binding_workspace;
  auto owner_store = owner.create_new_store(
      binding_workspace.make_run_directory("owner"), store_limits());
  auto foreign_store = foreign.create_new_store(
      binding_workspace.make_run_directory("foreign"), store_limits());
  bool foreign_store_rejected = false;
  try {
    static_cast<void>(owner.publish_next_chunk(foreign_store));
  } catch (const std::invalid_argument&) {
    foreign_store_rejected = true;
  }
  require(
      foreign_store_rejected &&
          owner.audit().producer_root_reconstruction_count == 0U &&
          owner.audit().prepared_chunk_count == 0U &&
          foreign.audit().producer_root_reconstruction_count == 0U &&
          foreign.audit().prepared_chunk_count == 0U,
      "a store from another Phase-14AB context crossed its process-local binding");
  require(
      owner.publish_next_chunk(owner_store).decision ==
          AtomicLinearRunPublishDecision::durably_published,
      "the process-local store binding rejected its owning context");

  ExactSparseHigherSupportH0ChunkRunLimits one_chunk_limits =
      chunk_run_limits();
  one_chunk_limits.maximum_chunk_count = 1U;
  ExactSparseHigherSupportH0ChunkRunContext capped{
      authority,
      chunk_budget(),
      projection_limits(),
      one_chunk_limits};
  TemporaryWorkspace cap_workspace;
  auto capped_store = capped.create_new_store(
      cap_workspace.make_run_directory(), store_limits());
  const AtomicLinearRunPublishResult first =
      capped.publish_next_chunk(capped_store);
  bool second_rejected = false;
  try {
    static_cast<void>(capped.publish_next_chunk(capped_store));
  } catch (const std::invalid_argument&) {
    second_rejected = true;
  }
  require(
      first.decision ==
              AtomicLinearRunPublishDecision::durably_published &&
          accepted_projection(first).run_chunk_limit_reached &&
          !accepted_projection(first).session_complete &&
          second_rejected &&
          capped_store.status().committed_transition_count == 1U &&
          capped.audit().prepared_chunk_count == 1U,
      "the one-chunk Phase-14AB cap advanced beyond its durable prefix");
}

void test_retryable_pre_head_failure_reuses_pending_segment() {
  constexpr std::size_t requested_maximum_order = 10U;
  const CanonicalPointCloud cloud = regular_tetrahedron();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportAuthorityContext authority{
      index, cloud, requested_maximum_order};
  ExactSparseHigherSupportH0ChunkRunContext context{
      authority,
      chunk_budget(),
      projection_limits(),
      chunk_run_limits()};
  TemporaryWorkspace workspace;
  const std::filesystem::path run_directory =
      workspace.make_run_directory();
  auto store = context.create_new_store(run_directory, store_limits());

  UnlinkSynchronizedHeadTemporary sabotage{
      run_directory / ".HEAD.tmp", false};
  AtomicLinearRunPublishOptions options;
  options.observer = unlink_synchronized_head_temporary;
  options.observer_state = &sabotage;
  const AtomicLinearRunPublishResult failed =
      context.publish_next_chunk(store, options);
  const ExactSparseHigherSupportH0ChunkRunAudit after_failure =
      context.audit();
  require(
      sabotage.unlink_succeeded &&
          failed.decision ==
              AtomicLinearRunPublishDecision::retryable_io_failure &&
          failed.system_error_number == ENOENT &&
          failed.recertification.accepted() &&
          !failed.trusted_state_advanced &&
          store.trusted_state().next_sequence == 0U &&
          store.status().committed_transition_count == 0U &&
          !store.status().failed_closed_reopen_required &&
          store.status().authoritative_head_certified &&
          first_transition_publication_clean(run_directory) &&
          after_failure.producer_root_reconstruction_count == 1U &&
          after_failure.producer_chunk_replay_count == 1U &&
          after_failure.prepared_chunk_count == 1U &&
          after_failure.durably_published_chunk_count == 0U &&
          after_failure.publication_recertification_count == 1U &&
          after_failure.verifier_chunk_replay_count == 1U,
      "a retryable pre-HEAD failure lost its clean pending segment");

  const AtomicLinearRunPublishResult retried =
      context.publish_next_chunk(store);
  const ExactSparseHigherSupportH0ChunkRunAudit after_retry =
      context.audit();
  require(
      retried.decision ==
              AtomicLinearRunPublishDecision::durably_published &&
          retried.trusted_state_advanced &&
          accepted_projection(failed) == accepted_projection(retried) &&
          store.trusted_state().next_sequence == 1U &&
          store.status().committed_transition_count == 1U &&
          after_retry.producer_root_reconstruction_count ==
              after_failure.producer_root_reconstruction_count &&
          after_retry.producer_chunk_replay_count ==
              after_failure.producer_chunk_replay_count &&
          after_retry.prepared_chunk_count == 2U &&
          after_retry.durably_published_chunk_count == 1U &&
          after_retry.publication_recertification_count == 2U &&
          after_retry.verifier_chunk_replay_count == 2U,
      "retrying a clean pre-HEAD failure reran the producer or changed its chunk");
  require_store_scope(store.status());
  require_bounded_memory_audit(after_retry);
}

void test_indeterminate_failure_requires_anchored_reopen() {
  constexpr std::size_t requested_maximum_order = 10U;
  const CanonicalPointCloud cloud = regular_tetrahedron();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportAuthorityContext authority{
      index, cloud, requested_maximum_order};
  ExactSparseHigherSupportH0ChunkRunContext context{
      authority,
      chunk_budget(),
      projection_limits(),
      chunk_run_limits()};
  TemporaryWorkspace workspace;
  const std::filesystem::path run_directory =
      workspace.make_run_directory();
  AtomicLinearRunExternalAnchor initial_anchor;
  ExactSparseHigherSupportH0ChunkRunAudit after_failure;
  std::shared_ptr<const AtomicLinearRunAcceptedProjection>
      indeterminate_projection;

  {
    auto store = context.create_new_store(run_directory, store_limits());
    initial_anchor = store.status().current_anchor;
    CorruptWrittenHeadTemporary sabotage{
        run_directory / ".HEAD.tmp", false};
    AtomicLinearRunPublishOptions options;
    options.observer = corrupt_written_head_temporary;
    options.observer_state = &sabotage;
    const AtomicLinearRunPublishResult failed =
        context.publish_next_chunk(store, options);
    indeterminate_projection =
        failed.recertification.accepted_projection;
    after_failure = context.audit();
    require(
        sabotage.corruption_succeeded &&
            failed.decision == AtomicLinearRunPublishDecision::
                                   indeterminate_io_failure_reopen_required &&
            failed.recertification.accepted() &&
            indeterminate_projection &&
            !failed.trusted_state_advanced &&
            store.trusted_state().next_sequence == 0U &&
            store.status().failed_closed_reopen_required &&
            !store.status().authoritative_head_certified &&
            std::filesystem::exists(
                first_transition_final(run_directory)) &&
            std::filesystem::exists(run_directory / ".HEAD.tmp") &&
            after_failure.producer_root_reconstruction_count == 1U &&
            after_failure.producer_chunk_replay_count == 1U &&
            after_failure.prepared_chunk_count == 1U &&
            after_failure.durably_published_chunk_count == 0U,
        "hostile HEAD staging did not leave a fail-closed recertifiable orphan");

    bool live_lock_rejected = false;
    try {
      auto locked = context.open_existing_store(
          run_directory, store_limits(), initial_anchor);
      static_cast<void>(locked.status());
    } catch (const std::system_error& error) {
      live_lock_rejected =
          error.code().value() == EAGAIN ||
          error.code().value() == EWOULDBLOCK;
    }
    require(
        live_lock_rejected &&
            context.audit().cleanup_recertification_count == 0U,
        "a live failed writer did not retain the exclusive run lock");
  }

  auto reopened = context.open_existing_store(
      run_directory, store_limits(), initial_anchor);
  const ExactSparseHigherSupportH0ChunkRunAudit after_reopen =
      context.audit();
  require(
      reopened.status().opened_existing_run &&
          reopened.status().external_anchor_supplied &&
          reopened.status().external_anchor_verified &&
          reopened.status().recovered_transition_count == 0U &&
          reopened.status().committed_transition_count == 0U &&
          reopened.status().uncommitted_cleanup_recertification_count ==
              1U &&
          reopened.status().removed_uncommitted_temporary_file_count ==
              1U &&
          reopened.status().removed_uncommitted_final_file_count == 1U &&
          first_transition_publication_clean(run_directory) &&
          after_reopen.cleanup_recertification_count == 1U &&
          after_reopen.producer_root_reconstruction_count ==
              after_failure.producer_root_reconstruction_count &&
          after_reopen.producer_chunk_replay_count ==
              after_failure.producer_chunk_replay_count,
      "the anchored reopen did not recertify and remove the uncertain suffix");
  require_store_scope(reopened.status());

  const AtomicLinearRunPublishResult resumed =
      context.publish_next_chunk(reopened);
  const ExactSparseHigherSupportH0ChunkRunAudit after_resume =
      context.audit();
  const auto* uncertain = dynamic_cast<const
      ExactSparseHigherSupportH0RecertifiedChunkProjection*>(
          indeterminate_projection.get());
  require(
      resumed.decision ==
              AtomicLinearRunPublishDecision::durably_published &&
          resumed.trusted_state_advanced && uncertain != nullptr &&
          *uncertain == accepted_projection(resumed) &&
          reopened.trusted_state().next_sequence == 1U &&
          after_resume.producer_root_reconstruction_count == 2U &&
          after_resume.producer_chunk_replay_count == 2U &&
          after_resume.prepared_chunk_count == 2U &&
          after_resume.durably_published_chunk_count == 1U &&
          after_resume.publication_recertification_count == 2U &&
          after_resume.cleanup_recertification_count == 1U,
      "the certified same-context reopen did not rebuild and publish from HEAD");
  require_store_scope(reopened.status());
  require_bounded_memory_audit(after_resume);
}

void test_publish_reopen_finish_and_recertify() {
  constexpr std::size_t requested_maximum_order = 10U;
  require(
      sparse_higher_support_h0_chunk_run_schema_version == 1U &&
          sparse_higher_support_h0_chunk_run_backend == "reference_cpu" &&
          sparse_higher_support_h0_chunk_run_profile == "hgp_reduced" &&
          sparse_higher_support_h0_chunk_run_mode ==
              "durable_recertified_higher_h0_chunk_run" &&
          sparse_higher_support_h0_chunk_run_deployment_status ==
              "architecture_only" &&
          sparse_higher_support_h0_chunk_run_public_status == "not_claimed",
      "the Phase-14AB schema or non-public status drifted");

  const CanonicalPointCloud cloud = regular_tetrahedron();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportAuthorityContext authority{
      index, cloud, requested_maximum_order};
  const ExpectedSequence expected = build_expected_sequence(authority);
  require(
      expected.runs.size() > 2U &&
          expected.runs.size() == expected.cumulative_audits.size() &&
          expected.runs.size() == expected.statuses.size() &&
          expected.empty_segment_count != 0U &&
          expected.event_segment_count != 0U &&
          expected.cumulative_audits.back().accepted_event_count == 5U &&
          expected.statuses.back() ==
              ExactHigherSupportStreamStatus::complete,
      "the short tetrahedron fixture covers neither empty nor event chunks");

  TemporaryWorkspace workspace;
  const std::filesystem::path run_directory =
      workspace.make_run_directory();
  AtomicLinearRunExternalAnchor prefix_anchor;
  AtomicLinearRunExternalAnchor terminal_anchor;
  std::vector<ExactSparseDirectH0CandidateRun> observed_runs;
  std::vector<ExactHigherSupportStreamAudit> observed_audits;

  {
    ExactSparseHigherSupportH0ChunkRunContext context{
        authority,
        chunk_budget(),
        projection_limits(),
        chunk_run_limits()};
    auto store = context.create_new_store(run_directory, store_limits());
    require(
        context.run_contract().application_contract_digest ==
                context.application_contract_digest() &&
            context.run_contract().initial_checkpoint_digest ==
                context.initial_checkpoint_digest(),
        "the Phase-14AB application contract lost its root anchor");

    TransitionCapture capture;
    AtomicLinearRunPublishOptions options;
    options.pre_head_commit = capture_transition;
    options.pre_head_commit_state = &capture;
    const AtomicLinearRunPublishResult publication =
        context.publish_next_chunk(store, options);
    require(
        publication.decision ==
                AtomicLinearRunPublishDecision::durably_published &&
            publication.trusted_state_advanced &&
            publication.process_local_commit_attempted &&
            publication.process_local_commit_succeeded &&
            !capture.wrong_projection_type &&
            capture.transition.has_value(),
        "the first Phase-14AB chunk was not durably published");
    append_projection(
        accepted_projection(publication), observed_runs, observed_audits);
    require(
        observed_runs.front() == expected.runs.front() &&
            observed_audits.front() == expected.cumulative_audits.front() &&
            !accepted_projection(publication).session_complete,
        "the first durable chunk differs from the Phase-14AA reference");
    prefix_anchor = publication.current_anchor;

    const AtomicLinearRunRecertification canonical =
        context.recertify_for_validation(
            *capture.transition,
            AtomicLinearRunRecertificationPhase::publication);
    const auto* canonical_projection = dynamic_cast<const
        ExactSparseHigherSupportH0RecertifiedChunkProjection*>(
            canonical.accepted_projection.get());
    require(
        canonical.accepted() && canonical_projection != nullptr &&
            canonical_projection->run == expected.runs.front(),
        "fresh validation did not reproduce the captured durable chunk");

    AtomicLinearRunTransition wrong_budget = *capture.transition;
    auto wrong_budget_bytes =
        wrong_budget.budget_snapshot_digest.bytes();
    wrong_budget_bytes.front() ^= UINT8_C(0x01);
    wrong_budget.budget_snapshot_digest =
        CanonicalId{wrong_budget_bytes};
    AtomicLinearRunTransition wrong_payload = *capture.transition;
    require(
        !wrong_payload.payload.empty(),
        "the Phase-14AB hostile payload fixture is empty");
    wrong_payload.payload[wrong_payload.payload.size() / 2U] ^=
        UINT8_C(0x01);
    const AtomicLinearRunRecertification budget_rejection =
        context.recertify_for_validation(
            wrong_budget,
            AtomicLinearRunRecertificationPhase::publication);
    const AtomicLinearRunRecertification payload_rejection =
        context.recertify_for_validation(
            wrong_payload,
            AtomicLinearRunRecertificationPhase::publication);
    const AtomicLinearRunRecertification healthy_after_mutations =
        context.recertify_for_validation(
            *capture.transition,
            AtomicLinearRunRecertificationPhase::publication);
    const auto* healthy_projection = dynamic_cast<const
        ExactSparseHigherSupportH0RecertifiedChunkProjection*>(
            healthy_after_mutations.accepted_projection.get());
    require(
        !budget_rejection.accepted() &&
            !budget_rejection.accepted_projection &&
            !payload_rejection.accepted() &&
            !payload_rejection.accepted_projection &&
            healthy_after_mutations.accepted() &&
            healthy_projection != nullptr &&
            healthy_projection->run == expected.runs.front() &&
            context.audit().rejected_recertification_count == 2U &&
            store.status().committed_transition_count == 1U,
        "a mutation passed recertification or poisoned the verifier cache");
    require_bounded_memory_audit(context.audit());
  }

  {
    ExactSparseHigherSupportH0ChunkRunContext context{
        authority,
        chunk_budget(),
        projection_limits(),
        chunk_run_limits()};
    std::vector<ExactSparseDirectH0CandidateRun> recovered_prefix_runs;
    std::vector<ExactHigherSupportStreamAudit> recovered_prefix_audits;
    auto store = context.open_existing_store(
        run_directory,
        store_limits(),
        prefix_anchor,
        [&](const ExactSparseHigherSupportH0RecertifiedChunkProjection&
                projection) {
          append_projection(
              projection,
              recovered_prefix_runs,
              recovered_prefix_audits);
        });
    require_store_scope(store.status());
    require(
        store.status().external_anchor_supplied &&
            store.status().external_anchor_verified &&
            store.status().recovered_transition_count == 1U &&
            recovered_prefix_runs == observed_runs &&
            recovered_prefix_runs ==
                std::vector<ExactSparseDirectH0CandidateRun>{
                    expected.runs.front()} &&
            recovered_prefix_audits == observed_audits,
        "reopening the anchored prefix changed its recertified projection");

    bool terminal_seen = false;
    while (observed_runs.size() < expected.runs.size()) {
      const AtomicLinearRunPublishResult publication =
          context.publish_next_chunk(store);
      require(
          publication.decision ==
                  AtomicLinearRunPublishDecision::durably_published &&
              publication.trusted_state_advanced,
          "a resumed Phase-14AB chunk was not durably published");
      const auto& projection = accepted_projection(publication);
      append_projection(projection, observed_runs, observed_audits);
      require(
          projection.run == expected.runs[projection.source_chunk_count] &&
              projection.cumulative_audit_after ==
                  expected.cumulative_audits[
                      projection.source_chunk_count] &&
              !projection.run_chunk_limit_reached,
          "a resumed durable chunk differs from the Phase-14AA reference");
      terminal_seen = projection.session_complete;
      terminal_anchor = publication.current_anchor;
    }
    require(
        terminal_seen && observed_runs == expected.runs &&
            observed_audits == expected.cumulative_audits &&
            store.status().committed_transition_count ==
                expected.runs.size(),
        "the resumed Phase-14AB run did not finish at the reference audit");
    require_bounded_memory_audit(context.audit());
  }

  {
    ExactSparseHigherSupportH0ChunkRunContext context{
        authority,
        chunk_budget(),
        projection_limits(),
        chunk_run_limits()};
    std::vector<ExactSparseDirectH0CandidateRun> recovered_runs;
    std::vector<ExactHigherSupportStreamAudit> recovered_audits;
    bool recovered_terminal = false;
    auto store = context.open_existing_store(
        run_directory,
        store_limits(),
        terminal_anchor,
        [&](const ExactSparseHigherSupportH0RecertifiedChunkProjection&
                projection) {
          append_projection(projection, recovered_runs, recovered_audits);
          recovered_terminal = projection.session_complete;
        });
    require_store_scope(store.status());
    require(
        store.status().external_anchor_supplied &&
            store.status().external_anchor_verified &&
            store.status().recovered_transition_count ==
                expected.runs.size() &&
            store.status().committed_transition_count ==
                expected.runs.size() &&
            recovered_terminal && recovered_runs == expected.runs &&
            recovered_audits == expected.cumulative_audits &&
            context.audit().recovery_recertification_count ==
                expected.runs.size(),
        "the final anchored reopen changed the complete run or audit sequence");
    require_bounded_memory_audit(context.audit());
  }
}

}  // namespace

int main() {
  try {
    test_general_rational_wire_budgets_are_syntax_only();
    test_non_dyadic_1105_over_242_durable_round_trip();
    test_context_binding_payload_floor_and_chunk_cap();
    test_retryable_pre_head_failure_reuses_pending_segment();
    test_indeterminate_failure_requires_anchored_reopen();
    test_publish_reopen_finish_and_recertify();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "sparse higher-support H0 chunk-run tests passed\n";
  return 0;
}
