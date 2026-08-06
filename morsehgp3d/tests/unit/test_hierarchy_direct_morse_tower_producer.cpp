#include "morsehgp3d/hierarchy/direct_morse_tower_producer.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <string>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::CertifiedPoint3;
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

[[nodiscard]] ExactDirectMorseTowerBudget unlimited_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  ExactDirectMorseTowerBudget budget;
  budget.terminal.pair = {
      maximum, maximum, maximum, maximum, maximum, maximum, maximum};
  budget.terminal.higher = {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
  budget.seeds = {maximum, maximum, maximum, maximum};
  return budget;
}

void test_composition_differential_and_receipt() {
  const std::array<CertifiedPoint3, 4U> points{
      point(1.0, 1.0, 1.0),
      point(-1.0, -1.0, 1.0),
      point(-1.0, 1.0, -1.0),
      point(1.0, -1.0, -1.0),
  };
  const auto budget = unlimited_budget();
  auto produced = build_exact_direct_morse_tower_from_cloud(
      points,
      1U,
      ExactDirectMorseTowerBackendKind::exhaustive_host_v1,
      budget);
  check(
      produced.certified_tower() &&
          produced.receipt.decision ==
              ExactDirectMorseTowerDecision::complete_exact_tower &&
          produced.receipt.point_count == points.size() &&
          produced.receipt.single_backend_sealed,
      "the composed tower certifies on the tetrahedron cloud");
  if (!produced.certified_tower()) {
    return;
  }

  // Composition differential: the produced tower must equal, member by
  // member, the manual stage-by-stage composition with the same inputs.
  CanonicalPointCloud cloud = CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto pair = build_exact_pair_support_stream(
      index, cloud, 1U, budget.terminal.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, 1U, budget.terminal.higher);
  const auto facade = build_exact_direct_support_terminal_facade(
      index, cloud, 1U, budget.terminal, pair, higher);
  const auto event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  const auto seed_journal = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, event_journal, budget.seeds);
  const auto& tower = *produced.tower;
  check(
      tower.pair == pair && tower.higher == higher &&
          tower.facade == facade && tower.event_journal == event_journal &&
          tower.seed_journal == seed_journal,
      "the produced tower equals the manual stage composition member by "
      "member");
  check(
      produced.receipt.higher_canonical_cloud_digest ==
              facade.certificate.higher_canonical_cloud_digest &&
          produced.receipt.pair_event_count == pair.events.size() &&
          produced.receipt.higher_event_count == higher.events.size() &&
          produced.receipt.facade_event_count == facade.events.size() &&
          produced.receipt.morse_event_projection_count ==
              event_journal.materialized_direct_event_projections.size() &&
          produced.receipt.saddle_arm_seed_count ==
              seed_journal.arm_seeds.size(),
      "the tower receipt seals the exact stage identities and counts");
}

void test_fail_closed_rejections() {
  const auto budget = unlimited_budget();
  const std::array<CertifiedPoint3, 2U> duplicated{
      point(1.0, 2.0, 3.0),
      point(1.0, 2.0, 3.0),
  };
  const auto duplicate_rejected = build_exact_direct_morse_tower_from_cloud(
      duplicated,
      1U,
      ExactDirectMorseTowerBackendKind::exhaustive_host_v1,
      budget);
  check(
      !duplicate_rejected.certified_tower() &&
          !duplicate_rejected.tower.has_value() &&
          duplicate_rejected.receipt.decision ==
              ExactDirectMorseTowerDecision::no_cloud_rejected,
      "a duplicated point rejects the tower fail-closed at the cloud stage");

  const std::array<CertifiedPoint3, 4U> points{
      point(1.0, 1.0, 1.0),
      point(-1.0, -1.0, 1.0),
      point(-1.0, 1.0, -1.0),
      point(1.0, -1.0, -1.0),
  };
  auto starved = unlimited_budget();
  starved.terminal.higher.maximum_work_unit_count = 1U;
  const auto higher_rejected = build_exact_direct_morse_tower_from_cloud(
      points,
      1U,
      ExactDirectMorseTowerBackendKind::exhaustive_host_v1,
      starved);
  check(
      !higher_rejected.certified_tower() &&
          !higher_rejected.tower.has_value() &&
          higher_rejected.receipt.decision ==
              ExactDirectMorseTowerDecision::no_higher_stream_rejected &&
          higher_rejected.receipt.pair_stream_complete,
      "a starved higher stage rejects the tower fail-closed after the "
      "pair stage");

  const auto forged_backend = build_exact_direct_morse_tower_from_cloud(
      points,
      1U,
      static_cast<ExactDirectMorseTowerBackendKind>(0x7FU),
      budget);
  check(
      !forged_backend.certified_tower() &&
          forged_backend.receipt.decision ==
              ExactDirectMorseTowerDecision::no_backend_rejected,
      "a forged backend value rejects before any stage");
}

}  // namespace

int main() {
  try {
    test_composition_differential_and_receipt();
    test_fail_closed_rejections();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: unexpected exception: " << error.what() << '\n';
    return 1;
  }
  if (failures != 0) {
    std::cerr << failures << " tower producer checks failed\n";
    return 1;
  }
  std::cout << "direct Morse tower producer checks passed\n";
  return 0;
}
