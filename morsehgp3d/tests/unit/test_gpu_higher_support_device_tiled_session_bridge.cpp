#include "fake_gpu_higher_support_device_tiled_frontier_launchers.hpp"
#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/higher_support_device_tiled_session_bridge.hpp"

#include "phase15_higher_support_device_tiled_frontier_internal.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/support.hpp"
#include "morsehgp3d/hierarchy/higher_support_stream.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::HigherSupportDeviceTiledFrontierConfig;
using morsehgp3d::gpu::HigherSupportDeviceTiledFrontierContext;
using morsehgp3d::gpu::HigherSupportDeviceTiledFrontierTerminalPolicy;
using morsehgp3d::gpu::HigherSupportDeviceTiledSessionBridge;
using morsehgp3d::gpu::HigherSupportDeviceTiledSessionBridgeAdvance;
using morsehgp3d::gpu::HigherSupportDeviceTiledSessionBridgeConfig;
using morsehgp3d::gpu::HigherSupportDeviceTiledSessionBridgeStatus;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonLbvhDeviceTraversalLease;
using morsehgp3d::gpu::test_support::
    bind_fake_higher_support_device_tiled_geometry;
using morsehgp3d::gpu::test_support::
    FakeHigherSupportDeviceTiledFrontierCorruption;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_higher_support_device_tiled_frontier;
using morsehgp3d::gpu::test_support::reset_fake_gpu_phase14_morton_lbvh_build;
using morsehgp3d::gpu::test_support::
    set_fake_gpu_higher_support_device_tiled_frontier_corruption;
using morsehgp3d::hierarchy::build_exact_higher_support_stream;
using morsehgp3d::hierarchy::exact_higher_support_candidate_universe_size;
using morsehgp3d::hierarchy::ExactHigherSupportAuthorityContext;
using morsehgp3d::hierarchy::ExactHigherSupportEvent;
using morsehgp3d::hierarchy::ExactHigherSupportExtraShellDiagnostic;
using morsehgp3d::hierarchy::ExactHigherSupportStreamBudget;
using morsehgp3d::hierarchy::ExactHigherSupportStreamResult;
using morsehgp3d::hierarchy::ExactSparseDirectH0CandidateRunLimits;
using morsehgp3d::hierarchy::ExactSparseHigherSupportH0ChunkRunContext;
using morsehgp3d::hierarchy::ExactSparseHigherSupportH0ChunkRunLimits;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

using BigInt = morsehgp3d::exact::BigInt;
using BridgeStatus = HigherSupportDeviceTiledSessionBridgeStatus;
using Corruption = FakeHigherSupportDeviceTiledFrontierCorruption;
using Policy = HigherSupportDeviceTiledFrontierTerminalPolicy;

static_assert(
    !std::is_copy_constructible_v<HigherSupportDeviceTiledSessionBridge>);
static_assert(
    !std::is_move_constructible_v<HigherSupportDeviceTiledSessionBridge>);
static_assert(
    !std::is_copy_constructible_v<
        HigherSupportDeviceTiledSessionBridgeAdvance>);
static_assert(std::is_nothrow_move_constructible_v<
              HigherSupportDeviceTiledSessionBridgeAdvance>);
static_assert(
    HigherSupportDeviceTiledSessionBridge::deployment_status ==
    "architecture_only");
static_assert(
    HigherSupportDeviceTiledSessionBridge::public_status == "not_claimed");

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Exception, typename Function>
void check_throws(Function&& function, const std::string& message) {
  try {
    std::forward<Function>(function)();
  } catch (const Exception&) {
    return;
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: " << message
              << " (unexpected exception: " << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] CanonicalPointCloud cloud_from(
    const std::vector<CertifiedPoint3>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud line_cloud(std::size_t count) {
  std::vector<CertifiedPoint3> points;
  points.reserve(count);
  for (std::size_t index = 0U; index < count; ++index) {
    const double coordinate = static_cast<double>(index);
    points.push_back(
        point(coordinate, coordinate / 8.0, coordinate / 64.0));
  }
  return cloud_from(points);
}

[[nodiscard]] CanonicalPointCloud cluster_cloud() {
  const std::vector<CertifiedPoint3> points{
      point(0.0, 0.0, 0.0),
      point(0.25, 0.0, 0.0),
      point(0.0, 0.25, 0.0),
      point(0.0, 0.0, 0.25),
      point(100.0, 100.0, 100.0),
      point(100.5, 100.0, 100.0),
      point(100.0, 100.5, 100.0),
      point(-200.0, 50.0, 300.0),
      point(-199.0, 50.0, 300.0),
      point(-200.0, 51.0, 300.0)};
  return cloud_from(points);
}

[[nodiscard]] CanonicalPointCloud perturbed_sphere_cloud() {
  const std::vector<CertifiedPoint3> points{
      point(1.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0),
      point(0.0, -1.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(0.0, 0.0, -1.0),
      point(0.5773502691896258, 0.5773502691896258, 0.5773502691896257),
      point(-0.5773502691896258, -0.5773502691896258, 0.5773502691896258)};
  return cloud_from(points);
}

// Fourteen points mixing three separated scales and a near-cospherical
// cluster: the M2 differential extension beyond the M1 n<=12 coverage.
[[nodiscard]] CanonicalPointCloud mixed_cloud_14() {
  const std::vector<CertifiedPoint3> points{
      point(0.0, 0.0, 0.0),
      point(0.25, 0.0, 0.0),
      point(0.0, 0.25, 0.0),
      point(0.0, 0.0, 0.25),
      point(0.125, 0.125, 0.125),
      point(100.0, 100.0, 100.0),
      point(100.5, 100.0, 100.0),
      point(100.0, 100.5, 100.0),
      point(100.0, 100.0, 100.5),
      point(-200.0, 50.0, 300.0),
      point(-199.0, 50.0, 300.0),
      point(-200.0, 51.0, 300.0),
      point(-200.0, 50.0, 301.0),
      point(-199.5, 50.5, 300.5)};
  return cloud_from(points);
}

[[nodiscard]] ExactHigherSupportStreamBudget unlimited_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return ExactHigherSupportStreamBudget{
      maximum, maximum, maximum, maximum, maximum, maximum, maximum,
      maximum};
}

[[nodiscard]] MortonLbvhDeviceTraversalLease traversal_lease(
    const CanonicalPointCloud& cloud) {
  reset_fake_gpu_phase14_morton_lbvh_build();
  MortonLbvhBuildContext builder{cloud.size() + 2U};
  const auto build = builder.build(cloud);
  return builder.release_device_traversal_lease(build);
}

// A deliberately tiny initial probe: the deterministic doubling and binary
// search of the minimal work-unit boundary are exercised on every
// differential cloud, and every session commits several session-counted
// expansions followed by several whole-root tile transactions.
[[nodiscard]] HigherSupportDeviceTiledSessionBridgeConfig bridge_config() {
  HigherSupportDeviceTiledSessionBridgeConfig config;
  config.initial_work_unit_probe = 2U;
  config.maximum_work_unit_probe_doublings = 48U;
  config.pre_expansion_target_entry_count = 8U;
  return config;
}

struct BridgeRun {
  std::vector<ExactHigherSupportEvent> events;
  std::vector<ExactHigherSupportExtraShellDiagnostic> diagnostics;
  std::size_t committed_transactions{};
  std::size_t total_tile_roots{};
  std::size_t total_tile_slots{};
  morsehgp3d::gpu::HigherSupportDeviceTiledSessionBridgeAudit audit;
  bool terminal{false};
};

[[nodiscard]] BridgeRun run_bridge_to_terminal(
    HigherSupportDeviceTiledSessionBridge& bridge,
    const std::string& label) {
  BridgeRun run;
  for (std::size_t iteration = 0U; iteration < 10'000U; ++iteration) {
    auto advance = bridge.advance_one_tile_transaction();
    if (advance.status == BridgeStatus::session_terminal) {
      run.audit = advance.audit;
      run.terminal = true;
      return run;
    }
    check(
        advance.status == BridgeStatus::transaction_committed,
        label + ": every bridge transaction commits");
    if (advance.status != BridgeStatus::transaction_committed) {
      return run;
    }
    ++run.committed_transactions;
    run.total_tile_roots += advance.tile_root_count;
    run.total_tile_slots += advance.tile_slot_count;
    for (auto& event : advance.committed_events) {
      run.events.push_back(std::move(event));
    }
    for (auto& diagnostic : advance.committed_extra_shell_diagnostics) {
      run.diagnostics.push_back(std::move(diagnostic));
    }
    run.audit = advance.audit;
  }
  check(false, label + ": the bridge session did not reach terminal");
  return run;
}

void run_bridge_differential_case(
    const CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactSparseHigherSupportH0ChunkRunContext* wire_context,
    const std::string& label) {
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportAuthorityContext authority{
      index, cloud, requested_maximum_order};
  const ExactHigherSupportStreamResult oracle =
      build_exact_higher_support_stream(
          index, cloud, requested_maximum_order, unlimited_budget());
  check(oracle.stream_complete(), label + ": the CPU oracle completes");

  reset_fake_gpu_higher_support_device_tiled_frontier();
  auto lease = traversal_lease(cloud);
  bind_fake_higher_support_device_tiled_geometry(index, cloud);
  HigherSupportDeviceTiledSessionBridge bridge{
      authority, std::move(lease), bridge_config(), wire_context};
  const BridgeRun run = run_bridge_to_terminal(bridge, label);
  if (!run.terminal) {
    return;
  }
  check(
      bridge.session_terminal() &&
          bridge.trusted_checkpoint().locally_complete(),
      label + ": the anchored session reaches its terminal checkpoint");

  // Full-payload record equality: the concatenated committed transitions
  // reproduce the oracle stream bit for bit (events and extra-shell
  // diagnostics, including exact centers, levels and interiors, in the
  // exact deterministic order).
  // The from-scratch oracle sorts its records canonically while the
  // chunked anchored session preserves the exact emission order
  // (canonical_sort_records_), so the differential compares the two
  // streams as full-payload sets keyed by their canonical supports.
  const auto support_key = [](std::uint8_t support_size,
                              const std::array<PointId, 4>& ids) {
    return std::pair{support_size, ids};
  };
  {
    std::map<std::pair<std::uint8_t, std::array<PointId, 4>>,
             const ExactHigherSupportEvent*>
        bridge_events;
    for (const ExactHigherSupportEvent& event : run.events) {
      check(
          bridge_events
              .emplace(
                  support_key(event.support_size, event.support_ids),
                  &event)
              .second,
          label + ": a committed event support is unique");
    }
    bool events_equal = bridge_events.size() == oracle.events.size();
    for (const ExactHigherSupportEvent& event : oracle.events) {
      const auto found = bridge_events.find(
          support_key(event.support_size, event.support_ids));
      events_equal = events_equal && found != bridge_events.end() &&
          *found->second == event;
    }
    check(
        events_equal,
        label + ": the committed events equal the oracle events with "
                "their full exact payloads");
  }
  {
    std::map<std::pair<std::uint8_t, std::array<PointId, 4>>,
             const ExactHigherSupportExtraShellDiagnostic*>
        bridge_diagnostics;
    for (const ExactHigherSupportExtraShellDiagnostic& diagnostic :
         run.diagnostics) {
      check(
          bridge_diagnostics
              .emplace(
                  support_key(
                      diagnostic.support_size, diagnostic.support_ids),
                  &diagnostic)
              .second,
          label + ": a committed extra-shell diagnostic support is unique");
    }
    bool diagnostics_equal =
        bridge_diagnostics.size() ==
        oracle.relevant_extra_shell_diagnostics.size();
    for (const ExactHigherSupportExtraShellDiagnostic& diagnostic :
         oracle.relevant_extra_shell_diagnostics) {
      const auto found = bridge_diagnostics.find(
          support_key(diagnostic.support_size, diagnostic.support_ids));
      diagnostics_equal = diagnostics_equal &&
          found != bridge_diagnostics.end() && *found->second == diagnostic;
    }
    check(
        diagnostics_equal,
        label + ": the committed extra-shell diagnostics equal the oracle "
                "diagnostics with their full exact payloads");
  }

  // Exact BigInt accounting across all committed transactions.
  const BigInt universe =
      exact_higher_support_candidate_universe_size(cloud.size());
  check(
      run.audit.committed_universe_support_mass == universe &&
          run.audit.committed_session_resolved_support_mass == universe &&
          run.audit.committed_device_well_prune_support_mass +
                  run.audit.committed_device_rank_prune_support_mass +
                  run.audit.committed_device_terminal_support_mass ==
              universe,
      label + ": the committed device and session masses close the exact "
              "universe");
  check(
      run.committed_transactions >= 2U &&
          run.total_tile_slots >= run.total_tile_roots &&
          run.audit.committed_expansion_transition_count >= 1U &&
          run.audit.committed_tile_transaction_count >= 1U &&
          run.audit.committed_expansion_transition_count +
                  run.audit.committed_tile_transaction_count ==
              run.audit.committed_transaction_count &&
          run.audit.minimal_work_unit_boundary_theorem_applied &&
          run.audit.session_counted_canonical_pre_expansion &&
          run.audit.work_unit_probe_count >
              run.audit.committed_transaction_count,
      label + ": the session commits session-counted expansions followed "
              "by pre-expanded whole-root tile transactions");
  check(
      run.audit.tile_boundary_prefix_criterion_enforced &&
          run.audit.sealed_largest_mass_pre_expansion_v1 &&
          run.audit.pre_expansion_partition_bigint_verified &&
          run.audit.device_and_session_masses_cross_validated &&
          run.audit.per_record_prune_replay_enforced &&
          run.audit.terminal_geometry_replay_enforced &&
          run.audit.induction_identity_reverified_each_commit &&
          run.audit.strict_interior_carrier_rejected_at_construction &&
          !run.audit.bigint_support_mass_transferred &&
          !run.audit.floating_point_decision_performed &&
          !run.audit.slo_claimed && !run.audit.public_status_claimed,
      label + ": the closing bridge audit keeps its proof flags");
  if (!run.events.empty()) {
    check(
        run.audit.bidirectional_event_equality_enforced &&
            run.audit.host_closed_ball_classification_performed,
        label + ": event equality and host classification were enforced");
  }
  if (wire_context != nullptr && !run.events.empty()) {
    check(
        run.audit.wire_caps_validated &&
            run.audit.wire_round_trip_count >= 4U * run.events.size() &&
            run.audit.maximum_wire_numerator_byte_count <=
                morsehgp3d::hierarchy::
                    sparse_higher_support_tetrahedron_exact_integer_byte_count &&
            run.audit.maximum_wire_denominator_byte_count <=
                morsehgp3d::hierarchy::
                    sparse_higher_support_tetrahedron_exact_integer_byte_count,
        label + ": every committed event round-trips the sealed exact "
                "wire under the 6973/9503 caps");
  }
}

// ---------------------------------------------------------------------------
// 1. Scientific bridge differential against the exact CPU stream oracle,
// extended to n <= 14.
// ---------------------------------------------------------------------------

void test_bridge_differential_matches_exact_stream() {
  const std::array<std::size_t, 2> orders{3U, 5U};
  {
    const CanonicalPointCloud cloud = line_cloud(12U);
    for (const std::size_t order : orders) {
      run_bridge_differential_case(
          cloud, order, nullptr, "line12/K=" + std::to_string(order));
    }
  }
  {
    const CanonicalPointCloud cloud = cluster_cloud();
    for (const std::size_t order : orders) {
      run_bridge_differential_case(
          cloud, order, nullptr, "clusters10/K=" + std::to_string(order));
    }
  }
  {
    const CanonicalPointCloud cloud = perturbed_sphere_cloud();
    for (const std::size_t order : orders) {
      run_bridge_differential_case(
          cloud, order, nullptr, "sphere8/K=" + std::to_string(order));
    }
  }
  {
    const CanonicalPointCloud cloud = mixed_cloud_14();
    const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
    const ExactHigherSupportAuthorityContext authority{index, cloud, 5U};
    const ExactSparseHigherSupportH0ChunkRunContext wire_context{
        authority,
        ExactHigherSupportStreamBudget{
            1U, 64U, 64U, 8U, 128U, 32U, 32U, 256U},
        ExactSparseDirectH0CandidateRunLimits{16U, 16U, 256U},
        ExactSparseHigherSupportH0ChunkRunLimits{
            256U, 9503U, 64U * 9503U, 1048576U}};
    run_bridge_differential_case(cloud, 5U, &wire_context, "mixed14/K=5");
  }
  {
    const CanonicalPointCloud cloud = line_cloud(14U);
    run_bridge_differential_case(cloud, 5U, nullptr, "line14/K=5");
  }
}

// ---------------------------------------------------------------------------
// 2. Fail-closed configuration surface.
// ---------------------------------------------------------------------------

void test_bridge_rejects_invalid_configurations() {
  const CanonicalPointCloud cloud = line_cloud(8U);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportAuthorityContext authority{index, cloud, 3U};

  const auto try_config =
      [&](HigherSupportDeviceTiledSessionBridgeConfig config,
          const std::string& message) {
        reset_fake_gpu_higher_support_device_tiled_frontier();
        auto lease = traversal_lease(cloud);
        bind_fake_higher_support_device_tiled_geometry(index, cloud);
        check_throws<std::invalid_argument>(
            [&authority, &lease, &config] {
              HigherSupportDeviceTiledSessionBridge bridge{
                  authority, std::move(lease), config};
            },
            message);
      };

  HigherSupportDeviceTiledSessionBridgeConfig carrier = bridge_config();
  carrier.frontier_config.terminal_policy =
      Policy::strict_interior_carrier_q3;
  try_config(
      carrier,
      "the bridge rejects the strict-interior carrier policy: the "
      "anchored induction has no carrier semantics");

  HigherSupportDeviceTiledSessionBridgeConfig zero_probe = bridge_config();
  zero_probe.initial_work_unit_probe = 0U;
  try_config(zero_probe, "the bridge rejects a zero work-unit probe");

  HigherSupportDeviceTiledSessionBridgeConfig no_doubling = bridge_config();
  no_doubling.maximum_work_unit_probe_doublings = 0U;
  try_config(
      no_doubling, "the bridge rejects a zero probe doubling allowance");

  HigherSupportDeviceTiledSessionBridgeConfig zero_backstop =
      bridge_config();
  zero_backstop.maximum_session_pre_expansion_transition_count = 0U;
  try_config(
      zero_backstop,
      "the bridge rejects a zero session pre-expansion backstop");

  HigherSupportDeviceTiledSessionBridgeConfig zero_target = bridge_config();
  zero_target.pre_expansion_target_entry_count = 0U;
  try_config(zero_target, "the bridge rejects a zero pre-expansion target");

  HigherSupportDeviceTiledSessionBridgeConfig oversized_target =
      bridge_config();
  oversized_target.pre_expansion_target_entry_count =
      oversized_target.frontier_config.slot_tile_capacity + 1U;
  try_config(
      oversized_target,
      "the bridge rejects a pre-expansion target beyond the slot tile "
      "capacity");
  reset_fake_gpu_higher_support_device_tiled_frontier();
}

// ---------------------------------------------------------------------------
// 3. A censored tile never commits: the anchored checkpoint is retained,
// and the idempotent retry after an engine rebind reproduces the identical
// transitions.
// ---------------------------------------------------------------------------

void test_bridge_censure_never_commits_and_retry_reproduces() {
  const CanonicalPointCloud cloud = cluster_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportAuthorityContext authority{index, cloud, 5U};

  // Reference run without any corruption.
  reset_fake_gpu_higher_support_device_tiled_frontier();
  auto reference_lease = traversal_lease(cloud);
  bind_fake_higher_support_device_tiled_geometry(index, cloud);
  HigherSupportDeviceTiledSessionBridge reference_bridge{
      authority, std::move(reference_lease), bridge_config()};
  const BridgeRun reference =
      run_bridge_to_terminal(reference_bridge, "censure-reference");
  check(
      reference.terminal && reference.committed_transactions >= 2U,
      "censure-reference: the reference session resolves through at least "
      "two transactions");

  // Interrupted run: commit one transaction, censor the second, retry.
  reset_fake_gpu_higher_support_device_tiled_frontier();
  auto lease = traversal_lease(cloud);
  bind_fake_higher_support_device_tiled_geometry(index, cloud);
  HigherSupportDeviceTiledSessionBridge bridge{
      authority, std::move(lease), bridge_config()};
  std::vector<ExactHigherSupportEvent> events;
  std::vector<ExactHigherSupportExtraShellDiagnostic> diagnostics;
  auto first = bridge.advance_one_tile_transaction();
  check(
      first.status == BridgeStatus::transaction_committed,
      "censure: the first transaction commits before the corruption");
  for (auto& event : first.committed_events) {
    events.push_back(std::move(event));
  }
  for (auto& diagnostic : first.committed_extra_shell_diagnostics) {
    diagnostics.push_back(std::move(diagnostic));
  }

  // Session-counted expansion transitions never touch the device engine,
  // so the armed corruption censors the first genuine tile transaction.
  set_fake_gpu_higher_support_device_tiled_frontier_corruption(
      Corruption::broken_partition);
  auto checkpoint_before_censure = bridge.trusted_checkpoint();
  bool censored_seen = false;
  for (std::size_t iteration = 0U; iteration < 1'000U && !censored_seen;
       ++iteration) {
    checkpoint_before_censure = bridge.trusted_checkpoint();
    auto attempt = bridge.advance_one_tile_transaction();
    if (attempt.status == BridgeStatus::censored_without_commit) {
      check(
          attempt.committed_events.empty() &&
              attempt.audit.censored_without_commit_count == 1U &&
              attempt.audit.no_commit_performed_on_censure,
          "censure: a corrupted device tile is censored without any "
          "commit");
      censored_seen = true;
      break;
    }
    check(
        attempt.status == BridgeStatus::transaction_committed &&
            attempt.expansion_transition,
        "censure: only device-free expansion transitions commit while "
        "the corruption is armed");
    for (auto& event : attempt.committed_events) {
      events.push_back(std::move(event));
    }
    for (auto& diagnostic : attempt.committed_extra_shell_diagnostics) {
      diagnostics.push_back(std::move(diagnostic));
    }
  }
  check(censored_seen, "censure: the corrupted tile transaction censors");
  check(
      bridge.trusted_checkpoint() == checkpoint_before_censure,
      "censure: the anchored checkpoint Q_j is retained unchanged");
  check(
      !bridge.poisoned() && bridge.frontier_context_rebind_required(),
      "censure: the bridge survives and demands a fresh engine rebind");
  check_throws<std::runtime_error>(
      [&bridge] { (void)bridge.advance_one_tile_transaction(); },
      "censure: no further transaction runs before the engine rebind");
  check(
      bridge.trusted_checkpoint() == checkpoint_before_censure,
      "censure: the refused transaction leaves Q_j untouched");

  // Idempotent retry over a fresh engine reproduces the same transitions.
  reset_fake_gpu_higher_support_device_tiled_frontier();
  auto retry_lease = traversal_lease(cloud);
  bind_fake_higher_support_device_tiled_geometry(index, cloud);
  bridge.rebind_frontier_context(std::move(retry_lease));
  check(
      !bridge.frontier_context_rebind_required(),
      "censure: the rebind restores the transaction path");
  const BridgeRun resumed = run_bridge_to_terminal(bridge, "censure-retry");
  check(
      resumed.terminal,
      "censure-retry: the resumed session reaches terminal");
  for (const auto& event : resumed.events) {
    events.push_back(event);
  }
  for (const auto& diagnostic : resumed.diagnostics) {
    diagnostics.push_back(diagnostic);
  }
  check(
      events == reference.events &&
          diagnostics == reference.diagnostics,
      "censure-retry: the interrupted and reference sessions commit "
      "identical transition records");
  reset_fake_gpu_higher_support_device_tiled_frontier();
}

// ---------------------------------------------------------------------------
// 4. Terminal idempotence.
// ---------------------------------------------------------------------------

void test_bridge_terminal_is_idempotent() {
  const CanonicalPointCloud cloud = line_cloud(6U);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportAuthorityContext authority{index, cloud, 3U};
  reset_fake_gpu_higher_support_device_tiled_frontier();
  auto lease = traversal_lease(cloud);
  bind_fake_higher_support_device_tiled_geometry(index, cloud);
  HigherSupportDeviceTiledSessionBridge bridge{
      authority, std::move(lease), bridge_config()};
  const BridgeRun run = run_bridge_to_terminal(bridge, "terminal");
  check(run.terminal, "terminal: the session terminates");
  for (std::size_t repeat = 0U; repeat < 3U; ++repeat) {
    auto advance = bridge.advance_one_tile_transaction();
    check(
        advance.status == BridgeStatus::session_terminal &&
            advance.audit.session_terminal_reached,
        "terminal: a terminal session bridge stays terminal");
  }
  reset_fake_gpu_higher_support_device_tiled_frontier();
}

// ---------------------------------------------------------------------------
// Minimal JSON extraction helpers for the named regression fixtures.  The
// fixtures store integer coordinates and integer id lists; anything else in
// the files is ignored.  Parsing fails closed on missing keys.
// ---------------------------------------------------------------------------

[[nodiscard]] std::string read_fixture(const char* path) {
  std::ifstream stream{path};
  if (!stream) {
    throw std::runtime_error(
        std::string{"a named fixture is unreadable: "} + path);
  }
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

[[nodiscard]] std::size_t find_key(
    const std::string& text,
    const std::string& key,
    std::size_t from) {
  const std::size_t at = text.find("\"" + key + "\"", from);
  if (at == std::string::npos) {
    throw std::runtime_error("a named fixture misses the key " + key);
  }
  return at;
}

// Returns the [begin, end) span of the bracketed array value that follows
// the given key occurrence, honouring nested brackets.
[[nodiscard]] std::pair<std::size_t, std::size_t> array_span_after(
    const std::string& text,
    std::size_t key_position) {
  const std::size_t open = text.find('[', key_position);
  if (open == std::string::npos) {
    throw std::runtime_error("a named fixture key has no array value");
  }
  std::size_t depth = 0U;
  for (std::size_t at = open; at < text.size(); ++at) {
    if (text[at] == '[') {
      ++depth;
    } else if (text[at] == ']') {
      --depth;
      if (depth == 0U) {
        return {open, at + 1U};
      }
    }
  }
  throw std::runtime_error("a named fixture array is unterminated");
}

[[nodiscard]] std::vector<long long> integers_in(
    const std::string& text,
    std::size_t begin,
    std::size_t end) {
  std::vector<long long> values;
  std::size_t at = begin;
  while (at < end) {
    const char symbol = text[at];
    if (symbol == '-' ||
        (std::isdigit(static_cast<unsigned char>(symbol)) != 0)) {
      std::size_t consumed = 0U;
      values.push_back(std::stoll(text.substr(at, 32U), &consumed));
      at += consumed;
    } else {
      ++at;
    }
  }
  return values;
}

[[nodiscard]] std::vector<std::array<long long, 3>> integer_triples_after(
    const std::string& text,
    const std::string& key,
    std::size_t from) {
  const auto span = array_span_after(text, find_key(text, key, from));
  const std::vector<long long> flat =
      integers_in(text, span.first, span.second);
  if (flat.empty() || flat.size() % 3U != 0U) {
    throw std::runtime_error(
        "a named fixture point list is not a list of integer triples");
  }
  std::vector<std::array<long long, 3>> triples;
  triples.reserve(flat.size() / 3U);
  for (std::size_t at = 0U; at + 2U < flat.size(); at += 3U) {
    triples.push_back({flat[at], flat[at + 1U], flat[at + 2U]});
  }
  return triples;
}

[[nodiscard]] CanonicalPointCloud cloud_from_triples(
    const std::vector<std::array<long long, 3>>& triples) {
  std::vector<CertifiedPoint3> points;
  points.reserve(triples.size());
  for (const auto& triple : triples) {
    points.push_back(point(
        static_cast<double>(triple[0]),
        static_cast<double>(triple[1]),
        static_cast<double>(triple[2])));
  }
  return cloud_from(points);
}

// The canonical cloud reorders its input, so fixture point ids must be
// remapped through the exact canonical input bits.
[[nodiscard]] PointId canonical_id_of_triple(
    const CanonicalPointCloud& cloud,
    const std::array<long long, 3>& triple) {
  const CertifiedPoint3 target = point(
      static_cast<double>(triple[0]),
      static_cast<double>(triple[1]),
      static_cast<double>(triple[2]));
  for (PointId id = 0U; id < cloud.size(); ++id) {
    if (cloud.point(id).canonical_input_bits() ==
        target.canonical_input_bits()) {
      return id;
    }
  }
  throw std::runtime_error(
      "a fixture point is missing from its canonical cloud");
}

[[nodiscard]] bool boolean_after(
    const std::string& text,
    const std::string& key,
    std::size_t from,
    std::size_t before) {
  const std::size_t at = find_key(text, key, from);
  if (at >= before) {
    throw std::runtime_error(
        "a named fixture key escapes its case span: " + key);
  }
  const std::size_t true_at = text.find("true", at);
  const std::size_t false_at = text.find("false", at);
  return true_at != std::string::npos &&
      (false_at == std::string::npos || true_at < false_at);
}

// ---------------------------------------------------------------------------
// 5. Named fixture: the Hartigan triangle whose miniball event exists even
// though every side rank lies above K, driven through the session bridge.
// ---------------------------------------------------------------------------

void test_hartigan_triangle_fixture_through_bridge() {
  const std::string text =
      read_fixture(MORSEHGP3D_HARTIGAN_TRIANGLE_SIDE_RANK_FIXTURE);
  const auto triples = integer_triples_after(text, "points", 0U);
  const CanonicalPointCloud cloud = cloud_from_triples(triples);
  const auto order_key = find_key(text, "order", 0U);
  const std::vector<long long> order_values =
      integers_in(text, order_key, text.find(',', order_key));
  const auto rank_key = find_key(text, "target_closed_rank", 0U);
  const std::vector<long long> rank_values =
      integers_in(text, rank_key, text.find(',', rank_key));
  const auto support_span =
      array_span_after(text, find_key(text, "target_support_point_ids", 0U));
  const std::vector<long long> support_ids =
      integers_in(text, support_span.first, support_span.second);
  if (order_values.empty() || rank_values.empty() ||
      support_ids.size() != 3U) {
    throw std::runtime_error("the Hartigan fixture shape changed");
  }
  const auto order = static_cast<std::size_t>(order_values.front());
  const auto target_rank = static_cast<std::size_t>(rank_values.front());

  run_bridge_differential_case(cloud, order, nullptr, "hartigan/K=2");

  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportAuthorityContext authority{index, cloud, order};
  reset_fake_gpu_higher_support_device_tiled_frontier();
  auto lease = traversal_lease(cloud);
  bind_fake_higher_support_device_tiled_geometry(index, cloud);
  HigherSupportDeviceTiledSessionBridge bridge{
      authority, std::move(lease), bridge_config()};
  const BridgeRun run = run_bridge_to_terminal(bridge, "hartigan");
  std::array<PointId, 4> target{};
  for (std::size_t at = 0U; at < 3U; ++at) {
    target[at] = canonical_id_of_triple(
        cloud, triples[static_cast<std::size_t>(support_ids[at])]);
  }
  std::sort(target.begin(), target.begin() + 3);
  bool target_event_found = false;
  for (const ExactHigherSupportEvent& event : run.events) {
    if (event.support_size == 3U && event.support_ids == target &&
        event.closed_rank == target_rank) {
      target_event_found = true;
    }
  }
  check(
      run.terminal && target_event_found,
      "hartigan: the target triangle survives the tiled path as an event "
      "at its certified closed rank although every side rank lies above "
      "K");
  reset_fake_gpu_higher_support_device_tiled_frontier();
}

// ---------------------------------------------------------------------------
// 6. Named fixture: tetrahedron face-filter counterexamples.  Support-4
// admissibility is decided by the exact circumball alone, never by face
// acuteness, so each fixture case must surface (or withhold) its support-4
// event through the bridge accordingly.
// ---------------------------------------------------------------------------

void test_tetrahedron_face_filter_fixture_through_bridge() {
  const std::string text =
      read_fixture(MORSEHGP3D_TETRAHEDRON_FACE_FILTER_FIXTURE);
  std::size_t cursor = find_key(text, "cases", 0U);
  std::size_t case_index = 0U;
  while (true) {
    const std::size_t case_at = text.find("\"case_id\"", cursor);
    if (case_at == std::string::npos) {
      break;
    }
    const std::size_t next_case = text.find("\"case_id\"", case_at + 1U);
    const std::size_t case_end =
        next_case == std::string::npos ? text.size() : next_case;
    const auto triples = integer_triples_after(text, "points", case_at);
    if (triples.size() != 4U) {
      throw std::runtime_error(
          "a tetrahedron fixture case is not four points");
    }
    const bool support4_admissible =
        boolean_after(text, "support4_admissible", case_at, case_end);
    const CanonicalPointCloud cloud = cloud_from_triples(triples);
    const std::string label =
        "tetra-face-filter[" + std::to_string(case_index) + "]";

    run_bridge_differential_case(cloud, 5U, nullptr, label);

    const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
    const ExactHigherSupportAuthorityContext authority{index, cloud, 5U};
    reset_fake_gpu_higher_support_device_tiled_frontier();
    auto lease = traversal_lease(cloud);
    bind_fake_higher_support_device_tiled_geometry(index, cloud);
    HigherSupportDeviceTiledSessionBridge bridge{
        authority, std::move(lease), bridge_config()};
    const BridgeRun run = run_bridge_to_terminal(bridge, label);
    bool support4_event_found = false;
    for (const ExactHigherSupportEvent& event : run.events) {
      if (event.support_size == 4U) {
        support4_event_found = true;
      }
    }
    check(
        run.terminal && support4_event_found == support4_admissible,
        label + ": the support-4 event presence matches the exact "
                "circumball admissibility, not the face filter");
    cursor = case_end;
    ++case_index;
  }
  check(
      case_index >= 2U,
      "tetra-face-filter: the fixture supplies at least two cases");
  reset_fake_gpu_higher_support_device_tiled_frontier();
}

// ---------------------------------------------------------------------------
// 7. Named fixture: the Gabriel strict-interior/extra-shell carrier matrix.
// The bridge itself rejects the carrier policy, so each case cloud is driven
// through the M1 carrier tile: carried simplices must survive to terminal
// records, and the fixture's exact interior/shell expectations are
// revalidated with the host closed-ball primitives.
// ---------------------------------------------------------------------------

struct FixtureSupportKey {
  std::uint8_t support_size{};
  std::array<PointId, 4> support_ids{};

  friend bool operator==(
      const FixtureSupportKey&, const FixtureSupportKey&) = default;
  friend bool operator<(
      const FixtureSupportKey& lhs, const FixtureSupportKey& rhs) {
    if (lhs.support_size != rhs.support_size) {
      return lhs.support_size < rhs.support_size;
    }
    return lhs.support_ids < rhs.support_ids;
  }
};

// A carried simplex need not be a minimal circumball support (a right
// triangle carrying a Gabriel pair is boundary-reduced), so the interior
// check only applies when the exact analysis is minimal.
[[nodiscard]] std::optional<std::pair<std::size_t, std::size_t>>
triangle_interior_and_shell(
    const CanonicalPointCloud& cloud,
    const std::array<PointId, 4>& support_ids) {
  std::array<morsehgp3d::exact::ExactRational3, 3U> support{};
  for (std::size_t at = 0U; at < 3U; ++at) {
    support[at] = cloud.point(support_ids[at]).exact();
  }
  const auto analysis =
      morsehgp3d::exact::analyze_circumcenter_support(support);
  if (analysis.status() !=
      morsehgp3d::exact::CircumcenterSupportStatus::minimal) {
    return std::nullopt;
  }
  const auto& sphere = analysis.circumcenter_result();
  std::size_t interior = 0U;
  std::size_t shell = 0U;
  for (PointId id = 0U; id < cloud.size(); ++id) {
    const auto classification = morsehgp3d::exact::classify_sphere_point(
        *sphere.center(), *sphere.squared_level(), cloud.point(id));
    if (classification.location() ==
        morsehgp3d::exact::SpherePointLocation::strictly_inside) {
      ++interior;
    } else if (
        classification.location() ==
        morsehgp3d::exact::SpherePointLocation::boundary) {
      ++shell;
    }
  }
  return std::pair{interior, shell};
}

void test_gabriel_carrier_matrix_fixture() {
  const std::string text =
      read_fixture(MORSEHGP3D_GABRIEL_CARRIER_MATRIX_FIXTURE);
  std::size_t cursor = find_key(text, "cases", 0U);
  std::size_t case_index = 0U;
  while (true) {
    const std::size_t case_at = text.find("\"case_id\"", cursor);
    if (case_at == std::string::npos) {
      break;
    }
    const std::size_t next_case = text.find("\"case_id\"", case_at + 1U);
    const std::size_t case_end =
        next_case == std::string::npos ? text.size() : next_case;
    const auto triples = integer_triples_after(text, "points", case_at);
    const CanonicalPointCloud cloud = cloud_from_triples(triples);
    const std::string label =
        "gabriel-carrier[" + std::to_string(case_index) + "]";

    const auto carried_span = array_span_after(
        text, find_key(text, "carried_simplex_ids", case_at));
    const std::vector<long long> carried_flat =
        integers_in(text, carried_span.first, carried_span.second);
    if (carried_flat.size() % 3U != 0U) {
      throw std::runtime_error(
          label + ": carried simplices are not triangles");
    }
    std::set<FixtureSupportKey> carried;
    for (std::size_t at = 0U; at + 2U < carried_flat.size(); at += 3U) {
      FixtureSupportKey key;
      key.support_size = 3U;
      std::array<PointId, 3> ids{
          canonical_id_of_triple(
              cloud,
              triples[static_cast<std::size_t>(carried_flat[at])]),
          canonical_id_of_triple(
              cloud,
              triples[static_cast<std::size_t>(carried_flat[at + 1U])]),
          canonical_id_of_triple(
              cloud,
              triples[static_cast<std::size_t>(carried_flat[at + 2U])])};
      std::sort(ids.begin(), ids.end());
      for (std::size_t axis = 0U; axis < 3U; ++axis) {
        key.support_ids[axis] = ids[axis];
      }
      carried.insert(key);
    }

    // Every carried triangle with a minimal circumball must be
    // strictly-interior free: the exact closed-ball primitives revalidate
    // the fixture's expectation.  Boundary-reduced carriers (right
    // triangles on the pair diameter) have no circumball of their own.
    for (const FixtureSupportKey& key : carried) {
      const auto counts = triangle_interior_and_shell(cloud, key.support_ids);
      check(
          !counts.has_value() || counts->first == 0U,
          label + ": a carried triangle has no strict interior witness");
    }

    // Carrier tile over the support-3 universe of the case cloud.
    const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
    reset_fake_gpu_higher_support_device_tiled_frontier();
    auto lease = traversal_lease(cloud);
    bind_fake_higher_support_device_tiled_geometry(index, cloud);
    HigherSupportDeviceTiledFrontierConfig config;
    config.terminal_policy = Policy::strict_interior_carrier_q3;
    config.maximum_relevant_closed_rank = 4U;
    config.certified_maximum_lbvh_depth =
        index.build_counters().maximum_depth;
    HigherSupportDeviceTiledFrontierContext context{
        std::move(lease), config};
    morsehgp3d::hierarchy::ExactHigherSupportFrontierEntry root{};
    root.support_size = 3U;
    root.group_count = 1U;
    root.groups[0] = morsehgp3d::hierarchy::ExactHigherSupportNodeGroup{
        static_cast<std::uint64_t>(index.build_counters().node_count - 1U),
        0U,
        static_cast<std::uint64_t>(cloud.size()),
        3U};
    context.open_tile(
        std::span<const morsehgp3d::hierarchy::
                      ExactHigherSupportFrontierEntry>{&root, 1U});
    std::set<FixtureSupportKey> terminals;
    BigInt universe{0};
    BigInt resolved{0};
    BigInt unresolved{1};
    bool completed = false;
    for (std::size_t iteration = 0U; iteration < 10'000U; ++iteration) {
      auto advance = context.advance();
      if (advance.record_drain.has_value()) {
        const auto views = morsehgp3d::gpu::detail::
            Phase15HigherSupportDeviceTiledRecordDrainPrivateViewAccess::
                inspect(*advance.record_drain);
        for (std::size_t slot = views.slot_begin; slot < views.slot_end;
             ++slot) {
          const auto& control = views.slot_controls[slot];
          for (std::size_t at = 0U;
               at < static_cast<std::size_t>(control.terminal_record_count);
               ++at) {
            const auto& record = views.terminal_records
                [slot * views.terminal_record_stride + at];
            FixtureSupportKey key;
            key.support_size = record.support_size;
            for (std::size_t axis = 0U; axis < 4U; ++axis) {
              key.support_ids[axis] =
                  static_cast<PointId>(record.point_ids[axis]);
            }
            terminals.insert(key);
          }
        }
        advance.record_drain.reset();
      }
      if (advance.status ==
          morsehgp3d::gpu::HigherSupportDeviceTiledFrontierStatus::
              frontier_tile_complete) {
        universe = advance.audit.tile_universe_support_mass;
        resolved = advance.audit.cumulative_resolved_support_mass;
        unresolved = advance.audit.unresolved_support_mass;
        completed = true;
        break;
      }
      if (advance.status !=
          morsehgp3d::gpu::HigherSupportDeviceTiledFrontierStatus::
              chunk_ready) {
        break;
      }
    }
    check(
        completed && unresolved == 0 && resolved == universe,
        label + ": the carrier tile closes its support-3 universe");
    // A carried triangle with its own minimal circumball reaches terminal
    // classification under the carrier policy (no strict interior witness
    // can prune it); a boundary-reduced carrier is a universal well prune
    // and never becomes a terminal record.
    bool all_minimal_carried_terminal = true;
    for (const FixtureSupportKey& key : carried) {
      const auto counts = triangle_interior_and_shell(cloud, key.support_ids);
      if (!counts.has_value()) {
        continue;
      }
      all_minimal_carried_terminal =
          all_minimal_carried_terminal && terminals.count(key) != 0U;
    }
    check(
        all_minimal_carried_terminal,
        label + ": every minimal carried triangle survives the carrier "
                "policy to a terminal record");
    cursor = case_end;
    ++case_index;
  }
  check(
      case_index >= 4U,
      "gabriel-carrier: the fixture supplies its four documented cases");
  reset_fake_gpu_higher_support_device_tiled_frontier();
}

}  // namespace

int main() {
  try {
    test_bridge_differential_matches_exact_stream();
    test_bridge_rejects_invalid_configurations();
    test_bridge_censure_never_commits_and_retry_reproduces();
    test_bridge_terminal_is_idempotent();
    test_hartigan_triangle_fixture_through_bridge();
    test_tetrahedron_face_filter_fixture_through_bridge();
    test_gabriel_carrier_matrix_fixture();
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: unexpected exception: " << error.what() << '\n';
  }
  if (failures != 0) {
    std::cerr << failures << " failure(s)\n";
    return 1;
  }
  std::cout
      << "morsehgp3d gpu higher-support device tiled session bridge tests "
         "passed\n";
  return 0;
}
