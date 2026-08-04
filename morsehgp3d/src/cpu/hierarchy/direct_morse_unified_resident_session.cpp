#include "morsehgp3d/hierarchy/direct_morse_unified_resident_session.hpp"

#include "direct_frozen_unified_incidence_batch_internal.hpp"
#include "morsehgp3d/hierarchy/facet_miniball.hpp"

#include <algorithm>
#include <atomic>
#include <exception>
#include <limits>
#include <new>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

struct ResidentState {
  ResidentState() = default;
  ResidentState(const ResidentState&) = delete;
  ResidentState& operator=(const ResidentState&) = delete;
  ResidentState(ResidentState&&) noexcept = default;
  ResidentState& operator=(ResidentState&&) noexcept = default;

  std::vector<ExactDirectMorseUnifiedResidentComponentState> components;
  std::vector<ExactDirectMorseUnifiedResidentRootCoverage> roots;
  std::vector<ExactDirectMorseUnifiedResidentGroupRecord> group_records;
  ExactFrozenIncidencePriorRootId next_root_id{1U};
  std::uint64_t next_replay_token{1U};
  std::size_t resident_root_point_reference_count{};
  std::size_t resident_component_latent_point_reference_count{};
  std::size_t group_child_reference_count{};
  std::size_t group_coverage_delta_point_reference_count{};
};

static_assert(noexcept(std::declval<ResidentState&>() =
                       std::declval<ResidentState&&>()));

struct ResidentComponentPatch {
  std::size_t handle{};
  ExactDirectMorseUnifiedResidentComponentState next;
};

struct ResidentRootReplacement {
  std::size_t root_index{};
  ExactFrozenIncidencePriorRootId root_id{};
  std::vector<PointId> next_point_ids;
};

struct ResidentStateDelta {
  std::vector<ResidentComponentPatch> component_patches;
  std::vector<ResidentRootReplacement> root_replacements;
  std::vector<ExactDirectMorseUnifiedResidentRootCoverage> new_roots;
  std::vector<ExactDirectMorseUnifiedResidentGroupRecord> new_group_records;
  ExactFrozenIncidencePriorRootId next_root_id{1U};
  std::uint64_t next_replay_token{1U};
  std::size_t final_resident_root_point_reference_count{};
  std::size_t final_resident_component_latent_point_reference_count{};
  std::size_t final_group_child_reference_count{};
  std::size_t final_group_coverage_delta_point_reference_count{};
  std::size_t component_latent_point_materialization_count{};
};

struct OutstandingTicketRegistry {
  std::size_t live_ticket_count{};
  std::size_t maximum_ticket_count{};
};

static_assert(std::is_nothrow_swappable_v<
              ExactDirectMorseUnifiedResidentComponentState>);
static_assert(std::is_nothrow_move_constructible_v<
              ExactDirectMorseUnifiedResidentRootCoverage>);
static_assert(std::is_nothrow_move_constructible_v<
              ExactDirectMorseUnifiedResidentGroupRecord>);
static_assert(noexcept(std::declval<std::vector<PointId>&>().swap(
    std::declval<std::vector<PointId>&>())));

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

[[nodiscard]] bool valid_closure_traversal_order(
    spatial::LbvhTraversalOrder traversal_order) noexcept {
  return traversal_order == spatial::LbvhTraversalOrder::near_first ||
         traversal_order == spatial::LbvhTraversalOrder::far_first;
}

[[nodiscard]] bool closure_budget_within_confidence_caps(
    const ExactDirectSparseFacetDescentClosureBudget& budget) noexcept {
  return budget.maximum_seed_count <=
             direct_sparse_facet_descent_closure_maximum_seed_count &&
         budget.maximum_node_count <=
             direct_sparse_facet_descent_closure_maximum_node_count &&
         budget.maximum_step_call_count <=
             direct_sparse_facet_descent_closure_maximum_step_call_count &&
         budget.maximum_memo_slot_count <=
             direct_sparse_facet_descent_closure_maximum_memo_slot_count;
}

// Process-private authority issued only after fresh rank-window verification.
// Its type and constructor never cross the public header.  Keeping the closure
// controls in this move-only value prevents a caller-provided bool from
// enabling the certified carrier path after initialization.
class ResidentNormalizedStrictFacetClosureAuthority {
 public:
  ResidentNormalizedStrictFacetClosureAuthority(
      const ResidentNormalizedStrictFacetClosureAuthority&) = delete;
  ResidentNormalizedStrictFacetClosureAuthority& operator=(
      const ResidentNormalizedStrictFacetClosureAuthority&) = delete;
  ResidentNormalizedStrictFacetClosureAuthority(
      ResidentNormalizedStrictFacetClosureAuthority&&) noexcept = default;
  ResidentNormalizedStrictFacetClosureAuthority& operator=(
      ResidentNormalizedStrictFacetClosureAuthority&&) noexcept = default;

  [[nodiscard]] static std::optional<
      ResidentNormalizedStrictFacetClosureAuthority>
  issue(
      const ExactDirectSparseUnifiedLevelPlanResult& plan,
      const ExactDirectRankWindowSaturatedH0Authority& rank_authority,
      const ExactDirectRankWindowSaturatedH0Verification& verification,
      const ExactDirectSparseFacetDescentClosureBudget& closure_budget,
      const ExactDirectSparseFacetDescentClosureConfig& closure_config,
      spatial::LbvhTraversalOrder traversal_order,
      std::uint64_t session_authority_id) noexcept {
    if (!verification.result_certified || !rank_authority.certified() ||
        session_authority_id == 0U ||
        !closure_budget_within_confidence_caps(closure_budget) ||
        !valid_closure_traversal_order(traversal_order) ||
        rank_authority.requirements.point_count != plan.point_count ||
        rank_authority.requirements.effective_maximum_order >=
            rank_authority.requirements.point_count ||
        rank_authority.source_pair_canonical_cloud_digest !=
            plan.source_pair_canonical_cloud_digest ||
        rank_authority.source_higher_canonical_cloud_digest !=
            plan.source_higher_canonical_cloud_digest ||
        rank_authority.source_pair_semantic_digest !=
            plan.source_pair_semantic_digest ||
        rank_authority.source_higher_semantic_digest !=
            plan.source_higher_semantic_digest) {
      return std::nullopt;
    }
    return ResidentNormalizedStrictFacetClosureAuthority{
        &plan,
        rank_authority,
        closure_budget,
        closure_config,
        traversal_order,
        session_authority_id};
  }

  [[nodiscard]] bool certifies(
      const ExactDirectSparseUnifiedLevelPlanResult& plan,
      std::uint64_t session_authority_id) const noexcept {
    return plan_ == &plan && session_authority_id_ != 0U &&
           session_authority_id_ == session_authority_id &&
           rank_authority_.certified() &&
           rank_authority_.requirements.point_count == plan.point_count &&
           rank_authority_.requirements.effective_maximum_order <
               rank_authority_.requirements.point_count &&
           rank_authority_.source_pair_canonical_cloud_digest ==
               plan.source_pair_canonical_cloud_digest &&
           rank_authority_.source_higher_canonical_cloud_digest ==
               plan.source_higher_canonical_cloud_digest &&
           rank_authority_.source_pair_semantic_digest ==
               plan.source_pair_semantic_digest &&
           rank_authority_.source_higher_semantic_digest ==
               plan.source_higher_semantic_digest &&
           closure_budget_within_confidence_caps(closure_budget_) &&
           valid_closure_traversal_order(traversal_order_);
  }

  [[nodiscard]] const ExactDirectSparseFacetDescentClosureBudget& budget()
      const noexcept {
    return closure_budget_;
  }
  [[nodiscard]] const ExactDirectSparseFacetDescentClosureConfig& config()
      const noexcept {
    return closure_config_;
  }
  [[nodiscard]] spatial::LbvhTraversalOrder traversal_order()
      const noexcept {
    return traversal_order_;
  }

 private:
  ResidentNormalizedStrictFacetClosureAuthority(
      const ExactDirectSparseUnifiedLevelPlanResult* plan,
      const ExactDirectRankWindowSaturatedH0Authority& rank_authority,
      const ExactDirectSparseFacetDescentClosureBudget& closure_budget,
      const ExactDirectSparseFacetDescentClosureConfig& closure_config,
      spatial::LbvhTraversalOrder traversal_order,
      std::uint64_t session_authority_id) noexcept
      : plan_(plan),
        rank_authority_(rank_authority),
        closure_budget_(closure_budget),
        closure_config_(closure_config),
        traversal_order_(traversal_order),
        session_authority_id_(session_authority_id) {}

  const ExactDirectSparseUnifiedLevelPlanResult* plan_{};
  ExactDirectRankWindowSaturatedH0Authority rank_authority_{};
  ExactDirectSparseFacetDescentClosureBudget closure_budget_{};
  ExactDirectSparseFacetDescentClosureConfig closure_config_{};
  spatial::LbvhTraversalOrder traversal_order_{
      spatial::LbvhTraversalOrder::near_first};
  std::uint64_t session_authority_id_{};
};

// The public fixed-size incidence receipt remains caller-owned and copyable.
// Only a fresh recursive verification can mint this process-private move-only
// capability.  It is bound to the immutable compatibility cursor and session
// authority, so no public bool or later receipt mutation can promote an
// existing candidate or rank+closure session.
class ResidentNormalizedHorizontalIncidenceReductionAuthority {
 public:
  ResidentNormalizedHorizontalIncidenceReductionAuthority(
      const ResidentNormalizedHorizontalIncidenceReductionAuthority&) =
      delete;
  ResidentNormalizedHorizontalIncidenceReductionAuthority& operator=(
      const ResidentNormalizedHorizontalIncidenceReductionAuthority&) =
      delete;
  ResidentNormalizedHorizontalIncidenceReductionAuthority(
      ResidentNormalizedHorizontalIncidenceReductionAuthority&&) noexcept =
      default;
  ResidentNormalizedHorizontalIncidenceReductionAuthority& operator=(
      ResidentNormalizedHorizontalIncidenceReductionAuthority&&) noexcept =
      default;

  [[nodiscard]] static std::optional<
      ResidentNormalizedHorizontalIncidenceReductionAuthority>
  issue(
      const ExactDirectSparseUnifiedLevelPlanResult& compatibility_plan,
      const ExactDirectNormalizedH0SourcePlanResult& source_plan,
      const ExactDirectNormalizedH0IncidenceReductionAuthority& authority,
      const ExactDirectNormalizedH0IncidenceReductionVerification&
          verification,
      std::uint64_t session_authority_id) noexcept {
    const bool direct_birth_partition_fits =
        compatibility_plan.required_direct_birth_reference_count <=
        compatibility_plan.direct_references.size();
    const std::size_t compatibility_direct_coface_count =
        direct_birth_partition_fits
        ? compatibility_plan.direct_references.size() -
              compatibility_plan.required_direct_birth_reference_count
        : 0U;
    const bool compatibility_coface_count_fits =
        direct_birth_partition_fits &&
        compatibility_direct_coface_count <=
        std::numeric_limits<std::size_t>::max() -
            compatibility_plan.residual_references.size();
    const std::size_t compatibility_coface_count =
        compatibility_coface_count_fits
        ? compatibility_direct_coface_count +
              compatibility_plan.residual_references.size()
        : 0U;
    if (!verification.result_certified ||
        !verification.expected_authority_freshly_reconstructed ||
        !verification.observed_recursively_equal ||
        !verification.unsupported_claims_remain_false ||
        !authority.certified_horizontal_incidence_reduction() ||
        !authority.incidence_complete_reduction_proved ||
        session_authority_id == 0U ||
        !source_plan.certified_complete_candidate_source_plan() ||
        compatibility_plan.point_count != source_plan.point_count ||
        compatibility_plan.point_count != authority.point_count ||
        compatibility_plan.batches.size() != authority.normalized_batch_count ||
        !compatibility_coface_count_fits ||
        compatibility_coface_count != authority.normalized_coface_count ||
        source_plan.source_direct_event_count !=
            authority.source_direct_event_count ||
        source_plan.source_gateway_token_count !=
            authority.source_core_facet_count ||
        source_plan.source_gateway_candidate_count !=
            authority.source_gateway_candidate_count ||
        source_plan.cofaces.size() != authority.normalized_coface_count ||
        source_plan.batches.size() != authority.normalized_batch_count ||
        source_plan.source_gateway_scientific_identity_digest !=
            authority.source_gateway_scientific_identity_digest ||
        compatibility_plan.source_pair_canonical_cloud_digest !=
            authority.source_pair_canonical_cloud_digest ||
        compatibility_plan.source_higher_canonical_cloud_digest !=
            authority.source_higher_canonical_cloud_digest ||
        compatibility_plan.source_pair_semantic_digest !=
            authority.source_pair_semantic_digest ||
        compatibility_plan.source_higher_semantic_digest !=
            authority.source_higher_semantic_digest ||
        !compatibility_plan.no_k_plus_one_coface_key_persisted ||
        !compatibility_plan.no_global_facet_or_coface_catalog_materialized ||
        compatibility_plan.bounded_star_global_completeness_claimed ||
        compatibility_plan.public_status_claimed) {
      return std::nullopt;
    }
    return ResidentNormalizedHorizontalIncidenceReductionAuthority{
        &compatibility_plan, authority, session_authority_id};
  }

  [[nodiscard]] bool certifies(
      const ExactDirectSparseUnifiedLevelPlanResult& compatibility_plan,
      std::uint64_t session_authority_id) const noexcept {
    if (compatibility_plan.required_direct_birth_reference_count >
        compatibility_plan.direct_references.size()) {
      return false;
    }
    const std::size_t compatibility_direct_coface_count =
        compatibility_plan.direct_references.size() -
        compatibility_plan.required_direct_birth_reference_count;
    if (compatibility_direct_coface_count >
        std::numeric_limits<std::size_t>::max() -
            compatibility_plan.residual_references.size()) {
      return false;
    }
    const std::size_t compatibility_coface_count =
        compatibility_direct_coface_count +
        compatibility_plan.residual_references.size();
    return compatibility_plan_ == &compatibility_plan &&
           session_authority_id_ != 0U &&
           session_authority_id_ == session_authority_id &&
           authority_.certified_horizontal_incidence_reduction() &&
           authority_.incidence_complete_reduction_proved &&
           compatibility_plan.point_count == authority_.point_count &&
           compatibility_plan.batches.size() ==
               authority_.normalized_batch_count &&
           compatibility_coface_count == authority_.normalized_coface_count &&
           compatibility_plan.source_pair_canonical_cloud_digest ==
               authority_.source_pair_canonical_cloud_digest &&
           compatibility_plan.source_higher_canonical_cloud_digest ==
               authority_.source_higher_canonical_cloud_digest &&
           compatibility_plan.source_pair_semantic_digest ==
               authority_.source_pair_semantic_digest &&
           compatibility_plan.source_higher_semantic_digest ==
               authority_.source_higher_semantic_digest &&
           compatibility_plan.no_k_plus_one_coface_key_persisted &&
           compatibility_plan.no_global_facet_or_coface_catalog_materialized &&
           !compatibility_plan.bounded_star_global_completeness_claimed &&
           !compatibility_plan.public_status_claimed;
  }

 private:
  ResidentNormalizedHorizontalIncidenceReductionAuthority(
      const ExactDirectSparseUnifiedLevelPlanResult* compatibility_plan,
      const ExactDirectNormalizedH0IncidenceReductionAuthority& authority,
      std::uint64_t session_authority_id) noexcept
      : compatibility_plan_(compatibility_plan),
        authority_(authority),
        session_authority_id_(session_authority_id) {}

  const ExactDirectSparseUnifiedLevelPlanResult* compatibility_plan_{};
  ExactDirectNormalizedH0IncidenceReductionAuthority authority_{};
  std::uint64_t session_authority_id_{};
};

static_assert(
    !std::is_copy_constructible_v<
        ResidentNormalizedHorizontalIncidenceReductionAuthority> &&
    std::is_nothrow_move_constructible_v<
        ResidentNormalizedHorizontalIncidenceReductionAuthority>);

[[nodiscard]] bool certified_normalized_strict_closure_mode(
    ExactDirectNormalizedH0ResidentRetractionMode mode) noexcept {
  return mode ==
             ExactDirectNormalizedH0ResidentRetractionMode::
                 certified_rank_window_and_sparse_strict_facet_closure ||
         mode ==
             ExactDirectNormalizedH0ResidentRetractionMode::
                 certified_horizontal_incidence_complete_rank_window_and_sparse_strict_facet_closure;
}

[[nodiscard]] bool certified_normalized_incidence_complete_mode(
    ExactDirectNormalizedH0ResidentRetractionMode mode) noexcept {
  return mode ==
         ExactDirectNormalizedH0ResidentRetractionMode::
             certified_horizontal_incidence_complete_rank_window_and_sparse_strict_facet_closure;
}

[[nodiscard]] bool facet_key_is_canonical(
    const ExactDirectSparseFacetKey& key) noexcept;

struct ResidentStrictFacetCarrierRecord {
  std::size_t facet_token_index{};
  ExactDirectSparseFacetKey source_facet_key{};
  ExactDirectSparseFacetKey terminal_facet_key{};
  ExactDirectSparseComponentHandle terminal_component_handle{};
  ExactDirectSparseFacetWitness terminal_binding_witness{};
  ExactFrozenIncidenceTokenKind token_kind{
      ExactFrozenIncidenceTokenKind::latent_carrier};
  std::optional<ExactFrozenIncidencePriorRootId> prior_root_id;
};

// Compact ticket-local replacement for the transient closure graph.  It is
// move-only and binds every strict seed to the same pre-batch snapshot that
// was used by the frozen authority bundle.
class ResidentStrictFacetClosureAttestation {
 public:
  ResidentStrictFacetClosureAttestation(
      const ResidentStrictFacetClosureAttestation&) = delete;
  ResidentStrictFacetClosureAttestation& operator=(
      const ResidentStrictFacetClosureAttestation&) = delete;
  ResidentStrictFacetClosureAttestation(
      ResidentStrictFacetClosureAttestation&&) noexcept = default;
  ResidentStrictFacetClosureAttestation& operator=(
      ResidentStrictFacetClosureAttestation&&) noexcept = default;

  ResidentStrictFacetClosureAttestation(
      ExactDirectSparsePositiveFacetLocatorSnapshotStamp source_stamp,
      std::size_t source_batch_index,
      exact::ExactLevel squared_level,
      std::size_t order,
      std::vector<ResidentStrictFacetCarrierRecord> records,
      std::size_t node_count,
      std::size_t edge_count,
      std::size_t snapshot_check_count) noexcept
      : source_stamp_(source_stamp),
        source_batch_index_(source_batch_index),
        squared_level_(std::move(squared_level)),
        order_(order),
        records_(std::move(records)),
        node_count_(node_count),
        edge_count_(edge_count),
        snapshot_check_count_(snapshot_check_count) {}

  [[nodiscard]] bool attests(
      const ExactDirectMorseUnifiedResidentAuthorityBundle& bundle,
      std::span<const ExactDirectSparseFacetBinding> bindings) const
      noexcept {
    if (records_.empty() || bundle.identity.locator_stamp != source_stamp_ ||
        bundle.source_batch_index != source_batch_index_ ||
        bundle.squared_level != squared_level_ || bundle.order != order_ ||
        bundle.counters.normalized_strict_facet_seed_count !=
            records_.size() ||
        bundle.counters.normalized_strict_facet_closure_build_count != 1U ||
        bundle.counters.normalized_strict_facet_closure_node_count !=
            node_count_ ||
        bundle.counters.normalized_strict_facet_closure_edge_count !=
            edge_count_ ||
        bundle.counters
                .normalized_strict_facet_closure_locator_snapshot_check_count !=
            snapshot_check_count_ ||
        bundle.counters.normalized_strict_facet_carrier_resolution_count !=
            records_.size() ||
        bundle.counters.planned_strict_facet_binding_count !=
            records_.size() ||
        !bundle.rank_window_saturated_h0_authority_certified ||
        !bundle.private_strict_facet_closure_attestation_issued ||
        !bundle
             .every_strict_facet_miss_has_certified_relative_positive_closure ||
        !bundle.strict_facet_closure_bound_to_pre_batch_locator_snapshot) {
      return false;
    }
    for (const auto& record : records_) {
      if (!facet_key_is_canonical(record.source_facet_key) ||
          !facet_key_is_canonical(record.terminal_facet_key) ||
          record.source_facet_key.point_count != order_ ||
          record.terminal_facet_key.point_count != order_ ||
          record.terminal_binding_witness.external_authority_id !=
              bundle.identity.session_authority_id ||
          record.terminal_binding_witness.replay_token == 0U) {
        return false;
      }
      const auto binding = std::find_if(
          bindings.begin(),
          bindings.end(),
          [&](const ExactDirectSparseFacetBinding& candidate) {
            return candidate.key == record.source_facet_key &&
                   candidate.component_handle ==
                       record.terminal_component_handle;
          });
      if (binding == bindings.end() ||
          binding->witness.external_authority_id !=
              bundle.identity.session_authority_id ||
          binding->witness.replay_token == 0U) {
        return false;
      }
      const auto found = std::find_if(
          bundle.facet_resolutions.begin(),
          bundle.facet_resolutions.end(),
          [&](const ExactDirectFrozenUnifiedFacetResolution& resolution) {
            return resolution.facet_token_index ==
                   record.facet_token_index;
          });
      if (found == bundle.facet_resolutions.end() ||
          found->token.kind != record.token_kind ||
          found->token.token_id != record.terminal_component_handle ||
          found->prior_root_id != record.prior_root_id) {
        return false;
      }
    }
    return true;
  }

  // The compact record does not merely retain the terminal component id: it
  // replays the exact terminal key with the binding witness returned by the
  // closure against the still-immutable pre-batch locator.  Commit performs
  // this check before either staging or applying a locator batch.
  [[nodiscard]] bool attests_prebatch_locator(
      const ExactDirectMorseUnifiedResidentAuthorityBundle& bundle,
      std::span<const ExactDirectSparseFacetBinding> bindings,
      const ExactDirectSparsePositiveFacetLocator& locator,
      const ExactDirectSparsePositiveFacetProbeBudget& probe_budget) const
      noexcept {
    if (!attests(bundle, bindings) ||
        locator.snapshot_stamp() != source_stamp_) {
      return false;
    }
    for (const auto& record : records_) {
      const auto terminal_probe = locator.probe_positive_facet(
          record.terminal_facet_key,
          record.terminal_binding_witness,
          probe_budget);
      if (!terminal_probe.certified_positive_hit() ||
          !terminal_probe.component_handle_present ||
          !terminal_probe.source_binding_witness_present ||
          terminal_probe.component_handle !=
              record.terminal_component_handle ||
          terminal_probe.source_binding_witness !=
              record.terminal_binding_witness ||
          locator.snapshot_stamp() != source_stamp_) {
        return false;
      }
    }
    return true;
  }

 private:
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp source_stamp_{};
  std::size_t source_batch_index_{};
  exact::ExactLevel squared_level_{};
  std::size_t order_{};
  std::vector<ResidentStrictFacetCarrierRecord> records_;
  std::size_t node_count_{};
  std::size_t edge_count_{};
  std::size_t snapshot_check_count_{};
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

[[nodiscard]] const ResidentComponentPatch* find_component_patch(
    const ResidentStateDelta& delta,
    std::size_t handle) noexcept {
  const auto found = std::lower_bound(
      delta.component_patches.begin(),
      delta.component_patches.end(),
      handle,
      [](const ResidentComponentPatch& patch, std::size_t value) {
        return patch.handle < value;
      });
  return found != delta.component_patches.end() && found->handle == handle
             ? &*found
             : nullptr;
}

enum class ResidentLatentPatchPolicy : std::uint8_t {
  preserve_current_points,
  omit_points_before_overwrite,
};

[[nodiscard]] bool account_component_latent_materialization(
    ResidentStateDelta& delta,
    std::size_t point_count,
    const ExactDirectMorseUnifiedResidentSparseDeltaBudget& budget) noexcept {
  std::size_t next_count = 0U;
  if (add_overflow(
          delta.component_latent_point_materialization_count,
          point_count,
          next_count) ||
      next_count >
          budget.maximum_component_patch_latent_point_reference_count) {
    return false;
  }
  delta.component_latent_point_materialization_count = next_count;
  return true;
}

[[nodiscard]] ResidentComponentPatch* ensure_component_patch(
    const ResidentState& state,
    ResidentStateDelta& delta,
    std::size_t handle,
    const ExactDirectMorseUnifiedResidentSparseDeltaBudget& budget,
    ResidentLatentPatchPolicy latent_policy) {
  auto found = std::lower_bound(
      delta.component_patches.begin(),
      delta.component_patches.end(),
      handle,
      [](const ResidentComponentPatch& patch, std::size_t value) {
        return patch.handle < value;
      });
  if (found != delta.component_patches.end() && found->handle == handle) {
    return &*found;
  }
  if (handle >= state.components.size() ||
      delta.component_patches.size() >= budget.maximum_component_patch_count) {
    return nullptr;
  }
  const auto& current = state.components[handle];
  const bool preserve_latent =
      latent_policy == ResidentLatentPatchPolicy::preserve_current_points;
  if (preserve_latent &&
      !account_component_latent_materialization(
          delta, current.latent_point_coverage.size(), budget)) {
    return nullptr;
  }
  ExactDirectMorseUnifiedResidentComponentState next{
      current.component_handle,
      current.parent_handle,
      current.root_id,
      {},
      current.active};
  if (preserve_latent) {
    next.latent_point_coverage = current.latent_point_coverage;
  }
  found = delta.component_patches.insert(
      found, ResidentComponentPatch{handle, std::move(next)});
  return &*found;
}

[[nodiscard]] bool assign_component_latent_points(
    ResidentComponentPatch& patch,
    std::span<const PointId> points,
    ResidentStateDelta& delta,
    const ExactDirectMorseUnifiedResidentSparseDeltaBudget& budget) {
  if (!account_component_latent_materialization(
          delta, points.size(), budget)) {
    return false;
  }
  patch.next.latent_point_coverage.assign(points.begin(), points.end());
  return true;
}

void release_component_latent_points(
    ExactDirectMorseUnifiedResidentComponentState& component) noexcept {
  std::vector<PointId>{}.swap(component.latent_point_coverage);
}

[[nodiscard]] const ExactDirectMorseUnifiedResidentComponentState&
component_after_delta(
    const ResidentState& state,
    const ResidentStateDelta& delta,
    std::size_t handle) {
  if (handle >= state.components.size()) {
    throw std::logic_error("resident component handle out of range");
  }
  if (const auto* patch = find_component_patch(delta, handle);
      patch != nullptr) {
    return patch->next;
  }
  return state.components[handle];
}

[[nodiscard]] std::size_t find_component(
    const ResidentState& state,
    const ResidentStateDelta& delta,
    std::size_t handle) {
  if (handle >= state.components.size()) {
    throw std::logic_error("resident component handle out of range");
  }
  std::size_t hops = 0U;
  while (component_after_delta(state, delta, handle).parent_handle != handle) {
    handle = component_after_delta(state, delta, handle).parent_handle;
    if (handle >= state.components.size() ||
        ++hops > state.components.size()) {
      throw std::logic_error("resident sparse component overlay cycle");
    }
  }
  return handle;
}

[[nodiscard]] std::optional<std::size_t> root_index(
    const ResidentState& state,
    ExactFrozenIncidencePriorRootId root_id) noexcept {
  if (root_id == 0U ||
      root_id > std::numeric_limits<std::size_t>::max() ||
      root_id > state.roots.size()) {
    return std::nullopt;
  }
  const std::size_t index = static_cast<std::size_t>(root_id - 1U);
  if (state.roots[index].root_id != root_id) {
    return std::nullopt;
  }
  return index;
}

[[nodiscard]] ExactDirectMorseUnifiedResidentRootCoverage* find_root(
    ResidentState& state,
    ExactFrozenIncidencePriorRootId root_id) noexcept {
  const auto index = root_index(state, root_id);
  return index.has_value() ? &state.roots[*index] : nullptr;
}

[[nodiscard]] const ResidentRootReplacement* find_root_replacement(
    const ResidentStateDelta& delta,
    std::size_t index) noexcept {
  const auto found = std::lower_bound(
      delta.root_replacements.begin(),
      delta.root_replacements.end(),
      index,
      [](const ResidentRootReplacement& patch, std::size_t value) {
        return patch.root_index < value;
      });
  return found != delta.root_replacements.end() &&
                 found->root_index == index
             ? &*found
             : nullptr;
}

[[nodiscard]] const std::vector<PointId>* root_points_after_delta(
    const ResidentState& state,
    const ResidentStateDelta& delta,
    ExactFrozenIncidencePriorRootId root_id) noexcept {
  if (root_id == 0U ||
      root_id - 1U > std::numeric_limits<std::size_t>::max()) {
    return nullptr;
  }
  const std::size_t index = static_cast<std::size_t>(root_id - 1U);
  if (index < state.roots.size()) {
    if (state.roots[index].root_id != root_id) {
      return nullptr;
    }
    if (const auto* replacement = find_root_replacement(delta, index);
        replacement != nullptr) {
      return &replacement->next_point_ids;
    }
    return &state.roots[index].point_ids;
  }
  const std::size_t local = index - state.roots.size();
  if (local >= delta.new_roots.size() ||
      delta.new_roots[local].root_id != root_id) {
    return nullptr;
  }
  return &delta.new_roots[local].point_ids;
}

[[nodiscard]] bool set_root_replacement(
    const ResidentState& state,
    ResidentStateDelta& delta,
    ExactFrozenIncidencePriorRootId root_id,
    std::vector<PointId> next_points,
    std::size_t maximum_replacement_count) {
  const auto index = root_index(state, root_id);
  if (!index.has_value()) {
    return false;
  }
  auto found = std::lower_bound(
      delta.root_replacements.begin(),
      delta.root_replacements.end(),
      *index,
      [](const ResidentRootReplacement& patch, std::size_t value) {
        return patch.root_index < value;
      });
  if (found != delta.root_replacements.end() &&
      found->root_index == *index) {
    found->next_point_ids = std::move(next_points);
    return true;
  }
  if (delta.root_replacements.size() >= maximum_replacement_count) {
    return false;
  }
  delta.root_replacements.insert(
      found, ResidentRootReplacement{*index, root_id, std::move(next_points)});
  return true;
}

void canonicalize_points(std::vector<PointId>& points) {
  std::sort(points.begin(), points.end());
  points.erase(std::unique(points.begin(), points.end()), points.end());
}

[[nodiscard]] bool checked_next_witness(
    ResidentStateDelta& delta,
    std::uint64_t authority_id,
    ExactDirectSparseFacetWitness& witness) noexcept {
  if (delta.next_replay_token == 0U || authority_id == 0U) {
    return false;
  }
  witness = {authority_id, delta.next_replay_token};
  if (delta.next_replay_token ==
      std::numeric_limits<std::uint64_t>::max()) {
    delta.next_replay_token = 0U;
  } else {
    ++delta.next_replay_token;
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
             budget.maximum_fresh_facet_miniball_support_enumeration_count &&
         bundle.counters.normalized_facet_observation_scratch_entry_peak <=
             budget.maximum_normalized_facet_observation_scratch_count &&
         bundle.counters.normalized_strict_facet_scratch_entry_peak <=
             budget.maximum_normalized_strict_facet_scratch_count &&
         bundle.counters
                 .normalized_strict_facet_simultaneous_scratch_entry_peak <=
             budget
                 .maximum_normalized_strict_facet_simultaneous_scratch_entry_count;
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

[[nodiscard]] bool facet_key_is_canonical(
    const ExactDirectSparseFacetKey& key) noexcept {
  if (key.point_count == 0U ||
      key.point_count > key.point_ids.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    if ((index != 0U &&
         key.point_ids[index - 1U] >= key.point_ids[index]) ||
        static_cast<std::size_t>(key.point_ids[index]) ==
            std::numeric_limits<std::size_t>::max()) {
      return false;
    }
  }
  return std::all_of(
      key.point_ids.begin() +
          static_cast<std::ptrdiff_t>(key.point_count),
      key.point_ids.end(),
      [](PointId point_id) { return point_id == PointId{}; });
}

[[nodiscard]] bool facet_key_less(
    const ExactDirectSparseFacetKey& left,
    const ExactDirectSparseFacetKey& right) noexcept {
  if (left.point_count != right.point_count) {
    return left.point_count < right.point_count;
  }
  return std::lexicographical_compare(
      left.point_ids.begin(),
      left.point_ids.begin() +
          static_cast<std::ptrdiff_t>(left.point_count),
      right.point_ids.begin(),
      right.point_ids.begin() +
          static_cast<std::ptrdiff_t>(right.point_count));
}

struct NormalizedCofaceScratch {
  std::array<PointId, 11U> point_ids{};
  std::size_t point_count{};
  std::size_t occurrence_offset{};
};

struct NormalizedFacetOccurrenceScratch {
  std::size_t source_coface_index{};
  std::size_t removed_union_point_index{};
  PointId removed_point_id{};
  ExactDirectSparseFacetKey facet_key{};
};

struct NormalizedResidentCompatibilityBuild {
  ExactDirectSparseUnifiedLevelPlanResult plan{};
  std::size_t reconstructed_coface_count{};
  std::size_t transient_k_plus_one_key_count{};
  bool every_source_reference_consumed_once{false};
  bool every_coface_reconstructed_transiently{false};
  bool no_k_plus_one_key_persisted{false};
  bool no_successive_star_materialized{false};
  bool no_global_gamma_or_higher_delaunay_materialized{false};
  bool certified{false};
};

struct NormalizedResidentAdapterPreflight {
  std::size_t coface_facet_occurrence_count{};
  std::size_t coface_occurrence_facet_key_point_count{};
  std::size_t direct_coface_count{};
  std::size_t residual_coface_count{};
  std::size_t maximum_distinct_facet_count{};
  std::size_t maximum_facet_key_point_count{};
  std::size_t maximum_direct_reference_count{};
  std::size_t maximum_batch_coface_index_scratch_count{};
  std::size_t conservative_logical_storage_entry_count{};
  std::size_t simultaneous_adapter_entry_count{};
  bool certified{false};
};

[[nodiscard]] constexpr bool facet_token_count_fits_token_id(
    std::size_t token_count) noexcept {
  if (token_count == 0U) {
    return true;
  }
  if constexpr (
      sizeof(std::size_t) > sizeof(ExactFrozenIncidenceTokenId)) {
    return token_count - 1U <=
           static_cast<std::size_t>(
               std::numeric_limits<ExactFrozenIncidenceTokenId>::max());
  }
  return true;
}

[[nodiscard]] bool normalized_adapter_budget_accepts_source_shape(
    const ExactDirectSparseGatewayCandidateJournalResult& gateway,
    const ExactDirectNormalizedH0SourcePlanResult& source,
    const ExactDirectNormalizedH0ResidentAdapterBudget& budget,
    const ExactDirectMorseUnifiedResidentSessionBudget& session_budget,
    NormalizedResidentAdapterPreflight& preflight) noexcept {
  preflight = {};
  if (source.direct_birth_references.size() >
          budget.maximum_source_direct_birth_scan_count ||
      source.cofaces.size() > budget.maximum_source_coface_scan_count ||
      source.batches.size() > budget.maximum_source_batch_scan_count ||
      source.cofaces.size() >
          budget.maximum_coface_occurrence_offset_scratch_count ||
      source.batches.size() >
          session_budget.locator.maximum_committed_batch_count ||
      source.gateway_candidate_references.size() >
          budget.maximum_logical_storage_entry_count ||
      source.batch_coface_references.size() != source.cofaces.size() ||
      source.required_source_role_scan_count >
          source.requested_budget.maximum_source_role_scan_count ||
      source.required_source_gateway_token_scan_count >
          source.requested_budget.maximum_source_gateway_token_scan_count ||
      source.required_source_gateway_candidate_scan_count >
          source.requested_budget
              .maximum_source_gateway_candidate_scan_count ||
      source.required_distinct_coface_count >
          source.requested_budget.maximum_distinct_coface_count ||
      source.required_direct_coface_count >
          source.requested_budget.maximum_direct_coface_count ||
      source.required_residual_coface_count >
          source.requested_budget.maximum_residual_coface_count ||
      source.required_batch_count >
          source.requested_budget.maximum_batch_count ||
      source.required_direct_birth_reference_count >
          source.requested_budget.maximum_direct_birth_reference_count ||
      source.required_batch_coface_reference_count >
          source.requested_budget.maximum_batch_coface_reference_count ||
      source.logical_storage_entry_count >
          source.requested_budget.maximum_logical_storage_entry_count ||
      source.required_source_gateway_token_scan_count !=
          gateway.facet_tokens.size() ||
      source.required_source_gateway_candidate_scan_count !=
          gateway.gateway_candidates.size() ||
      source.source_gateway_token_count != gateway.facet_tokens.size() ||
      source.source_gateway_candidate_count !=
          gateway.gateway_candidates.size() ||
      source.required_higher_order_gateway_candidate_count !=
          source.gateway_candidate_references.size() ||
      source.required_distinct_coface_count != source.cofaces.size() ||
      source.required_direct_birth_reference_count !=
          source.direct_birth_references.size() ||
      source.required_batch_coface_reference_count !=
          source.batch_coface_references.size() ||
      source.required_batch_count != source.batches.size()) {
    return false;
  }

  std::size_t gateway_candidate_partition_count = 0U;
  if (add_overflow(
          source.excluded_order_one_gateway_candidate_count,
          source.required_higher_order_gateway_candidate_count,
          gateway_candidate_partition_count) ||
      gateway_candidate_partition_count !=
          source.source_gateway_candidate_count) {
    return false;
  }

  std::size_t source_support_point_count = 0U;
  for (std::size_t index = 0U; index < source.cofaces.size(); ++index) {
    const auto& coface = source.cofaces[index];
    std::size_t deletion_count = 0U;
    std::size_t occurrence_point_count = 0U;
    if (coface.coface_index != index || coface.order == 0U ||
        coface.order > direct_sparse_positive_facet_maximum_point_count ||
        coface.positive_support_point_count >
            coface.positive_support_point_ids.size() ||
        add_overflow(coface.order, 1U, deletion_count) ||
        add_overflow(
            preflight.coface_facet_occurrence_count,
            deletion_count,
            preflight.coface_facet_occurrence_count) ||
        preflight.coface_facet_occurrence_count >
            budget.maximum_coface_facet_reference_count ||
        deletion_count >
            std::numeric_limits<std::size_t>::max() / coface.order) {
      return false;
    }
    occurrence_point_count = deletion_count * coface.order;
    if (add_overflow(
            preflight.coface_occurrence_facet_key_point_count,
            occurrence_point_count,
            preflight.coface_occurrence_facet_key_point_count) ||
        add_overflow(
            source_support_point_count,
            coface.positive_support_point_count,
            source_support_point_count)) {
      return false;
    }
    switch (coface.kind) {
      case ExactDirectNormalizedH0SourceCofaceKind::direct_saddle:
        if (add_overflow(
                preflight.direct_coface_count,
                1U,
                preflight.direct_coface_count)) {
          return false;
        }
        break;
      case ExactDirectNormalizedH0SourceCofaceKind::
          first_incidence_residual:
        if (add_overflow(
                preflight.residual_coface_count,
                1U,
                preflight.residual_coface_count)) {
          return false;
        }
        break;
      default:
        return false;
    }
  }
  if (preflight.direct_coface_count !=
          source.required_direct_coface_count ||
      preflight.residual_coface_count !=
          source.required_residual_coface_count ||
      preflight.direct_coface_count >
          std::numeric_limits<std::size_t>::max() -
              preflight.residual_coface_count ||
      preflight.direct_coface_count + preflight.residual_coface_count !=
          source.cofaces.size()) {
    return false;
  }

  std::size_t higher_order_direct_role_count = 0U;
  std::size_t source_role_partition_count = 0U;
  if (add_overflow(
          source.direct_birth_references.size(),
          preflight.direct_coface_count,
          higher_order_direct_role_count) ||
      add_overflow(
          source.excluded_order_one_role_count,
          higher_order_direct_role_count,
          source_role_partition_count) ||
      source_role_partition_count != source.required_source_role_scan_count) {
    return false;
  }

  preflight.maximum_facet_key_point_count =
      preflight.coface_occurrence_facet_key_point_count;
  for (std::size_t index = 0U;
       index < source.direct_birth_references.size();
       ++index) {
    const auto& birth = source.direct_birth_references[index];
    if (birth.direct_birth_reference_index != index ||
        !facet_key_is_canonical(birth.birth_facet_key) ||
        add_overflow(
            preflight.maximum_facet_key_point_count,
            birth.birth_facet_key.point_count,
            preflight.maximum_facet_key_point_count)) {
      return false;
    }
  }
  if (add_overflow(
          preflight.coface_facet_occurrence_count,
          source.direct_birth_references.size(),
          preflight.maximum_distinct_facet_count) ||
      add_overflow(
          source.direct_birth_references.size(),
          preflight.direct_coface_count,
          preflight.maximum_direct_reference_count) ||
      preflight.maximum_distinct_facet_count >
          budget.maximum_distinct_facet_count ||
      preflight.maximum_distinct_facet_count >
          budget.maximum_distinct_facet_scratch_count ||
      preflight.coface_facet_occurrence_count >
          budget.maximum_facet_occurrence_scratch_count ||
      preflight.maximum_facet_key_point_count >
          budget.maximum_facet_key_point_count ||
      preflight.maximum_distinct_facet_count >
          session_budget.locator.maximum_component_handle_count ||
      !facet_token_count_fits_token_id(
          preflight.maximum_distinct_facet_count)) {
    return false;
  }

  std::size_t birth_cursor = 0U;
  std::size_t coface_reference_cursor = 0U;
  for (std::size_t index = 0U; index < source.batches.size(); ++index) {
    const auto& batch = source.batches[index];
    if (batch.batch_index != index ||
        batch.direct_birth_reference_offset != birth_cursor ||
        batch.direct_birth_reference_count >
            source.direct_birth_references.size() - birth_cursor ||
        batch.coface_reference_offset != coface_reference_cursor ||
        batch.coface_reference_count >
            source.batch_coface_references.size() -
                coface_reference_cursor ||
        add_overflow(
            birth_cursor,
            batch.direct_birth_reference_count,
            birth_cursor) ||
        add_overflow(
            coface_reference_cursor,
            batch.coface_reference_count,
            coface_reference_cursor)) {
      return false;
    }
    preflight.maximum_batch_coface_index_scratch_count = std::max(
        preflight.maximum_batch_coface_index_scratch_count,
        batch.coface_reference_count);
    if (preflight.maximum_batch_coface_index_scratch_count >
        budget.maximum_batch_coface_index_scratch_count) {
      return false;
    }
  }
  if (birth_cursor != source.direct_birth_references.size() ||
      coface_reference_cursor != source.batch_coface_references.size()) {
    return false;
  }

  std::size_t source_logical = 0U;
  const std::size_t source_logical_increments[]{
      source.gateway_candidate_references.size(),
      source.cofaces.size(),
      source_support_point_count,
      source.direct_birth_references.size(),
      source.batch_coface_references.size(),
      source.batches.size(),
  };
  for (const std::size_t increment : source_logical_increments) {
    if (add_overflow(source_logical, increment, source_logical)) {
      return false;
    }
  }
  if (source_logical != source.logical_storage_entry_count) {
    return false;
  }

  const std::size_t compatibility_logical_increments[]{
      preflight.maximum_distinct_facet_count,
      preflight.maximum_facet_key_point_count,
      preflight.maximum_direct_reference_count,
      preflight.residual_coface_count,
      preflight.coface_facet_occurrence_count,
      source.batches.size(),
  };
  for (const std::size_t increment : compatibility_logical_increments) {
    if (add_overflow(
            preflight.conservative_logical_storage_entry_count,
            increment,
            preflight.conservative_logical_storage_entry_count)) {
      return false;
    }
  }
  if (preflight.conservative_logical_storage_entry_count >
      budget.maximum_logical_storage_entry_count) {
    return false;
  }

  // Peak adapter coexistence: the durable plan reserves all five arenas while
  // coface offsets, occurrence records and keys, raw distinct facets and keys,
  // plus the largest per-batch coface-index sort scratch are still alive.
  // Counting this conservative peak keeps the candidate adapter explicitly
  // bounded; it is not an architecture for massive-cloud ingestion.
  preflight.simultaneous_adapter_entry_count =
      preflight.conservative_logical_storage_entry_count;
  const std::size_t simultaneous_scratch_increments[]{
      source.cofaces.size(),
      preflight.coface_facet_occurrence_count,
      preflight.coface_occurrence_facet_key_point_count,
      preflight.maximum_distinct_facet_count,
      preflight.maximum_facet_key_point_count,
      preflight.maximum_batch_coface_index_scratch_count,
  };
  for (const std::size_t increment : simultaneous_scratch_increments) {
    if (add_overflow(
            preflight.simultaneous_adapter_entry_count,
            increment,
            preflight.simultaneous_adapter_entry_count)) {
      return false;
    }
  }
  if (preflight.simultaneous_adapter_entry_count >
      budget.maximum_simultaneous_adapter_entry_count) {
    return false;
  }
  preflight.certified = true;
  return true;
}

[[nodiscard]] bool reconstruct_normalized_coface(
    const ExactDirectSparseGatewayCandidateJournalResult& gateway,
    const ExactDirectNormalizedH0SourceCoface& source,
    NormalizedCofaceScratch& destination) noexcept {
  if (source.owner_source_gateway_token_index >=
      gateway.facet_tokens.size()) {
    return false;
  }
  const auto& owner =
      gateway.facet_tokens[source.owner_source_gateway_token_index];
  const auto& core = owner.source_facet_key;
  if (owner.facet_token_index !=
          source.owner_source_gateway_token_index ||
      !facet_key_is_canonical(core) || core.point_count != source.order ||
      source.order + 1U > destination.point_ids.size() ||
      std::binary_search(
          core.point_ids.begin(),
          core.point_ids.begin() +
              static_cast<std::ptrdiff_t>(core.point_count),
          source.added_point_id)) {
    return false;
  }
  const auto insertion = std::lower_bound(
      core.point_ids.begin(),
      core.point_ids.begin() +
          static_cast<std::ptrdiff_t>(core.point_count),
      source.added_point_id);
  const std::size_t insertion_index = static_cast<std::size_t>(
      insertion - core.point_ids.begin());
  std::copy_n(
      core.point_ids.begin(),
      insertion_index,
      destination.point_ids.begin());
  destination.point_ids[insertion_index] = source.added_point_id;
  std::copy(
      insertion,
      core.point_ids.begin() +
          static_cast<std::ptrdiff_t>(core.point_count),
      destination.point_ids.begin() +
          static_cast<std::ptrdiff_t>(insertion_index + 1U));
  destination.point_count = core.point_count + 1U;
  return destination.point_count == source.order + 1U;
}

[[nodiscard]] ExactDirectSparseFacetKey delete_normalized_coface_point(
    const NormalizedCofaceScratch& coface,
    std::size_t removed_index) {
  if (removed_index >= coface.point_count || coface.point_count <= 1U ||
      coface.point_count - 1U >
          direct_sparse_positive_facet_maximum_point_count) {
    throw std::logic_error("invalid normalized coface deletion");
  }
  ExactDirectSparseFacetKey facet;
  facet.point_count = coface.point_count - 1U;
  std::size_t cursor = 0U;
  for (std::size_t index = 0U; index < coface.point_count; ++index) {
    if (index != removed_index) {
      facet.point_ids[cursor++] = coface.point_ids[index];
    }
  }
  return facet;
}

[[nodiscard]] std::optional<std::size_t> find_facet_token_index(
    const std::vector<ExactDirectSparseFacetKey>& facets,
    const ExactDirectSparseFacetKey& key) noexcept {
  const auto found = std::lower_bound(
      facets.begin(), facets.end(), key, facet_key_less);
  if (found == facets.end() || *found != key) {
    return std::nullopt;
  }
  return static_cast<std::size_t>(found - facets.begin());
}

[[nodiscard]] NormalizedResidentCompatibilityBuild
build_normalized_resident_compatibility_plan(
    const ExactDirectSparseGatewayCandidateJournalResult& gateway,
    const ExactDirectNormalizedH0SourcePlanResult& source,
    const ExactDirectNormalizedH0ResidentAdapterBudget& budget,
    const NormalizedResidentAdapterPreflight& preflight) {
  NormalizedResidentCompatibilityBuild output;
  if (!preflight.certified) {
    return output;
  }

  std::vector<std::size_t> coface_occurrence_offsets(
      source.cofaces.size());
  std::vector<NormalizedFacetOccurrenceScratch> occurrences;
  std::vector<ExactDirectSparseFacetKey> distinct_facets;
  occurrences.reserve(preflight.coface_facet_occurrence_count);
  distinct_facets.reserve(preflight.maximum_distinct_facet_count);

  for (const auto& birth : source.direct_birth_references) {
    if (birth.direct_birth_reference_index >=
            source.direct_birth_references.size() ||
        !facet_key_is_canonical(birth.birth_facet_key)) {
      return output;
    }
    distinct_facets.push_back(birth.birth_facet_key);
  }
  for (std::size_t index = 0U; index < source.cofaces.size(); ++index) {
    NormalizedCofaceScratch reconstructed;
    reconstructed.occurrence_offset = occurrences.size();
    coface_occurrence_offsets[index] = reconstructed.occurrence_offset;
    if (!reconstruct_normalized_coface(
            gateway, source.cofaces[index], reconstructed)) {
      return output;
    }
    ++output.reconstructed_coface_count;
    ++output.transient_k_plus_one_key_count;
    for (std::size_t removed = 0U;
         removed < reconstructed.point_count;
         ++removed) {
      auto facet = delete_normalized_coface_point(reconstructed, removed);
      if (!facet_key_is_canonical(facet)) {
        return output;
      }
      distinct_facets.push_back(facet);
      occurrences.push_back(
          {index,
           removed,
           reconstructed.point_ids[removed],
           std::move(facet)});
    }
  }
  if (occurrences.size() != preflight.coface_facet_occurrence_count) {
    return output;
  }
  std::sort(distinct_facets.begin(), distinct_facets.end(), facet_key_less);
  distinct_facets.erase(
      std::unique(distinct_facets.begin(), distinct_facets.end()),
      distinct_facets.end());
  if (distinct_facets.size() > budget.maximum_distinct_facet_count) {
    return output;
  }
  std::size_t facet_point_count = 0U;
  for (const auto& facet : distinct_facets) {
    if (add_overflow(
            facet_point_count, facet.point_count, facet_point_count) ||
        facet_point_count > budget.maximum_facet_key_point_count) {
      return output;
    }
  }

  auto& plan = output.plan;
  plan.point_count = source.point_count;
  plan.source_event_projection_count = source.source_event_projection_count;
  plan.source_role_record_count = source.source_role_record_count;
  plan.source_incidence_family_count = source.source_incidence_family_count;
  plan.required_source_role_scan_count =
      source.required_source_role_scan_count;
  plan.required_higher_order_direct_role_count =
      preflight.maximum_direct_reference_count;
  plan.excluded_order_one_role_count = source.excluded_order_one_role_count;
  plan.required_direct_birth_reference_count =
      source.direct_birth_references.size();
  plan.required_direct_saddle_reference_count =
      preflight.direct_coface_count;
  plan.required_source_family_scan_count =
      source.source_incidence_family_count;
  plan.required_higher_order_saddle_family_count =
      preflight.direct_coface_count;
  plan.required_coface_deletion_reference_count = occurrences.size();
  plan.required_distinct_facet_token_count = distinct_facets.size();
  plan.required_facet_key_point_count = facet_point_count;
  plan.required_batch_count = source.batches.size();
  plan.source_pair_canonical_cloud_digest =
      source.source_pair_canonical_cloud_digest;
  plan.source_higher_canonical_cloud_digest =
      source.source_higher_canonical_cloud_digest;
  plan.source_pair_semantic_digest = source.source_pair_semantic_digest;
  plan.source_higher_semantic_digest = source.source_higher_semantic_digest;
  plan.facet_tokens.reserve(distinct_facets.size());
  for (std::size_t index = 0U; index < distinct_facets.size(); ++index) {
    plan.facet_tokens.push_back(
        {index, distinct_facets[index], std::nullopt, 0U, 0U});
  }
  plan.direct_references.reserve(
      preflight.maximum_direct_reference_count);
  plan.residual_references.reserve(preflight.residual_coface_count);
  plan.coface_facet_references.reserve(occurrences.size());
  plan.batches.reserve(source.batches.size());

  for (std::size_t batch_index = 0U;
       batch_index < source.batches.size();
       ++batch_index) {
    const auto& source_batch = source.batches[batch_index];
    if (source_batch.batch_index != batch_index ||
        source_batch.direct_birth_reference_offset >
            source.direct_birth_references.size() ||
        source_batch.direct_birth_reference_count >
            source.direct_birth_references.size() -
                source_batch.direct_birth_reference_offset ||
        source_batch.coface_reference_offset >
            source.batch_coface_references.size() ||
        source_batch.coface_reference_count >
            source.batch_coface_references.size() -
                source_batch.coface_reference_offset) {
      return {};
    }
    const std::size_t direct_offset = plan.direct_references.size();
    const std::size_t residual_offset = plan.residual_references.size();
    const std::size_t facet_offset = plan.coface_facet_references.size();

    for (std::size_t local = 0U;
         local < source_batch.direct_birth_reference_count;
         ++local) {
      const auto& birth = source.direct_birth_references
          [source_batch.direct_birth_reference_offset + local];
      const auto facet_index =
          find_facet_token_index(distinct_facets, birth.birth_facet_key);
      if (!facet_index.has_value() ||
          birth.direct_birth_reference_index !=
              source_batch.direct_birth_reference_offset + local ||
          birth.birth_facet_key.point_count != source_batch.order) {
        return {};
      }
      ++plan.facet_tokens[*facet_index].direct_birth_reference_count;
      plan.direct_references.push_back(
          {plan.direct_references.size(),
           birth.source_role_record_index,
           birth.source_event_projection_index,
           ExactDirectMorseH0Role::birth,
           std::nullopt,
           std::nullopt,
           *facet_index});
    }

    std::vector<std::size_t> batch_coface_indices;
    batch_coface_indices.reserve(source_batch.coface_reference_count);
    for (std::size_t local = 0U;
         local < source_batch.coface_reference_count;
         ++local) {
      const auto& reference = source.batch_coface_references
          [source_batch.coface_reference_offset + local];
      if (reference.batch_coface_reference_index !=
              source_batch.coface_reference_offset + local ||
          reference.source_coface_index >= source.cofaces.size()) {
        return {};
      }
      batch_coface_indices.push_back(reference.source_coface_index);
    }
    std::sort(batch_coface_indices.begin(), batch_coface_indices.end());
    if (std::adjacent_find(
            batch_coface_indices.begin(), batch_coface_indices.end()) !=
        batch_coface_indices.end()) {
      return {};
    }

    for (const std::size_t coface_index : batch_coface_indices) {
      const auto& coface = source.cofaces[coface_index];
      if (coface.order != source_batch.order ||
          coface.squared_level != source_batch.squared_level) {
        return {};
      }
      if (coface.kind ==
          ExactDirectNormalizedH0SourceCofaceKind::direct_saddle) {
        if (!coface.source_role_record_index.has_value() ||
            !coface.source_event_projection_index.has_value() ||
            !coface.source_incidence_family_index.has_value()) {
          return {};
        }
        plan.direct_references.push_back(
            {plan.direct_references.size(),
             *coface.source_role_record_index,
             *coface.source_event_projection_index,
             ExactDirectMorseH0Role::saddle,
             coface.source_incidence_family_index,
             coface_index,
             std::nullopt});
      } else {
        plan.residual_references.push_back(
            {plan.residual_references.size(), coface_index});
      }
      for (std::size_t removed = 0U;
           removed < coface.order + 1U;
           ++removed) {
        const auto& occurrence = occurrences
            [coface_occurrence_offsets[coface_index] + removed];
        const auto facet_index = find_facet_token_index(
            distinct_facets, occurrence.facet_key);
        if (!facet_index.has_value() ||
            occurrence.source_coface_index != coface_index ||
            occurrence.removed_union_point_index != removed) {
          return {};
        }
        ++plan.facet_tokens[*facet_index]
              .coface_deletion_reference_count;
        plan.coface_facet_references.push_back(
            {plan.coface_facet_references.size(),
             coface_index,
             removed,
             occurrence.removed_point_id,
             *facet_index});
      }
    }
    plan.batches.push_back(
        {batch_index,
         source_batch.future_snapshot_index,
         source_batch.squared_level,
         source_batch.order,
         direct_offset,
         plan.direct_references.size() - direct_offset,
         residual_offset,
         plan.residual_references.size() - residual_offset,
         facet_offset,
         plan.coface_facet_references.size() - facet_offset});
  }

  plan.required_direct_reference_count = plan.direct_references.size();
  plan.required_residual_reference_count = plan.residual_references.size();
  std::size_t logical_storage = 0U;
  const std::size_t logical_increments[]{
      plan.facet_tokens.size(),
      facet_point_count,
      plan.direct_references.size(),
      plan.residual_references.size(),
      plan.coface_facet_references.size(),
      plan.batches.size(),
  };
  for (const std::size_t increment : logical_increments) {
    if (add_overflow(logical_storage, increment, logical_storage) ||
        logical_storage > budget.maximum_logical_storage_entry_count) {
      return {};
    }
  }
  if (plan.direct_references.size() !=
          preflight.maximum_direct_reference_count ||
      plan.residual_references.size() !=
          preflight.residual_coface_count ||
      plan.coface_facet_references.size() != occurrences.size()) {
    return {};
  }
  plan.logical_storage_entry_count = logical_storage;
  plan.order_one_roles_and_families_excluded_to_preserve_boruvka_authority =
      source.order_one_roles_excluded_to_preserve_boruvka_authority;
  plan.every_higher_order_direct_role_projected_once =
      source.every_higher_order_direct_birth_projected_once &&
      source.every_higher_order_direct_saddle_joined_once;
  plan.every_higher_order_saddle_role_joined_to_one_family =
      source.every_higher_order_direct_saddle_joined_once;
  plan.facet_tokens_canonical_and_deduplicated = true;
  plan.unique_batch_per_exact_level_and_order =
      source.unique_batch_per_exact_level_and_order;
  plan.direct_and_residual_same_level_order_share_one_future_snapshot = true;
  plan.batches_sorted_by_exact_level_then_order =
      source.batches_sorted_by_exact_level_then_order;
  plan.logical_storage_within_budget = true;
  plan.no_partial_scientific_payload_published = true;
  plan.no_k_plus_one_coface_key_persisted = true;
  plan.no_global_facet_or_coface_catalog_materialized = true;
  plan.reducer_snapshot_or_forest_mutated = false;
  plan.bounded_star_global_completeness_claimed = false;
  plan.public_status_claimed = false;
  plan.partial_refinement_only = true;
  // This compatibility cursor is deliberately not a certified
  // SuccessiveStar plan.  Its non-forgeable resident authority comes from the
  // freshly verified normalized source instead.
  plan.source_star_freshly_verified = false;
  plan.direct_star_cofaces_crosschecked_bijectively = false;
  plan.star_direct_cofaces_are_crosscheck_only_not_residual_references = false;
  plan.every_star_residual_coface_referenced_once = false;
  plan.every_star_coface_contributes_all_factorized_deletions = false;
  plan.decision = ExactDirectSparseUnifiedLevelPlanDecision::not_certified;
  plan.scope = ExactDirectSparseUnifiedLevelPlanScope::unspecified;

  output.every_source_reference_consumed_once = true;
  output.every_coface_reconstructed_transiently =
      output.reconstructed_coface_count == source.cofaces.size() &&
      output.transient_k_plus_one_key_count == source.cofaces.size();
  output.no_k_plus_one_key_persisted = plan.no_k_plus_one_coface_key_persisted;
  output.no_successive_star_materialized = true;
  output.no_global_gamma_or_higher_delaunay_materialized = true;
  output.certified =
      output.every_source_reference_consumed_once &&
      output.every_coface_reconstructed_transiently &&
      output.no_k_plus_one_key_persisted &&
      output.no_successive_star_materialized &&
      output.no_global_gamma_or_higher_delaunay_materialized;
  return output;
}

[[nodiscard]] bool finalize_resident_delta(
    const ResidentState& state,
    const ExactDirectMorseUnifiedResidentSessionBudget& budget,
    ResidentStateDelta& delta,
    ExactDirectMorseUnifiedResidentBatchCounters& counters,
    bool& sparse_budget_exhausted) noexcept {
  sparse_budget_exhausted = false;
  const auto& delta_budget = budget.sparse_delta;
  std::size_t final_component_latent_points =
      state.resident_component_latent_point_reference_count;
  std::size_t previous_component = 0U;
  bool first_component = true;
  for (const auto& patch : delta.component_patches) {
    if (patch.handle >= state.components.size() ||
        patch.next.component_handle != patch.handle ||
        (!first_component && patch.handle <= previous_component) ||
        state.components[patch.handle].latent_point_coverage.size() >
            final_component_latent_points) {
      return false;
    }
    final_component_latent_points -=
        state.components[patch.handle].latent_point_coverage.size();
    if (add_overflow(
            final_component_latent_points,
            patch.next.latent_point_coverage.size(),
            final_component_latent_points)) {
      return false;
    }
    first_component = false;
    previous_component = patch.handle;
  }

  std::size_t root_patch_points = 0U;
  std::size_t final_root_points =
      state.resident_root_point_reference_count;
  std::size_t previous_root_index = 0U;
  bool first_root = true;
  for (const auto& replacement : delta.root_replacements) {
    if (replacement.root_index >= state.roots.size() ||
        state.roots[replacement.root_index].root_id != replacement.root_id ||
        (!first_root && replacement.root_index <= previous_root_index) ||
        state.roots[replacement.root_index].point_ids.size() >
            final_root_points) {
      return false;
    }
    first_root = false;
    previous_root_index = replacement.root_index;
    final_root_points -=
        state.roots[replacement.root_index].point_ids.size();
    if (add_overflow(
            final_root_points,
            replacement.next_point_ids.size(),
            final_root_points) ||
        add_overflow(
            root_patch_points,
            replacement.next_point_ids.size(),
            root_patch_points)) {
      return false;
    }
  }
  for (std::size_t local = 0U; local < delta.new_roots.size(); ++local) {
    std::size_t root_number = 0U;
    if (add_overflow(state.roots.size(), local, root_number) ||
        add_overflow(root_number, 1U, root_number) ||
        root_number >
            std::numeric_limits<ExactFrozenIncidencePriorRootId>::max() ||
        delta.new_roots[local].root_id !=
            static_cast<ExactFrozenIncidencePriorRootId>(root_number) ||
        add_overflow(
            final_root_points,
            delta.new_roots[local].point_ids.size(),
            final_root_points) ||
        add_overflow(
            root_patch_points,
            delta.new_roots[local].point_ids.size(),
            root_patch_points)) {
      return false;
    }
  }

  std::size_t delta_group_children = 0U;
  std::size_t delta_group_points = 0U;
  for (std::size_t local = 0U;
       local < delta.new_group_records.size();
       ++local) {
    const auto& record = delta.new_group_records[local];
    std::size_t expected_group_record_index = 0U;
    if (add_overflow(
            state.group_records.size(),
            local,
            expected_group_record_index) ||
        record.group_record_index != expected_group_record_index ||
        add_overflow(
            delta_group_children,
            record.child_root_ids.size(),
            delta_group_children) ||
        add_overflow(
            delta_group_points,
            record.coverage_delta_points.size(),
            delta_group_points)) {
      return false;
    }
  }

  std::size_t final_root_count = 0U;
  std::size_t final_group_count = 0U;
  std::size_t final_group_children = 0U;
  std::size_t final_group_points = 0U;
  if (add_overflow(
          state.roots.size(), delta.new_roots.size(), final_root_count) ||
      add_overflow(
          state.group_records.size(),
          delta.new_group_records.size(),
          final_group_count) ||
      add_overflow(
          state.group_child_reference_count,
          delta_group_children,
          final_group_children) ||
      add_overflow(
          state.group_coverage_delta_point_reference_count,
          delta_group_points,
          final_group_points)) {
    return false;
  }

  if (delta.component_patches.size() >
          delta_budget.maximum_component_patch_count ||
      delta.component_latent_point_materialization_count >
          delta_budget.maximum_component_patch_latent_point_reference_count ||
      delta.root_replacements.size() >
          delta_budget.maximum_root_replacement_count ||
      delta.new_roots.size() > delta_budget.maximum_new_root_count ||
      root_patch_points >
          delta_budget.maximum_root_patch_point_reference_count ||
      delta.new_group_records.size() >
          delta_budget.maximum_group_append_count ||
      delta_group_children >
          delta_budget.maximum_group_child_reference_count ||
      delta_group_points >
          delta_budget.maximum_group_coverage_delta_point_reference_count) {
    sparse_budget_exhausted = true;
    return false;
  }
  if (final_root_count > budget.maximum_resident_root_count ||
      final_root_points >
          budget.maximum_resident_root_point_reference_count ||
      final_component_latent_points >
          budget.maximum_resident_component_latent_point_reference_count ||
      final_group_count > budget.maximum_group_record_count ||
      final_group_children > budget.maximum_group_child_reference_count ||
      final_group_points >
          budget.maximum_group_coverage_delta_point_reference_count) {
    return false;
  }

  delta.final_resident_root_point_reference_count = final_root_points;
  delta.final_resident_component_latent_point_reference_count =
      final_component_latent_points;
  delta.final_group_child_reference_count = final_group_children;
  delta.final_group_coverage_delta_point_reference_count = final_group_points;
  counters.resident_state_full_copy_count = 0U;
  counters.sparse_delta_component_patch_count =
      delta.component_patches.size();
  counters.sparse_delta_component_latent_point_reference_count =
      delta.component_latent_point_materialization_count;
  counters.sparse_delta_root_replacement_count =
      delta.root_replacements.size();
  counters.sparse_delta_new_root_count = delta.new_roots.size();
  counters.sparse_delta_root_point_reference_count = root_patch_points;
  counters.sparse_delta_group_append_count =
      delta.new_group_records.size();
  counters.sparse_delta_group_child_reference_count = delta_group_children;
  counters.sparse_delta_group_coverage_delta_point_reference_count =
      delta_group_points;
  return true;
}

[[nodiscard]] bool resident_delta_has_staging_capacity(
    const ResidentState& state,
    const ResidentStateDelta& delta) noexcept {
  return delta.new_roots.size() <=
             state.roots.max_size() - state.roots.size() &&
         delta.new_group_records.size() <=
             state.group_records.max_size() - state.group_records.size() &&
         std::all_of(
             delta.component_patches.begin(),
             delta.component_patches.end(),
             [&](const ResidentComponentPatch& patch) {
               return patch.handle < state.components.size();
             }) &&
         std::all_of(
             delta.root_replacements.begin(),
             delta.root_replacements.end(),
             [&](const ResidentRootReplacement& replacement) {
               return replacement.root_index < state.roots.size() &&
                      state.roots[replacement.root_index].root_id ==
                          replacement.root_id;
             });
}

template <class Value>
void reserve_geometrically_for_append(
    std::vector<Value>& arena,
    std::size_t append_count,
    std::size_t maximum_logical_count) {
  if (arena.size() > maximum_logical_count ||
      append_count > maximum_logical_count - arena.size()) {
    throw std::length_error("resident sparse append exceeds logical cap");
  }
  const std::size_t required = arena.size() + append_count;
  if (required <= arena.capacity()) {
    return;
  }
  std::size_t target = std::max<std::size_t>(arena.capacity(), 1U);
  while (target < required) {
    if (target > maximum_logical_count - target) {
      target = maximum_logical_count;
    } else {
      target *= 2U;
    }
    if (target < required && target == maximum_logical_count) {
      throw std::length_error("resident sparse append capacity overflow");
    }
  }
  arena.reserve(target);
}

class ResidentDeltaStage {
 public:
  ResidentDeltaStage(
      ResidentState& state,
      ResidentStateDelta& delta,
      std::size_t maximum_root_count,
      std::size_t maximum_group_count) noexcept
      : state_(&state),
        delta_(&delta),
        maximum_root_count_(maximum_root_count),
        maximum_group_count_(maximum_group_count),
        roots_before_(state.roots.size()),
        groups_before_(state.group_records.size()),
        old_next_root_id_(state.next_root_id),
        old_next_replay_token_(state.next_replay_token),
        old_root_point_count_(state.resident_root_point_reference_count),
        old_component_latent_point_count_(
            state.resident_component_latent_point_reference_count),
        old_group_child_count_(state.group_child_reference_count),
        old_group_delta_point_count_(
            state.group_coverage_delta_point_reference_count) {}

  ResidentDeltaStage(const ResidentDeltaStage&) = delete;
  ResidentDeltaStage& operator=(const ResidentDeltaStage&) = delete;

  ~ResidentDeltaStage() noexcept { rollback(); }

  void stage() {
    reserve_geometrically_for_append(
        state_->roots,
        delta_->new_roots.size(),
        maximum_root_count_);
    reserve_geometrically_for_append(
        state_->group_records,
        delta_->new_group_records.size(),
        maximum_group_count_);
    armed_ = true;
    for (auto& patch : delta_->component_patches) {
      std::swap(state_->components[patch.handle], patch.next);
      ++component_swap_count_;
    }
    for (auto& replacement : delta_->root_replacements) {
      state_->roots[replacement.root_index].point_ids.swap(
          replacement.next_point_ids);
      ++root_swap_count_;
    }
    for (auto& root : delta_->new_roots) {
      state_->roots.push_back(std::move(root));
      ++appended_root_count_;
    }
    for (auto& group : delta_->new_group_records) {
      state_->group_records.push_back(std::move(group));
      ++appended_group_count_;
    }
    state_->next_root_id = delta_->next_root_id;
    state_->next_replay_token = delta_->next_replay_token;
    state_->resident_root_point_reference_count =
        delta_->final_resident_root_point_reference_count;
    state_->resident_component_latent_point_reference_count =
        delta_->final_resident_component_latent_point_reference_count;
    state_->group_child_reference_count =
        delta_->final_group_child_reference_count;
    state_->group_coverage_delta_point_reference_count =
        delta_->final_group_coverage_delta_point_reference_count;
    scalars_staged_ = true;
  }

  void release() noexcept { armed_ = false; }

 private:
  void rollback() noexcept {
    if (!armed_) {
      return;
    }
    if (scalars_staged_) {
      state_->next_root_id = old_next_root_id_;
      state_->next_replay_token = old_next_replay_token_;
      state_->resident_root_point_reference_count = old_root_point_count_;
      state_->resident_component_latent_point_reference_count =
          old_component_latent_point_count_;
      state_->group_child_reference_count = old_group_child_count_;
      state_->group_coverage_delta_point_reference_count =
          old_group_delta_point_count_;
    }
    while (appended_group_count_ != 0U) {
      state_->group_records.pop_back();
      --appended_group_count_;
    }
    while (appended_root_count_ != 0U) {
      state_->roots.pop_back();
      --appended_root_count_;
    }
    if (state_->roots.size() != roots_before_ ||
        state_->group_records.size() != groups_before_) {
      std::terminate();
    }
    while (root_swap_count_ != 0U) {
      --root_swap_count_;
      auto& replacement = delta_->root_replacements[root_swap_count_];
      state_->roots[replacement.root_index].point_ids.swap(
          replacement.next_point_ids);
    }
    while (component_swap_count_ != 0U) {
      --component_swap_count_;
      auto& patch = delta_->component_patches[component_swap_count_];
      std::swap(state_->components[patch.handle], patch.next);
    }
    armed_ = false;
  }

  ResidentState* state_{};
  ResidentStateDelta* delta_{};
  std::size_t maximum_root_count_{};
  std::size_t maximum_group_count_{};
  std::size_t roots_before_{};
  std::size_t groups_before_{};
  ExactFrozenIncidencePriorRootId old_next_root_id_{};
  std::uint64_t old_next_replay_token_{};
  std::size_t old_root_point_count_{};
  std::size_t old_component_latent_point_count_{};
  std::size_t old_group_child_count_{};
  std::size_t old_group_delta_point_count_{};
  std::size_t component_swap_count_{};
  std::size_t root_swap_count_{};
  std::size_t appended_root_count_{};
  std::size_t appended_group_count_{};
  bool scalars_staged_{false};
  bool armed_{false};
};

enum class ResidentFacetObservationKind : std::uint8_t {
  positive_carrier,
  equal_facet,
  strict_facet_pending_closure,
};

struct ResidentFacetObservation {
  std::size_t facet_token_index{};
  ExactDirectSparseFacetKey facet_key{};
  ExactDirectSparseFacetWitness query_witness{};
  ResidentFacetObservationKind kind{
      ResidentFacetObservationKind::equal_facet};
  std::optional<ExactDirectSparseComponentHandle> component_handle;
  std::optional<ExactDirectSparseFacetWitness> terminal_binding_witness;
};

}  // namespace

namespace internal {

ExactDirectFrozenUnifiedImmutablePlanAuthorityInitialization
ExactDirectFrozenUnifiedImmutablePlanAuthorityFactory::
    create_from_normalized_direct_source(
        const spatial::MortonLbvhIndex& index,
        const spatial::CanonicalPointCloud& cloud,
        const ExactDirectSupportTerminalFacade& source_facade,
        const ExactDirectMorseEventJournalResult& source_journal,
        const ExactDirectSaddleArmSeedBudget& source_arm_budget,
        const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
        const ExactDirectClosedSaddleIncidenceBudget&
            source_incidence_budget,
        const ExactDirectClosedSaddleIncidenceJournalResult&
            source_incidence_journal,
        const ExactDirectSparseGatewayCandidateBudget& source_gateway_budget,
        spatial::LbvhTraversalOrder source_gateway_traversal_order,
        const ExactDirectSparseGatewayCandidateJournalResult& source_gateway,
        const ExactDirectNormalizedH0SourcePlanBudget& source_plan_budget,
        const ExactDirectNormalizedH0SourcePlanResult& source_plan,
        const ExactDirectNormalizedH0ResidentAdapterBudget& adapter_budget,
        const ExactDirectMorseUnifiedResidentSessionBudget& session_budget) {
  ExactDirectFrozenUnifiedImmutablePlanAuthorityInitialization output;
  NormalizedResidentAdapterPreflight preflight;
  if (!normalized_adapter_budget_accepts_source_shape(
          source_gateway,
          source_plan,
          adapter_budget,
          session_budget,
          preflight) ||
      !preflight.certified) {
    return output;
  }
  output.normalized_adapter_preflight_certified = true;
  output.normalized_preflight_coface_facet_occurrence_count =
      preflight.coface_facet_occurrence_count;
  output.source_plan_verification_count = 1U;
  const auto verification = verify_exact_direct_normalized_h0_source_plan(
      index,
      cloud,
      source_facade,
      source_journal,
      source_arm_budget,
      source_arm_journal,
      source_incidence_budget,
      source_incidence_journal,
      source_gateway_budget,
      source_gateway_traversal_order,
      source_gateway,
      source_plan_budget,
      source_plan);
  if (!verification.result_certified) {
    return output;
  }
  output.source_plan_freshly_verified_once = true;
  auto adapted = build_normalized_resident_compatibility_plan(
      source_gateway, source_plan, adapter_budget, preflight);
  if (!adapted.certified) {
    return output;
  }
  auto immutable_plan =
      std::make_unique<const ExactDirectSparseUnifiedLevelPlanResult>(
          std::move(adapted.plan));
  const bool compatibility_cursor_is_honestly_normalized =
      immutable_plan->point_count == source_plan.point_count &&
      immutable_plan->source_pair_canonical_cloud_digest ==
          source_plan.source_pair_canonical_cloud_digest &&
      immutable_plan->source_higher_canonical_cloud_digest ==
          source_plan.source_higher_canonical_cloud_digest &&
      immutable_plan->source_pair_semantic_digest ==
          source_plan.source_pair_semantic_digest &&
      immutable_plan->source_higher_semantic_digest ==
          source_plan.source_higher_semantic_digest &&
      !immutable_plan->source_star_freshly_verified &&
      !immutable_plan->direct_star_cofaces_crosschecked_bijectively &&
      !immutable_plan->bounded_star_global_completeness_claimed &&
      !immutable_plan->public_status_claimed &&
      immutable_plan->no_k_plus_one_coface_key_persisted &&
      immutable_plan->no_global_facet_or_coface_catalog_materialized &&
      immutable_plan->decision ==
          ExactDirectSparseUnifiedLevelPlanDecision::not_certified &&
      immutable_plan->scope ==
          ExactDirectSparseUnifiedLevelPlanScope::unspecified;
  if (!compatibility_cursor_is_honestly_normalized) {
    return output;
  }
  output.authority = ExactDirectFrozenUnifiedImmutablePlanAuthority{
      immutable_plan.get(),
      ExactDirectFrozenUnifiedImmutablePlanAuthorityKind::
          normalized_direct_h0_candidate_source};
  output.normalized_immutable_plan = std::move(immutable_plan);
  output.normalized_reconstructed_coface_count =
      adapted.reconstructed_coface_count;
  output.normalized_adapter_construction_certified = true;
  output.every_normalized_coface_reconstructed_transiently =
      adapted.every_coface_reconstructed_transiently;
  return output;
}

}  // namespace internal

ExactDirectNormalizedH0CandidateFacetDisposition
classify_exact_direct_normalized_h0_candidate_facet_birth(
    const exact::ExactLevel& facet_birth_squared_level,
    const exact::ExactLevel& active_squared_level) noexcept {
  if (facet_birth_squared_level < active_squared_level) {
    return ExactDirectNormalizedH0CandidateFacetDisposition::
        fail_open_strictly_earlier_without_retraction_authority;
  }
  if (active_squared_level < facet_birth_squared_level) {
    return ExactDirectNormalizedH0CandidateFacetDisposition::
        contradiction_strictly_later_than_active_level;
  }
  return ExactDirectNormalizedH0CandidateFacetDisposition::
      equal_at_active_level;
}

struct ExactDirectMorseUnifiedResidentSession::Impl {
  explicit Impl(
        const ExactDirectSparseUnifiedLevelPlanResult& source_plan,
      ExactDirectMorseUnifiedResidentSourceKind source_kind_value =
          ExactDirectMorseUnifiedResidentSourceKind::
              successive_incidence_star)
      : plan_storage(
            std::make_unique<const ExactDirectSparseUnifiedLevelPlanResult>(
                source_plan)),
        source_kind(source_kind_value) {}

  explicit Impl(ExactDirectMorseUnifiedResidentSourceKind source_kind_value)
      : source_kind(source_kind_value) {}

  [[nodiscard]] const ExactDirectSparseUnifiedLevelPlanResult&
  immutable_plan() const noexcept {
    return *plan_storage;
  }

  SourceReferences source{};
  ExactDirectMorseUnifiedResidentSessionBudget budget{};
  std::unique_ptr<const ExactDirectSparseUnifiedLevelPlanResult> plan_storage;
  std::optional<
      internal::ExactDirectFrozenUnifiedImmutablePlanAuthority>
      source_plan_authority;
  std::optional<ResidentNormalizedStrictFacetClosureAuthority>
      normalized_strict_facet_closure_authority;
  std::optional<ResidentNormalizedHorizontalIncidenceReductionAuthority>
      normalized_horizontal_incidence_reduction_authority;
  ExactDirectSparsePositiveFacetLocator locator{};
  ResidentState state{};
  std::shared_ptr<const SessionSeal> seal;
  std::shared_ptr<OutstandingTicketRegistry> ticket_registry;
  std::uint64_t authority_id{};
  std::uint64_t locator_instance_id{};
  std::size_t cursor{};
  std::size_t epoch{};
  std::size_t source_plan_initial_verification_count{};
  std::size_t frozen_batch_source_replay_count{};
  std::size_t frozen_batch_reconstruction_count{};
  ExactDirectMorseUnifiedResidentSourceKind source_kind{
      ExactDirectMorseUnifiedResidentSourceKind::successive_incidence_star};
  bool normalized_adapter_construction_certified{false};
  bool every_normalized_coface_reconstructed_transiently{false};
  ExactDirectNormalizedH0ResidentRetractionMode normalized_retraction_mode{
      ExactDirectNormalizedH0ResidentRetractionMode::
          not_applicable_successive_incidence_star};
  bool normalized_h0_retraction_authority_certified{false};
  bool initialized{false};
};

struct ExactDirectMorseUnifiedResidentPreparedBatch::Impl {
  ~Impl() noexcept {
    if (!owns_ticket_slot) {
      return;
    }
    if (ticket_registry == nullptr ||
        ticket_registry->live_ticket_count == 0U) {
      std::terminate();
    }
    --ticket_registry->live_ticket_count;
  }

  ExactDirectMorseUnifiedResidentAuthorityBundle bundle{};
  std::optional<
      internal::ExactDirectFrozenUnifiedResidentBatchAttestation>
      frozen_batch_attestation;
  std::optional<ResidentStrictFacetClosureAttestation>
      strict_facet_closure_attestation;
  std::shared_ptr<const SessionSeal> seal;
  std::shared_ptr<OutstandingTicketRegistry> ticket_registry;
  ExactDirectSparsePositiveFacetLocatorSnapshotStamp source_stamp{};
  std::size_t source_cursor{};
  std::size_t source_epoch{};
  std::vector<ExactDirectSparseFacetQuery> queries;
  std::vector<ExactDirectSparseComponentUnion> unions;
  std::vector<ExactDirectSparseFacetBinding> bindings;
  ResidentStateDelta delta{};
  bool owns_ticket_slot{false};
  bool consumed{false};
};

bool ExactDirectMorseUnifiedResidentFrozenBatchReceipt::
    certified_single_construction_receipt() const noexcept {
  return schema_version ==
             direct_morse_unified_resident_session_schema_version &&
         frozen_batch_construction_count == 1U &&
         quotient_streaming_verification_count == 1U &&
         action_plan_streaming_verification_count == 1U &&
         structural_certification_count == 1U &&
         source_plan_immutable_authority_certified &&
         frozen_batch_structurally_certified &&
         internal_move_only_attestation_issued &&
         !independent_expected_batch_freshly_reconstructed &&
         !global_facet_coface_or_gamma_catalog_materialized &&
         !supplied_star_global_completeness_claimed &&
         !public_status_claimed;
}

bool ExactDirectMorseUnifiedResidentAuthorityBundle::
    certified_strict_pre_batch_bundle() const noexcept {
  const bool certified_strict_closure_mode =
      rank_window_saturated_h0_authority_certified;
  const std::size_t strict_seed_count =
      counters.normalized_strict_facet_seed_count;
  std::size_t expected_strict_scratch_entry_peak = 0U;
  std::size_t expected_simultaneous_scratch_entry_peak =
      counters.normalized_facet_observation_scratch_entry_peak;
  const bool scratch_accounting_did_not_overflow =
      strict_seed_count <= std::numeric_limits<std::size_t>::max() / 3U &&
      !add_overflow(
          expected_strict_scratch_entry_peak,
          3U * strict_seed_count,
          expected_strict_scratch_entry_peak) &&
      !add_overflow(
          expected_simultaneous_scratch_entry_peak,
          expected_strict_scratch_entry_peak,
          expected_simultaneous_scratch_entry_peak) &&
      !add_overflow(
          expected_simultaneous_scratch_entry_peak,
          counters.normalized_strict_facet_closure_node_count,
          expected_simultaneous_scratch_entry_peak) &&
      !add_overflow(
          expected_simultaneous_scratch_entry_peak,
          counters.normalized_strict_facet_closure_edge_count,
          expected_simultaneous_scratch_entry_peak) &&
      !add_overflow(
          expected_simultaneous_scratch_entry_peak,
          counters.normalized_strict_facet_closure_seed_projection_count,
          expected_simultaneous_scratch_entry_peak);
  const bool certified_scratch_accounting =
      scratch_accounting_did_not_overflow &&
      counters.normalized_facet_observation_scratch_entry_peak ==
          counters.locator_probe_count &&
      counters.normalized_strict_facet_scratch_entry_peak ==
          expected_strict_scratch_entry_peak &&
      counters.normalized_strict_facet_closure_seed_projection_count ==
          strict_seed_count &&
      counters.normalized_strict_facet_simultaneous_scratch_entry_peak ==
          expected_simultaneous_scratch_entry_peak;
  const bool strict_closure_accounting =
      certified_strict_closure_mode
      ? every_locator_miss_has_fresh_exact_miniball &&
            every_strict_facet_miss_has_certified_relative_positive_closure &&
            strict_facet_closure_bound_to_pre_batch_locator_snapshot &&
            certified_scratch_accounting &&
            counters.normalized_strict_facet_seed_count ==
                counters.normalized_strict_facet_carrier_resolution_count &&
            counters.normalized_strict_facet_closure_build_count ==
                (counters.normalized_strict_facet_seed_count == 0U ? 0U
                                                                   : 1U) &&
            (counters.normalized_strict_facet_seed_count != 0U ||
             (counters.normalized_strict_facet_closure_node_count == 0U &&
              counters.normalized_strict_facet_closure_edge_count == 0U &&
              counters
                      .normalized_strict_facet_closure_seed_projection_count ==
                  0U &&
              counters
                      .normalized_strict_facet_closure_locator_snapshot_check_count ==
                  0U &&
              counters.normalized_strict_facet_carrier_resolution_count ==
                  0U)) &&
            private_strict_facet_closure_attestation_issued ==
                (counters.normalized_strict_facet_seed_count != 0U) &&
            counters.planned_strict_facet_binding_count ==
                counters.normalized_strict_facet_seed_count
      : every_unresolved_facet_has_fresh_exact_equal_miniball &&
            counters.normalized_strict_facet_seed_count == 0U &&
            counters.normalized_facet_observation_scratch_entry_peak == 0U &&
            counters.normalized_strict_facet_scratch_entry_peak == 0U &&
            counters
                    .normalized_strict_facet_simultaneous_scratch_entry_peak ==
                0U &&
            counters.normalized_strict_facet_closure_build_count == 0U &&
            counters.normalized_strict_facet_closure_node_count == 0U &&
            counters.normalized_strict_facet_closure_edge_count == 0U &&
            counters
                    .normalized_strict_facet_closure_seed_projection_count ==
                0U &&
            counters
                    .normalized_strict_facet_closure_locator_snapshot_check_count ==
                0U &&
            counters.normalized_strict_facet_carrier_resolution_count == 0U &&
            counters.planned_strict_facet_binding_count == 0U &&
            !private_strict_facet_closure_attestation_issued;
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
         counters.resident_state_full_copy_count == 0U &&
         counters.planned_group_record_count ==
             counters.sparse_delta_group_append_count &&
         frozen_batch.schema_version ==
             direct_frozen_unified_incidence_batch_schema_version &&
         frozen_batch.decision ==
             ExactDirectFrozenUnifiedIncidenceBatchDecision::
                 complete_certified_frozen_unified_incidence_batch &&
         !frozen_batch.source_plan_freshly_verified &&
         frozen_batch
             .source_plan_verified_once_by_immutable_resident_authority &&
         frozen_batch.frozen_quotient_freshly_streaming_verified &&
         frozen_batch.frozen_hgp_action_plan_freshly_streaming_verified &&
         frozen_batch_receipt.certified_single_construction_receipt() &&
         locator_snapshot_strictly_pre_batch &&
         strict_closure_accounting &&
         csr_authorities_share_identity_and_pre_batch_state &&
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
  const bool strict_attestation_matches =
      impl_ != nullptr &&
      (impl_->bundle.rank_window_saturated_h0_authority_certified
           ? (impl_->bundle.counters.normalized_strict_facet_seed_count == 0U
                  ? !impl_->strict_facet_closure_attestation.has_value()
                  : impl_->strict_facet_closure_attestation.has_value() &&
                        impl_->strict_facet_closure_attestation->attests(
                            impl_->bundle, impl_->bindings))
           : !impl_->strict_facet_closure_attestation.has_value());
  return impl_ != nullptr && impl_->seal != nullptr && !impl_->consumed &&
         impl_->owns_ticket_slot && impl_->ticket_registry != nullptr &&
         impl_->ticket_registry->live_ticket_count != 0U &&
         impl_->ticket_registry->live_ticket_count <=
             impl_->ticket_registry->maximum_ticket_count &&
         impl_->seal->authority_id ==
             impl_->bundle.identity.session_authority_id &&
         impl_->seal->locator_instance_id ==
             impl_->bundle.identity.locator_instance_id &&
         impl_->source_cursor == impl_->bundle.identity.batch_cursor &&
         impl_->source_epoch == impl_->bundle.identity.epoch &&
         impl_->source_stamp == impl_->bundle.identity.locator_stamp &&
         impl_->frozen_batch_attestation.has_value() &&
         impl_->frozen_batch_attestation->attests(
             impl_->bundle.frozen_batch) &&
         strict_attestation_matches &&
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
         sparse_delta_staged_with_rollback_before_locator &&
         staged_sparse_delta_released_after_locator_commit &&
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
  if (impl_ == nullptr || impl_->plan_storage == nullptr) {
    return false;
  }
  const bool normalized =
      impl_->source_kind ==
      ExactDirectMorseUnifiedResidentSourceKind::
          normalized_direct_h0_candidate_source;
  const bool authority_kind_matches =
      impl_->source_plan_authority.has_value() &&
      ((normalized &&
        impl_->source_plan_authority->kind() ==
            internal::ExactDirectFrozenUnifiedImmutablePlanAuthorityKind::
                normalized_direct_h0_candidate_source) ||
       (!normalized &&
        impl_->source_plan_authority->kind() ==
            internal::ExactDirectFrozenUnifiedImmutablePlanAuthorityKind::
                successive_incidence_star));
  const bool normalized_plan_facts_match =
      impl_->normalized_adapter_construction_certified &&
      impl_->every_normalized_coface_reconstructed_transiently &&
      !impl_->immutable_plan().source_star_freshly_verified &&
      !impl_->immutable_plan().direct_star_cofaces_crosschecked_bijectively &&
      !impl_->immutable_plan().bounded_star_global_completeness_claimed &&
      !impl_->immutable_plan().public_status_claimed &&
      impl_->immutable_plan().no_k_plus_one_coface_key_persisted &&
      impl_->immutable_plan().no_global_facet_or_coface_catalog_materialized &&
      impl_->immutable_plan().decision ==
          ExactDirectSparseUnifiedLevelPlanDecision::not_certified &&
      impl_->immutable_plan().scope ==
          ExactDirectSparseUnifiedLevelPlanScope::unspecified;
  const bool normalized_candidate_facts_match =
      normalized_plan_facts_match &&
      impl_->normalized_retraction_mode ==
          ExactDirectNormalizedH0ResidentRetractionMode::
              candidate_fail_open_without_h0_retraction_authority &&
      !impl_->normalized_h0_retraction_authority_certified &&
      !impl_->normalized_strict_facet_closure_authority.has_value() &&
      !impl_->normalized_horizontal_incidence_reduction_authority.has_value();
  const bool normalized_strict_closure_facts_match =
      normalized_plan_facts_match && impl_->source.index != nullptr &&
      impl_->source.cloud != nullptr &&
      certified_normalized_strict_closure_mode(
          impl_->normalized_retraction_mode) &&
      impl_->normalized_h0_retraction_authority_certified &&
      impl_->normalized_strict_facet_closure_authority.has_value() &&
      impl_->normalized_strict_facet_closure_authority->certifies(
          impl_->immutable_plan(), impl_->authority_id);
  const bool normalized_rank_closure_facts_match =
      normalized_strict_closure_facts_match &&
      impl_->normalized_retraction_mode ==
          ExactDirectNormalizedH0ResidentRetractionMode::
              certified_rank_window_and_sparse_strict_facet_closure &&
      !impl_->normalized_horizontal_incidence_reduction_authority.has_value();
  const bool normalized_incidence_complete_facts_match =
      normalized_strict_closure_facts_match &&
      certified_normalized_incidence_complete_mode(
          impl_->normalized_retraction_mode) &&
      impl_->normalized_horizontal_incidence_reduction_authority.has_value() &&
      impl_->normalized_horizontal_incidence_reduction_authority->certifies(
          impl_->immutable_plan(), impl_->authority_id);
  const bool normalized_adapter_facts_match =
      normalized
      ? normalized_candidate_facts_match ||
            normalized_rank_closure_facts_match ||
            normalized_incidence_complete_facts_match
      : impl_->normalized_retraction_mode ==
                ExactDirectNormalizedH0ResidentRetractionMode::
                    not_applicable_successive_incidence_star &&
            !impl_->normalized_h0_retraction_authority_certified &&
            !impl_->normalized_strict_facet_closure_authority.has_value() &&
            !impl_->normalized_horizontal_incidence_reduction_authority
                 .has_value();
  return impl_->initialized && impl_->seal != nullptr &&
         impl_->ticket_registry != nullptr &&
         impl_->ticket_registry->maximum_ticket_count ==
             impl_->budget.sparse_delta.maximum_outstanding_ticket_count &&
         impl_->ticket_registry->live_ticket_count <=
             impl_->ticket_registry->maximum_ticket_count &&
         impl_->authority_id != 0U &&
         impl_->locator_instance_id != 0U &&
         impl_->source_plan_initial_verification_count == 1U &&
         impl_->frozen_batch_source_replay_count == 0U &&
         impl_->source_plan_authority.has_value() &&
         impl_->source_plan_authority->certifies(impl_->immutable_plan()) &&
         authority_kind_matches && normalized_adapter_facts_match &&
         impl_->locator.certified_positive_locator() &&
         impl_->state.components.size() ==
             impl_->immutable_plan().facet_tokens.size() &&
         impl_->state.roots.size() <=
             impl_->budget.maximum_resident_root_count &&
         impl_->state.group_records.size() <=
             impl_->budget.maximum_group_record_count &&
         impl_->state.resident_root_point_reference_count <=
             impl_->budget.maximum_resident_root_point_reference_count &&
         impl_->state.resident_component_latent_point_reference_count <=
             impl_->budget
                 .maximum_resident_component_latent_point_reference_count &&
         impl_->state.group_child_reference_count <=
             impl_->budget.maximum_group_child_reference_count &&
         impl_->state.group_coverage_delta_point_reference_count <=
             impl_->budget
                 .maximum_group_coverage_delta_point_reference_count &&
         impl_->cursor <= impl_->immutable_plan().batches.size() &&
         impl_->epoch == impl_->cursor &&
         impl_->locator.snapshot_stamp().committed_batch_count ==
             impl_->cursor;
}

bool ExactDirectMorseUnifiedResidentSession::complete() const noexcept {
  return source_cursor_exhausted() &&
         (!normalized_direct_source_session() ||
          normalized_h0_retraction_authority_certified());
}

bool ExactDirectMorseUnifiedResidentSession::source_cursor_exhausted()
    const noexcept {
  return certified_resident_session() &&
         impl_->cursor == impl_->immutable_plan().batches.size();
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

std::size_t ExactDirectMorseUnifiedResidentSession::
    frozen_batch_reconstruction_count() const noexcept {
  return impl_ == nullptr ? 0U : impl_->frozen_batch_reconstruction_count;
}

ExactDirectMorseUnifiedResidentSourceKind
ExactDirectMorseUnifiedResidentSession::source_kind() const noexcept {
  return impl_ == nullptr
      ? ExactDirectMorseUnifiedResidentSourceKind::successive_incidence_star
      : impl_->source_kind;
}

bool ExactDirectMorseUnifiedResidentSession::
    normalized_direct_source_session() const noexcept {
  return impl_ != nullptr &&
         impl_->source_kind ==
             ExactDirectMorseUnifiedResidentSourceKind::
                 normalized_direct_h0_candidate_source;
}

ExactDirectNormalizedH0ResidentRetractionMode
ExactDirectMorseUnifiedResidentSession::normalized_h0_retraction_mode()
    const noexcept {
  return impl_ == nullptr
      ? ExactDirectNormalizedH0ResidentRetractionMode::
            not_applicable_successive_incidence_star
      : impl_->normalized_retraction_mode;
}

bool ExactDirectMorseUnifiedResidentSession::
    normalized_h0_retraction_authority_certified() const noexcept {
  return impl_ != nullptr &&
         impl_->normalized_h0_retraction_authority_certified;
}

bool ExactDirectMorseUnifiedResidentSession::
    normalized_horizontal_incidence_reduction_certified() const noexcept {
  return impl_ != nullptr &&
         certified_normalized_incidence_complete_mode(
             impl_->normalized_retraction_mode) &&
         impl_->normalized_horizontal_incidence_reduction_authority
             .has_value() &&
         impl_->normalized_horizontal_incidence_reduction_authority->certifies(
             impl_->immutable_plan(), impl_->authority_id);
}

const ExactDirectSparseUnifiedLevelPlanResult&
ExactDirectMorseUnifiedResidentSession::plan() const noexcept {
  static const ExactDirectSparseUnifiedLevelPlanResult empty{};
  return impl_ == nullptr ? empty : impl_->immutable_plan();
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

ExactDirectSparseFacetDescentClosureResult
ExactDirectMorseUnifiedResidentSession::
    build_resident_sparse_facet_descent_closure(
        std::span<const ExactDirectSparseFacetKey> canonical_distinct_keys,
        const exact::ExactLevel& closed_batch_squared_level,
        const ExactDirectSparseFacetWitness& locator_query_witness,
        const ExactDirectSparseFacetDescentClosureBudget& budget,
        const ExactDirectSparseFacetDescentClosureConfig& config,
        spatial::LbvhTraversalOrder traversal_order) const {
  if (!certified_resident_session() || impl_->source.index == nullptr ||
      impl_->source.cloud == nullptr) {
    return {};
  }
  return build_exact_direct_sparse_facet_descent_closure_from_canonical_distinct_keys(
      *impl_->source.index,
      *impl_->source.cloud,
      canonical_distinct_keys,
      closed_batch_squared_level,
      locator_query_witness,
      impl_->locator,
      budget,
      config,
      traversal_order);
}

bool ExactDirectMorseUnifiedResidentInitializationResult::
    certified_initialized_session() const noexcept {
  const bool normalized =
      source_kind == ExactDirectMorseUnifiedResidentSourceKind::
                         normalized_direct_h0_candidate_source;
  const bool normalized_common_receipt =
      normalized_source_plan_consumed_directly &&
      normalized_sparse_compatibility_plan_certified &&
      every_normalized_coface_reconstructed_transiently &&
      !successive_incidence_star_materialized_by_adapter;
  const bool normalized_candidate_receipt =
      normalized_common_receipt &&
      normalized_h0_retraction_mode ==
          ExactDirectNormalizedH0ResidentRetractionMode::
              candidate_fail_open_without_h0_retraction_authority &&
      !normalized_h0_retraction_authority_certified &&
      normalized_candidate_fails_open_on_strictly_earlier_facet &&
      !rank_window_saturated_h0_authority_freshly_verified &&
      !strict_facet_closure_session_capability_issued &&
      !normalized_h0_incidence_reduction_authority_freshly_verified &&
      !normalized_h0_incidence_reduction_session_capability_issued &&
      !incidence_complete_reduction_proved;
  const bool normalized_rank_closure_receipt =
      normalized_common_receipt &&
      normalized_h0_retraction_mode ==
          ExactDirectNormalizedH0ResidentRetractionMode::
              certified_rank_window_and_sparse_strict_facet_closure &&
      normalized_h0_retraction_authority_certified &&
      !normalized_candidate_fails_open_on_strictly_earlier_facet &&
      rank_window_saturated_h0_authority_freshly_verified &&
      strict_facet_closure_session_capability_issued &&
      !normalized_h0_incidence_reduction_authority_freshly_verified &&
      !normalized_h0_incidence_reduction_session_capability_issued &&
      !incidence_complete_reduction_proved;
  const bool normalized_incidence_complete_receipt =
      normalized_common_receipt &&
      normalized_h0_retraction_mode ==
          ExactDirectNormalizedH0ResidentRetractionMode::
              certified_horizontal_incidence_complete_rank_window_and_sparse_strict_facet_closure &&
      normalized_h0_retraction_authority_certified &&
      !normalized_candidate_fails_open_on_strictly_earlier_facet &&
      rank_window_saturated_h0_authority_freshly_verified &&
      strict_facet_closure_session_capability_issued &&
      normalized_h0_incidence_reduction_authority_freshly_verified &&
      normalized_h0_incidence_reduction_session_capability_issued &&
      incidence_complete_reduction_proved;
  const bool source_receipt_certified =
      normalized
      ? normalized_candidate_receipt || normalized_rank_closure_receipt ||
            normalized_incidence_complete_receipt
      : !normalized_source_plan_consumed_directly &&
            !normalized_sparse_compatibility_plan_certified &&
            !every_normalized_coface_reconstructed_transiently &&
            normalized_h0_retraction_mode ==
                ExactDirectNormalizedH0ResidentRetractionMode::
                    not_applicable_successive_incidence_star &&
            !normalized_h0_retraction_authority_certified &&
            !normalized_candidate_fails_open_on_strictly_earlier_facet &&
            !rank_window_saturated_h0_authority_freshly_verified &&
            !strict_facet_closure_session_capability_issued &&
            !successive_incidence_star_materialized_by_adapter &&
            !normalized_h0_incidence_reduction_authority_freshly_verified &&
            !normalized_h0_incidence_reduction_session_capability_issued &&
            !incidence_complete_reduction_proved;
  return session.has_value() && session->certified_resident_session() &&
         session->source_kind() == source_kind && source_receipt_certified &&
         session->normalized_h0_retraction_mode() ==
             normalized_h0_retraction_mode &&
         session->normalized_h0_retraction_authority_certified() ==
             normalized_h0_retraction_authority_certified &&
         session->normalized_horizontal_incidence_reduction_certified() ==
             incidence_complete_reduction_proved &&
         source_plan_initial_verification_count == 1U &&
         source_plan_freshly_verified_once && source_plan_owned_by_session &&
         locator_and_component_state_initialized &&
         !global_regularity_authority_certified &&
         !omitted_late_cofaces_qr1_noop_certified &&
         !normalized_verticality_certified &&
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
  if (impl_->cursor >= impl_->immutable_plan().batches.size()) {
    return reject(
        ExactDirectMorseUnifiedResidentPreparationDecision::
            no_plan_exhausted);
  }
  if (impl_->ticket_registry == nullptr ||
      impl_->ticket_registry->live_ticket_count >=
          impl_->ticket_registry->maximum_ticket_count) {
    return reject(
        ExactDirectMorseUnifiedResidentPreparationDecision::
            no_outstanding_ticket_budget_exhausted);
  }

  try {
    auto prepared = std::make_unique<
        ExactDirectMorseUnifiedResidentPreparedBatch::Impl>();
    prepared->seal = impl_->seal;
    prepared->source_cursor = impl_->cursor;
    prepared->source_epoch = impl_->epoch;
    prepared->source_stamp = impl_->locator.snapshot_stamp();
    prepared->delta.next_root_id = impl_->state.next_root_id;
    prepared->delta.next_replay_token = impl_->state.next_replay_token;
    prepared->bundle.counters.resident_state_full_copy_count = 0U;

    const auto& batch = impl_->immutable_plan().batches[impl_->cursor];
    auto& bundle = prepared->bundle;
    bundle.identity = {
        direct_morse_unified_resident_session_schema_version,
        impl_->authority_id,
        impl_->locator_instance_id,
        impl_->epoch,
        impl_->cursor,
        impl_->immutable_plan().source_pair_canonical_cloud_digest,
        impl_->immutable_plan().source_higher_canonical_cloud_digest,
        impl_->immutable_plan().source_pair_semantic_digest,
        impl_->immutable_plan().source_higher_semantic_digest,
        prepared->source_stamp,
    };
    bundle.source_batch_index = batch.batch_index;
    bundle.source_future_snapshot_index = batch.future_snapshot_index;
    bundle.squared_level = batch.squared_level;
    bundle.order = batch.order;

    const std::vector<std::size_t> touched =
        touched_facets(impl_->immutable_plan(), batch);
    if (touched.size() > impl_->budget.maximum_facet_resolution_count ||
        touched.size() > impl_->budget.locator.maximum_batch_query_count) {
      return reject(
          ExactDirectMorseUnifiedResidentPreparationDecision::
              no_authority_budget_exhausted);
    }
    bundle.facet_resolutions.reserve(touched.size());
    prepared->queries.reserve(touched.size());

    const bool certified_normalized_strict_closure =
        impl_->source_kind ==
            ExactDirectMorseUnifiedResidentSourceKind::
                normalized_direct_h0_candidate_source &&
        certified_normalized_strict_closure_mode(
            impl_->normalized_retraction_mode) &&
        impl_->normalized_h0_retraction_authority_certified &&
        impl_->normalized_strict_facet_closure_authority.has_value();

    if (!certified_normalized_strict_closure) {
      // Preserve the historical SuccessiveStar and normalized-candidate
      // one-pass behavior exactly.  In particular, the candidate path still
      // fails open on its first strict miss.
      for (const std::size_t facet_token_index : touched) {
      if (facet_token_index >=
          impl_->immutable_plan().facet_tokens.size()) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                contradiction_locator_state_inconsistent);
      }
      const auto& key =
          impl_->immutable_plan().facet_tokens[facet_token_index].facet_key;
      ExactDirectSparseFacetWitness witness;
      if (!checked_next_witness(
              prepared->delta, impl_->authority_id, witness)) {
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
                impl_->state.components.size()) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  contradiction_locator_state_inconsistent);
        }
        const std::size_t component_handle = find_component(
            impl_->state, probe.component_handle);
        const auto& component =
            impl_->state.components[component_handle];
        if (!component.active ||
            component_handle >
                std::numeric_limits<ExactFrozenIncidenceTokenId>::max()) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  contradiction_locator_state_inconsistent);
        }
        if (component.root_id.has_value()) {
          if (find_root(
                  impl_->state, *component.root_id) == nullptr) {
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
      const auto candidate_facet_disposition =
          classify_exact_direct_normalized_h0_candidate_facet_birth(
              miniball.squared_radius, batch.squared_level);
      if (candidate_facet_disposition ==
          ExactDirectNormalizedH0CandidateFacetDisposition::
              fail_open_strictly_earlier_without_retraction_authority) {
        return reject(
            impl_->source_kind ==
                        ExactDirectMorseUnifiedResidentSourceKind::
                            normalized_direct_h0_candidate_source &&
                    impl_->normalized_retraction_mode ==
                        ExactDirectNormalizedH0ResidentRetractionMode::
                            candidate_fail_open_without_h0_retraction_authority &&
                    !impl_->normalized_h0_retraction_authority_certified
                ? ExactDirectMorseUnifiedResidentPreparationDecision::
                      no_normalized_h0_retraction_authority_for_strictly_earlier_facet
                : ExactDirectMorseUnifiedResidentPreparationDecision::
                      contradiction_unresolved_facet_birth_below_active_level);
      }
      if (candidate_facet_disposition ==
          ExactDirectNormalizedH0CandidateFacetDisposition::
              contradiction_strictly_later_than_active_level) {
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
    } else {
      if (impl_->source.index == nullptr || impl_->source.cloud == nullptr ||
          !impl_->normalized_strict_facet_closure_authority->certifies(
              impl_->immutable_plan(), impl_->authority_id) ||
          batch.order >= impl_->source.cloud->size()) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_prepared_state_rejected);
      }

      // Pass 1 observes every touched key against one immutable locator stamp
      // and classifies every miss by a fresh exact facet miniball.  No bundle
      // resolution, frozen batch or resident delta exists yet.
      const auto& closure_authority =
          *impl_->normalized_strict_facet_closure_authority;
      if (touched.size() >
              impl_->budget
                  .maximum_normalized_facet_observation_scratch_count ||
          touched.size() >
              impl_->budget
                  .maximum_normalized_strict_facet_simultaneous_scratch_entry_count) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_authority_budget_exhausted);
      }
      std::vector<ResidentFacetObservation> observations;
      observations.reserve(touched.size());
      for (const std::size_t facet_token_index : touched) {
        if (facet_token_index >=
            impl_->immutable_plan().facet_tokens.size()) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  contradiction_locator_state_inconsistent);
        }
        const auto& key =
            impl_->immutable_plan().facet_tokens[facet_token_index].facet_key;
        if (!facet_key_is_canonical(key) || key.point_count != batch.order) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  contradiction_locator_state_inconsistent);
        }
        ExactDirectSparseFacetWitness witness;
        if (!checked_next_witness(
                prepared->delta, impl_->authority_id, witness)) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  no_prepared_state_rejected);
        }
        prepared->queries.push_back(
            {prepared->queries.size(), key, witness});
        const auto probe = impl_->locator.probe_positive_facet(
            key, witness, impl_->budget.probe);
        ++bundle.counters.locator_probe_count;
        if (impl_->locator.snapshot_stamp() != prepared->source_stamp) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  contradiction_locator_state_inconsistent);
        }
        if (probe.certified_positive_hit()) {
          ++bundle.counters.positive_locator_probe_count;
          if (!probe.component_handle_present ||
              probe.component_handle >= impl_->state.components.size()) {
            return reject(
                ExactDirectMorseUnifiedResidentPreparationDecision::
                    contradiction_locator_state_inconsistent);
          }
          const std::size_t component_handle =
              find_component(impl_->state, probe.component_handle);
          const auto& component = impl_->state.components[component_handle];
          if (!component.active ||
              component_handle >
                  std::numeric_limits<ExactFrozenIncidenceTokenId>::max() ||
              (component.root_id.has_value() &&
               find_root(impl_->state, *component.root_id) == nullptr)) {
            return reject(
                ExactDirectMorseUnifiedResidentPreparationDecision::
                    contradiction_locator_state_inconsistent);
          }
          observations.push_back(
              {facet_token_index,
               key,
               witness,
               ResidentFacetObservationKind::positive_carrier,
               component_handle,
               probe.source_binding_witness_present
                   ? std::optional<ExactDirectSparseFacetWitness>{
                         probe.source_binding_witness}
                   : std::nullopt});
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
        if (miniball.counters.enumerated_support_count >
            std::numeric_limits<std::size_t>::max() / 3U) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  no_authority_budget_exhausted);
        }
        const std::size_t replayed_support_count =
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
        const auto disposition =
            classify_exact_direct_normalized_h0_candidate_facet_birth(
                miniball.squared_radius, batch.squared_level);
        if (disposition ==
            ExactDirectNormalizedH0CandidateFacetDisposition::
                contradiction_strictly_later_than_active_level) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  contradiction_unresolved_facet_birth_above_active_level);
        }
        const ResidentFacetObservationKind kind =
            disposition ==
                    ExactDirectNormalizedH0CandidateFacetDisposition::
                        fail_open_strictly_earlier_without_retraction_authority
                ? ResidentFacetObservationKind::
                      strict_facet_pending_closure
                : ResidentFacetObservationKind::equal_facet;
        observations.push_back(
            {facet_token_index,
             key,
             witness,
             kind,
             std::nullopt,
             std::nullopt});
      }
      bundle.counters.normalized_facet_observation_scratch_entry_peak =
          observations.size();
      bundle.counters
          .normalized_strict_facet_simultaneous_scratch_entry_peak =
          observations.size();

      // Pass 2 sorts only the distinct strict misses by complete canonical
      // key, calls exactly one sparse closure for the nonempty set, and
      // compacts its terminal carrier evidence into a move-only ticket
      // attestation before the closure graph leaves scope.
      const std::size_t strict_seed_count = static_cast<std::size_t>(
          std::count_if(
              observations.begin(),
              observations.end(),
              [](const ResidentFacetObservation& observation) {
                return observation.kind ==
                       ResidentFacetObservationKind::
                           strict_facet_pending_closure;
              }));
      std::size_t strict_scratch_entry_count = 0U;
      if (strict_seed_count >
              closure_authority.budget().maximum_seed_count ||
          strict_seed_count >
              std::numeric_limits<std::size_t>::max() / 3U) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_strict_facet_closure_budget_exhausted);
      }
      strict_scratch_entry_count = 3U * strict_seed_count;
      if (strict_scratch_entry_count >
          impl_->budget.maximum_normalized_strict_facet_scratch_count) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_strict_facet_closure_budget_exhausted);
      }
      bundle.counters.normalized_strict_facet_scratch_entry_peak =
          strict_scratch_entry_count;
      std::vector<std::size_t> strict_observation_indices;
      strict_observation_indices.reserve(strict_seed_count);
      for (std::size_t observation_index = 0U;
           observation_index < observations.size();
           ++observation_index) {
        if (observations[observation_index].kind ==
            ResidentFacetObservationKind::strict_facet_pending_closure) {
          strict_observation_indices.push_back(observation_index);
        }
      }
      std::sort(
          strict_observation_indices.begin(),
          strict_observation_indices.end(),
          [&](std::size_t left, std::size_t right) {
            return facet_key_less(
                observations[left].facet_key,
                observations[right].facet_key);
          });
      std::vector<ExactDirectSparseFacetKey> strict_keys;
      strict_keys.reserve(strict_observation_indices.size());
      for (const std::size_t observation_index : strict_observation_indices) {
        if (observation_index >= observations.size() ||
            (!strict_keys.empty() &&
             !facet_key_less(
                 strict_keys.back(),
                 observations[observation_index].facet_key))) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  contradiction_strict_facet_closure_rejected);
        }
        strict_keys.push_back(observations[observation_index].facet_key);
      }
      bundle.counters.normalized_strict_facet_seed_count =
          strict_keys.size();
      if (!strict_keys.empty()) {
        // The caller-visible resident scratch cap covers the exact peak of
        // observations, strict indices, strict keys, the returned closure
        // graph and seed projections, and compact carrier records.  This
        // conservative preflight uses the guarded closure maxima before any
        // graph allocation; internal closure-builder arenas remain governed
        // by the closure's own explicit memo/step budgets.
        std::size_t simultaneous_scratch_preflight = observations.size();
        const std::size_t maximum_closure_edge_count = std::min(
            closure_authority.budget().maximum_node_count,
            closure_authority.budget().maximum_step_call_count);
        if (add_overflow(
                simultaneous_scratch_preflight,
                strict_scratch_entry_count,
                simultaneous_scratch_preflight) ||
            add_overflow(
                simultaneous_scratch_preflight,
                strict_keys.size(),
                simultaneous_scratch_preflight) ||
            add_overflow(
                simultaneous_scratch_preflight,
                closure_authority.budget().maximum_node_count,
                simultaneous_scratch_preflight) ||
            add_overflow(
                simultaneous_scratch_preflight,
                maximum_closure_edge_count,
                simultaneous_scratch_preflight) ||
            simultaneous_scratch_preflight >
                impl_->budget
                    .maximum_normalized_strict_facet_simultaneous_scratch_entry_count) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  no_strict_facet_closure_budget_exhausted);
        }
        const ExactDirectSparseFacetWitness closure_witness =
            observations[strict_observation_indices.front()].query_witness;
        const auto closure =
            build_exact_direct_sparse_facet_descent_closure_from_canonical_distinct_keys(
                *impl_->source.index,
                *impl_->source.cloud,
                strict_keys,
                batch.squared_level,
                closure_witness,
                impl_->locator,
                closure_authority.budget(),
                closure_authority.config(),
                closure_authority.traversal_order());
        ++bundle.counters.normalized_strict_facet_closure_build_count;
        bundle.counters.normalized_strict_facet_closure_node_count =
            closure.nodes.size();
        bundle.counters.normalized_strict_facet_closure_edge_count =
            closure.edges.size();
        bundle.counters
            .normalized_strict_facet_closure_seed_projection_count =
            closure.seed_projections.size();
        bundle.counters
            .normalized_strict_facet_closure_locator_snapshot_check_count =
            closure.counters.locator_snapshot_check_count;
        if (closure.certified_budget_exhaustion()) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  no_strict_facet_closure_budget_exhausted);
        }
        if (closure.certified_complete_with_unresolved_terminals()) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  no_strict_facet_closure_unresolved);
        }
        if (!closure.certified_complete_relative_positive_closure() ||
            closure.locator_snapshot_stamp != prepared->source_stamp ||
            closure.locator_query_witness != closure_witness ||
            closure.closed_batch_squared_level != batch.squared_level ||
            closure.common_facet_cardinality != batch.order ||
            closure.seed_projections.size() != strict_keys.size() ||
            impl_->locator.snapshot_stamp() != prepared->source_stamp) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  contradiction_strict_facet_closure_rejected);
        }

        std::vector<ResidentStrictFacetCarrierRecord> carrier_records;
        carrier_records.reserve(strict_keys.size());
        for (std::size_t seed_index = 0U;
             seed_index < strict_keys.size();
             ++seed_index) {
          const auto& projection = closure.seed_projections[seed_index];
          if (projection.seed_index != seed_index ||
              projection.source_facet_key != strict_keys[seed_index] ||
              projection.closure_disposition !=
                  ExactDirectSparseFacetDescentClosureDisposition::
                      relative_positive ||
              projection.terminal_node_index >= closure.nodes.size()) {
            return reject(
                ExactDirectMorseUnifiedResidentPreparationDecision::
                    contradiction_strict_facet_closure_rejected);
          }
          const auto& terminal =
              closure.nodes[projection.terminal_node_index];
          if (terminal.closure_disposition !=
                  ExactDirectSparseFacetDescentClosureDisposition::
                      relative_positive ||
              !terminal.resolved_component_handle.has_value() ||
              !terminal.resolved_binding_witness.has_value() ||
              *terminal.resolved_component_handle >=
                  impl_->state.components.size()) {
            return reject(
                ExactDirectMorseUnifiedResidentPreparationDecision::
                    contradiction_strict_facet_closure_rejected);
          }
          const std::size_t component_handle = find_component(
              impl_->state, *terminal.resolved_component_handle);
          const auto& component = impl_->state.components[component_handle];
          if (!component.active ||
              component_handle >
                  std::numeric_limits<ExactFrozenIncidenceTokenId>::max() ||
              (component.root_id.has_value() &&
               find_root(impl_->state, *component.root_id) == nullptr)) {
            return reject(
                ExactDirectMorseUnifiedResidentPreparationDecision::
                    contradiction_locator_state_inconsistent);
          }
          const std::size_t observation_index =
              strict_observation_indices[seed_index];
          observations[observation_index].component_handle =
              component_handle;
          observations[observation_index].terminal_binding_witness =
              *terminal.resolved_binding_witness;
          const auto token_kind = component.root_id.has_value()
              ? ExactFrozenIncidenceTokenKind::rooted_carrier
              : ExactFrozenIncidenceTokenKind::latent_carrier;
          carrier_records.push_back(
              {observations[observation_index].facet_token_index,
               strict_keys[seed_index],
               terminal.facet_key,
               component_handle,
               *terminal.resolved_binding_witness,
               token_kind,
               component.root_id});
        }
        bundle.counters.normalized_strict_facet_carrier_resolution_count =
            carrier_records.size();
        std::size_t simultaneous_scratch_peak = observations.size();
        if (add_overflow(
                simultaneous_scratch_peak,
                strict_scratch_entry_count,
                simultaneous_scratch_peak) ||
            add_overflow(
                simultaneous_scratch_peak,
                closure.nodes.size(),
                simultaneous_scratch_peak) ||
            add_overflow(
                simultaneous_scratch_peak,
                closure.edges.size(),
                simultaneous_scratch_peak) ||
            add_overflow(
                simultaneous_scratch_peak,
                closure.seed_projections.size(),
                simultaneous_scratch_peak) ||
            simultaneous_scratch_peak >
                impl_->budget
                    .maximum_normalized_strict_facet_simultaneous_scratch_entry_count) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  no_strict_facet_closure_budget_exhausted);
        }
        bundle.counters
            .normalized_strict_facet_simultaneous_scratch_entry_peak =
            simultaneous_scratch_peak;
        prepared->strict_facet_closure_attestation.emplace(
            prepared->source_stamp,
            batch.batch_index,
            batch.squared_level,
            batch.order,
            std::move(carrier_records),
            closure.nodes.size(),
            closure.edges.size(),
            closure.counters.locator_snapshot_check_count);
      }

      for (const auto& observation : observations) {
        if (observation.facet_token_index >
            std::numeric_limits<ExactFrozenIncidenceTokenId>::max()) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  no_prepared_state_rejected);
        }
        if (observation.kind == ResidentFacetObservationKind::equal_facet) {
          bundle.facet_resolutions.push_back(
              {observation.facet_token_index,
               {ExactFrozenIncidenceTokenKind::equal_facet,
                static_cast<ExactFrozenIncidenceTokenId>(
                    observation.facet_token_index)},
               std::nullopt});
          ++bundle.counters.equal_resolution_count;
          continue;
        }
        if (!observation.component_handle.has_value()) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  contradiction_strict_facet_closure_rejected);
        }
        const auto& component =
            impl_->state.components[*observation.component_handle];
        if (component.root_id.has_value()) {
          bundle.facet_resolutions.push_back(
              {observation.facet_token_index,
               {ExactFrozenIncidenceTokenKind::rooted_carrier,
                static_cast<ExactFrozenIncidenceTokenId>(
                    *observation.component_handle)},
               component.root_id});
          ++bundle.counters.rooted_resolution_count;
        } else {
          bundle.facet_resolutions.push_back(
              {observation.facet_token_index,
               {ExactFrozenIncidenceTokenKind::latent_carrier,
                static_cast<ExactFrozenIncidenceTokenId>(
                    *observation.component_handle)},
               std::nullopt});
          ++bundle.counters.latent_resolution_count;
        }
        if (observation.kind ==
            ResidentFacetObservationKind::strict_facet_pending_closure) {
          ExactDirectSparseFacetWitness binding_witness;
          if (!checked_next_witness(
                  prepared->delta,
                  impl_->authority_id,
                  binding_witness)) {
            return reject(
                ExactDirectMorseUnifiedResidentPreparationDecision::
                    no_prepared_state_rejected);
          }
          prepared->bindings.push_back(
              {prepared->bindings.size(),
               observation.facet_key,
               *observation.component_handle,
               binding_witness});
          ++bundle.counters.planned_strict_facet_binding_count;
        }
      }
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
      const auto* root = find_root(impl_->state, root_id);
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
      if (token_id >= impl_->state.components.size()) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                contradiction_locator_state_inconsistent);
      }
      const std::size_t component_handle = find_component(
          impl_->state,
          static_cast<std::size_t>(token_id));
      const auto& component =
          impl_->state.components[component_handle];
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
      const auto* root = find_root(impl_->state, root_id);
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
      if (token_id >= impl_->state.components.size()) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                contradiction_locator_state_inconsistent);
      }
      const std::size_t component_handle = find_component(
          impl_->state,
          static_cast<std::size_t>(token_id));
      const auto& component =
          impl_->state.components[component_handle];
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

    if (!impl_->source_plan_authority.has_value() ||
        !impl_->source_plan_authority->certifies(
            impl_->immutable_plan()) ||
        impl_->frozen_batch_reconstruction_count >
            std::numeric_limits<std::size_t>::max() - 1U) {
      return reject(
          ExactDirectMorseUnifiedResidentPreparationDecision::
              no_prepared_state_rejected);
    }
    prepared->frozen_batch_attestation =
        internal::ExactDirectFrozenUnifiedResidentBatchAttestedBuilder::
            build_once(
            *impl_->source_plan_authority,
            impl_->cursor,
            bundle.facet_resolutions,
            bundle.prior_root_coverages,
            bundle.prior_root_coverage_point_references,
            bundle.latent_carrier_coverages,
            bundle.latent_carrier_coverage_point_references,
            impl_->budget.frozen_batch,
            bundle.frozen_batch);
    ++impl_->frozen_batch_reconstruction_count;
    if (!prepared->frozen_batch_attestation.has_value()) {
      return reject(
          bundle.frozen_batch.decision ==
                  ExactDirectFrozenUnifiedIncidenceBatchDecision::
                      complete_certified_frozen_unified_incidence_batch
              ? ExactDirectMorseUnifiedResidentPreparationDecision::
                    no_frozen_batch_verification_rejected
              : ExactDirectMorseUnifiedResidentPreparationDecision::
                    no_frozen_batch_rejected);
    }
    if (!prepared->frozen_batch_attestation->attests(
            bundle.frozen_batch)) {
      return reject(
          ExactDirectMorseUnifiedResidentPreparationDecision::
              no_frozen_batch_verification_rejected);
    }

    const std::vector<std::size_t> births =
        direct_birth_facets(impl_->immutable_plan(), batch);
    std::vector<std::size_t> equal_facets;
    equal_facets.reserve(bundle.frozen_batch.equal_facet_binding_records.size());
    for (const auto& equal :
         bundle.frozen_batch.equal_facet_binding_records) {
      equal_facets.push_back(equal.facet_token_index);
    }
    std::sort(equal_facets.begin(), equal_facets.end());

    std::size_t maximum_required_component_patches = 0U;
    if (add_overflow(
            bundle.frozen_batch.quotient.group_tokens.size(),
            births.size(),
            maximum_required_component_patches)) {
      return reject(
          ExactDirectMorseUnifiedResidentPreparationDecision::
              no_sparse_delta_budget_exhausted);
    }
    const std::size_t component_patch_reserve = std::min(
        impl_->budget.sparse_delta.maximum_component_patch_count,
        maximum_required_component_patches);
    prepared->delta.component_patches.reserve(component_patch_reserve);
    prepared->delta.root_replacements.reserve(std::min(
        impl_->budget.sparse_delta.maximum_root_replacement_count,
        bundle.frozen_batch.quotient.groups.size()));
    prepared->delta.new_roots.reserve(std::min(
        impl_->budget.sparse_delta.maximum_new_root_count,
        bundle.frozen_batch.quotient.groups.size()));
    if (bundle.frozen_batch.quotient.groups.size() >
        impl_->budget.sparse_delta.maximum_group_append_count) {
      return reject(
          ExactDirectMorseUnifiedResidentPreparationDecision::
              no_sparse_delta_budget_exhausted);
    }
    prepared->delta.new_group_records.reserve(
        bundle.frozen_batch.quotient.groups.size());

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
        if (token.token_id >= impl_->state.components.size()) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  no_prepared_state_rejected);
        }
        const std::size_t handle =
            static_cast<std::size_t>(token.token_id);
        handles.push_back(handle);
        if (token.kind == ExactFrozenIncidenceTokenKind::equal_facet) {
          if (component_after_delta(
                  impl_->state, prepared->delta, handle)
                  .active) {
            return reject(
                ExactDirectMorseUnifiedResidentPreparationDecision::
                    contradiction_locator_state_inconsistent);
          }
          auto* equal_patch = ensure_component_patch(
              impl_->state,
              prepared->delta,
              handle,
              impl_->budget.sparse_delta,
              ResidentLatentPatchPolicy::omit_points_before_overwrite);
          if (equal_patch == nullptr) {
            return reject(
                ExactDirectMorseUnifiedResidentPreparationDecision::
                    no_sparse_delta_budget_exhausted);
          }
          auto& equal_component = equal_patch->next;
          equal_component.active = true;
          equal_component.parent_handle = handle;
          equal_component.root_id.reset();
          if (!assign_component_latent_points(
                  *equal_patch,
                  key_points(
                      impl_->immutable_plan().facet_tokens[handle].facet_key),
                  prepared->delta,
                  impl_->budget.sparse_delta)) {
            return reject(
                ExactDirectMorseUnifiedResidentPreparationDecision::
                    no_sparse_delta_budget_exhausted);
          }
        } else {
          const std::size_t representative = find_component(
              impl_->state, prepared->delta, handle);
          const auto& component =
              component_after_delta(
                  impl_->state, prepared->delta, representative);
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

      const bool empty_continuation =
          action_group.q_r == 1U && delta.point_reference_count == 0U;
      std::vector<PointId> resultant_coverage;
      if (!empty_continuation) {
        for (const auto root_id : planned_prior_roots) {
          const auto* points = root_points_after_delta(
              impl_->state, prepared->delta, root_id);
          if (points == nullptr) {
            return reject(
                ExactDirectMorseUnifiedResidentPreparationDecision::
                    contradiction_locator_state_inconsistent);
          }
          resultant_coverage.insert(
              resultant_coverage.end(),
              points->begin(),
              points->end());
        }
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
      if (!empty_continuation) {
        canonicalize_points(resultant_coverage);
      }

      ExactFrozenIncidencePriorRootId resultant_root_id{};
      if (action_group.q_r == 1U) {
        resultant_root_id = planned_prior_roots.front();
        if (root_points_after_delta(
                impl_->state, prepared->delta, resultant_root_id) == nullptr) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  no_prepared_state_rejected);
        }
        // An empty q_R=1 delta is a true continuation: canonical coverage is
        // unchanged, so it must not manufacture a resident root patch.
        if (!empty_continuation &&
            !set_root_replacement(
                impl_->state,
                prepared->delta,
                resultant_root_id,
                std::move(resultant_coverage),
                impl_->budget.sparse_delta.maximum_root_replacement_count)) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  no_sparse_delta_budget_exhausted);
        }
      } else {
        if (prepared->delta.next_root_id == 0U ||
            prepared->delta.new_roots.size() >=
                impl_->budget.sparse_delta.maximum_new_root_count) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  no_sparse_delta_budget_exhausted);
        }
        resultant_root_id = prepared->delta.next_root_id;
        if (prepared->delta.next_root_id ==
            std::numeric_limits<ExactFrozenIncidencePriorRootId>::max()) {
          prepared->delta.next_root_id = 0U;
        } else {
          ++prepared->delta.next_root_id;
        }
        prepared->delta.new_roots.push_back(
            {resultant_root_id, std::move(resultant_coverage)});
      }

      const std::size_t canonical_handle = handles.front();
      for (const std::size_t handle : handles) {
        const std::size_t left = find_component(
            impl_->state, prepared->delta, canonical_handle);
        const std::size_t right = find_component(
            impl_->state, prepared->delta, handle);
        if (left != right) {
          ExactDirectSparseFacetWitness witness;
          if (!checked_next_witness(
                  prepared->delta,
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
          auto* child_patch = ensure_component_patch(
              impl_->state,
              prepared->delta,
              child,
              impl_->budget.sparse_delta,
              ResidentLatentPatchPolicy::preserve_current_points);
          if (child_patch == nullptr) {
            return reject(
                ExactDirectMorseUnifiedResidentPreparationDecision::
                    no_sparse_delta_budget_exhausted);
          }
          child_patch->next.parent_handle = parent;
        }
      }
      const std::size_t representative = find_component(
          impl_->state, prepared->delta, canonical_handle);
      auto* resultant_patch = ensure_component_patch(
          impl_->state,
          prepared->delta,
          representative,
          impl_->budget.sparse_delta,
          ResidentLatentPatchPolicy::omit_points_before_overwrite);
      if (resultant_patch == nullptr) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_sparse_delta_budget_exhausted);
      }
      auto& resultant_component = resultant_patch->next;
      resultant_component.active = true;
      resultant_component.root_id = resultant_root_id;
      release_component_latent_points(resultant_component);
      for (const std::size_t handle : handles) {
        if (handle != representative) {
          auto* patch = ensure_component_patch(
              impl_->state,
              prepared->delta,
              handle,
              impl_->budget.sparse_delta,
              ResidentLatentPatchPolicy::omit_points_before_overwrite);
          if (patch == nullptr) {
            return reject(
                ExactDirectMorseUnifiedResidentPreparationDecision::
                    no_sparse_delta_budget_exhausted);
          }
          patch->next.root_id.reset();
          release_component_latent_points(patch->next);
        }
      }

      std::size_t next_group_record_index = 0U;
      if (add_overflow(
              impl_->state.group_records.size(),
              prepared->delta.new_group_records.size(),
              next_group_record_index)) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_sparse_delta_budget_exhausted);
      }
      prepared->delta.new_group_records.push_back(
          {next_group_record_index,
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
      if (equal.facet_token_index >= impl_->state.components.size()) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_prepared_state_rejected);
      }
      ExactDirectSparseFacetWitness witness;
      if (!checked_next_witness(
              prepared->delta, impl_->authority_id, witness)) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_prepared_state_rejected);
      }
      prepared->bindings.push_back(
          {prepared->bindings.size(),
           impl_->immutable_plan()
               .facet_tokens[equal.facet_token_index]
               .facet_key,
           equal.facet_token_index,
           witness});
      ++bundle.counters.planned_equal_binding_count;
    }

    for (const std::size_t birth_handle : births) {
      if (birth_handle >= impl_->state.components.size()) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_prepared_state_rejected);
      }
      const bool same_batch_equal =
          std::binary_search(
              equal_facets.begin(), equal_facets.end(), birth_handle);
      const std::size_t prior_representative =
          find_component(impl_->state, prepared->delta, birth_handle);
      if (component_after_delta(
              impl_->state, prepared->delta, prior_representative)
              .active &&
          !same_batch_equal) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                contradiction_locator_state_inconsistent);
      }
      if (!component_after_delta(
               impl_->state, prepared->delta, birth_handle)
               .active) {
        auto* birth_patch = ensure_component_patch(
            impl_->state,
            prepared->delta,
            birth_handle,
            impl_->budget.sparse_delta,
            ResidentLatentPatchPolicy::omit_points_before_overwrite);
        if (birth_patch == nullptr) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  no_sparse_delta_budget_exhausted);
        }
        auto& component = birth_patch->next;
        component.active = true;
        component.parent_handle = birth_handle;
        component.root_id.reset();
        if (!assign_component_latent_points(
                *birth_patch,
                key_points(
                    impl_->immutable_plan()
                        .facet_tokens[birth_handle]
                        .facet_key),
                prepared->delta,
                impl_->budget.sparse_delta)) {
          return reject(
              ExactDirectMorseUnifiedResidentPreparationDecision::
                  no_sparse_delta_budget_exhausted);
        }
      }
      ExactDirectSparseFacetWitness witness;
      if (!checked_next_witness(
              prepared->delta, impl_->authority_id, witness)) {
        return reject(
            ExactDirectMorseUnifiedResidentPreparationDecision::
                no_prepared_state_rejected);
      }
      prepared->bindings.push_back(
          {prepared->bindings.size(),
           impl_->immutable_plan().facet_tokens[birth_handle].facet_key,
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
            impl_->budget.locator.maximum_batch_binding_count) {
      return reject(
          ExactDirectMorseUnifiedResidentPreparationDecision::
              no_prepared_state_rejected);
    }
    bool sparse_budget_exhausted = false;
    if (!finalize_resident_delta(
            impl_->state,
            impl_->budget,
            prepared->delta,
            bundle.counters,
            sparse_budget_exhausted)) {
      return reject(
          sparse_budget_exhausted
              ? ExactDirectMorseUnifiedResidentPreparationDecision::
                    no_sparse_delta_budget_exhausted
              : ExactDirectMorseUnifiedResidentPreparationDecision::
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
    bundle.every_locator_miss_has_fresh_exact_miniball =
        bundle.counters.unresolved_locator_probe_count ==
            bundle.counters.fresh_facet_miniball_build_count &&
        bundle.counters.fresh_facet_miniball_build_count ==
            bundle.counters.fresh_facet_miniball_verification_count;
    bundle.rank_window_saturated_h0_authority_certified =
        certified_normalized_strict_closure;
    bundle.every_strict_facet_miss_has_certified_relative_positive_closure =
        bundle.counters.normalized_strict_facet_seed_count ==
        bundle.counters.normalized_strict_facet_carrier_resolution_count;
    bundle.private_strict_facet_closure_attestation_issued =
        prepared->strict_facet_closure_attestation.has_value();
    bundle.strict_facet_closure_bound_to_pre_batch_locator_snapshot =
        bundle.counters.normalized_strict_facet_seed_count == 0U ||
        (prepared->strict_facet_closure_attestation.has_value() &&
         bundle.counters
                 .normalized_strict_facet_closure_locator_snapshot_check_count !=
             0U &&
         prepared->source_stamp == impl_->locator.snapshot_stamp());
    bundle.csr_authorities_share_identity_and_pre_batch_state = true;
    bundle.frozen_batch_receipt.frozen_batch_construction_count = 1U;
    bundle.frozen_batch_receipt.quotient_streaming_verification_count = 1U;
    bundle.frozen_batch_receipt.action_plan_streaming_verification_count =
        1U;
    bundle.frozen_batch_receipt.structural_certification_count = 1U;
    bundle.frozen_batch_receipt.source_plan_immutable_authority_certified =
        true;
    bundle.frozen_batch_receipt.frozen_batch_structurally_certified = true;
    bundle.frozen_batch_receipt.internal_move_only_attestation_issued = true;
    bundle.frozen_batch_receipt
        .independent_expected_batch_freshly_reconstructed = false;
    bundle.frozen_batch_receipt
        .global_facet_coface_or_gamma_catalog_materialized = false;
    bundle.frozen_batch_receipt.supplied_star_global_completeness_claimed =
        false;
    bundle.frozen_batch_receipt.public_status_claimed = false;
    bundle.global_facet_coface_or_gamma_catalog_materialized = false;
    bundle.supplied_star_global_completeness_claimed = false;
    bundle.public_status_claimed = false;
    if (!budget_accepts_authority(impl_->budget, bundle) ||
        !bundle.certified_strict_pre_batch_bundle()) {
      return reject(
          ExactDirectMorseUnifiedResidentPreparationDecision::
              no_prepared_state_rejected);
    }

    prepared->ticket_registry = impl_->ticket_registry;
    ++prepared->ticket_registry->live_ticket_count;
    prepared->owns_ticket_slot = true;
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
  auto consumed_ticket = std::move(ticket.impl_);
  if (consumed_ticket == nullptr || consumed_ticket->consumed) {
    output.ticket_consumed = true;
    output.no_scientific_state_mutated_on_failure = true;
    output.decision =
        ExactDirectMorseUnifiedResidentCommitDecision::
            no_ticket_already_consumed;
    return output;
  }
  consumed_ticket->consumed = true;
  output.ticket_consumed = true;
  if (impl_ == nullptr || consumed_ticket->seal.get() != impl_->seal.get()) {
    output.no_scientific_state_mutated_on_failure = true;
    output.decision =
        ExactDirectMorseUnifiedResidentCommitDecision::
            no_foreign_ticket_rejected;
    return output;
  }
  if (consumed_ticket->source_cursor != impl_->cursor ||
      consumed_ticket->source_epoch != impl_->epoch ||
      consumed_ticket->source_stamp != impl_->locator.snapshot_stamp()) {
    output.no_scientific_state_mutated_on_failure = true;
    output.decision =
        ExactDirectMorseUnifiedResidentCommitDecision::
            no_stale_ticket_rejected;
    return output;
  }

  const auto& delta_counters = consumed_ticket->bundle.counters;
  const bool strict_attestation_matches =
      consumed_ticket->bundle.rank_window_saturated_h0_authority_certified
      ? (delta_counters.normalized_strict_facet_seed_count == 0U
             ? !consumed_ticket->strict_facet_closure_attestation.has_value()
             : consumed_ticket->strict_facet_closure_attestation.has_value() &&
                   consumed_ticket->strict_facet_closure_attestation
                       ->attests_prebatch_locator(
                       consumed_ticket->bundle,
                       consumed_ticket->bindings,
                       impl_->locator,
                       impl_->budget.probe))
      : !consumed_ticket->strict_facet_closure_attestation.has_value();
  if (!consumed_ticket->frozen_batch_attestation.has_value() ||
      !consumed_ticket->frozen_batch_attestation->attests(
          consumed_ticket->bundle.frozen_batch) ||
      !consumed_ticket->bundle.certified_strict_pre_batch_bundle() ||
      !strict_attestation_matches ||
      delta_counters.resident_state_full_copy_count != 0U ||
      delta_counters.sparse_delta_component_patch_count !=
          consumed_ticket->delta.component_patches.size() ||
      delta_counters.sparse_delta_component_latent_point_reference_count !=
          consumed_ticket->delta
              .component_latent_point_materialization_count ||
      delta_counters.sparse_delta_root_replacement_count !=
          consumed_ticket->delta.root_replacements.size() ||
      delta_counters.sparse_delta_new_root_count !=
          consumed_ticket->delta.new_roots.size() ||
      delta_counters.sparse_delta_group_append_count !=
          consumed_ticket->delta.new_group_records.size() ||
      !resident_delta_has_staging_capacity(
          impl_->state, consumed_ticket->delta)) {
    output.no_scientific_state_mutated_on_failure = true;
    output.decision =
        ExactDirectMorseUnifiedResidentCommitDecision::
            no_sparse_delta_staging_rejected;
    return output;
  }

  ResidentDeltaStage staged{
      impl_->state,
      consumed_ticket->delta,
      impl_->budget.maximum_resident_root_count,
      impl_->budget.maximum_group_record_count};
  try {
    staged.stage();
  } catch (...) {
    output.no_scientific_state_mutated_on_failure = true;
    output.decision =
        ExactDirectMorseUnifiedResidentCommitDecision::
            no_sparse_delta_staging_rejected;
    return output;
  }
  output.sparse_delta_staged_with_rollback_before_locator = true;

  output.committed_batch_index = consumed_ticket->source_cursor;
  output.exactly_one_locator_apply_batch_called = true;
  try {
    output.locator_batch = impl_->locator.apply_batch(
        consumed_ticket->queries,
        consumed_ticket->unions,
        consumed_ticket->bindings);
  } catch (const std::bad_alloc&) {
    if (impl_->locator.snapshot_stamp() != consumed_ticket->source_stamp) {
      std::terminate();
    }
    output.no_scientific_state_mutated_on_failure = true;
    output.decision =
        ExactDirectMorseUnifiedResidentCommitDecision::
            no_locator_transaction_rejected;
    return output;
  } catch (const std::length_error&) {
    if (impl_->locator.snapshot_stamp() != consumed_ticket->source_stamp) {
      std::terminate();
    }
    output.no_scientific_state_mutated_on_failure = true;
    output.decision =
        ExactDirectMorseUnifiedResidentCommitDecision::
            no_locator_transaction_rejected;
    return output;
  }
  if (!output.locator_batch.certified_committed_batch()) {
    if (impl_->locator.snapshot_stamp() != consumed_ticket->source_stamp) {
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
      committed_stamp == consumed_ticket->source_stamp ||
      output.locator_batch.counters.union_request_count !=
          consumed_ticket->unions.size() ||
      output.locator_batch.counters.binding_request_count !=
          consumed_ticket->bindings.size()) {
    // The locator has reported a commit.  Any disagreement now is a
    // fail-stop invariant breach: returning an ordinary atomic failure would
    // falsely claim that no scientific mutation occurred.
    std::terminate();
  }

  // The locator commit is now irreversible.  The complete sparse resident
  // delta was already installed through no-allocation swaps/appends, so this
  // suffix contains only noexcept publication scalars.  Any later
  // contradiction is fail-stop and can never be reported as atomic failure.
  staged.release();
  output.staged_sparse_delta_released_after_locator_commit = true;
  impl_->cursor = consumed_ticket->source_cursor + 1U;
  impl_->epoch = consumed_ticket->source_epoch + 1U;
  output.committed_epoch = impl_->epoch;
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
      budget.sparse_delta.maximum_outstanding_ticket_count == 0U ||
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
    auto impl =
        std::make_unique<ExactDirectMorseUnifiedResidentSession::Impl>(
            source_plan);
    auto authority_initialization =
        internal::ExactDirectFrozenUnifiedImmutablePlanAuthorityFactory::
            create(
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
            impl->immutable_plan());
    output.source_plan_initial_verification_count =
        authority_initialization.source_plan_verification_count;
    if (!authority_initialization.source_plan_freshly_verified_once ||
        !authority_initialization.authority.has_value()) {
      output.decision =
          ExactDirectMorseUnifiedResidentInitializationDecision::
              no_source_plan_not_freshly_verified;
      return output;
    }

    impl->source_plan_authority =
        std::move(authority_initialization.authority);
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
    impl->authority_id = session_authority_id;
    impl->locator_instance_id = allocate_locator_instance_id();
    if (impl->locator_instance_id == 0U) {
      output.decision =
          ExactDirectMorseUnifiedResidentInitializationDecision::
              no_session_budget_rejected;
      return output;
    }
    impl->source_plan_initial_verification_count =
        output.source_plan_initial_verification_count;
    impl->seal = std::make_shared<const SessionSeal>(
        SessionSeal{session_authority_id, impl->locator_instance_id});
    impl->ticket_registry = std::make_shared<OutstandingTicketRegistry>(
        OutstandingTicketRegistry{
            0U,
            budget.sparse_delta.maximum_outstanding_ticket_count});
    impl->locator = build_exact_direct_sparse_positive_facet_locator(
        impl->immutable_plan().facet_tokens.size(),
        budget.locator,
        {session_authority_id, ~std::uint64_t{0U}});
    if (!impl->locator.certified_positive_locator()) {
      output.decision =
          ExactDirectMorseUnifiedResidentInitializationDecision::
              no_locator_initialization_rejected;
      return output;
    }
    impl->state.components.resize(
        impl->immutable_plan().facet_tokens.size());
    for (std::size_t index_value = 0U;
         index_value < impl->state.components.size();
         ++index_value) {
      if (impl->immutable_plan()
              .facet_tokens[index_value]
              .facet_token_index !=
          index_value) {
        output.decision =
            ExactDirectMorseUnifiedResidentInitializationDecision::
                no_source_plan_not_freshly_verified;
        return output;
      }
      impl->state.components[index_value].component_handle = index_value;
      impl->state.components[index_value].parent_handle = index_value;
    }
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

ExactDirectMorseUnifiedResidentInitializationResult
initialize_exact_direct_normalized_h0_resident_session(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseGatewayCandidateBudget& source_gateway_budget,
    spatial::LbvhTraversalOrder source_gateway_traversal_order,
    const ExactDirectSparseGatewayCandidateJournalResult& source_gateway,
    const ExactDirectNormalizedH0SourcePlanBudget& source_plan_budget,
    const ExactDirectNormalizedH0SourcePlanResult& source_plan,
    const ExactDirectNormalizedH0ResidentAdapterBudget& adapter_budget,
    std::uint64_t session_authority_id,
    const ExactDirectMorseUnifiedResidentSessionBudget& budget) {
  ExactDirectMorseUnifiedResidentInitializationResult output;
  output.source_kind =
      ExactDirectMorseUnifiedResidentSourceKind::
          normalized_direct_h0_candidate_source;
  if (session_authority_id == 0U ||
      budget.sparse_delta.maximum_outstanding_ticket_count == 0U) {
    output.decision =
        ExactDirectMorseUnifiedResidentInitializationDecision::
            no_session_budget_rejected;
    return output;
  }
  try {
    auto authority_initialization =
        internal::ExactDirectFrozenUnifiedImmutablePlanAuthorityFactory::
            create_from_normalized_direct_source(
                index,
                cloud,
                source_facade,
                source_journal,
                source_arm_budget,
                source_arm_journal,
                source_incidence_budget,
                source_incidence_journal,
                source_gateway_budget,
                source_gateway_traversal_order,
                source_gateway,
                source_plan_budget,
                source_plan,
                adapter_budget,
                budget);
    output.source_plan_initial_verification_count =
        authority_initialization.source_plan_verification_count;
    if (!authority_initialization.normalized_adapter_preflight_certified) {
      output.decision =
          ExactDirectMorseUnifiedResidentInitializationDecision::
              no_normalized_adapter_budget_rejected;
      return output;
    }
    if (!authority_initialization.source_plan_freshly_verified_once) {
      output.decision =
          ExactDirectMorseUnifiedResidentInitializationDecision::
              no_source_plan_not_freshly_verified;
      return output;
    }
    if (!authority_initialization.authority.has_value() ||
        authority_initialization.normalized_immutable_plan == nullptr ||
        !authority_initialization.normalized_adapter_construction_certified) {
      output.decision =
          ExactDirectMorseUnifiedResidentInitializationDecision::
              no_normalized_adapter_construction_rejected;
      return output;
    }
    auto impl =
        std::make_unique<ExactDirectMorseUnifiedResidentSession::Impl>(
            ExactDirectMorseUnifiedResidentSourceKind::
                normalized_direct_h0_candidate_source);
    impl->plan_storage =
        std::move(authority_initialization.normalized_immutable_plan);
    if (!authority_initialization
             .every_normalized_coface_reconstructed_transiently ||
        authority_initialization.normalized_reconstructed_coface_count !=
            source_plan.cofaces.size() ||
        impl->immutable_plan().coface_facet_references.size() !=
            authority_initialization
                .normalized_preflight_coface_facet_occurrence_count) {
      output.decision =
          ExactDirectMorseUnifiedResidentInitializationDecision::
              no_normalized_adapter_construction_rejected;
      return output;
    }

    impl->source_plan_authority =
        std::move(authority_initialization.authority);
    impl->source.cloud = &cloud;
    impl->budget = budget;
    impl->authority_id = session_authority_id;
    impl->locator_instance_id = allocate_locator_instance_id();
    if (impl->locator_instance_id == 0U) {
      output.decision =
          ExactDirectMorseUnifiedResidentInitializationDecision::
              no_session_budget_rejected;
      return output;
    }
    impl->source_plan_initial_verification_count =
        output.source_plan_initial_verification_count;
    impl->normalized_adapter_construction_certified =
        authority_initialization.normalized_adapter_construction_certified;
    impl->every_normalized_coface_reconstructed_transiently =
        authority_initialization
            .every_normalized_coface_reconstructed_transiently;
    impl->normalized_retraction_mode =
        ExactDirectNormalizedH0ResidentRetractionMode::
            candidate_fail_open_without_h0_retraction_authority;
    impl->normalized_h0_retraction_authority_certified = false;
    impl->seal = std::make_shared<const SessionSeal>(
        SessionSeal{session_authority_id, impl->locator_instance_id});
    impl->ticket_registry = std::make_shared<OutstandingTicketRegistry>(
        OutstandingTicketRegistry{
            0U,
            budget.sparse_delta.maximum_outstanding_ticket_count});
    impl->locator = build_exact_direct_sparse_positive_facet_locator(
        impl->immutable_plan().facet_tokens.size(),
        budget.locator,
        {session_authority_id, ~std::uint64_t{0U}});
    if (!impl->locator.certified_positive_locator()) {
      output.decision =
          ExactDirectMorseUnifiedResidentInitializationDecision::
              no_locator_initialization_rejected;
      return output;
    }
    impl->state.components.resize(
        impl->immutable_plan().facet_tokens.size());
    for (std::size_t component_index = 0U;
         component_index < impl->state.components.size();
         ++component_index) {
      if (impl->immutable_plan()
              .facet_tokens[component_index]
              .facet_token_index !=
          component_index) {
        output.decision =
            ExactDirectMorseUnifiedResidentInitializationDecision::
                no_normalized_adapter_construction_rejected;
        return output;
      }
      impl->state.components[component_index].component_handle =
          component_index;
      impl->state.components[component_index].parent_handle =
          component_index;
    }
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
    output.normalized_source_plan_consumed_directly = true;
    output.normalized_sparse_compatibility_plan_certified = true;
    output.every_normalized_coface_reconstructed_transiently =
        authority_initialization
            .every_normalized_coface_reconstructed_transiently;
    output.normalized_h0_retraction_mode =
        ExactDirectNormalizedH0ResidentRetractionMode::
            candidate_fail_open_without_h0_retraction_authority;
    output.normalized_h0_retraction_authority_certified = false;
    output.normalized_candidate_fails_open_on_strictly_earlier_facet = true;
    output.successive_incidence_star_materialized_by_adapter = false;
    output.global_regularity_authority_certified = false;
    output.omitted_late_cofaces_qr1_noop_certified = false;
    output.normalized_verticality_certified = false;
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
            no_normalized_adapter_budget_rejected;
    return output;
  } catch (const std::logic_error&) {
    output.decision =
        ExactDirectMorseUnifiedResidentInitializationDecision::
            no_normalized_adapter_construction_rejected;
    return output;
  }
}

ExactDirectMorseUnifiedResidentInitializationResult
initialize_exact_direct_normalized_h0_certified_resident_session(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseGatewayCandidateBudget& source_gateway_budget,
    spatial::LbvhTraversalOrder source_gateway_traversal_order,
    const ExactDirectSparseGatewayCandidateJournalResult& source_gateway,
    const ExactDirectNormalizedH0SourcePlanBudget& source_plan_budget,
    const ExactDirectNormalizedH0SourcePlanResult& source_plan,
    const ExactDirectRankWindowSaturatedH0Authority&
        source_rank_window_authority,
    const ExactDirectSparseFacetDescentClosureBudget&
        strict_facet_closure_budget,
    const ExactDirectSparseFacetDescentClosureConfig&
        strict_facet_closure_config,
    spatial::LbvhTraversalOrder strict_facet_closure_traversal_order,
    const ExactDirectNormalizedH0ResidentAdapterBudget& adapter_budget,
    std::uint64_t session_authority_id,
    const ExactDirectMorseUnifiedResidentSessionBudget& budget) {
  ExactDirectMorseUnifiedResidentInitializationResult rejected;
  rejected.source_kind =
      ExactDirectMorseUnifiedResidentSourceKind::
          normalized_direct_h0_candidate_source;
  const auto& requirements = source_facade.certificate.requirements;
  if (source_facade.terminal_catalog_certified() &&
      requirements.point_count == cloud.size() &&
      requirements.effective_maximum_order == cloud.size()) {
    rejected.decision =
        ExactDirectMorseUnifiedResidentInitializationDecision::
            no_normalized_terminal_order_equal_point_count_unsupported;
    return rejected;
  }
  if (!closure_budget_within_confidence_caps(strict_facet_closure_budget) ||
      !valid_closure_traversal_order(
          strict_facet_closure_traversal_order)) {
    rejected.decision =
        ExactDirectMorseUnifiedResidentInitializationDecision::
            no_strict_facet_closure_configuration_rejected;
    return rejected;
  }

  // Reuse the unchanged candidate initializer for the one normalized-source
  // replay, compatibility cursor construction, locator and resident arenas.
  // Only after that succeeds is the private certified carrier capability
  // issued and installed.
  auto output = initialize_exact_direct_normalized_h0_resident_session(
      index,
      cloud,
      source_facade,
      source_journal,
      source_arm_budget,
      source_arm_journal,
      source_incidence_budget,
      source_incidence_journal,
      source_gateway_budget,
      source_gateway_traversal_order,
      source_gateway,
      source_plan_budget,
      source_plan,
      adapter_budget,
      session_authority_id,
      budget);
  if (!output.session.has_value()) {
    return output;
  }

  const auto rank_verification =
      verify_exact_direct_rank_window_saturated_h0_authority(
          source_facade, source_rank_window_authority);
  if (!rank_verification.result_certified) {
    output.session.reset();
    output.decision =
        ExactDirectMorseUnifiedResidentInitializationDecision::
            no_rank_window_saturated_h0_authority_rejected;
    return output;
  }

  auto& session = *output.session;
  auto& impl = *session.impl_;
  auto closure_authority =
      ResidentNormalizedStrictFacetClosureAuthority::issue(
          impl.immutable_plan(),
          source_rank_window_authority,
          rank_verification,
          strict_facet_closure_budget,
          strict_facet_closure_config,
          strict_facet_closure_traversal_order,
          session_authority_id);
  if (!closure_authority.has_value()) {
    output.session.reset();
    output.decision =
        ExactDirectMorseUnifiedResidentInitializationDecision::
            no_rank_window_saturated_h0_authority_rejected;
    return output;
  }
  impl.source.index = &index;
  impl.source.cloud = &cloud;
  impl.normalized_strict_facet_closure_authority.emplace(
      std::move(*closure_authority));
  impl.normalized_retraction_mode =
      ExactDirectNormalizedH0ResidentRetractionMode::
          certified_rank_window_and_sparse_strict_facet_closure;
  impl.normalized_h0_retraction_authority_certified = true;
  output.normalized_h0_retraction_mode = impl.normalized_retraction_mode;
  output.normalized_h0_retraction_authority_certified = true;
  output.normalized_candidate_fails_open_on_strictly_earlier_facet = false;
  output.rank_window_saturated_h0_authority_freshly_verified = true;
  output.strict_facet_closure_session_capability_issued = true;
  if (!session.certified_resident_session() ||
      !output.certified_initialized_session()) {
    output.session.reset();
    output.decision =
        ExactDirectMorseUnifiedResidentInitializationDecision::
            no_strict_facet_closure_configuration_rejected;
  }
  return output;
}

ExactDirectMorseUnifiedResidentInitializationResult
initialize_exact_direct_normalized_h0_incidence_complete_resident_session(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& source_incidence_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult&
        source_incidence_journal,
    const ExactDirectSparseGatewayCandidateBudget& source_gateway_budget,
    spatial::LbvhTraversalOrder source_gateway_traversal_order,
    const ExactDirectSparseGatewayCandidateJournalResult& source_gateway,
    const ExactDirectNormalizedH0SourcePlanBudget& source_plan_budget,
    const ExactDirectNormalizedH0SourcePlanResult& source_plan,
    const ExactDirectRankWindowSaturatedH0Authority&
        source_rank_window_authority,
    const ExactDirectNormalizedH0IncidenceReductionAuthority&
        source_incidence_reduction_authority,
    const ExactDirectSparseFacetDescentClosureBudget&
        strict_facet_closure_budget,
    const ExactDirectSparseFacetDescentClosureConfig&
        strict_facet_closure_config,
    spatial::LbvhTraversalOrder strict_facet_closure_traversal_order,
    const ExactDirectNormalizedH0ResidentAdapterBudget& adapter_budget,
    std::uint64_t session_authority_id,
    const ExactDirectMorseUnifiedResidentSessionBudget& budget) {
  ExactDirectMorseUnifiedResidentInitializationResult rejected;
  rejected.source_kind =
      ExactDirectMorseUnifiedResidentSourceKind::
          normalized_direct_h0_candidate_source;
  const auto& requirements = source_facade.certificate.requirements;
  if (source_facade.terminal_catalog_certified() &&
      requirements.point_count == cloud.size() &&
      requirements.effective_maximum_order == cloud.size()) {
    rejected.decision =
        ExactDirectMorseUnifiedResidentInitializationDecision::
            no_normalized_terminal_order_equal_point_count_unsupported;
    return rejected;
  }
  if (!closure_budget_within_confidence_caps(strict_facet_closure_budget) ||
      !valid_closure_traversal_order(
          strict_facet_closure_traversal_order)) {
    rejected.decision =
        ExactDirectMorseUnifiedResidentInitializationDecision::
            no_strict_facet_closure_configuration_rejected;
    return rejected;
  }
  if (session_authority_id == 0U ||
      budget.sparse_delta.maximum_outstanding_ticket_count == 0U) {
    rejected.decision =
        ExactDirectMorseUnifiedResidentInitializationDecision::
            no_session_budget_rejected;
    return rejected;
  }

  try {
    // This replay is deliberately independent of the one compatibility-plan
    // verification retained by the resident initializer.  It recursively
    // reconstructs the fixed-size horizontal source certificate before any
    // resident session or private capability can become observable.
    const auto incidence_verification =
        verify_exact_direct_normalized_h0_incidence_reduction_authority(
            index,
            cloud,
            source_facade,
            source_journal,
            source_arm_budget,
            source_arm_journal,
            source_incidence_budget,
            source_incidence_journal,
            source_gateway_budget,
            source_gateway_traversal_order,
            source_gateway,
            source_plan_budget,
            source_plan,
            source_rank_window_authority,
            source_incidence_reduction_authority);
    if (!incidence_verification.result_certified ||
        !source_incidence_reduction_authority
             .certified_horizontal_incidence_reduction() ||
        !source_incidence_reduction_authority
             .incidence_complete_reduction_proved) {
      rejected.decision =
          ExactDirectMorseUnifiedResidentInitializationDecision::
              no_normalized_h0_incidence_reduction_authority_rejected;
      return rejected;
    }

    // Reuse, without modifying, the existing rank-window + closure factory.
    // It retains its historical mode and output whenever called directly.
    auto output = initialize_exact_direct_normalized_h0_certified_resident_session(
        index,
        cloud,
        source_facade,
        source_journal,
        source_arm_budget,
        source_arm_journal,
        source_incidence_budget,
        source_incidence_journal,
        source_gateway_budget,
        source_gateway_traversal_order,
        source_gateway,
        source_plan_budget,
        source_plan,
        source_rank_window_authority,
        strict_facet_closure_budget,
        strict_facet_closure_config,
        strict_facet_closure_traversal_order,
        adapter_budget,
        session_authority_id,
        budget);
    if (!output.session.has_value()) {
      return output;
    }

    auto& session = *output.session;
    auto& impl = *session.impl_;
    auto incidence_authority =
        ResidentNormalizedHorizontalIncidenceReductionAuthority::issue(
            impl.immutable_plan(),
            source_plan,
            source_incidence_reduction_authority,
            incidence_verification,
            session_authority_id);
    if (!incidence_authority.has_value()) {
      output.session.reset();
      output.decision =
          ExactDirectMorseUnifiedResidentInitializationDecision::
              no_normalized_h0_incidence_reduction_authority_rejected;
      return output;
    }
    impl.normalized_horizontal_incidence_reduction_authority.emplace(
        std::move(*incidence_authority));
    impl.normalized_retraction_mode =
        ExactDirectNormalizedH0ResidentRetractionMode::
            certified_horizontal_incidence_complete_rank_window_and_sparse_strict_facet_closure;
    output.normalized_h0_retraction_mode = impl.normalized_retraction_mode;
    output.normalized_h0_incidence_reduction_authority_freshly_verified =
        true;
    output.normalized_h0_incidence_reduction_session_capability_issued = true;
    output.incidence_complete_reduction_proved = true;
    if (!session.certified_resident_session() ||
        !session.normalized_horizontal_incidence_reduction_certified() ||
        !output.certified_initialized_session()) {
      output.session.reset();
      output.normalized_h0_incidence_reduction_session_capability_issued =
          false;
      output.incidence_complete_reduction_proved = false;
      output.decision =
          ExactDirectMorseUnifiedResidentInitializationDecision::
              no_normalized_h0_incidence_reduction_authority_rejected;
    }
    return output;
  } catch (const std::bad_alloc&) {
    rejected.decision =
        ExactDirectMorseUnifiedResidentInitializationDecision::
            no_allocation_failed;
    return rejected;
  } catch (const std::length_error&) {
    rejected.decision =
        ExactDirectMorseUnifiedResidentInitializationDecision::
            no_normalized_h0_incidence_reduction_authority_rejected;
    return rejected;
  } catch (const std::logic_error&) {
    rejected.decision =
        ExactDirectMorseUnifiedResidentInitializationDecision::
            no_normalized_h0_incidence_reduction_authority_rejected;
    return rejected;
  }
}

}  // namespace morsehgp3d::hierarchy
