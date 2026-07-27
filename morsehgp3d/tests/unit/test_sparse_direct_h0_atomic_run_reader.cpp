#include "morsehgp3d/hierarchy/sparse_direct_h0_atomic_run_reader.hpp"

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
        (parent_ / "morsehgp3d-atomic-h0-reader-XXXXXX").string();
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
            "morsehgp3d-atomic-h0-reader-")) {
      std::error_code ignored;
      static_cast<void>(std::filesystem::remove_all(root_, ignored));
    }
  }

  TemporaryWorkspace(const TemporaryWorkspace&) = delete;
  TemporaryWorkspace& operator=(const TemporaryWorkspace&) = delete;

  [[nodiscard]] std::filesystem::path make_run_directory(
      std::string_view name) const {
    const std::filesystem::path directory = root_ / name;
    if (!std::filesystem::create_directory(directory)) {
      throw std::runtime_error("cannot create an atomic H0 reader directory");
    }
    return directory;
  }

 private:
  std::filesystem::path parent_;
  std::filesystem::path root_;
};

[[nodiscard]] CanonicalId initial_output_chain() {
  CanonicalSha256Builder builder;
  builder.update("MorseHGP3D/phase14Z/test/initial-output-chain");
  return builder.finalize();
}

[[nodiscard]] CanonicalPointCloud right_triangle() {
  const std::array<CertifiedPoint3, 3U> points{
      CertifiedPoint3::from_binary64(0.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(2.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(0.0, 2.0, 0.0)};
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
  return {1U, 4096U, 1U, 4U, 512U, 16U * 1024U, 64U * 1024U};
}

[[nodiscard]] AtomicLinearRunStoreLimits store_limits() {
  constexpr std::size_t transition_count = 4096U;
  constexpr std::size_t payload_bytes = 64U * 1024U;
  constexpr std::size_t encoded_bytes =
      atomic_linear_run_transition_fixed_wire_byte_count + payload_bytes;
  return {transition_count,
          payload_bytes,
          encoded_bytes,
          transition_count * encoded_bytes,
          1U};
}

[[nodiscard]] ExactSparseDirectH0AtomicPairRunReaderLimits reader_limits() {
  return {
      {1U, 1U, 1U, 4U},
      {1U, 1U, 4U},
      4096U,
      4096U,
      4096U,
      16U * 1024U};
}

[[nodiscard]] ExactSparseDirectH0CandidateRun project(
    const ExactSparseAnchoredPairRecertifiedChunkProjection& source,
    std::size_t point_count,
    std::size_t effective_maximum_order) {
  const ExactSparseDirectH0AtomicPairRunReaderLimits limits = reader_limits();
  return project_exact_sparse_direct_h0_pair_candidate_run(
      project_exact_sparse_anchored_pair_h0_run(
          source,
          point_count,
          effective_maximum_order,
          limits.pair_projection),
      limits.common_projection);
}

struct PublishedFixture {
  AtomicLinearRunExternalAnchor first_anchor{};
  AtomicLinearRunExternalAnchor final_anchor{};
  std::vector<ExactSparseDirectH0CandidateRun> expected_runs;
};

[[nodiscard]] PublishedFixture publish_complete_run(
    const ExactSparseAnchoredPairChunkRunContext& context,
    const std::filesystem::path& directory,
    const CanonicalId& output_chain,
    std::size_t point_count,
    std::size_t effective_maximum_order) {
  PublishedFixture result;
  auto store = context.create_new_store(
      directory, output_chain, store_limits());
  for (std::size_t index = 0U; index < 4096U; ++index) {
    const AtomicLinearRunPublishResult published =
        context.publish_next_chunk(store);
    require(
        published.decision ==
                AtomicLinearRunPublishDecision::durably_published &&
            published.trusted_state_advanced,
        "the 14Z fixture could not publish a source chunk");
    const auto* projection = dynamic_cast<const
        ExactSparseAnchoredPairRecertifiedChunkProjection*>(
            published.recertification.accepted_projection.get());
    require(projection != nullptr, "the 14Z fixture lost its projection");
    result.expected_runs.push_back(project(
        *projection, point_count, effective_maximum_order));
    if (index == 0U) {
      result.first_anchor = published.current_anchor;
    }
    result.final_anchor = published.current_anchor;
    if (projection->session_complete) {
      return result;
    }
    require(
        !projection->total_capacity_exhausted &&
            !projection->run_advance_limit_reached,
        "the 14Z fixture stopped before terminality");
  }
  throw std::runtime_error("the 14Z fixture did not terminate");
}

[[nodiscard]] AtomicLinearRunExternalAnchor publish_incomplete_prefix(
    const ExactSparseAnchoredPairChunkRunContext& context,
    const std::filesystem::path& directory,
    const CanonicalId& output_chain) {
  auto store = context.create_new_store(
      directory, output_chain, store_limits());
  const AtomicLinearRunPublishResult published =
      context.publish_next_chunk(store);
  require(
      published.decision ==
          AtomicLinearRunPublishDecision::durably_published,
      "the incomplete 14Z fixture could not publish its prefix");
  const auto* projection = dynamic_cast<const
      ExactSparseAnchoredPairRecertifiedChunkProjection*>(
          published.recertification.accepted_projection.get());
  require(
      projection != nullptr && !projection->session_complete,
      "the incomplete 14Z fixture unexpectedly reached terminality");
  return published.current_anchor;
}

void test_certified_bounded_reader() {
  constexpr std::size_t maximum_closed_rank = 3U;
  constexpr std::size_t effective_maximum_order = 2U;
  const ExactMortonGroupedAnchoredPairScheduleConfig schedule_config{4U, 0U};
  CanonicalPointCloud cloud = right_triangle();
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  ExactPairSupportAuthorityContext authority(
      index, cloud, maximum_closed_rank - 1U);
  ExactSparseAnchoredPairChunkRunContext context(
      authority,
      maximum_closed_rank,
      schedule_config,
      total_capacity(),
      advance_budget(maximum_closed_rank),
      chunk_limits());
  TemporaryWorkspace workspace;
  const std::filesystem::path complete_directory =
      workspace.make_run_directory("complete");
  const CanonicalId output_chain = initial_output_chain();
  const PublishedFixture fixture = publish_complete_run(
      context,
      complete_directory,
      output_chain,
      cloud.size(),
      effective_maximum_order);
  require(
      fixture.expected_runs.size() > 1U &&
          fixture.first_anchor.committed_transition_count <
              fixture.final_anchor.committed_transition_count,
      "the 14Z fixture did not produce a multi-run prefix");

  require_throws<std::runtime_error>(
      [&] {
        static_cast<void>(read_exact_sparse_direct_h0_atomic_pair_runs(
            context,
            complete_directory,
            output_chain,
            store_limits(),
            fixture.final_anchor,
            cloud.size(),
            effective_maximum_order,
            reader_limits(),
            [](std::size_t, const ExactSparseDirectH0CandidateRun&) {
              throw std::runtime_error("scripted borrowed visitor failure");
            }));
      },
      "the 14Z reader accepted a throwing visitor");

  std::vector<ExactSparseDirectH0CandidateRun> observed_runs;
  const ExactSparseDirectH0AtomicPairRunReaderAudit audit =
      read_exact_sparse_direct_h0_atomic_pair_runs(
          context,
          complete_directory,
          output_chain,
          store_limits(),
          fixture.final_anchor,
          cloud.size(),
          effective_maximum_order,
          reader_limits(),
          [&](std::size_t run_index,
              const ExactSparseDirectH0CandidateRun& run) {
            require(
                run_index == observed_runs.size(),
                "the 14Z callback order is not sequential");
            observed_runs.push_back(run);
          });
  require(
      observed_runs == fixture.expected_runs,
      "the 14Z recovery reader changed a projected run");

  std::size_t candidate_count = 0U;
  std::size_t diagnostic_count = 0U;
  std::size_t reference_count = 0U;
  for (const ExactSparseDirectH0CandidateRun& run : observed_runs) {
    candidate_count += run.candidates.size();
    diagnostic_count += run.diagnostics.size();
    reference_count += run.source_point_id_reference_count;
  }
  require(
      candidate_count > 0U && diagnostic_count > 0U &&
          reference_count > 0U,
      "the 14Z fixture does not exercise all global populations");
  require(
      audit.visited_transition_count == observed_runs.size() &&
          audit.emitted_run_count == observed_runs.size() &&
          audit.emitted_candidate_count == candidate_count &&
          audit.emitted_diagnostic_count == diagnostic_count &&
          audit.emitted_point_id_reference_count == reference_count &&
          audit.terminal_projection_count == 1U &&
          audit.maximum_reader_owned_live_run_count == 1U &&
          audit.retained_reader_owned_run_history_count == 0U &&
          audit.source_retained_transition_history_count == 0U &&
          audit.source_global_gamma_cell_count == 0U &&
          audit.source_higher_order_delaunay_cell_count == 0U &&
          audit.final_anchor == fixture.final_anchor &&
          audit.final_anchor_exactly_verified &&
          audit.terminal_projection_unique_and_last &&
          audit.runs_individually_strictly_sorted &&
          audit.callback_runs_borrowed && !audit.global_merge_performed &&
          !audit.global_event_indices_assigned &&
          !audit.hierarchy_reduction_performed &&
          !audit.complete_h0_authority_claimed,
      "the 14Z audit overclaimed scope or lost boundedness");

  std::size_t old_anchor_callback_count = 0U;
  require_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(read_exact_sparse_direct_h0_atomic_pair_runs(
            context,
            complete_directory,
            output_chain,
            store_limits(),
            fixture.first_anchor,
            cloud.size(),
            effective_maximum_order,
            reader_limits(),
            [&](std::size_t, const ExactSparseDirectH0CandidateRun&) {
              ++old_anchor_callback_count;
            }));
      },
      "the 14Z reader accepted an older non-final anchor");
  require(
      old_anchor_callback_count == 0U,
      "an older anchor escaped one provisional run");

  ExactSparseDirectH0AtomicPairRunReaderLimits too_few_runs =
      reader_limits();
  too_few_runs.maximum_run_count = observed_runs.size() - 1U;
  require_throws<std::length_error>(
      [&] {
        static_cast<void>(read_exact_sparse_direct_h0_atomic_pair_runs(
            context,
            complete_directory,
            output_chain,
            store_limits(),
            fixture.final_anchor,
            cloud.size(),
            effective_maximum_order,
            too_few_runs,
            [](std::size_t, const ExactSparseDirectH0CandidateRun&) {}));
      },
      "the 14Z reader accepted a one-below run cap");

  ExactSparseDirectH0AtomicPairRunReaderLimits too_few_candidates =
      reader_limits();
  too_few_candidates.maximum_total_candidate_count = candidate_count - 1U;
  require_throws<std::length_error>(
      [&] {
        static_cast<void>(read_exact_sparse_direct_h0_atomic_pair_runs(
            context,
            complete_directory,
            output_chain,
            store_limits(),
            fixture.final_anchor,
            cloud.size(),
            effective_maximum_order,
            too_few_candidates,
            [](std::size_t, const ExactSparseDirectH0CandidateRun&) {}));
      },
      "the 14Z reader accepted a one-below candidate cap");

  ExactSparseDirectH0AtomicPairRunReaderLimits too_few_diagnostics =
      reader_limits();
  too_few_diagnostics.maximum_total_diagnostic_count =
      diagnostic_count - 1U;
  require_throws<std::length_error>(
      [&] {
        static_cast<void>(read_exact_sparse_direct_h0_atomic_pair_runs(
            context,
            complete_directory,
            output_chain,
            store_limits(),
            fixture.final_anchor,
            cloud.size(),
            effective_maximum_order,
            too_few_diagnostics,
            [](std::size_t, const ExactSparseDirectH0CandidateRun&) {}));
      },
      "the 14Z reader accepted a one-below diagnostic cap");

  ExactSparseDirectH0AtomicPairRunReaderLimits too_few_references =
      reader_limits();
  too_few_references.maximum_total_point_id_reference_count =
      reference_count - 1U;
  require_throws<std::length_error>(
      [&] {
        static_cast<void>(read_exact_sparse_direct_h0_atomic_pair_runs(
            context,
            complete_directory,
            output_chain,
            store_limits(),
            fixture.final_anchor,
            cloud.size(),
            effective_maximum_order,
            too_few_references,
            [](std::size_t, const ExactSparseDirectH0CandidateRun&) {}));
      },
      "the 14Z reader accepted a one-below reference cap");

  const std::filesystem::path incomplete_directory =
      workspace.make_run_directory("incomplete");
  const AtomicLinearRunExternalAnchor incomplete_anchor =
      publish_incomplete_prefix(context, incomplete_directory, output_chain);
  require_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(read_exact_sparse_direct_h0_atomic_pair_runs(
            context,
            incomplete_directory,
            output_chain,
            store_limits(),
            incomplete_anchor,
            cloud.size(),
            effective_maximum_order,
            reader_limits(),
            [](std::size_t, const ExactSparseDirectH0CandidateRun&) {}));
      },
      "the 14Z reader accepted an exactly anchored non-terminal prefix");
}

}  // namespace

int main() {
  try {
    test_certified_bounded_reader();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "sparse direct H0 atomic run reader tests passed\n";
  return 0;
}
