#include "morsehgp3d/hierarchy/direct_k1_normalized_product_stream.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::
    ExactDirectK1NormalizedAtLeast20TransitionKind;
using morsehgp3d::hierarchy::
    ExactDirectK1BoruvkaClosedCutSessionBudget;
using morsehgp3d::hierarchy::
    ExactDirectK1NormalizedGenesisSliceDecision;
using morsehgp3d::hierarchy::
    ExactDirectK1NormalizedProductBatchKind;
using morsehgp3d::hierarchy::
    ExactDirectK1NormalizedProductBatchRecord;
using morsehgp3d::hierarchy::
    ExactDirectK1NormalizedProductCommitDecision;
using morsehgp3d::hierarchy::
    ExactDirectK1NormalizedProductPrepareDecision;
using morsehgp3d::hierarchy::
    ExactDirectK1NormalizedProductRestoreDecision;
using morsehgp3d::hierarchy::
    ExactDirectK1NormalizedProductStreamBudget;
using morsehgp3d::hierarchy::
    ExactDirectK1NormalizedProductStreamCheckpoint;
using morsehgp3d::hierarchy::
    ExactDirectK1NormalizedProductStreamInitializationDecision;
using morsehgp3d::hierarchy::
    ExactDirectK1NormalizedProductStreamSession;
using morsehgp3d::hierarchy::K1CompactForest;
using morsehgp3d::hierarchy::K1ExactBoruvkaResult;
using morsehgp3d::hierarchy::build_compact_k1_forest;
using morsehgp3d::hierarchy::build_exact_lbvh_boruvka;
using morsehgp3d::hierarchy::
    build_exact_direct_k1_boruvka_closed_cut_session;
using morsehgp3d::hierarchy::
    initialize_exact_direct_k1_normalized_product_stream;
using morsehgp3d::hierarchy::
    restore_exact_direct_k1_normalized_product_stream;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;

static_assert(!std::is_copy_constructible_v<
              ExactDirectK1NormalizedProductStreamSession>);
static_assert(std::is_nothrow_move_constructible_v<
              ExactDirectK1NormalizedProductStreamSession>);

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(double x) {
  return CertifiedPoint3::from_binary64(x, 0.0, 0.0);
}

[[nodiscard]] CanonicalPointCloud mixed_level_cloud() {
  const std::array<CertifiedPoint3, 6U> points{
      point(0.0),
      point(1.0),
      point(2.0),
      point(10.0),
      point(11.0),
      point(30.0)};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud threshold_cloud() {
  std::vector<CertifiedPoint3> points;
  points.reserve(20U);
  for (std::size_t index = 0U; index < 20U; ++index) {
    points.push_back(point(static_cast<double>(index)));
  }
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] CanonicalPointCloud visible_transition_cloud() {
  std::vector<CertifiedPoint3> points;
  points.reserve(41U);
  for (std::size_t index = 0U; index < 20U; ++index) {
    points.push_back(point(static_cast<double>(index)));
  }
  points.push_back(point(50.0));
  for (std::size_t index = 0U; index < 20U; ++index) {
    points.push_back(point(100.0 + static_cast<double>(index)));
  }
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactDirectK1BoruvkaClosedCutSessionBudget authority_budget(
    std::size_t point_count) {
  const std::size_t edge_count = point_count - 1U;
  return ExactDirectK1BoruvkaClosedCutSessionBudget{
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
      4096U};
}

[[nodiscard]] ExactDirectK1NormalizedProductStreamBudget stream_budget(
    std::size_t point_count,
    std::size_t maximum_outstanding = 2U) {
  const std::size_t edge_count = point_count - 1U;
  return ExactDirectK1NormalizedProductStreamBudget{
      point_count,
      point_count,
      edge_count,
      2U * edge_count,
      point_count + 1U,
      point_count,
      maximum_outstanding,
      edge_count,
      2U * edge_count,
      point_count / 20U,
      point_count / 20U};
}

struct Fixture {
  CanonicalPointCloud cloud{mixed_level_cloud()};
  MortonLbvhIndex index{MortonLbvhIndex::build(cloud)};
  K1ExactBoruvkaResult boruvka{build_exact_lbvh_boruvka(index, cloud)};
  K1CompactForest forest{
      build_compact_k1_forest(cloud.size(), boruvka.emst_edges)};
  ExactDirectK1BoruvkaClosedCutSessionBudget k1_budget{
      authority_budget(cloud.size())};
  ExactDirectK1NormalizedProductStreamBudget product_budget{
      stream_budget(cloud.size())};
};

[[nodiscard]] bool same_semantic_checkpoint(
    const ExactDirectK1NormalizedProductStreamCheckpoint& left,
    const ExactDirectK1NormalizedProductStreamCheckpoint& right) {
  return left.schema_version == right.schema_version &&
      left.stamp.schema_version == right.stamp.schema_version &&
      left.stamp.next_batch_cursor == right.stamp.next_batch_cursor &&
      left.stamp.epoch == right.stamp.epoch &&
      left.stamp.last_committed_squared_level ==
          right.stamp.last_committed_squared_level &&
      left.stamp.canonical_cloud_digest ==
          right.stamp.canonical_cloud_digest &&
      left.stamp.source_forest_digest == right.stamp.source_forest_digest &&
      left.stamp.scientific_digest == right.stamp.scientific_digest &&
      left.stamp.committed_scientific_chain_digest ==
          right.stamp.committed_scientific_chain_digest &&
      left.stamp.at_least20_view_digest ==
          right.stamp.at_least20_view_digest &&
      left.stamp.active_at_least20_root_count ==
          right.stamp.active_at_least20_root_count &&
      left.total_batch_count == right.total_batch_count &&
      left.total_merge_node_count == right.total_merge_node_count &&
      left.total_child_reference_count == right.total_child_reference_count &&
      left.active_at_least20_root_ids ==
          right.active_at_least20_root_ids &&
      left.state_digest == right.state_digest &&
      left.durable_file_codec_claimed == right.durable_file_codec_claimed &&
      left.public_status_claimed == right.public_status_claimed;
}

void test_unique_closed_zero_batch_and_positive_equal_levels() {
  Fixture fixture;
  auto initialization = initialize_exact_direct_k1_normalized_product_stream(
      fixture.index,
      fixture.cloud,
      fixture.boruvka,
      fixture.k1_budget,
      fixture.product_budget);
  check(
      initialization.certified_initialized_stream() &&
          initialization.exact_product_batch_count ==
              fixture.forest.equal_level_batches.size() + 1U &&
          initialization.exact_merge_node_count ==
              fixture.forest.merge_nodes.size() &&
          initialization.exact_child_reference_count ==
              fixture.forest.child_ids.size(),
      "fresh Boruvka authentication did not mint the compact K1 product stream");

  auto authority = build_exact_direct_k1_boruvka_closed_cut_session(
      fixture.index, fixture.cloud, fixture.boruvka, fixture.k1_budget);
  check(
      authority.certified_ready_session() &&
          authority.source_forest_digest ==
              initialization.source_forest_digest &&
          authority.source_forest_digest ==
              initialization.session.source_forest_digest(),
      "the product stream did not reuse the authoritative K1 forest identity");

  auto genesis = initialization.session.prepare_next();
  check(
      genesis.certified_prepared_batch(),
      "the unique closed level-zero product batch was not prepared");
  const auto& zero = genesis.ticket->prepared_record();
  check(
      zero.kind == ExactDirectK1NormalizedProductBatchKind::
                       implicit_singleton_genesis &&
          zero.normalized_batch_index == 0U && zero.order == 1U &&
          zero.squared_level == ExactLevel{} &&
          zero.group_count == fixture.cloud.size() &&
          zero.qr0_group_count == fixture.cloud.size() &&
          zero.qr1_group_count == 0U &&
          zero.qr_greater_equal2_group_count == 0U &&
          zero.core_facet_delta_count == fixture.cloud.size() &&
          zero.point_delta_count == fixture.cloud.size() &&
          zero.node_delta_count == fixture.cloud.size() &&
          zero.parent_delta_count == 0U &&
          zero.merge_node_count == 0U &&
          zero.child_reference_count == 0U &&
          zero.closed_zero_level_partition_certified &&
          zero.at_least20_hidden_result_count == fixture.cloud.size() &&
          zero.at_least20_threshold_entry_count == 0U &&
          zero.at_least20_continuation_count == 0U &&
          zero.at_least20_multifusion_count == 0U &&
          zero.active_at_least20_root_count_before == 0U &&
          zero.active_at_least20_root_count_after == 0U &&
          zero.at_least20_complete_compact_fold_certified &&
          zero.implicit_singleton_genesis_without_group_materialization,
      "the closed zero partition is not the compact n-singleton qR0 batch");
  const auto zero_view = initialization.session.batch_view(zero);
  check(
      zero_view.has_value() && zero_view->merge_nodes.empty() &&
          zero_view->child_root_ids.empty() &&
          zero_view->implicit_singleton_count == fixture.cloud.size(),
      "the level-zero view materialized unexpected merge/group arenas");
  const auto slice = initialization.session.genesis_slice(zero, 1U, 3U, 3U);
  check(
      slice.certified() && slice.root_id(0U) == 1U &&
          slice.point_id(2U) == 3U,
      "the bounded implicit singleton slice is not root=PointId exact");
  check(
      initialization.session.genesis_slice(zero, 0U, 4U, 3U).decision ==
          ExactDirectK1NormalizedGenesisSliceDecision::
              no_requested_slice_budget_exhausted &&
          initialization.session
                  .genesis_slice(zero, fixture.cloud.size(), 1U, 1U)
                  .decision ==
              ExactDirectK1NormalizedGenesisSliceDecision::
                  no_requested_slice_out_of_range,
      "the genesis range API did not fail closed at its explicit bounds");
  const auto zero_commit =
      initialization.session.commit(std::move(*genesis.ticket));
  check(
      zero_commit.certified_atomic_batch_commit() &&
          zero_commit.no_allocation_after_preparation,
      "the prepared zero batch did not commit atomically");

  ExactLevel previous_level{};
  std::size_t observed_merge_nodes = 0U;
  std::size_t observed_child_references = 0U;
  bool saw_multifusion = false;
  bool saw_equal_level_multiple_groups = false;
  while (!initialization.session.complete()) {
    auto prepared = initialization.session.prepare_next();
    check(prepared.certified_prepared_batch(), "a positive K1 batch failed");
    if (!prepared.certified_prepared_batch()) {
      break;
    }
    const auto& record = prepared.ticket->prepared_record();
    const auto view = initialization.session.batch_view(record);
    check(
        view.has_value() && record.squared_level > previous_level &&
            record.kind == ExactDirectK1NormalizedProductBatchKind::
                               compact_equal_level_multifusions &&
            record.qr0_group_count == 0U && record.qr1_group_count == 0U &&
            record.qr_greater_equal2_group_count == record.group_count &&
            record.core_facet_delta_count == 0U &&
            record.point_delta_count == 0U &&
            record.node_delta_count == record.group_count &&
            record.parent_delta_count == record.child_reference_count,
        "a positive equal-level batch violated chronology, qR, or deltas");
    if (view.has_value()) {
      std::size_t child_sum = 0U;
      for (const auto& node : view->merge_nodes) {
        child_sum += node.child_count;
        saw_multifusion = saw_multifusion || node.child_count >= 3U;
      }
      check(
          child_sum == view->child_root_ids.size() &&
              child_sum == record.parent_delta_count,
          "the compact child CSR is not exactly sum(qR)");
      saw_equal_level_multiple_groups =
          saw_equal_level_multiple_groups || view->merge_nodes.size() >= 2U;
      observed_merge_nodes += view->merge_nodes.size();
      observed_child_references += view->child_root_ids.size();
    }
    previous_level = record.squared_level;
    const auto committed =
        initialization.session.commit(std::move(*prepared.ticket));
    check(
        committed.certified_atomic_batch_commit(),
        "a positive compact K1 batch did not commit");
  }
  check(
      observed_merge_nodes == fixture.forest.merge_nodes.size() &&
          observed_child_references == fixture.forest.child_ids.size() &&
          saw_multifusion && saw_equal_level_multiple_groups,
      "the stream did not cover equality groups, multifusions, and both CSR arenas");
  const auto final_seal = initialization.session.seal();
  check(
      final_seal.certified_terminal_closure() &&
          initialization.session.verify_final_seal(final_seal) &&
          final_seal.root_node_id == fixture.forest.root_node_id,
      "the terminal compact K1 stream did not issue a verified closure seal");
}

void test_fresh_digest_determinism_and_sibling_ticket_scope() {
  Fixture fixture;
  auto first = initialize_exact_direct_k1_normalized_product_stream(
      fixture.index,
      fixture.cloud,
      fixture.boruvka,
      fixture.k1_budget,
      fixture.product_budget);
  auto second = initialize_exact_direct_k1_normalized_product_stream(
      fixture.index,
      fixture.cloud,
      fixture.boruvka,
      fixture.k1_budget,
      fixture.product_budget);
  check(
      first.certified_initialized_stream() &&
          second.certified_initialized_stream() &&
          first.session.current_stamp().session_instance_id !=
              second.session.current_stamp().session_instance_id &&
          first.session.source_forest_digest() ==
              second.session.source_forest_digest() &&
          first.session.scientific_digest() ==
              second.session.scientific_digest() &&
          first.session.scientific_chain_digest() ==
              second.session.scientific_chain_digest(),
      "fresh streams did not separate live capabilities from stable science");

  auto sibling_a = first.session.prepare_next();
  auto sibling_b = first.session.prepare_next();
  check(
      sibling_a.certified_prepared_batch() &&
          sibling_b.certified_prepared_batch() &&
          sibling_a.ticket->prepared_record() ==
              sibling_b.ticket->prepared_record() &&
          first.session.outstanding_prepared_batch_count() == 2U &&
          first.session.prepare_next().decision ==
              ExactDirectK1NormalizedProductPrepareDecision::
                  no_outstanding_ticket_budget_exhausted,
      "bounded sibling tickets were not identical or capacity guarded");
  const auto first_commit =
      first.session.commit(std::move(*sibling_a.ticket));
  const std::size_t cursor_after_first = first.session.next_batch_cursor();
  const auto stale_commit =
      first.session.commit(std::move(*sibling_b.ticket));
  check(
      first_commit.certified_atomic_batch_commit() &&
          stale_commit.decision ==
              ExactDirectK1NormalizedProductCommitDecision::
                  no_stale_sibling_ticket_rejected &&
          stale_commit.ticket_consumed && !stale_commit.state_mutated &&
          first.session.next_batch_cursor() == cursor_after_first &&
          first.session.outstanding_prepared_batch_count() == 0U,
      "committing one sibling did not make the other stale without mutation");

  auto foreign = first.session.prepare_next();
  const auto foreign_commit =
      second.session.commit(std::move(*foreign.ticket));
  check(
      foreign_commit.decision ==
              ExactDirectK1NormalizedProductCommitDecision::
                  no_foreign_ticket_rejected &&
          foreign_commit.ticket_consumed && !foreign_commit.state_mutated &&
          first.session.outstanding_prepared_batch_count() == 0U &&
          second.session.next_batch_cursor() == 0U,
      "a foreign prepared capability changed scientific state or stayed live");

  auto aligned_first = initialize_exact_direct_k1_normalized_product_stream(
      fixture.index,
      fixture.cloud,
      fixture.boruvka,
      fixture.k1_budget,
      fixture.product_budget);
  auto aligned_second = initialize_exact_direct_k1_normalized_product_stream(
      fixture.index,
      fixture.cloud,
      fixture.boruvka,
      fixture.k1_budget,
      fixture.product_budget);
  while (!aligned_first.session.complete()) {
    auto prepared_first = aligned_first.session.prepare_next();
    auto prepared_second = aligned_second.session.prepare_next();
    check(
        prepared_first.ticket->prepared_record() ==
            prepared_second.ticket->prepared_record(),
        "fresh replay prepared a different canonical record");
    const auto committed_first =
        aligned_first.session.commit(std::move(*prepared_first.ticket));
    const auto committed_second =
        aligned_second.session.commit(std::move(*prepared_second.ticket));
    check(
        committed_first.record == committed_second.record &&
            committed_first.post_stamp.committed_scientific_chain_digest ==
                committed_second.post_stamp.committed_scientific_chain_digest,
        "fresh replay diverged in its process-independent batch chain");
  }
}

void test_preflight_cap_minus_one() {
  Fixture fixture;
  auto insufficient = fixture.product_budget;
  --insufficient.maximum_child_reference_count;
  const auto rejected = initialize_exact_direct_k1_normalized_product_stream(
      fixture.index,
      fixture.cloud,
      fixture.boruvka,
      fixture.k1_budget,
      insufficient);
  check(
      !rejected.certified_initialized_stream() &&
          rejected.decision ==
              ExactDirectK1NormalizedProductStreamInitializationDecision::
                  no_preflight_budget_exhausted &&
          !rejected.allocation_free_structural_preflight_certified &&
          !rejected.boruvka_authority_freshly_replayed &&
          !rejected.session.certified_stream(),
      "child-reference cap minus one did not reject before fresh replay");
}

void test_semantic_checkpoint_restore_and_continuation() {
  Fixture fixture;
  auto uninterrupted = initialize_exact_direct_k1_normalized_product_stream(
      fixture.index,
      fixture.cloud,
      fixture.boruvka,
      fixture.k1_budget,
      fixture.product_budget);
  for (std::size_t index = 0U; index < 2U; ++index) {
    auto prepared = uninterrupted.session.prepare_next();
    check(
        uninterrupted.session
            .commit(std::move(*prepared.ticket))
            .certified_atomic_batch_commit(),
        "the checkpoint fixture did not reach its mid-stream prefix");
  }
  const auto checkpoint = uninterrupted.session.export_checkpoint();
  check(
      checkpoint.certified_checkpoint() &&
          uninterrupted.session.verify_checkpoint(*checkpoint.checkpoint),
      "the semantic K1 product checkpoint was not exported and verified");

  auto restored = restore_exact_direct_k1_normalized_product_stream(
      fixture.index,
      fixture.cloud,
      fixture.boruvka,
      *checkpoint.checkpoint,
      fixture.k1_budget,
      fixture.product_budget);
  check(
      restored.certified_restored_stream() &&
          restored.receipt.decision ==
              ExactDirectK1NormalizedProductRestoreDecision::
                  complete_certified_fresh_semantic_replay_restore &&
          restored.receipt.replayed_product_batch_count == 2U &&
          restored.receipt.checkpoint_stamp.session_instance_id !=
              restored.receipt.restored_stamp.session_instance_id &&
          !restored.receipt.durable_file_codec_claimed,
      "restore did not fresh-replay Boruvka, forest, and product prefix");
  const auto restored_checkpoint = restored.session.export_checkpoint();
  check(
      restored_checkpoint.certified_checkpoint() &&
          same_semantic_checkpoint(
              *checkpoint.checkpoint, *restored_checkpoint.checkpoint),
      "the fresh restored checkpoint is not semantically identical");

  while (!uninterrupted.session.complete()) {
    auto left_prepared = uninterrupted.session.prepare_next();
    auto right_prepared = restored.session.prepare_next();
    check(
        left_prepared.ticket->prepared_record() ==
            right_prepared.ticket->prepared_record(),
        "restored continuation prepared a different exact K1 batch");
    static_cast<void>(uninterrupted.session.commit(
        std::move(*left_prepared.ticket)));
    static_cast<void>(restored.session.commit(
        std::move(*right_prepared.ticket)));
  }
  check(
      uninterrupted.session.scientific_chain_digest() ==
          restored.session.scientific_chain_digest() &&
          uninterrupted.session.root_node_id() ==
              restored.session.root_node_id(),
      "restored terminal science diverged from uninterrupted execution");

  auto mutated = *checkpoint.checkpoint;
  ++mutated.stamp.next_batch_cursor;
  const auto rejected_mutation =
      restore_exact_direct_k1_normalized_product_stream(
          fixture.index,
          fixture.cloud,
          fixture.boruvka,
          mutated,
          fixture.k1_budget,
          fixture.product_budget);
  check(
      rejected_mutation.receipt.decision ==
          ExactDirectK1NormalizedProductRestoreDecision::
              no_checkpoint_shape_rejected,
      "a cursor mutation survived the semantic checkpoint digest");

  auto replay_limited = fixture.product_budget;
  replay_limited.maximum_checkpoint_replay_batch_count = 1U;
  const auto rejected_replay_cap =
      restore_exact_direct_k1_normalized_product_stream(
          fixture.index,
          fixture.cloud,
          fixture.boruvka,
          *checkpoint.checkpoint,
          fixture.k1_budget,
          replay_limited);
  check(
      rejected_replay_cap.receipt.decision ==
          ExactDirectK1NormalizedProductRestoreDecision::
              no_checkpoint_replay_budget_exhausted,
      "checkpoint replay cap minus one did not fail before fresh replay");
}

void test_at_least20_transition_is_compactly_derivable() {
  CanonicalPointCloud cloud = threshold_cloud();
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  K1ExactBoruvkaResult boruvka = build_exact_lbvh_boruvka(index, cloud);
  auto initialization = initialize_exact_direct_k1_normalized_product_stream(
      index,
      cloud,
      boruvka,
      authority_budget(cloud.size()),
      stream_budget(cloud.size()));
  check(
      initialization.certified_initialized_stream(),
      "the 20-point threshold fixture did not initialize");
  auto genesis = initialization.session.prepare_next();
  const ExactDirectK1NormalizedProductBatchRecord zero =
      genesis.ticket->prepared_record();
  std::size_t seeded_singletons = 0U;
  constexpr std::size_t chunk_cap = 7U;
  while (seeded_singletons < cloud.size()) {
    const std::size_t count =
        std::min(chunk_cap, cloud.size() - seeded_singletons);
    const auto slice = initialization.session.genesis_slice(
        zero, seeded_singletons, count, chunk_cap);
    check(slice.certified(), "a bounded threshold genesis slice failed");
    seeded_singletons += slice.count;
  }
  static_cast<void>(
      initialization.session.commit(std::move(*genesis.ticket)));
  auto threshold = initialization.session.prepare_next();
  const auto& record = threshold.ticket->prepared_record();
  const auto view = initialization.session.batch_view(record);
  const auto transitions = threshold.ticket->at_least20_transitions();
  check(
      seeded_singletons == 20U && view.has_value() &&
          view->merge_nodes.size() == 1U &&
          view->merge_nodes.front().coverage_size == 20U &&
          view->merge_nodes.front().child_count == 20U &&
          view->child_root_ids.size() == 20U && record.group_count == 1U &&
          record.qr_greater_equal2_group_count == 1U &&
          record.parent_delta_count == 20U && record.node_delta_count == 1U &&
          record.point_delta_count == 0U &&
          transitions.size() == 1U &&
          transitions.front().kind ==
              ExactDirectK1NormalizedAtLeast20TransitionKind::threshold_entry &&
          transitions.front().q_visible == 0U &&
          transitions.front().coverage_size == 20U &&
          threshold.ticket->at_least20_visible_child_root_ids().empty() &&
          view->at_least20_prior_roots_are_csr_children &&
          view->at_least20_coverage_delta_is_empty_after_genesis,
      "the >=20 threshold entry is not derivable from compact genesis plus CSR");
  const auto threshold_commit =
      initialization.session.commit(std::move(*threshold.ticket));
  check(
      threshold_commit.certified_atomic_batch_commit() &&
          initialization.session.active_at_least20_root_count() == 1U &&
          initialization.session.at_least20_root_active(
              initialization.session.root_node_id()) &&
          threshold_commit.post_stamp.at_least20_view_digest ==
              initialization.session.at_least20_view_digest(),
      "the complete at_least20 threshold state was not atomically published");

  bool duplicate_rejected = false;
  try {
    const std::array<CertifiedPoint3, 2U> duplicates{point(1.0), point(1.0)};
    static_cast<void>(CanonicalPointCloud::rejecting_duplicates(
        std::span<const CertifiedPoint3>{duplicates}));
  } catch (const std::invalid_argument&) {
    duplicate_rejected = true;
  }
  check(
      duplicate_rejected,
      "the canonical cloud unexpectedly admitted a zero-length duplicate edge");
}

void test_complete_at_least20_threshold_continuation_multifusion_state() {
  CanonicalPointCloud cloud = visible_transition_cloud();
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  K1ExactBoruvkaResult boruvka = build_exact_lbvh_boruvka(index, cloud);
  const auto k1_budget = authority_budget(cloud.size());
  const auto product_budget = stream_budget(cloud.size());
  auto initialization = initialize_exact_direct_k1_normalized_product_stream(
      index, cloud, boruvka, k1_budget, product_budget);
  check(
      initialization.certified_initialized_stream() &&
          initialization.at_least20_complete_compact_view_certified,
      "the full at_least20 transition fixture did not initialize");

  std::size_t threshold_count = 0U;
  std::size_t continuation_count = 0U;
  std::size_t multifusion_count = 0U;
  while (!initialization.session.complete()) {
    auto prepared = initialization.session.prepare_next();
    check(
        prepared.certified_prepared_batch(),
        "an at_least20 lifecycle batch was not prepared");
    if (!prepared.certified_prepared_batch()) {
      break;
    }
    const auto transitions = prepared.ticket->at_least20_transitions();
    const auto visible_children =
        prepared.ticket->at_least20_visible_child_root_ids();
    for (const auto& transition : transitions) {
      check(
          transition.visible_child_reference_count == transition.q_visible &&
              transition.visible_child_reference_offset <=
                  visible_children.size() &&
              transition.visible_child_reference_count <=
                  visible_children.size() -
                      transition.visible_child_reference_offset,
          "an at_least20 transition does not own an exact visible-child slice");
      const auto child_slice = visible_children.subspan(
          transition.visible_child_reference_offset,
          transition.visible_child_reference_count);
      for (const auto child_id : child_slice) {
        check(
            initialization.session.at_least20_root_active(child_id),
            "a qVisible child was not active in the strict pre-batch state");
      }
      switch (transition.kind) {
        case ExactDirectK1NormalizedAtLeast20TransitionKind::threshold_entry:
          ++threshold_count;
          check(
              transition.q_visible == 0U,
              "a threshold entry has a nonzero qVisible");
          break;
        case ExactDirectK1NormalizedAtLeast20TransitionKind::continuation:
          ++continuation_count;
          check(
              transition.q_visible == 1U,
              "a continuation does not have qVisible=1");
          break;
        case ExactDirectK1NormalizedAtLeast20TransitionKind::multifusion:
          ++multifusion_count;
          check(
              transition.q_visible >= 2U,
              "a visible multifusion has qVisible below two");
          break;
      }
    }
    const auto committed =
        initialization.session.commit(std::move(*prepared.ticket));
    check(
        committed.certified_atomic_batch_commit(),
        "the at_least20 lifecycle batch did not commit atomically");
    for (const auto& transition : committed.at_least20_transitions) {
      check(
          initialization.session.at_least20_root_active(
              transition.resultant_root_id),
          "a visible resultant was not active after its commit");
      const auto child_slice =
          std::span<const morsehgp3d::hierarchy::K1NodeId>{
              committed.at_least20_visible_child_root_ids}
              .subspan(
                  transition.visible_child_reference_offset,
                  transition.visible_child_reference_count);
      for (const auto child_id : child_slice) {
        check(
            !initialization.session.at_least20_root_active(child_id),
            "a consumed visible child remained active after its parent commit");
      }
    }
  }
  check(
      threshold_count == 2U && continuation_count == 1U &&
          multifusion_count == 1U &&
          initialization.session.active_at_least20_root_count() == 1U &&
          initialization.session.at_least20_root_active(
              initialization.session.root_node_id()),
      "threshold, continuation, and multifusion did not close to one visible root");
  check(
      initialization.session.seal().certified_terminal_closure(),
      "the terminal at_least20 keyed state did not close the product seal");

  const auto checkpoint = initialization.session.export_checkpoint();
  check(
      checkpoint.certified_checkpoint() &&
          checkpoint.checkpoint->active_at_least20_root_ids.size() == 1U &&
          checkpoint.checkpoint->active_at_least20_root_ids.front() ==
              initialization.session.root_node_id(),
      "the at_least20 active keyed state is absent from the checkpoint");
  auto restored = restore_exact_direct_k1_normalized_product_stream(
      index,
      cloud,
      boruvka,
      *checkpoint.checkpoint,
      k1_budget,
      product_budget);
  check(
      restored.certified_restored_stream() &&
          restored.receipt.at_least20_view_freshly_replayed &&
          restored.session.active_at_least20_root_count() == 1U &&
          restored.session.at_least20_root_active(
              restored.session.root_node_id()) &&
          restored.session.at_least20_view_digest() ==
              initialization.session.at_least20_view_digest(),
      "fresh semantic restore did not reproduce the complete at_least20 state");
}

}  // namespace

int main() {
  test_unique_closed_zero_batch_and_positive_equal_levels();
  test_fresh_digest_determinism_and_sibling_ticket_scope();
  test_preflight_cap_minus_one();
  test_semantic_checkpoint_restore_and_continuation();
  test_at_least20_transition_is_compactly_derivable();
  test_complete_at_least20_threshold_continuation_multifusion_state();
  if (failures != 0) {
    std::cerr << failures << " direct K1 normalized product test(s) failed\n";
    return 1;
  }
  return 0;
}
