#include "morsehgp3d/hierarchy/direct_morse_event_journal.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <limits>
#include <span>
#include <string>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactDirectMorseEventJournalDecision;
using morsehgp3d::hierarchy::ExactDirectMorseEventSource;
using morsehgp3d::hierarchy::ExactDirectMorseH0Role;
using morsehgp3d::hierarchy::ExactDirectSupportTerminalBudget;
using morsehgp3d::hierarchy::ExactDirectSupportTerminalFacade;
using morsehgp3d::hierarchy::ExactHigherSupportStreamBudget;
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

[[nodiscard]] CanonicalPointCloud translated_tetrahedron() {
  const std::array<CertifiedPoint3, 4U> points{
      point(3.0, 1.0, 1.0),
      point(3.0, -1.0, -1.0),
      point(1.0, 1.0, -1.0),
      point(1.0, -1.0, 1.0)};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud cocircular_square() {
  const std::array<CertifiedPoint3, 4U> points{
      point(-1.0, 0.0, 0.0),
      point(0.0, -1.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(1.0, 0.0, 0.0)};
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

[[nodiscard]] ExactDirectSupportTerminalFacade terminal_facade(
    const CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactDirectSupportTerminalBudget& budget) {
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto pair =
      morsehgp3d::hierarchy::build_exact_pair_support_stream(
          index, cloud, requested_maximum_order, budget.pair);
  const auto higher =
      morsehgp3d::hierarchy::build_exact_higher_support_stream(
          index, cloud, requested_maximum_order, budget.higher);
  return morsehgp3d::hierarchy::
      build_exact_direct_support_terminal_facade(
          index,
          cloud,
          requested_maximum_order,
          budget,
          pair,
          higher);
}

void test_exact_roles_and_batches() {
  const CanonicalPointCloud cloud = regular_tetrahedron();
  const ExactDirectSupportTerminalBudget budget{
      unlimited_pair_budget(), unlimited_higher_budget()};
  const ExactDirectSupportTerminalFacade facade =
      terminal_facade(cloud, 10U, budget);
  const auto journal =
      morsehgp3d::hierarchy::build_exact_direct_morse_event_journal(
          cloud, facade);
  const auto verification =
      morsehgp3d::hierarchy::verify_exact_direct_morse_event_journal(
          cloud, facade, journal);

  check(
      journal.certified_partial_refinement() &&
          verification.result_certified && journal.point_count == 4U &&
          journal.source_direct_event_count == 11U &&
          journal.event_projection_count == 15U &&
          journal.role_record_count == 26U &&
          journal.batch_count == 7U,
      "a regular tetrahedron yields four singleton births and all exact direct-support H0 roles in seven equality batches");

  bool singleton_prefix_is_canonical = true;
  for (std::size_t index = 0U; index < cloud.size(); ++index) {
    const auto& projection = journal.event_projections[index];
    singleton_prefix_is_canonical =
        singleton_prefix_is_canonical &&
        projection.event_projection_index == index &&
        projection.source ==
            ExactDirectMorseEventSource::canonical_singleton &&
        projection.source_index == index && projection.support_size == 1U &&
        projection.support_ids[0] == index &&
        projection.squared_level == ExactLevel{} &&
        projection.closed_rank == 1U &&
        projection.birth_order == 1U &&
        !projection.saddle_order.has_value();
  }
  check(
      singleton_prefix_is_canonical,
      "singleton births are indexed by canonical PointId at exact level zero");

  const auto order_one_zero = std::find_if(
      journal.batches.begin(),
      journal.batches.end(),
      [](const auto& batch) {
        return batch.order == 1U && batch.squared_level == ExactLevel{};
      });
  check(
      order_one_zero != journal.batches.end() &&
          order_one_zero->role_record_count == 4U &&
          order_one_zero->birth_role_count == 4U &&
          order_one_zero->saddle_role_count == 0U,
      "the order-one level-zero batch contains exactly the singleton births");

  bool batches_are_strictly_sorted = true;
  bool role_partition_is_exact = true;
  std::size_t expected_offset = 0U;
  for (std::size_t index = 0U; index < journal.batches.size(); ++index) {
    const auto& batch = journal.batches[index];
    batches_are_strictly_sorted =
        batches_are_strictly_sorted && batch.batch_index == index &&
        (index == 0U ||
         journal.batches[index - 1U].order < batch.order ||
         (journal.batches[index - 1U].order == batch.order &&
          journal.batches[index - 1U].squared_level <
              batch.squared_level));
    role_partition_is_exact =
        role_partition_is_exact &&
        batch.role_record_offset == expected_offset;
    for (std::size_t local = 0U; local < batch.role_record_count; ++local) {
      const std::size_t role_index = batch.role_record_offset + local;
      const auto& role = journal.role_records[role_index];
      role_partition_is_exact =
          role_partition_is_exact && role.role_record_index == role_index &&
          role.batch_index == batch.batch_index &&
          role.event_projection_index < journal.event_projections.size() &&
          (role.role == ExactDirectMorseH0Role::birth ||
           role.role == ExactDirectMorseH0Role::saddle);
    }
    expected_offset += batch.role_record_count;
  }
  role_partition_is_exact =
      role_partition_is_exact && expected_offset == journal.role_records.size();
  check(
      batches_are_strictly_sorted && role_partition_is_exact,
      "batches are strictly sorted by (order, exact level) and partition the canonical role journal");

  check(
      journal.logical_linear_storage_entry_count <=
              journal.logical_linear_storage_entry_limit &&
          journal.no_forbidden_global_structure_materialized &&
          !journal.hierarchy_reduction_performed &&
          !journal.forest_or_gateway_attach_performed &&
          !journal.public_status_claimed && journal.partial_refinement_only,
      "the journal stays O(n+E) and claims neither hierarchy, forest, GatewayAttach nor public status");

  auto mutated = journal;
  ++mutated.batches.front().order;
  check(
      !morsehgp3d::hierarchy::verify_exact_direct_morse_event_journal(
           cloud, facade, mutated)
           .result_certified,
      "fresh replay rejects a mutated exact batch key");
}

void test_source_authority_and_payload_fail_closed() {
  const CanonicalPointCloud cloud = regular_tetrahedron();
  const ExactDirectSupportTerminalBudget budget{
      unlimited_pair_budget(), unlimited_higher_budget()};
  const ExactDirectSupportTerminalFacade facade =
      terminal_facade(cloud, 10U, budget);

  const CanonicalPointCloud other_cloud = translated_tetrahedron();
  const auto authority_mismatch =
      morsehgp3d::hierarchy::build_exact_direct_morse_event_journal(
          other_cloud, facade);
  check(
      authority_mismatch.decision ==
              ExactDirectMorseEventJournalDecision::
                  no_journal_source_authority_mismatch &&
          authority_mismatch.event_projections.empty() &&
          authority_mismatch.role_records.empty() &&
          authority_mismatch.batches.empty(),
      "a same-cardinality but different canonical cloud fails closed through both Phase-9 cloud digests");

  auto inconsistent = facade;
  inconsistent.events.front().birth_order.reset();
  check(
      inconsistent.terminal_catalog_certified(),
      "the source terminal type alone intentionally does not replay its event payload");
  const auto rejected_payload =
      morsehgp3d::hierarchy::build_exact_direct_morse_event_journal(
          cloud, inconsistent);
  check(
      rejected_payload.decision ==
              ExactDirectMorseEventJournalDecision::
                  no_journal_source_facade_payload_inconsistent &&
          rejected_payload.event_projections.empty() &&
          rejected_payload.role_records.empty() &&
          rejected_payload.batches.empty(),
      "a locally inconsistent terminal facade produces no Phase-10 payload");
}

void test_nonterminal_and_extra_shell_sources_fail_closed() {
  const CanonicalPointCloud cloud = regular_tetrahedron();
  ExactDirectSupportTerminalBudget zero_work{
      unlimited_pair_budget(), unlimited_higher_budget()};
  zero_work.pair.maximum_work_unit_count = 0U;
  zero_work.higher.maximum_work_unit_count = 0U;
  const ExactDirectSupportTerminalFacade nonterminal =
      terminal_facade(cloud, 10U, zero_work);
  const auto no_terminal_journal =
      morsehgp3d::hierarchy::build_exact_direct_morse_event_journal(
          cloud, nonterminal);
  check(
      no_terminal_journal.decision ==
              ExactDirectMorseEventJournalDecision::
                  no_journal_source_facade_not_terminal &&
          no_terminal_journal.event_projections.empty() &&
          no_terminal_journal.role_records.empty() &&
          no_terminal_journal.batches.empty(),
      "a residual Phase-9 frontier cannot publish a Phase-10 journal");

  const CanonicalPointCloud square = cocircular_square();
  const ExactDirectSupportTerminalBudget unlimited{
      unlimited_pair_budget(), unlimited_higher_budget()};
  const ExactDirectSupportTerminalFacade degenerate =
      terminal_facade(square, 4U, unlimited);
  const auto no_degenerate_journal =
      morsehgp3d::hierarchy::build_exact_direct_morse_event_journal(
          square, degenerate);
  check(
      degenerate.terminal_catalog_certified() &&
          !degenerate.relevant_extra_shell_diagnostics.empty() &&
          no_degenerate_journal.decision ==
              ExactDirectMorseEventJournalDecision::
                  no_journal_relevant_extra_shell_diagnostics &&
          no_degenerate_journal.event_projections.empty() &&
          no_degenerate_journal.role_records.empty() &&
          no_degenerate_journal.batches.empty(),
      "a relevant cocircular extra shell is retained by Phase 9 and fails closed before any Phase-10 payload");
}

}  // namespace

int main() {
  test_exact_roles_and_batches();
  test_source_authority_and_payload_fail_closed();
  test_nonterminal_and_extra_shell_sources_fail_closed();
  if (failures != 0) {
    std::cerr << failures << " direct Morse event journal checks failed\n";
    return 1;
  }
  std::cout << "all direct Morse event journal checks passed\n";
  return 0;
}
