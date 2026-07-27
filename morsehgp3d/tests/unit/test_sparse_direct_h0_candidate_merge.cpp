#include "morsehgp3d/hierarchy/direct_support_terminal.hpp"
#include "morsehgp3d/hierarchy/sparse_direct_h0_candidate_merge.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;

static_assert(
    !std::is_copy_constructible_v<
        ExactSparseDirectH0CandidateMergeSession>);
static_assert(
    !std::is_move_constructible_v<
        ExactSparseDirectH0CandidateMergeSession>);
static_assert(
    !std::is_copy_constructible_v<ExactHigherSupportUnsealedDrain> &&
        std::is_nothrow_move_constructible_v<
            ExactHigherSupportUnsealedDrain>);
static_assert(
    !std::is_constructible_v<
        ExactHigherSupportUnsealedDrain,
        ExactHigherSupportTerminalSegment&&,
        ExactHigherSupportCheckpointManifest&&>,
    "callers must not be able to forge a segment/manifest drain pair");

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

[[nodiscard]] CanonicalPointCloud shifted_tetrahedron() {
  const std::array<CertifiedPoint3, 4U> points{
      point(11.0, 11.0, 11.0),
      point(11.0, 9.0, 9.0),
      point(9.0, 11.0, 9.0),
      point(9.0, 9.0, 11.0)};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud right_triangle() {
  const std::array<CertifiedPoint3, 3U> points{
      point(0.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
      point(0.0, 2.0, 0.0)};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud five_site_shell() {
  const std::array<CertifiedPoint3, 5U> points{
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0),
      point(-1.0, -1.0, -1.0)};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud rank_prune_cloud() {
  const std::array<CertifiedPoint3, 9U> points{
      point(10.0, 10.0, 10.0),
      point(10.125, 10.0, 10.0),
      point(10.0, -10.0, -10.0),
      point(10.125, -10.0, -10.0),
      point(-10.0, 10.0, -10.0),
      point(-9.875, 10.0, -10.0),
      point(-10.0, -10.0, 10.0),
      point(-9.875, -10.0, 10.0),
      point(0.0, 0.0, 0.0)};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactPairSupportStreamBudget unlimited_pair_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactHigherSupportStreamBudget unlimited_higher_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum,
          maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactHigherSupportStreamBudget segmented_higher_budget() {
  ExactHigherSupportStreamBudget result = unlimited_higher_budget();
  result.maximum_work_unit_count = 1U;
  result.maximum_emitted_record_count = 1U;
  result.maximum_emitted_point_id_reference_count = 32U;
  result.maximum_prune_receipt_count = 8U;
  result.maximum_global_closed_ball_query_count = 8U;
  result.maximum_point_classification_count = 32U;
  return result;
}

[[nodiscard]] std::pair<
    std::optional<std::size_t>, std::optional<std::size_t>>
expected_roles(
    std::size_t closed_rank,
    std::size_t effective_maximum_order) {
  std::optional<std::size_t> birth;
  std::optional<std::size_t> saddle;
  if (closed_rank <= effective_maximum_order) {
    birth = closed_rank;
  }
  if (closed_rank >= 2U &&
      closed_rank - 1U <= effective_maximum_order) {
    saddle = closed_rank - 1U;
  }
  return {birth, saddle};
}

[[nodiscard]] std::size_t event_reference_count(
    const ExactPairSupportEvent& event) {
  return 2U + event.interior_ids.size();
}

[[nodiscard]] ExactSparseAnchoredPairH0Run make_pair_h0_run(
    const ExactPairSupportStreamResult& pair,
    std::size_t point_count,
    std::size_t effective_maximum_order) {
  require(pair.stream_complete(), "the pair fixture is not terminal");

  ExactSparseAnchoredPairH0Run result;
  result.point_count = point_count;
  result.effective_maximum_order = effective_maximum_order;
  result.source_record_count =
      pair.events.size() + pair.relevant_extra_shell_diagnostics.size();
  result.event_candidates.reserve(pair.events.size());
  result.diagnostics.reserve(
      pair.relevant_extra_shell_diagnostics.size());
  std::size_t source_index = 0U;
  for (const ExactPairSupportEvent& event : pair.events) {
    const auto [birth, saddle] =
        expected_roles(event.closed_rank, effective_maximum_order);
    result.event_candidates.push_back(
        ExactSparseAnchoredPairH0EventCandidate{
            source_index, event, birth, saddle});
    result.source_point_id_reference_count +=
        event_reference_count(event);
    ++source_index;
  }
  for (const ExactPairSupportExtraShellDiagnostic& diagnostic :
       pair.relevant_extra_shell_diagnostics) {
    result.diagnostics.push_back(
        ExactSparseAnchoredPairH0Diagnostic{source_index, diagnostic});
    result.source_point_id_reference_count +=
        3U + diagnostic.interior_ids.size();
    ++source_index;
  }
  std::sort(
      result.event_candidates.begin(), result.event_candidates.end(),
      exact_sparse_anchored_pair_h0_event_candidate_less);
  result.candidates_sorted_by_terminal_facade_event_key = true;
  result.diagnostics_preserved = true;
  return result;
}

[[nodiscard]] ExactHigherSupportTerminalAuthority build_higher_authority(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order) {
  ExactHigherSupportTerminalSession session{
      index,
      cloud,
      requested_maximum_order,
      segmented_higher_budget(),
      256U};
  require(
      session.run_to_terminal() ==
          ExactHigherSupportTerminalRunStatus::terminal,
      "the higher fixture did not reach terminality");
  return std::move(session).seal();
}

[[nodiscard]] bool candidate_matches_facade_event(
    const ExactSparseDirectH0Candidate& candidate,
    const ExactDirectSupportEvent& event) {
  return candidate.support_size == event.support_size &&
      candidate.support_ids == event.support_ids &&
      candidate.center == event.center &&
      candidate.squared_level == event.squared_level &&
      candidate.interior_ids == event.interior_ids &&
      candidate.closed_rank == event.closed_rank &&
      candidate.exterior_count == event.exterior_count &&
      candidate.birth_order == event.birth_order &&
      candidate.saddle_order == event.saddle_order;
}

[[nodiscard]] std::vector<ExactSparseDirectH0Candidate> drain(
  ExactSparseDirectH0CandidateMergeSession& session) {
  std::vector<ExactSparseDirectH0Candidate> result;
  while (!session.complete()) {
    ExactSparseDirectH0MergedPage page;
    try {
      page = session.merge_next_page();
    } catch (const std::exception& error) {
      throw std::runtime_error(
          "merge page " + std::to_string(result.size()) + ": " +
          error.what());
    }
    require(
        page.schema_version ==
                sparse_direct_h0_candidate_merge_schema_version &&
            page.point_count == 4U &&
            page.effective_maximum_order == 4U &&
            page.candidates.size() == 1U &&
            page.candidates_strictly_sorted_by_terminal_facade_key &&
            !page.global_event_indices_assigned &&
            !page.hierarchy_reduction_performed &&
            !page.complete_h0_authority_claimed &&
            page.successor_cursor == session.cursor(),
        "a one-record merge page violated its bounded scope");
    const bool expected_complete =
        result.size() + page.candidates.size() ==
        session.audit().validated_candidate_count;
    require(
        page.merge_complete == expected_complete,
        "a merge page published the wrong terminal flag");
    result.insert(
        result.end(), page.candidates.begin(), page.candidates.end());
    require(
        page.successor_cursor.emitted_candidate_count == result.size() &&
            session.audit().input_validation_pass_count == 1U &&
            session.audit().emitted_page_count == result.size() &&
            session.audit().emitted_candidate_count == result.size(),
        "the session-owned merge cursor lost count conservation");
  }
  require(
      session.cursor().last_emitted_position.has_value(),
      "the terminal cursor omitted its last source position");
  return result;
}

void test_cross_arity_projection_and_session_merge() {
  constexpr std::size_t requested_maximum_order = 10U;
  const CanonicalPointCloud cloud = regular_tetrahedron();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportStreamBudget pair_budget = unlimited_pair_budget();
  const ExactHigherSupportStreamBudget higher_budget =
      unlimited_higher_budget();
  const ExactPairSupportStreamResult pair =
      build_exact_pair_support_stream(
          index, cloud, requested_maximum_order, pair_budget);
  const ExactHigherSupportStreamResult higher =
      build_exact_higher_support_stream(
          index, cloud, requested_maximum_order, higher_budget);
  const ExactDirectSupportTerminalFacade facade =
      build_exact_direct_support_terminal_facade(
          index,
          cloud,
          requested_maximum_order,
          ExactDirectSupportTerminalBudget{pair_budget, higher_budget},
          pair,
          higher);
  require(
      facade.terminal_catalog_certified() && facade.events.size() == 11U &&
          facade.relevant_extra_shell_diagnostics.empty(),
      "the resident differential facade is not the eleven-event fixture");
  const std::size_t effective_maximum_order =
      facade.certificate.requirements.effective_maximum_order;

  const ExactSparseAnchoredPairH0Run pair_h0 = make_pair_h0_run(
      pair, cloud.size(), effective_maximum_order);
  const ExactSparseDirectH0CandidateRunLimits projection_limits{
      64U, 64U, 1024U};
  std::vector<ExactSparseDirectH0CandidateRun> runs;
  runs.push_back(project_exact_sparse_direct_h0_pair_candidate_run(
      pair_h0, projection_limits));

  ExactHigherSupportTerminalAuthority authority = build_higher_authority(
      index, cloud, requested_maximum_order);
  std::vector<ExactSparseDirectH0CandidateRun> resident_higher_runs;
  for (std::size_t segment_index = 0U;
       segment_index < authority.segments().size(); ++segment_index) {
    resident_higher_runs.push_back(
        project_exact_sparse_direct_h0_higher_candidate_run(
            authority, segment_index, projection_limits));
  }

  ExactHigherSupportTerminalSession drained_higher{
      index,
      cloud,
      requested_maximum_order,
      segmented_higher_budget(),
      256U};
  bool observed_consumed_payload = false;
  bool observed_retry_after_precommit_rejection = false;
  std::size_t drained_event_count = 0U;
  std::size_t drained_diagnostic_count = 0U;
  std::size_t drained_prune_count = 0U;
  std::size_t drained_rank_receipt_count = 0U;
  std::size_t drained_record_count = 0U;
  std::size_t drained_reference_count = 0U;
  while (!drained_higher.trusted_checkpoint().locally_complete()) {
    auto drained = drained_higher.drain_next_unsealed_segment();
    require(
        drained.status() ==
                ExactHigherSupportUnsealedDrainStatus::segment_ready &&
            drained.segment_available(),
        "the higher producer did not return one ready opaque segment");
    const ExactHigherSupportTerminalSegment& segment = *drained.segment();
    const std::size_t source_candidate_count = segment.events.size();
    const std::size_t source_diagnostic_count =
        segment.relevant_extra_shell_diagnostics.size();
    drained_event_count += source_candidate_count;
    drained_diagnostic_count += source_diagnostic_count;
    drained_prune_count +=
        segment.destroyed_prune_certificate_count;
    drained_rank_receipt_count +=
        segment.destroyed_rank_receipt_count;
    drained_record_count += segment.emitted_record_count();
    drained_reference_count +=
        segment.emitted_point_id_reference_count;
    if (!observed_retry_after_precommit_rejection &&
        segment.emitted_point_id_reference_count != 0U) {
      ExactSparseDirectH0CandidateRunLimits rejecting_limits =
          projection_limits;
      rejecting_limits.maximum_point_id_reference_count =
          segment.emitted_point_id_reference_count - 1U;
      require_throws<std::invalid_argument>(
          [&] {
            static_cast<void>(
                project_exact_sparse_direct_h0_higher_candidate_run(
                    std::move(drained), rejecting_limits));
          },
          "the consuming projection accepted an undersized reference cap");
      const auto retry_checkpoint =
          drained_higher.trusted_checkpoint();
      require_throws<std::logic_error>(
          [&] {
            static_cast<void>(
                drained_higher.drain_next_unsealed_segment());
          },
          "the producer skipped a rejected outstanding projection token");
      require(
          drained.segment_available() && !drained.consumed() &&
              drained_higher.unconsumed_segment_outstanding() &&
              drained_higher.trusted_checkpoint() == retry_checkpoint &&
              drained.segment()->events.size() == source_candidate_count &&
              drained.segment()
                      ->relevant_extra_shell_diagnostics.size() ==
                  source_diagnostic_count,
          "a rejected precommit projection consumed or changed its retry token");
      observed_retry_after_precommit_rejection = true;
    }
    ExactSparseDirectH0CandidateRun run =
        project_exact_sparse_direct_h0_higher_candidate_run(
            std::move(drained), projection_limits);
    require(
        drained.consumed() && !drained.segment_available() &&
            drained.segment() == nullptr && drained.manifest() == nullptr &&
            !drained_higher.unconsumed_segment_outstanding() &&
            run.candidates.size() == source_candidate_count &&
            run.diagnostics.size() == source_diagnostic_count &&
            drained_higher.resident_segment_count() == 0U,
        "the rvalue higher projection did not consume exactly one drained segment");
    observed_consumed_payload = observed_consumed_payload ||
        source_candidate_count != 0U || source_diagnostic_count != 0U;
    runs.push_back(std::move(run));
  }
  const auto& drained_audit =
      drained_higher.trusted_checkpoint().cumulative_audit;
  require(
      observed_consumed_payload &&
          observed_retry_after_precommit_rejection &&
          drained_higher.unsealed_segment_drain_performed() &&
          drained_higher.released_segment_count() ==
              resident_higher_runs.size() &&
          drained_higher.chunk_count() == resident_higher_runs.size() &&
          drained_event_count == drained_audit.accepted_event_count &&
          drained_diagnostic_count ==
              drained_audit.relevant_extra_shell_diagnostic_count &&
          drained_prune_count ==
              drained_audit.emitted_prune_certificate_count &&
          drained_rank_receipt_count ==
              drained_audit.emitted_rank_receipt_count &&
          drained_record_count == drained_audit.emitted_record_count &&
          drained_reference_count ==
              drained_audit.emitted_point_id_reference_count &&
          runs.size() == resident_higher_runs.size() + 1U &&
          runs.size() > 2U,
      "the higher fixture was not consumed as a bounded segment sequence");
  for (std::size_t segment_index = 0U;
       segment_index < resident_higher_runs.size(); ++segment_index) {
    require(
        runs[segment_index + 1U] ==
            resident_higher_runs[segment_index],
        "the consuming higher projection differs from the resident authority projection");
  }
  require_throws<std::logic_error>(
      [&] {
        static_cast<void>(std::move(drained_higher).seal());
      },
      "a consumed higher segment sequence minted a resident authority");

  const std::vector<ExactSparseDirectH0CandidateRun> original_runs = runs;
  ExactSparseDirectH0CandidateMergeSession session{
      std::move(runs),
      ExactSparseDirectH0MergeLimits{64U, 1U}};
  require(
      session.audit().input_validation_pass_count == 1U &&
          session.audit().validated_candidate_count == 11U &&
          session.audit().validated_diagnostic_count == 0U,
      "the merge session did not validate its complete inputs exactly once");
  const std::vector<ExactSparseDirectH0Candidate> merged = drain(session);
  require(
      merged.size() == facade.events.size(),
      "the paginated merge changed the terminal event count");
  require(
      std::is_sorted(
          merged.begin(), merged.end(),
          exact_sparse_direct_h0_candidate_less),
      "the concatenated one-record pages are not canonically sorted");

  std::array<std::size_t, 5U> support_counts{};
  for (std::size_t index_in_output = 0U;
       index_in_output < merged.size(); ++index_in_output) {
    require(
        candidate_matches_facade_event(
            merged[index_in_output], facade.events[index_in_output]),
        "the paginated candidate differs from the resident facade event");
    ++support_counts[merged[index_in_output].support_size];
    if (merged[index_in_output].support_size == 2U) {
      require(
          std::holds_alternative<ExactSparseDirectH0PairSourceLocator>(
              merged[index_in_output].source_locator),
          "a pair candidate lost its typed source locator");
    } else {
      require(
          std::holds_alternative<ExactSparseDirectH0HigherSourceLocator>(
              merged[index_in_output].source_locator),
          "a higher candidate lost its typed source locator");
    }
  }
  require(
      support_counts[2U] == 6U && support_counts[3U] == 4U &&
          support_counts[4U] == 1U,
      "the merge changed the support-cardinality lanes");
  const ExactSparseDirectH0MergeCursor completed_cursor = session.cursor();
  const ExactSparseDirectH0CandidateMergeAudit completed_audit =
      session.audit();
  require_throws<std::logic_error>(
      [&] { static_cast<void>(session.merge_next_page()); },
      "a complete merge session advanced again");
  require(
      session.cursor() == completed_cursor &&
          session.audit() == completed_audit,
      "an advance after completion mutated the session");

  std::vector<ExactSparseDirectH0CandidateRun> permuted_runs = original_runs;
  std::reverse(permuted_runs.begin(), permuted_runs.end());
  ExactSparseDirectH0CandidateMergeSession permuted_session{
      std::move(permuted_runs),
      ExactSparseDirectH0MergeLimits{64U, 1U}};
  require(
      drain(permuted_session) == merged,
      "permuting input runs changed the canonical merged stream");

  require_throws<std::invalid_argument>(
      [&] {
        std::vector<ExactSparseDirectH0CandidateRun> invalid_runs =
            original_runs;
        ExactSparseDirectH0CandidateMergeSession invalid{
            std::move(invalid_runs),
            ExactSparseDirectH0MergeLimits{0U, 1U}};
        static_cast<void>(invalid);
      },
      "the merge accepted a zero input-run cap");
  require_throws<std::invalid_argument>(
      [&] {
        std::vector<ExactSparseDirectH0CandidateRun> invalid_runs =
            original_runs;
        ExactSparseDirectH0CandidateMergeSession invalid{
            std::move(invalid_runs),
            ExactSparseDirectH0MergeLimits{64U, 0U}};
        static_cast<void>(invalid);
      },
      "the merge accepted a zero page cap");

  std::vector<ExactSparseDirectH0CandidateRun> old_schema = original_runs;
  old_schema.front().schema_version = 1U;
  require_throws<std::invalid_argument>(
      [&] {
        ExactSparseDirectH0CandidateMergeSession invalid{
            std::move(old_schema),
            ExactSparseDirectH0MergeLimits{64U, 1U}};
        static_cast<void>(invalid);
      },
      "the schema-v2 merge accepted one legacy schema-v1 run");

  std::vector<ExactSparseDirectH0CandidateRun> unsorted = original_runs;
  std::reverse(
      unsorted.front().candidates.begin(),
      unsorted.front().candidates.end());
  require_throws<std::invalid_argument>(
      [&] {
        ExactSparseDirectH0CandidateMergeSession invalid{
            std::move(unsorted),
            ExactSparseDirectH0MergeLimits{64U, 1U}};
        static_cast<void>(invalid);
      },
      "the session accepted an unsorted input run");

  std::vector<ExactSparseDirectH0CandidateRun> mismatched = original_runs;
  ++mismatched.back().effective_maximum_order;
  require_throws<std::invalid_argument>(
      [&] {
        ExactSparseDirectH0CandidateMergeSession invalid{
            std::move(mismatched),
            ExactSparseDirectH0MergeLimits{64U, 1U}};
        static_cast<void>(invalid);
      },
      "the session accepted runs with distinct order contracts");

  require_throws<std::invalid_argument>(
      [&] {
        std::vector<ExactSparseDirectH0CandidateRun> empty;
        ExactSparseDirectH0CandidateMergeSession invalid{
            std::move(empty),
            ExactSparseDirectH0MergeLimits{1U, 1U}};
        static_cast<void>(invalid);
      },
      "the session accepted an empty input-run set");

  require_throws<std::invalid_argument>(
      [&] {
        std::vector<ExactSparseDirectH0CandidateRun> invalid_runs =
            original_runs;
        ExactSparseDirectH0CandidateMergeSession invalid{
            std::move(invalid_runs),
            ExactSparseDirectH0MergeLimits{
                original_runs.size() - 1U, 1U}};
        static_cast<void>(invalid);
      },
      "the session accepted a fan-in above its cap");

  ExactSparseDirectH0CandidateRun duplicate_left = original_runs.front();
  duplicate_left.candidates.resize(1U);
  duplicate_left.diagnostics.clear();
  duplicate_left.source_point_id_reference_count =
      duplicate_left.candidates.front().support_size +
      duplicate_left.candidates.front().interior_ids.size();
  ExactSparseDirectH0CandidateRun duplicate_right = duplicate_left;
  ExactSparseDirectH0CandidateMergeSession duplicate_session{
      std::vector<ExactSparseDirectH0CandidateRun>{
          std::move(duplicate_left), std::move(duplicate_right)},
      ExactSparseDirectH0MergeLimits{2U, 1U}};
  const ExactSparseDirectH0MergedPage first_duplicate_page =
      duplicate_session.merge_next_page();
  require(
      first_duplicate_page.candidates.size() == 1U &&
          !first_duplicate_page.merge_complete,
      "the duplicate fixture did not cross a page boundary");
  require_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(duplicate_session.merge_next_page());
      },
      "the session accepted a duplicate on the next page");
  require(
      duplicate_session.poisoned() && !duplicate_session.complete() &&
          duplicate_session.cursor() ==
              first_duplicate_page.successor_cursor &&
          duplicate_session.audit().emitted_page_count == 1U,
      "a rejected duplicate did not leave a poisoned transactional session");
}

void test_pair_diagnostic_stays_outside_merge_pages() {
  constexpr std::size_t requested_maximum_order = 2U;
  const CanonicalPointCloud cloud = right_triangle();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportStreamResult pair =
      build_exact_pair_support_stream(
          index,
          cloud,
          requested_maximum_order,
          unlimited_pair_budget());
  const ExactSparseAnchoredPairH0Run pair_h0 = make_pair_h0_run(
      pair, cloud.size(), requested_maximum_order);
  ExactSparseDirectH0CandidateRun run =
      project_exact_sparse_direct_h0_pair_candidate_run(
          pair_h0,
          ExactSparseDirectH0CandidateRunLimits{64U, 64U, 1024U});
  require(
      run.diagnostics.size() == 1U,
      "the right-triangle fixture lost its pair extra-shell diagnostic");
  const std::size_t expected_candidate_count = run.candidates.size();
  std::vector<ExactSparseDirectH0CandidateRun> runs;
  runs.push_back(std::move(run));
  ExactSparseDirectH0CandidateMergeSession session{
      std::move(runs),
      ExactSparseDirectH0MergeLimits{1U, 1U}};
  require(
      session.audit().validated_diagnostic_count == 1U &&
          session.audit().validated_candidate_count ==
              expected_candidate_count,
      "the session audit did not separate candidates and diagnostics");
  std::size_t emitted_candidate_count = 0U;
  while (!session.complete()) {
    const ExactSparseDirectH0MergedPage page = session.merge_next_page();
    emitted_candidate_count += page.candidates.size();
  }
  require(
      emitted_candidate_count == expected_candidate_count &&
          session.audit().validated_diagnostic_count == 1U,
      "a preserved diagnostic entered or disappeared from merge pages");
}

void test_drained_higher_extra_shell_diagnostic() {
  constexpr std::size_t requested_maximum_order = 10U;
  const CanonicalPointCloud cloud = five_site_shell();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactSparseDirectH0CandidateRunLimits projection_limits{
      128U, 128U, 2048U};
  ExactHigherSupportTerminalAuthority resident = build_higher_authority(
      index, cloud, requested_maximum_order);
  ExactHigherSupportTerminalSession drained{
      index,
      cloud,
      requested_maximum_order,
      segmented_higher_budget(),
      256U};

  std::size_t segment_index = 0U;
  std::size_t diagnostic_count = 0U;
  bool observed_five_point_shell = false;
  while (!drained.trusted_checkpoint().locally_complete()) {
    auto source = drained.drain_next_unsealed_segment();
    require(
        source.status() ==
                ExactHigherSupportUnsealedDrainStatus::segment_ready &&
            source.segment_available(),
        "the extra-shell producer did not return one ready segment");
    const ExactHigherSupportTerminalSegment& segment = *source.segment();
    diagnostic_count +=
        segment.relevant_extra_shell_diagnostics.size();
    for (const auto& diagnostic :
         segment.relevant_extra_shell_diagnostics) {
      observed_five_point_shell = observed_five_point_shell ||
          (diagnostic.support_size == 4U &&
           diagnostic.interior_ids.empty() &&
           diagnostic.shell_count == 5U &&
           diagnostic.observed_closed_rank == 5U);
    }
    const auto expected =
        project_exact_sparse_direct_h0_higher_candidate_run(
            resident, segment_index, projection_limits);
    const auto consumed =
        project_exact_sparse_direct_h0_higher_candidate_run(
            std::move(source), projection_limits);
    require(
        consumed == expected && source.consumed(),
        "the consuming higher projection changed an extra-shell diagnostic");
    ++segment_index;
  }
  require(
      segment_index == resident.segments().size() &&
          diagnostic_count != 0U && observed_five_point_shell,
      "the consuming higher fixture did not cover its five-site diagnostic");
}

void test_higher_source_provenance_and_chunk_cap() {
  constexpr std::size_t requested_maximum_order = 10U;
  const ExactSparseDirectH0CandidateRunLimits projection_limits{
      128U, 128U, 2048U};
  const CanonicalPointCloud first_cloud = regular_tetrahedron();
  const CanonicalPointCloud second_cloud = shifted_tetrahedron();
  const MortonLbvhIndex first_index =
      MortonLbvhIndex::build(first_cloud);
  const MortonLbvhIndex second_index =
      MortonLbvhIndex::build(second_cloud);
  ExactHigherSupportTerminalAuthority first_authority =
      build_higher_authority(
          first_index, first_cloud, requested_maximum_order);
  ExactHigherSupportTerminalAuthority second_authority =
      build_higher_authority(
          second_index, second_cloud, requested_maximum_order);
  require(
      !first_authority.segments().empty() &&
          !second_authority.segments().empty(),
      "the provenance fixtures emitted no bounded higher run");
  ExactSparseDirectH0CandidateRun first_run =
      project_exact_sparse_direct_h0_higher_candidate_run(
          first_authority, 0U, projection_limits);
  ExactSparseDirectH0CandidateRun second_run =
      project_exact_sparse_direct_h0_higher_candidate_run(
          second_authority, 0U, projection_limits);
  require(
      first_run.higher_source_contract.has_value() &&
          second_run.higher_source_contract.has_value() &&
          first_run.higher_source_contract->manifest !=
              second_run.higher_source_contract->manifest,
      "distinct same-size clouds did not retain distinct higher provenance");
  require_throws<std::invalid_argument>(
      [&] {
        ExactSparseDirectH0CandidateMergeSession invalid{
            std::vector<ExactSparseDirectH0CandidateRun>{
                std::move(first_run), std::move(second_run)},
            ExactSparseDirectH0MergeLimits{2U, 16U}};
        static_cast<void>(invalid);
      },
      "the merge accepted higher runs from distinct source manifests");

  ExactHigherSupportTerminalSession capped{
      first_index,
      first_cloud,
      requested_maximum_order,
      segmented_higher_budget(),
      1U};
  auto first = capped.drain_next_unsealed_segment();
  require(
      first.status() ==
              ExactHigherSupportUnsealedDrainStatus::segment_ready &&
          first.segment_available() &&
          !capped.trusted_checkpoint().locally_complete(),
      "the one-chunk projection fixture unexpectedly reached terminality");
  static_cast<void>(
      project_exact_sparse_direct_h0_higher_candidate_run(
          std::move(first), projection_limits));
  const auto capped_checkpoint = capped.trusted_checkpoint();
  auto cap = capped.drain_next_unsealed_segment();
  require(
      cap.status() == ExactHigherSupportUnsealedDrainStatus::
              maximum_chunk_count_reached &&
          !cap.segment_available() &&
          capped.trusted_checkpoint() == capped_checkpoint &&
          capped.chunk_count() == 1U &&
          capped.released_segment_count() == 1U,
      "the consumed one-chunk session did not report its stable cap");
}

void test_drained_higher_projection_with_destroyed_prunes() {
  constexpr std::size_t requested_maximum_order = 3U;
  const CanonicalPointCloud cloud = rank_prune_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  ExactHigherSupportStreamBudget budget = unlimited_higher_budget();
  budget.maximum_work_unit_count = 4U;
  budget.maximum_emitted_record_count = 1U;
  budget.maximum_emitted_point_id_reference_count = 64U;
  budget.maximum_prune_receipt_count = 16U;
  budget.maximum_global_closed_ball_query_count = 16U;
  budget.maximum_point_classification_count = 128U;
  constexpr std::size_t maximum_chunk_count = 4096U;
  const ExactSparseDirectH0CandidateRunLimits projection_limits{
      64U, 64U, 1024U};

  ExactHigherSupportTerminalSession resident_session{
      index,
      cloud,
      requested_maximum_order,
      budget,
      maximum_chunk_count};
  require(
      resident_session.run_to_terminal() ==
          ExactHigherSupportTerminalRunStatus::terminal,
      "the rank-prune resident fixture did not reach terminality");
  ExactHigherSupportTerminalAuthority resident =
      std::move(resident_session).seal();
  require(
      resident.destroyed_prune_certificate_count() != 0U &&
          resident.destroyed_rank_receipt_count() != 0U,
      "the rank-prune fixture emitted no destroyed certificate payload");

  ExactHigherSupportTerminalSession drained{
      index,
      cloud,
      requested_maximum_order,
      budget,
      maximum_chunk_count};
  std::size_t segment_index = 0U;
  std::size_t destroyed_prune_count = 0U;
  std::size_t destroyed_rank_receipt_count = 0U;
  while (!drained.trusted_checkpoint().locally_complete()) {
    auto drained_segment = drained.drain_next_unsealed_segment();
    require(
        drained_segment.status() ==
                ExactHigherSupportUnsealedDrainStatus::segment_ready &&
            drained_segment.segment_available(),
        "the rank-prune producer did not return one ready segment");
    const ExactHigherSupportTerminalSegment& segment =
        *drained_segment.segment();
    destroyed_prune_count += segment.destroyed_prune_certificate_count;
    destroyed_rank_receipt_count += segment.destroyed_rank_receipt_count;
    const auto expected =
        project_exact_sparse_direct_h0_higher_candidate_run(
            resident, segment_index, projection_limits);
    const auto consumed =
        project_exact_sparse_direct_h0_higher_candidate_run(
            std::move(drained_segment), projection_limits);
    require(
        consumed == expected && drained_segment.consumed() &&
            !drained_segment.segment_available() &&
            drained.resident_segment_count() == 0U,
        "a consumed rank-prune segment differs from its resident projection");
    ++segment_index;
  }

  require(
      segment_index == resident.segments().size() &&
          destroyed_prune_count ==
              resident.destroyed_prune_certificate_count() &&
          destroyed_rank_receipt_count ==
              resident.destroyed_rank_receipt_count() &&
          drained.released_segment_count() == segment_index &&
          drained.trusted_checkpoint().locally_complete(),
      "draining lost destroyed prune or rank-receipt accounting");
}

}  // namespace

int main() {
  try {
    test_cross_arity_projection_and_session_merge();
    test_pair_diagnostic_stays_outside_merge_pages();
    test_drained_higher_extra_shell_diagnostic();
    test_higher_source_provenance_and_chunk_cap();
    test_drained_higher_projection_with_destroyed_prunes();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "sparse direct H0 candidate merge tests passed\n";
  return 0;
}
