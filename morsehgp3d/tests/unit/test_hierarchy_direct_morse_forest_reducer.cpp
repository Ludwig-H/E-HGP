#include "fake_gpu_phase14_facet_top_k_proposal_launchers.hpp"
#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/direct_sparse_facet_top_k_integrated_adapter.hpp"
#include "morsehgp3d/hierarchy/direct_morse_forest_durable_run_archive.hpp"
#include "morsehgp3d/hierarchy/direct_morse_forest_reducer.hpp"
#include "morsehgp3d/hierarchy/direct_morse_forest_source_wire.hpp"

#include <cstdio>

#include <unistd.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactLbvhTopKBudget;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::gpu::DirectSparseFacetTopKIntegratedAdapter;
using morsehgp3d::gpu::DirectSparseFacetTopKProposalContext;
using morsehgp3d::gpu::DirectSparseFacetTopKProposalPolicy;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_phase14_facet_top_k_proposal;
using morsehgp3d::gpu::test_support::
    reset_fake_gpu_phase14_morton_lbvh_build;

constexpr std::uint64_t authority_id = UINT64_C(0x15C);
int failures = 0;

[[nodiscard]] morsehgp3d::contract::CanonicalId digest_text(
    std::string_view text) {
  morsehgp3d::contract::CanonicalSha256Builder builder;
  builder.update(text);
  return builder.finalize();
}

template <typename Source>
concept ForestReducerProjectable = requires(Source&& source) {
  project_exact_direct_morse_forest_reducer_batch(
      std::forward<Source>(source));
};

static_assert(ForestReducerProjectable<
              const ExactDirectSparseFacetDescentBatchExecutionResult&>);
static_assert(
    !ForestReducerProjectable<
        ExactDirectSparseFacetDescentBatchExecutionResult>);

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] ExactPairSupportStreamBudget pair_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
  };
}

[[nodiscard]] ExactHigherSupportStreamBudget higher_budget() {
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

[[nodiscard]] ExactDirectSaddleArmSeedBudget seed_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectSparseFacetDescentStepBudget step_budget() {
  return {
      ExactDirectSparsePositiveFacetProbeBudget{513U, 256U},
      ExactLbvhTopKBudget{
          4096U, 4096U, 4096U, 4096U, 256U, 16U, 16U},
      ExactDirectSparsePositiveFacetProbeBudget{513U, 256U},
  };
}

[[nodiscard]] ExactDirectMorseForestBudget forest_budget() {
  constexpr std::size_t capacity = 4096U;
  ExactDirectMorseForestBudget budget;
  budget.maximum_source_role_scan_count = capacity;
  budget.maximum_source_batch_scan_count = capacity;
  budget.maximum_source_family_scan_count = capacity;
  budget.maximum_source_arm_seed_scan_count = capacity;
  budget.maximum_birth_record_count = capacity;
  budget.maximum_arm_root_binding_count = capacity;
  budget.maximum_saddle_record_count = capacity;
  budget.maximum_atomic_group_count = capacity;
  budget.maximum_child_reference_count = capacity;
  budget.maximum_batch_record_count = capacity;
  budget.maximum_node_count = capacity;
  budget.maximum_final_root_count = capacity;
  budget.maximum_batch_distinct_arm_count = 256U;
  budget.maximum_logical_output_entry_count = capacity;
  budget.maximum_aggregate_closure_node_count = capacity;
  budget.maximum_aggregate_closure_step_call_count = capacity;
  budget.locator_budget = {
      256U,
      256U,
      2560U,
      256U,
      256U,
      256U,
      256U,
      256U,
      2560U,
      513U,
      513U,
  };
  budget.closure_budget = {
      256U,
      256U,
      256U,
      513U,
      step_budget(),
  };
  budget.quotient_budget = {256U, 256U, 256U, 256U, 1024U};
  return budget;
}

[[nodiscard]] ExactDirectMorseForestConfig forest_config() {
  ExactDirectMorseForestConfig config;
  config.locator_config.external_authority_id = authority_id;
  return config;
}

[[nodiscard]] ExactDirectMorseIndustrialPlanConfig plan_config(
    std::uint64_t maximum_batch_count) {
  ExactDirectMorseIndustrialPlanConfig config;
  config.policy =
      ExactDirectMorseIndustrialPolicy::massive_external_streaming;
  config.memory_model = {
      64U,
      16U,
      16U,
      8U,
      16U,
      16U,
      16U,
      4U,
      16U,
      8U,
      16U,
      2U,
  };
  config.chunk_budget = {
      1'000'000U,
      maximum_batch_count,
      4096U,
      4096U,
      4096U,
      4096U,
  };
  return config;
}

[[nodiscard]] ExactDirectSparseFacetDescentBatchPlanBudget plan_budget() {
  return {
      16U,
      256U,
      256U,
      1024U,
      256U,
      1024U,
      1'000'000U,
      64U,
  };
}

[[nodiscard]] ExactDirectSparseFacetDescentBatchExecutionBudget
execution_budget() {
  return {3U, 256U, 256U, 2560U, 256U};
}

[[nodiscard]] ExactDirectSparseFacetTopKProposalTranscriptBudget
proposal_budget() {
  constexpr std::size_t capacity = 4096U;
  return {capacity, capacity, capacity, capacity * capacity, capacity};
}

[[nodiscard]] ExactDirectSparseFacetTopKProposalTranscriptResult
empty_proposal_transcript(
    std::size_t source_batch_index,
    const morsehgp3d::exact::ExactLevel& closed_batch_squared_level,
    const ExactDirectSparsePositiveFacetLocator& locator) {
  const std::array<ExactDirectSparseFacetTopKProposalRecord, 0U> records{};
  return build_exact_direct_sparse_facet_top_k_proposal_transcript(
      {source_batch_index,
       closed_batch_squared_level,
       locator.snapshot_stamp()},
      records,
      {0U, 0U, 0U, 0U, 0U});
}

void check_initial_singleton_bulk_audit(
    const ExactDirectMorseForestReducerFoldResult& folded,
    std::size_t point_count,
    const std::string& path) {
  check(
      folded.canonical_singleton_bulk_count == point_count &&
          folded.staged_birth_record_count == 0U &&
          folded.staged_birth_node_count == 0U &&
          folded.staged_locator_binding_count == 0U &&
          folded.implicit_singleton_carrier_count == point_count &&
          folded.total_carrier_handle_count >= point_count &&
          folded.materialized_direct_carrier_state_count ==
              folded.total_carrier_handle_count - point_count &&
          folded.root_override_slot_capacity ==
              2U * folded.maximum_atomic_group_count + 1U &&
          folded.locator_parent_authority_reused_by_carrier_state &&
          folded.no_dense_singleton_carrier_state_materialized,
      "the " + path +
          " first fold keeps singleton output and carrier state implicit with zero per-birth staging");
  const std::size_t trusted_total_carrier_handle_count =
      folded.total_carrier_handle_count;
  const std::size_t trusted_maximum_atomic_group_count =
      folded.maximum_atomic_group_count;
  check(
      verify_exact_direct_morse_forest_reducer_fold_layout(
          folded,
          trusted_total_carrier_handle_count,
          point_count,
          trusted_maximum_atomic_group_count),
      "the " + path +
          " fold layout is freshly bound to trusted source and budget counts");
  auto mutated = folded;
  ++mutated.materialized_direct_carrier_state_count;
  check(
      !mutated.certified_committed_batch(),
      "the " + path +
          " fold certificate rejects a forged direct carrier-state count");
  mutated = folded;
  ++mutated.total_carrier_handle_count;
  check(
      !mutated.certified_committed_batch(),
      "the " + path +
          " fold certificate rejects a forged total carrier count");
  mutated = folded;
  ++mutated.root_override_slot_capacity;
  check(
      !mutated.certified_committed_batch(),
      "the " + path +
          " fold certificate rejects a non-2G+1 override capacity");
  mutated = folded;
  ++mutated.total_carrier_handle_count;
  ++mutated.materialized_direct_carrier_state_count;
  check(
      mutated.certified_committed_batch() &&
          !verify_exact_direct_morse_forest_reducer_fold_layout(
              mutated,
              trusted_total_carrier_handle_count,
              point_count,
              trusted_maximum_atomic_group_count),
      "the " + path +
          " fresh verifier rejects coordinated forged carrier counts");
  mutated = folded;
  ++mutated.maximum_atomic_group_count;
  mutated.root_override_slot_capacity += 2U;
  check(
      mutated.certified_committed_batch() &&
          !verify_exact_direct_morse_forest_reducer_fold_layout(
              mutated,
              trusted_total_carrier_handle_count,
              point_count,
              trusted_maximum_atomic_group_count),
      "the " + path +
          " fresh verifier rejects a coordinated forged override budget");
}

struct Scenario {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult event_journal;
  ExactDirectSaddleArmSeedJournalResult seed_journal;
};

struct SegmentedRun {
  std::vector<ExactDirectMorseForestBatchSegment> segments;
  ExactDirectMorseForestFinalSeal seal;
};

[[nodiscard]] ExactDirectMorseForestSegmentLimits segment_limits() {
  constexpr std::size_t capacity = 4096U;
  return {
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
      capacity,
  };
}

bool collect_segment(
    void* state,
    const ExactDirectMorseForestBatchSegment& segment) noexcept {
  try {
    static_cast<std::vector<ExactDirectMorseForestBatchSegment>*>(state)
        ->push_back(segment);
    return true;
  } catch (...) {
    return false;
  }
}

bool reject_segment(
    void*,
    const ExactDirectMorseForestBatchSegment&) noexcept {
  return false;
}

bool discard_segment(
    void*,
    const ExactDirectMorseForestBatchSegment&) noexcept {
  return true;
}

struct InstrumentedSourceProvider {
  const ExactDirectMorseForestResidentSourceAdapter* adapter{};
  std::size_t live_window_count{};
  std::size_t maximum_live_window_count{};
  std::size_t acquire_count{};
  std::size_t release_count{};
  std::size_t overlap_rejection_count{};
  std::size_t mutation_batch_index{
      std::numeric_limits<std::size_t>::max()};
  std::size_t mutation_delivery_count{};

  [[nodiscard]] ExactDirectMorseForestSourceBatchVisitDecision operator()(
      std::size_t batch_index,
      ExactDirectMorseForestSourceBatchConsumerView consumer) {
    if (adapter == nullptr || live_window_count != 0U) {
      ++overlap_rejection_count;
      return ExactDirectMorseForestSourceBatchVisitDecision::
          no_window_inconsistent;
    }
    ++live_window_count;
    ++acquire_count;
    maximum_live_window_count = std::max(
        maximum_live_window_count, live_window_count);
    struct Release {
      InstrumentedSourceProvider* provider{};
      ~Release() {
        --provider->live_window_count;
        ++provider->release_count;
      }
    } release{this};

    auto relay = [&](const ExactDirectMorseForestSourceBatchWindow& window) {
      if (batch_index == mutation_batch_index &&
          mutation_delivery_count == 0U) {
        auto mutated = window;
        auto digest_bytes = mutated.batch_identity_digest.bytes();
        digest_bytes[0U] ^= std::uint8_t{1U};
        mutated.batch_identity_digest =
            morsehgp3d::contract::CanonicalId{digest_bytes};
        ++mutation_delivery_count;
        return consumer(mutated);
      }
      return consumer(window);
    };
    return adapter->visit_batch(
        batch_index,
        ExactDirectMorseForestSourceBatchConsumerView{relay});
  }
};

struct InMemorySourceWireProvider {
  const std::vector<std::vector<std::uint8_t>>* wires{};
  std::size_t live_wire_count{};
  std::size_t maximum_live_wire_count{};
  std::size_t acquire_count{};
  std::size_t release_count{};
  std::size_t overlap_rejection_count{};
  std::size_t mutation_batch_index{
      std::numeric_limits<std::size_t>::max()};
  std::size_t mutation_delivery_count{};

  [[nodiscard]] ExactDirectMorseForestSourceBatchWireVisitDecision
  operator()(
      std::size_t batch_index,
      ExactDirectMorseForestSourceBatchWireConsumerView consumer) {
    if (wires == nullptr || batch_index >= wires->size()) {
      return ExactDirectMorseForestSourceBatchWireVisitDecision::
          no_batch_out_of_range;
    }
    if (live_wire_count != 0U) {
      ++overlap_rejection_count;
      return ExactDirectMorseForestSourceBatchWireVisitDecision::
          no_wire_unavailable;
    }
    ++live_wire_count;
    ++acquire_count;
    maximum_live_wire_count =
        std::max(maximum_live_wire_count, live_wire_count);
    struct Release {
      InMemorySourceWireProvider* provider{};
      ~Release() {
        --provider->live_wire_count;
        ++provider->release_count;
      }
    } release{this};

    bool accepted = false;
    if (batch_index == mutation_batch_index &&
        mutation_delivery_count == 0U) {
      auto mutated = (*wires)[batch_index];
      if (!mutated.empty()) {
        mutated.back() ^= std::uint8_t{1U};
      }
      ++mutation_delivery_count;
      accepted = consumer(mutated);
    } else {
      accepted = consumer((*wires)[batch_index]);
    }
    return accepted
               ? ExactDirectMorseForestSourceBatchWireVisitDecision::
                     complete_synchronous_visit
               : ExactDirectMorseForestSourceBatchWireVisitDecision::
                     no_consumer_rejected;
  }
};

struct InstrumentedCanonicalWireSourceProvider {
  ExactDirectMorseForestCanonicalWireSourceProvider* provider{};
  const InMemorySourceWireProvider* wire_provider{};
  std::size_t live_window_count{};
  std::size_t maximum_live_window_count{};
  std::size_t acquire_count{};
  std::size_t release_count{};
  std::size_t overlap_rejection_count{};
  std::size_t mutation_batch_index{
      std::numeric_limits<std::size_t>::max()};
  std::size_t mutation_delivery_count{};
  std::optional<ExactDirectMorseForestSourceWireDecision>
      first_mutation_wire_decision;

  [[nodiscard]] ExactDirectMorseForestSourceBatchVisitDecision operator()(
      std::size_t batch_index,
      ExactDirectMorseForestSourceBatchConsumerView consumer) {
    if (provider == nullptr || wire_provider == nullptr ||
        live_window_count != 0U) {
      ++overlap_rejection_count;
      return ExactDirectMorseForestSourceBatchVisitDecision::
          no_window_inconsistent;
    }
    ++live_window_count;
    ++acquire_count;
    maximum_live_window_count =
        std::max(maximum_live_window_count, live_window_count);
    const std::size_t mutation_count_before =
        wire_provider->mutation_delivery_count;
    const auto decision = provider->visit_batch(batch_index, consumer);
    if (mutation_count_before == 0U &&
        wire_provider->mutation_delivery_count == 1U) {
      first_mutation_wire_decision = provider->last_wire_decision();
    }
    --live_window_count;
    ++release_count;
    mutation_delivery_count = wire_provider->mutation_delivery_count;
    return decision;
  }
};

struct PaddingMutatedSourceLookups {
  std::vector<ExactDirectMorseEventProjection> projections;
  std::vector<ExactDirectSupportEvent> events;
};

const ExactDirectMorseEventProjection* find_padding_mutated_projection(
    const void* state, std::size_t logical_index) noexcept {
  const auto& lookups =
      *static_cast<const PaddingMutatedSourceLookups*>(state);
  const auto found = std::find_if(
      lookups.projections.begin(),
      lookups.projections.end(),
      [logical_index](const auto& projection) {
        return projection.event_projection_index == logical_index;
      });
  return found == lookups.projections.end() ? nullptr : &*found;
}

const ExactDirectSupportEvent* find_padding_mutated_event(
    const void* state, std::size_t source_index) noexcept {
  const auto& lookups =
      *static_cast<const PaddingMutatedSourceLookups*>(state);
  const auto found = std::find_if(
      lookups.events.begin(),
      lookups.events.end(),
      [source_index](const auto& event) {
        return event.event_index == source_index;
      });
  return found == lookups.events.end() ? nullptr : &*found;
}

[[nodiscard]] Scenario tetrahedron() {
  const std::array<CertifiedPoint3, 4U> points{
      point(1.0, 1.0, 1.0),
      point(-1.0, -1.0, 1.0),
      point(-1.0, 1.0, -1.0),
      point(1.0, -1.0, -1.0),
  };
  CanonicalPointCloud cloud = CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSupportTerminalBudget terminal_budget{
      pair_budget(), higher_budget()};
  const auto pair = build_exact_pair_support_stream(
      index, cloud, 1U, terminal_budget.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, 1U, terminal_budget.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index, cloud, 1U, terminal_budget, pair, higher);
  auto event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  auto seed_journal = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, event_journal, seed_budget());
  return {
      std::move(cloud),
      std::move(index),
      std::move(facade),
      std::move(event_journal),
      std::move(seed_journal),
  };
}

[[nodiscard]] Scenario gabriel_ac_to_de() {
  // Input labels A, B, C, D, E canonicalize to D=0, A=1, B=2, C=3, E=4.
  const std::array<CertifiedPoint3, 5U> points{
      point(0.0, 0.0, 7.0),
      point(0.0, 9.0, 6.0),
      point(1.0, 4.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(4.0, 1.0, 2.0),
  };
  CanonicalPointCloud cloud = CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSupportTerminalBudget terminal_budget{
      pair_budget(), higher_budget()};
  const auto pair = build_exact_pair_support_stream(
      index, cloud, 2U, terminal_budget.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, 2U, terminal_budget.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index, cloud, 2U, terminal_budget, pair, higher);
  auto event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  auto seed_journal = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, event_journal, seed_budget());
  return {
      std::move(cloud),
      std::move(index),
      std::move(facade),
      std::move(event_journal),
      std::move(seed_journal),
  };
}

[[nodiscard]] ExactDirectMorseForestJournalResult run_stream(
    const Scenario& scenario,
    std::uint64_t maximum_batch_count,
    bool exercise_atomic_rejection,
    bool use_live_transaction) {
  const auto industrial_config = plan_config(maximum_batch_count);
  const auto observed_plan =
      build_exact_direct_sparse_facet_descent_batch_plan(
          scenario.cloud,
          scenario.facade,
          scenario.event_journal,
          seed_budget(),
          scenario.seed_journal,
          industrial_config,
          plan_budget());
  check(
      observed_plan.complete_architecture_plan(),
      "the reducer fixture has one complete 14C plan");

  ExactDirectMorseForestReducer reducer(
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal,
      forest_budget(),
      forest_config());
  ExactDirectSparseFacetDescentAnchoredBatchExecutor executor(
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal,
      industrial_config,
      plan_budget(),
      observed_plan,
      reducer.strict_locator());
  bool terminal_substitution_exercised = false;
  bool stale_sibling_exercised = false;
  bool live_fold_rejection_exercised = false;

  while (!executor.complete()) {
    const std::size_t batch_index = executor.next_source_batch_index();
    const ExactDirectSparseFacetWitness witness{
        authority_id,
        (static_cast<std::uint64_t>(batch_index) + 1U) * 3U};
    if (use_live_transaction) {
      const auto transcript = empty_proposal_transcript(
          batch_index,
          scenario.event_journal.batches[batch_index].squared_level,
          reducer.strict_locator());
      if (!live_fold_rejection_exercised) {
        auto mismatched_closure_budget =
            forest_budget().closure_budget;
        --mismatched_closure_budget.maximum_node_count;
        auto mismatched_ticket =
            executor.prepare_next_sealed_with_top_k_proposal_transcript(
                witness,
                execution_budget(),
                mismatched_closure_budget,
                transcript);
        check(
            mismatched_ticket.prepared(),
            "14H prepares a valid ticket whose closure budget differs from the reducer contract");
        const auto stamp_before =
            reducer.strict_locator().snapshot_stamp();
        const auto rejected =
            reducer.fold_prepared_ticket(
                executor, std::move(mismatched_ticket));
        check(
            rejected.certified_atomic_rejection() &&
                rejected.reducer_fold_attempted &&
                rejected.decision ==
                    ExactDirectMorseForestLiveCommitDecision::
                        no_live_commit_reducer_fold_rejected &&
                executor.next_source_batch_index() == batch_index &&
                reducer.next_source_batch_index() == batch_index &&
                reducer.strict_locator().snapshot_stamp() == stamp_before,
            "a valid shared ticket rejected by the reducer leaves locator and both cursors unchanged");
        live_fold_rejection_exercised = true;
      }
      auto ticket =
          executor.prepare_next_sealed_with_top_k_proposal_transcript(
              witness,
              execution_budget(),
              forest_budget().closure_budget,
              transcript);
      check(
          ticket.prepared(),
          "14H mints one sealed live reducer ticket");
      auto sibling =
          executor.prepare_next_sealed_with_top_k_proposal_transcript(
              witness,
              execution_budget(),
              forest_budget().closure_budget,
              transcript);
      const auto committed =
          reducer.fold_prepared_ticket(executor, std::move(ticket));
      check(
          committed.certified_live_commit() &&
              committed.scientific_delta.has_value() &&
              committed.reducer_fold.certified_committed_batch() &&
              executor.next_source_batch_index() == batch_index + 1U &&
              reducer.next_source_batch_index() == batch_index + 1U,
          "the live transaction folds the reducer before one allocation-free 14H cursor advance");
      if (batch_index == 0U) {
        check_initial_singleton_bulk_audit(
            committed.reducer_fold, scenario.cloud.size(), "live");
      }
      if (!stale_sibling_exercised) {
        const auto stamp_before =
            reducer.strict_locator().snapshot_stamp();
        const auto stale =
            reducer.fold_prepared_ticket(
                executor, std::move(sibling));
        check(
            stale.certified_atomic_rejection() &&
                stale.decision ==
                    ExactDirectMorseForestLiveCommitDecision::
                        no_live_commit_stale_epoch_or_cursor &&
                executor.next_source_batch_index() == batch_index + 1U &&
                reducer.next_source_batch_index() == batch_index + 1U &&
                reducer.strict_locator().snapshot_stamp() == stamp_before,
            "a stale sibling ticket is consumed without a second cursor or reducer mutation");
        stale_sibling_exercised = true;
      }
      continue;
    }

    const auto delta = executor.prepare_next(
        witness, execution_budget(), forest_budget().closure_budget);
    check(
        delta.complete_architecture_execution(),
        "14D produces one complete compact reducer delta");
    const auto replay = executor.commit_prepared(
        witness,
        execution_budget(),
        forest_budget().closure_budget,
        delta);
    check(
        replay.result_certified && replay.session_advanced,
        "the anchored 14D cursor advances before the reducer mutates its locator");

    auto projected =
        project_exact_direct_morse_forest_reducer_batch(delta);
    if (exercise_atomic_rejection && batch_index == 0U) {
      auto reordered = projected;
      ++reordered.source_batch_index;
      const auto stamp_before = reducer.strict_locator().snapshot_stamp();
      const auto rejected = reducer.fold(reordered);
      check(
          rejected.certified_atomic_rejection() &&
              reducer.next_source_batch_index() == 0U &&
              reducer.strict_locator().snapshot_stamp() == stamp_before,
          "a reordered batch is rejected without locator or reducer advance");
    }
    if (exercise_atomic_rejection &&
        !terminal_substitution_exercised) {
      std::vector<ExactDirectSparseFacetDescentBatchResolvedKey>
          substituted_keys(
              projected.resolved_keys.begin(),
              projected.resolved_keys.end());
      for (std::size_t left = 0U;
           left < substituted_keys.size() &&
           !terminal_substitution_exercised;
           ++left) {
        for (std::size_t right = left + 1U;
             right < substituted_keys.size();
             ++right) {
          if (substituted_keys[left].resolved_component_handle ==
              substituted_keys[right].resolved_component_handle) {
            continue;
          }
          substituted_keys[left].resolved_component_handle =
              substituted_keys[right].resolved_component_handle;
          substituted_keys[left].resolved_binding_witness =
              substituted_keys[right].resolved_binding_witness;
          auto substituted = projected;
          substituted.resolved_keys = substituted_keys;
          const auto stamp_before =
              reducer.strict_locator().snapshot_stamp();
          const auto rejected = reducer.fold(substituted);
          check(
              rejected.certified_atomic_rejection() &&
                  reducer.next_source_batch_index() == batch_index &&
                  reducer.strict_locator().snapshot_stamp() ==
                      stamp_before,
              "a substituted terminal carrier is rejected atomically");
          terminal_substitution_exercised = true;
          break;
        }
      }
    }
    const auto committed = reducer.fold(projected);
    check(
        committed.certified_committed_batch() &&
            reducer.next_source_batch_index() == batch_index + 1U,
        "one projected batch commits locator then scientific state");
    if (batch_index == 0U) {
      check_initial_singleton_bulk_audit(
          committed, scenario.cloud.size(), "projected");
    }
  }
  check(
      reducer.complete(),
      "the reducer consumes every exact source batch once");
  if (exercise_atomic_rejection) {
    check(
        terminal_substitution_exercised,
        "the fixture exercised a distinct terminal-carrier substitution");
  }
  if (use_live_transaction) {
    check(
        stale_sibling_exercised && live_fold_rejection_exercised,
        "the live fixture exercised stale-capability and reducer-fold rejection paths");
  }
  return reducer.finish();
}

template <class Provider>
[[nodiscard]] ExactDirectMorseForestJournalResult
run_instrumented_source_provider_stream(
    const Scenario& scenario,
    const ExactDirectMorseForestResidentSourceAdapter& adapter,
    Provider& provider) {
  const auto industrial_config = plan_config(2U);
  const auto observed_plan =
      build_exact_direct_sparse_facet_descent_batch_plan(
          scenario.cloud,
          scenario.facade,
          scenario.event_journal,
          seed_budget(),
          scenario.seed_journal,
          industrial_config,
          plan_budget());
  check(
      observed_plan.complete_architecture_plan(),
      "the bounded-source reducer fixture has one complete 14C plan");

  ExactDirectMorseForestReducer reducer(
      adapter.manifest(),
      ExactDirectMorseForestSourceBatchProviderView{provider},
      forest_budget(),
      forest_config());
  ExactDirectSparseFacetDescentAnchoredBatchExecutor executor(
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal,
      industrial_config,
      plan_budget(),
      observed_plan,
      reducer.strict_locator());
  bool retry_exercised = false;

  while (!executor.complete()) {
    const std::size_t batch_index = executor.next_source_batch_index();
    const ExactDirectSparseFacetWitness witness{
        authority_id,
        (static_cast<std::uint64_t>(batch_index) + 1U) * 3U};
    const auto delta = executor.prepare_next(
        witness,
        execution_budget(),
        forest_budget().closure_budget);
    const auto replay = executor.commit_prepared(
        witness,
        execution_budget(),
        forest_budget().closure_budget,
        delta);
    check(
        delta.complete_architecture_execution() &&
            replay.result_certified && replay.session_advanced,
        "14D produces one certified delta for the bounded-source reducer");
    const auto projected =
        project_exact_direct_morse_forest_reducer_batch(delta);
    const auto stamp_before = reducer.strict_locator().snapshot_stamp();
    const std::size_t reducer_batch_before =
        reducer.next_source_batch_index();
    auto folded = reducer.fold(projected);

    if (batch_index == provider.mutation_batch_index &&
        !retry_exercised) {
      bool expected_wire_failure = true;
      if constexpr (requires {
                      provider.first_mutation_wire_decision;
                    }) {
        expected_wire_failure =
            provider.first_mutation_wire_decision ==
            std::optional<ExactDirectMorseForestSourceWireDecision>{
                ExactDirectMorseForestSourceWireDecision::
                    wire_content_mismatch};
      }
      check(
          provider.mutation_delivery_count == 1U &&
              expected_wire_failure &&
              folded.certified_atomic_rejection() &&
              folded.decision ==
                  ExactDirectMorseForestReducerFoldDecision::
                      no_reducer_batch_inconsistent &&
              reducer.next_source_batch_index() == reducer_batch_before &&
              reducer.strict_locator().snapshot_stamp() == stamp_before &&
              provider.live_window_count == 0U &&
              provider.acquire_count == provider.release_count,
          "an incoherent source-window digest is rejected before locator or reducer cursor mutation");
      folded = reducer.fold(projected);
      retry_exercised = true;
    }

    if (!folded.certified_committed_batch()) {
      std::cerr << "bounded source batch=" << batch_index
                << ", fold decision="
                << static_cast<unsigned>(folded.decision)
                << ", acquired=" << provider.acquire_count
                << ", released=" << provider.release_count << '\n';
      break;
    }

    check(
        folded.certified_committed_batch() &&
            reducer.next_source_batch_index() == batch_index + 1U &&
            provider.live_window_count == 0U &&
            provider.acquire_count == provider.release_count,
        "one synchronous source window is released after its reducer fold");
  }

  check(
      retry_exercised && reducer.complete(),
      "the bounded-source stream retries one mutated window and consumes every batch");
  return reducer.finish();
}

[[nodiscard]] SegmentedRun run_segmented_stream(
    const Scenario& scenario) {
  const auto industrial_config = plan_config(2U);
  const auto observed_plan =
      build_exact_direct_sparse_facet_descent_batch_plan(
          scenario.cloud,
          scenario.facade,
          scenario.event_journal,
          seed_budget(),
          scenario.seed_journal,
          industrial_config,
          plan_budget());
  check(
      observed_plan.complete_architecture_plan(),
      "the segmented reducer fixture has one complete 14C plan");

  ExactDirectMorseForestReducer reducer(
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal,
      forest_budget(),
      forest_config(),
      segment_limits(),
      morsehgp3d::contract::CanonicalId{});
  ExactDirectSparseFacetDescentAnchoredBatchExecutor executor(
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal,
      industrial_config,
      plan_budget(),
      observed_plan,
      reducer.strict_locator());
  SegmentedRun run;
  bool retry_path_exercised = false;

  check(
      reducer.segmented_output_enabled() &&
          !reducer.has_pending_output_segment() &&
          reducer.pending_output_segment() == nullptr,
      "the segmented mode is fixed before the first fold and starts without output history");

  while (!executor.complete()) {
    const std::size_t batch_index = executor.next_source_batch_index();
    const ExactDirectSparseFacetWitness witness{
        authority_id,
        (static_cast<std::uint64_t>(batch_index) + 1U) * 3U};
    const auto delta = executor.prepare_next(
        witness,
        execution_budget(),
        forest_budget().closure_budget);
    check(
        delta.complete_architecture_execution(),
        "14D produces one complete delta for the segmented reducer");
    const auto replay = executor.commit_prepared(
        witness,
        execution_budget(),
        forest_budget().closure_budget,
        delta);
    check(
        replay.result_certified && replay.session_advanced,
        "the anchored cursor advances before one segmented reducer fold");

    const auto projected =
        project_exact_direct_morse_forest_reducer_batch(delta);
    const auto cursor_before = reducer.output_cursor();
    const auto folded = reducer.fold(projected);
    check(
        folded.certified_committed_batch() &&
            reducer.next_source_batch_index() == batch_index + 1U &&
            reducer.has_pending_output_segment(),
        "one committed fold exposes exactly one pending output segment");
    const auto* pending = reducer.pending_output_segment();
    check(
        pending != nullptr && pending->certified_structure() &&
            pending->begin_cursor == cursor_before &&
            pending->end_cursor.segment_count ==
                cursor_before.segment_count + 1U &&
            pending->batch.batch_index == batch_index &&
            pending->batch.source_journal_batch_index == batch_index,
        "the pending segment preserves dense source order and global cursors");
    if (pending == nullptr) {
      break;
    }
    const auto pending_before_drain = *pending;

    if (!retry_path_exercised) {
      check(
          batch_index == 0U &&
              pending->canonical_singleton_prefix_implicit &&
              pending->birth_records.empty() &&
              pending->nodes.empty() &&
              pending->batch.birth_record_count == scenario.cloud.size(),
          "the singleton batch is one logical segment with no physical birth or node payload");

      auto blocked_batch = projected;
      ++blocked_batch.source_batch_index;
      const auto blocked_stamp =
          reducer.strict_locator().snapshot_stamp();
      const auto blocked_cursor = reducer.output_cursor();
      const auto blocked = reducer.fold(blocked_batch);
      check(
          blocked.certified_atomic_rejection() &&
              blocked.decision ==
                  ExactDirectMorseForestReducerFoldDecision::
                      no_reducer_output_segment_pending &&
              reducer.next_source_batch_index() == batch_index + 1U &&
              reducer.strict_locator().snapshot_stamp() == blocked_stamp &&
              reducer.output_cursor() == blocked_cursor &&
              reducer.pending_output_segment() != nullptr &&
              *reducer.pending_output_segment() == pending_before_drain,
          "a next fold is refused without mutation while one segment awaits acknowledgement");

      bool rejecting_sink_state = false;
      const auto rejected = reducer.drain_pending_output_segment(
          {&rejecting_sink_state, reject_segment});
      check(
          rejected.certified_retryable_rejection() &&
              rejected.decision ==
                  ExactDirectMorseForestSegmentDrainDecision::
                      no_sink_rejected &&
              rejected.pending_segment_retained &&
              reducer.output_cursor() == cursor_before &&
              reducer.pending_output_segment() != nullptr &&
              *reducer.pending_output_segment() == pending_before_drain,
          "a rejecting sink leaves the complete pending segment bit-for-bit retryable");
      retry_path_exercised = true;
    }

    const auto accepted = reducer.drain_pending_output_segment(
        {&run.segments, collect_segment});
    check(
        accepted.certified_acknowledged() &&
            accepted.decision ==
                ExactDirectMorseForestSegmentDrainDecision::
                    complete_segment_acknowledged &&
            accepted.reducer_output_history_released &&
            accepted.post_drain_cursor == pending_before_drain.end_cursor &&
            reducer.output_cursor() == pending_before_drain.end_cursor &&
            !reducer.has_pending_output_segment() &&
            reducer.pending_output_segment() == nullptr,
        "a successful sink acknowledgement advances only the scalar cursor and releases reducer-owned output");
  }

  check(
      retry_path_exercised && reducer.complete() &&
          run.segments.size() == scenario.event_journal.batches.size(),
      "the segmented fixture exercised retry and acknowledged every source batch once");
  run.seal = reducer.finish_segmented();
  return run;
}

[[nodiscard]] ExactDirectMorseForestJournalResult
run_integrated_adapter_stream(const Scenario& scenario) {
  reset_fake_gpu_phase14_facet_top_k_proposal();
  reset_fake_gpu_phase14_morton_lbvh_build();

  const auto industrial_config = plan_config(1U);
  const auto observed_plan =
      build_exact_direct_sparse_facet_descent_batch_plan(
          scenario.cloud,
          scenario.facade,
          scenario.event_journal,
          seed_budget(),
          scenario.seed_journal,
          industrial_config,
          plan_budget());
  check(
      observed_plan.complete_architecture_plan(),
      "the composed 14O-to-15D fixture has one complete industrial plan");

  ExactDirectMorseForestReducer reducer(
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal,
      forest_budget(),
      forest_config());
  ExactDirectSparseFacetDescentAnchoredBatchExecutor executor(
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal,
      industrial_config,
      plan_budget(),
      observed_plan,
      reducer.strict_locator());

  MortonLbvhBuildContext device_builder{scenario.cloud.size()};
  auto device_build = device_builder.build(scenario.cloud);
  auto device_lease =
      device_builder.release_device_lease(device_build);
  DirectSparseFacetTopKProposalContext proposal_context{
      device_build.certified_index(),
      scenario.cloud,
      std::move(device_lease),
      2U};
  DirectSparseFacetTopKIntegratedAdapter adapter{
      proposal_context,
      scenario.cloud,
      DirectSparseFacetTopKProposalPolicy{1U},
      proposal_budget()};
  const ExactDirectSparseFacetDescentBatchIntegratedRunBudget run_budget{
      2U, 8U, proposal_budget()};
  const auto prepare_inputs = adapter.prepare_inputs_callback();
  const auto seal_inputs = adapter.seal_inputs_callback();
  std::uint64_t clock_tick = 0U;
  const ExactDirectSparseFacetDescentBatchNanosecondClock clock =
      [&clock_tick]() {
        clock_tick += UINT64_C(10);
        return clock_tick;
      };

  std::size_t prepared_ticket_count = 0U;
  while (!executor.complete()) {
    const std::size_t batch_index =
        executor.next_source_batch_index();
    const ExactDirectSparseFacetWitness witness{
        authority_id,
        (static_cast<std::uint64_t>(batch_index) + 1U) * 3U};
    auto prepared = executor.prepare_next_integrated(
        witness,
        execution_budget(),
        forest_budget().closure_budget,
        run_budget,
        prepare_inputs,
        seal_inputs,
        clock);
    check(
        prepared.complete_architecture_preparation() &&
            prepared.prepared_ticket.has_value() &&
            executor.next_source_batch_index() == batch_index &&
            reducer.next_source_batch_index() == batch_index,
        "the real 14O adapter seals one 14L preparation without advancing either live cursor");
    if (!prepared.prepared_ticket.has_value()) {
      break;
    }
    ++prepared_ticket_count;
    const auto committed = reducer.fold_prepared_ticket(
        executor, std::move(*prepared.prepared_ticket));
    const bool live_commit_certified =
        committed.certified_live_commit() &&
        committed.reducer_fold.certified_committed_batch() &&
        executor.next_source_batch_index() == batch_index + 1U &&
        reducer.next_source_batch_index() == batch_index + 1U;
    check(
        live_commit_certified,
        "the adapter-backed 14H ticket folds through 15D before one cursor advance");
    if (!live_commit_certified) {
      std::cerr << "integrated live decision="
                << static_cast<unsigned>(committed.decision)
                << ", reducer decision="
                << static_cast<unsigned>(
                       committed.reducer_fold.decision)
                << '\n';
      break;
    }
  }

  const auto& adapter_audit = adapter.audit();
  check(
      prepared_ticket_count == scenario.event_journal.batches.size() &&
          adapter_audit.prepared_chunk_count == 2U &&
          adapter_audit.supported_query_count == 4U &&
          adapter_audit.snapshot_host_to_device_byte_count == 0U &&
          adapter_audit.every_chunk_used_adopted_device_snapshot &&
          adapter_audit.every_chunk_retained_snapshot_owner &&
          adapter_audit.every_chunk_was_proposal_only &&
          adapter_audit.aggregate_transcript_sealed &&
          !adapter_audit.forbidden_global_structure_materialized &&
          !adapter_audit.public_status_claimed,
      "the composed seam preserves adopted-snapshot, active-traffic-only and proposal-only evidence");
  check(
      executor.complete() && reducer.complete(),
      "the composed 14O-to-15D path consumes every scientific batch");
  return reducer.finish();
}

void test_final_root_budget_is_rejected_at_open() {
  const Scenario scenario = tetrahedron();
  auto insufficient = forest_budget();
  insufficient.maximum_final_root_count = 0U;
  bool rejected = false;
  try {
    ExactDirectMorseForestReducer reducer(
        scenario.cloud,
        scenario.facade,
        scenario.event_journal,
        seed_budget(),
        scenario.seed_journal,
        insufficient,
        forest_config());
    static_cast<void>(reducer);
  } catch (const std::invalid_argument&) {
    rejected = true;
  }
  check(
      rejected,
      "an insufficient derivable final-root budget is rejected at open");
}

void test_live_transaction_rejects_a_distinct_reducer_locator() {
  const Scenario scenario = tetrahedron();
  const auto industrial_config = plan_config(2U);
  const auto observed_plan =
      build_exact_direct_sparse_facet_descent_batch_plan(
          scenario.cloud,
          scenario.facade,
          scenario.event_journal,
          seed_budget(),
          scenario.seed_journal,
          industrial_config,
          plan_budget());
  ExactDirectMorseForestReducer bound_reducer(
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal,
      forest_budget(),
      forest_config());
  ExactDirectMorseForestReducer foreign_reducer(
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal,
      forest_budget(),
      forest_config());
  ExactDirectSparseFacetDescentAnchoredBatchExecutor executor(
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal,
      industrial_config,
      plan_budget(),
      observed_plan,
      bound_reducer.strict_locator());
  const ExactDirectSparseFacetWitness witness{authority_id, 3U};
  const auto transcript = empty_proposal_transcript(
      0U,
      scenario.event_journal.batches[0U].squared_level,
      bound_reducer.strict_locator());
  auto ticket =
      executor.prepare_next_sealed_with_top_k_proposal_transcript(
          witness,
          execution_budget(),
          forest_budget().closure_budget,
          transcript);
  const auto bound_stamp =
      bound_reducer.strict_locator().snapshot_stamp();
  const auto foreign_stamp =
      foreign_reducer.strict_locator().snapshot_stamp();
  const auto rejected =
      foreign_reducer.fold_prepared_ticket(
          executor, std::move(ticket));
  check(
      rejected.certified_atomic_rejection() &&
          rejected.decision ==
              ExactDirectMorseForestLiveCommitDecision::
                  no_live_commit_distinct_locator_or_snapshot &&
          executor.next_source_batch_index() == 0U &&
          bound_reducer.next_source_batch_index() == 0U &&
          foreign_reducer.next_source_batch_index() == 0U &&
          bound_reducer.strict_locator().snapshot_stamp() ==
              bound_stamp &&
          foreign_reducer.strict_locator().snapshot_stamp() ==
              foreign_stamp,
      "a live ticket cannot be folded through an equal-looking but distinct reducer locator");
}

void test_incremental_identity_and_chunk_independence() {
  const Scenario scenario = tetrahedron();
  check(
      scenario.event_journal
                  .materialized_direct_event_projections.size() ==
              scenario.event_journal.source_direct_event_count &&
          scenario.event_journal.materialized_direct_role_records.size() +
                  scenario.cloud.size() ==
              scenario.event_journal.role_record_count &&
          scenario.event_journal
              .canonical_singletons_implicit_and_unmaterialized,
      "the reducer source retains only the direct journal suffix");
  const auto resident = build_exact_direct_morse_forest_journal(
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal,
      forest_budget(),
      forest_config());
  const auto one_batch_chunks =
      run_stream(scenario, 1U, true, false);
  const auto two_batch_chunks =
      run_stream(scenario, 2U, false, true);
  const auto integrated_adapter =
      run_integrated_adapter_stream(scenario);

  check(
      resident.certified_conditional_h0_candidate() &&
          one_batch_chunks.certified_conditional_h0_candidate() &&
          two_batch_chunks.certified_conditional_h0_candidate() &&
          integrated_adapter.certified_conditional_h0_candidate() &&
          resident.source_higher_canonical_cloud_digest ==
              scenario.facade.certificate.higher_canonical_cloud_digest &&
          one_batch_chunks.source_higher_canonical_cloud_digest ==
              resident.source_higher_canonical_cloud_digest &&
          two_batch_chunks.source_higher_canonical_cloud_digest ==
              resident.source_higher_canonical_cloud_digest &&
          integrated_adapter.source_higher_canonical_cloud_digest ==
              resident.source_higher_canonical_cloud_digest,
      "resident and streaming reductions certify the same conditional scope");
  check(
      one_batch_chunks.implicit_order_one_prefix_count ==
              scenario.cloud.size() &&
          one_batch_chunks
              .order_one_birth_and_node_prefix_implicit_and_unmaterialized &&
          one_batch_chunks.birth_records.size() +
                  scenario.cloud.size() ==
              one_batch_chunks.counters.birth_record_count &&
          one_batch_chunks.nodes.size() + scenario.cloud.size() ==
              one_batch_chunks.counters.node_count,
      "the streaming reducer stores only the direct birth and node suffix");
  check(
      one_batch_chunks == resident &&
          two_batch_chunks == resident &&
          integrated_adapter == resident,
      "projected, live and real-adapter streaming outputs are recursively identical to resident output");
}

void test_source_authority_adapter_and_one_window_provider() {
  const Scenario scenario = tetrahedron();
  ExactDirectMorseForestResidentSourceAdapter adapter(
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal);
  check(
      adapter.manifest().certified() &&
          adapter.manifest().batch_count ==
              scenario.event_journal.batches.size(),
      "the resident adapter exposes one certified scalar source manifest");

  std::size_t visited_batch_count = 0U;
  std::size_t borrowed_role_count = 0U;
  std::size_t borrowed_family_count = 0U;
  std::size_t borrowed_arm_seed_count = 0U;
  std::size_t borrowed_projection_count = 0U;
  std::size_t borrowed_event_count = 0U;
  for (std::size_t batch_index = 0U;
       batch_index < adapter.manifest().batch_count;
       ++batch_index) {
    auto inspect = [&](const ExactDirectMorseForestSourceBatchWindow& window) {
      ++visited_batch_count;
      check(
          window.certified_relative_to(adapter.manifest()) &&
              window.resident_records_borrowed_without_copy &&
              window.synchronous_lifetime_only &&
              window.batch ==
                  &scenario.event_journal.batches[batch_index],
          "one resident source window borrows its batch descriptor in place");
      if (batch_index == 0U) {
        check(
            window.implicit_singleton_role_count ==
                    scenario.cloud.size() &&
                window.roles.empty(),
            "the resident singleton source prefix stays implicit");
      } else {
        const std::size_t physical_role_offset =
            window.logical_role_begin_index - scenario.cloud.size();
        check(
            window.implicit_singleton_role_count == 0U &&
                window.roles.data() ==
                    scenario.event_journal
                            .materialized_direct_role_records.data() +
                        physical_role_offset,
            "one resident role slice aliases the source journal without a copy");
      }
      if (!window.families.empty()) {
        check(
            window.families.data() ==
                scenario.seed_journal.families.data() +
                    window.family_begin_index,
            "one resident family slice aliases the source seed journal");
      }
      if (!window.arm_seeds.empty()) {
        check(
            window.arm_seeds.data() ==
                scenario.seed_journal.arm_seeds.data() +
                    window.arm_seed_begin_index,
            "one resident arm-seed slice aliases the source seed journal");
      }
      borrowed_role_count += window.roles.size();
      borrowed_family_count += window.families.size();
      borrowed_arm_seed_count += window.arm_seeds.size();

      for (const auto& role : window.roles) {
        const auto* projection =
            window.projections.find(role.event_projection_index);
        const bool projection_index_valid =
            role.event_projection_index >= scenario.cloud.size() &&
            role.event_projection_index - scenario.cloud.size() <
                scenario.event_journal
                    .materialized_direct_event_projections.size();
        check(
            projection_index_valid && projection != nullptr &&
                projection ==
                    &scenario.event_journal
                         .materialized_direct_event_projections[
                             role.event_projection_index -
                             scenario.cloud.size()],
            "the resident projection lookup returns the original journal record");
        if (projection == nullptr ||
            projection->source_index >= scenario.facade.events.size()) {
          continue;
        }
        ++borrowed_projection_count;
        const auto* event =
            window.events.find(projection->source_index);
        check(
            event == &scenario.facade.events[projection->source_index],
            "the resident event lookup returns the original terminal event");
        if (event != nullptr) {
          ++borrowed_event_count;
        }
      }
      for (const auto& family : window.families) {
        const auto* event = window.events.find(family.source_event_index);
        check(
            family.source_event_index < scenario.facade.events.size() &&
                event ==
                    &scenario.facade.events[family.source_event_index],
            "a resident saddle family borrows its original terminal event");
        if (event != nullptr) {
          ++borrowed_event_count;
        }
      }
      return true;
    };
    const auto decision = adapter.visit_batch(
        batch_index,
        ExactDirectMorseForestSourceBatchConsumerView{inspect});
    check(
        decision == ExactDirectMorseForestSourceBatchVisitDecision::
                        complete_synchronous_visit,
        "the resident adapter completes one synchronous zero-copy visit");
  }
  check(
      visited_batch_count == adapter.manifest().batch_count &&
          borrowed_role_count ==
              scenario.event_journal
                  .materialized_direct_role_records.size() &&
          borrowed_family_count == scenario.seed_journal.families.size() &&
          borrowed_arm_seed_count ==
              scenario.seed_journal.arm_seeds.size() &&
          borrowed_projection_count != 0U && borrowed_event_count != 0U,
      "the zero-copy visits cover every physical role, family and arm seed plus indexed projections and events");

  const auto legacy = build_exact_direct_morse_forest_journal(
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal,
      forest_budget(),
      forest_config());
  InstrumentedSourceProvider provider;
  provider.adapter = &adapter;
  provider.mutation_batch_index = 1U;
  const auto bounded = run_instrumented_source_provider_stream(
      scenario, adapter, provider);
  check(
      bounded == legacy &&
          bounded.certified_conditional_h0_candidate(),
      "the one-window provider reconstructs exactly the legacy resident forest");
  check(
      provider.live_window_count == 0U &&
          provider.maximum_live_window_count == 1U &&
          provider.acquire_count == provider.release_count &&
          provider.acquire_count == adapter.manifest().batch_count + 1U &&
          provider.overlap_rejection_count == 0U &&
          provider.mutation_delivery_count == 1U,
      "the provider retains at most one live source window and releases the rejected mutation before retry");
}

void test_canonical_source_batch_wire_and_provider() {
  const Scenario scenario = tetrahedron();
  ExactDirectMorseForestResidentSourceAdapter adapter(
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal);
  const ExactDirectMorseForestSourceWireLimits limits{};
  check(
      limits.finite(),
      "the canonical source wire starts from finite per-batch limits");

  std::vector<std::vector<std::uint8_t>> wires(
      adapter.manifest().batch_count);
  std::size_t nontrivial_batch_index =
      std::numeric_limits<std::size_t>::max();
  for (std::size_t batch_index = 0U;
       batch_index < adapter.manifest().batch_count;
       ++batch_index) {
    auto encode = [&](const ExactDirectMorseForestSourceBatchWindow& window) {
      const auto first =
          encode_exact_direct_morse_forest_source_batch_wire(
              adapter.manifest(), window, limits);
      const auto second =
          encode_exact_direct_morse_forest_source_batch_wire(
              adapter.manifest(), window, limits);
      if (batch_index == 0U) {
        constexpr std::array<std::uint8_t, 32U> golden_header_prefix{
            0x4dU, 0x33U, 0x44U, 0x31U, 0x35U, 0x4bU, 0x42U, 0x31U,
            0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U, 0x00U, 0x01U,
            0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
            0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U, 0x27U};
        // Golden provenance: minted with the wire at 91ed8a3; the encoder
        // never changed, but the G4-qualified shared-LBVH refactor 2ebd8c2
        // legitimately changed the upstream scenario content and only this
        // payload digest (size and header bytes unchanged), as established
        // by an eight-point bisection on 2026-08-06.  Re-minted accordingly.
        const auto golden_payload_digest =
            morsehgp3d::contract::CanonicalId::from_lower_hex(
                "6a73779a352efa2ca8f44de02dc461deb0642580b5bd54c211f3d89645e87006");
        check(
            first.bytes.size() == 359U &&
                std::equal(
                    golden_header_prefix.begin(),
                    golden_header_prefix.end(),
                    first.bytes.begin()) &&
                first.receipt.payload_digest == golden_payload_digest &&
                std::equal(
                    golden_payload_digest.bytes().begin(),
                    golden_payload_digest.bytes().end(),
                    first.bytes.begin() + 32),
            "the singleton wire v1 header and payload digest remain golden across toolchains");
      }
      const auto verified =
          verify_exact_direct_morse_forest_source_batch_wire(
              adapter.manifest(), window, first.bytes, limits);
      check(
          first.bytes == second.bytes &&
              first.receipt == second.receipt && verified.accepted() &&
              verified.receipt ==
                  std::optional<
                      ExactDirectMorseForestSourceBatchWireReceipt>{
                      first.receipt},
          "one fresh source window has a deterministic canonical wire and receipt");
      wires[batch_index] = first.bytes;
      if (!window.roles.empty() &&
          nontrivial_batch_index ==
              std::numeric_limits<std::size_t>::max()) {
        nontrivial_batch_index = batch_index;

        PaddingMutatedSourceLookups padded_lookups;
        padded_lookups.projections.reserve(window.roles.size());
        padded_lookups.events.reserve(window.roles.size());
        for (const auto& role : window.roles) {
          auto projection =
              *window.projections.find(role.event_projection_index);
          auto event = *window.events.find(projection.source_index);
          for (std::size_t index = projection.support_size;
               index < projection.support_ids.size();
               ++index) {
            projection.support_ids[index] = UINT64_C(0xf000000000000000) +
                                            index;
            event.support_ids[index] = UINT64_C(0xe000000000000000) + index;
          }
          padded_lookups.projections.push_back(std::move(projection));
          padded_lookups.events.push_back(std::move(event));
        }
        auto padded_window = window;
        padded_window.projections = ExactDirectMorseForestProjectionLookupView{
            &padded_lookups, find_padding_mutated_projection};
        padded_window.events = ExactDirectMorseForestEventLookupView{
            &padded_lookups, find_padding_mutated_event};
        const auto padded =
            encode_exact_direct_morse_forest_source_batch_wire(
                adapter.manifest(), padded_window, limits);
        check(
            padded.bytes == first.bytes && padded.receipt == first.receipt,
            "unused support padding is nonsemantic in the canonical source wire");

        auto corrupted = first.bytes;
        corrupted.back() ^= std::uint8_t{1U};
        auto invalid_header = first.bytes;
        invalid_header.front() ^= std::uint8_t{1U};
        auto truncated = first.bytes;
        truncated.pop_back();
        auto extended = first.bytes;
        extended.push_back(std::uint8_t{0U});
        const auto corrupt_result =
            verify_exact_direct_morse_forest_source_batch_wire(
                adapter.manifest(), window, corrupted, limits);
        const auto invalid_header_result =
            verify_exact_direct_morse_forest_source_batch_wire(
                adapter.manifest(), window, invalid_header, limits);
        const auto truncated_result =
            verify_exact_direct_morse_forest_source_batch_wire(
                adapter.manifest(), window, truncated, limits);
        const auto extended_result =
            verify_exact_direct_morse_forest_source_batch_wire(
                adapter.manifest(), window, extended, limits);
        check(
          corrupt_result.decision ==
                    ExactDirectMorseForestSourceWireDecision::
                        wire_content_mismatch &&
                invalid_header_result.decision ==
                    ExactDirectMorseForestSourceWireDecision::
                        wire_content_mismatch &&
                truncated_result.decision ==
                    ExactDirectMorseForestSourceWireDecision::
                        wire_size_mismatch &&
                extended_result.decision ==
                    ExactDirectMorseForestSourceWireDecision::
                        wire_size_mismatch,
            "one payload corruption, invalid header, truncation and trailing byte fail closed without decoding a scientific record");

        auto byte_tight = limits;
        byte_tight.maximum_encoded_byte_count = first.bytes.size() - 1U;
        auto role_tight = limits;
        role_tight.maximum_role_record_count = window.roles.size() - 1U;
        auto exact_tight = limits;
        exact_tight.maximum_exact_text_byte_count = 2U;
        const auto byte_result =
            verify_exact_direct_morse_forest_source_batch_wire(
                adapter.manifest(), window, first.bytes, byte_tight);
        const auto role_result =
            verify_exact_direct_morse_forest_source_batch_wire(
                adapter.manifest(), window, first.bytes, role_tight);
        const auto exact_result =
            verify_exact_direct_morse_forest_source_batch_wire(
                adapter.manifest(), window, first.bytes, exact_tight);
        check(
            byte_result.decision ==
                    ExactDirectMorseForestSourceWireDecision::
                        encoded_byte_limit_exceeded &&
                role_result.decision ==
                    ExactDirectMorseForestSourceWireDecision::
                        role_record_limit_exceeded &&
                exact_result.decision ==
                    ExactDirectMorseForestSourceWireDecision::
                        exact_text_limit_exceeded,
            "exact wire, population and rational-text caps reject the whole batch before publication");
      }
      return true;
    };
    const auto decision = adapter.visit_batch(
        batch_index,
        ExactDirectMorseForestSourceBatchConsumerView{encode});
    check(
        decision == ExactDirectMorseForestSourceBatchVisitDecision::
                        complete_synchronous_visit,
        "the source archive fixture encodes one borrowed batch at a time");
  }
  check(
      nontrivial_batch_index !=
          std::numeric_limits<std::size_t>::max(),
      "the canonical wire fixture contains a nontrivial direct source batch");
  auto unbounded = limits;
  unbounded.maximum_event_record_count =
      std::numeric_limits<std::size_t>::max();
  check(
      !unbounded.finite(),
      "an effectively unbounded source wire configuration is rejected");

  InstrumentedSourceProvider fresh_provider;
  fresh_provider.adapter = &adapter;
  InMemorySourceWireProvider wire_provider;
  wire_provider.wires = &wires;
  wire_provider.mutation_batch_index = nontrivial_batch_index;
  ExactDirectMorseForestCanonicalWireSourceProvider canonical_provider(
      adapter.manifest(),
      ExactDirectMorseForestSourceBatchProviderView{fresh_provider},
      ExactDirectMorseForestSourceBatchWireProviderView{wire_provider},
      limits);
  InstrumentedCanonicalWireSourceProvider instrumented;
  instrumented.provider = &canonical_provider;
  instrumented.wire_provider = &wire_provider;
  instrumented.mutation_batch_index = nontrivial_batch_index;

  const auto legacy = build_exact_direct_morse_forest_journal(
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal,
      forest_budget(),
      forest_config());
  const auto bounded = run_instrumented_source_provider_stream(
      scenario, adapter, instrumented);
  check(
      bounded == legacy && bounded.certified_conditional_h0_candidate(),
      "the canonical-wire provider produces exactly the legacy resident forest");
  check(
      canonical_provider.verified_visit_count() ==
              adapter.manifest().batch_count &&
          canonical_provider.last_wire_decision() ==
              ExactDirectMorseForestSourceWireDecision::accepted &&
          fresh_provider.live_window_count == 0U &&
          fresh_provider.maximum_live_window_count == 1U &&
          fresh_provider.acquire_count == fresh_provider.release_count &&
          fresh_provider.acquire_count ==
              adapter.manifest().batch_count + 1U &&
          fresh_provider.overlap_rejection_count == 0U &&
          wire_provider.live_wire_count == 0U &&
          wire_provider.maximum_live_wire_count == 1U &&
          wire_provider.acquire_count == wire_provider.release_count &&
          wire_provider.acquire_count ==
              adapter.manifest().batch_count + 1U &&
          wire_provider.overlap_rejection_count == 0U &&
          wire_provider.mutation_delivery_count == 1U &&
          instrumented.first_mutation_wire_decision ==
              std::optional<ExactDirectMorseForestSourceWireDecision>{
                  ExactDirectMorseForestSourceWireDecision::
                      wire_content_mismatch},
      "one corrupted wire is rejected atomically, retried, and both source windows are released synchronously");
}

void test_segmented_output_identity_retry_and_no_history() {
  const Scenario scenario = tetrahedron();
  const auto resident = build_exact_direct_morse_forest_journal(
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal,
      forest_budget(),
      forest_config());
  const auto segmented = run_segmented_stream(scenario);

  std::vector<ExactDirectMorseForestBirthRecord> birth_records;
  std::vector<ExactDirectMorseForestArmRootBinding> arm_root_bindings;
  std::vector<ExactDirectMorseForestSaddleRecord> saddle_records;
  std::vector<ExactDirectMorseForestAtomicGroup> atomic_groups;
  std::vector<ExactDirectMorseForestNodeId> child_node_ids;
  std::vector<ExactDirectMorseForestBatch> batches;
  std::vector<ExactDirectMorseForestNode> nodes;
  ExactDirectMorseForestSegmentCursor expected_begin{};
  expected_begin.chain_digest =
      morsehgp3d::contract::CanonicalId{};

  for (std::size_t index = 0U;
       index < segmented.segments.size();
       ++index) {
    const auto& segment = segmented.segments[index];
    check(
        segment.certified_structure() &&
            segment.begin_cursor == expected_begin &&
            segment.end_cursor.segment_count == index + 1U,
        "acknowledged forest segments form one dense cursor chain");
    birth_records.insert(
        birth_records.end(),
        segment.birth_records.begin(),
        segment.birth_records.end());
    arm_root_bindings.insert(
        arm_root_bindings.end(),
        segment.arm_root_bindings.begin(),
        segment.arm_root_bindings.end());
    saddle_records.insert(
        saddle_records.end(),
        segment.saddle_records.begin(),
        segment.saddle_records.end());
    atomic_groups.insert(
        atomic_groups.end(),
        segment.atomic_groups.begin(),
        segment.atomic_groups.end());
    child_node_ids.insert(
        child_node_ids.end(),
        segment.child_node_ids.begin(),
        segment.child_node_ids.end());
    batches.push_back(segment.batch);
    nodes.insert(
        nodes.end(), segment.nodes.begin(), segment.nodes.end());
    expected_begin = segment.end_cursor;
  }

  check(
      birth_records == resident.birth_records &&
          arm_root_bindings == resident.arm_root_bindings &&
          saddle_records == resident.saddle_records &&
          atomic_groups == resident.atomic_groups &&
          child_node_ids == resident.child_node_ids &&
          batches == resident.batches && nodes == resident.nodes,
      "concatenated segment payloads preserve every resident global index, offset and node identifier");
  check(
      segmented.seal.certified_conditional_h0_candidate() &&
          segmented.seal.point_count == resident.point_count &&
          segmented.seal.effective_maximum_order ==
              resident.effective_maximum_order &&
          segmented.seal.source_higher_canonical_cloud_digest ==
              resident.source_higher_canonical_cloud_digest &&
          segmented.seal.final_cursor == expected_begin &&
          segmented.seal.final_locator_stamp ==
              resident.final_locator_stamp &&
          segmented.seal.counters == resident.counters &&
          segmented.seal.final_roots == resident.final_roots &&
          segmented.seal.logical_output_entry_count ==
              resident.logical_output_entry_count,
      "the O(K) segmented seal closes the same counters, roots and locator stamp as the resident journal");
  auto forged_seal = segmented.seal;
  forged_seal.source_higher_canonical_cloud_digest = {};
  check(
      !forged_seal.certified_conditional_h0_candidate(),
      "the segmented seal rejects erasure of its source cloud identity");
  check(
      expected_begin.segment_count == resident.batches.size() &&
          expected_begin.implicit_order_one_prefix_count ==
              resident.implicit_order_one_prefix_count &&
          expected_begin.birth_record_count ==
              resident.birth_records.size() &&
          expected_begin.arm_root_binding_count ==
              resident.arm_root_bindings.size() &&
          expected_begin.saddle_record_count ==
              resident.saddle_records.size() &&
          expected_begin.atomic_group_count ==
              resident.atomic_groups.size() &&
          expected_begin.child_reference_count ==
              resident.child_node_ids.size() &&
          expected_begin.batch_record_count ==
              resident.batches.size() &&
          expected_begin.node_count == resident.nodes.size(),
      "the final scalar cursor counts the implicit prefix and all acknowledged physical suffixes");

  // 15L resident--stream--restore identity: the acknowledged segments and
  // the terminal seal of the real reducer pipeline survive an exercised
  // interruption, an atomic durable publication and a bounded reload,
  // field-exact against the stream they came from.
  char directory_template[] = "/tmp/ehgp-reducer-durable-XXXXXX";
  const char* directory = ::mkdtemp(directory_template);
  check(
      directory != nullptr,
      "the durable identity fixture directory is created");
  if (directory == nullptr) {
    return;
  }
  const std::string directory_path{directory};
  const std::string final_name{"reducer-run.mh3d"};
  ExactDirectMorseForestSegmentCodecLimits codec_limits;
  codec_limits.segment_limits = segment_limits();
  codec_limits.maximum_final_root_count = 4096U;
  ExactDirectMorseForestRunArchiveIdentity identity;
  identity.manifest_digest = morsehgp3d::contract::CanonicalId{};
  identity.source_identity_digest =
      digest_text("reducer-fixture-source-identity");
  identity.source_higher_canonical_cloud_digest =
      segmented.seal.source_higher_canonical_cloud_digest;
  identity.terminal_source_chain_digest =
      digest_text("reducer-fixture-terminal-source-chain");
  identity.point_count = segmented.seal.point_count;
  identity.effective_maximum_order =
      segmented.seal.effective_maximum_order;

  auto interrupted_decision =
      ExactDirectMorseForestRunArchiveDecision::not_certified;
  {
    auto interrupted = ExactDirectMorseForestRunArchiveWriter::open_new(
        directory_path,
        final_name,
        identity,
        codec_limits,
        interrupted_decision);
    check(
        interrupted.open() &&
            !segmented.segments.empty() &&
            interrupted.append_segment(segmented.segments.front()) ==
                ExactDirectMorseForestRunArchiveDecision::
                    complete_atomic_run_archive,
        "the interrupted durable writer appends the first real segment");
    // Destruction before finalize is the exercised interruption: the
    // temporary must vanish and no final file may appear.
  }
  const auto post_interruption =
      recover_exact_direct_morse_forest_run_archive_directory(
          directory_path, final_name, identity, codec_limits);
  check(
      post_interruption.certified_recovered() &&
          post_interruption.decision ==
              ExactDirectMorseForestRunArchiveRecoveryDecision::absent,
      "an exercised interruption leaves a clean absent archive namespace");

  auto publish_decision =
      ExactDirectMorseForestRunArchiveDecision::not_certified;
  auto writer = ExactDirectMorseForestRunArchiveWriter::open_new(
      directory_path, final_name, identity, codec_limits, publish_decision);
  check(
      writer.open(),
      "the durable writer reopens after the exercised interruption");
  for (const auto& segment : segmented.segments) {
    check(
        writer.append_segment(segment) ==
            ExactDirectMorseForestRunArchiveDecision::
                complete_atomic_run_archive,
        "every acknowledged real segment appends in chain order");
  }
  const auto published = writer.finalize(segmented.seal);
  check(
      published.certified_published() &&
          published.appended_segment_count == segmented.segments.size(),
      "the real segmented run publishes atomically");

  std::vector<ExactDirectMorseForestBatchSegment> restored;
  const auto restored_consume =
      [](void* state,
         const ExactDirectMorseForestBatchSegment& segment) noexcept {
        try {
          static_cast<std::vector<ExactDirectMorseForestBatchSegment>*>(
              state)
              ->push_back(segment);
          return true;
        } catch (...) {
          return false;
        }
      };
  const auto loaded = load_exact_direct_morse_forest_run_archive(
      directory_path,
      final_name,
      identity,
      codec_limits,
      {&restored, restored_consume});
  check(
      loaded.certified_loaded() &&
          restored.size() == segmented.segments.size() &&
          std::equal(
              restored.begin(),
              restored.end(),
              segmented.segments.begin()) &&
          loaded.final_seal == segmented.seal &&
          loaded.archive_digest == published.archive_digest,
      "the bounded reload restores the real stream and its terminal seal "
      "field-exact");
  const auto recovered =
      recover_exact_direct_morse_forest_run_archive_directory(
          directory_path, final_name, identity, codec_limits);
  check(
      recovered.certified_recovered() &&
          recovered.decision ==
              ExactDirectMorseForestRunArchiveRecoveryDecision::
                  valid_final_present,
      "recovery certifies the published real run as the valid final");
}

void test_segment_cap_rejects_before_locator_mutation() {
  const Scenario scenario = tetrahedron();
  const auto industrial_config = plan_config(2U);
  const auto observed_plan =
      build_exact_direct_sparse_facet_descent_batch_plan(
          scenario.cloud,
          scenario.facade,
          scenario.event_journal,
          seed_budget(),
          scenario.seed_journal,
          industrial_config,
          plan_budget());
  const ExactDirectMorseForestSegmentLimits zero_payload_limits{};
  ExactDirectMorseForestReducer reducer(
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal,
      forest_budget(),
      forest_config(),
      zero_payload_limits,
      morsehgp3d::contract::CanonicalId{});
  ExactDirectSparseFacetDescentAnchoredBatchExecutor executor(
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal,
      industrial_config,
      plan_budget(),
      observed_plan,
      reducer.strict_locator());
  std::size_t discard_state = 0U;
  bool cap_rejection_exercised = false;

  while (!executor.complete() && !cap_rejection_exercised) {
    const std::size_t batch_index = executor.next_source_batch_index();
    const ExactDirectSparseFacetWitness witness{
        authority_id,
        (static_cast<std::uint64_t>(batch_index) + 1U) * 3U};
    const auto delta = executor.prepare_next(
        witness,
        execution_budget(),
        forest_budget().closure_budget);
    const auto replay = executor.commit_prepared(
        witness,
        execution_budget(),
        forest_budget().closure_budget,
        delta);
    check(
        delta.complete_architecture_execution() &&
            replay.result_certified && replay.session_advanced,
        "the segment-cap fixture produces one certified source delta");
    const auto projected =
        project_exact_direct_morse_forest_reducer_batch(delta);
    const auto cursor_before = reducer.output_cursor();
    const auto stamp_before = reducer.strict_locator().snapshot_stamp();
    const std::size_t reducer_batch_before =
        reducer.next_source_batch_index();
    const auto folded = reducer.fold(projected);
    if (folded.certified_committed_batch()) {
      const auto drained = reducer.drain_pending_output_segment(
          {&discard_state, discard_segment});
      check(
          drained.certified_acknowledged(),
          "an empty segment within the zero payload caps can be acknowledged");
      continue;
    }
    cap_rejection_exercised = true;
    check(
        folded.certified_atomic_rejection() &&
            folded.decision ==
                ExactDirectMorseForestReducerFoldDecision::
                    no_reducer_budget_exhausted &&
            reducer.next_source_batch_index() == reducer_batch_before &&
            reducer.strict_locator().snapshot_stamp() == stamp_before &&
            reducer.output_cursor() == cursor_before &&
            !reducer.has_pending_output_segment(),
        "an insufficient per-segment payload cap rejects before locator, reducer cursor or output mutation");
  }
  check(
      cap_rejection_exercised,
      "the tetrahedron fixture reaches a nonempty physical forest segment");
}

void test_gabriel_arm_may_descend_to_a_different_terminal_key() {
  const Scenario scenario = gabriel_ac_to_de();
  const auto resident = build_exact_direct_morse_forest_journal(
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget(),
      scenario.seed_journal,
      forest_budget(),
      forest_config());
  const auto streaming = run_stream(scenario, 2U, false, true);
  check(
      streaming == resident &&
          streaming.certified_conditional_h0_candidate(),
      "the permanent Gabriel AC-to-DE descent stays identical through the streaming reducer");
}

}  // namespace

int main() {
  test_final_root_budget_is_rejected_at_open();
  test_live_transaction_rejects_a_distinct_reducer_locator();
  test_incremental_identity_and_chunk_independence();
  test_source_authority_adapter_and_one_window_provider();
  test_canonical_source_batch_wire_and_provider();
  test_segmented_output_identity_retry_and_no_history();
  test_segment_cap_rejects_before_locator_mutation();
  test_gabriel_arm_may_descend_to_a_different_terminal_key();
  if (failures != 0) {
    std::cerr << failures << " direct Morse forest reducer test(s) failed\n";
    return 1;
  }
  std::cout << "direct Morse forest reducer tests passed\n";
  return 0;
}
