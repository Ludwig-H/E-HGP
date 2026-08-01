#pragma once

#include "morsehgp3d/hierarchy/direct_frozen_unified_incidence_batch.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <utility>

namespace morsehgp3d::hierarchy::internal {

// This lease is deliberately confined to the two implementation translation
// units that need it.  Its constructor is private: the only construction path
// performs the complete unified-plan verification against the immutable plan
// owned by the resident session.
class ExactDirectFrozenUnifiedImmutablePlanAuthority final {
 public:
  ExactDirectFrozenUnifiedImmutablePlanAuthority(
      ExactDirectFrozenUnifiedImmutablePlanAuthority&& other) noexcept
      : plan_(std::exchange(other.plan_, nullptr)),
        certified_(std::exchange(other.certified_, false)) {}
  ExactDirectFrozenUnifiedImmutablePlanAuthority& operator=(
      ExactDirectFrozenUnifiedImmutablePlanAuthority&& other) noexcept {
    if (this != &other) {
      plan_ = std::exchange(other.plan_, nullptr);
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

  [[nodiscard]] const ExactDirectSparseUnifiedLevelPlanResult& plan()
      const noexcept {
    return *plan_;
  }

 private:
  explicit ExactDirectFrozenUnifiedImmutablePlanAuthority(
      const ExactDirectSparseUnifiedLevelPlanResult* plan) noexcept
      : plan_(plan), certified_(plan != nullptr) {}

  const ExactDirectSparseUnifiedLevelPlanResult* plan_{};
  bool certified_{false};

  friend class ExactDirectFrozenUnifiedImmutablePlanAuthorityFactory;
};

struct ExactDirectFrozenUnifiedImmutablePlanAuthorityInitialization {
  std::optional<ExactDirectFrozenUnifiedImmutablePlanAuthority> authority;
  std::size_t source_plan_verification_count{};
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
};

[[nodiscard]] ExactDirectFrozenUnifiedIncidenceBatchResult
build_exact_direct_frozen_unified_incidence_batch_from_immutable_authority(
    const ExactDirectFrozenUnifiedImmutablePlanAuthority& authority,
    std::size_t batch_index,
    std::span<const ExactDirectFrozenUnifiedFacetResolution>
        facet_resolutions,
    std::span<const ExactDirectFrozenUnifiedPriorRootCoverage>
        prior_root_coverages,
    std::span<const spatial::PointId> prior_root_coverage_point_references,
    std::span<const ExactDirectFrozenUnifiedLatentCarrierCoverage>
        latent_carrier_coverages,
    std::span<const spatial::PointId>
        latent_carrier_coverage_point_references,
    const ExactDirectFrozenUnifiedIncidenceBatchBudget& budget);

[[nodiscard]] ExactDirectFrozenUnifiedIncidenceBatchVerification
verify_exact_direct_frozen_unified_incidence_batch_from_immutable_authority(
    const ExactDirectFrozenUnifiedImmutablePlanAuthority& authority,
    std::size_t batch_index,
    std::span<const ExactDirectFrozenUnifiedFacetResolution>
        facet_resolutions,
    std::span<const ExactDirectFrozenUnifiedPriorRootCoverage>
        prior_root_coverages,
    std::span<const spatial::PointId> prior_root_coverage_point_references,
    std::span<const ExactDirectFrozenUnifiedLatentCarrierCoverage>
        latent_carrier_coverages,
    std::span<const spatial::PointId>
        latent_carrier_coverage_point_references,
    const ExactDirectFrozenUnifiedIncidenceBatchBudget& trusted_budget,
    const ExactDirectFrozenUnifiedIncidenceBatchResult& observed);

}  // namespace morsehgp3d::hierarchy::internal
