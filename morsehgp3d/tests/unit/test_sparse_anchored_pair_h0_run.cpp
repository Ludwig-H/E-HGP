#include "morsehgp3d/hierarchy/sparse_anchored_pair_h0_run.hpp"

#include "morsehgp3d/exact/point.hpp"

#include <algorithm>
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

template <class Exception, class Function>
void require_throws(Function&& function, std::string_view message) {
  try {
    std::forward<Function>(function)();
  } catch (const Exception&) {
    return;
  }
  throw std::runtime_error(std::string{message});
}

class TemporaryWorkspace {
 public:
  TemporaryWorkspace() {
    parent_ = std::filesystem::temp_directory_path();
    std::string pattern =
        (parent_ / "morsehgp3d-pair-h0-XXXXXX").string();
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
        root_.filename().string().starts_with("morsehgp3d-pair-h0-")) {
      std::error_code ignored;
      static_cast<void>(std::filesystem::remove_all(root_, ignored));
    }
  }

  [[nodiscard]] std::filesystem::path make_run_directory() const {
    const std::filesystem::path directory = root_ / "run";
    if (!std::filesystem::create_directory(directory)) {
      throw std::runtime_error("cannot create the 14W test directory");
    }
    return directory;
  }

 private:
  std::filesystem::path parent_;
  std::filesystem::path root_;
};

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
  return {2U, 4096U, 2U, 8U, 512U, 16U * 1024U, 64U * 1024U};
}

[[nodiscard]] AtomicLinearRunStoreLimits store_limits() {
  constexpr std::size_t transition_count = 1024U;
  constexpr std::size_t payload_bytes = 64U * 1024U;
  constexpr std::size_t encoded_bytes =
      atomic_linear_run_transition_fixed_wire_byte_count + payload_bytes;
  return {transition_count, payload_bytes, encoded_bytes,
          transition_count * encoded_bytes, 2U};
}

[[nodiscard]] ExactSparseAnchoredPairH0RunLimits h0_limits() {
  return {2U, 2U, 2U, 8U};
}

[[nodiscard]] CanonicalId initial_output_chain() {
  CanonicalSha256Builder builder;
  builder.update("MorseHGP3D/phase14W/test/initial-output-chain");
  return builder.finalize();
}

[[nodiscard]] ExactSparseAnchoredPairTerminalAuthority resident_reference(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank,
    ExactMortonGroupedAnchoredPairScheduleConfig schedule_config) {
  ExactSparseAnchoredPairSession session = ExactSparseAnchoredPairSession::start(
      index, cloud, maximum_closed_rank, schedule_config, total_capacity());
  for (std::size_t call = 0U; call < 4096U; ++call) {
    const auto step = session.advance(
        index, cloud, advance_budget(maximum_closed_rank));
    require(
        step.kind() !=
            ExactSparseAnchoredPairSessionStepKind::total_capacity_exhausted,
        "the resident 14W reference exhausted its capacity");
    if (step.kind() == ExactSparseAnchoredPairSessionStepKind::complete) {
      return std::move(session).seal();
    }
  }
  throw std::runtime_error("the resident 14W reference did not terminate");
}

[[nodiscard]] ExactSparseAnchoredPairH0EventCandidate expected_candidate(
    std::size_t source_index,
    const ExactPairSupportEvent& event,
    std::size_t effective_maximum_order) {
  ExactSparseAnchoredPairH0EventCandidate result;
  result.source_output_record_index = source_index;
  result.event = event;
  if (event.closed_rank <= effective_maximum_order) {
    result.birth_order = event.closed_rank;
  }
  if (event.closed_rank >= 2U &&
      event.closed_rank - 1U <= effective_maximum_order) {
    result.saddle_order = event.closed_rank - 1U;
  }
  return result;
}

void test_bounded_pair_only_projection() {
  constexpr std::size_t maximum_closed_rank = 3U;
  constexpr std::size_t effective_maximum_order = 2U;
  const ExactMortonGroupedAnchoredPairScheduleConfig schedule_config{4U, 0U};
  CanonicalPointCloud cloud = make_cloud();
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  ExactSparseAnchoredPairTerminalAuthority reference = resident_reference(
      index, cloud, maximum_closed_rank, schedule_config);

  std::vector<ExactSparseAnchoredPairH0EventCandidate> observed_events;
  std::vector<ExactSparseAnchoredPairH0Diagnostic> observed_diagnostics;
  bool terminal_seen = false;
  bool rejection_checked = false;
  TemporaryWorkspace workspace;
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
      workspace.make_run_directory(), initial_output_chain(), store_limits());

  for (std::size_t chunk_index = 0U; chunk_index < 1024U; ++chunk_index) {
    const AtomicLinearRunPublishResult published =
        context.publish_next_chunk(store);
    require(
        published.decision ==
            AtomicLinearRunPublishDecision::durably_published,
        "a 14W source chunk was not durably published");
    const auto* projection = dynamic_cast<const
        ExactSparseAnchoredPairRecertifiedChunkProjection*>(
            published.recertification.accepted_projection.get());
    require(projection != nullptr, "a 14W source projection is missing");

    const ExactSparseAnchoredPairH0Run run =
        project_exact_sparse_anchored_pair_h0_run(
            *projection,
            cloud.size(),
            effective_maximum_order,
            h0_limits());
    require(
        run.pair_only &&
            run.candidates_sorted_by_terminal_facade_event_key &&
            run.diagnostics_preserved &&
            !run.global_event_indices_assigned &&
            !run.supports_three_and_four_joined &&
            !run.complete_h0_authority_claimed &&
            std::is_sorted(
                run.event_candidates.begin(), run.event_candidates.end(),
                exact_sparse_anchored_pair_h0_event_candidate_less),
        "a 14W run violated its pair-only sorted scope");
    observed_events.insert(
        observed_events.end(),
        run.event_candidates.begin(), run.event_candidates.end());
    observed_diagnostics.insert(
        observed_diagnostics.end(),
        run.diagnostics.begin(), run.diagnostics.end());

    if (!rejection_checked) {
      ExactSparseAnchoredPairRecertifiedChunkProjection bad = *projection;
      ++bad.record_segment.event_count;
      require_throws<std::invalid_argument>(
          [&] {
            static_cast<void>(project_exact_sparse_anchored_pair_h0_run(
                bad,
                cloud.size(),
                effective_maximum_order,
                h0_limits()));
          },
          "14W accepted inconsistent source counts");
      rejection_checked = true;
    }
    if (projection->session_complete) {
      terminal_seen = true;
      break;
    }
    require(
        !projection->total_capacity_exhausted &&
            !projection->run_advance_limit_reached,
        "the 14W source stopped before terminality");
  }

  require(terminal_seen, "the 14W source did not terminate");
  std::vector<ExactSparseAnchoredPairH0EventCandidate> expected_events;
  std::vector<ExactSparseAnchoredPairH0Diagnostic> expected_diagnostics;
  for (std::size_t index_in_stream = 0U;
       index_in_stream < reference.records().size(); ++index_in_stream) {
    const ExactSparseAnchoredPairRecord& record =
        reference.records()[index_in_stream];
    if (const auto* event = std::get_if<ExactPairSupportEvent>(&record)) {
      expected_events.push_back(expected_candidate(
          index_in_stream, *event, effective_maximum_order));
    } else {
      expected_diagnostics.push_back(ExactSparseAnchoredPairH0Diagnostic{
          index_in_stream,
          std::get<ExactPairSupportExtraShellDiagnostic>(record)});
    }
  }
  std::sort(
      expected_events.begin(), expected_events.end(),
      exact_sparse_anchored_pair_h0_event_candidate_less);
  std::sort(
      observed_events.begin(), observed_events.end(),
      exact_sparse_anchored_pair_h0_event_candidate_less);
  require(
      observed_events == expected_events,
      "14W changed an event, locator or H0 role rule");
  require(
      observed_diagnostics == expected_diagnostics,
      "14W did not preserve the pair diagnostics in stream order");
}

}  // namespace

int main() {
  try {
    test_bounded_pair_only_projection();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "sparse anchored pair H0-run tests passed\n";
  return 0;
}
