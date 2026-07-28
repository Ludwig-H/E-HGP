#include "fake_gpu_pair_support_phi_launchers.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/gpu/pair_support_phi.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::gpu::PairSupportPhiBatchResult;
using morsehgp3d::gpu::PairSupportPhiContext;
using morsehgp3d::gpu::PairSupportPhiDecision;
using morsehgp3d::gpu::PairSupportPhiDecisionRecord;
using morsehgp3d::gpu::PairSupportPhiProposalKind;
using morsehgp3d::gpu::PairSupportPhiProposalRecord;
using morsehgp3d::gpu::PairSupportPhiWitnessQuery;
using morsehgp3d::gpu::PairSupportRankPruneBatchResult;
using morsehgp3d::gpu::PairSupportRankPruneBudget;
using morsehgp3d::gpu::PairSupportRankPruneCapacity;
using morsehgp3d::gpu::PairSupportRankTerminalKind;
using morsehgp3d::gpu::PairSupportRankTraversalBackend;
using morsehgp3d::gpu::test_support::
    FakePairSupportPhiCorruption;
using morsehgp3d::gpu::test_support::
    configure_fake_gpu_pair_support_phi;
using morsehgp3d::gpu::test_support::
    fake_gpu_pair_support_phi_last_node_count;
using morsehgp3d::gpu::test_support::
    fake_gpu_pair_support_phi_last_query_count;
using morsehgp3d::gpu::test_support::
    fake_gpu_pair_support_phi_last_record_capacity;
using morsehgp3d::gpu::test_support::
    fake_gpu_pair_support_phi_launch_count;
using morsehgp3d::gpu::test_support::
    fake_gpu_pair_support_rank_last_product_count;
using morsehgp3d::gpu::test_support::
    fake_gpu_pair_support_rank_last_receipt_capacity;
using morsehgp3d::gpu::test_support::
    fake_gpu_pair_support_rank_last_work_item_capacity;
using morsehgp3d::gpu::test_support::
    fake_gpu_pair_support_rank_launch_count;
using morsehgp3d::hierarchy::ExactPairSupportFrontierEntry;
using morsehgp3d::gpu::test_support::reset_fake_gpu_pair_support_phi;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;

static_assert(!std::is_copy_constructible_v<PairSupportPhiContext>);
static_assert(!std::is_copy_assignable_v<PairSupportPhiContext>);
static_assert(std::is_nothrow_move_constructible_v<PairSupportPhiContext>);
static_assert(std::is_nothrow_move_assignable_v<PairSupportPhiContext>);

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
    std::cerr << "FAIL: " << message << " (unexpected exception: "
              << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] CanonicalPointCloud line_cloud() {
  const std::array points{
      CertifiedPoint3::from_binary64(-1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(0.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(1.0, 0.0, 0.0)};
  return CanonicalPointCloud::rejecting_duplicates(points);
}

[[nodiscard]] CanonicalPointCloud five_point_line_cloud() {
  const std::array points{
      CertifiedPoint3::from_binary64(-2.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(-1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(0.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(2.0, 0.0, 0.0)};
  return CanonicalPointCloud::rejecting_duplicates(points);
}

[[nodiscard]] CanonicalPointCloud anchor_culling_line_cloud() {
  const std::array points{
      CertifiedPoint3::from_binary64(-1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(32.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(33.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(34.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(35.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(36.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(37.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(38.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(39.0, 0.0, 0.0)};
  return CanonicalPointCloud::rejecting_duplicates(points);
}

[[nodiscard]] CanonicalPointCloud anchor_shell_cloud() {
  const std::array points{
      CertifiedPoint3::from_binary64(-1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(1.0, 0.0, 0.0),
      CertifiedPoint3::from_binary64(0.0, -1.0, 0.0),
      CertifiedPoint3::from_binary64(0.0, 1.0, 0.0)};
  return CanonicalPointCloud::rejecting_duplicates(points);
}

struct Fixture {
  Fixture() : cloud(line_cloud()), index(MortonLbvhIndex::build(cloud)) {}

  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
};

[[nodiscard]] auto query_key(const PairSupportPhiWitnessQuery& query) {
  return std::tuple{
      query.first_support_node_index,
      query.second_support_node_index,
      query.witness_node_index};
}

[[nodiscard]] auto product_key(
    const ExactPairSupportFrontierEntry& product) {
  return std::tuple{
      product.first_leaf_begin,
      product.first_leaf_end,
      product.second_leaf_begin,
      product.second_leaf_end,
      product.first_node_index,
      product.second_node_index,
      product.self_product};
}

[[nodiscard]] ExactPairSupportFrontierEntry product_from_query(
    const PairSupportPhiContext& context,
    const PairSupportPhiWitnessQuery& query) {
  const auto first = context.node_descriptor(
      static_cast<std::size_t>(query.first_support_node_index));
  const auto second = context.node_descriptor(
      static_cast<std::size_t>(query.second_support_node_index));
  return ExactPairSupportFrontierEntry{
      first.node_index,
      second.node_index,
      first.leaf_begin,
      first.leaf_end,
      second.leaf_begin,
      second.leaf_end,
      0U};
}

struct FixtureQueries {
  PairSupportPhiWitnessQuery strict;
  PairSupportPhiWitnessQuery descend;
  std::array<PairSupportPhiWitnessQuery, 2U> canonical;
};

[[nodiscard]] FixtureQueries fixture_queries(
    const PairSupportPhiContext& context) {
  FixtureQueries result{
      context.make_leaf_witness_query(0U, 2U, 1U),
      context.make_leaf_witness_query(0U, 1U, 2U),
      {}};
  result.canonical = {result.strict, result.descend};
  std::sort(
      result.canonical.begin(),
      result.canonical.end(),
      [](const auto& left, const auto& right) {
        return query_key(left) < query_key(right);
      });
  return result;
}

[[nodiscard]] const PairSupportPhiProposalRecord* find_proposal(
    const PairSupportPhiBatchResult& result,
    const PairSupportPhiWitnessQuery& query) {
  const auto found = std::find_if(
      result.proposals.begin(),
      result.proposals.end(),
      [&query](const auto& record) { return record.query == query; });
  return found == result.proposals.end() ? nullptr : &*found;
}

[[nodiscard]] const PairSupportPhiDecisionRecord* find_decision(
    const PairSupportPhiBatchResult& result,
    const PairSupportPhiWitnessQuery& query) {
  const auto found = std::find_if(
      result.decisions.begin(),
      result.decisions.end(),
      [&query](const auto& record) { return record.query == query; });
  return found == result.decisions.end() ? nullptr : &*found;
}

void check_healthy_result(
    const PairSupportPhiBatchResult& result,
    const FixtureQueries& queries,
    std::uint64_t expected_epoch,
    std::size_t expected_capacity,
    const std::string& label) {
  const PairSupportPhiProposalRecord* strict_proposal =
      find_proposal(result, queries.strict);
  const PairSupportPhiProposalRecord* descend_proposal =
      find_proposal(result, queries.descend);
  const PairSupportPhiDecisionRecord* strict =
      find_decision(result, queries.strict);
  const PairSupportPhiDecisionRecord* descend =
      find_decision(result, queries.descend);
  check(
      strict_proposal != nullptr &&
          strict_proposal->kind ==
              PairSupportPhiProposalKind::proposed_strict_interior &&
          ExactRational::from_binary64_bits(
              strict_proposal->upper_phi_binary64_bits) ==
              ExactRational{-1},
      label + " proposes the exact strict upper bound -1");
  check(
      descend_proposal != nullptr &&
          descend_proposal->kind ==
              PairSupportPhiProposalKind::requires_descent &&
          ExactRational::from_binary64_bits(
              descend_proposal->upper_phi_binary64_bits) ==
              ExactRational{2},
      label + " preserves the exact positive value 2 as a descent");
  check(
      strict != nullptr &&
          strict->decision ==
              PairSupportPhiDecision::certified_strict_interior &&
          strict->exact_receipt.has_value() &&
          strict->exact_receipt->exact_phi_maximum.maximum_phi ==
              ExactRational{-1},
      label + " publishes strict interior only after exact CPU replay");
  check(
      descend != nullptr &&
          descend->decision == PairSupportPhiDecision::descend &&
          !descend->exact_receipt.has_value(),
      label + " never fabricates an exact receipt for descent");
  if (strict != nullptr && strict->exact_receipt.has_value()) {
    const auto descriptor = strict->exact_receipt->witness_node;
    check(
        descriptor.node_index == queries.strict.witness_node_index &&
            descriptor.leaf_begin < descriptor.leaf_end,
        label + " retains a fixed-width checkpoint witness identity");
  }

  const auto& audit = result.audit;
  check(
      result.proposals.size() == 2U && result.decisions.size() == 2U &&
          audit.resident_lbvh_node_count == 5U &&
          audit.maximum_query_count == expected_capacity &&
          audit.canonical_query_count == 2U &&
          audit.gpu_output_record_count == 2U &&
          audit.gpu_strict_interior_proposal_count == 1U &&
          audit.gpu_requires_descent_count == 1U &&
          audit.gpu_kernel_launch_count == 1U &&
          audit.cpu_exact_phi_recertification_count == 1U &&
          audit.certified_strict_interior_receipt_count == 1U &&
          audit.buffer_epoch == expected_epoch &&
          audit.minimum_certified_strict_margin.has_value() &&
          *audit.minimum_certified_strict_margin == ExactRational{1},
      label + " closes all bounded proposal and receipt counters");
  check(
      audit.immutable_lbvh_snapshot_validated &&
          audit.canonical_query_order_validated &&
          audit.exhaustive_proposal_permutation_validated &&
          audit.cpu_exact_recertification_complete &&
          !audit.global_support_product_prune_published &&
          !audit.public_status_published,
      label + " separates GPU proposal, CPU decision, and public status");
}

void test_exact_fixture_and_two_epochs() {
  reset_fake_gpu_pair_support_phi();
  Fixture fixture;
  PairSupportPhiContext context{fixture.index, fixture.cloud, 3U};
  const FixtureQueries queries = fixture_queries(context);
  const PairSupportPhiBatchResult first =
      context.classify_witnesses(std::span{queries.canonical});
  const PairSupportPhiBatchResult second =
      context.classify_witnesses(std::span{queries.canonical});

  check_healthy_result(first, queries, 1U, 3U, "the first epoch");
  check_healthy_result(second, queries, 2U, 3U, "the second epoch");
  check(
      first.proposals == second.proposals &&
          first.decisions == second.decisions &&
          first.audit.proposal_digest_fnv1a ==
              second.audit.proposal_digest_fnv1a,
      "two epochs preserve the canonical proposal digest and exact decisions");
  check(
      fake_gpu_pair_support_phi_launch_count() == 2U &&
          fake_gpu_pair_support_phi_last_node_count() == 5U &&
          fake_gpu_pair_support_phi_last_query_count() == 2U &&
          fake_gpu_pair_support_phi_last_record_capacity() == 3U,
      "the fake launcher sees only five resident nodes and bounded batch storage");
}

void test_preflight_validation_and_capacity() {
  reset_fake_gpu_pair_support_phi();
  Fixture fixture;
  PairSupportPhiContext context{fixture.index, fixture.cloud, 3U};
  const FixtureQueries queries = fixture_queries(context);

  const std::array<PairSupportPhiWitnessQuery, 0U> empty{};
  check_throws<std::invalid_argument>(
      [&] { static_cast<void>(context.classify_witnesses(empty)); },
      "an empty batch is rejected before launch");

  std::array reversed = queries.canonical;
  std::reverse(reversed.begin(), reversed.end());
  check_throws<std::invalid_argument>(
      [&] { static_cast<void>(context.classify_witnesses(reversed)); },
      "a reversed query batch is rejected before launch");

  const std::array duplicate{queries.strict, queries.strict};
  check_throws<std::invalid_argument>(
      [&] { static_cast<void>(context.classify_witnesses(duplicate)); },
      "a duplicate query is rejected before launch");

  PairSupportPhiWitnessQuery intersecting = queries.strict;
  intersecting.witness_node_index =
      intersecting.first_support_node_index;
  const std::array intersection{intersecting};
  check_throws<std::invalid_argument>(
      [&] { static_cast<void>(context.classify_witnesses(intersection)); },
      "a witness intersecting its support product is rejected before launch");

  PairSupportPhiWitnessQuery swapped = queries.strict;
  std::swap(
      swapped.first_support_node_index,
      swapped.second_support_node_index);
  const std::array reversed_supports{swapped};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(
            context.classify_witnesses(reversed_supports));
      },
      "a reversed support interval order is rejected before launch");

  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(
            context.make_leaf_witness_query(0U, 0U, 1U));
      },
      "a repeated leaf PointId is rejected before query formation");
  check(
      fake_gpu_pair_support_phi_launch_count() == 0U,
      "all canonical-input failures avoid a GPU launch");

  const auto healthy =
      context.classify_witnesses(std::span{queries.canonical});
  check_healthy_result(
      healthy, queries, 1U, 3U, "the post-validation batch");

  reset_fake_gpu_pair_support_phi();
  PairSupportPhiContext one_slot{fixture.index, fixture.cloud, 1U};
  const FixtureQueries one_slot_queries = fixture_queries(one_slot);
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(one_slot.classify_witnesses(
            std::span{one_slot_queries.canonical}));
      },
      "a two-query batch cannot exceed its fixed one-record capacity");
  const std::array one_query{one_slot_queries.strict};
  const auto within_capacity = one_slot.classify_witnesses(one_query);
  check(
      within_capacity.decisions.size() == 1U &&
          within_capacity.decisions[0U].decision ==
              PairSupportPhiDecision::certified_strict_interior &&
          fake_gpu_pair_support_phi_launch_count() == 1U &&
          fake_gpu_pair_support_phi_last_record_capacity() == 1U,
      "the capacity rejection does not poison one later in-cap batch");

  check_throws<std::invalid_argument>(
      [&] {
        PairSupportPhiContext zero_capacity{
            fixture.index, fixture.cloud, 0U};
      },
      "zero query capacity is rejected at construction");
  const CanonicalPointCloud equivalent_cloud = line_cloud();
  check_throws<std::invalid_argument>(
      [&] {
        PairSupportPhiContext mismatched{
            fixture.index, equivalent_cloud, 1U};
      },
      "an independently rebuilt cloud cannot borrow another LBVH authority");
}

void check_corruption_and_poisoning(
    FakePairSupportPhiCorruption corruption,
    std::size_t capacity,
    const std::string& label) {
  reset_fake_gpu_pair_support_phi();
  Fixture fixture;
  PairSupportPhiContext poisoned{fixture.index, fixture.cloud, capacity};
  const FixtureQueries queries = fixture_queries(poisoned);
  configure_fake_gpu_pair_support_phi(corruption);
  check_throws<std::runtime_error>(
      [&] {
        static_cast<void>(poisoned.classify_witnesses(
            std::span{queries.canonical}));
      },
      label + " is rejected fail-closed");
  configure_fake_gpu_pair_support_phi(
      FakePairSupportPhiCorruption::none);
  check_throws<std::runtime_error>(
      [&] {
        static_cast<void>(poisoned.classify_witnesses(
            std::span{queries.canonical}));
      },
      label + " poisons its resident context");
  check(
      fake_gpu_pair_support_phi_launch_count() == 1U,
      label + " prevents a second launch from the poisoned context");

  PairSupportPhiContext fresh{fixture.index, fixture.cloud, 2U};
  const FixtureQueries fresh_queries = fixture_queries(fresh);
  const auto recovered =
      fresh.classify_witnesses(std::span{fresh_queries.canonical});
  check_healthy_result(
      recovered, fresh_queries, 1U, 2U, label + " fresh context");
  check(
      fake_gpu_pair_support_phi_launch_count() == 2U,
      label + " remains isolated from a fresh resident context");
}

void test_corruption_matrix_and_poisoning() {
  const std::array cases{
      std::pair{FakePairSupportPhiCorruption::duplicate_transcript_query,
                "a duplicated transcript query"},
      std::pair{FakePairSupportPhiCorruption::changed_transcript_query,
                "a changed transcript identity"},
      std::pair{FakePairSupportPhiCorruption::false_strict_interior,
                "a false strict-interior proposal"},
      std::pair{FakePairSupportPhiCorruption::zero_epoch,
                "an unauthenticated zero epoch"},
      std::pair{FakePairSupportPhiCorruption::simulated_async_failure,
                "an asynchronous device failure"}};
  for (const auto& [corruption, label] : cases) {
    check_corruption_and_poisoning(corruption, 2U, label);
  }
  check_corruption_and_poisoning(
      FakePairSupportPhiCorruption::stale_tail,
      3U,
      "a write into the fixed-capacity tail");

  reset_fake_gpu_pair_support_phi();
  Fixture fixture;
  PairSupportPhiContext stale{fixture.index, fixture.cloud, 2U};
  const FixtureQueries queries = fixture_queries(stale);
  const auto first =
      stale.classify_witnesses(std::span{queries.canonical});
  check(
      first.audit.buffer_epoch == 1U,
      "the stale-epoch fixture first establishes a healthy epoch");
  configure_fake_gpu_pair_support_phi(
      FakePairSupportPhiCorruption::stale_epoch_without_advance);
  check_throws<std::runtime_error>(
      [&] {
        static_cast<void>(stale.classify_witnesses(
            std::span{queries.canonical}));
      },
      "a repeated device epoch is rejected");
  configure_fake_gpu_pair_support_phi(
      FakePairSupportPhiCorruption::none);
  check_throws<std::runtime_error>(
      [&] {
        static_cast<void>(stale.classify_witnesses(
            std::span{queries.canonical}));
      },
      "a repeated epoch poisons the resident context");
  check(
      fake_gpu_pair_support_phi_launch_count() == 2U,
      "the poisoned stale-epoch context cannot launch a third batch");
}

void test_rank_prune_exact_receipts_and_traffic() {
  reset_fake_gpu_pair_support_phi();
  Fixture fixture;
  constexpr PairSupportRankPruneCapacity capacity{2U, 8U, 4U};
  PairSupportPhiContext context{
      fixture.index, fixture.cloud, 2U, capacity};
  const FixtureQueries queries = fixture_queries(context);
  const std::array products{
      product_from_query(context, queries.strict)};
  const PairSupportRankPruneBudget budget{8U};

  const PairSupportRankPruneBatchResult first =
      context.propose_rank_prunes(products, 1U, budget);
  const PairSupportRankPruneBatchResult second =
      context.propose_rank_prunes(products, 1U, budget);
  check(
      first.proposals.size() == 1U &&
          first.keep_certificates.empty() &&
          first.proposals[0U].product_index == 0U &&
          first.proposals[0U].product == products[0U] &&
          first.proposals[0U].strict_interior_point_count == 1U &&
          first.proposals[0U].strict_witness_receipts.size() == 1U &&
          first.proposals[0U].strict_witness_receipts[0U].node_index ==
              queries.strict.witness_node_index,
      "P2 returns one stable exact receipt for the strict middle witness");
  check(
      first.proposals == second.proposals &&
          first.keep_certificates == second.keep_certificates &&
          first.audit.terminal_digest_fnv1a ==
              second.audit.terminal_digest_fnv1a,
      "two P4 calls preserve proposal and stable terminal digest");

  const auto& audit = first.audit;
  check(
      audit.capacity == capacity && audit.budget == budget &&
          audit.traversal_backend ==
              PairSupportRankTraversalBackend::
                  stackless_product_batch &&
          !audit.stackless_single_product_traversal_used &&
          audit.stackless_product_batch_traversal_used &&
          audit.input_product_count == 1U &&
          audit.required_strict_interior_point_count == 1U &&
          audit.gpu_traversal_epoch_count == 1U &&
          audit.gpu_count_kernel_launch_count == 0U &&
          audit.gpu_exclusive_scan_count == 0U &&
          audit.gpu_emit_kernel_launch_count == 1U &&
          audit.gpu_stackless_kernel_launch_count == 1U &&
          audit.gpu_host_synchronization_count == 2U &&
          audit.gpu_visit_budget_count == 5U &&
          audit.gpu_output_terminal_count == 1U &&
          audit.cpu_exact_terminal_recertification_count == 1U &&
          audit.strict_interior_terminal_count == 1U &&
          audit.anchor_noninterior_terminal_count == 0U &&
          audit.unresolved_external_leaf_terminal_count == 0U &&
          audit.prune_product_count == 1U &&
          audit.keep_certificate_product_count == 0U &&
          audit.gpu_output_receipt_count == 1U &&
          audit.cpu_exact_phi_recertification_count == 1U &&
          audit.proposed_product_count == 1U &&
          audit.fallback_product_count == 0U &&
          audit.buffer_epoch == 1U,
      "P5d closes one stackless batch launch and exact terminal counters");
  check(
      audit.snapshot_h2d_byte_count == 5U * 80U &&
          audit.escape_snapshot_h2d_byte_count == 5U * 4U &&
          audit.active_product_h2d_byte_count == 32U &&
          audit.initial_frontier_h2d_byte_count == 0U &&
          audit.traversal_metadata_d2h_byte_count == 5U * 8U &&
          audit.physical_terminal_d2h_byte_count == 16U &&
          audit.active_terminal_d2h_byte_count == 16U &&
          audit.device_terminal_byte_capacity == 4U * 16U &&
          audit.physical_receipt_d2h_byte_count == 16U &&
          audit.active_receipt_d2h_byte_count == 16U &&
          audit.device_frontier_double_buffer_byte_capacity == 0U &&
          audit.device_escape_snapshot_byte_capacity == 5U * 4U &&
          audit.device_receipt_byte_capacity == 4U * 16U &&
          audit.device_scan_workspace_byte_capacity == 0U &&
          audit.device_fixed_workspace_byte_capacity ==
              32U * 2U + 16U * 4U + 5U * 8U * 2U &&
          second.audit.snapshot_h2d_byte_count == 0U &&
          second.audit.escape_snapshot_h2d_byte_count == 0U &&
          second.audit.buffer_epoch == 2U,
      "P5d audits segmented 16-byte terminals and exact fixed workspace");
  check(
      audit.device_frontier_exhausted &&
          !audit.visit_budget_exhausted &&
          !audit.work_item_capacity_exhausted &&
          !audit.receipt_capacity_exhausted &&
          !audit.epoch_budget_exhausted &&
          audit.immutable_lbvh_snapshot_validated &&
          audit.product_records_validated &&
          audit.stable_terminal_transcript_validated &&
          audit.disjoint_terminal_antichains_validated &&
          audit.keep_coverage_recertification_complete &&
          audit.stable_receipt_transcript_validated &&
          audit.disjoint_receipt_antichains_validated &&
          audit.cpu_exact_recertification_complete &&
          audit.anchor_ball_culling_enabled &&
          !audit.global_support_product_prune_published &&
          !audit.public_status_published,
      "P4 separates recertified certificates from scientific publication");
  check(
      fake_gpu_pair_support_rank_launch_count() == 2U &&
          fake_gpu_pair_support_rank_last_product_count() == 1U &&
          fake_gpu_pair_support_rank_last_work_item_capacity() == 8U &&
          fake_gpu_pair_support_rank_last_receipt_capacity() == 4U,
      "the fake P2 launcher sees only active products and fixed capacities");
}

void test_stackless_two_product_batch_and_segment_fallback() {
  reset_fake_gpu_pair_support_phi();
  Fixture fixture;
  PairSupportPhiContext context{
      fixture.index,
      fixture.cloud,
      2U,
      PairSupportRankPruneCapacity{2U, 8U, 4U}};
  const FixtureQueries queries = fixture_queries(context);
  std::array products{
      product_from_query(context, queries.strict),
      product_from_query(context, queries.descend)};
  std::sort(
      products.begin(),
      products.end(),
      [](const auto& left, const auto& right) {
        return product_key(left) < product_key(right);
      });

  const auto complete = context.propose_rank_prunes(
      products, 1U, PairSupportRankPruneBudget{8U});
  check(
      complete.proposals.size() == 1U &&
          complete.keep_certificates.size() == 1U &&
          complete.audit.prune_product_count == 1U &&
          complete.audit.keep_certificate_product_count == 1U &&
          complete.audit.fallback_product_count == 0U &&
          complete.audit.traversal_backend ==
              PairSupportRankTraversalBackend::
                  stackless_product_batch &&
          complete.audit.stackless_product_batch_traversal_used &&
          !complete.audit.stackless_single_product_traversal_used &&
          complete.audit.input_product_count == 2U &&
          complete.audit.gpu_traversal_epoch_count == 1U &&
          complete.audit.gpu_count_kernel_launch_count == 0U &&
          complete.audit.gpu_exclusive_scan_count == 0U &&
          complete.audit.gpu_emit_kernel_launch_count == 1U &&
          complete.audit.gpu_stackless_kernel_launch_count == 1U &&
          complete.audit.gpu_host_synchronization_count == 2U &&
          complete.audit.device_frontier_exhausted &&
          !complete.audit.visit_budget_exhausted &&
          !complete.audit.receipt_capacity_exhausted,
      "P5d concludes two products in one kernel and two host "
      "synchronizations");

  reset_fake_gpu_pair_support_phi();
  PairSupportPhiContext segmented{
      fixture.index,
      fixture.cloud,
      2U,
      PairSupportRankPruneCapacity{2U, 8U, 1U}};
  const FixtureQueries segmented_queries = fixture_queries(segmented);
  std::array segmented_products{
      product_from_query(segmented, segmented_queries.strict),
      product_from_query(segmented, segmented_queries.descend)};
  std::sort(
      segmented_products.begin(),
      segmented_products.end(),
      [](const auto& left, const auto& right) {
        return product_key(left) < product_key(right);
      });
  const auto partial = segmented.propose_rank_prunes(
      segmented_products, 1U, PairSupportRankPruneBudget{8U});
  check(
      partial.proposals.empty() &&
          partial.keep_certificates.size() == 1U &&
          partial.audit.prune_product_count == 0U &&
          partial.audit.keep_certificate_product_count == 1U &&
          partial.audit.fallback_product_count == 1U &&
          partial.audit.gpu_output_terminal_count == 1U &&
          partial.audit.cpu_exact_terminal_recertification_count == 1U &&
          partial.audit.receipt_capacity_exhausted &&
          !partial.audit.device_frontier_exhausted &&
          partial.audit.stackless_product_batch_traversal_used,
      "a full terminal segment falls back independently while a concluded "
      "neighbor remains exactly consumable");
}

void test_rank_prune_anchor_culls_remote_subtree() {
  reset_fake_gpu_pair_support_phi();
  CanonicalPointCloud cloud = anchor_culling_line_cloud();
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  PairSupportPhiContext context{
      index,
      cloud,
      2U,
      PairSupportRankPruneCapacity{1U, 32U, 4U}};
  const PairSupportPhiWitnessQuery query =
      context.make_leaf_witness_query(0U, 1U, 2U);
  const std::array product{product_from_query(context, query)};

  const auto result = context.propose_rank_prunes(
      product, cloud.size(), PairSupportRankPruneBudget{16U});
  check(
      result.proposals.empty() &&
          result.keep_certificates.size() == 1U &&
          result.keep_certificates[0U].product_index == 0U &&
          result.keep_certificates[0U].product == product[0U] &&
          result.keep_certificates[0U].strict_interior_point_count == 0U &&
          result.keep_certificates[0U].canonical_terminals.size() == 1U &&
          result.keep_certificates[0U].canonical_terminals[0U].kind ==
              PairSupportRankTerminalKind::anchor_noninterior &&
          result.keep_certificates[0U]
                  .canonical_terminals[0U]
                  .terminal.leaf_begin == 2U &&
          result.keep_certificates[0U]
                  .canonical_terminals[0U]
                  .terminal.leaf_end == cloud.size(),
      "P4 publishes one exact covered keep certificate for the remote subtree");
  check(
      result.audit.anchor_ball_culling_enabled &&
          result.audit.gpu_output_terminal_count == 1U &&
          result.audit.cpu_exact_terminal_recertification_count == 1U &&
          result.audit.strict_interior_terminal_count == 0U &&
          result.audit.anchor_noninterior_terminal_count == 1U &&
          result.audit.unresolved_external_leaf_terminal_count == 0U &&
          result.audit.prune_product_count == 0U &&
          result.audit.keep_certificate_product_count == 1U &&
          result.audit.fallback_product_count == 0U &&
          result.audit.keep_coverage_recertification_complete &&
          result.audit.device_frontier_exhausted &&
          !result.audit.work_item_capacity_exhausted &&
          !result.audit.receipt_capacity_exhausted &&
          !result.audit.visit_budget_exhausted &&
          !result.audit.epoch_budget_exhausted &&
          context.node_count() == 19U &&
          result.audit.gpu_visited_work_item_count == 5U &&
          result.audit.gpu_visit_budget_count == 19U &&
          result.audit.gpu_traversal_epoch_count == 1U &&
          result.audit.gpu_count_kernel_launch_count == 0U &&
          result.audit.gpu_exclusive_scan_count == 0U &&
          result.audit.gpu_emit_kernel_launch_count == 1U &&
          result.audit.gpu_stackless_kernel_launch_count == 1U &&
          result.audit.gpu_host_synchronization_count == 2U &&
          result.audit.traversal_backend ==
              PairSupportRankTraversalBackend::
                  stackless_single_product &&
          result.audit.stackless_single_product_traversal_used &&
          result.audit.snapshot_h2d_byte_count == 19U * 80U &&
          result.audit.escape_snapshot_h2d_byte_count == 19U * 4U &&
          result.audit.initial_frontier_h2d_byte_count == 0U &&
          result.audit.device_frontier_double_buffer_byte_capacity == 0U &&
          result.audit.device_scan_workspace_byte_capacity == 0U &&
          result.audit.device_escape_snapshot_byte_capacity == 19U * 4U &&
          result.audit.traversal_metadata_d2h_byte_count == 5U * 8U &&
          result.audit.device_fixed_workspace_byte_capacity ==
              32U + 4U * 16U + 5U * 8U,
      "P5a closes remote coverage in one stackless launch and five visits "
      "without a frontier or scan arena");
}

void test_rank_keep_accepts_exact_anchor_shell_equality() {
  reset_fake_gpu_pair_support_phi();
  CanonicalPointCloud cloud = anchor_shell_cloud();
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  PairSupportPhiContext context{
      index,
      cloud,
      2U,
      PairSupportRankPruneCapacity{1U, 16U, 8U}};
  const PairSupportPhiWitnessQuery query =
      context.make_leaf_witness_query(0U, 3U, 1U);
  const std::array product{product_from_query(context, query)};

  const auto result = context.propose_rank_prunes(
      product, 1U, PairSupportRankPruneBudget{16U});
  check(
      result.proposals.empty() &&
          result.keep_certificates.size() == 1U &&
          result.keep_certificates[0U].strict_interior_point_count == 0U &&
          result.keep_certificates[0U].canonical_terminals.size() == 2U &&
          std::all_of(
              result.keep_certificates[0U].canonical_terminals.begin(),
              result.keep_certificates[0U].canonical_terminals.end(),
              [](const auto& terminal) {
                return terminal.kind ==
                           PairSupportRankTerminalKind::
                               anchor_noninterior &&
                       terminal.terminal.leaf_end -
                               terminal.terminal.leaf_begin ==
                           1U;
              }) &&
          result.audit.anchor_noninterior_terminal_count == 2U &&
          result.audit.strict_interior_terminal_count == 0U &&
          result.audit.keep_certificate_product_count == 1U,
      "P4 recertifies exact anchor-shell equality as noninterior coverage");
}

void test_rank_prune_preflight_and_fallbacks() {
  reset_fake_gpu_pair_support_phi();
  Fixture fixture;
  PairSupportPhiContext context{
      fixture.index,
      fixture.cloud,
      2U,
      PairSupportRankPruneCapacity{3U, 8U, 4U}};
  const FixtureQueries queries = fixture_queries(context);
  std::array products{
      product_from_query(context, queries.strict),
      product_from_query(context, queries.descend)};
  std::sort(
      products.begin(),
      products.end(),
      [](const auto& left, const auto& right) {
        return product_key(left) < product_key(right);
      });
  const std::array<ExactPairSupportFrontierEntry, 0U> empty{};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(context.propose_rank_prunes(
            empty, 1U, PairSupportRankPruneBudget{8U}));
      },
      "P2 rejects an empty product batch before launch");
  const std::array duplicate{products[0U], products[0U]};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(context.propose_rank_prunes(
            duplicate, 1U, PairSupportRankPruneBudget{8U}));
      },
      "P2 rejects duplicate products before launch");
  std::reverse(products.begin(), products.end());
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(context.propose_rank_prunes(
            products, 1U, PairSupportRankPruneBudget{8U}));
      },
      "P2 rejects a noncanonical product order before launch");
  std::reverse(products.begin(), products.end());
  auto changed_range = products[0U];
  ++changed_range.first_leaf_begin;
  const std::array changed{changed_range};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(context.propose_rank_prunes(
            changed, 1U, PairSupportRankPruneBudget{8U}));
      },
      "P4 rejects a product range start that would substitute its "
      "authenticated Morton anchor");
  const std::array one_product{products[0U]};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(context.propose_rank_prunes(
            one_product, 0U, PairSupportRankPruneBudget{8U}));
      },
      "P2 rejects a zero rank threshold before launch");
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(context.propose_rank_prunes(
            one_product, 1U, PairSupportRankPruneBudget{0U}));
      },
      "P2 rejects a zero epoch budget before launch");
  check(
      fake_gpu_pair_support_rank_launch_count() == 0U,
      "all P2 preflight failures avoid a fake GPU launch");

  PairSupportPhiContext p1_only{fixture.index, fixture.cloud, 2U};
  const FixtureQueries p1_queries = fixture_queries(p1_only);
  const std::array disabled_product{
      product_from_query(p1_only, p1_queries.strict)};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(p1_only.propose_rank_prunes(
            disabled_product, 1U, PairSupportRankPruneBudget{8U}));
      },
      "the legacy P1 constructor leaves P2 explicitly disabled");

  reset_fake_gpu_pair_support_phi();
  PairSupportPhiContext epoch_limited{
      fixture.index,
      fixture.cloud,
      2U,
      PairSupportRankPruneCapacity{1U, 1U, 4U}};
  const FixtureQueries epoch_queries = fixture_queries(epoch_limited);
  const std::array epoch_product{
      product_from_query(epoch_limited, epoch_queries.strict)};
  const auto incomplete = epoch_limited.propose_rank_prunes(
      epoch_product, 1U, PairSupportRankPruneBudget{1U});
  check(
      incomplete.proposals.empty() &&
          incomplete.keep_certificates.empty() &&
          incomplete.audit.fallback_product_count == 1U &&
          incomplete.audit.visit_budget_exhausted &&
          incomplete.audit.epoch_budget_exhausted &&
          !incomplete.audit.device_frontier_exhausted &&
          incomplete.audit.gpu_visit_budget_count == 1U &&
          incomplete.audit.gpu_visited_work_item_count == 1U &&
          incomplete.audit.gpu_host_synchronization_count == 1U &&
          incomplete.audit.stackless_single_product_traversal_used &&
          !incomplete.audit.global_support_product_prune_published,
      "a stackless visit-budget gap cannot fabricate a P4 certificate");
  const auto recovered = epoch_limited.propose_rank_prunes(
      epoch_product, 1U, PairSupportRankPruneBudget{8U});
  check(
      recovered.proposals.size() == 1U &&
          recovered.audit.buffer_epoch == 2U &&
          recovered.audit.snapshot_h2d_byte_count == 0U &&
          recovered.audit.escape_snapshot_h2d_byte_count == 0U &&
          !recovered.audit.visit_budget_exhausted,
      "ordinary stackless visit exhaustion does not poison the resident "
      "context or repeat either immutable snapshot upload");

  reset_fake_gpu_pair_support_phi();
  PairSupportPhiContext work_limited{
      fixture.index,
      fixture.cloud,
      2U,
      PairSupportRankPruneCapacity{2U, 1U, 4U}};
  const FixtureQueries work_queries = fixture_queries(work_limited);
  const std::array work_product{
      product_from_query(work_limited, work_queries.strict)};
  const auto capacity_fallback = work_limited.propose_rank_prunes(
      work_product, 1U, PairSupportRankPruneBudget{1U});
  check(
      capacity_fallback.proposals.empty() &&
          capacity_fallback.keep_certificates.empty() &&
          capacity_fallback.audit.fallback_product_count == 1U &&
          !capacity_fallback.audit.work_item_capacity_exhausted &&
          !capacity_fallback.audit.stackless_single_product_traversal_used &&
          capacity_fallback.audit
              .stackless_product_batch_traversal_used &&
          capacity_fallback.audit.visit_budget_exhausted &&
          capacity_fallback.audit.epoch_budget_exhausted &&
          capacity_fallback.audit.gpu_count_kernel_launch_count == 0U &&
          capacity_fallback.audit.gpu_emit_kernel_launch_count == 1U &&
          capacity_fallback.audit.gpu_output_terminal_count == 0U &&
          capacity_fallback.audit.physical_terminal_d2h_byte_count == 0U &&
          capacity_fallback.audit.active_terminal_d2h_byte_count == 0U &&
          capacity_fallback.audit.physical_receipt_d2h_byte_count == 0U &&
          capacity_fallback.audit.active_receipt_d2h_byte_count == 0U &&
          capacity_fallback.audit.device_terminal_byte_capacity == 4U * 16U,
      "P5d copies no terminal bytes for a visit-bounded product while "
      "retaining the bounded segmented device capacity");
}

void test_stackless_terminal_capacity_fallback_and_replay() {
  reset_fake_gpu_pair_support_phi();
  CanonicalPointCloud cloud = anchor_shell_cloud();
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  PairSupportPhiContext context{
      index,
      cloud,
      2U,
      PairSupportRankPruneCapacity{1U, 16U, 1U}};
  const PairSupportPhiWitnessQuery query =
      context.make_leaf_witness_query(0U, 3U, 1U);
  const std::array product{product_from_query(context, query)};

  const auto first = context.propose_rank_prunes(
      product, cloud.size(), PairSupportRankPruneBudget{16U});
  const auto replay = context.propose_rank_prunes(
      product, cloud.size(), PairSupportRankPruneBudget{16U});
  check(
      first.proposals.empty() &&
          first.keep_certificates.empty() &&
          first.audit.fallback_product_count == 1U &&
          first.audit.receipt_capacity_exhausted &&
          !first.audit.work_item_capacity_exhausted &&
          !first.audit.visit_budget_exhausted &&
          !first.audit.device_frontier_exhausted &&
          first.audit.gpu_output_terminal_count == 1U &&
          first.audit.cpu_exact_terminal_recertification_count == 1U &&
          first.audit.gpu_host_synchronization_count == 2U &&
          first.audit.gpu_visit_budget_count == context.node_count() &&
          first.audit.device_terminal_byte_capacity == 16U &&
          first.audit.physical_terminal_d2h_byte_count == 16U &&
          first.audit.active_terminal_d2h_byte_count == 16U &&
          first.audit.device_fixed_workspace_byte_capacity ==
              32U + 16U + 5U * 8U,
      "P5a authenticates but never publishes a C-truncated stackless "
      "terminal prefix");
  check(
      replay.proposals == first.proposals &&
          replay.keep_certificates == first.keep_certificates &&
          replay.audit.terminal_digest_fnv1a ==
              first.audit.terminal_digest_fnv1a &&
          replay.audit.snapshot_h2d_byte_count == 0U &&
          replay.audit.escape_snapshot_h2d_byte_count == 0U &&
          replay.audit.buffer_epoch == 2U,
      "a resident C-truncated stackless replay is stable and repeats no "
      "immutable snapshot upload");
}

void test_rank_prune_canonical_minimal_prefix() {
  reset_fake_gpu_pair_support_phi();
  CanonicalPointCloud cloud = five_point_line_cloud();
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  PairSupportPhiContext context{
      index,
      cloud,
      2U,
      PairSupportRankPruneCapacity{1U, 16U, 8U}};
  const PairSupportPhiWitnessQuery query =
      context.make_leaf_witness_query(0U, 4U, 2U);
  const std::array product{product_from_query(context, query)};
  const auto result = context.propose_rank_prunes(
      product, 2U, PairSupportRankPruneBudget{16U});
  check(
      result.proposals.size() == 1U &&
          !result.proposals[0U].strict_witness_receipts.empty() &&
          result.proposals[0U].strict_witness_receipts.size() <= 2U &&
          result.proposals[0U].strict_interior_point_count >= 2U &&
          result.audit.gpu_output_receipt_count >=
              result.proposals[0U].strict_witness_receipts.size(),
      "P2 projects the full GPU receipt transcript to a threshold-minimal prefix");
  if (!result.proposals.empty()) {
    const auto& receipts =
        result.proposals[0U].strict_witness_receipts;
    std::size_t prefix_count = 0U;
    bool canonical = true;
    for (std::size_t index_position = 0U;
         index_position < receipts.size();
         ++index_position) {
      canonical =
          canonical && prefix_count < 2U &&
          (index_position == 0U ||
           std::tuple{
               receipts[index_position - 1U].leaf_begin,
               receipts[index_position - 1U].leaf_end,
               receipts[index_position - 1U].node_index} <
               std::tuple{
                   receipts[index_position].leaf_begin,
                   receipts[index_position].leaf_end,
                   receipts[index_position].node_index});
      prefix_count += static_cast<std::size_t>(
          receipts[index_position].leaf_end -
          receipts[index_position].leaf_begin);
    }
    check(
        canonical && prefix_count ==
            result.proposals[0U].strict_interior_point_count,
        "P2 receipts are strictly Morton-canonical and minimal before CPU adaptation");
  }
}

void test_rank_prune_uses_cpu_range_first_product_order() {
  reset_fake_gpu_pair_support_phi();
  CanonicalPointCloud cloud = five_point_line_cloud();
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  PairSupportPhiContext context{
      index,
      cloud,
      2U,
      PairSupportRankPruneCapacity{2U, 16U, 8U}};
  std::vector<ExactPairSupportFrontierEntry> candidates;
  for (std::size_t first_index = 0U;
       first_index < context.node_count();
       ++first_index) {
    const auto first = context.node_descriptor(first_index);
    for (std::size_t second_index = 0U;
         second_index < context.node_count();
         ++second_index) {
      const auto second = context.node_descriptor(second_index);
      if (first.leaf_end <= second.leaf_begin) {
        candidates.push_back(ExactPairSupportFrontierEntry{
            first.node_index,
            second.node_index,
            first.leaf_begin,
            first.leaf_end,
            second.leaf_begin,
            second.leaf_end,
            0U});
      }
    }
  }
  std::sort(
      candidates.begin(),
      candidates.end(),
      [](const auto& left, const auto& right) {
        return product_key(left) < product_key(right);
      });
  std::optional<std::array<ExactPairSupportFrontierEntry, 2U>>
      divergent;
  const auto node_first_key = [](const auto& product) {
    return std::tuple{
        product.first_node_index,
        product.second_node_index,
        product.first_leaf_begin,
        product.first_leaf_end,
        product.second_leaf_begin,
        product.second_leaf_end,
        product.self_product};
  };
  for (std::size_t left = 0U;
       left < candidates.size() && !divergent.has_value();
       ++left) {
    for (std::size_t right = left + 1U;
         right < candidates.size();
         ++right) {
      if (node_first_key(candidates[left]) >
          node_first_key(candidates[right])) {
        divergent = std::array{
            candidates[left], candidates[right]};
        break;
      }
    }
  }
  check(
      divergent.has_value(),
      "the ordering fixture distinguishes CPU range-first order from node-first order");
  if (divergent.has_value()) {
    const auto result = context.propose_rank_prunes(
        *divergent, 1U, PairSupportRankPruneBudget{1U});
    check(
        result.audit.product_records_validated &&
            result.audit.input_product_count == 2U &&
            fake_gpu_pair_support_rank_launch_count() == 1U,
        "P2 accepts the exact range-first canonical order emitted by the CPU stream");
  }
}

void check_rank_corruption_and_poisoning(
    FakePairSupportPhiCorruption corruption,
    const std::string& label) {
  reset_fake_gpu_pair_support_phi();
  Fixture fixture;
  PairSupportPhiContext context{
      fixture.index,
      fixture.cloud,
      2U,
      PairSupportRankPruneCapacity{1U, 8U, 4U}};
  const FixtureQueries queries = fixture_queries(context);
  const std::array product{
      product_from_query(context, queries.strict)};
  configure_fake_gpu_pair_support_phi(corruption);
  check_throws<std::runtime_error>(
      [&] {
        static_cast<void>(context.propose_rank_prunes(
            product, 1U, PairSupportRankPruneBudget{8U}));
      },
      label + " is rejected fail-closed");
  configure_fake_gpu_pair_support_phi(
      FakePairSupportPhiCorruption::none);
  check_throws<std::runtime_error>(
      [&] {
        static_cast<void>(context.propose_rank_prunes(
            product, 1U, PairSupportRankPruneBudget{8U}));
      },
      label + " poisons only its resident context");
  check(
      fake_gpu_pair_support_rank_launch_count() == 1U,
      label + " prevents a second P2 launch");
}

void test_rank_prune_corruption_matrix() {
  const std::array cases{
      std::pair{FakePairSupportPhiCorruption::rank_capacity_metadata,
                "corrupted P2 capacity metadata"},
      std::pair{FakePairSupportPhiCorruption::rank_zero_epoch,
                "an unauthenticated P2 zero epoch"},
      std::pair{FakePairSupportPhiCorruption::rank_changed_receipt,
                "a falsified internal P4 terminal"},
      std::pair{FakePairSupportPhiCorruption::rank_false_strict_receipt,
                "a duplicated P4 terminal"},
      std::pair{FakePairSupportPhiCorruption::rank_stale_tail,
                "a surplus terminal beyond the P5 active prefix"},
      std::pair{FakePairSupportPhiCorruption::rank_epoch_count_over_budget,
                "a P2 traversal beyond its epoch budget"},
      std::pair{
          FakePairSupportPhiCorruption::rank_simulated_async_failure,
          "an asynchronous P2 device failure"}};
  for (const auto& [corruption, label] : cases) {
    check_rank_corruption_and_poisoning(corruption, label);
  }

  reset_fake_gpu_pair_support_phi();
  Fixture fixture;
  PairSupportPhiContext stale{
      fixture.index,
      fixture.cloud,
      2U,
      PairSupportRankPruneCapacity{1U, 8U, 4U}};
  const FixtureQueries queries = fixture_queries(stale);
  const std::array product{
      product_from_query(stale, queries.strict)};
  const auto first = stale.propose_rank_prunes(
      product, 1U, PairSupportRankPruneBudget{8U});
  check(
      first.audit.buffer_epoch == 1U,
      "the P2 stale-epoch fixture first establishes a healthy epoch");
  configure_fake_gpu_pair_support_phi(
      FakePairSupportPhiCorruption::rank_stale_epoch_without_advance);
  check_throws<std::runtime_error>(
      [&] {
        static_cast<void>(stale.propose_rank_prunes(
            product, 1U, PairSupportRankPruneBudget{8U}));
      },
      "a repeated P2 buffer epoch is rejected");
  configure_fake_gpu_pair_support_phi(
      FakePairSupportPhiCorruption::none);
  check_throws<std::runtime_error>(
      [&] {
        static_cast<void>(stale.propose_rank_prunes(
            product, 1U, PairSupportRankPruneBudget{8U}));
      },
      "a repeated P2 epoch poisons the resident context");
  check(
      fake_gpu_pair_support_rank_launch_count() == 2U,
      "the poisoned stale-P2 context cannot launch a third batch");
}

void test_move_only_context() {
  reset_fake_gpu_pair_support_phi();
  Fixture fixture;
  PairSupportPhiContext source{fixture.index, fixture.cloud, 2U};
  const FixtureQueries queries = fixture_queries(source);
  PairSupportPhiContext moved{std::move(source)};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(source.classify_witnesses(
            std::span{queries.canonical}));
      },
      "a moved-from pair-support phi context is not queryable");
  const auto result =
      moved.classify_witnesses(std::span{queries.canonical});
  check_healthy_result(result, queries, 1U, 2U, "the moved-to context");
}

}  // namespace

int main() {
  test_exact_fixture_and_two_epochs();
  test_preflight_validation_and_capacity();
  test_corruption_matrix_and_poisoning();
  test_rank_prune_exact_receipts_and_traffic();
  test_stackless_two_product_batch_and_segment_fallback();
  test_rank_prune_anchor_culls_remote_subtree();
  test_rank_keep_accepts_exact_anchor_shell_equality();
  test_rank_prune_preflight_and_fallbacks();
  test_stackless_terminal_capacity_fallback_and_replay();
  test_rank_prune_canonical_minimal_prefix();
  test_rank_prune_uses_cpu_range_first_product_order();
  test_rank_prune_corruption_matrix();
  test_move_only_context();

  if (failures != 0) {
    std::cerr << failures
              << " GPU pair-support phi context test(s) failed\n";
    return 1;
  }
  std::cout << "GPU pair-support phi context tests passed\n";
  return 0;
}
