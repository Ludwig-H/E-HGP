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

[[nodiscard]] CanonicalPointCloud right_triangle() {
  const std::array<CertifiedPoint3, 3U> points{
      point(0.0, 0.0, 0.0),
      point(2.0, 0.0, 0.0),
      point(0.0, 2.0, 0.0)};
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
  for (std::size_t segment_index = 0U;
       segment_index < authority.segments().size(); ++segment_index) {
    runs.push_back(project_exact_sparse_direct_h0_higher_candidate_run(
        authority, segment_index, projection_limits));
  }
  require(runs.size() > 2U, "the higher fixture was not segmented");

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

}  // namespace

int main() {
  try {
    test_cross_arity_projection_and_session_merge();
    test_pair_diagnostic_stays_outside_merge_pages();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "sparse direct H0 candidate merge tests passed\n";
  return 0;
}
