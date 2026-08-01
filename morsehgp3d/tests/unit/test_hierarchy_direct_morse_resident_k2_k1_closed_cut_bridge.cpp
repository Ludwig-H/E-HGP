#include "morsehgp3d/hierarchy/direct_morse_resident_k2_k1_closed_cut_bridge.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

static_assert(
    direct_morse_resident_k2_k1_closed_cut_bridge_schema_version == 1U);
static_assert(!std::is_copy_constructible_v<
              ExactDirectMorseResidentK2K1ClosedCutBridge>);
static_assert(std::is_nothrow_move_constructible_v<
              ExactDirectMorseResidentK2K1ClosedCutBridge>);
static_assert(!std::is_copy_constructible_v<
              ExactDirectMorseResidentK2K1ClosedCutPreparedBatch>);

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(double x, double y) {
  return CertifiedPoint3::from_binary64(x, y, 0.0);
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
      1024U,
      1024U,
      10240U,
      1024U,
      1024U,
      1024U,
      1024U,
      1024U,
      10240U,
      4097U,
      4097U,
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
      1024U,
      10240U,
      1024U,
      1024U,
      10240U,
      1024U,
      10240U,
      10240U,
      4U,
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
    const CanonicalPointCloud& cloud) {
  const ExactDirectSupportTerminalBudget terminal_budget{
      unlimited_pair_budget(), unlimited_higher_budget()};
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto pair = build_exact_pair_support_stream(
      index, cloud, 2U, terminal_budget.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, 2U, terminal_budget.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index, cloud, 2U, terminal_budget, pair, higher);
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

[[nodiscard]] Fixture fixture() {
  const std::array points{
      point(-2.0, -1.0),
      point(-2.0, 1.0),
      point(0.0, 0.0),
      point(3.0, 2.0),
      point(4.0, -1.0),
  };
  auto cloud = CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
  auto index = MortonLbvhIndex::build(cloud);
  auto source = direct_sources(cloud);
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

[[nodiscard]] ExactDirectMorseUnifiedResidentInitializationResult
resident_initialization(
    const Fixture& source,
    std::uint64_t authority_id) {
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
bridge_budget(const Fixture& source) {
  std::size_t k2_batch_count = 0U;
  for (const auto& batch : source.plan.batches) {
    if (batch.order == 2U) {
      ++k2_batch_count;
    }
  }
  return {
      k2_batch_count,
      4096U,
      1024U,
      65536U,
      65536U,
      65536U,
      65536U,
  };
}

[[nodiscard]] ExactDirectMorseResidentK2K1ClosedCutInitialization
bridge_initialization(
    const Fixture& source,
    std::uint64_t resident_authority_id,
    const ExactDirectMorseResidentK2K1ClosedCutBridgeBudget& budget) {
  auto resident = resident_initialization(source, resident_authority_id);
  auto k1 = k1_initialization(source);
  check(
      resident.certified_initialized_session() &&
          k1.certified_ready_session(),
      "the resident and sealed K1 prerequisites initialize");
  return initialize_exact_direct_morse_resident_k2_k1_closed_cut_bridge(
      std::move(*resident.session), std::move(k1.session), budget);
}

[[nodiscard]] std::vector<PointId> independent_first_group_image_points(
    const ExactDirectMorseUnifiedResidentAuthorityBundle& bundle) {
  std::vector<PointId> points;
  const auto& frozen = bundle.frozen_batch;
  if (frozen.action_plan.groups.empty() || frozen.quotient.groups.empty() ||
      frozen.coverage_deltas.empty()) {
    return points;
  }
  const auto& action_group = frozen.action_plan.groups.front();
  for (std::size_t local_index = 0U;
       local_index < action_group.prior_root_count;
       ++local_index) {
    const auto root_id = frozen.action_plan.prior_root_ids[
        action_group.prior_root_offset + local_index];
    const auto coverage = std::find_if(
        bundle.prior_root_coverages.begin(),
        bundle.prior_root_coverages.end(),
        [root_id](const ExactDirectFrozenUnifiedPriorRootCoverage& value) {
          return value.prior_root_id == root_id;
        });
    if (coverage == bundle.prior_root_coverages.end()) {
      return {};
    }
    const auto first = bundle.prior_root_coverage_point_references.begin() +
        static_cast<std::ptrdiff_t>(coverage->point_reference_offset);
    points.insert(
        points.end(),
        first,
        first + static_cast<std::ptrdiff_t>(coverage->point_reference_count));
  }

  const auto& quotient_group = frozen.quotient.groups.front();
  for (std::size_t local_index = 0U;
       local_index < quotient_group.token_count;
       ++local_index) {
    const auto& token = frozen.quotient.group_tokens[
        quotient_group.token_offset + local_index];
    if (token.kind != ExactFrozenIncidenceTokenKind::latent_carrier) {
      continue;
    }
    const auto coverage = std::find_if(
        bundle.latent_carrier_coverages.begin(),
        bundle.latent_carrier_coverages.end(),
        [token](
            const ExactDirectFrozenUnifiedLatentCarrierCoverage& value) {
          return value.latent_carrier_token_id == token.token_id;
        });
    if (coverage == bundle.latent_carrier_coverages.end()) {
      return {};
    }
    const auto first =
        bundle.latent_carrier_coverage_point_references.begin() +
        static_cast<std::ptrdiff_t>(coverage->point_reference_offset);
    points.insert(
        points.end(),
        first,
        first + static_cast<std::ptrdiff_t>(coverage->point_reference_count));
  }

  const auto& delta = frozen.coverage_deltas.front();
  for (std::size_t local_index = 0U;
       local_index < delta.point_reference_count;
       ++local_index) {
    const auto& reference = frozen.coverage_delta_points[
        delta.point_reference_offset + local_index];
    if (reference.owner_group_index != 0U) {
      return {};
    }
    points.push_back(reference.point_id);
  }
  std::sort(points.begin(), points.end());
  points.erase(std::unique(points.begin(), points.end()), points.end());
  return points;
}

void test_private_seals_retry_and_atomic_resident_rejection(
    const Fixture& source) {
  auto first = bridge_initialization(source, 91001U, bridge_budget(source));
  auto second = bridge_initialization(source, 91002U, bridge_budget(source));
  check(
      first.certified_ready_bridge() && second.certified_ready_bridge(),
      "two bounded bridges initialize with independent live capabilities");
  if (!first.certified_ready_bridge() || !second.certified_ready_bridge()) {
    std::cerr << "bridge init decisions: "
              << static_cast<int>(first.decision) << ", "
              << static_cast<int>(second.decision) << "; k2 requirements="
              << first.requirements.resident_k2_batch_count << ", budget="
              << first.requested_budget.maximum_committed_k2_batch_count
              << '\n';
    return;
  }

  const auto first_initial_stamp = first.bridge.current_stamp();
  const auto second_initial_stamp = second.bridge.current_stamp();
  check(
      first_initial_stamp.bridge_session_instance_id !=
              second_initial_stamp.bridge_session_instance_id &&
          first_initial_stamp.committed_k1_stamp.session_instance_id !=
              second_initial_stamp.committed_k1_stamp.session_instance_id &&
          !first.bridge.verify_stamp(second_initial_stamp),
      "a same-cloud K1 stamp from another bridge remains foreign");
  auto forged = first_initial_stamp;
  ++forged.committed_k1_stamp.session_instance_id;
  check(
      !first.bridge.verify_stamp(forged),
      "a forged nested K1 session id cannot pass live bridge replay");

  auto abandoned = first.bridge.prepare_next();
  check(
      abandoned.certified_prepared_batch() &&
          abandoned.ticket->k2_vertical_batch() &&
          abandoned.ticket->resident_authority_bundle()
                  .certified_strict_pre_batch_bundle() &&
          abandoned.ticket->conditional_batch_record() != nullptr &&
          abandoned.ticket->conditional_batch_record()
              ->k1_cut_advanced_before_resident_commit,
      "the private composite ticket exposes the true resident bundle and a prepared K2 image");
  const auto foreign_rejection =
      second.bridge.commit(std::move(*abandoned.ticket));
  check(
      foreign_rejection.decision ==
              ExactDirectMorseResidentK2K1ClosedCutCommitDecision::
                  no_foreign_ticket_rejected &&
          first.bridge.current_stamp() == first_initial_stamp &&
          first.bridge.committed_k2_batches().empty(),
      "a foreign bridge consumes no resident or vertical state in the issuing bridge");

  auto retry = first.bridge.prepare_next();
  check(
      retry.certified_prepared_batch() && retry.ticket->k2_vertical_batch(),
      "the same K2 resident batch remains retryable after the abandoned commit");
  const auto retry_record = retry.ticket->conditional_batch_record();
  const std::size_t retry_consumed_level_count =
      retry_record == nullptr
          ? 0U
          : retry_record->consumed_intermediate_k1_level_count;
  const auto retry_commit = first.bridge.commit(std::move(*retry.ticket));
  check(
      retry_commit.certified_committed_batch() &&
          retry_commit.vertical_state_mutated &&
          first.bridge.committed_k2_batches().size() == 1U &&
          first.bridge.committed_k2_batches().front()
              .certified_conditional_k2_to_k1_batch() &&
          first.bridge.committed_k2_batches().front()
                  .consumed_intermediate_k1_level_count ==
              retry_consumed_level_count &&
          !first.bridge.verify_stamp(first_initial_stamp),
      "retry publishes exactly one conditional K2-to-K1 batch and stales the old stamp");

  auto duplicate_a = first.bridge.prepare_next();
  auto duplicate_b = first.bridge.prepare_next();
  check(
      duplicate_a.certified_prepared_batch() &&
          duplicate_b.certified_prepared_batch(),
      "two private resident tickets may observe the same strict pre-batch snapshot");
  const auto committed_a =
      first.bridge.commit(std::move(*duplicate_a.ticket));
  const auto stamp_after_a = first.bridge.current_stamp();
  const std::size_t batch_count_after_a =
      first.bridge.committed_k2_batches().size();
  const auto rejected_b =
      first.bridge.commit(std::move(*duplicate_b.ticket));
  check(
      committed_a.certified_committed_batch() &&
          rejected_b.decision ==
              ExactDirectMorseResidentK2K1ClosedCutCommitDecision::
                  no_resident_commit_rejected_without_vertical_mutation &&
          rejected_b.no_vertical_state_mutated_on_resident_rejection &&
          !rejected_b.vertical_state_mutated &&
          first.bridge.current_stamp() == stamp_after_a &&
          first.bridge.committed_k2_batches().size() == batch_count_after_a,
      "a stale resident-ticket rejection leaves the vertical transcript and stamp atomic");

  bool independent_nonempty_group_checked = false;
  while (!first.bridge.resident_complete()) {
    auto prepared = first.bridge.prepare_next();
    check(
        prepared.certified_prepared_batch(),
        "every remaining resident batch prepares through the composite stream");
    if (!prepared.certified_prepared_batch()) {
      break;
    }
    const auto* conditional_record =
        prepared.ticket->conditional_batch_record();
    if (!independent_nonempty_group_checked &&
        conditional_record != nullptr &&
        !conditional_record->group_images.empty()) {
      const auto independently_reconstructed_first_group =
          independent_first_group_image_points(
              prepared.ticket->resident_authority_bundle());
      check(
          !independently_reconstructed_first_group.empty() &&
              conditional_record->group_images.front()
                      .exhaustive_distinct_point_count ==
                  independently_reconstructed_first_group.size() &&
              conditional_record->group_images.front()
                      .canonical_representative_point_id ==
                  independently_reconstructed_first_group.front(),
          "an independent CSR union reproduces a nonempty prepared group count and representative");
      independent_nonempty_group_checked = true;
    }
    const auto committed = first.bridge.commit(std::move(*prepared.ticket));
    check(
        committed.certified_committed_batch(),
        "every remaining resident batch commits through the composite stream");
    if (!committed.certified_committed_batch()) {
      break;
    }
  }
  check(
      independent_nonempty_group_checked,
      "the strict fixture reaches a nonempty K2 group for the independent CSR oracle");
  for (const auto& record : first.bridge.committed_k2_batches()) {
    check(
        record.certified_conditional_k2_to_k1_batch() &&
            record.group_images_exhaustive_from_frozen_csr &&
            record.every_group_has_one_live_closed_k1_root &&
            !record.incidence_complete_reduction &&
            !record.vertical_maps_complete && !record.public_status_claimed,
        "every K2 group image is CSR-exhaustive, live-query consistent, and conditional only");
  }
}

void test_midstream_attach_consumes_every_skipped_k1_level(
    const Fixture& source) {
  auto resident = resident_initialization(source, 91501U);
  check(
      resident.certified_initialized_session(),
      "the midstream resident prerequisite initializes");
  if (!resident.certified_initialized_session()) {
    return;
  }
  for (std::size_t batch_index = 0U; batch_index < 2U; ++batch_index) {
    auto prepared = resident.session->prepare_next();
    check(
        prepared.certified_prepared_batch(),
        "the resident advances before the vertical seam is attached");
    if (!prepared.certified_prepared_batch()) {
      return;
    }
    const auto committed = resident.session->commit(std::move(*prepared.ticket));
    check(
        committed.certified_committed_batch(),
        "the pre-bridge resident batch commits");
    if (!committed.certified_committed_batch()) {
      return;
    }
  }
  auto k1 = k1_initialization(source);
  auto initialized =
      initialize_exact_direct_morse_resident_k2_k1_closed_cut_bridge(
          std::move(*resident.session),
          std::move(k1.session),
          bridge_budget(source));
  check(
      initialized.certified_ready_bridge(),
      "a conditional bridge may attach to a certified resident cursor without claiming past vertical maps");
  if (!initialized.certified_ready_bridge()) {
    return;
  }
  auto prepared = initialized.bridge.prepare_next();
  const auto* record = prepared.ticket.has_value()
                           ? prepared.ticket->conditional_batch_record()
                           : nullptr;
  check(
      prepared.certified_prepared_batch() && record != nullptr &&
          record->published_pre_k1_stamp.committed_level_cursor == 0U &&
          record->consumed_intermediate_k1_level_count >= 2U &&
          record->live_post_k1_stamp.committed_level_cursor ==
              record->consumed_intermediate_k1_level_count &&
          record->every_intermediate_k1_level_consumed,
      "the first attached K2 batch consumes every skipped exact K1 equality level");
  if (prepared.certified_prepared_batch()) {
    const auto committed =
        initialized.bridge.commit(std::move(*prepared.ticket));
    check(
        committed.certified_committed_batch(),
        "the skipped-level K2 batch commits after the lower cut is closed");
  }
}

void test_canonical_cloud_digest_binding_and_negative_cloud(
    const Fixture& source) {
  auto resident = resident_initialization(source, 91701U);
  auto matching_k1 = k1_initialization(source);
  check(
      resident.session->plan().source_pair_canonical_cloud_digest ==
          matching_k1.canonical_cloud_digest,
      "the sealed K1 cloud digest uses the canonical pair-support domain encoding");

  const std::array other_points{
      point(-2.0, -1.0),
      point(-2.0, 1.0),
      point(0.0, 0.0),
      point(3.0, 2.0),
      point(5.0, -1.0),
  };
  auto other_cloud = CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{other_points});
  auto other_index = MortonLbvhIndex::build(other_cloud);
  auto other_boruvka = build_exact_lbvh_boruvka(other_index, other_cloud);
  auto foreign_k1 = build_exact_direct_k1_boruvka_closed_cut_session(
      other_index,
      other_cloud,
      other_boruvka,
      k1_budget(other_cloud.size()));
  check(
      foreign_k1.certified_ready_session() &&
          resident.session->plan().source_pair_canonical_cloud_digest !=
              foreign_k1.canonical_cloud_digest,
      "a one-point cloud mutation changes the sealed pair-namespace digest");
  const auto rejected =
      initialize_exact_direct_morse_resident_k2_k1_closed_cut_bridge(
          std::move(*resident.session),
          std::move(foreign_k1.session),
          bridge_budget(source));
  check(
      rejected.decision ==
              ExactDirectMorseResidentK2K1ClosedCutInitializationDecision::
                  no_cloud_or_point_namespace_mismatch &&
          !rejected.resident_live_session_consumed &&
          !rejected.k1_live_session_consumed &&
          resident.session->certified_resident_session() &&
          foreign_k1.session.ready(),
      "a same-size foreign K1 cloud is rejected before either live session is consumed");
}

void test_cap_minus_one_rejects_before_consuming_sessions(
    const Fixture& source) {
  auto resident = resident_initialization(source, 92001U);
  auto k1 = k1_initialization(source);
  auto insufficient = bridge_budget(source);
  check(
      insufficient.maximum_committed_k2_batch_count != 0U,
      "the fixture has at least one K2 resident batch");
  --insufficient.maximum_committed_k2_batch_count;
  const auto rejected =
      initialize_exact_direct_morse_resident_k2_k1_closed_cut_bridge(
          std::move(*resident.session), std::move(k1.session), insufficient);
  check(
      rejected.decision ==
              ExactDirectMorseResidentK2K1ClosedCutInitializationDecision::
                  no_budget_rejected &&
          !rejected.resident_live_session_consumed &&
          !rejected.k1_live_session_consumed &&
          resident.session->certified_resident_session() &&
          k1.session.ready(),
      "the exact K2-batch cap minus one rejects before either live session is consumed");
}

}  // namespace

int main() {
  const Fixture source = fixture();
  check(
      source.plan.certified_bounded_plan(),
      "the focused fixture provides a certified sparse resident source plan");
  test_private_seals_retry_and_atomic_resident_rejection(source);
  test_midstream_attach_consumes_every_skipped_k1_level(source);
  test_canonical_cloud_digest_binding_and_negative_cloud(source);
  test_cap_minus_one_rejects_before_consuming_sessions(source);
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "resident K2-to-sealed-K1 closed-cut bridge tests passed\n";
  return 0;
}
