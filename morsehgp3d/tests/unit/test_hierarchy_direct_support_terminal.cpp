#include "morsehgp3d/hierarchy/direct_support_terminal.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::hierarchy::ExactDirectSupportTerminalBudget;
using morsehgp3d::hierarchy::ExactDirectSupportTerminalDecision;
using morsehgp3d::hierarchy::ExactDirectSupportHigherSourceKind;
using morsehgp3d::hierarchy::ExactDirectSupportPairSourceKind;
using morsehgp3d::hierarchy::ExactHigherSupportAnchoredSession;
using morsehgp3d::hierarchy::ExactHigherSupportAuthorityContext;
using morsehgp3d::hierarchy::ExactHigherSupportStreamBudget;
using morsehgp3d::hierarchy::ExactHigherSupportTerminalRunStatus;
using morsehgp3d::hierarchy::ExactHigherSupportTerminalSession;
using morsehgp3d::hierarchy::ExactPairSupportAuthorityContext;
using morsehgp3d::hierarchy::ExactPairSupportIncrementalVerifier;
using morsehgp3d::hierarchy::ExactPairSupportStreamBudget;
using morsehgp3d::hierarchy::ExactMortonGroupedAnchoredPairScheduleConfig;
using morsehgp3d::hierarchy::ExactSparseAnchoredPairSession;
using morsehgp3d::hierarchy::ExactSparseAnchoredPairSessionAdvanceBudget;
using morsehgp3d::hierarchy::ExactSparseAnchoredPairSessionStepKind;
using morsehgp3d::hierarchy::ExactSparseAnchoredPairSessionTotalCapacity;
using morsehgp3d::hierarchy::ExactSparseAnchoredPairTerminalAuthority;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
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
  return ExactPairSupportStreamBudget{
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

[[nodiscard]] ExactHigherSupportStreamBudget unlimited_higher_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return ExactHigherSupportStreamBudget{
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

[[nodiscard]] ExactHigherSupportStreamBudget small_higher_chunk_budget() {
  ExactHigherSupportStreamBudget budget = unlimited_higher_budget();
  budget.maximum_work_unit_count = 1U;
  budget.maximum_emitted_record_count = 8U;
  budget.maximum_emitted_point_id_reference_count = 64U;
  budget.maximum_prune_receipt_count = 8U;
  budget.maximum_global_closed_ball_query_count = 8U;
  budget.maximum_point_classification_count = 64U;
  return budget;
}

[[nodiscard]] ExactSparseAnchoredPairSessionTotalCapacity
unlimited_sparse_pair_capacity() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return ExactSparseAnchoredPairSessionTotalCapacity{
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

[[nodiscard]] ExactSparseAnchoredPairSessionAdvanceBudget
sparse_pair_advance_budget(
    std::size_t maximum_closed_rank,
    bool segmented) {
  const std::size_t work = segmented ? 1U : 4096U;
  return ExactSparseAnchoredPairSessionAdvanceBudget{
      {work, work, work, work},
      {work},
      1U,
      maximum_closed_rank + 1U};
}

[[nodiscard]] ExactSparseAnchoredPairTerminalAuthority
build_sparse_pair_authority(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank,
    bool segmented = false) {
  ExactSparseAnchoredPairSession session =
      ExactSparseAnchoredPairSession::start(
          index,
          cloud,
          maximum_closed_rank,
          ExactMortonGroupedAnchoredPairScheduleConfig{4U, 0U},
          unlimited_sparse_pair_capacity());
  const ExactSparseAnchoredPairSessionAdvanceBudget budget =
      sparse_pair_advance_budget(maximum_closed_rank, segmented);
  for (std::size_t call = 0U; call < 100000U; ++call) {
    const auto step = session.advance(index, cloud, budget);
    if (step.kind() ==
        ExactSparseAnchoredPairSessionStepKind::total_capacity_exhausted) {
      throw std::logic_error(
          "a roomy sparse pair authority exhausted total capacity");
    }
    if (step.kind() == ExactSparseAnchoredPairSessionStepKind::complete) {
      return std::move(session).seal();
    }
  }
  throw std::logic_error("the sparse pair authority did not terminate");
}

[[nodiscard]] morsehgp3d::hierarchy::ExactHigherSupportTerminalAuthority
build_higher_authority(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    ExactHigherSupportStreamBudget budget) {
  ExactHigherSupportTerminalSession session{
      index, cloud, requested_maximum_order, budget, 256U};
  if (session.run_to_terminal() !=
      ExactHigherSupportTerminalRunStatus::terminal) {
    throw std::logic_error("the higher terminal authority did not close");
  }
  return std::move(session).seal();
}

void test_terminal_facade_and_fresh_composition() {
  const CanonicalPointCloud cloud = regular_tetrahedron();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSupportTerminalBudget budget{
      unlimited_pair_budget(), unlimited_higher_budget()};
  const auto pair =
      morsehgp3d::hierarchy::build_exact_pair_support_stream(
          index, cloud, 10U, budget.pair);
  const auto higher =
      morsehgp3d::hierarchy::build_exact_higher_support_stream(
          index, cloud, 10U, budget.higher);
  const auto facade =
      morsehgp3d::hierarchy::build_exact_direct_support_terminal_facade(
          index, cloud, 10U, budget, pair, higher);
  const auto verification =
      morsehgp3d::hierarchy::verify_exact_direct_support_terminal_facade(
          index, cloud, 10U, budget, pair, higher, facade);

  check(
      facade.terminal_catalog_certified() && verification.result_certified &&
          facade.certificate.decision ==
              ExactDirectSupportTerminalDecision::
                  complete_direct_support_catalog &&
          facade.events.size() == 11U &&
          facade.relevant_extra_shell_diagnostics.empty() &&
          facade.certificate.arity_certificates[0]
                  .accepted_event_count == 6U &&
          facade.certificate.arity_certificates[1]
                  .accepted_event_count == 4U &&
          facade.certificate.arity_certificates[2]
                  .accepted_event_count == 1U &&
          facade.certificate.higher_source_kind ==
              ExactDirectSupportHigherSourceKind::
                  fresh_resident_replay &&
          !facade.certificate.common_durable_checkpoint_certified &&
          !facade.certificate.hierarchy_or_forest_certified &&
          !facade.certificate.public_status_claimed,
      "one fresh terminal certificate closes arities two through four without claiming durability, forest semantics or public status");

  auto mutated = facade;
  ++mutated.certificate.normalized_event_count;
  check(
      !morsehgp3d::hierarchy::verify_exact_direct_support_terminal_facade(
           index, cloud, 10U, budget, pair, higher, mutated)
           .result_certified,
      "a mutated common terminal count fails the fresh composition replay");
}

void test_sealed_higher_authority_avoids_fresh_higher_replay() {
  const CanonicalPointCloud cloud = regular_tetrahedron();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  ExactHigherSupportStreamBudget higher_budget =
      unlimited_higher_budget();
  higher_budget.maximum_work_unit_count = 1U;
  higher_budget.maximum_emitted_record_count = 8U;
  higher_budget.maximum_emitted_point_id_reference_count = 64U;
  higher_budget.maximum_prune_receipt_count = 8U;
  higher_budget.maximum_global_closed_ball_query_count = 8U;
  higher_budget.maximum_point_classification_count = 64U;
  const ExactDirectSupportTerminalBudget budget{
      unlimited_pair_budget(), higher_budget};
  const auto pair =
      morsehgp3d::hierarchy::build_exact_pair_support_stream(
          index, cloud, 10U, budget.pair);
  const auto legacy_higher =
      morsehgp3d::hierarchy::build_exact_higher_support_stream(
          index, cloud, 10U, unlimited_higher_budget());
  const ExactDirectSupportTerminalBudget legacy_budget{
      budget.pair, unlimited_higher_budget()};
  const auto legacy_facade =
      morsehgp3d::hierarchy::build_exact_direct_support_terminal_facade(
          index,
          cloud,
          10U,
          legacy_budget,
          pair,
          legacy_higher);

  ExactHigherSupportTerminalSession session{
      index, cloud, 10U, budget.higher, 256U};
  check(
      session.run_to_terminal() ==
          ExactHigherSupportTerminalRunStatus::terminal,
      "the fixed-budget higher-support producer reaches its terminal root");
  const std::size_t produced_chunk_count = session.chunk_count();
  auto authority = std::move(session).seal();
  const auto facade =
      morsehgp3d::hierarchy::build_exact_direct_support_terminal_facade(
          index,
          cloud,
          10U,
          budget,
          pair,
          std::move(authority));

  check(
      facade.terminal_catalog_certified() &&
          facade.events == legacy_facade.events &&
          facade.relevant_extra_shell_diagnostics ==
              legacy_facade.relevant_extra_shell_diagnostics &&
          facade.certificate.higher_source_kind ==
              ExactDirectSupportHigherSourceKind::
                  sealed_anchored_fixed_chunk_run &&
          !facade.certificate.higher_result_freshly_replayed &&
          facade.certificate.higher_root_anchored_run_certified &&
          facade.certificate.higher_terminal_authority_consumed &&
          facade.certificate.higher_terminal_records_captured_once &&
          facade.certificate.higher_anchored_chunk_count ==
              produced_chunk_count &&
          facade.certificate.higher_maximum_chunk_count == 256U,
      "the sealed higher authority yields the same terminal catalogue without claiming a fresh higher replay");

  auto erased_sealed_anchor = facade;
  erased_sealed_anchor.certificate.higher_output_chain_digest = {};
  check(
      !erased_sealed_anchor.terminal_catalog_certified(),
      "a sealed facade rejects an erased higher output-chain anchor");
  auto polluted_fresh_provenance = legacy_facade;
  polluted_fresh_provenance.certificate.higher_output_chain_digest =
      facade.certificate.higher_output_chain_digest;
  check(
      !polluted_fresh_provenance.terminal_catalog_certified(),
      "a fresh facade rejects sealed-only higher provenance");
}

void test_sparse_pair_authority_avoids_p7b_replay() {
  const CanonicalPointCloud cloud = regular_tetrahedron();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  constexpr std::size_t requested_maximum_order = 10U;
  constexpr std::size_t maximum_closed_rank = 4U;
  const ExactHigherSupportStreamBudget higher_budget =
      small_higher_chunk_budget();
  const ExactHigherSupportStreamBudget legacy_higher_budget =
      unlimited_higher_budget();
  const ExactDirectSupportTerminalBudget legacy_budget{
      unlimited_pair_budget(), legacy_higher_budget};
  const auto legacy_pair =
      morsehgp3d::hierarchy::build_exact_pair_support_stream(
          index,
          cloud,
          requested_maximum_order,
          legacy_budget.pair);
  const auto legacy_higher =
      morsehgp3d::hierarchy::build_exact_higher_support_stream(
          index,
          cloud,
          requested_maximum_order,
          legacy_higher_budget);
  const auto legacy_facade =
      morsehgp3d::hierarchy::build_exact_direct_support_terminal_facade(
          index,
          cloud,
          requested_maximum_order,
          legacy_budget,
          legacy_pair,
          legacy_higher);

  ExactSparseAnchoredPairTerminalAuthority original_pair_authority =
      build_sparse_pair_authority(
          index, cloud, maximum_closed_rank);
  ExactSparseAnchoredPairTerminalAuthority transferred_pair_authority(
      std::move(original_pair_authority));
  check(
      !original_pair_authority.sealed_in_process_terminal_authority() &&
          transferred_pair_authority
              .sealed_in_process_terminal_authority(),
      "moving a sparse pair authority revokes exactly its source scope");
  auto higher_authority = build_higher_authority(
      index, cloud, requested_maximum_order, higher_budget);
  const auto sparse_facade =
      morsehgp3d::hierarchy::build_exact_direct_support_terminal_facade(
          index,
          cloud,
          requested_maximum_order,
          higher_budget,
          std::move(transferred_pair_authority),
          std::move(higher_authority));

  check(
      sparse_facade.terminal_catalog_certified() &&
          sparse_facade.events == legacy_facade.events &&
          sparse_facade.relevant_extra_shell_diagnostics ==
              legacy_facade.relevant_extra_shell_diagnostics &&
          sparse_facade.certificate.pair_source_kind ==
              ExactDirectSupportPairSourceKind::
                  sealed_sparse_anchored_session &&
          !sparse_facade.certificate.pair_legacy_budget_applicable &&
          !sparse_facade.certificate.pair_result_freshly_replayed &&
          sparse_facade.certificate.pair_terminal_authority_consumed &&
          sparse_facade.certificate
              .pair_terminal_records_captured_once &&
          sparse_facade.certificate.pair_directed_pair_universe_size ==
              16U &&
          sparse_facade.certificate.arity_certificates[0]
                  .exact_candidate_universe_size == 6 &&
          sparse_facade.certificate.arity_certificates[0]
                  .accepted_event_count == 6U &&
          sparse_facade.certificate.pair_terminal_output_digest !=
              morsehgp3d::contract::CanonicalId{} &&
          sparse_facade.certificate.pair_semantic_digest !=
              legacy_facade.certificate.pair_semantic_digest &&
          sparse_facade.certificate.requested_budget.pair ==
              ExactPairSupportStreamBudget{} &&
          !transferred_pair_authority
               .sealed_in_process_terminal_authority(),
      "the sealed sparse pair authority reproduces the legacy catalogue without replaying or relabelling P7b");

  auto segmented_pair_authority = build_sparse_pair_authority(
      index, cloud, maximum_closed_rank, true);
  auto segmented_higher_authority = build_higher_authority(
      index, cloud, requested_maximum_order, higher_budget);
  const auto segmented_facade =
      morsehgp3d::hierarchy::build_exact_direct_support_terminal_facade(
          index,
          cloud,
          requested_maximum_order,
          higher_budget,
          std::move(segmented_pair_authority),
          std::move(segmented_higher_authority));
  check(
      segmented_facade.terminal_catalog_certified() &&
          segmented_facade.events == sparse_facade.events &&
          segmented_facade.relevant_extra_shell_diagnostics ==
              sparse_facade.relevant_extra_shell_diagnostics &&
          segmented_facade.certificate.pair_terminal_output_digest ==
              sparse_facade.certificate.pair_terminal_output_digest &&
          segmented_facade.certificate.pair_semantic_digest ==
              sparse_facade.certificate.pair_semantic_digest,
      "different P8l segmentation preserves the normalized pair science and its digests");

  const CanonicalPointCloud foreign_cloud = regular_tetrahedron();
  const MortonLbvhIndex foreign_index = MortonLbvhIndex::build(foreign_cloud);
  auto foreign_pair_authority = build_sparse_pair_authority(
      foreign_index, foreign_cloud, maximum_closed_rank);
  auto target_higher_authority = build_higher_authority(
      index, cloud, requested_maximum_order, higher_budget);
  const auto foreign_rejected =
      morsehgp3d::hierarchy::build_exact_direct_support_terminal_facade(
          index,
          cloud,
          requested_maximum_order,
          higher_budget,
          std::move(foreign_pair_authority),
          std::move(target_higher_authority));
  check(
      !foreign_rejected.terminal_catalog_certified() &&
          !foreign_rejected.certificate.source_authorities_match &&
          foreign_rejected.events.empty() &&
          foreign_rejected.relevant_extra_shell_diagnostics.empty() &&
          foreign_rejected.certificate.decision ==
              ExactDirectSupportTerminalDecision::
                  source_result_not_certified,
      "a sparse pair authority with equal coordinates but foreign tokens fails closed");

  auto revoked_pair_authority = build_sparse_pair_authority(
      index, cloud, maximum_closed_rank);
  static_cast<void>(
      std::move(revoked_pair_authority).release_records());
  auto fail_closed_higher_authority = build_higher_authority(
      index, cloud, requested_maximum_order, higher_budget);
  const auto rejected =
      morsehgp3d::hierarchy::build_exact_direct_support_terminal_facade(
          index,
          cloud,
          requested_maximum_order,
          higher_budget,
          std::move(revoked_pair_authority),
          std::move(fail_closed_higher_authority));
  check(
      !rejected.terminal_catalog_certified() && rejected.events.empty() &&
          rejected.relevant_extra_shell_diagnostics.empty() &&
          rejected.certificate.decision ==
              ExactDirectSupportTerminalDecision::
                  source_result_not_certified,
      "a previously released sparse pair authority fails closed without payload");

  auto mutated_digest = sparse_facade;
  auto output_digest_bytes =
      mutated_digest.certificate.pair_terminal_output_digest.bytes();
  output_digest_bytes[0] ^= 1U;
  mutated_digest.certificate.pair_terminal_output_digest =
      morsehgp3d::contract::CanonicalId{output_digest_bytes};
  check(
      !mutated_digest.terminal_catalog_certified(),
      "a sparse facade rejects a nonzero mutated pair output digest");

  auto mutated_index = sparse_facade;
  ++mutated_index.events.front().event_index;
  check(
      !mutated_index.terminal_catalog_certified(),
      "a terminal facade rejects a mutated normalized event index");
  auto mutated_h0_order = sparse_facade;
  mutated_h0_order.events.front().birth_order.reset();
  check(
      !mutated_h0_order.terminal_catalog_certified(),
      "a terminal facade rejects a mutated derived H0 order");
}

void test_sparse_pair_diagnostic_normalization() {
  const CanonicalPointCloud cloud = right_triangle();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  constexpr std::size_t requested_maximum_order = 2U;
  constexpr std::size_t maximum_closed_rank = 3U;
  const ExactHigherSupportStreamBudget higher_budget =
      small_higher_chunk_budget();
  const ExactHigherSupportStreamBudget legacy_higher_budget =
      unlimited_higher_budget();
  const ExactDirectSupportTerminalBudget legacy_budget{
      unlimited_pair_budget(), legacy_higher_budget};
  const auto legacy_pair =
      morsehgp3d::hierarchy::build_exact_pair_support_stream(
          index,
          cloud,
          requested_maximum_order,
          legacy_budget.pair);
  const auto legacy_higher =
      morsehgp3d::hierarchy::build_exact_higher_support_stream(
          index,
          cloud,
          requested_maximum_order,
          legacy_higher_budget);
  const auto legacy_facade =
      morsehgp3d::hierarchy::build_exact_direct_support_terminal_facade(
          index,
          cloud,
          requested_maximum_order,
          legacy_budget,
          legacy_pair,
          legacy_higher);

  auto pair_authority = build_sparse_pair_authority(
      index, cloud, maximum_closed_rank);
  auto higher_authority = build_higher_authority(
      index, cloud, requested_maximum_order, higher_budget);
  const auto sparse_facade =
      morsehgp3d::hierarchy::build_exact_direct_support_terminal_facade(
          index,
          cloud,
          requested_maximum_order,
          higher_budget,
          std::move(pair_authority),
          std::move(higher_authority));

  check(
      legacy_facade.terminal_catalog_certified() &&
          sparse_facade.terminal_catalog_certified() &&
          !legacy_facade.relevant_extra_shell_diagnostics.empty() &&
          sparse_facade.events == legacy_facade.events &&
          sparse_facade.relevant_extra_shell_diagnostics ==
              legacy_facade.relevant_extra_shell_diagnostics &&
          sparse_facade.certificate.arity_certificates[0]
                  .relevant_extra_shell_diagnostic_count == 1U,
      "the moved P8l variant preserves the right-triangle pair extra-shell diagnostic");
  auto mutated_diagnostic_index = sparse_facade;
  ++mutated_diagnostic_index.relevant_extra_shell_diagnostics.front()
        .diagnostic_index;
  check(
      !mutated_diagnostic_index.terminal_catalog_certified(),
      "a terminal facade rejects a mutated normalized diagnostic index");
}

void test_budgeted_source_never_publishes_terminal_payload() {
  const CanonicalPointCloud cloud = regular_tetrahedron();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  ExactDirectSupportTerminalBudget budget{
      unlimited_pair_budget(), unlimited_higher_budget()};
  budget.pair.maximum_work_unit_count = 0U;
  budget.higher.maximum_work_unit_count = 0U;
  const auto pair =
      morsehgp3d::hierarchy::build_exact_pair_support_stream(
          index, cloud, 10U, budget.pair);
  const auto higher =
      morsehgp3d::hierarchy::build_exact_higher_support_stream(
          index, cloud, 10U, budget.higher);
  const auto facade =
      morsehgp3d::hierarchy::build_exact_direct_support_terminal_facade(
          index, cloud, 10U, budget, pair, higher);
  check(
      !facade.terminal_catalog_certified() && facade.events.empty() &&
          facade.relevant_extra_shell_diagnostics.empty() &&
          facade.certificate.decision ==
              ExactDirectSupportTerminalDecision::source_stream_not_terminal &&
          !facade.certificate.all_arities_terminal,
      "a budgeted residual frontier publishes no normalized terminal payload");
}

void test_one_record_chunks_exceed_resident_output_capacity() {
  const CanonicalPointCloud cloud = regular_tetrahedron();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);

  ExactPairSupportStreamBudget pair_budget = unlimited_pair_budget();
  pair_budget.maximum_emitted_record_count = 1U;
  const ExactPairSupportAuthorityContext pair_authority{index, cloud, 10U};
  ExactPairSupportIncrementalVerifier pair_verifier{pair_authority};
  std::size_t pair_transition_count = 0U;
  std::size_t maximum_pair_chunk_record_count = 0U;
  while (!pair_verifier.trusted_checkpoint().complete()) {
    if (++pair_transition_count > 1000U) {
      throw std::logic_error("the synthetic pair stream made no progress");
    }
    const auto source = pair_verifier.trusted_checkpoint();
    const auto chunk =
        morsehgp3d::hierarchy::build_exact_pair_support_stream_chunk(
            pair_authority, pair_budget, source);
    maximum_pair_chunk_record_count = std::max(
        maximum_pair_chunk_record_count, chunk.record_order.size());
    check(
        pair_verifier.verify_next(pair_budget, chunk)
            .chunk_transition_verified,
        "every one-record pair transition remains anchored");
  }

  ExactHigherSupportStreamBudget higher_budget = unlimited_higher_budget();
  higher_budget.maximum_emitted_record_count = 1U;
  const ExactHigherSupportAuthorityContext higher_authority{
      index, cloud, 10U};
  ExactHigherSupportAnchoredSession higher_session{higher_authority};
  std::size_t higher_transition_count = 0U;
  std::size_t maximum_higher_chunk_record_count = 0U;
  while (!higher_session.trusted_checkpoint().locally_complete()) {
    if (++higher_transition_count > 1000U) {
      throw std::logic_error("the synthetic higher stream made no progress");
    }
    const auto source = higher_session.trusted_checkpoint();
    const auto chunk = higher_session.prepare_next(higher_budget, source);
    maximum_higher_chunk_record_count = std::max(
        maximum_higher_chunk_record_count, chunk.record_order.size());
    check(
        higher_session.commit_prepared(higher_budget, source, chunk)
            .chunk_transition_verified,
        "every one-record higher transition remains root-anchored");
  }

  const std::size_t cumulative_output_record_count =
      pair_verifier.trusted_checkpoint().output_record_count +
      higher_session.trusted_checkpoint().output_record_count;
  check(
      pair_verifier.status().anchored_run_certified &&
          pair_verifier.status().retained_chunk_count == 0U &&
          maximum_pair_chunk_record_count <= 1U &&
          maximum_higher_chunk_record_count <= 1U &&
          cumulative_output_record_count > 1U,
      "the synthetic arity-two-through-four stream exceeds its one-record resident output capacity with zero retained pair chunk history");
}

}  // namespace

int main() {
  test_terminal_facade_and_fresh_composition();
  test_sealed_higher_authority_avoids_fresh_higher_replay();
  test_sparse_pair_authority_avoids_p7b_replay();
  test_sparse_pair_diagnostic_normalization();
  test_budgeted_source_never_publishes_terminal_payload();
  test_one_record_chunks_exceed_resident_output_capacity();
  if (failures != 0) {
    std::cerr << failures << " direct terminal checks failed\n";
    return 1;
  }
  std::cout << "all direct support terminal checks passed\n";
  return 0;
}
