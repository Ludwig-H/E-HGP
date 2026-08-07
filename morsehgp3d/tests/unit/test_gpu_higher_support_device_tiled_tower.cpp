#include "fake_gpu_higher_support_device_tiled_frontier_launchers.hpp"
#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/higher_support_device_tiled_tower.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d;
using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::hierarchy::ExactHigherSupportVerificationBasis;

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

// R1-b: the same device-tiled assembly driven with the tile-certified
// commit path -- no host generator, no work-unit boundary search -- must
// reproduce the exhaustive oracle exactly, and its certificate must
// declare the tile-certified basis.
void run_tile_certified_assembly_differential(
    std::span<const CertifiedPoint3> points,
    std::size_t requested_maximum_order,
    const std::string& label) {
  CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(points);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto oracle = build_exact_higher_support_stream(
      index, cloud, requested_maximum_order, unlimited_higher_budget());
  check(
      oracle.stream_complete(),
      label + ": the tile-certified oracle completes");

  gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build();
  gpu::test_support::reset_fake_gpu_higher_support_device_tiled_frontier();
  gpu::test_support::bind_fake_higher_support_device_tiled_geometry(
      index, cloud);
  gpu::HigherSupportDeviceTiledSessionBridgeConfig config;
  config.tile_certified_commit = true;
  const auto assembly =
      gpu::assemble_exact_higher_support_stream_device_tiled(
          cloud,
          index,
          requested_maximum_order,
          unlimited_higher_budget(),
          config);
  check(
      assembly.certified_assembled(),
      label + ": the tile-certified assembly seals");
  if (!assembly.certified_assembled() || !oracle.stream_complete()) {
    return;
  }
  check(
      assembly.higher->events == oracle.events &&
          assembly.higher->relevant_extra_shell_diagnostics ==
              oracle.relevant_extra_shell_diagnostics &&
          assembly.higher->requirements == oracle.requirements,
      label + ": the tile-certified assembly equals the oracle records");
  check(
      assembly.higher->audit.total_support_count ==
              oracle.audit.total_support_count &&
          assembly.higher->audit.resolved_support_count ==
              oracle.audit.resolved_support_count &&
          assembly.higher->audit.remaining_frontier_support_count == 0U &&
          assembly.higher->stream_complete() &&
          assembly.higher->absence_of_additional_higher_supports_certified(),
      label + ": the tile-certified assembly closes the exact universe");
  check(
      assembly.higher->audit.work_unit_count == 0U &&
          assembly.higher->audit.support_product_visit_count == 0U &&
          assembly.higher->audit.rank_witness_node_visit_count == 0U,
      label + ": the tile-certified assembly runs no host generator");
  check(
      assembly.certificate.verification_basis() ==
          ExactHigherSupportVerificationBasis::
              device_search_host_exact_record_classification_bigint_closure,
      label + ": the certificate declares the tile-certified basis");
}

void test_tile_certified_assembly_differentials() {
  const std::array<CertifiedPoint3, 4U> tetrahedron{
      point(1.0, 1.0, 1.0),
      point(-1.0, -1.0, 1.0),
      point(-1.0, 1.0, -1.0),
      point(1.0, -1.0, -1.0),
  };
  run_tile_certified_assembly_differential(
      tetrahedron, 1U, "tile-certified tetrahedron/K1");

  std::vector<CertifiedPoint3> line12;
  for (std::size_t index = 0U; index < 12U; ++index) {
    const double coordinate = static_cast<double>(index);
    line12.push_back(
        point(coordinate, coordinate / 8.0, coordinate / 64.0));
  }
  run_tile_certified_assembly_differential(
      line12, 2U, "tile-certified line12/K2");

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
  run_tile_certified_assembly_differential(
      sphere, 3U, "tile-certified sphere8/K3");
}

// R2-c differential: the operational guard is a session circuit breaker,
// never a scientific parameter.  Three properties are certified here.
// (i) Fidelity: an unarmed guard reproduces the unguarded assembly exactly.
// (ii) Prefix truncation: a guard armed at k committed transitions yields
// exactly the state the unguarded chain held after its own k-th commit --
// same resolved mass, same terminal count, same frontier -- so the guard
// only ever truncates a prefix of the identical chain.  (iii) Honesty: a
// censored assembly certifies nothing, mints no certificate, and still
// satisfies the anchored induction identity R + C(F) = C(n,3)+C(n,4) in
// exact::BigInt, which is what makes its progress transcript a usable
// measurement rather than a guess.
[[nodiscard]] gpu::HigherSupportDeviceTiledStreamAssembly
assemble_tile_certified(
    const CanonicalPointCloud& cloud,
    const MortonLbvhIndex& index,
    std::size_t requested_maximum_order,
    const gpu::HigherSupportDeviceTiledAssemblyOperationalGuard& guard) {
  gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build();
  gpu::test_support::reset_fake_gpu_higher_support_device_tiled_frontier();
  gpu::test_support::bind_fake_higher_support_device_tiled_geometry(
      index, cloud);
  gpu::HigherSupportDeviceTiledSessionBridgeConfig config;
  config.tile_certified_commit = true;
  return gpu::assemble_exact_higher_support_stream_device_tiled(
      cloud,
      index,
      requested_maximum_order,
      unlimited_higher_budget(),
      config,
      guard);
}

void run_operational_guard_differential(
    std::span<const CertifiedPoint3> points,
    std::size_t requested_maximum_order,
    const std::string& label) {
  CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(points);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const exact::BigInt universe =
      exact_higher_support_candidate_universe_size(cloud.size());

  const auto unguarded = assemble_tile_certified(
      cloud, index, requested_maximum_order, {});
  check(
      unguarded.certified_assembled(),
      label + ": the unguarded tile-certified assembly seals");
  if (!unguarded.certified_assembled()) {
    return;
  }
  check(
      !unguarded.censored() &&
          unguarded.censure ==
              gpu::HigherSupportDeviceTiledAssemblyCensure::none,
      label + ": an unarmed guard censors nothing");
  // The progress transcript is published on the successful path too, and
  // it is the sealed audit itself -- not a parallel accounting.
  check(
      unguarded.progress_audit.resolved_support_count ==
              unguarded.higher->audit.resolved_support_count &&
          unguarded.progress_audit.total_support_count == universe &&
          unguarded.progress_audit.remaining_frontier_support_count == 0U &&
          unguarded.progress_frontier_entry_count == 0U,
      label + ": the terminal progress transcript is the sealed audit");
  const std::size_t terminal_transaction_count =
      unguarded.progress_committed_transaction_count;
  check(
      terminal_transaction_count != 0U,
      label + ": the chain committed at least one transition");

  // (iii) Zero-transaction censure: nothing decided, the whole universe
  // still on the frontier, no certificate.
  gpu::HigherSupportDeviceTiledAssemblyOperationalGuard immediate;
  immediate.maximum_committed_transaction_count = 0U;
  const auto censored_at_zero = assemble_tile_certified(
      cloud, index, requested_maximum_order, immediate);
  check(
      censored_at_zero.censored() &&
          censored_at_zero.censure ==
              gpu::HigherSupportDeviceTiledAssemblyCensure::
                  committed_transaction_ceiling &&
          !censored_at_zero.certified_assembled() &&
          !censored_at_zero.certificate.minted() &&
          !censored_at_zero.higher.has_value(),
      label + ": a zero-transaction ceiling censors and certifies nothing");
  check(
      censored_at_zero.progress_audit.resolved_support_count == 0U &&
          censored_at_zero.progress_audit
                  .remaining_frontier_support_count == universe &&
          censored_at_zero.progress_audit.total_support_count == universe &&
          censored_at_zero.progress_committed_transaction_count == 0U,
      label + ": the censored-at-zero transcript holds the whole universe");

  // (ii) Prefix truncation, and (iii) the induction identity at every stop.
  exact::BigInt previous_resolved{0};
  for (std::size_t ceiling = 1U; ceiling <= terminal_transaction_count;
       ++ceiling) {
    gpu::HigherSupportDeviceTiledAssemblyOperationalGuard guard;
    guard.maximum_committed_transaction_count = ceiling;
    const auto truncated =
        assemble_tile_certified(cloud, index, requested_maximum_order, guard);
    const std::string at = label + " @" + std::to_string(ceiling);
    const bool reaches_terminal = ceiling >= terminal_transaction_count;
    check(
        truncated.censored() != reaches_terminal,
        at + ": the ceiling censors exactly below the terminal");
    check(
        truncated.progress_audit.resolved_support_count +
                truncated.progress_audit
                    .remaining_frontier_support_count ==
            universe,
        at + ": R + C(F) == C(n,3)+C(n,4) holds at the stop");
    check(
        truncated.progress_audit.total_support_count == universe,
        at + ": the censored transcript still knows the exact universe");
    check(
        truncated.progress_audit.resolved_support_count >= previous_resolved,
        at + ": resolved mass never decreases as the ceiling rises");
    previous_resolved = truncated.progress_audit.resolved_support_count;
    if (reaches_terminal) {
      // The truncation is a prefix of the identical chain: at or above the
      // real transaction count the guard is invisible.
      check(
          truncated.certified_assembled() &&
              truncated.higher->events == unguarded.higher->events &&
              truncated.higher->relevant_extra_shell_diagnostics ==
                  unguarded.higher->relevant_extra_shell_diagnostics &&
              truncated.higher->audit == unguarded.higher->audit &&
              truncated.certificate.verification_basis() ==
                  ExactHigherSupportVerificationBasis::
                      device_search_host_exact_record_classification_bigint_closure,
          at + ": a non-binding ceiling reproduces the unguarded assembly");
    } else {
      check(
          !truncated.certified_assembled() &&
              !truncated.certificate.minted() &&
              !truncated.higher.has_value() &&
              truncated.progress_committed_transaction_count == ceiling &&
              truncated.progress_frontier_entry_count != 0U,
          at + ": a binding ceiling stops on a live frontier and seals nothing");
      check(
          truncated.progress_audit.minimal_leaf_count <=
                  unguarded.higher->audit.minimal_leaf_count &&
              truncated.progress_audit.resolved_support_count < universe,
          at + ": the censored transcript is a prefix of the terminal one");
    }
  }

  // (i) and the deadline axis: a deadline already in the past censors with
  // the deadline reason before any transition; a deadline that cannot be
  // reached is invisible.
  gpu::HigherSupportDeviceTiledAssemblyOperationalGuard elapsed;
  elapsed.deadline =
      std::chrono::steady_clock::now() - std::chrono::hours{1};
  const auto censored_by_deadline = assemble_tile_certified(
      cloud, index, requested_maximum_order, elapsed);
  check(
      censored_by_deadline.censored() &&
          censored_by_deadline.censure ==
              gpu::HigherSupportDeviceTiledAssemblyCensure::
                  operational_deadline &&
          !censored_by_deadline.certified_assembled() &&
          !censored_by_deadline.certificate.minted() &&
          censored_by_deadline.progress_committed_transaction_count == 0U &&
          censored_by_deadline.progress_audit
                  .remaining_frontier_support_count == universe,
      label + ": an elapsed deadline censors before any transition");

  gpu::HigherSupportDeviceTiledAssemblyOperationalGuard distant;
  distant.deadline =
      std::chrono::steady_clock::now() + std::chrono::hours{24};
  const auto unreachable_deadline = assemble_tile_certified(
      cloud, index, requested_maximum_order, distant);
  check(
      unreachable_deadline.certified_assembled() &&
          !unreachable_deadline.censored() &&
          unreachable_deadline.higher->events == unguarded.higher->events &&
          unreachable_deadline.higher->audit == unguarded.higher->audit,
      label + ": an unreachable deadline reproduces the unguarded assembly");
}

// R2-i: a report of failure must always say how far the run got.  Whatever
// makes the assembly fail -- a censored engine, a poisoned bridge, an
// exception escaping the transaction loop -- the transcript must still
// state the exact universe and a mass partition that closes.  The only
// admissible empty transcript is the one where the bridge itself could not
// be constructed, which no fake reaches.
void run_failed_assembly_transcript_case(
    std::span<const CertifiedPoint3> points,
    std::size_t requested_maximum_order,
    gpu::test_support::FakeHigherSupportDeviceTiledFrontierCorruption
        corruption,
    const std::string& label) {
  CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(points);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const exact::BigInt universe =
      exact_higher_support_candidate_universe_size(cloud.size());

  gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build();
  gpu::test_support::reset_fake_gpu_higher_support_device_tiled_frontier();
  gpu::test_support::bind_fake_higher_support_device_tiled_geometry(
      index, cloud);
  gpu::test_support::
      set_fake_gpu_higher_support_device_tiled_frontier_corruption(
          corruption);
  gpu::HigherSupportDeviceTiledSessionBridgeConfig config;
  config.tile_certified_commit = true;
  const auto assembly =
      gpu::assemble_exact_higher_support_stream_device_tiled(
          cloud,
          index,
          requested_maximum_order,
          unlimited_higher_budget(),
          config);
  gpu::test_support::reset_fake_gpu_higher_support_device_tiled_frontier();

  check(
      !assembly.certified_assembled(),
      label + ": a corrupted engine must never certify");
  if (assembly.certified_assembled()) {
    return;
  }
  check(
      assembly.progress_audit.total_support_count == universe,
      label + ": the failure transcript states the exact universe");
  check(
      assembly.progress_audit.resolved_support_count +
              assembly.progress_audit.remaining_frontier_support_count ==
          universe,
      label + ": the failure transcript still closes R + C(F)");
  check(
      !assembly.certificate.minted() && !assembly.higher.has_value(),
      label + ": a failed assembly mints nothing and holds no stream");
}

void test_failed_assembly_publishes_its_transcript() {
  using Corruption =
      gpu::test_support::FakeHigherSupportDeviceTiledFrontierCorruption;
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
  for (const auto& [corruption, name] :
       std::array<std::pair<Corruption, const char*>, 4U>{
           {{Corruption::broken_partition, "broken_partition"},
            {Corruption::mass_without_record, "mass_without_record"},
            {Corruption::cumulative_rollback, "cumulative_rollback"},
            {Corruption::forged_root_digest, "forged_root_digest"}}}) {
    run_failed_assembly_transcript_case(
        sphere, 3U, corruption, std::string{"failed/"} + name);
  }
}

// T1: bounding how many frontier roots one tile consumes must change the
// GRANULARITY of the chain and nothing else.  A capped tile commits more,
// smaller transactions -- which is the whole point, since a transaction
// resolves its roots entirely or commits nothing -- and must still reach
// the identical terminal: same events, same diagnostics, same audit, same
// exact universe closure, same declared basis.
void run_bounded_tile_root_differential(
    std::span<const CertifiedPoint3> points,
    std::size_t requested_maximum_order,
    std::size_t tile_root_cap,
    const std::string& label) {
  CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(points);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);

  const auto whole = assemble_tile_certified(
      cloud, index, requested_maximum_order, {});
  gpu::HigherSupportDeviceTiledSessionBridgeConfig capped;
  capped.tile_certified_commit = true;
  capped.maximum_tile_root_count = tile_root_cap;
  gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build();
  gpu::test_support::reset_fake_gpu_higher_support_device_tiled_frontier();
  gpu::test_support::bind_fake_higher_support_device_tiled_geometry(
      index, cloud);
  const auto bounded =
      gpu::assemble_exact_higher_support_stream_device_tiled(
          cloud,
          index,
          requested_maximum_order,
          unlimited_higher_budget(),
          capped);

  check(
      whole.certified_assembled() && bounded.certified_assembled(),
      label + ": both the whole-frontier and the capped tile assemblies "
              "seal");
  if (!whole.certified_assembled() || !bounded.certified_assembled()) {
    return;
  }
  check(
      bounded.higher->events == whole.higher->events,
      label + ": same events");
  check(
      bounded.higher->relevant_extra_shell_diagnostics ==
          whole.higher->relevant_extra_shell_diagnostics,
      label + ": same diagnostics");
  check(
      bounded.higher->requirements == whole.higher->requirements,
      label + ": same requirements");
  // The audit is NOT expected to match in full, and must not be asserted
  // to: cutting the chain into more transactions legitimately changes the
  // expansion counts, the child-product counts and the peak frontier size.
  // What must be invariant is everything that describes the RESULT -- the
  // six exact masses and every per-support category count -- because those
  // are properties of the universe, not of how it was sliced.
  const auto& b = bounded.higher->audit;
  const auto& w = whole.higher->audit;
  check(
      b.total_support_count == w.total_support_count &&
          b.resolved_support_count == w.resolved_support_count &&
          b.remaining_frontier_support_count ==
              w.remaining_frontier_support_count &&
          b.leaf_classified_support_count ==
              w.leaf_classified_support_count,
      label + ": universe, resolved, remaining and classified mass are "
              "invariant under the cut");
  // The split of the PRUNED mass between well-centering and rank is a
  // property of the traversal, not of the universe: a coarser tile can
  // discard a whole subtree as not well centred where a finer one descends
  // into it and rank-prunes the parts.  Their SUM is invariant, and that is
  // the statement the partition actually makes.
  check(
      b.well_centering_pruned_support_count + b.rank_pruned_support_count ==
          w.well_centering_pruned_support_count +
              w.rank_pruned_support_count,
      label + ": the total pruned mass is invariant under the cut");
  check(
      b.well_centering_pruned_support_count +
              b.rank_pruned_support_count +
              b.leaf_classified_support_count ==
          b.total_support_count,
      label + ": the capped chain's own mass partition closes");
  check(
      b.minimal_leaf_count == w.minimal_leaf_count &&
          b.above_rank_leaf_count == w.above_rank_leaf_count &&
          b.affinely_dependent_leaf_count ==
              w.affinely_dependent_leaf_count &&
          b.boundary_reduced_leaf_count == w.boundary_reduced_leaf_count &&
          b.exterior_circumcenter_leaf_count ==
              w.exterior_circumcenter_leaf_count &&
          b.leaf_support_analysis_count == w.leaf_support_analysis_count,
      label + ": every per-support category count is invariant");
  check(
      b.accepted_event_count == w.accepted_event_count &&
          b.relevant_extra_shell_diagnostic_count ==
              w.relevant_extra_shell_diagnostic_count &&
          b.emitted_record_count == w.emitted_record_count &&
          b.exact_bigint_universe_certified &&
          b.grouped_partition_accounting_certified,
      label + ": the emitted record accounting is invariant and certified");
  check(
      bounded.certificate.verification_basis() ==
          ExactHigherSupportVerificationBasis::
              device_search_host_exact_record_classification_bigint_closure,
      label + ": the capped chain still declares the tile-certified basis");
  check(
      bounded.bridge_audit.committed_tile_transaction_count >=
          whole.bridge_audit.committed_tile_transaction_count,
      label + ": a capped tile commits at least as many tile transactions");
  check(
      bounded.progress_audit.resolved_support_count ==
              whole.progress_audit.resolved_support_count &&
          bounded.progress_audit.remaining_frontier_support_count == 0U,
      label + ": the capped chain closes the same exact universe");
}

void test_bounded_tile_root_differentials() {
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
  // One root per tile is the finest granularity the contract allows and
  // the strongest form of the property.
  run_bounded_tile_root_differential(sphere, 3U, 1U, "tile-roots=1 sphere8/K3");
  run_bounded_tile_root_differential(sphere, 3U, 2U, "tile-roots=2 sphere8/K3");

  std::vector<CertifiedPoint3> line12;
  for (std::size_t index = 0U; index < 12U; ++index) {
    const double coordinate = static_cast<double>(index);
    line12.push_back(
        point(coordinate, coordinate / 8.0, coordinate / 64.0));
  }
  run_bounded_tile_root_differential(line12, 2U, 1U, "tile-roots=1 line12/K2");
}

void test_operational_guard_differentials() {
  const std::array<CertifiedPoint3, 4U> tetrahedron{
      point(1.0, 1.0, 1.0),
      point(-1.0, -1.0, 1.0),
      point(-1.0, 1.0, -1.0),
      point(1.0, -1.0, -1.0),
  };
  run_operational_guard_differential(
      tetrahedron, 1U, "guard tetrahedron/K1");

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
  run_operational_guard_differential(sphere, 3U, "guard sphere8/K3");
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
    test_tile_certified_assembly_differentials();
    test_bounded_tile_root_differentials();
    test_operational_guard_differentials();
    test_failed_assembly_publishes_its_transcript();
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
