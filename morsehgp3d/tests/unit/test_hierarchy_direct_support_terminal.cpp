#include "morsehgp3d/hierarchy/direct_support_terminal.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::hierarchy::ExactDirectSupportTerminalBudget;
using morsehgp3d::hierarchy::ExactDirectSupportTerminalDecision;
using morsehgp3d::hierarchy::ExactHigherSupportAnchoredSession;
using morsehgp3d::hierarchy::ExactHigherSupportAuthorityContext;
using morsehgp3d::hierarchy::ExactHigherSupportStreamBudget;
using morsehgp3d::hierarchy::ExactPairSupportAuthorityContext;
using morsehgp3d::hierarchy::ExactPairSupportIncrementalVerifier;
using morsehgp3d::hierarchy::ExactPairSupportStreamBudget;
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
  test_budgeted_source_never_publishes_terminal_payload();
  test_one_record_chunks_exceed_resident_output_capacity();
  if (failures != 0) {
    std::cerr << failures << " direct terminal checks failed\n";
    return 1;
  }
  std::cout << "all direct support terminal checks passed\n";
  return 0;
}
