#include "fake_gpu_higher_support_device_tiled_frontier_launchers.hpp"
#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/higher_support_device_tiled_tower.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <vector>

namespace {

using namespace morsehgp3d;
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

[[nodiscard]] ExactHigherSupportStreamBudget unlimited_higher_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

[[nodiscard]] ExactDirectMorseTowerBudget unlimited_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  ExactDirectMorseTowerBudget budget;
  budget.terminal.pair = {
      maximum, maximum, maximum, maximum, maximum, maximum, maximum};
  budget.terminal.higher = unlimited_higher_budget();
  budget.seeds = {maximum, maximum, maximum, maximum};
  return budget;
}

// 4-b1 differential: the device-tiled assembly -- the anchored session
// driven to terminal through the sealed M2 bridge, backed locally by the
// certified scientific fake -- must reproduce the exhaustive oracle's
// events, diagnostics, requirements and universe accounting under the
// oracle's own public canonical order.
void run_assembly_differential(
    std::span<const CertifiedPoint3> points,
    std::size_t requested_maximum_order,
    const std::string& label) {
  CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(points);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto oracle = build_exact_higher_support_stream(
      index, cloud, requested_maximum_order, unlimited_higher_budget());
  check(oracle.stream_complete(), label + ": the exhaustive oracle completes");

  gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build();
  gpu::test_support::reset_fake_gpu_higher_support_device_tiled_frontier();
  gpu::test_support::bind_fake_higher_support_device_tiled_geometry(
      index, cloud);
  const auto assembly =
      gpu::assemble_exact_higher_support_stream_device_tiled(
          cloud,
          index,
          requested_maximum_order,
          unlimited_higher_budget());
  check(
      assembly.certified_assembled(),
      label + ": the device-tiled assembly reaches session terminal");
  if (!assembly.certified_assembled() || !oracle.stream_complete()) {
    return;
  }
  check(
      assembly.certificate.minted() &&
          assembly.certificate.certifies(*assembly.higher),
      label + ": the minted anchored certificate certifies the sealed "
              "result");
  const auto& higher = *assembly.higher;
  check(
      higher.events == oracle.events,
      label + ": the assembled events equal the oracle in canonical order");
  check(
      higher.relevant_extra_shell_diagnostics ==
          oracle.relevant_extra_shell_diagnostics,
      label + ": the assembled diagnostics equal the oracle in canonical "
              "order");
  check(
      higher.requirements == oracle.requirements,
      label + ": the assembled requirements equal the oracle");
  check(
      higher.audit.exact_bigint_universe_certified &&
          higher.audit.total_support_count ==
              oracle.audit.total_support_count &&
          higher.audit.resolved_support_count ==
              oracle.audit.resolved_support_count &&
          higher.audit.remaining_frontier_support_count == 0U,
      label + ": the assembled universe accounting equals the oracle");
  check(
      higher.stream_complete() &&
          higher.absence_of_additional_higher_supports_certified(),
      label + ": the assembled result carries the completion certification");
}

// 4-b2 end-to-end differential: the complete device-tiled tower -- pair
// stream, anchored device-tiled higher stream sealed under its certificate,
// facade through the anchored_session_chain source kind, both journals --
// must reproduce the exhaustive host tower's scientific content.  Only the
// facade's provenance facts (source kind, replay flag, chunk accounting and
// checkpoint digest) may differ between the two backends.
void run_tower_differential(
    std::span<const CertifiedPoint3> points,
    std::size_t requested_maximum_order,
    const std::string& label) {
  const auto host = build_exact_direct_morse_tower_from_cloud(
      points,
      requested_maximum_order,
      ExactDirectMorseTowerBackendKind::exhaustive_host_v1,
      unlimited_budget());
  check(host.certified_tower(), label + ": the exhaustive host tower is certified");

  auto cloud = CanonicalPointCloud::rejecting_duplicates(points);
  auto index = MortonLbvhIndex::build(cloud);
  gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build();
  gpu::test_support::reset_fake_gpu_higher_support_device_tiled_frontier();
  gpu::test_support::bind_fake_higher_support_device_tiled_geometry(
      index, cloud);
  const auto device =
      gpu::build_exact_direct_morse_tower_from_cloud_device_tiled(
          std::move(cloud),
          std::move(index),
          requested_maximum_order,
          unlimited_budget());
  check(
      device.certified_tower(),
      label + ": the device-tiled tower is certified end to end (decision=" +
          std::to_string(static_cast<int>(device.receipt.decision)) + ")");
  if (!host.certified_tower() || !device.certified_tower()) {
    return;
  }
  const auto& host_tower = *host.tower;
  const auto& device_tower = *device.tower;
  check(
      device_tower.pair == host_tower.pair,
      label + ": the device tower's pair stream equals the host tower's");
  check(
      device_tower.higher.events == host_tower.higher.events &&
          device_tower.higher.relevant_extra_shell_diagnostics ==
              host_tower.higher.relevant_extra_shell_diagnostics &&
          device_tower.higher.requirements ==
              host_tower.higher.requirements,
      label + ": the device tower's higher records equal the host tower's");
  const auto& host_facade = host_tower.facade.certificate;
  const auto& device_facade = device_tower.facade.certificate;
  check(
      device_tower.facade.events == host_tower.facade.events &&
          device_tower.facade.relevant_extra_shell_diagnostics ==
              host_tower.facade.relevant_extra_shell_diagnostics,
      label + ": the device facade payload equals the host facade payload");
  check(
      device_facade.normalized_terminal_output_digest ==
              host_facade.normalized_terminal_output_digest &&
          device_facade.pair_semantic_digest ==
              host_facade.pair_semantic_digest &&
          device_facade.higher_semantic_digest ==
              host_facade.higher_semantic_digest &&
          device_facade.exact_candidate_universe_size ==
              host_facade.exact_candidate_universe_size &&
          device_facade.arity_certificates ==
              host_facade.arity_certificates,
      label + ": the device facade certificate carries the host facade's "
              "scientific digests and universe closure");
  check(
      device_facade.higher_source_kind ==
              ExactDirectSupportHigherSourceKind::anchored_session_chain &&
          !device_facade.higher_result_freshly_replayed &&
          host_facade.higher_source_kind ==
              ExactDirectSupportHigherSourceKind::fresh_resident_replay &&
          host_facade.higher_result_freshly_replayed,
      label + ": the two backends declare their distinct higher source "
              "kinds truthfully");
  check(
      device_tower.event_journal == host_tower.event_journal,
      label + ": the device tower's Morse event journal equals the host "
              "tower's");
  check(
      device_tower.seed_journal == host_tower.seed_journal,
      label + ": the device tower's saddle-arm seed journal equals the "
              "host tower's");
  check(
      device.receipt.backend ==
              ExactDirectMorseTowerBackendKind::device_tiled_session_v1 &&
          device.receipt.decision ==
              ExactDirectMorseTowerDecision::complete_exact_tower &&
          device.receipt.higher_event_count ==
              host.receipt.higher_event_count &&
          device.receipt.facade_event_count ==
              host.receipt.facade_event_count &&
          device.receipt.morse_event_projection_count ==
              host.receipt.morse_event_projection_count &&
          device.receipt.saddle_arm_seed_count ==
              host.receipt.saddle_arm_seed_count,
      label + ": the device receipt seals its backend and matches the host "
              "stage counts");
}

// Anti-forge: the anchored certificate must fail closed against a mutated
// result, an inflated audit, a foreign authority and a pre-terminal mint.
void test_anchored_certificate_anti_forge() {
  const std::array<CertifiedPoint3, 4U> tetrahedron{
      point(1.0, 1.0, 1.0),
      point(-1.0, -1.0, 1.0),
      point(-1.0, 1.0, -1.0),
      point(1.0, -1.0, -1.0),
  };
  const auto tetra_cloud = CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{tetrahedron});
  const auto tetra_index = MortonLbvhIndex::build(tetra_cloud);
  gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build();
  gpu::test_support::reset_fake_gpu_higher_support_device_tiled_frontier();
  gpu::test_support::bind_fake_higher_support_device_tiled_geometry(
      tetra_index, tetra_cloud);
  const auto tetra = gpu::assemble_exact_higher_support_stream_device_tiled(
      tetra_cloud, tetra_index, 1U, unlimited_higher_budget());
  check(
      tetra.certified_assembled(),
      "anti-forge: the tetrahedron assembly seals");
  if (!tetra.certified_assembled()) {
    return;
  }

  // Inflated audit: one forged support in the universe accounting.
  auto inflated = *tetra.higher;
  ++inflated.audit.total_support_count;
  check(
      !tetra.certificate.certifies(inflated),
      "anti-forge: an inflated audit fails the certificate");

  // Mutated record set: one appropriated event dropped.
  if (!tetra.higher->events.empty()) {
    auto truncated = *tetra.higher;
    truncated.events.pop_back();
    check(
        !tetra.certificate.certifies(truncated),
        "anti-forge: a dropped event fails the certificate");
  }

  // Foreign certificate: a second anchored authority on a different cloud.
  const std::array<CertifiedPoint3, 5U> pyramid{
      point(1.0, 1.0, 0.0),
      point(-1.0, 1.0, 0.0),
      point(-1.0, -1.0, 0.0),
      point(1.0, -1.0, 0.0),
      point(0.0, 0.0, 1.5),
  };
  const auto pyramid_cloud = CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{pyramid});
  const auto pyramid_index = MortonLbvhIndex::build(pyramid_cloud);
  gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build();
  gpu::test_support::reset_fake_gpu_higher_support_device_tiled_frontier();
  gpu::test_support::bind_fake_higher_support_device_tiled_geometry(
      pyramid_index, pyramid_cloud);
  const auto pyramid_assembly =
      gpu::assemble_exact_higher_support_stream_device_tiled(
          pyramid_cloud, pyramid_index, 1U, unlimited_higher_budget());
  check(
      pyramid_assembly.certified_assembled(),
      "anti-forge: the pyramid assembly seals");
  if (pyramid_assembly.certified_assembled()) {
    check(
        !tetra.certificate.certified_for_authority(
            pyramid_assembly.certificate.manifest()) &&
            !pyramid_assembly.certificate.certified_for_authority(
                tetra.certificate.manifest()),
        "anti-forge: certificates are bound to their own authority "
        "manifest");
    check(
        !tetra.certificate.certifies(*pyramid_assembly.higher) &&
            !pyramid_assembly.certificate.certifies(*tetra.higher),
        "anti-forge: a foreign result fails the certificate");
  }

  // Pre-terminal mint: sealing before the session is locally complete
  // yields no certificate.
  ExactHigherSupportAuthorityContext authority{tetra_index, tetra_cloud, 1U};
  ExactHigherSupportAnchoredStreamAssembler premature{authority};
  const auto premature_seal =
      premature.seal_terminal_stream(unlimited_higher_budget());
  check(
      !premature_seal.certified_sealed() &&
          !premature_seal.certificate.minted(),
      "anti-forge: a pre-terminal mint is refused");
}

// 4-c2: the two scalable lanes together.  The sealed P8l sparse pair
// authority and the anchored-session-chain certified device higher stream
// compose into a certified facade whose payload equals the fresh oracle's,
// with truthfully distinct source kinds on both lanes and no exhaustive
// replay anywhere.
[[nodiscard]] ExactSparseAnchoredPairTerminalAuthority
build_sparse_pair_authority(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_closed_rank) {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  ExactSparseAnchoredPairSession session =
      ExactSparseAnchoredPairSession::start(
          index,
          cloud,
          maximum_closed_rank,
          ExactMortonGroupedAnchoredPairScheduleConfig{4U, 0U},
          ExactSparseAnchoredPairSessionTotalCapacity{
              maximum,
              maximum,
              maximum,
              maximum,
              maximum,
              maximum,
              maximum,
              maximum});
  const ExactSparseAnchoredPairSessionAdvanceBudget advance_budget{
      {4096U, 4096U, 4096U, 4096U},
      {4096U},
      1U,
      maximum_closed_rank + 1U};
  for (std::size_t call = 0U; call < 100000U; ++call) {
    const auto step = session.advance(index, cloud, advance_budget);
    if (step.kind() ==
        ExactSparseAnchoredPairSessionStepKind::complete) {
      return std::move(session).seal();
    }
    if (step.kind() ==
        ExactSparseAnchoredPairSessionStepKind::total_capacity_exhausted) {
      break;
    }
  }
  throw std::logic_error(
      "the sparse pair authority fixture did not terminate");
}

void run_scalable_lanes_differential(
    std::span<const CertifiedPoint3> points,
    std::size_t requested_maximum_order,
    const std::string& label) {
  CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(points);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSupportTerminalBudget oracle_budget{
      {std::numeric_limits<std::size_t>::max(),
       std::numeric_limits<std::size_t>::max(),
       std::numeric_limits<std::size_t>::max(),
       std::numeric_limits<std::size_t>::max(),
       std::numeric_limits<std::size_t>::max(),
       std::numeric_limits<std::size_t>::max(),
       std::numeric_limits<std::size_t>::max()},
      unlimited_higher_budget()};
  const auto pair_oracle = build_exact_pair_support_stream(
      index, cloud, requested_maximum_order, oracle_budget.pair);
  const auto higher_oracle = build_exact_higher_support_stream(
      index, cloud, requested_maximum_order, oracle_budget.higher);
  const auto oracle_facade = build_exact_direct_support_terminal_facade(
      index,
      cloud,
      requested_maximum_order,
      oracle_budget,
      pair_oracle,
      higher_oracle);
  check(
      oracle_facade.terminal_catalog_certified(),
      label + ": the fresh oracle facade certifies");

  gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build();
  gpu::test_support::reset_fake_gpu_higher_support_device_tiled_frontier();
  gpu::test_support::bind_fake_higher_support_device_tiled_geometry(
      index, cloud);
  const auto assembly =
      gpu::assemble_exact_higher_support_stream_device_tiled(
          cloud,
          index,
          requested_maximum_order,
          unlimited_higher_budget());
  check(
      assembly.certified_assembled(),
      label + ": the device-tiled higher assembly seals");
  if (!assembly.certified_assembled() ||
      !oracle_facade.terminal_catalog_certified()) {
    return;
  }

  const auto pair_manifest =
      make_exact_pair_support_checkpoint_manifest(
          index, cloud, requested_maximum_order);
  auto pair_authority = build_sparse_pair_authority(
      index,
      cloud,
      std::max<std::size_t>(
          2U, pair_manifest.maximum_relevant_closed_rank));
  const auto facade = build_exact_direct_support_terminal_facade(
      index,
      cloud,
      requested_maximum_order,
      unlimited_higher_budget(),
      std::move(pair_authority),
      *assembly.higher,
      assembly.certificate);
  check(
      facade.terminal_catalog_certified(),
      label + ": the scalable-lanes facade certifies");
  check(
      facade.events == oracle_facade.events &&
          facade.relevant_extra_shell_diagnostics ==
              oracle_facade.relevant_extra_shell_diagnostics &&
          facade.certificate.normalized_terminal_output_digest ==
              oracle_facade.certificate.normalized_terminal_output_digest,
      label + ": the scalable-lanes facade payload equals the fresh "
              "oracle facade payload");
  check(
      facade.certificate.pair_source_kind ==
              ExactDirectSupportPairSourceKind::
                  sealed_sparse_anchored_session &&
          facade.certificate.higher_source_kind ==
              ExactDirectSupportHigherSourceKind::anchored_session_chain &&
          !facade.certificate.pair_result_freshly_replayed &&
          !facade.certificate.higher_result_freshly_replayed,
      label + ": both scalable lanes declare their sealed source kinds "
              "truthfully");

  // Anti-forge: a declared-budget mismatch is refused before any payload.
  auto mismatch_authority = build_sparse_pair_authority(
      index,
      cloud,
      std::max<std::size_t>(
          2U, pair_manifest.maximum_relevant_closed_rank));
  ExactHigherSupportStreamBudget wrong_budget = unlimited_higher_budget();
  wrong_budget.maximum_work_unit_count = 1U;
  const auto mismatched = build_exact_direct_support_terminal_facade(
      index,
      cloud,
      requested_maximum_order,
      wrong_budget,
      std::move(mismatch_authority),
      *assembly.higher,
      assembly.certificate);
  check(
      !mismatched.terminal_catalog_certified() &&
          mismatched.certificate.decision ==
              ExactDirectSupportTerminalDecision::
                  source_result_not_certified &&
          mismatched.events.empty(),
      label + ": a declared higher-budget mismatch fails closed with no "
              "payload");
}

void test_scalable_lanes_differentials() {
  const std::array<CertifiedPoint3, 4U> tetrahedron{
      point(1.0, 1.0, 1.0),
      point(-1.0, -1.0, 1.0),
      point(-1.0, 1.0, -1.0),
      point(1.0, -1.0, -1.0),
  };
  run_scalable_lanes_differential(
      tetrahedron, 1U, "scalable-lanes tetrahedron/K1");

  std::vector<CertifiedPoint3> skew_cube;
  for (int x = 0; x < 2; ++x) {
    for (int y = 0; y < 2; ++y) {
      for (int z = 0; z < 2; ++z) {
        skew_cube.push_back(point(
            x * 1.0 + 0.125 * z, y * 1.0 + 0.0625 * x, z * 1.0 + 0.03125 * y));
      }
    }
  }
  run_scalable_lanes_differential(
      skew_cube, 3U, "scalable-lanes skewcube8/K3");
}

void test_tower_differentials() {
  const std::array<CertifiedPoint3, 4U> tetrahedron{
      point(1.0, 1.0, 1.0),
      point(-1.0, -1.0, 1.0),
      point(-1.0, 1.0, -1.0),
      point(1.0, -1.0, -1.0),
  };
  run_tower_differential(tetrahedron, 1U, "tower tetrahedron/K1");

  // The octahedron-plus-poles sphere cloud legitimately fails the event
  // journal's partial-refinement certification on every backend, so the
  // end-to-end fixture is a skewed cube in general position whose
  // exhaustive host tower certifies through both journals at K=3.
  std::vector<CertifiedPoint3> skew_cube;
  for (int x = 0; x < 2; ++x) {
    for (int y = 0; y < 2; ++y) {
      for (int z = 0; z < 2; ++z) {
        skew_cube.push_back(point(
            x * 1.0 + 0.125 * z, y * 1.0 + 0.0625 * x, z * 1.0 + 0.03125 * y));
      }
    }
  }
  run_tower_differential(skew_cube, 3U, "tower skewcube8/K3");
}

void test_assembly_differentials() {
  const std::array<CertifiedPoint3, 4U> tetrahedron{
      point(1.0, 1.0, 1.0),
      point(-1.0, -1.0, 1.0),
      point(-1.0, 1.0, -1.0),
      point(1.0, -1.0, -1.0),
  };
  run_assembly_differential(tetrahedron, 1U, "tetrahedron/K1");

  std::vector<CertifiedPoint3> line12;
  for (std::size_t index = 0U; index < 12U; ++index) {
    const double coordinate = static_cast<double>(index);
    line12.push_back(
        point(coordinate, coordinate / 8.0, coordinate / 64.0));
  }
  run_assembly_differential(line12, 2U, "line12/K2");

  const std::array<CertifiedPoint3, 8U> sphere{
      point(1.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(0.0, -1.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(0.0, 0.0, -1.0),
      point(0.5773502691896258, 0.5773502691896258, 0.5773502691896257),
      point(-0.5773502691896258, -0.5773502691896258, 0.5773502691896258),
  };
  run_assembly_differential(sphere, 3U, "sphere8/K3");
}

}  // namespace

int main() {
  try {
    test_assembly_differentials();
    test_tower_differentials();
    test_scalable_lanes_differentials();
    test_anchored_certificate_anti_forge();
  } catch (const std::exception& error) {
    std::cerr << "FAIL: unexpected exception: " << error.what() << '\n';
    return 1;
  }
  if (failures != 0) {
    std::cerr << failures << " device-tiled tower checks failed\n";
    return 1;
  }
  std::cout << "device-tiled tower checks passed\n";
  return 0;
}
