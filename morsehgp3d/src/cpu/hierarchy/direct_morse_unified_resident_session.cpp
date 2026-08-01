#include "morsehgp3d/hierarchy/direct_morse_unified_resident_session.hpp"

#include "morsehgp3d/hierarchy/facet_miniball.hpp"

#include <algorithm>
#include <atomic>
#include <exception>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

struct ResidentState {
  std::vector<ExactDirectMorseUnifiedResidentComponentState> components;
  std::vector<ExactDirectMorseUnifiedResidentRootCoverage> roots;
  std::vector<ExactDirectMorseUnifiedResidentGroupRecord> group_records;
  ExactFrozenIncidencePriorRootId next_root_id{1U};
  std::uint64_t next_replay_token{1U};
  std::size_t resident_root_point_reference_count{};
  std::size_t group_child_reference_count{};
  std::size_t group_coverage_delta_point_reference_count{};
};

static_assert(noexcept(std::declval<ResidentState&>() =
                       std::declval<ResidentState&&>()));

struct SourceReferences {
  const spatial::MortonLbvhIndex* index{};
  const spatial::CanonicalPointCloud* cloud{};
  const ExactDirectSupportTerminalFacade* facade{};
  const ExactDirectMorseEventJournalResult* journal{};
  const ExactDirectSaddleArmSeedBudget* arm_budget{};
  const ExactDirectSaddleArmSeedJournalResult* arm_journal{};
  const ExactDirectClosedSaddleIncidenceBudget* incidence_budget{};
  const ExactDirectClosedSaddleIncidenceJournalResult* incidence_journal{};
  const ExactDirectSparseSuccessiveIncidenceStarJournalBudget* star_budget{};
  spatial::LbvhTraversalOrder traversal_order{
      spatial::LbvhTraversalOrder::near_first};
  const ExactDirectSparseSuccessiveIncidenceStarJournalResult* star{};
  const ExactDirectSparseUnifiedLevelPlanBudget* plan_budget{};
};

struct SessionSeal {
  std::uint64_t authority_id{};
  std::uint64_t locator_instance_id{};
};

std::atomic<std::uint64_t> next_locator_instance_id{1U};

[[nodiscard]] std::uint64_t allocate_locator_instance_id() noexcept {
  std::uint64_t candidate =
      next_locator_instance_id.load(std::memory_order_relaxed);
  while (candidate != 0U &&
         candidate != std::numeric_limits<std::uint64_t>::max()) {
    if (next_locator_instance_id.compare_exchange_weak(
            candidate,
            candidate + 1U,
            std::memory_order_relaxed,
            std::memory_order_relaxed)) {
      return candidate;
    }
  }
  return 0U;
}

[[nodiscard]] bool add_overflow(
    std::size_t left,
    std::size_t right,
    std::size_t& sum) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return true;
  }
  sum = left + right;
  return false;
}

[[nodiscard]] std::span<const PointId> key_points(
    const ExactDirectSparseFacetKey& key) noexcept {
  return {key.point_ids.data(), key.point_count};
}

[[nodiscard]] std::size_t find_component(
    const ResidentState& state,
    std::size_t handle) {
  if (handle >= state.components.size()) {
    throw std::logic_error("resident component handle out of range");
  }
  std::size_t hops = 0U;
  while (state.components[handle].parent_handle != handle) {
    handle = state.components[handle].parent_handle;
    if (handle >= state.components.size() ||
        ++hops > state.components.size()) {
      throw std::logic_error("resident component parent cycle");
    }
  }
  return handle;
}

[[nodiscard]] ExactDirectMorseUnifiedResidentRootCoverage* find_root(
    ResidentState& state,
    ExactFrozenIncidencePriorRootId root_id) noexcept {
  const auto found = std::find_if(
      state.roots.begin(), state.roots.end(), [&](const auto& root) {
        return root.root_id == root_id;
      });
  return found == state.roots.end() ? nullptr : &*found;
}

void canonicalize_points(std::vector<PointId>& points) {
  std::sort(points.begin(), points.end());
  points.erase(std::unique(points.begin(), points.end()), points.end());
}

[[nodiscard]] bool checked_next_witness(
    ResidentState& state,
    std::uint64_t authority_id,
    ExactDirectSparseFacetWitness& witness) noexcept {
  if (state.next_replay_token == 0U || authority_id == 0U) {
    return false;
  }
  witness = {authority_id, state.next_replay_token};
  if (state.next_replay_token ==
      std::numeric_limits<std::uint64_t>::max()) {
    state.next_replay_token = 0U;
  } else {
    ++state.next_replay_token;
  }
  return true;
}

[[nodiscard]] bool budget_accepts_authority(
    const ExactDirectMorseUnifiedResidentSessionBudget& budget,
    const ExactDirectMorseUnifiedResidentAuthorityBundle& bundle) noexcept {
  return bundle.facet_resolutions.size() <=
             budget.maximum_facet_resolution_count &&
         bundle.prior_root_coverages.size() <=
             budget.maximum_prior_root_coverage_count &&
         bundle.prior_root_coverage_point_references.size() <=
             budget.maximum_prior_root_coverage_point_reference_count &&
         bundle.latent_carrier_coverages.size() <=
             budget.maximum_latent_carrier_coverage_count &&
         bundle.latent_carrier_coverage_point_references.size() <=
             budget.maximum_latent_carrier_coverage_point_reference_count &&
         bundle.counters.fresh_facet_miniball_build_count <=
             budget.maximum_fresh_facet_miniball_count &&
         bundle.counters.fresh_facet_miniball_support_enumeration_count <=
             budget.maximum_fresh_facet_miniball_support_enumeration_count;
}

[[nodiscard]] bool frozen_verification_complete(
    const ExactDirectFrozenUnifiedIncidenceBatchVerification& verification)
    noexcept {
  return verification.requested_budget_certified &&
         verification.source_plan_freshly_verified &&
         verification.expected_result_freshly_reconstructed &&
         verification.supplied_latent_carrier_coverage_freshly_replayed &&
         verification.quotient_freshly_streaming_verified &&
         verification.action_plan_freshly_streaming_verified &&
         verification.observed_recursively_equal &&
         verification.result_facts_and_scope_certified &&
         verification.no_forbidden_global_structure_or_mutation &&
         verification.fresh_replay_certified &&
         verification.result_certified;
}

[[nodiscard]] std::vector<std::size_t> touched_facets(
    const ExactDirectSparseUnifiedLevelPlanResult& plan,
    const ExactDirectSparseUnifiedLevelPlanBatch& batch) {
  std::vector<std::size_t> touched;
  touched.reserve(batch.coface_facet_reference_count);
  for (std::size_t local = 0U;
       local < batch.coface_facet_reference_count;
       ++local) {
    touched.push_back(
        plan.coface_facet_references
            [batch.coface_facet_reference_offset + local]
                .facet_token_index);
  }
  std::sort(touched.begin(), touched.end());
  touched.erase(std::unique(touched.begin(), touched.end()), touched.end());
  return touched;
}

[[nodiscard]] std::vector<std::size_t> direct_birth_facets(
    const ExactDirectSparseUnifiedLevelPlanResult& plan,
    const ExactDirectSparseUnifiedLevelPlanBatch& batch) {
  std::vector<std::size_t> births;
  births.reserve(batch.direct_reference_count);
  for (std::size_t local = 0U; local < batch.direct_reference_count;
       ++local) {
    const auto& reference = plan.direct_references
        [batch.direct_reference_offset + local];
    if (reference.role == ExactDirectMorseH0Role::birth) {
      if (!reference.direct_birth_facet_token_index.has_value()) {
        throw std::logic_error("resident direct birth has no facet token");
      }
      births.push_back(*reference.direct_birth_facet_token_index);
    }
  }
  std::sort(births.begin(), births.end());
  if (std::adjacent_find(births.begin(), births.end()) != births.end()) {
    throw std::logic_error("resident batch repeats a direct birth token");
  }
  return births;
}

[[nodiscard]] bool update_resident_storage_counts(
    ResidentState& state,
    const ExactDirectMorseUnifiedResidentSessionBudget& budget) noexcept {
  std::size_t root_points = 0U;
  for (const auto& root : state.roots) {
    if (add_overflow(root_points, root.point_ids.size(), root_points)) {
      return false;
    }
  }
  std::size_t child_count = 0U;
  std::size_t delta_point_count = 0U;
  for (const auto& record : state.group_records) {
    if (add_overflow(
            child_count, record.child_root_ids.size(), child_count)) {
      return false;
    }
    if (add_overflow(
            delta_point_count,
            record.coverage_delta_points.size(),
            delta_point_count)) {
      return false;
    }
  }
  if (state.roots.size() > budget.maximum_resident_root_count ||
      root_points > budget.maximum_resident_root_point_reference_count ||
      state.group_records.size() > budget.maximum_group_record_count ||
      child_count > budget.maximum_group_child_reference_count ||
      delta_point_count >
          budget.maximum_group_coverage_delta_point_reference_count) {
    return false;
  }
  state.resident_root_point_reference_count = root_points;
  state.group_child_reference_count = child_count;
  state.group_coverage_delta_point_reference_count = delta_point_count;
  return true;
}

}  // namespace

struct ExactDirectMorseUnifiedResidentSession::Impl {
  SourceReferences source{};
  ExactDirectMorseUnifiedResidentSessionBudget budget{};
  ExactDirectSparseUnifiedLevelPlanResult plan{};
  ExactDirectSparsePositiveFacetLocator locator{};
  ResidentState state{};
  std::shared_ptr<const SessionSeal> seal;
  std::uint64_t authority_id{};
  std::uint64_t locator_instance_id{};
  std::size_t cursor{};
  std::size_t epoch{};
  std::size_t source_plan_initial_verification_count{};
  std::size_t frozen_batch_source_replay_count{};
  bool initialized{false};
};

struct ExactDirectMorseUnifiedResidentPreparedBatch::Impl {
  ExactDirectMorseUnifiedResidentAuthorityBundle bundle{};
  std::shared_ptr<const SessionSeal> seal;
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp source_stamp{};
  std::size_t source_cursor{};
  std::size_t source_epoch{};
  std::vector<ExactDirectSparseFacetQuery> queries;
  std::vector<ExactDirectSparseComponentUnion> unions;
  std::vector<ExactDirectSparseFacetBinding> bindings;
  ResidentState prepared_state{};
  bool consumed{false};
};

bool ExactDirectMorseUnifiedResidentAuthorityBundle::
    certified_strict_pre_batch_bundle() const noexcept {
  return identity.schema_version ==
             direct_morse_unified_resident_session_schema_version &&
         identity.session_authority_id != 0U &&
         identity.locator_instance_id != 0U &&
         identity.epoch == identity.batch_cursor &&
         source_batch_index == identity.batch_cursor &&
         identity.locator_stamp.external_authority_id ==
             identity.session_authority_id &&
         identity.locator_stamp.committed_batch_count ==
             identity.batch_cursor &&
         identity.source_pair_canonical_cloud_digest !=
             contract::CanonicalId{} &&
         identity.source_higher_canonical_cloud_digest !=
             contract::CanonicalId{} &&
         identity.source_pair_semantic_digest !=
             contract::CanonicalId{} &&
         identity.source_higher_semantic_digest !=
             contract::CanonicalId{} &&
         frozen_batch.source_batch_index == source_batch_index &&
         frozen_batch.source_future_snapshot_index ==
             source_future_snapshot_index &&
         frozen_batch.squared_level == squared_level &&
         frozen_batch.order == order &&
         frozen_batch.counters.facet_resolution_count ==
             facet_resolutions.size() &&
         frozen_batch.counters.prior_root_coverage_count ==
             prior_root_coverages.size() &&
         frozen_batch.counters.prior_root_coverage_point_reference_count ==
             prior_root_coverage_point_references.size() &&
         frozen_batch.counters.latent_carrier_coverage_count ==
             latent_carrier_coverages.size() &&
         frozen_batch.counters
                 .latent_carrier_coverage_point_reference_count ==
             latent_carrier_coverage_point_references.size() &&
         frozen_batch.certified_frozen_unified_incidence_batch() &&
         frozen_verification_complete(frozen_verification) &&
         locator_snapshot_strictly_pre_batch &&
         every_unresolved_facet_has_fresh_exact_equal_miniball &&
         csr_authorities_share_identity_and_pre_batch_state &&
         frozen_batch_built_and_freshly_verified &&
         !global_facet_coface_or_gamma_catalog_materialized &&
         !supplied_star_global_completeness_claimed &&
         !public_status_claimed;
}

ExactDirectMorseUnifiedResidentPreparedBatch::
    ExactDirectMorseUnifiedResidentPreparedBatch() noexcept = default;
ExactDirectMorseUnifiedResidentPreparedBatch::
    ~ExactDirectMorseUnifiedResidentPreparedBatch() = default;
ExactDirectMorseUnifiedResidentPreparedBatch::
    ExactDirectMorseUnifiedResidentPreparedBatch(
        ExactDirectMorseUnifiedResidentPreparedBatch&&) noexcept = default;
ExactDirectMorseUnifiedResidentPreparedBatch&
ExactDirectMorseUnifiedResidentPreparedBatch::operator=(
    ExactDirectMorseUnifiedResidentPreparedBatch&&) noexcept = default;
ExactDirectMorseUnifiedResidentPreparedBatch::
    ExactDirectMorseUnifiedResidentPreparedBatch(
        std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

const ExactDirectMorseUnifiedResidentAuthorityBundle&
ExactDirectMorseUnifiedResidentPreparedBatch::authority_bundle()
    const noexcept {
  static const ExactDirectMorseUnifiedResidentAuthorityBundle empty{};
  return impl_ == nullptr ? empty : impl_->bundle;
}

bool ExactDirectMorseUnifiedResidentPreparedBatch::valid() const noexcept {
  return impl_ != nullptr && impl_->seal != nullptr && !impl_->consumed &&
         impl_->seal->authority_id ==
             impl_->bundle.identity.session_authority_id &&
         impl_->seal->locator_instance_id ==
             impl_->bundle.identity.locator_instance_id &&
         impl_->source_cursor == impl_->bundle.identity.batch_cursor &&
         impl_->source_epoch == impl_->bundle.identity.epoch &&
         impl_->source_stamp == impl_->bundle.identity.locator_stamp &&
         impl_->bundle.certified_strict_pre_batch_bundle();
}

bool ExactDirectMorseUnifiedResidentPreparedBatch::consumed() const noexcept {
  return impl_ == nullptr || impl_->consumed;
}

bool ExactDirectMorseUnifiedResidentPreparationResult::
    certified_prepared_batch() const noexcept {
  return ticket.has_value() && ticket->valid() &&
         no_scientific_state_mutated &&
         strict_pre_batch_bundle_certified &&
         decision == ExactDirectMorseUnifiedResidentPreparationDecision::
                         complete_certified_prepared_batch;
}

bool ExactDirectMorseUnifiedResidentCommitResult::certified_committed_batch()
    const noexcept {
  return locator_batch.certified_committed_batch() && ticket_consumed &&
         exactly_one_locator_apply_batch_called &&
         prepared_state_installed_after_locator_commit &&
         cursor_and_epoch_advanced_once &&
         !no_scientific_state_mutated_on_failure &&
         decision == ExactDirectMorseUnifiedResidentCommitDecision::
                         complete_certified_atomic_batch_commit;
}

ExactDirectMorseUnifiedResidentSession::
    ExactDirectMorseUnifiedResidentSession() noexcept = default;
ExactDirectMorseUnifiedResidentSession::
    ~ExactDirectMorseUnifiedResidentSession() = default;
ExactDirectMorseUnifiedResidentSession::
    ExactDirectMorseUnifiedResidentSession(
        ExactDirectMorseUnifiedResidentSession&&) noexcept = default;
ExactDirectMorseUnifiedResidentSession&
ExactDirectMorseUnifiedResidentSession::operator=(
    ExactDirectMorseUnifiedResidentSession&&) noexcept = default;
ExactDirectMorseUnifiedResidentSession::
    ExactDirectMorseUnifiedResidentSession(std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectMorseUnifiedResidentSession::certified_resident_session()
    const noexcept {
  return impl_ != nullptr && impl_->initialized && impl_->seal != nullptr &&
         impl_->authority_id != 0U &&
         impl_->locator_instance_id != 0U &&
         impl_->source_plan_initial_verification_count == 1U &&
         impl_->plan.certified_bounded_plan() &&
         impl_->locator.certified_positive_locator() &&
         impl_->state.components.size() == impl_->plan.facet_tokens.size() &&
         impl_->cursor <= impl_->plan.batches.size() &&
         impl_->epoch == impl_->cursor &&
         impl_->locator.snapshot_stamp().committed_batch_count ==
             impl_->cursor;
}

bool ExactDirectMorseUnifiedResidentSession::complete() const noexcept {
  return certified_resident_session() &&
         impl_->cursor == impl_->plan.batches.size();
}

std::size_t ExactDirectMorseUnifiedResidentSession::batch_cursor()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->cursor;
}

std::size_t ExactDirectMorseUnifiedResidentSession::epoch() const noexcept {
  return impl_ == nullptr ? 0U : impl_->epoch;
}

std::size_t ExactDirectMorseUnifiedResidentSession::
    source_plan_initial_verification_count() const noexcept {
  return impl_ == nullptr ? 0U :
                            impl_->source_plan_initial_verification_count;
}

std::size_t ExactDirectMorseUnifiedResidentSession::
    frozen_batch_source_replay_count() const noexcept {
  return impl_ == nullptr ? 0U : impl_->frozen_batch_source_replay_count;
}

const ExactDirectSparseUnifiedLevelPlanResult&
ExactDirectMorseUnifiedResidentSession::plan() const noexcept {
  static const ExactDirectSparseUnifiedLevelPlanResult empty{};
  return impl_ == nullptr ? empty : impl_->plan;
}

const ExactDirectSparsePositiveFacetLocator&
ExactDirectMorseUnifiedResidentSession::locator() const noexcept {
  static const ExactDirectSparsePositiveFacetLocator empty{};
  return impl_ == nullptr ? empty : impl_->locator;
}

const std::vector<ExactDirectMorseUnifiedResidentComponentState>&
ExactDirectMorseUnifiedResidentSession::component_states() const noexcept {
  static const std::vector<ExactDirectMorseUnifiedResidentComponentState>
      empty;
  return impl_ == nullptr ? empty : impl_->state.components;
}

const std::vector<ExactDirectMorseUnifiedResidentRootCoverage>&
ExactDirectMorseUnifiedResidentSession::root_coverages() const noexcept {
  static const std::vector<ExactDirectMorseUnifiedResidentRootCoverage> empty;
  return impl_ == nullptr ? empty : impl_->state.roots;
}

const std::vector<ExactDirectMorseUnifiedResidentGroupRecord>&
ExactDirectMorseUnifiedResidentSession::group_records() const noexcept {
  static const std::vector<ExactDirectMorseUnifiedResidentGroupRecord> empty;
  return impl_ == nullptr ? empty : impl_->state.group_records;
}

bool ExactDirectMorseUnifiedResidentInitializationResult::
    certified_initialized_session() const noexcept {
  return session.has_value() && session->certified_resident_session() &&
         source_plan_initial_verification_count == 1U &&
         source_plan_freshly_verified_once && source_plan_owned_by_session &&
         locator_and_component_state_initialized &&
         no_global_facet_coface_or_gamma_catalog_materialized &&
         !public_status_claimed &&
         decision == ExactDirectMorseUnifiedResidentInitializationDecision::
                         complete_certified_bounded_resident_session;
}

ExactDirectMorseUnifiedResidentPreparationResult
ExactDirectMorseUnifiedResidentSession::prepare_next() {
  ExactDirectMorseUnifiedResidentPreparationResult output;
  output.no_scientific_state_mutated = true;
  const auto reject = [&](ExactDirectMorseUnifiedResidentPreparationDecision
                              decision) {
    output.ticket.reset();
    output.strict_pre_batch_bundle_certified = false;
    output.decision = decision;
    return std::move(output);
  };
  if (!certified_resident_session()) {
    return reject(
        ExactDirectMorseUnifiedResidentPreparationDecision::
            no_session_not_initialized);
  }
  if (impl_->cursor >= impl_->plan.batches.size()) {
    return reject(
        ExactDirectMorseUnifiedResidentPreparationDecision::
            no_plan_exhausted);
  }

  try {
    auto prepared = std::make_unique<
        ExactDirectMorseUnifiedResidentPreparedBatch::Impl>();
    prepared->seal = impl_->seal;
    prepared->source_cursor = impl_->cursor;
    prepared->source_epoch = impl_->epoch;
    prepared->source_stamp = impl_->locator.snapshot_stamp();
    prepared->prepared_state = impl_->state;
    prepared->bundle.counters.resident_state_full_copy_count = 1U;

    const auto& batch = impl_->plan.batches[impl_->cursor];
    auto& bundle = prepared->bundle;
    bundle.identity = {
        direct_morse_unified_resident_session_schema_version,
        impl_->authority_id,
        impl_->locator_instance_id,
        impl_->epoch,
        impl_->cursor,
        impl_->plan.source_pair_canonical_cloud_digest,
        impl_->plan.source_higher_canonical_cloud_digest,
        impl_->plan.source_pair_semantic_digest,
        impl_->plan.source_higher_semantic_digest,
        prepared->source_stamp,
    };
    bundle.source_batch_index = batch.batch_index;
    bundle.source_future_snapshot_index = batch.future_snapshot_index;
    bundle.squared_level = batch.squared_level;
    bundle.order = batch.order;

    const std::vector<std::size_t> touched =
        touched_facets(impl_->plan, batch);
    if (touched.size() > impl_->budget.maximum_facet_resolution_count ||
        touched.size() > impl_->budget.locator.maximum_batch_query_count) {
      return reject(
          ExactDirectMorseUnifiedResidentPreparationDecision::
              no_authority_budget_exhausted);
    }
    bundle.facet_resolutions.reserve(touched.size());
    prepared->queries.reserve(touched.size());

    for (const std::size_t facet_token_index : touched) {
      if (facet_token_index >= impl_->plan.facet_tokens.size()) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                contradiction_locator_state_inconsistent);
      }
      const auto& key =
          impl_->plan.facet_tokens[facet_token_index].facet_key;
      ExactDirectSparseFacetWitness witness;
      if (!checked_next_witness(
              prepared->prepared_state, impl_->authority_id, witness)) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_prepared_state_rejected);
      }
      prepared->queries.push_back(
          {prepared->queries.size(), key, witness});
      const auto probe = impl_->locator.probe_positive_facet(
          key, witness, impl_->budget.probe);
      ++bundle.counters.locator_probe_count;
      if (probe.certified_positive_hit()) {
        ++bundle.counters.positive_locator_probe_count;
        if (!probe.component_handle_present ||
            probe.component_handle >=
                prepared->prepared_state.components.size()) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  contradiction_locator_state_inconsistent);
        }
        const std::size_t component_handle = find_component(
            prepared->prepared_state, probe.component_handle);
        const auto& component =
            prepared->prepared_state.components[component_handle];
        if (!component.active ||
            component_handle >
                std::numeric_limits<ExactFrozenIncidenceTokenId>::max()) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  contradiction_locator_state_inconsistent);
        }
        if (component.root_id.has_value()) {
          if (find_root(
                  prepared->prepared_state, *component.root_id) == nullptr) {
            return reject(
                ExactDirectMorseUnifiedResidentPreparationDecision::
                    contradiction_locator_state_inconsistent);
          }
          bundle.facet_resolutions.push_back(
              {facet_token_index,
               {ExactFrozenIncidenceTokenKind::rooted_carrier,
                static_cast<ExactFrozenIncidenceTokenId>(component_handle)},
               component.root_id});
          ++bundle.counters.rooted_resolution_count;
        } else {
          bundle.facet_resolutions.push_back(
              {facet_token_index,
               {ExactFrozenIncidenceTokenKind::latent_carrier,
                static_cast<ExactFrozenIncidenceTokenId>(component_handle)},
               std::nullopt});
          ++bundle.counters.latent_resolution_count;
        }
        continue;
      }
      if (!probe.certified_unresolved_miss()) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_authority_budget_exhausted);
      }
      ++bundle.counters.unresolved_locator_probe_count;
      if (bundle.counters.fresh_facet_miniball_build_count >=
          impl_->budget.maximum_fresh_facet_miniball_count) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_authority_budget_exhausted);
      }
      const auto miniball = build_exact_facet_miniball(
          *impl_->source.cloud, key_points(key));
      ++bundle.counters.fresh_facet_miniball_build_count;
      const auto miniball_verification = verify_exact_facet_miniball(
          *impl_->source.cloud, key_points(key), miniball);
      ++bundle.counters.fresh_facet_miniball_verification_count;
      std::size_t replayed_support_count = 0U;
      if (miniball.counters.enumerated_support_count >
          std::numeric_limits<std::size_t>::max() / 3U) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_authority_budget_exhausted);
      }
      replayed_support_count =
          3U * miniball.counters.enumerated_support_count;
      std::size_t support_count = 0U;
      if (add_overflow(
              bundle.counters
                  .fresh_facet_miniball_support_enumeration_count,
              replayed_support_count,
              support_count)) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_authority_budget_exhausted);
      }
      bundle.counters.fresh_facet_miniball_support_enumeration_count =
          support_count;
      if (miniball.status !=
              ExactFacetMiniballStatus::exact_facet_miniball_certified ||
          !miniball_verification.local_exact_facet_miniball_certified) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_facet_miniball_certification_failed);
      }
      if (miniball.squared_radius < batch.squared_level) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                contradiction_unresolved_facet_birth_below_active_level);
      }
      if (batch.squared_level < miniball.squared_radius) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                contradiction_unresolved_facet_birth_above_active_level);
      }
      if (facet_token_index >
          std::numeric_limits<ExactFrozenIncidenceTokenId>::max()) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_prepared_state_rejected);
      }
      bundle.facet_resolutions.push_back(
          {facet_token_index,
           {ExactFrozenIncidenceTokenKind::equal_facet,
            static_cast<ExactFrozenIncidenceTokenId>(facet_token_index)},
           std::nullopt});
      ++bundle.counters.equal_resolution_count;
    }

    std::vector<ExactFrozenIncidencePriorRootId> root_ids;
    std::vector<ExactFrozenIncidenceTokenId> latent_ids;
    for (const auto& resolution : bundle.facet_resolutions) {
      if (resolution.prior_root_id.has_value()) {
        root_ids.push_back(*resolution.prior_root_id);
      } else if (resolution.token.kind ==
                 ExactFrozenIncidenceTokenKind::latent_carrier) {
        latent_ids.push_back(resolution.token.token_id);
      }
    }
    std::sort(root_ids.begin(), root_ids.end());
    root_ids.erase(std::unique(root_ids.begin(), root_ids.end()),
                   root_ids.end());
    std::sort(latent_ids.begin(), latent_ids.end());
    latent_ids.erase(std::unique(latent_ids.begin(), latent_ids.end()),
                     latent_ids.end());
    if (root_ids.size() >
            impl_->budget.maximum_prior_root_coverage_count ||
        latent_ids.size() >
            impl_->budget.maximum_latent_carrier_coverage_count) {
      return reject(
          ExactDirectMorseUnifiedResidentPreparationDecision::
              no_authority_budget_exhausted);
    }

    std::size_t required_root_points = 0U;
    for (const auto root_id : root_ids) {
      const auto* root = find_root(prepared->prepared_state, root_id);
      if (root == nullptr ||
          add_overflow(
              required_root_points,
              root->point_ids.size(),
              required_root_points)) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                contradiction_locator_state_inconsistent);
      }
    }
    std::size_t required_latent_points = 0U;
    for (const auto token_id : latent_ids) {
      if (token_id >= prepared->prepared_state.components.size()) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                contradiction_locator_state_inconsistent);
      }
      const std::size_t component_handle = find_component(
          prepared->prepared_state,
          static_cast<std::size_t>(token_id));
      const auto& component =
          prepared->prepared_state.components[component_handle];
      if (!component.active || component.root_id.has_value() ||
          add_overflow(
              required_latent_points,
              component.latent_point_coverage.size(),
              required_latent_points)) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                contradiction_locator_state_inconsistent);
      }
    }
    if (required_root_points >
            impl_->budget
                .maximum_prior_root_coverage_point_reference_count ||
        required_latent_points >
            impl_->budget
                .maximum_latent_carrier_coverage_point_reference_count) {
      return reject(
          ExactDirectMorseUnifiedResidentPreparationDecision::
              no_authority_budget_exhausted);
    }
    bundle.prior_root_coverages.reserve(root_ids.size());
    bundle.prior_root_coverage_point_references.reserve(
        required_root_points);
    bundle.latent_carrier_coverages.reserve(latent_ids.size());
    bundle.latent_carrier_coverage_point_references.reserve(
        required_latent_points);

    for (const auto root_id : root_ids) {
      const auto* root = find_root(prepared->prepared_state, root_id);
      if (root == nullptr) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                contradiction_locator_state_inconsistent);
      }
      const std::size_t offset =
          bundle.prior_root_coverage_point_references.size();
      bundle.prior_root_coverage_point_references.insert(
          bundle.prior_root_coverage_point_references.end(),
          root->point_ids.begin(),
          root->point_ids.end());
      bundle.prior_root_coverages.push_back(
          {root_id, offset, root->point_ids.size()});
    }
    for (const auto token_id : latent_ids) {
      if (token_id >= prepared->prepared_state.components.size()) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                contradiction_locator_state_inconsistent);
      }
      const std::size_t component_handle = find_component(
          prepared->prepared_state,
          static_cast<std::size_t>(token_id));
      const auto& component =
          prepared->prepared_state.components[component_handle];
      if (!component.active || component.root_id.has_value()) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                contradiction_locator_state_inconsistent);
      }
      const std::size_t offset =
          bundle.latent_carrier_coverage_point_references.size();
      bundle.latent_carrier_coverage_point_references.insert(
          bundle.latent_carrier_coverage_point_references.end(),
          component.latent_point_coverage.begin(),
          component.latent_point_coverage.end());
      bundle.latent_carrier_coverages.push_back(
          {token_id, offset, component.latent_point_coverage.size()});
    }
    if (!budget_accepts_authority(impl_->budget, bundle)) {
      return reject(
          ExactDirectMorseUnifiedResidentPreparationDecision::
              no_authority_budget_exhausted);
    }

    if (impl_->frozen_batch_source_replay_count >
        std::numeric_limits<std::size_t>::max() - 3U) {
      return reject(
          ExactDirectMorseUnifiedResidentPreparationDecision::
              no_prepared_state_rejected);
    }
    bundle.frozen_batch =
        build_exact_direct_frozen_unified_incidence_batch(
            *impl_->source.index,
            *impl_->source.cloud,
            *impl_->source.facade,
            *impl_->source.journal,
            *impl_->source.arm_budget,
            *impl_->source.arm_journal,
            *impl_->source.incidence_budget,
            *impl_->source.incidence_journal,
            *impl_->source.star_budget,
            impl_->source.traversal_order,
            *impl_->source.star,
            *impl_->source.plan_budget,
            impl_->plan,
            impl_->cursor,
            bundle.facet_resolutions,
            bundle.prior_root_coverages,
            bundle.prior_root_coverage_point_references,
            bundle.latent_carrier_coverages,
            bundle.latent_carrier_coverage_point_references,
            impl_->budget.frozen_batch);
    ++impl_->frozen_batch_source_replay_count;
    if (!bundle.frozen_batch.certified_frozen_unified_incidence_batch()) {
      return reject(
          ExactDirectMorseUnifiedResidentPreparationDecision::
              no_frozen_batch_rejected);
    }
    bundle.frozen_verification =
        verify_exact_direct_frozen_unified_incidence_batch(
            *impl_->source.index,
            *impl_->source.cloud,
            *impl_->source.facade,
            *impl_->source.journal,
            *impl_->source.arm_budget,
            *impl_->source.arm_journal,
            *impl_->source.incidence_budget,
            *impl_->source.incidence_journal,
            *impl_->source.star_budget,
            impl_->source.traversal_order,
            *impl_->source.star,
            *impl_->source.plan_budget,
            impl_->plan,
            impl_->cursor,
            bundle.facet_resolutions,
            bundle.prior_root_coverages,
            bundle.prior_root_coverage_point_references,
            bundle.latent_carrier_coverages,
            bundle.latent_carrier_coverage_point_references,
            impl_->budget.frozen_batch,
            bundle.frozen_batch);
    impl_->frozen_batch_source_replay_count += 2U;
    if (!frozen_verification_complete(bundle.frozen_verification)) {
      return reject(
          ExactDirectMorseUnifiedResidentPreparationDecision::
              no_frozen_batch_verification_rejected);
    }

    const std::vector<std::size_t> births =
        direct_birth_facets(impl_->plan, batch);
    std::vector<std::size_t> equal_facets;
    equal_facets.reserve(bundle.frozen_batch.equal_facet_binding_records.size());
    for (const auto& equal :
         bundle.frozen_batch.equal_facet_binding_records) {
      equal_facets.push_back(equal.facet_token_index);
    }
    std::sort(equal_facets.begin(), equal_facets.end());

    for (std::size_t group_index = 0U;
         group_index < bundle.frozen_batch.quotient.groups.size();
         ++group_index) {
      const auto& quotient_group =
          bundle.frozen_batch.quotient.groups[group_index];
      const auto& action_group =
          bundle.frozen_batch.action_plan.groups[group_index];
      const auto& delta = bundle.frozen_batch.coverage_deltas[group_index];
      if (quotient_group.group_index != group_index ||
          action_group.group_index != group_index ||
          delta.owner_group_index != group_index) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_prepared_state_rejected);
      }

      std::vector<std::size_t> handles;
      handles.reserve(quotient_group.token_count);
      std::vector<ExactFrozenIncidencePriorRootId> observed_prior_roots;
      for (std::size_t local = 0U; local < quotient_group.token_count;
           ++local) {
        const auto& token = bundle.frozen_batch.quotient.group_tokens
            [quotient_group.token_offset + local];
        if (token.token_id >=
            prepared->prepared_state.components.size()) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  no_prepared_state_rejected);
        }
        const std::size_t handle =
            static_cast<std::size_t>(token.token_id);
        handles.push_back(handle);
        if (token.kind == ExactFrozenIncidenceTokenKind::equal_facet) {
          auto& equal_component =
              prepared->prepared_state.components[handle];
          if (equal_component.active) {
            return reject(
                ExactDirectMorseUnifiedResidentPreparationDecision::
                    contradiction_locator_state_inconsistent);
          }
          equal_component.active = true;
          equal_component.parent_handle = handle;
          equal_component.root_id.reset();
          equal_component.latent_point_coverage.assign(
              key_points(impl_->plan.facet_tokens[handle].facet_key).begin(),
              key_points(impl_->plan.facet_tokens[handle].facet_key).end());
        } else {
          const std::size_t representative = find_component(
              prepared->prepared_state, handle);
          const auto& component =
              prepared->prepared_state.components[representative];
          if (!component.active) {
            return reject(
                ExactDirectMorseUnifiedResidentPreparationDecision::
                    contradiction_locator_state_inconsistent);
          }
          if (token.kind ==
              ExactFrozenIncidenceTokenKind::rooted_carrier) {
            if (!component.root_id.has_value()) {
              return reject(
                  ExactDirectMorseUnifiedResidentPreparationDecision::
                      contradiction_locator_state_inconsistent);
            }
            observed_prior_roots.push_back(*component.root_id);
          } else if (component.root_id.has_value()) {
            return reject(
                ExactDirectMorseUnifiedResidentPreparationDecision::
                    contradiction_locator_state_inconsistent);
          }
        }
      }
      std::sort(handles.begin(), handles.end());
      handles.erase(std::unique(handles.begin(), handles.end()),
                    handles.end());
      std::sort(observed_prior_roots.begin(), observed_prior_roots.end());
      observed_prior_roots.erase(
          std::unique(observed_prior_roots.begin(),
                      observed_prior_roots.end()),
          observed_prior_roots.end());
      const auto prior_begin =
          bundle.frozen_batch.action_plan.prior_root_ids.begin() +
          static_cast<std::ptrdiff_t>(action_group.prior_root_offset);
      const std::vector<ExactFrozenIncidencePriorRootId> planned_prior_roots(
          prior_begin,
          prior_begin +
              static_cast<std::ptrdiff_t>(action_group.prior_root_count));
      if (handles.empty() || observed_prior_roots != planned_prior_roots ||
          planned_prior_roots.size() != action_group.q_r) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_prepared_state_rejected);
      }

      std::vector<PointId> resultant_coverage;
      for (const auto root_id : planned_prior_roots) {
        const auto* root = find_root(prepared->prepared_state, root_id);
        if (root == nullptr) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  contradiction_locator_state_inconsistent);
        }
        resultant_coverage.insert(
            resultant_coverage.end(),
            root->point_ids.begin(),
            root->point_ids.end());
      }
      std::vector<PointId> delta_points;
      delta_points.reserve(delta.point_reference_count);
      for (std::size_t local = 0U; local < delta.point_reference_count;
           ++local) {
        const PointId point_id =
            bundle.frozen_batch.coverage_delta_points
                [delta.point_reference_offset + local]
                    .point_id;
        delta_points.push_back(point_id);
        resultant_coverage.push_back(point_id);
      }
      canonicalize_points(resultant_coverage);

      ExactFrozenIncidencePriorRootId resultant_root_id{};
      if (action_group.q_r == 1U) {
        resultant_root_id = planned_prior_roots.front();
        auto* root =
            find_root(prepared->prepared_state, resultant_root_id);
        if (root == nullptr) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  no_prepared_state_rejected);
        }
        root->point_ids = resultant_coverage;
      } else {
        if (prepared->prepared_state.next_root_id == 0U) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  no_prepared_state_rejected);
        }
        resultant_root_id = prepared->prepared_state.next_root_id;
        if (prepared->prepared_state.next_root_id ==
            std::numeric_limits<ExactFrozenIncidencePriorRootId>::max()) {
          prepared->prepared_state.next_root_id = 0U;
        } else {
          ++prepared->prepared_state.next_root_id;
        }
        prepared->prepared_state.roots.push_back(
            {resultant_root_id, std::move(resultant_coverage)});
      }

      const std::size_t canonical_handle = handles.front();
      for (const std::size_t handle : handles) {
        const std::size_t left = find_component(
            prepared->prepared_state, canonical_handle);
        const std::size_t right = find_component(
            prepared->prepared_state, handle);
        if (left != right) {
          ExactDirectSparseFacetWitness witness;
          if (!checked_next_witness(
                  prepared->prepared_state,
                  impl_->authority_id,
                  witness)) {
            return reject(
                ExactDirectMorseUnifiedResidentPreparationDecision::
                    no_prepared_state_rejected);
          }
          prepared->unions.push_back(
              {prepared->unions.size(), left, right, witness});
          const std::size_t parent = std::min(left, right);
          const std::size_t child = std::max(left, right);
          prepared->prepared_state.components[child].parent_handle = parent;
        }
      }
      const std::size_t representative = find_component(
          prepared->prepared_state, canonical_handle);
      auto& resultant_component =
          prepared->prepared_state.components[representative];
      resultant_component.active = true;
      resultant_component.root_id = resultant_root_id;
      resultant_component.latent_point_coverage.clear();
      for (const std::size_t handle : handles) {
        if (handle != representative) {
          prepared->prepared_state.components[handle].root_id.reset();
          prepared->prepared_state.components[handle]
              .latent_point_coverage.clear();
        }
      }

      prepared->prepared_state.group_records.push_back(
          {prepared->prepared_state.group_records.size(),
           batch.batch_index,
           group_index,
           batch.squared_level,
           batch.order,
           action_group.q_r,
           action_group.action,
           resultant_root_id,
           planned_prior_roots,
           std::move(delta_points),
           delta.point_reference_count == 0U});
    }

    for (const auto& equal :
         bundle.frozen_batch.equal_facet_binding_records) {
      if (equal.facet_token_index >=
          prepared->prepared_state.components.size()) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_prepared_state_rejected);
      }
      ExactDirectSparseFacetWitness witness;
      if (!checked_next_witness(
              prepared->prepared_state, impl_->authority_id, witness)) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_prepared_state_rejected);
      }
      prepared->bindings.push_back(
          {prepared->bindings.size(),
           impl_->plan.facet_tokens[equal.facet_token_index].facet_key,
           equal.facet_token_index,
           witness});
      ++bundle.counters.planned_equal_binding_count;
    }

    for (const std::size_t birth_handle : births) {
      if (birth_handle >= prepared->prepared_state.components.size()) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_prepared_state_rejected);
      }
      const bool same_batch_equal =
          std::binary_search(
              equal_facets.begin(), equal_facets.end(), birth_handle);
      const std::size_t prior_representative =
          find_component(impl_->state, birth_handle);
      if (impl_->state.components[prior_representative].active &&
          !same_batch_equal) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                contradiction_locator_state_inconsistent);
      }
      auto& component =
          prepared->prepared_state.components[birth_handle];
      if (!component.active) {
        component.active = true;
        component.parent_handle = birth_handle;
        component.root_id.reset();
        component.latent_point_coverage.assign(
            key_points(impl_->plan.facet_tokens[birth_handle].facet_key)
                .begin(),
            key_points(impl_->plan.facet_tokens[birth_handle].facet_key)
                .end());
      }
      ExactDirectSparseFacetWitness witness;
      if (!checked_next_witness(
              prepared->prepared_state, impl_->authority_id, witness)) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_prepared_state_rejected);
      }
      prepared->bindings.push_back(
          {prepared->bindings.size(),
           impl_->plan.facet_tokens[birth_handle].facet_key,
           birth_handle,
           witness});
      ++bundle.counters.planned_birth_binding_count;
    }

    bundle.counters.planned_component_union_count =
        prepared->unions.size();
    bundle.counters.planned_group_record_count =
        bundle.frozen_batch.coverage_deltas.size();
    if (prepared->unions.size() >
            impl_->budget.locator.maximum_batch_union_count ||
        prepared->bindings.size() >
            impl_->budget.locator.maximum_batch_binding_count ||
        !update_resident_storage_counts(
            prepared->prepared_state, impl_->budget)) {
      return reject(
          ExactDirectMorseUnifiedResidentPreparationDecision::
              no_prepared_state_rejected);
    }

    bundle.locator_snapshot_strictly_pre_batch =
        prepared->source_stamp == impl_->locator.snapshot_stamp() &&
        prepared->source_stamp.committed_batch_count == impl_->cursor;
    bundle.every_unresolved_facet_has_fresh_exact_equal_miniball =
        bundle.counters.unresolved_locator_probe_count ==
            bundle.counters.fresh_facet_miniball_build_count &&
        bundle.counters.fresh_facet_miniball_build_count ==
            bundle.counters.fresh_facet_miniball_verification_count &&
        bundle.counters.equal_resolution_count ==
            bundle.counters.unresolved_locator_probe_count;
    bundle.csr_authorities_share_identity_and_pre_batch_state = true;
    bundle.frozen_batch_built_and_freshly_verified = true;
    bundle.global_facet_coface_or_gamma_catalog_materialized = false;
    bundle.supplied_star_global_completeness_claimed = false;
    bundle.public_status_claimed = false;
    if (!budget_accepts_authority(impl_->budget, bundle) ||
        !bundle.certified_strict_pre_batch_bundle()) {
      return reject(
          ExactDirectMorseUnifiedResidentPreparationDecision::
              no_prepared_state_rejected);
    }

    output.ticket.emplace(
        ExactDirectMorseUnifiedResidentPreparedBatch{
            std::move(prepared)});
    output.strict_pre_batch_bundle_certified = true;
    output.decision =
        ExactDirectMorseUnifiedResidentPreparationDecision::
            complete_certified_prepared_batch;
    return output;
  } catch (const std::bad_alloc&) {
    return reject(
        ExactDirectMorseUnifiedResidentPreparationDecision::
            no_allocation_failed);
  } catch (const std::length_error&) {
    return reject(
        ExactDirectMorseUnifiedResidentPreparationDecision::
            no_authority_budget_exhausted);
  } catch (const std::logic_error&) {
    return reject(
        ExactDirectMorseUnifiedResidentPreparationDecision::
            no_prepared_state_rejected);
  }
}

ExactDirectMorseUnifiedResidentCommitResult
ExactDirectMorseUnifiedResidentSession::commit(
    ExactDirectMorseUnifiedResidentPreparedBatch&& ticket) noexcept {
  ExactDirectMorseUnifiedResidentCommitResult output;
  if (ticket.impl_ == nullptr || ticket.impl_->consumed) {
    output.ticket_consumed = true;
    output.no_scientific_state_mutated_on_failure = true;
    output.decision =
        ExactDirectMorseUnifiedResidentCommitDecision::
            no_ticket_already_consumed;
    return output;
  }
  ticket.impl_->consumed = true;
  output.ticket_consumed = true;
  if (impl_ == nullptr || ticket.impl_->seal.get() != impl_->seal.get()) {
    output.no_scientific_state_mutated_on_failure = true;
    output.decision =
        ExactDirectMorseUnifiedResidentCommitDecision::
            no_foreign_ticket_rejected;
    return output;
  }
  if (ticket.impl_->source_cursor != impl_->cursor ||
      ticket.impl_->source_epoch != impl_->epoch ||
      ticket.impl_->source_stamp != impl_->locator.snapshot_stamp()) {
    output.no_scientific_state_mutated_on_failure = true;
    output.decision =
        ExactDirectMorseUnifiedResidentCommitDecision::
            no_stale_ticket_rejected;
    return output;
  }

  output.committed_batch_index = ticket.impl_->source_cursor;
  output.exactly_one_locator_apply_batch_called = true;
  try {
    output.locator_batch = impl_->locator.apply_batch(
        ticket.impl_->queries,
        ticket.impl_->unions,
        ticket.impl_->bindings);
  } catch (const std::bad_alloc&) {
    if (impl_->locator.snapshot_stamp() != ticket.impl_->source_stamp) {
      std::terminate();
    }
    output.no_scientific_state_mutated_on_failure = true;
    output.decision =
        ExactDirectMorseUnifiedResidentCommitDecision::
            no_locator_transaction_rejected;
    return output;
  } catch (const std::length_error&) {
    if (impl_->locator.snapshot_stamp() != ticket.impl_->source_stamp) {
      std::terminate();
    }
    output.no_scientific_state_mutated_on_failure = true;
    output.decision =
        ExactDirectMorseUnifiedResidentCommitDecision::
            no_locator_transaction_rejected;
    return output;
  }
  if (!output.locator_batch.certified_committed_batch()) {
    if (impl_->locator.snapshot_stamp() != ticket.impl_->source_stamp) {
      std::terminate();
    }
    output.no_scientific_state_mutated_on_failure = true;
    output.decision =
        ExactDirectMorseUnifiedResidentCommitDecision::
            no_locator_transaction_rejected;
    return output;
  }

  const auto committed_stamp = impl_->locator.snapshot_stamp();
  if (committed_stamp.external_authority_id != impl_->authority_id ||
      committed_stamp.committed_batch_count != impl_->cursor + 1U ||
      committed_stamp == ticket.impl_->source_stamp) {
    // The locator has reported a commit.  Any disagreement now is a
    // fail-stop invariant breach: returning an ordinary atomic failure would
    // falsely claim that no scientific mutation occurred.
    std::terminate();
  }

  impl_->state = std::move(ticket.impl_->prepared_state);
  impl_->cursor = ticket.impl_->source_cursor + 1U;
  impl_->epoch = ticket.impl_->source_epoch + 1U;
  output.committed_epoch = impl_->epoch;
  output.prepared_state_installed_after_locator_commit = true;
  output.cursor_and_epoch_advanced_once = true;
  output.no_scientific_state_mutated_on_failure = false;
  output.decision =
      ExactDirectMorseUnifiedResidentCommitDecision::
          complete_certified_atomic_batch_commit;
  return output;
}

ExactDirectMorseUnifiedResidentInitializationResult
initialize_exact_direct_morse_unified_resident_session(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget&
        source_star_budget,
    spatial::LbvhTraversalOrder source_star_traversal_order,
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& source_star,
    const ExactDirectSparseUnifiedLevelPlanBudget& source_plan_budget,
    const ExactDirectSparseUnifiedLevelPlanResult& source_plan,
    std::uint64_t session_authority_id,
    const ExactDirectMorseUnifiedResidentSessionBudget& budget) {
  ExactDirectMorseUnifiedResidentInitializationResult output;
  if (session_authority_id == 0U ||
      source_plan.facet_tokens.size() >
          budget.locator.maximum_component_handle_count ||
      source_plan.batches.size() >
          budget.locator.maximum_committed_batch_count ||
      source_plan.facet_tokens.size() >
          std::numeric_limits<ExactFrozenIncidenceTokenId>::max()) {
    output.decision =
        ExactDirectMorseUnifiedResidentInitializationDecision::
            no_session_budget_rejected;
    return output;
  }
  try {
    const auto plan_verification =
        verify_exact_direct_sparse_unified_level_plan(
            index,
            cloud,
            source_facade,
            source_journal,
            source_arm_budget,
            source_arm_journal,
            source_incidence_budget,
            source_incidence_journal,
            source_star_budget,
            source_star_traversal_order,
            source_star,
            source_plan_budget,
            source_plan);
    output.source_plan_initial_verification_count = 1U;
    if (!plan_verification.result_certified) {
      output.decision =
          ExactDirectMorseUnifiedResidentInitializationDecision::
              no_source_plan_not_freshly_verified;
      return output;
    }

    auto impl = std::make_unique<ExactDirectMorseUnifiedResidentSession::Impl>();
    impl->source = {
        &index,
        &cloud,
        &source_facade,
        &source_journal,
        &source_arm_budget,
        &source_arm_journal,
        &source_incidence_budget,
        &source_incidence_journal,
        &source_star_budget,
        source_star_traversal_order,
        &source_star,
        &source_plan_budget,
    };
    impl->budget = budget;
    impl->plan = source_plan;
    impl->authority_id = session_authority_id;
    impl->locator_instance_id = allocate_locator_instance_id();
    if (impl->locator_instance_id == 0U) {
      output.decision =
          ExactDirectMorseUnifiedResidentInitializationDecision::
              no_session_budget_rejected;
      return output;
    }
    impl->source_plan_initial_verification_count = 1U;
    impl->seal = std::make_shared<const SessionSeal>(
        SessionSeal{session_authority_id, impl->locator_instance_id});
    impl->locator = build_exact_direct_sparse_positive_facet_locator(
        impl->plan.facet_tokens.size(),
        budget.locator,
        {session_authority_id, ~std::uint64_t{0U}});
    if (!impl->locator.certified_positive_locator()) {
      output.decision =
          ExactDirectMorseUnifiedResidentInitializationDecision::
              no_locator_initialization_rejected;
      return output;
    }
    impl->state.components.resize(impl->plan.facet_tokens.size());
    for (std::size_t index_value = 0U;
         index_value < impl->state.components.size();
         ++index_value) {
      if (impl->plan.facet_tokens[index_value].facet_token_index !=
          index_value) {
        output.decision =
            ExactDirectMorseUnifiedResidentInitializationDecision::
                no_source_plan_not_freshly_verified;
        return output;
      }
      impl->state.components[index_value].component_handle = index_value;
      impl->state.components[index_value].parent_handle = index_value;
    }
    impl->state.roots.reserve(budget.maximum_resident_root_count);
    impl->state.group_records.reserve(budget.maximum_group_record_count);
    impl->initialized = true;
    ExactDirectMorseUnifiedResidentSession session{std::move(impl)};
    if (!session.certified_resident_session()) {
      output.decision =
          ExactDirectMorseUnifiedResidentInitializationDecision::
              no_locator_initialization_rejected;
      return output;
    }
    output.session.emplace(std::move(session));
    output.source_plan_freshly_verified_once = true;
    output.source_plan_owned_by_session = true;
    output.locator_and_component_state_initialized = true;
    output.no_global_facet_coface_or_gamma_catalog_materialized = true;
    output.public_status_claimed = false;
    output.decision =
        ExactDirectMorseUnifiedResidentInitializationDecision::
            complete_certified_bounded_resident_session;
    return output;
  } catch (const std::bad_alloc&) {
    output.decision =
        ExactDirectMorseUnifiedResidentInitializationDecision::
            no_allocation_failed;
    return output;
  } catch (const std::length_error&) {
    output.decision =
        ExactDirectMorseUnifiedResidentInitializationDecision::
            no_session_budget_rejected;
    return output;
  }
}

}  // namespace morsehgp3d::hierarchy
