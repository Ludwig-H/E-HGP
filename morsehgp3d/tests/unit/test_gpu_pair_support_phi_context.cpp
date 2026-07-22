#include "fake_gpu_pair_support_phi_launchers.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/gpu/pair_support_phi.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

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
  test_move_only_context();

  if (failures != 0) {
    std::cerr << failures
              << " GPU pair-support phi context test(s) failed\n";
    return 1;
  }
  std::cout << "GPU pair-support phi context tests passed\n";
  return 0;
}
