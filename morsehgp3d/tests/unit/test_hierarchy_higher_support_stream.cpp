#include "morsehgp3d/exact/support.hpp"
#include "morsehgp3d/hierarchy/higher_support_stream.hpp"
#include "morsehgp3d/spatial/brute_force.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::hierarchy::ExactHigherSupportPruneReason;
using morsehgp3d::hierarchy::ExactHigherSupportAnchoredSession;
using morsehgp3d::hierarchy::ExactHigherSupportAuthorityContext;
using morsehgp3d::hierarchy::ExactHigherSupportCheckpoint;
using morsehgp3d::hierarchy::ExactHigherSupportPendingStage;
using morsehgp3d::hierarchy::ExactHigherSupportStreamChunk;
using morsehgp3d::hierarchy::ExactHigherSupportStopReason;
using morsehgp3d::hierarchy::ExactHigherSupportStreamBudget;
using morsehgp3d::hierarchy::ExactHigherSupportStreamStatus;
using morsehgp3d::hierarchy::build_exact_higher_support_stream;
using morsehgp3d::hierarchy::compute_exact_higher_support_checkpoint_digest;
using morsehgp3d::hierarchy::exact_higher_support_candidate_universe_size;
using morsehgp3d::hierarchy::make_initial_exact_higher_support_checkpoint;
using morsehgp3d::hierarchy::verify_exact_higher_support_checkpoint;
using morsehgp3d::hierarchy::verify_exact_higher_support_stream;
using morsehgp3d::hierarchy::verify_exact_higher_support_stream_run;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

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

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] CanonicalPointCloud cloud_from(
    std::initializer_list<CertifiedPoint3> points) {
  const std::vector<CertifiedPoint3> storage{points};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{storage});
}

[[nodiscard]] CanonicalPointCloud cloud_from(
    const std::vector<CertifiedPoint3>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactHigherSupportStreamBudget unlimited_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return ExactHigherSupportStreamBudget{
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

struct SupportKey {
  std::uint8_t support_size{};
  std::array<PointId, 4> support_ids{};

  friend bool operator==(const SupportKey&, const SupportKey&) = default;
  friend bool operator<(const SupportKey& left, const SupportKey& right) {
    if (left.support_size != right.support_size) {
      return left.support_size < right.support_size;
    }
    return left.support_ids < right.support_ids;
  }
};

struct ExhaustiveHigherDecision {
  std::vector<SupportKey> events;
  std::vector<SupportKey> diagnostics;
};

template <std::size_t SupportSize>
void classify_exhaustive_support(
    const CanonicalPointCloud& cloud,
    const std::array<PointId, SupportSize>& support_ids,
    std::size_t maximum_rank,
    ExhaustiveHigherDecision& decision) {
  if (SupportSize > maximum_rank) {
    return;
  }
  std::array<morsehgp3d::exact::ExactRational3, SupportSize> points{};
  for (std::size_t index = 0U; index < SupportSize; ++index) {
    points[index] = cloud.point(support_ids[index]).exact();
  }
  const auto analysis =
      morsehgp3d::exact::analyze_circumcenter_support(points);
  if (analysis.status() !=
      morsehgp3d::exact::CircumcenterSupportStatus::minimal) {
    return;
  }
  const auto& sphere = analysis.circumcenter_result();
  if (!sphere.center().has_value() ||
      !sphere.squared_level().has_value()) {
    throw std::logic_error(
        "an exhaustive minimal support omitted its exact sphere");
  }
  std::size_t interior_count = 0U;
  std::size_t shell_count = 0U;
  for (PointId point_id = 0U; point_id < cloud.size(); ++point_id) {
    const auto classification = morsehgp3d::exact::classify_sphere_point(
        *sphere.center(), *sphere.squared_level(), cloud.point(point_id));
    if (classification.location() ==
        morsehgp3d::exact::SpherePointLocation::strictly_inside) {
      ++interior_count;
    } else if (classification.location() ==
               morsehgp3d::exact::SpherePointLocation::boundary) {
      ++shell_count;
    }
  }
  if (interior_count > maximum_rank - SupportSize) {
    return;
  }
  SupportKey key;
  key.support_size = static_cast<std::uint8_t>(SupportSize);
  std::copy(support_ids.begin(), support_ids.end(), key.support_ids.begin());
  if (shell_count == SupportSize) {
    decision.events.push_back(key);
  } else {
    decision.diagnostics.push_back(key);
  }
}

[[nodiscard]] ExhaustiveHigherDecision exhaustive_higher_decision(
    const CanonicalPointCloud& cloud,
    std::size_t maximum_rank) {
  ExhaustiveHigherDecision decision;
  for (PointId first = 0U; first < cloud.size(); ++first) {
    for (PointId second = first + 1U; second < cloud.size(); ++second) {
      for (PointId third = second + 1U; third < cloud.size(); ++third) {
        classify_exhaustive_support<3U>(
            cloud, {first, second, third}, maximum_rank, decision);
        for (PointId fourth = third + 1U; fourth < cloud.size(); ++fourth) {
          classify_exhaustive_support<4U>(
              cloud,
              {first, second, third, fourth},
              maximum_rank,
              decision);
        }
      }
    }
  }
  std::sort(decision.events.begin(), decision.events.end());
  std::sort(decision.diagnostics.begin(), decision.diagnostics.end());
  return decision;
}

void test_bigint_universe() {
  check(
      exact_higher_support_candidate_universe_size(0U) == 0 &&
          exact_higher_support_candidate_universe_size(3U) == 1 &&
          exact_higher_support_candidate_universe_size(4U) == 5,
      "small higher-support universes equal C(n,3)+C(n,4)");
  const BigInt ten_million_expected{"416666583333329166667500000"};
  check(
      exact_higher_support_candidate_universe_size(10'000'000U) ==
              ten_million_expected &&
          ten_million_expected >
              BigInt{std::numeric_limits<std::uint64_t>::max()},
      "the 10M support universe is exact beyond 64 bits");
}

void test_regular_tetrahedron_complete_and_fresh_replay() {
  CanonicalPointCloud cloud = cloud_from({
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0)});
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportStreamBudget budget = unlimited_budget();
  const auto result =
      build_exact_higher_support_stream(index, cloud, 10U, budget);
  const std::size_t triangle_count = static_cast<std::size_t>(
      std::count_if(
          result.events.begin(),
          result.events.end(),
          [](const auto& event) { return event.support_size == 3U; }));
  const std::size_t tetrahedron_count = static_cast<std::size_t>(
      std::count_if(
          result.events.begin(),
          result.events.end(),
          [](const auto& event) { return event.support_size == 4U; }));
  check(
      result.stream_complete() &&
          result.absence_of_additional_higher_supports_certified() &&
          result.audit.total_support_count == 5 &&
          result.audit.leaf_classified_support_count == 5 &&
          result.audit.resolved_support_count == 5 &&
          result.prune_certificates.empty() &&
          result.relevant_extra_shell_diagnostics.empty() &&
          triangle_count == 4U && tetrahedron_count == 1U,
      "the regular tetrahedron closes four triangles and one tetrahedron without a cell atlas");
  const auto verification = verify_exact_higher_support_stream(
      index, cloud, 10U, budget, result);
  check(
      verification.result_certified &&
          verification.prune_certificates_replayed &&
          verification.grouped_frontier_replayed &&
          verification.fresh_replay_certified,
      "a fresh authority replay certifies the complete regular-tetrahedron stream");

  auto mutated = result;
  mutated.audit.total_support_count += 1;
  check(
      !verify_exact_higher_support_stream(
           index, cloud, 10U, budget, mutated)
           .result_certified,
      "a mutated BigInt universe fails fresh verification");
}

void test_intrinsically_above_rank_and_budgeted_frontier() {
  CanonicalPointCloud cloud = cloud_from({
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0)});
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportStreamBudget budget = unlimited_budget();
  const auto rank_two =
      build_exact_higher_support_stream(index, cloud, 1U, budget);
  check(
      rank_two.stream_complete() && rank_two.events.empty() &&
          rank_two.audit.total_support_count == 5 &&
          rank_two.audit.rank_pruned_support_count == 5 &&
          std::all_of(
              rank_two.prune_certificates.begin(),
              rank_two.prune_certificates.end(),
              [](const auto& certificate) {
                return certificate.reason ==
                           ExactHigherSupportPruneReason::
                               strict_interior_rank_bound &&
                       certificate.required_strict_interior_point_count ==
                           0U;
              }),
      "supports larger than s_max are exactly resolved without leaf geometry");

  ExactHigherSupportStreamBudget stopped_budget = unlimited_budget();
  stopped_budget.maximum_work_unit_count = 0U;
  const auto stopped = build_exact_higher_support_stream(
      index, cloud, 10U, stopped_budget);
  check(
      stopped.status ==
              ExactHigherSupportStreamStatus::budget_exhausted &&
          stopped.stop_reason ==
              ExactHigherSupportStopReason::work_unit_limit &&
          !stopped.stream_complete() &&
          !stopped.absence_of_additional_higher_supports_certified() &&
          stopped.remaining_frontier.size() == 2U &&
          stopped.audit.remaining_frontier_support_count == 5 &&
          stopped.audit.resolved_support_count == 0,
      "a zero-work run retains the exact triangle and tetrahedron frontier");
  check(
      verify_exact_higher_support_stream(
          index, cloud, 10U, stopped_budget, stopped)
          .result_certified,
      "the budgeted residual frontier is freshly replayable");
}

void test_sparse_extra_shell_diagnostic() {
  CanonicalPointCloud cloud = cloud_from({
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0),
      point(-1.0, -1.0, -1.0)});
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportStreamBudget budget = unlimited_budget();
  const auto result =
      build_exact_higher_support_stream(index, cloud, 10U, budget);
  const ExhaustiveHigherDecision exhaustive =
      exhaustive_higher_decision(
          cloud, result.requirements.maximum_relevant_closed_rank);
  std::vector<SupportKey> streamed_events;
  std::vector<SupportKey> streamed_diagnostics;
  for (const auto& event : result.events) {
    streamed_events.push_back(
        SupportKey{event.support_size, event.support_ids});
  }
  for (const auto& diagnostic :
       result.relevant_extra_shell_diagnostics) {
    streamed_diagnostics.push_back(
        SupportKey{diagnostic.support_size, diagnostic.support_ids});
  }
  std::sort(streamed_events.begin(), streamed_events.end());
  std::sort(streamed_diagnostics.begin(), streamed_diagnostics.end());
  const bool observed_five_point_shell = std::any_of(
      result.relevant_extra_shell_diagnostics.begin(),
      result.relevant_extra_shell_diagnostics.end(),
      [](const auto& diagnostic) {
        return diagnostic.support_size == 4U &&
               diagnostic.interior_ids.empty() &&
               diagnostic.shell_count == 5U &&
               diagnostic.minimum_possible_closed_rank == 4U &&
               diagnostic.observed_closed_rank == 5U &&
               diagnostic.exterior_count == 0U;
      });
  check(
      result.stream_complete() &&
          result.audit.total_support_count == 15 &&
          observed_five_point_shell &&
          streamed_events == exhaustive.events &&
          streamed_diagnostics == exhaustive.diagnostics,
      "the sparse five-site output agrees bidirectionally with exhaustive support enumeration");
  check(
      verify_exact_higher_support_stream(
          index, cloud, 10U, budget, result)
          .result_certified,
      "the sparse extra-shell decision survives fresh replay");
}

void test_nonzero_universal_rank_receipts() {
  const std::vector<CertifiedPoint3> points{
      point(10.0, 10.0, 10.0),
      point(10.125, 10.0, 10.0),
      point(10.0, -10.0, -10.0),
      point(10.125, -10.0, -10.0),
      point(-10.0, 10.0, -10.0),
      point(-9.875, 10.0, -10.0),
      point(-10.0, -10.0, 10.0),
      point(-9.875, -10.0, 10.0),
      point(0.0, 0.0, 0.0)};
  CanonicalPointCloud cloud = cloud_from(points);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportStreamBudget budget = unlimited_budget();
  const auto result =
      build_exact_higher_support_stream(index, cloud, 3U, budget);
  check(
      result.stream_complete() &&
          result.audit.total_support_count == 210 &&
          result.audit.rank_pruned_support_count == 32 &&
          result.audit.emitted_rank_receipt_count == 9U &&
          result.audit.above_rank_leaf_count == 12U &&
          result.audit.resolved_support_count == 210 &&
          result.audit.remaining_frontier_support_count == 0,
      "nine exact universal receipts prune 32 higher supports at Kmax three");
  check(
      verify_exact_higher_support_stream(
          index, cloud, 3U, budget, result)
          .result_certified,
      "nonzero rank receipts survive independent fresh replay");
}

void test_input_contract() {
  CanonicalPointCloud cloud = cloud_from({point(0.0, 0.0, 0.0)});
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportStreamBudget budget = unlimited_budget();
  check_throws<std::out_of_range>(
      [&]() {
        static_cast<void>(
            build_exact_higher_support_stream(index, cloud, 0U, budget));
      },
      "Kmax zero is outside the higher-support contract");
  check_throws<std::out_of_range>(
      [&]() {
        static_cast<void>(
            build_exact_higher_support_stream(index, cloud, 11U, budget));
      },
      "Kmax above ten is outside the higher-support contract");

  CanonicalPointCloud tetrahedron = cloud_from({
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0)});
  MortonLbvhIndex tetrahedron_index =
      MortonLbvhIndex::build(tetrahedron);
  ExactHigherSupportStreamBudget undersized = unlimited_budget();
  undersized.maximum_frontier_entry_count = 1U;
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(build_exact_higher_support_stream(
            tetrahedron_index, tetrahedron, 10U, undersized));
      },
      "the initial triangle and tetrahedron roots require two frontier slots");
}

void test_reinjectable_chunks_and_hostile_mutations() {
  CanonicalPointCloud cloud = cloud_from({
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0)});
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactHigherSupportAuthorityContext authority{index, cloud, 10U};
  ExactHigherSupportAnchoredSession session{authority};
  check(
      authority.audit().manifest_cached &&
          authority.audit().manifest_build_count == 1U &&
          authority.audit().canonical_cloud_point_hash_count == cloud.size(),
      "the higher-support authority manifest is built and cached once");

  ExactHigherSupportCheckpoint checkpoint = session.trusted_checkpoint();
  check(
      verify_exact_higher_support_checkpoint(authority, checkpoint)
          .local_integrity_verified &&
          checkpoint.next_chunk_sequence == 0U &&
          checkpoint.frontier.size() == 2U &&
          checkpoint.cumulative_audit.remaining_frontier_support_count == 5,
      "the two exact arity roots form an integrity-verified initial checkpoint");

  ExactHigherSupportStreamBudget unit_budget = unlimited_budget();
  unit_budget.maximum_work_unit_count = 1U;
  std::vector<ExactHigherSupportStreamBudget> budgets;
  std::vector<ExactHigherSupportStreamChunk> chunks;
  for (std::size_t step = 0U;
       step < 256U && !checkpoint.locally_complete();
       ++step) {
    ExactHigherSupportStreamChunk chunk =
        session.prepare_next(unit_budget, checkpoint);
    const auto transition =
        session.commit_prepared(unit_budget, checkpoint, chunk);
    check(
        transition.chunk_transition_verified,
        "each unit-work higher-support transition is freshly replayed");
    budgets.push_back(unit_budget);
    chunks.push_back(std::move(chunk));
    checkpoint = session.trusted_checkpoint();
  }
  check(
      checkpoint.locally_complete() && chunks.size() > 1U,
      "unit-work chunks persist and resume every charged product stage");
  const auto resident =
      build_exact_higher_support_stream(index, cloud, 10U, unlimited_budget());
  check(
      checkpoint.cumulative_audit == resident.audit &&
          checkpoint.output_record_count ==
              resident.audit.emitted_record_count,
      "chunked execution reaches the resident exact audit without double charging");
  ExactHigherSupportAnchoredSession resident_session{authority};
  const ExactHigherSupportCheckpoint resident_source =
      resident_session.trusted_checkpoint();
  const ExactHigherSupportStreamChunk resident_chunk =
      resident_session.prepare_next(unlimited_budget(), resident_source);
  const auto resident_transition = resident_session.commit_prepared(
      unlimited_budget(), resident_source, resident_chunk);
  check(
      resident_transition.chunk_transition_verified &&
          resident_session.trusted_checkpoint().locally_complete() &&
          resident_session.trusted_checkpoint().output_chain_digest ==
              checkpoint.output_chain_digest &&
          resident_session.trusted_checkpoint().output_record_count ==
              checkpoint.output_record_count,
      "the three-kind output chain is independent of chunk boundaries");
  const auto run = verify_exact_higher_support_stream_run(
      index, cloud, 10U, budgets, chunks);
  check(
      run.anchored_run_certified &&
          run.verified_chunk_count == chunks.size(),
      "the terminal chunk lineage is anchored at the reconstructed roots");

  ExactHigherSupportCheckpoint invalid =
      make_initial_exact_higher_support_checkpoint(authority);
  invalid.frontier.back().groups[0].leaf_end += 1U;
  invalid.checkpoint_digest =
      compute_exact_higher_support_checkpoint_digest(invalid);
  check(
      !verify_exact_higher_support_checkpoint(authority, invalid)
           .local_integrity_verified,
      "a self-rehashed false Morton range fails closed");

  invalid = make_initial_exact_higher_support_checkpoint(authority);
  invalid.cumulative_audit.remaining_frontier_support_count += 1;
  invalid.checkpoint_digest =
      compute_exact_higher_support_checkpoint_digest(invalid);
  check(
      !verify_exact_higher_support_checkpoint(authority, invalid)
           .local_integrity_verified,
      "a self-rehashed BigInt partition mutation fails closed");

  invalid = make_initial_exact_higher_support_checkpoint(authority);
  invalid.output_chain_digest = invalid.manifest.semantic_digest;
  invalid.checkpoint_digest =
      compute_exact_higher_support_checkpoint_digest(invalid);
  check(
      !verify_exact_higher_support_checkpoint(authority, invalid)
           .local_integrity_verified,
      "a self-rehashed nonempty output chain at record zero fails closed");

  ExactHigherSupportAnchoredSession mutation_session{authority};
  const ExactHigherSupportCheckpoint mutation_source =
      mutation_session.trusted_checkpoint();
  auto mutated_chunk =
      mutation_session.prepare_next(unit_budget, mutation_source);
  mutated_chunk.next_checkpoint.output_record_count += 1U;
  mutated_chunk.next_checkpoint.checkpoint_digest =
      compute_exact_higher_support_checkpoint_digest(
          mutated_chunk.next_checkpoint);
  check(
      !mutation_session
           .commit_prepared(unit_budget, mutation_source, mutated_chunk)
           .chunk_transition_verified,
      "a self-rehashed successor mutation fails fresh transition replay");
  check(
      mutation_session.trusted_checkpoint() == mutation_source,
      "a rejected successor cannot advance the anchored session");
  const auto valid_mutation_chunk =
      mutation_session.prepare_next(unit_budget, mutation_source);
  check(
      mutation_session
          .commit_prepared(
              unit_budget, mutation_source, valid_mutation_chunk)
          .chunk_transition_verified,
      "a freshly replayed successor advances the anchored session");
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(mutation_session.prepare_next(
            unit_budget, mutation_source));
      },
      "a previously valid source token becomes stale after commit");

  ExactHigherSupportCheckpoint locally_coherent_forgery =
      make_initial_exact_higher_support_checkpoint(authority);
  const auto tetrahedron_root = locally_coherent_forgery.frontier.back();
  locally_coherent_forgery.frontier.assign(5U, tetrahedron_root);
  locally_coherent_forgery.cumulative_audit.maximum_frontier_entry_count = 5U;
  locally_coherent_forgery.checkpoint_digest =
      compute_exact_higher_support_checkpoint_digest(
          locally_coherent_forgery);
  check(
      verify_exact_higher_support_checkpoint(
          authority, locally_coherent_forgery)
          .local_integrity_verified,
      "local integrity intentionally does not claim frontier provenance");
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(mutation_session.prepare_next(
            unit_budget, locally_coherent_forgery));
      },
      "an equal-cardinality forged frontier cannot enter the anchored session");

  const std::vector<CertifiedPoint3> receipt_points{
      point(10.0, 10.0, 10.0),
      point(10.125, 10.0, 10.0),
      point(10.0, -10.0, -10.0),
      point(10.125, -10.0, -10.0),
      point(-10.0, 10.0, -10.0),
      point(-9.875, 10.0, -10.0),
      point(-10.0, -10.0, 10.0),
      point(-9.875, -10.0, 10.0),
      point(0.0, 0.0, 0.0)};
  CanonicalPointCloud receipt_cloud = cloud_from(receipt_points);
  MortonLbvhIndex receipt_index = MortonLbvhIndex::build(receipt_cloud);
  const ExactHigherSupportAuthorityContext receipt_authority{
      receipt_index, receipt_cloud, 3U};
  ExactHigherSupportCheckpoint receipt_checkpoint =
      make_initial_exact_higher_support_checkpoint(receipt_authority);
  ExactHigherSupportAnchoredSession receipt_session{receipt_authority};
  receipt_checkpoint = receipt_session.trusted_checkpoint();
  bool observed_rank_cursor = false;
  for (std::size_t step = 0U;
       step < 512U && !receipt_checkpoint.locally_complete() &&
       !observed_rank_cursor;
       ++step) {
    const auto chunk =
        receipt_session.prepare_next(unit_budget, receipt_checkpoint);
    const auto transition = receipt_session.commit_prepared(
        unit_budget, receipt_checkpoint, chunk);
    check(
        transition.chunk_transition_verified,
        "the anchored receipt session accepts its freshly replayed chunk");
    receipt_checkpoint = receipt_session.trusted_checkpoint();
    if (receipt_checkpoint.pending_product.has_value()) {
      const auto& pending = *receipt_checkpoint.pending_product;
      observed_rank_cursor =
          pending.stage == ExactHigherSupportPendingStage::rank_search &&
          pending.rank_search_started &&
          !pending.rank_frontier.empty();
    }
  }
  check(
      observed_rank_cursor &&
          verify_exact_higher_support_checkpoint(
              receipt_authority, receipt_checkpoint)
              .local_integrity_verified,
      "a nonempty exact rank DFS cursor is independently recertified");
  if (observed_rank_cursor) {
    ExactHigherSupportCheckpoint mutated_receipt = receipt_checkpoint;
    mutated_receipt.pending_product->rank_frontier.back().leaf_end += 1U;
    mutated_receipt.checkpoint_digest =
        compute_exact_higher_support_checkpoint_digest(mutated_receipt);
    check(
        !verify_exact_higher_support_checkpoint(
             receipt_authority, mutated_receipt)
             .local_integrity_verified,
        "a self-rehashed active rank receipt with a false range fails closed");

    ExactHigherSupportCheckpoint oversized_receipts = receipt_checkpoint;
    const auto receipt =
        oversized_receipts.pending_product->rank_frontier.back();
    oversized_receipts.pending_product->strict_interior_receipts.assign(
        10U, receipt);
    oversized_receipts.checkpoint_digest =
        compute_exact_higher_support_checkpoint_digest(oversized_receipts);
    check(
        !verify_exact_higher_support_checkpoint(
             receipt_authority, oversized_receipts)
             .local_integrity_verified,
        "more than nine strict receipts fail before exact recomputation");

    ExactHigherSupportCheckpoint oversized_rank_frontier =
        receipt_checkpoint;
    oversized_rank_frontier.pending_product->rank_frontier.assign(
        receipt_index.build_counters().maximum_depth + 2U, receipt);
    oversized_rank_frontier.checkpoint_digest =
        compute_exact_higher_support_checkpoint_digest(
            oversized_rank_frontier);
    check(
        !verify_exact_higher_support_checkpoint(
             receipt_authority, oversized_rank_frontier)
             .local_integrity_verified,
        "a rank DFS stack above LBVH depth plus one fails before replay");
  }

  CanonicalPointCloud singleton = cloud_from({point(0.0, 0.0, 0.0)});
  MortonLbvhIndex singleton_index = MortonLbvhIndex::build(singleton);
  const ExactHigherSupportAuthorityContext singleton_authority{
      singleton_index, singleton, 10U};
  ExactHigherSupportAnchoredSession singleton_session{singleton_authority};
  const auto terminal = make_initial_exact_higher_support_checkpoint(
      singleton_authority);
  check(
      terminal.locally_complete() &&
          verify_exact_higher_support_checkpoint(
              singleton_index, singleton, 10U, terminal)
              .local_integrity_verified,
      "a cloud below arity three starts at a certified terminal checkpoint");
  check_throws<std::logic_error>(
      [&]() {
        static_cast<void>(singleton_session.prepare_next(
            unlimited_budget(), singleton_session.trusted_checkpoint()));
      },
      "an anchored terminal checkpoint has no no-op successor");
  const std::vector<ExactHigherSupportStreamBudget> redundant_budgets{
      unlimited_budget()};
  const std::vector<ExactHigherSupportStreamChunk> redundant_chunks(1U);
  check(
      !verify_exact_higher_support_stream_run(
           singleton_index,
           singleton,
           10U,
           redundant_budgets,
           redundant_chunks)
           .anchored_run_certified,
      "an anchored run rejects every chunk after a terminal root state");

  ExactHigherSupportAnchoredSession temporary_authority_session{
      ExactHigherSupportAuthorityContext{index, cloud, 10U}};
  check(
      temporary_authority_session.trusted_checkpoint() ==
          make_initial_exact_higher_support_checkpoint(authority),
      "the session owns its immutable authority cache rather than its wrapper lifetime");
}

}  // namespace

int main() {
  test_bigint_universe();
  test_regular_tetrahedron_complete_and_fresh_replay();
  test_intrinsically_above_rank_and_budgeted_frontier();
  test_sparse_extra_shell_diagnostic();
  test_nonzero_universal_rank_receipts();
  test_input_contract();
  test_reinjectable_chunks_and_hostile_mutations();
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "higher-support stream tests passed\n";
  return 0;
}
