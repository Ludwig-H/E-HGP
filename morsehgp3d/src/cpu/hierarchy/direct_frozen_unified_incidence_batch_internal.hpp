#pragma once

#include "morsehgp3d/hierarchy/direct_frozen_unified_incidence_batch.hpp"
#include "morsehgp3d/hierarchy/direct_morse_unified_resident_session.hpp"
#include "morsehgp3d/hierarchy/direct_normalized_h0_source_plan.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <utility>

namespace morsehgp3d::hierarchy::internal {

enum class ExactDirectFrozenUnifiedImmutablePlanAuthorityKind
    : std::uint8_t {
  successive_incidence_star,
  normalized_direct_h0_candidate_source,
};

// This lease is deliberately confined to the two implementation translation
// units that need it.  Its constructor is private: the only construction path
// performs the complete unified-plan verification against the immutable plan
// owned by the resident session.
class ExactDirectFrozenUnifiedImmutablePlanAuthority final {
 public:
  ExactDirectFrozenUnifiedImmutablePlanAuthority(
      ExactDirectFrozenUnifiedImmutablePlanAuthority&& other) noexcept
      : plan_(std::exchange(other.plan_, nullptr)),
        kind_(other.kind_),
        certified_(std::exchange(other.certified_, false)) {}
  ExactDirectFrozenUnifiedImmutablePlanAuthority& operator=(
      ExactDirectFrozenUnifiedImmutablePlanAuthority&& other) noexcept {
    if (this != &other) {
      plan_ = std::exchange(other.plan_, nullptr);
      kind_ = other.kind_;
      certified_ = std::exchange(other.certified_, false);
    }
    return *this;
  }
  ExactDirectFrozenUnifiedImmutablePlanAuthority(
      const ExactDirectFrozenUnifiedImmutablePlanAuthority&) = delete;
  ExactDirectFrozenUnifiedImmutablePlanAuthority& operator=(
      const ExactDirectFrozenUnifiedImmutablePlanAuthority&) = delete;

  [[nodiscard]] bool certifies(
      const ExactDirectSparseUnifiedLevelPlanResult& plan) const noexcept {
    return certified_ && plan_ == &plan && plan_ != nullptr;
  }

  [[nodiscard]] bool valid() const noexcept {
    return certified_ && plan_ != nullptr;
  }

  [[nodiscard]] ExactDirectFrozenUnifiedImmutablePlanAuthorityKind kind()
      const noexcept {
    return kind_;
  }

  [[nodiscard]] const ExactDirectSparseUnifiedLevelPlanResult& plan()
      const noexcept {
    return *plan_;
  }

 private:
  explicit ExactDirectFrozenUnifiedImmutablePlanAuthority(
      const ExactDirectSparseUnifiedLevelPlanResult* plan,
      ExactDirectFrozenUnifiedImmutablePlanAuthorityKind kind) noexcept
      : plan_(plan), kind_(kind), certified_(plan != nullptr) {}

  const ExactDirectSparseUnifiedLevelPlanResult* plan_{};
  ExactDirectFrozenUnifiedImmutablePlanAuthorityKind kind_{
      ExactDirectFrozenUnifiedImmutablePlanAuthorityKind::
          successive_incidence_star};
  bool certified_{false};

  friend class ExactDirectFrozenUnifiedImmutablePlanAuthorityFactory;
};

struct ExactDirectFrozenUnifiedImmutablePlanAuthorityInitialization {
  std::optional<ExactDirectFrozenUnifiedImmutablePlanAuthority> authority;
  std::unique_ptr<const ExactDirectSparseUnifiedLevelPlanResult>
      normalized_immutable_plan;
  std::size_t source_plan_verification_count{};
  std::size_t normalized_preflight_coface_facet_occurrence_count{};
  std::size_t normalized_reconstructed_coface_count{};
  bool normalized_adapter_preflight_certified{false};
  bool normalized_adapter_construction_certified{false};
  bool every_normalized_coface_reconstructed_transiently{false};
  bool source_plan_freshly_verified_once{false};
};

class ExactDirectFrozenUnifiedImmutablePlanAuthorityFactory final {
 public:
  [[nodiscard]] static
      ExactDirectFrozenUnifiedImmutablePlanAuthorityInitialization
      create(
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
          const ExactDirectSparseSuccessiveIncidenceStarJournalBudget&
              source_star_budget,
          spatial::LbvhTraversalOrder source_star_traversal_order,
          const ExactDirectSparseSuccessiveIncidenceStarJournalResult&
              source_star,
          const ExactDirectSparseUnifiedLevelPlanBudget& source_plan_budget,
          const ExactDirectSparseUnifiedLevelPlanResult& immutable_plan);

  [[nodiscard]] static
      ExactDirectFrozenUnifiedImmutablePlanAuthorityInitialization
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
          const ExactDirectSparseGatewayCandidateBudget&
              source_gateway_budget,
          spatial::LbvhTraversalOrder source_gateway_traversal_order,
          const ExactDirectSparseGatewayCandidateJournalResult&
              source_gateway,
          const ExactDirectNormalizedH0SourcePlanBudget& source_plan_budget,
          const ExactDirectNormalizedH0SourcePlanResult& source_plan,
          const ExactDirectNormalizedH0ResidentAdapterBudget& adapter_budget,
          const ExactDirectMorseUnifiedResidentSessionBudget& session_budget);
};

class ExactDirectFrozenUnifiedResidentBatchAttestedBuilder;

// This capability is tied to the stable address of the batch stored in a
// resident ticket.  It cannot be copied or constructed by callers.  Moving a
// ticket moves only its owning unique_ptr, so the attested address remains
// stable through prepare, validation and commit.
class ExactDirectFrozenUnifiedResidentBatchAttestation final {
 public:
  ExactDirectFrozenUnifiedResidentBatchAttestation(
      ExactDirectFrozenUnifiedResidentBatchAttestation&& other) noexcept
      : batch_(std::exchange(other.batch_, nullptr)),
        certified_(std::exchange(other.certified_, false)) {}
  ExactDirectFrozenUnifiedResidentBatchAttestation& operator=(
      ExactDirectFrozenUnifiedResidentBatchAttestation&& other) noexcept {
    if (this != &other) {
      batch_ = std::exchange(other.batch_, nullptr);
      certified_ = std::exchange(other.certified_, false);
    }
    return *this;
  }
  ExactDirectFrozenUnifiedResidentBatchAttestation(
      const ExactDirectFrozenUnifiedResidentBatchAttestation&) = delete;
  ExactDirectFrozenUnifiedResidentBatchAttestation& operator=(
      const ExactDirectFrozenUnifiedResidentBatchAttestation&) = delete;

  [[nodiscard]] bool attests(
      const ExactDirectFrozenUnifiedIncidenceBatchResult& batch)
      const noexcept {
    return certified_ && batch_ == &batch &&
           batch.schema_version ==
               direct_frozen_unified_incidence_batch_schema_version &&
           batch.decision == ExactDirectFrozenUnifiedIncidenceBatchDecision::
                                 complete_certified_frozen_unified_incidence_batch &&
           !batch.source_plan_freshly_verified &&
           batch.source_plan_verified_once_by_immutable_resident_authority &&
           batch.frozen_quotient_freshly_streaming_verified &&
           batch.frozen_hgp_action_plan_freshly_streaming_verified;
  }

 private:
  explicit ExactDirectFrozenUnifiedResidentBatchAttestation(
      const ExactDirectFrozenUnifiedIncidenceBatchResult* batch) noexcept
      : batch_(batch), certified_(batch != nullptr) {}

  const ExactDirectFrozenUnifiedIncidenceBatchResult* batch_{};
  bool certified_{false};

  friend class ExactDirectFrozenUnifiedResidentBatchAttestedBuilder;
};

class ExactDirectFrozenUnifiedResidentBatchAttestedBuilder final {
 public:
  // Exactly one scientific batch construction is performed.  The shared core
  // performs exactly one streaming quotient verification and exactly one
  // streaming action-plan verification before a capability can be issued.
  [[nodiscard]] static
      std::optional<ExactDirectFrozenUnifiedResidentBatchAttestation>
      build_once(
          const ExactDirectFrozenUnifiedImmutablePlanAuthority& authority,
          std::size_t batch_index,
          std::span<const ExactDirectFrozenUnifiedFacetResolution>
              facet_resolutions,
          std::span<const ExactDirectFrozenUnifiedPriorRootCoverage>
              prior_root_coverages,
          std::span<const spatial::PointId>
              prior_root_coverage_point_references,
          std::span<const ExactDirectFrozenUnifiedLatentCarrierCoverage>
              latent_carrier_coverages,
          std::span<const spatial::PointId>
              latent_carrier_coverage_point_references,
          const ExactDirectFrozenUnifiedIncidenceBatchBudget& budget,
          ExactDirectFrozenUnifiedIncidenceBatchResult& destination);
};

}  // namespace morsehgp3d::hierarchy::internal
