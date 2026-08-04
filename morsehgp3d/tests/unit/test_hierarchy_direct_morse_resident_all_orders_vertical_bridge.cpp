#include "morsehgp3d/hierarchy/direct_morse_resident_all_orders_vertical_bridge.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <utility>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;

static_assert(
    direct_morse_resident_all_orders_vertical_bridge_schema_version == 3U);
static_assert(!std::is_copy_constructible_v<
              ExactDirectMorseResidentAllOrdersVerticalBridge>);
static_assert(std::is_nothrow_move_constructible_v<
              ExactDirectMorseResidentAllOrdersVerticalBridge>);
static_assert(!std::is_copy_constructible_v<
              ExactDirectMorseResidentAllOrdersVerticalPreparedBatch>);

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] ExactPairSupportStreamBudget unlimited_pair_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum, maximum, maximum, maximum};
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
      maximum,
  };
}

[[nodiscard]] ExactDirectSaddleArmSeedBudget unlimited_arm_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectClosedSaddleIncidenceBudget
unlimited_incidence_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceBudget
generous_successive_budget() {
  return {
      1024U,
      65536U,
      65536U,
      1048576U,
      65536U,
      1048576U,
      16777216U,
      65536U,
      65536U,
  };
}

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceStarJournalBudget
generous_star_budget() {
  return {
      1024U,
      11264U,
      11264U,
      112640U,
      131072U,
      131072U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      8388608U,
      generous_successive_budget(),
  };
}

[[nodiscard]] ExactDirectSparseUnifiedLevelPlanBudget generous_plan_budget() {
  return {
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      8388608U,
      8388608U,
      1048576U,
      8388608U,
      1048576U,
      1048576U,
      1048576U,
      16777216U,
  };
}

[[nodiscard]] ExactDirectFrozenUnifiedIncidenceBatchBudget
unlimited_frozen_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum, maximum, maximum, maximum, maximum, maximum,
      maximum, maximum, maximum, maximum, maximum, maximum,
      maximum, maximum, maximum, maximum, maximum, maximum,
      maximum, maximum, maximum, maximum,
  };
}

[[nodiscard]] ExactDirectMorseUnifiedResidentSessionBudget
generous_resident_budget() {
  ExactDirectMorseUnifiedResidentSessionBudget budget{};
  budget.locator = {
      1024U, 1024U, 10240U, 1024U, 1024U, 1024U,
      1024U, 1024U, 10240U, 4097U, 4097U,
  };
  budget.probe = {4097U, 1024U};
  budget.frozen_batch = unlimited_frozen_budget();
  budget.maximum_facet_resolution_count = 1024U;
  budget.maximum_prior_root_coverage_count = 1024U;
  budget.maximum_prior_root_coverage_point_reference_count = 10240U;
  budget.maximum_latent_carrier_coverage_count = 1024U;
  budget.maximum_latent_carrier_coverage_point_reference_count = 10240U;
  budget.maximum_fresh_facet_miniball_count = 1024U;
  budget.maximum_fresh_facet_miniball_support_enumeration_count = 1048576U;
  budget.maximum_normalized_facet_observation_scratch_count = 1024U;
  budget.maximum_normalized_strict_facet_scratch_count = 1024U;
  budget.maximum_normalized_strict_facet_simultaneous_scratch_entry_count =
      16384U;
  budget.maximum_resident_root_count = 1024U;
  budget.maximum_resident_root_point_reference_count = 10240U;
  budget.maximum_resident_component_latent_point_reference_count = 10240U;
  budget.maximum_group_record_count = 1024U;
  budget.maximum_group_child_reference_count = 1024U;
  budget.maximum_group_coverage_delta_point_reference_count = 10240U;
  budget.sparse_delta = {
      1024U, 10240U, 1024U, 1024U, 10240U,
      1024U, 10240U, 10240U, 8U,
  };
  return budget;
}

struct DirectSources {
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult event_journal;
  ExactDirectSaddleArmSeedBudget arm_budget;
  ExactDirectSaddleArmSeedJournalResult arm_journal;
  ExactDirectClosedSaddleIncidenceBudget incidence_budget;
  ExactDirectClosedSaddleIncidenceJournalResult incidence_journal;
};

[[nodiscard]] DirectSources direct_sources(
    const CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order) {
  const ExactDirectSupportTerminalBudget terminal_budget{
      unlimited_pair_budget(), unlimited_higher_budget()};
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto pair = build_exact_pair_support_stream(
      index, cloud, requested_maximum_order, terminal_budget.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, requested_maximum_order, terminal_budget.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index,
      cloud,
      requested_maximum_order,
      terminal_budget,
      pair,
      higher);
  auto event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  const auto arm_budget = unlimited_arm_budget();
  auto arm_journal = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, event_journal, arm_budget);
  const auto incidence_budget = unlimited_incidence_budget();
  auto incidence_journal =
      build_exact_direct_closed_saddle_incidence_journal(
          cloud,
          facade,
          event_journal,
          arm_budget,
          arm_journal,
          incidence_budget);
  return {
      std::move(facade),
      std::move(event_journal),
      arm_budget,
      std::move(arm_journal),
      incidence_budget,
      std::move(incidence_journal),
  };
}

struct Fixture {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  DirectSources source;
  ExactDirectSparseSuccessiveIncidenceStarJournalBudget star_budget;
  ExactDirectSparseSuccessiveIncidenceStarJournalResult star;
  ExactDirectSparseUnifiedLevelPlanBudget plan_budget;
  ExactDirectSparseUnifiedLevelPlanResult plan;
  K1ExactBoruvkaResult boruvka;
};

[[nodiscard]] Fixture fixture_from_points(
    const std::span<const CertifiedPoint3> points,
    const std::size_t requested_maximum_order) {
  auto cloud = CanonicalPointCloud::rejecting_duplicates(
      points);
  auto index = MortonLbvhIndex::build(cloud);
  auto source = direct_sources(cloud, requested_maximum_order);
  auto star_budget = generous_star_budget();
  auto star = build_exact_direct_sparse_successive_incidence_star_journal(
      index,
      cloud,
      source.facade,
      source.event_journal,
      source.arm_budget,
      source.arm_journal,
      source.incidence_budget,
      source.incidence_journal,
      star_budget,
      LbvhTraversalOrder::near_first);
  auto plan_budget = generous_plan_budget();
  auto plan = build_exact_direct_sparse_unified_level_plan(
      index,
      cloud,
      source.facade,
      source.event_journal,
      source.arm_budget,
      source.arm_journal,
      source.incidence_budget,
      source.incidence_journal,
      star_budget,
      LbvhTraversalOrder::near_first,
      star,
      plan_budget);
  auto boruvka = build_exact_lbvh_boruvka(index, cloud);
  return {
      std::move(cloud),
      std::move(index),
      std::move(source),
      star_budget,
      std::move(star),
      plan_budget,
      std::move(plan),
      std::move(boruvka),
  };
}

[[nodiscard]] Fixture fixture(std::size_t requested_maximum_order) {
  const std::array points{
      point(-2.0, 4.0, -8.0),
      point(-1.0, 1.0, -1.0),
      point(0.0, 0.0, 0.0),
      point(1.0, 1.0, 1.0),
      point(3.0, 9.0, 27.0),
  };
  return fixture_from_points(points, requested_maximum_order);
}

// Ten points are the minimum needed to request all nine adjacent arrows.  This
// exact moment-curve source plans orders 2 through 10.  At cursor 98 its
// conditional SuccessiveStar locator lacks the direct K4 key {0,1,5,6}; the
// all-orders bridge must resolve that deletion through one bounded transient
// sparse descent, never by accepting the miss or scanning a global universe.
// Completing this relative replay still does not prove M.1 or public exactness.
[[nodiscard]] Fixture maximum_order_ten_fixture() {
  const std::array points{
      point(0.0, 0.0, 0.0),
      point(10000.0, 1.0, 1.0),
      point(20000.0, 4.0, 8.0),
      point(30000.0, 9.0, 27.0),
      point(40000.0, 16.0, 64.0),
      point(50000.0, 25.0, 125.0),
      point(60000.0, 36.0, 216.0),
      point(70000.0, 49.0, 343.0),
      point(80000.0, 64.0, 512.0),
      point(90000.0, 81.0, 729.0),
  };
  return fixture_from_points(points, 10U);
}

[[nodiscard]] ExactDirectMorseUnifiedResidentInitializationResult
resident_initialization(const Fixture& source, std::uint64_t authority_id) {
  return initialize_exact_direct_morse_unified_resident_session(
      source.index,
      source.cloud,
      source.source.facade,
      source.source.event_journal,
      source.source.arm_budget,
      source.source.arm_journal,
      source.source.incidence_budget,
      source.source.incidence_journal,
      source.star_budget,
      LbvhTraversalOrder::near_first,
      source.star,
      source.plan_budget,
      source.plan,
      authority_id,
      generous_resident_budget());
}

[[nodiscard]] ExactDirectK1BoruvkaClosedCutSessionBudget k1_budget(
    std::size_t point_count) {
  const std::size_t edge_count = point_count - 1U;
  return {
      point_count,
      64U,
      edge_count,
      edge_count,
      edge_count,
      2U * edge_count,
      point_count,
      point_count,
      point_count,
      point_count,
      point_count,
      2U * point_count,
      4096U,
  };
}

[[nodiscard]] ExactDirectK1BoruvkaClosedCutSessionInitialization
k1_initialization(const Fixture& source) {
  return build_exact_direct_k1_boruvka_closed_cut_session(
      source.index,
      source.cloud,
      source.boruvka,
      k1_budget(source.cloud.size()));
}

[[nodiscard]] ExactDirectMorseResidentK2K1ClosedCutBridgeBudget
k2_bridge_budget(const Fixture& source) {
  ExactDirectMorseResidentK2K1ClosedCutBridgeBudget budget;
  std::size_t k2_batch_count = 0U;
  std::size_t saddle_count = 0U;
  std::size_t birth_count = 0U;
  std::size_t maximum_batch_saddle_count = 0U;
  std::size_t maximum_batch_birth_count = 0U;
  for (const auto& batch : source.plan.batches) {
    if (batch.order == 2U) {
      ++k2_batch_count;
      std::size_t batch_saddle_count = 0U;
      std::size_t batch_birth_count = 0U;
      for (std::size_t local = 0U;
           local < batch.direct_reference_count;
           ++local) {
        if (source.plan
                .direct_references[batch.direct_reference_offset + local]
                .role == ExactDirectMorseH0Role::saddle) {
          ++batch_saddle_count;
          ++saddle_count;
        } else {
          ++batch_birth_count;
          ++birth_count;
        }
      }
      maximum_batch_saddle_count =
          std::max(maximum_batch_saddle_count, batch_saddle_count);
      maximum_batch_birth_count =
          std::max(maximum_batch_birth_count, batch_birth_count);
    }
  }
  budget.maximum_committed_k2_batch_count = k2_batch_count;
  budget.maximum_committed_k2_group_count = 4096U;
  budget.maximum_prepared_k2_group_count = 4096U;
  budget.maximum_committed_k2_direct_saddle_group_binding_count =
      saddle_count;
  budget.maximum_prepared_k2_direct_saddle_group_binding_count =
      maximum_batch_saddle_count;
  budget.maximum_committed_k2_direct_birth_k1_binding_count = birth_count;
  budget.maximum_prepared_k2_direct_birth_k1_binding_count =
      maximum_batch_birth_count;
  budget.maximum_direct_birth_k1_singleton_root_query_count =
      2U * maximum_batch_birth_count;
  budget.maximum_group_coverage_point_reference_scan_count = 65536U;
  budget.maximum_group_point_scratch_count = 65536U;
  budget.maximum_distinct_group_point_count = 65536U;
  budget.maximum_singleton_root_query_count = 65536U;
  return budget;
}

[[nodiscard]] ExactDirectMorseResidentK2K1ClosedCutInitialization
k2_bridge_initialization(const Fixture& source, std::uint64_t authority_id) {
  auto resident = resident_initialization(source, authority_id);
  auto k1 = k1_initialization(source);
  check(
      resident.certified_initialized_session() && k1.certified_ready_session(),
      "resident and K1 prerequisites initialize");
  if (!resident.certified_initialized_session() ||
      !k1.certified_ready_session()) {
    std::cerr << "resident decision="
              << static_cast<int>(resident.decision)
              << ", k1 decision=" << static_cast<int>(k1.decision)
              << ", plan decision=" << static_cast<int>(source.plan.decision)
              << '\n';
  }
  return initialize_exact_direct_morse_resident_k2_k1_closed_cut_bridge(
      std::move(*resident.session),
      std::move(k1.session),
      k2_bridge_budget(source));
}

[[nodiscard]] std::size_t higher_batch_count(const Fixture& source) {
  std::size_t count = 0U;
  for (const auto& batch : source.plan.batches) {
    count += batch.order >= 3U ? 1U : 0U;
  }
  return count;
}

[[nodiscard]] ExactDirectMorseResidentAllOrdersVerticalBridgeBudget
all_orders_budget(const Fixture& source) {
  ExactDirectMorseResidentAllOrdersVerticalBridgeBudget budget{};
  budget.maximum_committed_higher_batch_count = higher_batch_count(source);
  budget.maximum_committed_higher_group_count = 4096U;
  budget.maximum_prepared_higher_group_count = 4096U;
  std::size_t higher_saddle_count = 0U;
  std::size_t maximum_batch_higher_saddle_count = 0U;
  for (const auto& batch : source.plan.batches) {
    if (batch.order < 3U) {
      continue;
    }
    std::size_t batch_saddle_count = 0U;
    for (std::size_t local = 0U;
         local < batch.direct_reference_count;
         ++local) {
      if (source.plan
              .direct_references[batch.direct_reference_offset + local]
              .role == ExactDirectMorseH0Role::saddle) {
        ++batch_saddle_count;
        ++higher_saddle_count;
      }
    }
    maximum_batch_higher_saddle_count =
        std::max(maximum_batch_higher_saddle_count, batch_saddle_count);
  }
  budget.maximum_committed_higher_direct_saddle_group_binding_count =
      higher_saddle_count;
  budget.maximum_prepared_higher_direct_saddle_group_binding_count =
      maximum_batch_higher_saddle_count;
  budget.maximum_persistent_source_root_witness_count = 4096U;
  budget.maximum_prior_root_witness_probe_count = 65536U;
  budget.maximum_final_root_witness_probe_count = 4096U;
  budget.maximum_source_facet_resolution_scan_count = 1048576U;
  budget.maximum_projected_target_facet_probe_count = 1048576U;
  budget.maximum_sparse_target_closure_count = 1048576U;
  budget.maximum_expected_group_child_root_reference_count = 65536U;
  budget.maximum_expected_group_coverage_delta_point_reference_count =
      65536U;
  budget.maximum_query_replay_token =
      std::numeric_limits<std::uint64_t>::max();
  budget.target_probe = {4097U, 1024U};
  budget.target_closure = {
      4096U,
      4096U,
      4096U,
      8193U,
      {ExactDirectSparsePositiveFacetProbeBudget{4097U, 1024U},
       morsehgp3d::spatial::ExactLbvhTopKBudget{
           1048576U,
           1048576U,
           1048576U,
           1048576U,
           1024U,
           10U,
           10U},
       ExactDirectSparsePositiveFacetProbeBudget{4097U, 1024U}},
  };
  budget.target_closure_traversal_order = LbvhTraversalOrder::near_first;
  return budget;
}

[[nodiscard]] ExactDirectMorseResidentAllOrdersVerticalInitialization
all_orders_initialization(const Fixture& source, std::uint64_t authority_id) {
  auto inner = k2_bridge_initialization(source, authority_id);
  check(inner.certified_ready_bridge(), "owned K2/K1 bridge initializes");
  return initialize_exact_direct_morse_resident_all_orders_vertical_bridge(
      std::move(inner.bridge), all_orders_budget(source));
}

void inspect_prepared_higher_record(
    const ExactDirectMorseResidentAllOrdersVerticalPreparedBatch& ticket,
    const ExactDirectSparseUnifiedLevelPlanBatch& planned,
    bool& nonempty_higher_group_observed) {
  const auto* record = ticket.conditional_batch_record();
  check(record != nullptr, "higher ticket carries a conditional record");
  if (record == nullptr) {
    return;
  }
  check(
      record->source_batch_index == planned.batch_index &&
          record->squared_level == planned.squared_level &&
          record->source_order == planned.order &&
          record->target_order + 1U == record->source_order &&
          record->exact_product_order_target_batch_already_committed &&
          !record->all_naturality_squares_replayed &&
          !record->vertical_maps_complete,
      "higher preparation is bound to the exact product batch and remains conditional");
  for (const auto& group : record->group_images) {
    check(
        group.nonroot_source_facet_resolution_count <=
                std::numeric_limits<std::size_t>::max() /
                    record->source_order &&
            group.projected_target_facet_probe_count ==
                group.nonroot_source_facet_resolution_count *
                    record->source_order &&
            group.one_live_target_root_for_complete_group,
        "every nonroot source key projects all codimension-one deletions to one target root");
  }
  nonempty_higher_group_observed =
      nonempty_higher_group_observed || !record->group_images.empty();
}

void test_cap_minus_one_and_canonical_k1_digest(const Fixture& source) {
  auto k1 = k1_initialization(source);
  const K1CompactForest forest = build_compact_k1_forest(
      source.cloud.size(), source.boruvka.emst_edges);
  const auto canonical_digest =
      canonical_exact_direct_k1_closed_cut_source_forest_digest(
          k1.canonical_cloud_digest, forest);
  check(
      k1.certified_ready_session() &&
          canonical_digest == k1.source_forest_digest &&
          k1.session.verify_source_forest_digest(canonical_digest) &&
          k1.session.distinct_level_count() == forest.levels.size(),
      "the public canonical K1 forest digest and live level count match the sealed source");

  std::uint64_t authority_id = 93101U;
  const auto reject_budget = [&](
                                 const ExactDirectMorseResidentAllOrdersVerticalBridgeBudget&
                                     insufficient,
                                 const std::string& message) {
    auto inner = k2_bridge_initialization(source, authority_id++);
    const auto rejected =
        initialize_exact_direct_morse_resident_all_orders_vertical_bridge(
            std::move(inner.bridge), insufficient);
    check(
        rejected.decision ==
                ExactDirectMorseResidentAllOrdersVerticalInitializationDecision::
                    no_budget_rejected &&
            !rejected.owned_k2_k1_bridge_consumed && inner.bridge.ready(),
        message);
  };

  auto insufficient = all_orders_budget(source);
  check(
      insufficient.maximum_committed_higher_batch_count != 0U,
      "E5/K4 has at least one higher-order resident batch");
  --insufficient.maximum_committed_higher_batch_count;
  reject_budget(
      insufficient,
      "higher-batch cap minus one rejects before consuming the owned inner bridge");

  insufficient = all_orders_budget(source);
  if (insufficient
          .maximum_committed_higher_direct_saddle_group_binding_count != 0U) {
    --insufficient
          .maximum_committed_higher_direct_saddle_group_binding_count;
    reject_budget(
        insufficient,
        "higher direct-saddle binding cap minus one rejects before consuming the owned inner bridge");
  }
  insufficient = all_orders_budget(source);
  if (insufficient
          .maximum_prepared_higher_direct_saddle_group_binding_count != 0U) {
    --insufficient
          .maximum_prepared_higher_direct_saddle_group_binding_count;
    reject_budget(
        insufficient,
        "prepared higher direct-saddle binding cap minus one rejects before consuming the owned inner bridge");
  }
}

void test_owned_k1_target_only_terminal_drain(const Fixture& source) {
  check(
      source.plan.certified_bounded_plan() && source.plan.batches.empty(),
      "the order-one source yields an authenticated empty higher resident plan");
  auto initialized = all_orders_initialization(source, 93151U);
  check(
      initialized.certified_ready_bridge() &&
          initialized.bridge.resident_complete() &&
          !initialized.bridge.owned_k1_terminal_complete(),
      "an exhausted empty resident still owns an unconsumed nontrivial K1 target history");
  if (!initialized.certified_ready_bridge() ||
      !initialized.bridge.resident_complete()) {
    return;
  }
  const auto pre_stamp = initialized.bridge.owned_k1_current_stamp();
  const auto sealed = initialized.bridge.seal();
  check(
      sealed.certified_final_vertical_seal() &&
          sealed.seal->pre_terminal_k1_level_cursor ==
              pre_stamp.committed_level_cursor &&
          sealed.seal->sealed_target_only_k1_level_count > 0U &&
          sealed.seal->sealed_target_only_k1_level_count ==
              sealed.seal->distinct_k1_level_count -
                  pre_stamp.committed_level_cursor &&
          sealed.seal->terminal_k1_level_cursor ==
              sealed.seal->distinct_k1_level_count &&
          sealed.seal->every_target_only_k1_level_consumed &&
          initialized.bridge.owned_k1_terminal_complete(),
      "the owned terminal seam consumes and certifies a nonempty K1-only suffix after resident exhaustion");
}

void test_complete_all_orders_vertical_seal(const Fixture& source) {
  auto initialized = all_orders_initialization(source, 93201U);
  auto foreign = all_orders_initialization(source, 93202U);
  check(
      initialized.certified_ready_bridge() && foreign.certified_ready_bridge(),
      "two all-orders bridges initialize with distinct private capabilities");
  if (!initialized.certified_ready_bridge() ||
      !foreign.certified_ready_bridge()) {
    return;
  }
  auto& bridge = initialized.bridge;
  check(
      bridge.resident_source_kind() ==
              ExactDirectMorseUnifiedResidentSourceKind::
                  successive_incidence_star &&
          !bridge.resident_normalized_direct_source_session() &&
          bridge.resident_normalized_h0_retraction_mode() ==
              ExactDirectNormalizedH0ResidentRetractionMode::
                  not_applicable_successive_incidence_star &&
          !bridge
               .resident_normalized_horizontal_incidence_reduction_certified() &&
          !bridge.normalized_incidence_complete_v7_source_capability(),
      "the live SuccessiveStar source is observable and cannot impersonate normalized v7");
  const auto early_seal = bridge.seal();
  check(
      early_seal.decision ==
              ExactDirectMorseResidentAllOrdersVerticalSealDecision::
                  no_resident_not_exhausted &&
          early_seal.no_partial_seal_published_on_failure,
      "final seal is unavailable before resident exhaustion");

  const auto initial_stamp = bridge.current_stamp();
  auto foreign_ticket = bridge.prepare_next();
  check(
      foreign_ticket.certified_prepared_batch(),
      "an authentic initial ticket prepares");
  if (!foreign_ticket.certified_prepared_batch()) {
    return;
  }
  const auto foreign_rejection =
      foreign.bridge.commit(std::move(*foreign_ticket.ticket));
  check(
      foreign_rejection.decision ==
              ExactDirectMorseResidentAllOrdersVerticalCommitDecision::
                  no_foreign_ticket_rejected &&
          bridge.current_stamp() == initial_stamp,
      "a foreign outer bridge consumes no issuing resident or vertical state");

  bool stale_higher_sibling_checked = false;
  bool nonempty_higher_group_observed = false;
  std::optional<ExactDirectMorseResidentAllOrdersVerticalPreparedBatch>
      outstanding_final_sibling;
  while (!bridge.resident_complete()) {
    const std::size_t cursor = bridge.current_stamp().resident_batch_cursor;
    check(cursor < source.plan.batches.size(), "resident cursor stays in plan");
    if (cursor >= source.plan.batches.size()) {
      break;
    }
    const auto& planned = source.plan.batches[cursor];
    const bool final_batch = cursor + 1U == source.plan.batches.size();
    if (planned.order >= 3U && !stale_higher_sibling_checked) {
      auto first = bridge.prepare_next();
      auto stale = bridge.prepare_next();
      auto held = final_batch ? bridge.prepare_next()
                              : ExactDirectMorseResidentAllOrdersVerticalPreparationResult{};
      check(
          first.certified_prepared_batch() && stale.certified_prepared_batch() &&
              (!final_batch || held.certified_prepared_batch()),
          "same-snapshot higher sibling tickets prepare");
      if (!first.certified_prepared_batch() ||
          !stale.certified_prepared_batch() ||
          (final_batch && !held.certified_prepared_batch())) {
        break;
      }
      inspect_prepared_higher_record(
          *first.ticket, planned, nonempty_higher_group_observed);
      const auto committed = bridge.commit(std::move(*first.ticket));
      const auto stamp_after = bridge.current_stamp();
      const std::size_t records_after = bridge.committed_higher_batches().size();
      const std::size_t witnesses_after =
          bridge.source_root_target_witnesses().size();
      const auto stale_rejection = bridge.commit(std::move(*stale.ticket));
      check(
          committed.certified_committed_batch() &&
              stale_rejection.decision ==
                  ExactDirectMorseResidentAllOrdersVerticalCommitDecision::
                      no_owned_bridge_commit_rejected_without_outer_vertical_mutation &&
              stale_rejection
                  .no_outer_vertical_state_mutated_on_owned_bridge_rejection &&
              bridge.current_stamp() == stamp_after &&
              bridge.committed_higher_batches().size() == records_after &&
              bridge.source_root_target_witnesses().size() == witnesses_after,
          "a stale higher sibling leaves the outer transcript and witnesses atomic");
      stale_higher_sibling_checked = true;
      if (final_batch) {
        outstanding_final_sibling.emplace(std::move(*held.ticket));
      }
      continue;
    }

    if (final_batch) {
      auto first = bridge.prepare_next();
      auto held = bridge.prepare_next();
      check(
          first.certified_prepared_batch() && held.certified_prepared_batch(),
          "two tickets prepare on the final resident snapshot");
      if (!first.certified_prepared_batch() ||
          !held.certified_prepared_batch()) {
        break;
      }
      if (planned.order >= 3U) {
        inspect_prepared_higher_record(
            *first.ticket, planned, nonempty_higher_group_observed);
      }
      const auto committed = bridge.commit(std::move(*first.ticket));
      check(
          committed.certified_committed_batch(),
          "the final resident batch commits");
      outstanding_final_sibling.emplace(std::move(*held.ticket));
      continue;
    }

    auto prepared = bridge.prepare_next();
    check(
        prepared.certified_prepared_batch(),
        "each intermediate resident batch prepares");
    if (!prepared.certified_prepared_batch()) {
      break;
    }
    if (planned.order >= 3U) {
      inspect_prepared_higher_record(
          *prepared.ticket, planned, nonempty_higher_group_observed);
    }
    const auto committed = bridge.commit(std::move(*prepared.ticket));
    check(
        committed.certified_committed_batch(),
        "each intermediate resident batch commits");
    if (!committed.certified_committed_batch()) {
      break;
    }
  }

  check(
      bridge.resident_complete() && stale_higher_sibling_checked &&
          nonempty_higher_group_observed &&
          outstanding_final_sibling.has_value(),
      "E5/K4 exhausts resident batches with nonempty K>=3 groups and a held final sibling");
  const auto outstanding_rejection = bridge.seal();
  check(
      outstanding_rejection.decision ==
              ExactDirectMorseResidentAllOrdersVerticalSealDecision::
                  no_outstanding_prepared_ticket &&
          outstanding_rejection.no_partial_seal_published_on_failure,
      "a live stale ticket blocks final publication after resident exhaustion");
  outstanding_final_sibling.reset();

  const auto first_seal = bridge.seal();
  check(
      first_seal.certified_final_vertical_seal() &&
          first_seal.final_seal_published &&
          !first_seal.existing_final_seal_returned_without_mutation &&
          bridge.final_vertical_sealed(),
      "the first terminal call publishes one certified final vertical seal");
  if (!first_seal.certified_final_vertical_seal()) {
    std::cerr << "seal decision=" << static_cast<int>(first_seal.decision)
              << ", witnesses="
              << bridge.source_root_target_witnesses().size() << '\n';
    return;
  }
  const auto& seal = *first_seal.seal;
  const bool terminal_seal_facts =
      seal.all_naturality_squares_replayed && seal.vertical_maps_complete &&
          seal.every_intermediate_target_fusion_composed_by_terminal_root_reprobe &&
          seal.every_persistent_target_binding_reprobed_at_terminal_locator &&
          seal.sealed_final_root_witness_probe_count ==
              bridge.source_root_target_witnesses().size() &&
          seal.sealed_expected_projected_target_facet_probe_count ==
              seal.sealed_projected_target_facet_probe_count &&
          seal.terminal_k1_level_cursor == seal.distinct_k1_level_count &&
          seal.sealed_target_only_k1_level_count ==
              seal.terminal_k1_level_cursor -
                  seal.pre_terminal_k1_level_cursor &&
          seal.every_target_only_k1_level_consumed &&
          seal.owned_k1_terminal_cursor_complete &&
          bridge.owned_k1_terminal_complete() &&
          bridge.verify_owned_k1_source_forest_digest(
              seal.k1_source_forest_digest) &&
          !seal.normalized_incidence_complete_v7_source_capability &&
          !bridge.normalized_incidence_complete_v7_source_capability() &&
          !seal.global_morse_obligation_replayed && !seal.m1_replayed &&
          !seal.public_status_claimed;
  check(
      terminal_seal_facts,
      "final seal covers terminal target fusions and the strictly-later K1 suffix without promoting M1 or public exactness");
  if (!terminal_seal_facts) {
    std::cerr << "final probes="
              << seal.sealed_final_root_witness_probe_count
              << ", witnesses=" << bridge.source_root_target_witnesses().size()
              << ", expected deletions="
              << seal.sealed_expected_projected_target_facet_probe_count
              << ", observed deletions="
              << seal.sealed_projected_target_facet_probe_count
              << ", k1 pre=" << seal.pre_terminal_k1_level_cursor
              << ", k1 terminal=" << seal.terminal_k1_level_cursor
              << ", k1 total=" << seal.distinct_k1_level_count
              << ", k1 suffix=" << seal.sealed_target_only_k1_level_count
              << '\n';
  }

  std::size_t observed_higher_groups = 0U;
  bool saw_higher_direct_saddle_binding = false;
  bool checked_higher_membership_mutations = false;
  for (const auto& record : bridge.committed_higher_batches()) {
    observed_higher_groups += record.group_images.size();
    check(
        record.certified_conditional_higher_batch() &&
            record.o4_membership_digest !=
                morsehgp3d::contract::CanonicalId{} &&
            record.o4_membership_digest ==
                canonical_exact_direct_morse_resident_higher_o4_membership_digest(
                    record) &&
            !record.all_naturality_squares_replayed &&
            !record.vertical_maps_complete &&
            !record.global_morse_obligation_replayed &&
            !record.full_pi0_membership_claimed && !record.m1_replayed &&
            !record.public_status_claimed,
        "batch records remain conditional after final sealing");
    for (const auto& binding : record.direct_saddle_group_bindings) {
      saw_higher_direct_saddle_binding = true;
      const auto& direct =
          source.plan.direct_references[binding.source_direct_reference_index];
      check(
          binding.certified_conditional_saddle_group_binding() &&
              direct.role == ExactDirectMorseH0Role::saddle &&
              direct.source_role_record_index ==
                  binding.source_role_record_index &&
              direct.source_event_projection_index ==
                  binding.source_event_projection_index &&
              direct.source_incidence_family_index ==
                  std::optional<std::size_t>{
                      binding.source_incidence_family_index} &&
              binding.owner_group_index < record.group_images.size() &&
              binding.resident_resultant_root_id ==
                  record.group_images[binding.owner_group_index]
                      .resident_resultant_source_root_id,
          "a higher direct saddle retains its authentic event, family, frozen quotient group and committed source root");
      if (!checked_higher_membership_mutations) {
        auto event_mutation = record;
        ++event_mutation.direct_saddle_group_bindings.front()
              .source_event_projection_index;
        check(
            !event_mutation.certified_conditional_higher_batch(),
            "mutating a higher direct-saddle event invalidates its O.4 digest");
        auto group_mutation = record;
        group_mutation.direct_saddle_group_bindings.front().owner_group_index =
            group_mutation.group_images.size();
        check(
            !group_mutation.certified_conditional_higher_batch(),
            "mutating a higher direct-saddle quotient group fails closed");
        auto digest_mutation = record;
        digest_mutation.o4_membership_digest = {};
        check(
            !digest_mutation.certified_conditional_higher_batch(),
            "a missing higher O.4 membership digest fails closed");
        auto m1_overclaim = record;
        m1_overclaim.m1_replayed = true;
        m1_overclaim.o4_membership_digest =
            canonical_exact_direct_morse_resident_higher_o4_membership_digest(
                m1_overclaim);
        check(
            !m1_overclaim.certified_conditional_higher_batch(),
            "a coherently rehashed higher membership record cannot claim M.1");
        checked_higher_membership_mutations = true;
      }
    }
  }
  check(
      observed_higher_groups ==
          bridge.current_stamp().committed_higher_group_count &&
          observed_higher_groups == seal.sealed_higher_group_count &&
          saw_higher_direct_saddle_binding &&
          checked_higher_membership_mutations,
      "every committed K>=3 resident group has one retained image and the fixture exercises authentic higher O.4 saddle membership");

  const auto stamp_before_repeat = bridge.current_stamp();
  const auto second_seal = bridge.seal();
  check(
      second_seal.certified_final_vertical_seal() &&
          !second_seal.final_seal_published &&
          second_seal.existing_final_seal_returned_without_mutation &&
          second_seal.seal == first_seal.seal &&
          bridge.current_stamp() == stamp_before_repeat,
      "a second seal call is a certified idempotent read without mutation");

  auto mutated = seal;
  ++mutated.sealed_expected_projected_target_facet_probe_count;
  check(
      !mutated.certified_final_vertical_seal() &&
          !bridge.verify_final_vertical_seal(mutated),
      "a mutated final deletion counter cannot replay against the live bridge");
}

void test_maximum_order_ten_vertical_falsifier(const Fixture& source) {
  constexpr std::size_t maximum_order = 10U;
  std::array<std::size_t, maximum_order + 1U> planned_batches_by_order{};
  bool shared_multi_order_level = false;
  for (std::size_t index = 0U; index < source.plan.batches.size(); ++index) {
    const auto& batch = source.plan.batches[index];
    if (batch.order <= maximum_order) {
      ++planned_batches_by_order[batch.order];
    }
    if (index != 0U &&
        source.plan.batches[index - 1U].squared_level == batch.squared_level &&
        source.plan.batches[index - 1U].order != batch.order) {
      shared_multi_order_level = true;
    }
  }

  bool every_adjacent_source_order_planned = true;
  for (std::size_t order = 2U; order <= maximum_order; ++order) {
    every_adjacent_source_order_planned =
        every_adjacent_source_order_planned &&
        planned_batches_by_order[order] != 0U;
  }
  check(
      source.source.event_journal.requested_maximum_order == maximum_order &&
          source.source.event_journal.effective_maximum_order == maximum_order &&
          source.plan.certified_bounded_plan() &&
          source.plan.unique_batch_per_exact_level_and_order &&
          source.plan.batches_sorted_by_exact_level_then_order &&
          every_adjacent_source_order_planned && shared_multi_order_level,
      "the ten-point fixture plans every adjacent source order 2 through 10 and shares an exact level across orders");
  if (!source.plan.certified_bounded_plan() ||
      !every_adjacent_source_order_planned || !shared_multi_order_level) {
    std::cerr << "K10 plan decision=" << static_cast<int>(source.plan.decision)
              << ", batches=" << source.plan.batches.size()
              << ", effective K="
              << source.source.event_journal.effective_maximum_order
              << ", shared level=" << shared_multi_order_level;
    for (std::size_t order = 2U; order <= maximum_order; ++order) {
      std::cerr << ", K" << order << "=" << planned_batches_by_order[order];
    }
    std::cerr << '\n';
    return;
  }

  auto initialized = all_orders_initialization(source, 93301U);
  check(
      initialized.certified_ready_bridge(),
      "the maximum-order all-orders vertical bridge initializes");
  if (!initialized.certified_ready_bridge()) {
    std::cerr << "K10 all-orders init decision="
              << static_cast<int>(initialized.decision) << '\n';
    return;
  }
  auto& bridge = initialized.bridge;
  std::array<std::size_t, maximum_order + 1U> committed_batches_by_order{};
  std::array<std::size_t, maximum_order + 1U> committed_groups_by_order{};
  std::array<bool, maximum_order + 1U> witnessed_adjacent_arrow{};
  bool continuation_observed = false;
  bool cursor_98_direct_gap_certified = false;
  bool cursor_98_sparse_target_closure_observed = false;
  std::size_t sparse_target_closure_count = 0U;

  while (!bridge.resident_complete()) {
    const auto before = bridge.current_stamp();
    check(
        before.resident_batch_cursor < source.plan.batches.size(),
        "the maximum-order resident cursor remains inside the certified plan");
    if (before.resident_batch_cursor >= source.plan.batches.size()) {
      return;
    }
    const auto& planned = source.plan.batches[before.resident_batch_cursor];
    const auto locator_stamp_before = bridge.resident_locator().snapshot_stamp();
    const std::size_t higher_records_before =
        bridge.committed_higher_batches().size();
    const std::size_t k2_records_before = bridge.committed_k2_batches().size();
    const std::size_t witnesses_before =
        bridge.source_root_target_witnesses().size();
    if (before.resident_batch_cursor == 98U) {
      ExactDirectSparseFacetKey missing_direct_target{};
      missing_direct_target.point_count = 4U;
      missing_direct_target.point_ids[0] = 0U;
      missing_direct_target.point_ids[1] = 1U;
      missing_direct_target.point_ids[2] = 5U;
      missing_direct_target.point_ids[3] = 6U;
      const ExactDirectSparseFacetWitness diagnostic_witness{
          before.resident_session_authority_id, 987654321U};
      const auto direct_probe = bridge.resident_locator().probe_positive_facet(
          missing_direct_target,
          diagnostic_witness,
          all_orders_budget(source).target_probe);
      const auto direct_verification =
          verify_exact_direct_sparse_positive_facet_probe(
              bridge.resident_locator(),
              missing_direct_target,
              diagnostic_witness,
              all_orders_budget(source).target_probe,
              direct_probe);
      cursor_98_direct_gap_certified =
          planned.order == 5U && direct_probe.certified_unresolved_miss() &&
          direct_verification.result_certified;
      check(
          cursor_98_direct_gap_certified,
          "cursor 98 retains the permanent certified direct-locator miss that requires sparse target descent");
    }
    auto prepared = bridge.prepare_next();
    if (!prepared.certified_prepared_batch()) {
      check(
          false,
          "every maximum-order batch, including the K5-to-K4 direct miss, prepares through exact sparse target descent");
      std::cerr << "K10 prepare decision="
                << static_cast<int>(prepared.decision)
                << ", cursor=" << before.resident_batch_cursor
                << ", order=" << planned.order
                << ", stamp unchanged=" << (bridge.current_stamp() == before)
                << ", locator unchanged="
                << (bridge.resident_locator().snapshot_stamp() ==
                    locator_stamp_before)
                << ", higher records unchanged="
                << (bridge.committed_higher_batches().size() ==
                    higher_records_before)
                << ", K2 records unchanged="
                << (bridge.committed_k2_batches().size() == k2_records_before)
                << ", witnesses unchanged="
                << (bridge.source_root_target_witnesses().size() ==
                    witnesses_before)
                << '\n';
      return;
    }
    if (planned.order >= 3U) {
      const auto* record = prepared.ticket->conditional_batch_record();
      check(
          record != nullptr && record->source_order == planned.order &&
              record->target_order + 1U == record->source_order &&
              record->source_batch_index == before.resident_batch_cursor,
          "every higher-order ticket names its adjacent lower-order target and exact source batch");
      if (record == nullptr) {
        return;
      }
      sparse_target_closure_count += record->sparse_target_closure_count;
      if (before.resident_batch_cursor == 98U) {
        cursor_98_sparse_target_closure_observed =
            record->source_order == 5U &&
            record->sparse_target_closure_count != 0U;
      }
      witnessed_adjacent_arrow[record->source_order] =
          witnessed_adjacent_arrow[record->source_order] ||
          !record->group_images.empty();
      committed_groups_by_order[record->source_order] +=
          record->group_images.size();
      for (const auto& group : record->group_images) {
        continuation_observed =
            continuation_observed ||
            (group.q_r == 1U &&
             group.action == ExactFrozenIncidenceHgpAction::continuation);
      }
    } else {
      check(
          planned.order == 2U && prepared.nonhigher_transit_only,
          "each K2 batch is routed through the owned adjacent K2-to-K1 bridge");
    }

    const auto committed = bridge.commit(std::move(*prepared.ticket));
    check(
        committed.certified_committed_batch(),
        "every maximum-order vertical product batch commits");
    if (!committed.certified_committed_batch()) {
      std::cerr << "K10 commit decision="
                << static_cast<int>(committed.decision)
                << ", cursor=" << before.resident_batch_cursor
                << ", order=" << planned.order << '\n';
      return;
    }
    ++committed_batches_by_order[planned.order];
    const auto after = bridge.current_stamp();
    check(
        after.resident_batch_cursor == before.resident_batch_cursor + 1U &&
            after.resident_epoch == before.resident_epoch + 1U,
        "maximum-order commits advance the resident cursor and epoch exactly once in product order");
  }

  for (const auto& record : bridge.committed_k2_batches()) {
    check(
        record.order == 2U && record.certified_conditional_k2_to_k1_batch(),
        "every retained K2 record is one certified conditional K2-to-K1 batch");
    committed_groups_by_order[2U] += record.group_images.size();
  }
  witnessed_adjacent_arrow[2U] = committed_groups_by_order[2U] != 0U;

  bool every_nonterminal_adjacent_arrow_committed = true;
  for (std::size_t order = 2U; order < maximum_order; ++order) {
    every_nonterminal_adjacent_arrow_committed =
        every_nonterminal_adjacent_arrow_committed &&
        witnessed_adjacent_arrow[order] &&
        committed_batches_by_order[order] == planned_batches_by_order[order];
  }
  const auto terminal_stamp = bridge.current_stamp();
  check(
      every_nonterminal_adjacent_arrow_committed &&
          terminal_stamp.resident_batch_cursor == source.plan.batches.size() &&
          terminal_stamp.committed_k2_batch_count == planned_batches_by_order[2U] &&
          terminal_stamp.committed_higher_batch_count ==
              source.plan.batches.size() - planned_batches_by_order[2U] &&
          continuation_observed && cursor_98_direct_gap_certified &&
          cursor_98_sparse_target_closure_observed &&
          sparse_target_closure_count != 0U &&
          committed_batches_by_order[maximum_order] ==
              planned_batches_by_order[maximum_order] &&
          committed_groups_by_order[maximum_order] == 0U &&
          !witnessed_adjacent_arrow[maximum_order] && bridge.resident_complete() &&
          !bridge.conditional_all_adjacent_order_group_images_complete(),
      "the sparse closure repairs K5-to-K4 and exhausts all batches, while the missing K10 terminal component remains explicit");

  const auto k1_stamp_before_rejected_seal = bridge.owned_k1_current_stamp();
  const auto sealed = bridge.seal();
  check(
      sealed.decision ==
              ExactDirectMorseResidentAllOrdersVerticalSealDecision::
                  no_adjacent_source_order_component_coverage &&
          !sealed.seal.has_value() &&
          sealed.no_partial_seal_published_on_failure &&
          bridge.owned_k1_current_stamp() == k1_stamp_before_rejected_seal &&
          !bridge.final_vertical_sealed(),
      "K10 terminal-source omission blocks the seal before K1 drain and cannot be promoted to vertical completeness");

  for (const auto& witness : bridge.source_root_target_witnesses()) {
    check(
        witness.source_order == witness.target_order + 1U &&
            witness.source_order >= 3U &&
            witness.source_order <= maximum_order,
        "each persistent maximum-order witness belongs to one adjacent higher-order arrow");
  }
}

}  // namespace

int main() {
  const Fixture source = fixture(3U);
  std::size_t k2_batches = 0U;
  std::size_t higher_batches = 0U;
  bool orders_in_range = true;
  for (const auto& batch : source.plan.batches) {
    k2_batches += batch.order == 2U ? 1U : 0U;
    higher_batches += batch.order >= 3U ? 1U : 0U;
    orders_in_range =
        orders_in_range && batch.order >= 2U && batch.order <= 3U;
  }
  check(
      source.plan.certified_bounded_plan() &&
          source.plan.unique_batch_per_exact_level_and_order &&
          source.plan.batches_sorted_by_exact_level_then_order &&
          orders_in_range && k2_batches != 0U && higher_batches != 0U,
      "the focused E5/K3 fixture supplies exact K2 and K3 product batches");
  if (!source.plan.certified_bounded_plan() || k2_batches == 0U ||
      higher_batches == 0U) {
    std::cerr << "facade=" << source.source.facade.terminal_catalog_certified()
              << ", star decision=" << static_cast<int>(source.star.decision)
              << ", plan decision=" << static_cast<int>(source.plan.decision)
              << ", plan batches=" << source.plan.batches.size()
              << ", k2=" << k2_batches << ", higher=" << higher_batches
              << '\n';
  }
  test_cap_minus_one_and_canonical_k1_digest(source);
  test_complete_all_orders_vertical_seal(source);
  const Fixture maximum_order_source = maximum_order_ten_fixture();
  test_maximum_order_ten_vertical_falsifier(maximum_order_source);
  const Fixture k1_target_only_source = fixture(1U);
  test_owned_k1_target_only_terminal_drain(k1_target_only_source);
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "resident all-orders vertical bridge tests passed\n";
  return 0;
}
