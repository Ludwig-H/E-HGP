#include "morsehgp3d/hierarchy/direct_morse_chunk_run.hpp"
#include "morsehgp3d/hierarchy/direct_morse_chunk_forest_recovery.hpp"
#include "morsehgp3d/hierarchy/direct_morse_durable_live_commit.hpp"

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include <unistd.h>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::contract::CanonicalId;
using morsehgp3d::contract::CanonicalSha256Builder;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactLbvhTopKBudget;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

constexpr std::uint64_t authority_id = UINT64_C(0x15B);
int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

class TemporaryWorkspace {
 public:
  TemporaryWorkspace() {
    temporary_parent_ = std::filesystem::temp_directory_path();
    std::string pattern =
        (temporary_parent_ / "morsehgp3d-chunk-run-XXXXXX").string();
    std::vector<char> writable(pattern.begin(), pattern.end());
    writable.push_back('\0');
    char* const created = ::mkdtemp(writable.data());
    if (created == nullptr) {
      throw std::system_error(
          errno, std::generic_category(), "mkdtemp failed");
    }
    root_ = std::filesystem::path{created};
  }

  ~TemporaryWorkspace() {
    if (!root_.empty() && root_.parent_path() == temporary_parent_ &&
        root_.filename().string().starts_with(
            "morsehgp3d-chunk-run-")) {
      std::error_code ignored;
      static_cast<void>(std::filesystem::remove_all(root_, ignored));
    }
  }

  TemporaryWorkspace(const TemporaryWorkspace&) = delete;
  TemporaryWorkspace& operator=(const TemporaryWorkspace&) = delete;

  [[nodiscard]] std::filesystem::path make_directory() const {
    const std::filesystem::path directory = root_ / "run";
    if (!std::filesystem::create_directory(directory)) {
      throw std::runtime_error(
          "cannot create a direct Morse chunk-run test directory");
    }
    return directory;
  }

 private:
  std::filesystem::path temporary_parent_;
  std::filesystem::path root_;
};

[[nodiscard]] CanonicalId digest(std::string_view text) {
  CanonicalSha256Builder builder;
  builder.update("MorseHGP3D/direct-morse-chunk-run/test/");
  builder.update(text);
  return builder.finalize();
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] ExactPairSupportStreamBudget source_pair_budget() {
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

[[nodiscard]] ExactHigherSupportStreamBudget source_higher_budget() {
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

[[nodiscard]] ExactDirectSaddleArmSeedBudget source_seed_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

struct Scenario {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult event_journal;
  ExactDirectSaddleArmSeedJournalResult seed_journal;
};

[[nodiscard]] Scenario regular_tetrahedron_order_one_scenario() {
  const std::array<CertifiedPoint3, 4U> points{
      point(1.0, 1.0, 1.0),
      point(-1.0, -1.0, 1.0),
      point(-1.0, 1.0, -1.0),
      point(1.0, -1.0, -1.0),
  };
  CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(
          std::span<const CertifiedPoint3>{points});
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactDirectSupportTerminalBudget terminal_budget{
      source_pair_budget(), source_higher_budget()};
  const auto pair = build_exact_pair_support_stream(
      index, cloud, 1U, terminal_budget.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, 1U, terminal_budget.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index, cloud, 1U, terminal_budget, pair, higher);
  auto event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  auto seed_journal = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, event_journal, source_seed_budget());
  return {
      std::move(cloud),
      std::move(index),
      std::move(facade),
      std::move(event_journal),
      std::move(seed_journal),
  };
}

[[nodiscard]] ExactDirectMorseIndustrialPlanConfig plan_config(
    std::uint64_t maximum_batch_count = 1U) {
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
  return {
      3U,
      6U,
      12U,
      120U,
      4U,
  };
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

[[nodiscard]] ExactDirectSparseFacetDescentStepBudget step_budget() {
  return {
      ExactDirectSparsePositiveFacetProbeBudget{513U, 256U},
      ExactLbvhTopKBudget{
          4096U, 4096U, 4096U, 4096U, 256U, 16U, 16U},
      ExactDirectSparsePositiveFacetProbeBudget{513U, 256U},
  };
}

[[nodiscard]] ExactDirectSparseFacetDescentClosureBudget closure_budget() {
  return {
      256U,
      256U,
      256U,
      513U,
      step_budget(),
  };
}

[[nodiscard]] ExactDirectSparsePositiveFacetLocatorBudget locator_budget() {
  return {
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
  budget.locator_budget = locator_budget();
  budget.closure_budget = closure_budget();
  budget.quotient_budget = {256U, 256U, 256U, 256U, 1024U};
  return budget;
}

[[nodiscard]] ExactDirectMorseForestConfig forest_config() {
  ExactDirectMorseForestConfig config;
  config.locator_config.external_authority_id = authority_id;
  return config;
}

[[nodiscard]] ExactDirectSparseFacetWitness query_witness(
    std::uint64_t replay_token) {
  return {authority_id, replay_token};
}

[[nodiscard]] ExactDirectSparsePositiveFacetLocator positive_locator() {
  ExactDirectSparsePositiveFacetLocator locator =
      build_exact_direct_sparse_positive_facet_locator(
          4U,
          locator_budget(),
          ExactDirectSparsePositiveFacetLocatorConfig{
              authority_id,
              std::numeric_limits<std::uint64_t>::max()});
  std::array<ExactDirectSparseFacetBinding, 4U> bindings{};
  for (std::size_t key_index = 0U;
       key_index < bindings.size();
       ++key_index) {
    ExactDirectSparseFacetKey key;
    key.point_ids[0U] = static_cast<PointId>(key_index);
    key.point_count = 1U;
    bindings[key_index] = {
        key_index,
        key,
        key_index,
        query_witness(
            static_cast<std::uint64_t>(key_index) * UINT64_C(3) +
            UINT64_C(1))};
  }
  const auto committed = locator.apply_batch(
      std::span<const ExactDirectSparseFacetQuery>{},
      std::span<const ExactDirectSparseComponentUnion>{},
      bindings);
  if (!committed.certified_committed_batch()) {
    throw std::runtime_error(
        "cannot bind the four positive singleton keys");
  }
  return locator;
}

[[nodiscard]] ExactDirectSparsePositiveFacetLocator empty_locator() {
  return build_exact_direct_sparse_positive_facet_locator(
      4U,
      locator_budget(),
      ExactDirectSparsePositiveFacetLocatorConfig{
          authority_id,
          std::numeric_limits<std::uint64_t>::max()});
}

struct LocatorPrefixes {
  ExactDirectSparsePositiveFacetLocator before_batch_zero;
  ExactDirectSparsePositiveFacetLocator before_batch_one;
};

[[nodiscard]] LocatorPrefixes locator_prefixes() {
  return {empty_locator(), positive_locator()};
}

[[nodiscard]] ExactDirectSparseFacetDescentBatchPlanResult build_plan(
    const Scenario& scenario,
    const ExactDirectSaddleArmSeedBudget& seed_budget,
    const ExactDirectMorseIndustrialPlanConfig& industrial_config,
    const ExactDirectSparseFacetDescentBatchPlanBudget& batch_plan_budget) {
  return build_exact_direct_sparse_facet_descent_batch_plan(
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget,
      scenario.seed_journal,
      industrial_config,
      batch_plan_budget);
}

[[nodiscard]]
std::array<ExactDirectMorseChunkBatchReplayAuthority, 2U>
batch_authorities(
    const LocatorPrefixes& locators) {
  const ExactDirectMorseChunkBatchReplayAuthority::ResourceEnvelope
      resources{
          0U,
          16U * 1024U,
          16U * 1024U,
          UINT64_C(1'000'000)};
  return {{
      {locators.before_batch_zero.snapshot_stamp(),
       query_witness(UINT64_C(3)),
       execution_budget(),
       closure_budget(),
       resources},
      {locators.before_batch_one.snapshot_stamp(),
       query_witness(UINT64_C(6)),
       execution_budget(),
       closure_budget(),
       resources},
  }};
}

[[nodiscard]] ExactDirectMorseChunkBatchLocatorResolverFunction
locator_resolver(
    const LocatorPrefixes& locators) {
  return [&locators](
             std::size_t batch_index,
             const ExactDirectSparsePositiveFacetLocatorSnapshotStamp&
                 requested_stamp)
             -> std::shared_ptr<
                 const ExactDirectSparsePositiveFacetLocator> {
    const ExactDirectSparsePositiveFacetLocator* locator = nullptr;
    if (batch_index == 0U) {
      locator = &locators.before_batch_zero;
    } else if (batch_index == 1U) {
      locator = &locators.before_batch_one;
    }
    if (locator == nullptr ||
        requested_stamp != locator->snapshot_stamp()) {
      return {};
    }
    return {
        locator,
        [](const ExactDirectSparsePositiveFacetLocator*) noexcept {}};
  };
}

static_assert(
    std::is_constructible_v<
        ExactDirectMorseChunkBatchLocatorResolverView,
        ExactDirectMorseChunkBatchLocatorResolverFunction&>);
static_assert(
    !std::is_constructible_v<
        ExactDirectMorseChunkBatchLocatorResolverView,
        ExactDirectMorseChunkBatchLocatorResolverFunction&&>);

[[nodiscard]] ExactDirectMorseChunkRunLimits chunk_run_limits(
    std::size_t maximum_batch_count = 1U,
    std::size_t maximum_payload_byte_count = 64U * 1024U) {
  return {
      maximum_batch_count,
      64U * 1024U,
      8U * 1024U,
      4U,
      12U,
      maximum_payload_byte_count,
      UINT64_C(1'000'000),
      UINT64_C(1'000'000),
      4096U,
  };
}

[[nodiscard]] AtomicLinearRunStoreLimits store_limits() {
  constexpr std::size_t payload_limit = 64U * 1024U;
  constexpr std::size_t transition_limit =
      atomic_linear_run_transition_fixed_wire_byte_count +
      payload_limit;
  return {
      2U,
      payload_limit,
      transition_limit,
      2U * transition_limit,
      1U,
  };
}

[[nodiscard]] std::unique_ptr<ExactDirectMorseBudgetTracker>
budget_tracker_with_policy(
    ExactDirectMorseEffectiveBudgetPolicy policy,
    std::uint64_t committed_elapsed_ns = 0U,
    ExactDirectMorseBudgetNanosecondClock clock = {}) {
  if (!clock) {
    auto current = std::make_shared<std::uint64_t>(UINT64_C(42));
    clock = [current] {
      const std::uint64_t value = *current;
      ++*current;
      return value;
    };
  }
  return std::make_unique<ExactDirectMorseBudgetTracker>(
      policy,
      committed_elapsed_ns,
      std::move(clock));
}

[[nodiscard]] std::unique_ptr<ExactDirectMorseBudgetTracker>
budget_tracker(
    std::uint64_t limit = UINT64_C(1) << 40U,
    std::uint64_t committed_elapsed_ns = 0U,
    ExactDirectMorseBudgetNanosecondClock clock = {}) {
  return budget_tracker_with_policy(
      ExactDirectMorseEffectiveBudgetPolicy{
          limit, limit, limit, limit, limit},
      committed_elapsed_ns,
      std::move(clock));
}

[[nodiscard]] bool no_forbidden_context_ownership(
    const ExactDirectMorseChunkRunAudit& audit) {
  return audit.cursor_progression_count == 0U &&
         audit.context_owned_process_local_ticket_count == 0U &&
         audit.context_owned_historical_locator_count == 0U &&
         audit.context_owned_dsu_parent_count == 0U &&
         audit.context_owned_forest_record_count == 0U &&
         audit.context_owned_global_facet_or_coface_count == 0U &&
         audit.context_owned_cell_gamma_or_delaunay_count == 0U &&
         !audit.external_locator_resolver_residency_audited &&
         !audit.external_locator_resolver_cache_absence_claimed;
}

struct UnlinkDurableLiveHeadState {
  std::string head_temporary_path;
  bool unlink_succeeded{false};
};

void unlink_durable_live_head_before_commit(
    AtomicLinearRunPublishStage stage,
    void* opaque) noexcept {
  if (stage != AtomicLinearRunPublishStage::
                   head_temporary_file_synchronized_and_reread) {
    return;
  }
  auto* const state =
      static_cast<UnlinkDurableLiveHeadState*>(opaque);
  state->unlink_succeeded =
      ::unlink(state->head_temporary_path.c_str()) == 0;
}

void test_two_real_chunks_resume_with_fresh_context() {
  TemporaryWorkspace workspace;
  const std::filesystem::path directory = workspace.make_directory();
  const CanonicalId initial_checkpoint =
      digest("initial-checkpoint");
  const CanonicalId initial_output_chain =
      digest("initial-output-chain");
  const auto run_limits = chunk_run_limits();
  const auto linear_limits = store_limits();

  Scenario first_scenario =
      regular_tetrahedron_order_one_scenario();
  const auto first_seed_budget = source_seed_budget();
  const auto first_industrial_config = plan_config();
  const auto first_plan_budget = plan_budget();
  const auto first_plan = build_plan(
      first_scenario,
      first_seed_budget,
      first_industrial_config,
      first_plan_budget);
  LocatorPrefixes first_locators = locator_prefixes();
  const auto first_authorities = batch_authorities(first_locators);
  auto first_locator_resolver = locator_resolver(first_locators);
  check(
      first_plan.complete_architecture_plan() &&
          first_scenario.event_journal.batches.size() == 2U &&
          first_plan.source_industrial_plan.chunks.size() == 2U &&
          first_plan.source_industrial_plan.chunks[0U]
                  .source_batch_begin_index == 0U &&
          first_plan.source_industrial_plan.chunks[0U]
                  .source_batch_end_index == 1U &&
          first_plan.source_industrial_plan.chunks[1U]
                  .source_batch_begin_index == 1U &&
          first_plan.source_industrial_plan.chunks[1U]
                  .source_batch_end_index == 2U,
      "massive_external_streaming with a one-batch cap yields two real one-batch 14A chunks");
  if (!first_plan.complete_architecture_plan() ||
      first_plan.source_industrial_plan.chunks.size() != 2U) {
    return;
  }

  auto first_context =
      std::make_unique<ExactDirectMorseChunkRunContext>(
          first_scenario.index,
          first_scenario.cloud,
          first_scenario.facade,
          first_scenario.event_journal,
          first_seed_budget,
          first_scenario.seed_journal,
          first_industrial_config,
          first_plan_budget,
          first_plan,
          ExactDirectMorseChunkBatchLocatorResolverView{
              first_locator_resolver},
          first_authorities,
          run_limits,
          budget_tracker());
  const auto first_contract = first_context->run_contract(
      initial_checkpoint, initial_output_chain);
  AtomicLinearRunExternalAnchor first_anchor;
  {
    AtomicLinearRunStore store = first_context->create_new_store(
        directory,
        initial_checkpoint,
        initial_output_chain,
        linear_limits);
    const auto first_published =
        first_context->publish_next_chunk(store);
    check(
        first_published.decision ==
                AtomicLinearRunPublishDecision::durably_published &&
            first_published.trusted_state_advanced &&
            store.trusted_state().next_chunk_index == 1U &&
            store.trusted_state().next_batch_index == 1U,
        "the empty first exact batch is recertified and durably published as complete chunk zero");
    first_anchor = first_published.current_anchor;

    const auto abandoned = first_context->prepare_chunk(
        store.trusted_state(), 1U);
    check(
        abandoned.chunk_index == 1U &&
            abandoned.batch_begin_index == 1U &&
            abandoned.batch_end_index == 2U &&
            !abandoned.payload.empty() &&
            store.status().committed_transition_count == 1U &&
            store.trusted_state().next_chunk_index == 1U,
        "forming chunk one without publication simulates an abandoned attempt without advancing durable state");
  }

  const ExactDirectMorseChunkRunAudit first_audit =
      first_context->audit();
  check(
      first_audit.source_plan_reconstruction_count == 1U &&
          first_audit.indexed_batch_cursor_count == 2U &&
          first_audit.arbitrary_batch_replay_count == 3U &&
          first_audit.publication_recertification_count == 1U &&
          first_audit.recovery_recertification_count == 0U &&
          first_audit.cleanup_recertification_count == 0U &&
          first_audit.rejected_recertification_count == 0U &&
          first_audit.maximum_chunk_resolved_key_count == 4U &&
          first_audit.maximum_chunk_arm_join_count == 12U &&
          first_audit.maximum_live_external_locator_count == 1U &&
          first_audit.fresh_budget_evaluation_count == 7U &&
          first_audit.rejected_budget_evaluation_count == 0U &&
          no_forbidden_context_ownership(first_audit),
      "the first context reconstructs and indexes once, replays twice for publication plus once for the abandoned attempt, freshly evaluates each application budget gate, and owns no forbidden state");
  first_context.reset();

  Scenario resumed_scenario =
      regular_tetrahedron_order_one_scenario();
  const auto resumed_seed_budget = source_seed_budget();
  const auto resumed_industrial_config = plan_config();
  const auto resumed_plan_budget = plan_budget();
  const auto resumed_plan = build_plan(
      resumed_scenario,
      resumed_seed_budget,
      resumed_industrial_config,
      resumed_plan_budget);
  LocatorPrefixes resumed_locators = locator_prefixes();
  const auto resumed_authorities =
      batch_authorities(resumed_locators);
  auto resumed_locator_resolver = locator_resolver(resumed_locators);
  ExactDirectMorseChunkRunContext resumed_context{
      resumed_scenario.index,
      resumed_scenario.cloud,
      resumed_scenario.facade,
      resumed_scenario.event_journal,
      resumed_seed_budget,
      resumed_scenario.seed_journal,
      resumed_industrial_config,
      resumed_plan_budget,
      resumed_plan,
      ExactDirectMorseChunkBatchLocatorResolverView{
          resumed_locator_resolver},
      resumed_authorities,
      run_limits,
      budget_tracker(),
      ExactDirectMorseChunkRunSessionUsage{0U, 1U, 0U, 0U}};
  const auto resumed_contract = resumed_context.run_contract(
      initial_checkpoint, initial_output_chain);
  check(
      resumed_context.application_contract_digest() ==
              first_contract.application_contract_digest &&
          resumed_contract == first_contract,
      "a fresh scientific context with different session occupancy reconstructs the identical durable run contract");

  {
    constexpr std::uint64_t limit = UINT64_C(1) << 40U;
    ExactDirectMorseChunkRunContext starved_context{
        resumed_scenario.index,
        resumed_scenario.cloud,
        resumed_scenario.facade,
        resumed_scenario.event_journal,
        resumed_seed_budget,
        resumed_scenario.seed_journal,
        resumed_industrial_config,
        resumed_plan_budget,
        resumed_plan,
        ExactDirectMorseChunkBatchLocatorResolverView{
            resumed_locator_resolver},
        resumed_authorities,
        run_limits,
        budget_tracker(limit),
        ExactDirectMorseChunkRunSessionUsage{
            0U, limit - 1U, 0U, 0U}};
    bool recovery_budget_rejected = false;
    try {
      static_cast<void>(starved_context.open_existing_store(
          directory,
          initial_checkpoint,
          initial_output_chain,
          linear_limits,
          first_anchor));
    } catch (const AtomicLinearRunRecoveryError& error) {
      recovery_budget_rejected =
          error.reason() ==
              AtomicLinearRunRecoveryFailureReason::
                  resource_gate_before_recertification_rejected &&
          error.sequence() == 0U;
    }
    const auto starved_audit = starved_context.audit();
    check(
        recovery_budget_rejected &&
            starved_audit.fresh_budget_evaluation_count == 1U &&
            starved_audit.rejected_budget_evaluation_count == 1U &&
            starved_audit.arbitrary_batch_replay_count == 0U,
        "recovery uses a fresh session budget and rejects before locator resolution even though the durable snapshot was accepted earlier");
  }

  AtomicLinearRunExternalAnchor completed_anchor;
  {
    AtomicLinearRunStore store = resumed_context.open_existing_store(
        directory,
        initial_checkpoint,
        initial_output_chain,
        linear_limits,
        first_anchor);
    check(
        store.status().recovered_transition_count == 1U &&
            store.status().recovery_recertification_count == 1U &&
            store.trusted_state().next_chunk_index == 1U &&
            store.trusted_state().next_batch_index == 1U,
        "fresh recovery recertifies only committed chunk zero and resumes exactly at chunk one");

    const auto second_published =
        resumed_context.publish_next_chunk(store);
    completed_anchor = second_published.current_anchor;
    check(
        second_published.decision ==
                AtomicLinearRunPublishDecision::durably_published &&
            second_published.trusted_state_advanced &&
            second_published.current_anchor.committed_transition_count ==
                2U &&
            second_published.current_anchor.next_chunk_index == 2U &&
            second_published.current_anchor.next_batch_index == 2U &&
            store.status().committed_transition_count == 2U &&
            !store.status().process_local_ticket_serialized &&
            store.status().global_gamma_cell_count == 0U &&
            store.status().higher_order_delaunay_cell_count == 0U,
        "recovery publishes the second batch only after fresh acceptance of its four resolved keys and twelve arm joins");
  }

  const ExactDirectMorseChunkRunAudit resumed_audit =
      resumed_context.audit();
  check(
      resumed_audit.source_plan_reconstruction_count == 1U &&
          resumed_audit.indexed_batch_cursor_count == 2U &&
          resumed_audit.arbitrary_batch_replay_count == 3U &&
          resumed_audit.publication_recertification_count == 1U &&
          resumed_audit.recovery_recertification_count == 1U &&
          resumed_audit.cleanup_recertification_count == 0U &&
          resumed_audit.rejected_recertification_count == 0U &&
          resumed_audit.maximum_chunk_resolved_key_count == 4U &&
          resumed_audit.maximum_chunk_arm_join_count == 12U &&
          resumed_audit.maximum_live_external_locator_count == 1U &&
          resumed_audit.fresh_budget_evaluation_count == 6U &&
          resumed_audit.rejected_budget_evaluation_count == 0U &&
          no_forbidden_context_ownership(resumed_audit),
      "the fresh context reconstructs once, replays once for recovery and twice for publication under fresh session budgets, never advances a live cursor, and owns no forbidden structure");

  bool projected_science_matches = true;
  std::vector<std::size_t> projected_chunk_indices;
  const auto consume_projection =
      [&](const ExactDirectMorseRecertifiedChunkProjection&
              projection) {
        const std::size_t expected_chunk_index =
            projected_chunk_indices.size();
        if (expected_chunk_index >=
                resumed_plan.source_industrial_plan.chunks.size() ||
            projection.source_chunk !=
                resumed_plan.source_industrial_plan
                    .chunks[expected_chunk_index] ||
            projection.batches.size() != 1U) {
          projected_science_matches = false;
          projected_chunk_indices.push_back(
              projection.source_chunk.chunk_index);
          return;
        }
        const auto& batch = projection.batches.front();
        const std::size_t expected_batch_index =
            projection.source_chunk.source_batch_begin_index;
        const auto& source_batch =
            resumed_scenario.event_journal
                .batches[expected_batch_index];
        const auto& authority =
            resumed_authorities[expected_batch_index];
        projected_science_matches =
            projected_science_matches &&
            projection.source_chunk.chunk_index ==
                expected_chunk_index &&
            batch.source_batch_index == expected_batch_index &&
            batch.source_chunk_index == expected_chunk_index &&
            batch.order == source_batch.order &&
            batch.closed_batch_squared_level ==
                source_batch.squared_level &&
            batch.traversal_order ==
                morsehgp3d::spatial::LbvhTraversalOrder::near_first &&
            batch.requested_closure_budget ==
                authority.closure_budget &&
            batch.locator_query_witness ==
                authority.locator_query_witness &&
            batch.strict_pre_batch_locator_stamp ==
                authority.strict_pre_batch_locator_stamp &&
            batch.source_family_begin_index <=
                batch.source_family_end_index &&
            batch.source_arm_seed_begin_index <=
                batch.source_arm_seed_end_index &&
            batch.closure_summary.complete_relative_positive &&
            batch.execution_counters.resolved_key_projection_count ==
                batch.resolved_keys.size() &&
            batch.execution_counters.arm_join_count ==
                batch.arm_joins.size() &&
            (expected_batch_index != 0U ||
             (batch.resolved_keys.empty() &&
              batch.arm_joins.empty())) &&
            (expected_batch_index != 1U ||
             (batch.resolved_keys.size() == 4U &&
              batch.arm_joins.size() == 12U));
        projected_chunk_indices.push_back(
            projection.source_chunk.chunk_index);
      };
  const auto before_projection = resumed_context.audit();
  {
    AtomicLinearRunStore projected =
        resumed_context.open_existing_store(
            directory,
            initial_checkpoint,
            initial_output_chain,
            linear_limits,
            completed_anchor,
            consume_projection);
    check(
        projected.status().recovered_transition_count == 2U &&
            projected.status().recovery_recertification_count == 2U,
        "typed recovery still performs one generic recertification per committed chunk");
  }
  const auto after_projection = resumed_context.audit();
  check(
      projected_science_matches &&
          projected_chunk_indices ==
              std::vector<std::size_t>{0U, 1U} &&
          after_projection.recovery_recertification_count -
                  before_projection.recovery_recertification_count ==
              2U &&
          after_projection.arbitrary_batch_replay_count -
                  before_projection.arbitrary_batch_replay_count ==
              2U,
      "the real two-chunk prefix hands off each fresh 14D batch exactly once in order without a second decode or scientific replay");

  std::vector<std::size_t> disposable_projection_prefix;
  bool projection_throw_observed = false;
  try {
    AtomicLinearRunStore rejected =
        resumed_context.open_existing_store(
            directory,
            initial_checkpoint,
            initial_output_chain,
            linear_limits,
            completed_anchor,
            [&disposable_projection_prefix](
                const ExactDirectMorseRecertifiedChunkProjection&
                    projection) {
              disposable_projection_prefix.push_back(
                  projection.source_chunk.chunk_index);
              if (projection.source_chunk.chunk_index == 1U) {
                throw std::runtime_error(
                    "scripted typed projection failure");
              }
            });
    static_cast<void>(rejected.status());
  } catch (const std::runtime_error& error) {
    projection_throw_observed =
        std::string_view{error.what()} ==
        "scripted typed projection failure";
  }
  const bool failed_projection_was_disposable =
      disposable_projection_prefix ==
      std::vector<std::size_t>{0U, 1U};
  disposable_projection_prefix.clear();
  {
    AtomicLinearRunStore rebuilt =
        resumed_context.open_existing_store(
            directory,
            initial_checkpoint,
            initial_output_chain,
            linear_limits,
            completed_anchor,
            [&disposable_projection_prefix](
                const ExactDirectMorseRecertifiedChunkProjection&
                    projection) {
              disposable_projection_prefix.push_back(
                  projection.source_chunk.chunk_index);
            });
    check(
        projection_throw_observed &&
            failed_projection_was_disposable &&
            disposable_projection_prefix ==
                std::vector<std::size_t>{0U, 1U} &&
            rebuilt.status().committed_transition_count == 2U,
        "a typed visitor failure aborts recovery and releases the store for one fresh linear derived rebuild");
  }

  const auto resident_forest =
      build_exact_direct_morse_forest_journal(
          resumed_scenario.index,
          resumed_scenario.cloud,
          resumed_scenario.facade,
          resumed_scenario.event_journal,
          resumed_seed_budget,
          resumed_scenario.seed_journal,
          forest_budget(),
          forest_config());
  ExactDirectMorseForestReducer recovered_reducer{
      resumed_scenario.cloud,
      resumed_scenario.facade,
      resumed_scenario.event_journal,
      resumed_seed_budget,
      resumed_scenario.seed_journal,
      forest_budget(),
      forest_config()};
  bool every_recovered_batch_folded = true;
  {
    AtomicLinearRunStore recovered =
        resumed_context.open_existing_store(
            directory,
            initial_checkpoint,
            initial_output_chain,
            linear_limits,
            completed_anchor,
            [&recovered_reducer, &every_recovered_batch_folded](
                const ExactDirectMorseRecertifiedChunkProjection&
                    projection) {
              for (const auto& batch : projection.batches) {
                const auto borrowed =
                    project_exact_direct_morse_recertified_forest_reducer_batch(
                        batch);
                every_recovered_batch_folded =
                    recovered_reducer.fold(borrowed)
                        .certified_committed_batch() &&
                    every_recovered_batch_folded;
              }
            });
    every_recovered_batch_folded =
        recovered.status().recovered_transition_count == 2U &&
        every_recovered_batch_folded;
  }
  std::optional<ExactDirectMorseForestJournalResult>
      recovered_forest;
  if (every_recovered_batch_folded && recovered_reducer.complete()) {
    recovered_forest.emplace(recovered_reducer.finish());
  }
  check(
      recovered_forest.has_value() &&
          *recovered_forest == resident_forest,
      "the authoritative two-chunk prefix rebuilds the exact resident forest through one zero-copy fold per freshly recertified 14D batch");
}

void test_two_batch_payload_cap_is_preflighted_before_retention() {
  Scenario scenario = regular_tetrahedron_order_one_scenario();
  const auto seed_budget = source_seed_budget();
  const auto industrial_config = plan_config(2U);
  const auto batch_plan_budget = plan_budget();
  const auto observed_plan = build_plan(
      scenario,
      seed_budget,
      industrial_config,
      batch_plan_budget);
  LocatorPrefixes locators = locator_prefixes();
  const auto authorities = batch_authorities(locators);
  auto resolver = locator_resolver(locators);
  check(
      observed_plan.source_industrial_plan.chunks.size() == 1U &&
          observed_plan.source_industrial_plan.chunks[0U]
                  .source_batch_begin_index == 0U &&
          observed_plan.source_industrial_plan.chunks[0U]
                  .source_batch_end_index == 2U,
      "a two-batch cap keeps the empty and populated batches in one complete 14A chunk");
  if (observed_plan.source_industrial_plan.chunks.size() != 1U) {
    return;
  }

  AtomicLinearRunTrustedState trusted_state;
  trusted_state.checkpoint_digest =
      digest("wire-cap-initial-checkpoint");
  trusted_state.output_chain_digest =
      digest("wire-cap-initial-output-chain");
  ExactDirectMorseChunkRunContext measuring_context{
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget,
      scenario.seed_journal,
      industrial_config,
      batch_plan_budget,
      observed_plan,
      ExactDirectMorseChunkBatchLocatorResolverView{resolver},
      authorities,
      chunk_run_limits(2U),
      budget_tracker()};
  const auto measured =
      measuring_context.prepare_chunk(trusted_state, 0U);
  const std::size_t exact_payload_byte_count = measured.payload.size();
  check(
      exact_payload_byte_count != 0U &&
          measuring_context.audit().maximum_retained_batch_count == 2U,
      "the large-cap reference forms one canonical payload containing both batch entries");
  if (exact_payload_byte_count == 0U) {
    return;
  }

  ExactDirectMorseChunkRunContext exact_context{
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget,
      scenario.seed_journal,
      industrial_config,
      batch_plan_budget,
      observed_plan,
      ExactDirectMorseChunkBatchLocatorResolverView{resolver},
      authorities,
      chunk_run_limits(2U, exact_payload_byte_count),
      budget_tracker()};
  const auto exact = exact_context.prepare_chunk(trusted_state, 0U);
  check(
      exact.payload.size() == exact_payload_byte_count &&
          exact_context.audit().payload_pre_retention_refusal_count ==
              0U,
      "the exact cumulative payload cap is accepted");

  ExactDirectMorseChunkRunContext short_context{
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget,
      scenario.seed_journal,
      industrial_config,
      batch_plan_budget,
      observed_plan,
      ExactDirectMorseChunkBatchLocatorResolverView{resolver},
      authorities,
      chunk_run_limits(2U, exact_payload_byte_count - 1U),
      budget_tracker()};
  bool one_short_rejected = false;
  try {
    static_cast<void>(
        short_context.prepare_chunk(trusted_state, 0U));
  } catch (const std::length_error&) {
    one_short_rejected = true;
  }
  const auto short_audit = short_context.audit();
  check(
      one_short_rejected &&
          short_audit.arbitrary_batch_replay_count == 2U &&
          short_audit.maximum_retained_batch_count == 1U &&
          short_audit.payload_pre_retention_refusal_count == 1U,
      "a payload cap one byte short is refused while only the first batch is retained, before the second science segment joins the chunk");
}

void test_budget_policy_digest_and_cumulative_output() {
  TemporaryWorkspace workspace;
  Scenario scenario = regular_tetrahedron_order_one_scenario();
  const auto seed_budget = source_seed_budget();
  const auto industrial_config = plan_config();
  const auto batch_plan_budget = plan_budget();
  const auto observed_plan = build_plan(
      scenario,
      seed_budget,
      industrial_config,
      batch_plan_budget);
  LocatorPrefixes locators = locator_prefixes();
  const auto authorities = batch_authorities(locators);
  auto resolver = locator_resolver(locators);
  constexpr std::uint64_t generous = UINT64_C(1) << 40U;
  constexpr std::uint64_t payload = 64U * 1024U;
  constexpr std::uint64_t transition =
      atomic_linear_run_transition_fixed_wire_byte_count + payload;
  constexpr std::uint64_t head =
      atomic_linear_run_head_wire_byte_count;
  constexpr std::uint64_t first_output_peak =
      transition + 2U * head;
  const ExactDirectMorseEffectiveBudgetPolicy exact_policy{
      generous,
      generous,
      generous,
      first_output_peak,
      generous};
  ExactDirectMorseChunkRunContext exact_context{
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget,
      scenario.seed_journal,
      industrial_config,
      batch_plan_budget,
      observed_plan,
      ExactDirectMorseChunkBatchLocatorResolverView{resolver},
      authorities,
      chunk_run_limits(),
      budget_tracker_with_policy(exact_policy)};
  AtomicLinearRunTrustedState first_state;
  first_state.checkpoint_digest =
      digest("cumulative-output-initial-checkpoint");
  first_state.output_chain_digest =
      digest("cumulative-output-initial-chain");
  const auto first =
      exact_context.prepare_chunk(first_state, 0U);

  AtomicLinearRunTrustedState second_state;
  second_state.next_sequence = 1U;
  second_state.next_chunk_index = 1U;
  second_state.next_batch_index = 1U;
  second_state.checkpoint_digest =
      first.successor_checkpoint_digest;
  second_state.output_chain_digest =
      digest("cumulative-output-second-chain");
  bool second_rejected = false;
  try {
    static_cast<void>(
        exact_context.prepare_chunk(second_state, 1U));
  } catch (const std::length_error&) {
    second_rejected = true;
  }
  const auto exact_audit = exact_context.audit();
  check(
      !first.payload.empty() &&
          second_rejected &&
          exact_audit.arbitrary_batch_replay_count == 1U &&
          exact_audit.rejected_budget_evaluation_count == 1U,
      "the exact output budget admits sequence zero and charges the conservative durable prefix before refusing sequence one without a locator call");

  ExactDirectMorseEffectiveBudgetPolicy different_policy =
      exact_policy;
  ++different_policy.output_limit_bytes;
  ExactDirectMorseChunkRunContext different_policy_context{
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget,
      scenario.seed_journal,
      industrial_config,
      batch_plan_budget,
      observed_plan,
      ExactDirectMorseChunkBatchLocatorResolverView{resolver},
      authorities,
      chunk_run_limits(),
      budget_tracker_with_policy(different_policy)};
  check(
      different_policy_context.application_contract_digest() !=
          exact_context.application_contract_digest(),
      "changing only one effective budget limit changes the application and run contract authority");

  ExactDirectMorseEffectiveBudgetPolicy short_head_policy{
      generous,
      generous,
      generous,
      atomic_linear_run_head_wire_byte_count - 1U,
      generous};
  ExactDirectMorseChunkRunContext short_head_context{
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget,
      scenario.seed_journal,
      industrial_config,
      batch_plan_budget,
      observed_plan,
      ExactDirectMorseChunkBatchLocatorResolverView{resolver},
      authorities,
      chunk_run_limits(),
      budget_tracker_with_policy(short_head_policy)};
  const std::filesystem::path short_head_directory =
      workspace.make_directory();
  bool initial_head_rejected = false;
  try {
    static_cast<void>(short_head_context.create_new_store(
        short_head_directory,
        digest("short-head-initial-checkpoint"),
        digest("short-head-initial-chain"),
        store_limits()));
  } catch (const std::length_error&) {
    initial_head_rejected = true;
  }
  const auto short_head_audit = short_head_context.audit();
  check(
      initial_head_rejected &&
          std::filesystem::is_empty(short_head_directory) &&
          short_head_audit.fresh_budget_evaluation_count == 1U &&
          short_head_audit.rejected_budget_evaluation_count == 1U,
      "the initial HEAD output reserve is rejected before the dedicated directory is mutated");
}

void test_resource_gate_keeps_recertification_pure_and_bounds_time() {
  TemporaryWorkspace workspace;
  const std::filesystem::path directory = workspace.make_directory();
  Scenario scenario = regular_tetrahedron_order_one_scenario();
  const auto seed_budget = source_seed_budget();
  const auto industrial_config = plan_config();
  const auto batch_plan_budget = plan_budget();
  const auto observed_plan = build_plan(
      scenario,
      seed_budget,
      industrial_config,
      batch_plan_budget);
  LocatorPrefixes locators = locator_prefixes();
  const auto authorities = batch_authorities(locators);
  auto resolver = locator_resolver(locators);
  auto times = std::make_shared<std::vector<std::uint64_t>>(
      std::initializer_list<std::uint64_t>{
          0U, 1U, 2U, 3U, 4U, UINT64_C(3'000'005)});
  auto time_index = std::make_shared<std::size_t>(0U);
  ExactDirectMorseBudgetNanosecondClock clock =
      [times, time_index] {
        if (*time_index >= times->size()) {
          return times->back();
        }
        const std::uint64_t value = (*times)[*time_index];
        ++*time_index;
        return value;
      };
  ExactDirectMorseChunkRunContext context{
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget,
      scenario.seed_journal,
      industrial_config,
      batch_plan_budget,
      observed_plan,
      ExactDirectMorseChunkBatchLocatorResolverView{resolver},
      authorities,
      chunk_run_limits(),
      budget_tracker(
          UINT64_C(1) << 40U, 0U, std::move(clock))};
  AtomicLinearRunStore store = context.create_new_store(
      directory,
      digest("time-gate-initial-checkpoint"),
      digest("time-gate-initial-chain"),
      store_limits());
  const AtomicLinearRunTrustedState source_state =
      store.trusted_state();
  const auto proposal =
      context.prepare_chunk(source_state, 0U);

  AtomicLinearRunTransition transition_value;
  transition_value.sequence = source_state.next_sequence;
  transition_value.chunk_index = proposal.chunk_index;
  transition_value.batch_begin_index = proposal.batch_begin_index;
  transition_value.batch_end_index = proposal.batch_end_index;
  transition_value.source_checkpoint_digest =
      source_state.checkpoint_digest;
  transition_value.successor_checkpoint_digest =
      proposal.successor_checkpoint_digest;
  transition_value.budget_snapshot_digest =
      proposal.budget_snapshot_digest;
  transition_value.payload = proposal.payload;
  const auto first_replay = context.recertify_for_validation(
      transition_value,
      AtomicLinearRunRecertificationPhase::publication);
  const auto second_replay = context.recertify_for_validation(
      transition_value,
      AtomicLinearRunRecertificationPhase::publication);
  check(
      first_replay.accepted() &&
          second_replay == first_replay &&
          context.audit().fresh_budget_evaluation_count == 3U,
      "repeating the scientific recertifier has an identical result and does not advance the session clock");

  const auto result = store.publish_next(proposal);
  const auto audit = context.audit();
  check(
      result.decision ==
              AtomicLinearRunPublishDecision::resource_gate_rejected &&
          result.recertification.accepted() &&
          !result.trusted_state_advanced &&
          store.status().resource_gate_evaluation_count == 2U &&
          store.status().resource_gate_rejection_count == 1U &&
          store.status().committed_transition_count == 0U &&
          !std::filesystem::exists(
              directory / "linear-run-00000000000000000000.run") &&
          !std::filesystem::exists(
              directory /
              ".linear-run-00000000000000000000.tmp") &&
          !std::filesystem::exists(directory / ".HEAD.tmp") &&
          audit.fresh_budget_evaluation_count == 5U &&
          audit.rejected_budget_evaluation_count == 1U,
      "the separate stateful gate rejects an observed time delta after recertification and reversible writes, removes their files, and stops before HEAD replacement");
}

void test_public_payload_mutations_fail_closed() {
  TemporaryWorkspace workspace;
  const std::filesystem::path directory = workspace.make_directory();
  Scenario scenario = regular_tetrahedron_order_one_scenario();
  const auto seed_budget = source_seed_budget();
  const auto industrial_config = plan_config();
  const auto batch_plan_budget = plan_budget();
  const auto observed_plan = build_plan(
      scenario,
      seed_budget,
      industrial_config,
      batch_plan_budget);
  LocatorPrefixes locators = locator_prefixes();
  const auto authorities = batch_authorities(locators);
  auto resolver = locator_resolver(locators);
  ExactDirectMorseChunkRunContext context{
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget,
      scenario.seed_journal,
      industrial_config,
      batch_plan_budget,
      observed_plan,
      ExactDirectMorseChunkBatchLocatorResolverView{resolver},
      authorities,
      chunk_run_limits(),
      budget_tracker()};
  AtomicLinearRunStore store = context.create_new_store(
      directory,
      digest("mutation-initial-checkpoint"),
      digest("mutation-initial-output-chain"),
      store_limits());
  const auto canonical = context.prepare_chunk(
      store.trusted_state(), 0U);
  check(
      !canonical.payload.empty(),
      "the public proposal exposes a bounded canonical payload for hostile-input checks");
  if (canonical.payload.empty()) {
    return;
  }

  auto wrong_stamp_authorities = authorities;
  ++wrong_stamp_authorities[0U]
        .strict_pre_batch_locator_stamp.committed_batch_count;
  ExactDirectMorseChunkRunContext wrong_stamp_context{
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget,
      scenario.seed_journal,
      industrial_config,
      batch_plan_budget,
      observed_plan,
      ExactDirectMorseChunkBatchLocatorResolverView{resolver},
      wrong_stamp_authorities,
      chunk_run_limits(),
      budget_tracker()};
  bool wrong_stamp_rejected = false;
  try {
    static_cast<void>(wrong_stamp_context.prepare_chunk(
        store.trusted_state(), 0U));
  } catch (const std::invalid_argument&) {
    wrong_stamp_rejected = true;
  }
  check(
      wrong_stamp_rejected &&
          wrong_stamp_context.audit().arbitrary_batch_replay_count == 0U,
      "a substituted post-batch locator stamp is rejected before any scientific batch replay");

  auto mutated = canonical;
  mutated.payload[mutated.payload.size() / 2U] ^= UINT8_C(0x01);
  const auto mutated_result =
      store.publish_next(std::move(mutated));
  auto trailing = canonical;
  trailing.payload.push_back(UINT8_C(0x00));
  const auto trailing_result =
      store.publish_next(std::move(trailing));
  const auto audit = context.audit();
  check(
      mutated_result.decision ==
              AtomicLinearRunPublishDecision::recertification_rejected &&
          trailing_result.decision ==
              AtomicLinearRunPublishDecision::recertification_rejected &&
          !mutated_result.trusted_state_advanced &&
          !trailing_result.trusted_state_advanced &&
          store.status().committed_transition_count == 0U &&
          store.trusted_state().next_chunk_index == 0U &&
          store.trusted_state().next_batch_index == 0U &&
          audit.rejected_recertification_count == 2U &&
          no_forbidden_context_ownership(audit),
      "one payload mutation and one trailing byte are rejected through the public recertifier without durable cursor movement");
}

void test_durable_live_commit_reopens_at_certified_prefix() {
  TemporaryWorkspace workspace;
  const std::filesystem::path directory = workspace.make_directory();
  const CanonicalId initial_checkpoint =
      digest("durable-live-initial-checkpoint");
  const CanonicalId initial_output_chain =
      digest("durable-live-initial-output-chain");
  Scenario scenario = regular_tetrahedron_order_one_scenario();
  const auto seed_budget = source_seed_budget();
  const auto industrial_config = plan_config();
  const auto batch_plan_budget = plan_budget();
  const auto observed_plan = build_plan(
      scenario,
      seed_budget,
      industrial_config,
      batch_plan_budget);
  LocatorPrefixes locators = locator_prefixes();
  const auto authorities = batch_authorities(locators);
  auto resolver = locator_resolver(locators);
  ExactDirectMorseChunkRunContext context{
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget,
      scenario.seed_journal,
      industrial_config,
      batch_plan_budget,
      observed_plan,
      ExactDirectMorseChunkBatchLocatorResolverView{resolver},
      authorities,
      chunk_run_limits(),
      budget_tracker()};
  check(
      observed_plan.complete_architecture_plan() &&
          scenario.event_journal.batches.size() == 2U &&
          observed_plan.source_industrial_plan.chunks.size() == 2U,
      "the durable/live fixture exposes exactly two one-batch chunks");
  if (!observed_plan.complete_architecture_plan() ||
      scenario.event_journal.batches.size() != 2U ||
      observed_plan.source_industrial_plan.chunks.size() != 2U) {
    return;
  }

  const auto resident_forest =
      build_exact_direct_morse_forest_journal(
          scenario.index,
          scenario.cloud,
          scenario.facade,
          scenario.event_journal,
          seed_budget,
          scenario.seed_journal,
          forest_budget(),
          forest_config());
  AtomicLinearRunExternalAnchor first_anchor;
  {
    AtomicLinearRunStore store = context.create_new_store(
        directory,
        initial_checkpoint,
        initial_output_chain,
        store_limits());
    ExactDirectMorseForestReducer reducer{
        scenario.cloud,
        scenario.facade,
        scenario.event_journal,
        seed_budget,
        scenario.seed_journal,
        forest_budget(),
        forest_config()};
    ExactDirectSparseFacetDescentAnchoredBatchExecutor executor{
        scenario.index,
        scenario.cloud,
        scenario.facade,
        scenario.event_journal,
        seed_budget,
        scenario.seed_journal,
        industrial_config,
        batch_plan_budget,
        observed_plan,
        reducer.strict_locator()};
    const auto transcript = empty_proposal_transcript(
        0U,
        scenario.event_journal.batches[0U].squared_level,
        reducer.strict_locator());
    auto ticket =
        executor.prepare_next_sealed_with_top_k_proposal_transcript(
            authorities[0U].locator_query_witness,
            authorities[0U].execution_budget,
            authorities[0U].closure_budget,
            transcript);
    ExactDirectMorseDurableLiveCommitCoordinator coordinator{
        context, store, executor, reducer};
    const auto first =
        coordinator.commit_next_single_batch_chunk(ticket);
    check(
        first.certified_complete_commit() &&
            !coordinator.poisoned() &&
            store.trusted_state().next_chunk_index == 1U &&
            store.trusted_state().next_batch_index == 1U &&
            executor.next_source_batch_index() == 1U &&
            reducer.next_source_batch_index() == 1U &&
            reducer.strict_locator().snapshot_stamp() ==
                locators.before_batch_one.snapshot_stamp(),
        "one 15E transaction aligns durable HEAD, the 14H cursor, and the live reducer after committing batch zero");
    if (!first.certified_complete_commit()) {
      return;
    }
    first_anchor = first.publication.current_anchor;
  }

  ExactDirectMorseForestReducer resumed_reducer{
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget,
      scenario.seed_journal,
      forest_budget(),
      forest_config()};
  bool recovered_prefix_folded = true;
  AtomicLinearRunStore resumed_store = context.open_existing_store(
      directory,
      initial_checkpoint,
      initial_output_chain,
      store_limits(),
      first_anchor,
      [&resumed_reducer, &recovered_prefix_folded](
          const ExactDirectMorseRecertifiedChunkProjection&
              projection) {
        for (const auto& batch : projection.batches) {
          recovered_prefix_folded =
              resumed_reducer
                  .fold(
                      project_exact_direct_morse_recertified_forest_reducer_batch(
                          batch))
                  .certified_committed_batch() &&
              recovered_prefix_folded;
        }
      });
  check(
      recovered_prefix_folded &&
          resumed_store.trusted_state().next_chunk_index == 1U &&
          resumed_store.trusted_state().next_batch_index == 1U &&
          resumed_reducer.next_source_batch_index() == 1U,
      "reopen recertifies HEAD batch zero and rebuilds the reducer before any live continuation");
  if (!recovered_prefix_folded ||
      resumed_reducer.next_source_batch_index() != 1U) {
    return;
  }

  ExactDirectSparseFacetDescentAnchoredBatchExecutor resumed_executor{
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget,
      scenario.seed_journal,
      industrial_config,
      batch_plan_budget,
      observed_plan,
      resumed_reducer.strict_locator(),
      ExactDirectSparseFacetDescentCertifiedPrefixResume{1U}};
  check(
      resumed_executor.next_source_batch_index() == 1U &&
          resumed_executor.next_source_chunk_index() == 1U &&
          resumed_executor.audit()
                  .session_resumed_from_certified_prefix &&
          resumed_executor.audit().resumed_source_batch_count == 1U,
      "the fresh 14H session derives its full cursor from the certified one-batch prefix");
  const auto resumed_transcript = empty_proposal_transcript(
      1U,
      scenario.event_journal.batches[1U].squared_level,
      resumed_reducer.strict_locator());
  auto resumed_ticket =
      resumed_executor
          .prepare_next_sealed_with_top_k_proposal_transcript(
              authorities[1U].locator_query_witness,
              authorities[1U].execution_budget,
              authorities[1U].closure_budget,
              resumed_transcript);
  ExactDirectMorseDurableLiveCommitCoordinator resumed_coordinator{
      context,
      resumed_store,
      resumed_executor,
      resumed_reducer};
  const auto second =
      resumed_coordinator.commit_next_single_batch_chunk(
          resumed_ticket);
  check(
      second.certified_complete_commit() &&
          resumed_store.trusted_state().next_chunk_index == 2U &&
          resumed_store.trusted_state().next_batch_index == 2U &&
          resumed_executor.complete() &&
          resumed_reducer.complete(),
      "the fresh prefix-bound session commits batch one without replaying a process-local ticket");
  if (!second.certified_complete_commit() ||
      !resumed_reducer.complete()) {
    return;
  }
  const auto resumed_forest = resumed_reducer.finish();
  check(
      resumed_forest == resident_forest,
      "durable/live prefix recovery followed by one live commit reproduces the resident exact forest");
}

void test_durable_live_post_commit_io_failure_poisons() {
  TemporaryWorkspace workspace;
  const std::filesystem::path directory = workspace.make_directory();
  Scenario scenario = regular_tetrahedron_order_one_scenario();
  const auto seed_budget = source_seed_budget();
  const auto industrial_config = plan_config();
  const auto batch_plan_budget = plan_budget();
  const auto observed_plan = build_plan(
      scenario,
      seed_budget,
      industrial_config,
      batch_plan_budget);
  LocatorPrefixes locators = locator_prefixes();
  const auto authorities = batch_authorities(locators);
  auto resolver = locator_resolver(locators);
  ExactDirectMorseChunkRunContext context{
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget,
      scenario.seed_journal,
      industrial_config,
      batch_plan_budget,
      observed_plan,
      ExactDirectMorseChunkBatchLocatorResolverView{resolver},
      authorities,
      chunk_run_limits(),
      budget_tracker()};
  AtomicLinearRunStore store = context.create_new_store(
      directory,
      digest("durable-live-poison-initial-checkpoint"),
      digest("durable-live-poison-initial-output-chain"),
      store_limits());
  ExactDirectMorseForestReducer reducer{
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget,
      scenario.seed_journal,
      forest_budget(),
      forest_config()};
  ExactDirectSparseFacetDescentAnchoredBatchExecutor executor{
      scenario.index,
      scenario.cloud,
      scenario.facade,
      scenario.event_journal,
      seed_budget,
      scenario.seed_journal,
      industrial_config,
      batch_plan_budget,
      observed_plan,
      reducer.strict_locator()};
  const auto transcript = empty_proposal_transcript(
      0U,
      scenario.event_journal.batches[0U].squared_level,
      reducer.strict_locator());
  auto ticket =
      executor.prepare_next_sealed_with_top_k_proposal_transcript(
          authorities[0U].locator_query_witness,
          authorities[0U].execution_budget,
          authorities[0U].closure_budget,
          transcript);
  ExactDirectMorseDurableLiveCommitCoordinator coordinator{
      context, store, executor, reducer};
  UnlinkDurableLiveHeadState sabotage{
      (directory / ".HEAD.tmp").string(), false};
  const auto result =
      coordinator.commit_next_single_batch_chunk(
          ticket,
          AtomicLinearRunPublishOptions{
              unlink_durable_live_head_before_commit,
              &sabotage});
  check(
      sabotage.unlink_succeeded &&
          result.certified_reopen_required() &&
          result.publication.process_local_commit_succeeded &&
          result.publication.decision ==
              AtomicLinearRunPublishDecision::
                  indeterminate_io_failure_reopen_required &&
          coordinator.poisoned() &&
          store.status().failed_closed_reopen_required &&
          executor.next_source_batch_index() == 1U &&
          reducer.next_source_batch_index() == 1U &&
          store.trusted_state().next_batch_index == 0U,
      "an I/O failure after the live commit but before HEAD replacement poisons every live authority and requires reopen from durable batch zero");

  bool poisoned_call_rejected = false;
  try {
    static_cast<void>(
        coordinator.commit_next_single_batch_chunk(
            ticket));
  } catch (const std::logic_error&) {
    poisoned_call_rejected = true;
  }
  check(
      poisoned_call_rejected,
      "a poisoned 15E coordinator cannot be reused before reconstruction from HEAD");
}

}  // namespace

int main() {
  test_two_real_chunks_resume_with_fresh_context();
  test_two_batch_payload_cap_is_preflighted_before_retention();
  test_budget_policy_digest_and_cumulative_output();
  test_resource_gate_keeps_recertification_pure_and_bounds_time();
  test_public_payload_mutations_fail_closed();
  test_durable_live_commit_reopens_at_certified_prefix();
  test_durable_live_post_commit_io_failure_poisons();

  if (failures != 0) {
    std::cerr << failures
              << " direct Morse chunk-run test(s) failed\n";
    return 1;
  }
  return 0;
}
