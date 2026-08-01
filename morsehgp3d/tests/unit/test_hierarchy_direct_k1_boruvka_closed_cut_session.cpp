#include "morsehgp3d/hierarchy/direct_k1_boruvka_closed_cut_session.hpp"

#include "morsehgp3d/hierarchy/k1_forest.hpp"

#include <array>
#include <cstddef>
#include <iostream>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::
    ExactDirectK1BoruvkaClosedCutAdvanceDecision;
using morsehgp3d::hierarchy::
    ExactDirectK1BoruvkaClosedCutInitializationDecision;
using morsehgp3d::hierarchy::
    ExactDirectK1BoruvkaClosedCutRootQueryDecision;
using morsehgp3d::hierarchy::
    ExactDirectK1BoruvkaClosedCutSession;
using morsehgp3d::hierarchy::
    ExactDirectK1BoruvkaClosedCutSessionBudget;
using morsehgp3d::hierarchy::K1CompactForest;
using morsehgp3d::hierarchy::K1ExactBoruvkaResult;
using morsehgp3d::hierarchy::build_compact_k1_forest;
using morsehgp3d::hierarchy::
    build_exact_direct_k1_boruvka_closed_cut_session;
using morsehgp3d::hierarchy::build_exact_lbvh_boruvka;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

static_assert(!std::is_copy_constructible_v<
              ExactDirectK1BoruvkaClosedCutSession>);
static_assert(std::is_nothrow_move_constructible_v<
              ExactDirectK1BoruvkaClosedCutSession>);

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

[[nodiscard]] CanonicalPointCloud chain_cloud() {
  const std::array<CertifiedPoint3, 4> points{
      point(0.0), point(1.0), point(3.0), point(7.0)};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactDirectK1BoruvkaClosedCutSessionBudget budget_for(
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

struct Fixture {
  CanonicalPointCloud cloud{chain_cloud()};
  MortonLbvhIndex index{MortonLbvhIndex::build(cloud)};
  K1ExactBoruvkaResult boruvka{build_exact_lbvh_boruvka(index, cloud)};
  K1CompactForest forest{
      build_compact_k1_forest(cloud.size(), boruvka.emst_edges)};
  ExactDirectK1BoruvkaClosedCutSessionBudget budget{
      budget_for(cloud.size())};
};

void test_fresh_seal_and_forged_source_rejection() {
  Fixture fixture;
  auto initialization = build_exact_direct_k1_boruvka_closed_cut_session(
      fixture.index, fixture.cloud, fixture.boruvka, fixture.budget);
  check(
      initialization.certified_ready_session() &&
          initialization.boruvka_authority_freshly_replayed &&
          initialization.compact_k1_equal_level_forest_certified &&
          initialization.checkpoint_semantic_state_ready,
      "fresh Boruvka replay is the only route to a sealed ready session");
  check(
      initialization.requirements.point_count == fixture.cloud.size() &&
          initialization.requirements.source_edge_count ==
              fixture.cloud.size() - 1U &&
          initialization.requirements.checkpoint_state_entry_count ==
              2U * fixture.cloud.size(),
      "allocation-free preflight publishes conservative structural sizes");

  const auto initial_stamp = initialization.session.current_stamp();
  check(
      initial_stamp.session_instance_id != 0U &&
          initial_stamp.committed_level_cursor == 0U &&
          initial_stamp.closed_squared_level == ExactLevel{} &&
          initialization.session.verify_stamp(initial_stamp),
      "initial closed zero cut has a live nonzero sealed stamp");
  for (std::size_t point_index = 0U;
       point_index < fixture.cloud.size();
       ++point_index) {
    const auto query = initialization.session.query_singleton_root(
        static_cast<PointId>(point_index), initial_stamp);
    check(
        query.decision ==
                ExactDirectK1BoruvkaClosedCutRootQueryDecision::
                    complete_certified_singleton_closed_root &&
            query.closed_root_node_id == point_index &&
            initialization.session.verify_root_query_result(query),
        "the zero cut binds every singleton to its implicit K1 leaf");
  }

  auto checkpoint = initialization.session.export_checkpoint();
  check(
      initialization.session.verify_checkpoint(checkpoint.checkpoint),
      "checkpoint-ready semantic state verifies through the live capability");

  auto second_initialization =
      build_exact_direct_k1_boruvka_closed_cut_session(
          fixture.index, fixture.cloud, fixture.boruvka, fixture.budget);
  const auto second_checkpoint =
      second_initialization.session.export_checkpoint();
  check(
      checkpoint.checkpoint.stamp.session_instance_id !=
              second_checkpoint.checkpoint.stamp.session_instance_id &&
          checkpoint.checkpoint.state_digest ==
              second_checkpoint.checkpoint.state_digest,
      "fresh sessions keep distinct live ids but one process-independent checkpoint digest");
  checkpoint.checkpoint.closed_root_node_id_by_point.front() += 1U;
  check(
      !initialization.session.verify_checkpoint(checkpoint.checkpoint),
      "a forged checkpoint root is rejected");

  K1ExactBoruvkaResult forged = fixture.boruvka;
  forged.emst_witness_certified = true;
  forged.emst_edges.front().merge_level = ExactLevel{999};
  const auto forged_initialization =
      build_exact_direct_k1_boruvka_closed_cut_session(
          fixture.index, fixture.cloud, forged, fixture.budget);
  check(
      !forged_initialization.certified_ready_session() &&
          forged_initialization.decision ==
              ExactDirectK1BoruvkaClosedCutInitializationDecision::
                  no_boruvka_authority_rejected,
      "a caller-forged Boruvka flag and edge cannot mint authority");
}

void test_intermediate_levels_equal_cuts_and_stamp_scope() {
  Fixture fixture;
  check(
      fixture.forest.levels.size() == 3U,
      "the chain fixture exposes three distinct exact K1 levels");
  auto initialization = build_exact_direct_k1_boruvka_closed_cut_session(
      fixture.index, fixture.cloud, fixture.boruvka, fixture.budget);
  auto foreign_initialization =
      build_exact_direct_k1_boruvka_closed_cut_session(
          fixture.index, fixture.cloud, fixture.boruvka, fixture.budget);
  const auto initial_stamp = initialization.session.current_stamp();
  const auto foreign_stamp = foreign_initialization.session.current_stamp();
  check(
      initial_stamp.session_instance_id != foreign_stamp.session_instance_id &&
          initialization.session
                  .query_singleton_root(PointId{0U}, foreign_stamp)
                  .decision ==
              ExactDirectK1BoruvkaClosedCutRootQueryDecision::
                  no_stale_or_foreign_stamp,
      "a stamp from another freshly verified session is still foreign");

  auto first_receipt = initialization.session.advance_closed_to(
      ExactLevel{
          fixture.forest.levels.front().numerator() * 2,
          fixture.forest.levels.front().denominator() * 2});
  check(
      first_receipt.decision ==
              ExactDirectK1BoruvkaClosedCutAdvanceDecision::
                  complete_certified_monotone_closed_cut &&
          first_receipt.consumed_intermediate_level_count == 1U &&
          initialization.session.verify_advance_receipt(first_receipt),
      "an exactly equal rational cut consumes its whole equality batch");
  const auto equal_stamp = initialization.session.current_stamp();
  const auto equal_receipt = initialization.session.advance_closed_to(
      fixture.forest.levels.front());
  check(
      equal_receipt.decision ==
              ExactDirectK1BoruvkaClosedCutAdvanceDecision::
                  complete_certified_monotone_closed_cut &&
          equal_receipt.consumed_intermediate_level_count == 0U &&
          !equal_receipt.state_mutated &&
          equal_receipt.pre_stamp == equal_stamp &&
          equal_receipt.post_stamp == equal_stamp &&
          initialization.session.verify_advance_receipt(equal_receipt),
      "repeating the same exact closed cut is idempotent");
  check(
      initialization.session
              .query_singleton_root(PointId{0U}, initial_stamp)
              .decision ==
          ExactDirectK1BoruvkaClosedCutRootQueryDecision::
              no_stale_or_foreign_stamp,
      "a previously live stamp becomes stale after a cut advance");

  auto forged_stamp = equal_stamp;
  ++forged_stamp.session_instance_id;
  check(
      initialization.session
              .query_singleton_root(PointId{0U}, forged_stamp)
              .decision ==
          ExactDirectK1BoruvkaClosedCutRootQueryDecision::
              no_stale_or_foreign_stamp,
      "a forged instance id is rejected before the root query");

  const auto jump_receipt = initialization.session.advance_closed_to(
      fixture.forest.levels.back());
  check(
      jump_receipt.consumed_intermediate_level_count == 2U &&
          jump_receipt.post_stamp.committed_level_cursor ==
              fixture.forest.levels.size() &&
          initialization.session.verify_advance_receipt(jump_receipt),
      "a direct jump consumes every unvisited intermediate K1 level");
  const auto final_stamp = initialization.session.current_stamp();
  for (std::size_t point_index = 0U;
       point_index < fixture.cloud.size();
       ++point_index) {
    const auto query = initialization.session.query_singleton_root(
        static_cast<PointId>(point_index), final_stamp);
    check(
        query.closed_root_node_id == fixture.forest.root_node_id &&
            initialization.session.verify_root_query_result(query),
        "the terminal cut maps every singleton to the compact K1 root node");
  }
  auto forged_root = initialization.session.query_singleton_root(
      PointId{0U}, final_stamp);
  ++*forged_root.closed_root_node_id;
  check(
      !initialization.session.verify_root_query_result(forged_root),
      "a forged singleton-to-root result is rejected by live replay");

  const auto before_rejection = initialization.session.current_stamp();
  const auto nonmonotone =
      initialization.session.advance_closed_to(ExactLevel{});
  check(
      nonmonotone.decision ==
              ExactDirectK1BoruvkaClosedCutAdvanceDecision::
                  no_nonmonotone_closed_cut &&
          !nonmonotone.state_mutated &&
          nonmonotone.pre_stamp == before_rejection &&
          nonmonotone.post_stamp == before_rejection &&
          initialization.session.current_stamp() == before_rejection &&
          initialization.session.verify_advance_receipt(nonmonotone),
      "a nonmonotone cut is a certified non-mutating rejection");
}

void test_cap_minus_one_rejects_before_authority_replay() {
  Fixture fixture;
  const auto source_edges = fixture.boruvka.emst_edges;
  const auto source_rounds = fixture.boruvka.rounds;
  auto insufficient = fixture.budget;
  insufficient.maximum_parent_state_entry_count = fixture.cloud.size() - 1U;
  const auto rejected = build_exact_direct_k1_boruvka_closed_cut_session(
      fixture.index, fixture.cloud, fixture.boruvka, insufficient);
  check(
      rejected.decision ==
              ExactDirectK1BoruvkaClosedCutInitializationDecision::
                  no_preflight_budget_exhausted &&
          !rejected.allocation_free_structural_preflight_certified &&
          !rejected.boruvka_authority_freshly_replayed &&
          !rejected.session.ready(),
      "parent-state cap minus one rejects before fresh replay or allocation");
  check(
      fixture.boruvka.emst_edges == source_edges &&
          fixture.boruvka.rounds == source_rounds,
      "preflight rejection does not mutate the supplied Boruvka witness");
}

}  // namespace

int main() {
  test_fresh_seal_and_forged_source_rejection();
  test_intermediate_levels_equal_cuts_and_stamp_scope();
  test_cap_minus_one_rejects_before_authority_replay();
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "direct K1 Boruvka closed-cut session tests passed\n";
  return 0;
}
