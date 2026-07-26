#include "morsehgp3d/hierarchy/sparse_anchored_pair_chunk_run.hpp"

#include "morsehgp3d/exact/point.hpp"

#include <array>
#include <cerrno>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <unistd.h>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::contract::CanonicalId;
using morsehgp3d::contract::CanonicalSha256Builder;
using morsehgp3d::exact::CertifiedPoint3;
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
        (parent_ / "morsehgp3d-pair-chunk-XXXXXX").string();
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
            "morsehgp3d-pair-chunk-")) {
      std::error_code ignored;
      static_cast<void>(std::filesystem::remove_all(root_, ignored));
    }
  }

  TemporaryWorkspace(const TemporaryWorkspace&) = delete;
  TemporaryWorkspace& operator=(const TemporaryWorkspace&) = delete;

  [[nodiscard]] std::filesystem::path make_run_directory() const {
    const std::filesystem::path directory = root_ / "run";
    if (!std::filesystem::create_directory(directory)) {
      throw std::runtime_error("cannot create the 14V run directory");
    }
    return directory;
  }

 private:
  std::filesystem::path parent_;
  std::filesystem::path root_;
};

[[nodiscard]] CanonicalId digest(std::string_view text) {
  CanonicalSha256Builder builder;
  builder.update("MorseHGP3D/phase14V/test/");
  builder.update(text);
  return builder.finalize();
}

[[nodiscard]] CanonicalPointCloud make_cloud() {
  const std::array<CertifiedPoint3, 4U> points{
      CertifiedPoint3::from_binary64(0.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(2.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(3.0, 0.0, 0.0)};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactSparseAnchoredPairSessionTotalCapacity total_capacity() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum,
          maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactSparseAnchoredPairSessionAdvanceBudget advance_budget(
    std::size_t maximum_closed_rank) {
  return {{4096U, 4096U, 4096U, 4096U},
          {4096U},
          1U,
          maximum_closed_rank + 1U};
}

[[nodiscard]] ExactSparseAnchoredPairChunkRunLimits chunk_limits() {
  return {
      2U,
      4096U,
      2U,
      8U,
      512U,
      16U * 1024U,
      64U * 1024U};
}

[[nodiscard]] AtomicLinearRunStoreLimits store_limits() {
  constexpr std::size_t transition_count = 1024U;
  constexpr std::size_t payload_bytes = 64U * 1024U;
  constexpr std::size_t encoded_bytes =
      atomic_linear_run_transition_fixed_wire_byte_count + payload_bytes;
  return {
      transition_count,
      payload_bytes,
      encoded_bytes,
      transition_count * encoded_bytes,
      2U};
}

[[nodiscard]] std::size_t reference_count(
    const ExactSparseAnchoredPairRecord& record) {
  if (const auto* event = std::get_if<ExactPairSupportEvent>(&record)) {
    return 2U + event->interior_ids.size();
  }
  return 3U +
      std::get<ExactPairSupportExtraShellDiagnostic>(record)
          .interior_ids.size();
}

void append_projection(
    const ExactSparseAnchoredPairRecertifiedChunkProjection& projection,
    std::vector<ExactSparseAnchoredPairRecord>& records,
    std::size_t& point_id_reference_count) {
  require(
      projection.record_segment.first_output_record_index == records.size(),
      "a 14V segment lost its global record offset");
  require(
      projection.record_segment.first_output_point_id_reference_index ==
          point_id_reference_count,
      "a 14V segment lost its global point-reference offset");
  for (const ExactSparseAnchoredPairRecord& record :
       projection.record_segment.records) {
    point_id_reference_count += reference_count(record);
    records.push_back(record);
  }
}

[[nodiscard]] ExactSparseAnchoredPairTerminalAuthority resident_reference(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank,
    ExactMortonGroupedAnchoredPairScheduleConfig schedule_config) {
  ExactSparseAnchoredPairSession session = ExactSparseAnchoredPairSession::start(
      index,
      cloud,
      maximum_closed_rank,
      schedule_config,
      total_capacity());
  for (std::size_t call = 0U; call < 4096U; ++call) {
    const auto step = session.advance(
        index, cloud, advance_budget(maximum_closed_rank));
    require(
        step.kind() !=
            ExactSparseAnchoredPairSessionStepKind::total_capacity_exhausted,
        "the resident reference exhausted a roomy capacity");
    if (step.kind() == ExactSparseAnchoredPairSessionStepKind::complete) {
      return std::move(session).seal();
    }
  }
  throw std::runtime_error("the resident reference did not terminate");
}

void test_publish_reopen_and_finish() {
  constexpr std::size_t maximum_closed_rank = 3U;
  const ExactMortonGroupedAnchoredPairScheduleConfig schedule_config{4U, 0U};
  CanonicalPointCloud cloud = make_cloud();
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  ExactSparseAnchoredPairTerminalAuthority reference = resident_reference(
      index, cloud, maximum_closed_rank, schedule_config);

  TemporaryWorkspace workspace;
  const std::filesystem::path run_directory =
      workspace.make_run_directory();
  const CanonicalId initial_output_chain = digest("initial-output-chain");
  AtomicLinearRunExternalAnchor first_anchor;
  std::vector<ExactSparseAnchoredPairRecord> streamed_records;
  std::size_t streamed_reference_count = 0U;
  ExactSparseAnchoredPairSessionAudit final_audit{};
  bool terminal_seen = false;

  {
    ExactPairSupportAuthorityContext authority(
        index, cloud, maximum_closed_rank - 1U);
    ExactSparseAnchoredPairChunkRunContext context(
        authority,
        maximum_closed_rank,
        schedule_config,
        total_capacity(),
        advance_budget(maximum_closed_rank),
        chunk_limits());
    auto store = context.create_new_store(
        run_directory, initial_output_chain, store_limits());
    const AtomicLinearRunPublishResult result =
        context.publish_next_chunk(store);
    require(
        result.decision == AtomicLinearRunPublishDecision::durably_published &&
            result.trusted_state_advanced,
        "the first 14V chunk was not durably published");
    const auto* projection = dynamic_cast<const
        ExactSparseAnchoredPairRecertifiedChunkProjection*>(
            result.recertification.accepted_projection.get());
    require(projection != nullptr, "the first 14V projection is missing");
    append_projection(
        *projection, streamed_records, streamed_reference_count);
    require(
        !projection->session_complete,
        "the restart fixture unexpectedly completed in one chunk");
    first_anchor = result.current_anchor;
  }

  {
    ExactPairSupportAuthorityContext authority(
        index, cloud, maximum_closed_rank - 1U);
    ExactSparseAnchoredPairChunkRunContext context(
        authority,
        maximum_closed_rank,
        schedule_config,
        total_capacity(),
        advance_budget(maximum_closed_rank),
        chunk_limits());
    std::vector<ExactSparseAnchoredPairRecord> recovered_records;
    std::size_t recovered_reference_count = 0U;
    auto store = context.open_existing_store(
        run_directory,
        initial_output_chain,
        store_limits(),
        first_anchor,
        [&](const ExactSparseAnchoredPairRecertifiedChunkProjection& projection) {
          append_projection(
              projection, recovered_records, recovered_reference_count);
        });
    require(
        recovered_records == streamed_records &&
            recovered_reference_count == streamed_reference_count &&
            store.status().linear_prefix_replayed &&
            store.status().retained_transition_history_count == 0U &&
            store.status().global_gamma_cell_count == 0U &&
            store.status().higher_order_delaunay_cell_count == 0U,
        "the reopened 14V prefix lost data or materialized forbidden state");

    for (std::size_t chunk = 0U; chunk < 1024U; ++chunk) {
      const AtomicLinearRunPublishResult result =
          context.publish_next_chunk(store);
      require(
          result.decision ==
                  AtomicLinearRunPublishDecision::durably_published &&
              result.trusted_state_advanced,
          "a resumed 14V chunk was not durably published");
      const auto* projection = dynamic_cast<const
          ExactSparseAnchoredPairRecertifiedChunkProjection*>(
              result.recertification.accepted_projection.get());
      require(projection != nullptr, "a resumed 14V projection is missing");
      append_projection(
          *projection, streamed_records, streamed_reference_count);
      final_audit = projection->cumulative_audit;
      if (projection->session_complete) {
        terminal_seen = true;
        break;
      }
      require(
          !projection->total_capacity_exhausted &&
              !projection->run_advance_limit_reached,
          "the roomy 14V fixture stopped before scientific completion");
    }
    const ExactSparseAnchoredPairChunkRunAudit audit = context.audit();
    require(
        audit.maximum_retained_record_count <=
                chunk_limits().maximum_record_count_per_chunk &&
            audit.maximum_retained_point_id_reference_count <=
                chunk_limits().maximum_point_id_reference_count_per_chunk &&
            audit.retained_transition_history_count == 0U &&
            audit.context_owned_global_pair_count == 0U &&
            audit.context_owned_global_facet_or_coface_count == 0U &&
            audit.context_owned_cell_gamma_or_delaunay_count == 0U &&
            audit.maximum_live_session_cache_count == 2U,
        "the 14V context exceeded its declared bounded residency");
  }

  require(terminal_seen, "the resumed 14V run did not terminate");
  require(
      streamed_records == std::vector<ExactSparseAnchoredPairRecord>{
          reference.records().begin(), reference.records().end()},
      "chunking changed the exact P8l record stream");
  ExactSparseAnchoredPairSessionAudit expected_audit = reference.audit();
  final_audit.retained_records_certified = true;
  require(
      final_audit == expected_audit,
      "chunking changed the cumulative P8l scientific audit");
}

}  // namespace

int main() {
  try {
    test_publish_reopen_and_finish();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "sparse anchored pair chunk-run tests passed\n";
  return 0;
}
