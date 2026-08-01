#pragma once

#include "morsehgp3d/hierarchy/direct_frozen_incidence_hgp_action_plan.hpp"

#include <optional>
#include <span>
#include <utility>

namespace morsehgp3d::hierarchy::internal {

class ExactFrozenIncidenceVerifiedQuotientAuthorityFactory;

class ExactFrozenIncidenceVerifiedQuotientAuthority final {
 public:
  ExactFrozenIncidenceVerifiedQuotientAuthority(
      ExactFrozenIncidenceVerifiedQuotientAuthority&& other) noexcept
      : hyperedges_(std::exchange(
            other.hyperedges_,
            std::span<const ExactFrozenIncidenceHyperedge>{})),
        tokens_(std::exchange(
            other.tokens_, std::span<const ExactFrozenIncidenceToken>{})),
        quotient_(std::exchange(other.quotient_, nullptr)),
        budget_(other.budget_),
        certified_(std::exchange(other.certified_, false)) {}
  ExactFrozenIncidenceVerifiedQuotientAuthority& operator=(
      ExactFrozenIncidenceVerifiedQuotientAuthority&&) = delete;
  ExactFrozenIncidenceVerifiedQuotientAuthority(
      const ExactFrozenIncidenceVerifiedQuotientAuthority&) = delete;
  ExactFrozenIncidenceVerifiedQuotientAuthority& operator=(
      const ExactFrozenIncidenceVerifiedQuotientAuthority&) = delete;

  [[nodiscard]] bool valid() const noexcept {
    return certified_ && quotient_ != nullptr;
  }
  [[nodiscard]] std::span<const ExactFrozenIncidenceHyperedge> hyperedges()
      const noexcept { return hyperedges_; }
  [[nodiscard]] std::span<const ExactFrozenIncidenceToken> tokens()
      const noexcept { return tokens_; }
  [[nodiscard]] const ExactFrozenIncidenceQuotientBudget& budget()
      const noexcept { return budget_; }
  [[nodiscard]] const ExactFrozenIncidenceQuotientResult& quotient()
      const noexcept { return *quotient_; }

 private:
  ExactFrozenIncidenceVerifiedQuotientAuthority(
      std::span<const ExactFrozenIncidenceHyperedge> hyperedges,
      std::span<const ExactFrozenIncidenceToken> tokens,
      const ExactFrozenIncidenceQuotientBudget& budget,
      const ExactFrozenIncidenceQuotientResult* quotient) noexcept
      : hyperedges_(hyperedges),
        tokens_(tokens),
        quotient_(quotient),
        budget_(budget),
        certified_(quotient != nullptr) {}

  std::span<const ExactFrozenIncidenceHyperedge> hyperedges_;
  std::span<const ExactFrozenIncidenceToken> tokens_;
  const ExactFrozenIncidenceQuotientResult* quotient_{};
  ExactFrozenIncidenceQuotientBudget budget_{};
  bool certified_{false};

  friend class ExactFrozenIncidenceVerifiedQuotientAuthorityFactory;
};

class ExactFrozenIncidenceVerifiedQuotientAuthorityFactory final {
 public:
  [[nodiscard]] static
      std::optional<ExactFrozenIncidenceVerifiedQuotientAuthority>
      verify_once(
          std::span<const ExactFrozenIncidenceHyperedge> hyperedges,
          std::span<const ExactFrozenIncidenceToken> tokens,
          const ExactFrozenIncidenceQuotientBudget& budget,
          const ExactFrozenIncidenceQuotientResult& quotient);
};

[[nodiscard]] ExactFrozenIncidenceHgpActionPlanResult
build_exact_direct_frozen_incidence_hgp_action_plan_from_verified_quotient(
    const ExactFrozenIncidenceVerifiedQuotientAuthority& authority,
    std::span<const ExactFrozenIncidenceHyperedgeProvenance> provenance,
    std::span<const ExactFrozenIncidenceRootAttachment> root_attachments,
    const ExactFrozenIncidenceHgpActionPlanBudget& budget);

[[nodiscard]] ExactFrozenIncidenceHgpActionPlanStreamingVerification
verify_exact_direct_frozen_incidence_hgp_action_plan_streaming_from_verified_quotient(
    const ExactFrozenIncidenceVerifiedQuotientAuthority& authority,
    std::span<const ExactFrozenIncidenceHyperedgeProvenance> provenance,
    std::span<const ExactFrozenIncidenceRootAttachment> root_attachments,
    const ExactFrozenIncidenceHgpActionPlanBudget& trusted_budget,
    const ExactFrozenIncidenceHgpActionPlanResult& observed);

}  // namespace morsehgp3d::hierarchy::internal
