#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace morsehgp3d::exact {

enum class PredicateSign : int {
  negative = -1,
  zero = 0,
  positive = 1,
};

enum class FilterState {
  certified,
  uncertain,
};

enum class CertificationStage {
  fp64_filtered,
  expansion,
  cpu_multiprecision,
};

enum class PredicateFilterPolicy {
  // Preserve the original 2A.3 values and behavior for existing callers.
  allow_fp64 = 0,
  multiprecision_only = 1,
  allow_adaptive = 2,
};

[[nodiscard]] inline PredicateSign predicate_sign(int value) noexcept;

class FilterResult {
 public:
  FilterResult() = delete;

  [[nodiscard]] static FilterResult uncertain() noexcept {
    return FilterResult(FilterState::uncertain, std::nullopt);
  }

  [[nodiscard]] static FilterResult certified(PredicateSign value) {
    if (value == PredicateSign::zero) {
      throw std::invalid_argument("a floating filter cannot certify an exact zero");
    }
    if (value != PredicateSign::negative && value != PredicateSign::positive) {
      throw std::invalid_argument("a floating filter sign is invalid");
    }
    return FilterResult(FilterState::certified, value);
  }

  [[nodiscard]] FilterState state() const noexcept { return state_; }
  [[nodiscard]] const std::optional<PredicateSign>& sign() const noexcept { return sign_; }

 private:
  FilterResult(FilterState state, std::optional<PredicateSign> sign) noexcept
      : state_(state), sign_(sign) {}

  FilterState state_;
  std::optional<PredicateSign> sign_;
};

// Unlike the interval filter, an exact floating expansion can also certify a
// zero. Keeping this result distinct from FilterResult prevents an interval
// that merely contains zero from being mistaken for an exact cancellation.
class ExpansionResult {
 public:
  ExpansionResult() = delete;

  [[nodiscard]] static ExpansionResult uncertain() noexcept {
    return ExpansionResult(FilterState::uncertain, std::nullopt);
  }

  [[nodiscard]] static ExpansionResult certified(PredicateSign value) {
    if (value != PredicateSign::negative && value != PredicateSign::zero &&
        value != PredicateSign::positive) {
      throw std::invalid_argument("an expansion sign is invalid");
    }
    return ExpansionResult(FilterState::certified, value);
  }

  [[nodiscard]] FilterState state() const noexcept { return state_; }
  [[nodiscard]] const std::optional<PredicateSign>& sign() const noexcept {
    return sign_;
  }

 private:
  ExpansionResult(FilterState state, std::optional<PredicateSign> sign) noexcept
      : state_(state), sign_(sign) {}

  FilterState state_;
  std::optional<PredicateSign> sign_;
};

class PredicateDecision {
 public:
  PredicateDecision() = delete;

  PredicateDecision(PredicateSign sign, CertificationStage certification_stage)
      : sign_(sign), certification_stage_(certification_stage) {
    if (sign != PredicateSign::negative && sign != PredicateSign::zero &&
        sign != PredicateSign::positive) {
      throw std::invalid_argument("predicate decision sign is invalid");
    }
    if (certification_stage != CertificationStage::fp64_filtered &&
        certification_stage != CertificationStage::expansion &&
        certification_stage != CertificationStage::cpu_multiprecision) {
      throw std::invalid_argument("predicate certification stage is invalid");
    }
    if (sign == PredicateSign::zero &&
        certification_stage == CertificationStage::fp64_filtered) {
      throw std::invalid_argument("an fp64 filter cannot certify an exact zero");
    }
  }

  [[nodiscard]] PredicateSign sign() const noexcept { return sign_; }
  [[nodiscard]] CertificationStage certification_stage() const noexcept {
    return certification_stage_;
  }

 private:
  PredicateSign sign_;
  CertificationStage certification_stage_;
};

class PredicateCounters {
 public:
  void record_fp32_proposal() noexcept { ++fp32_proposals_; }

  void record_certification(const PredicateDecision& decision) noexcept {
    record_certification_stage(
        decision.certification_stage(), decision.sign() == PredicateSign::zero);
  }

  // Collective classifications such as a three-plane rank decision have one
  // terminal authority but no single scientific sign. This entry point keeps
  // their counters honest without inventing a synthetic PredicateDecision.
  void record_certification_stage(
      CertificationStage certification_stage,
      bool exact_zero = false) noexcept {
    switch (certification_stage) {
      case CertificationStage::fp64_filtered:
        ++fp64_filtered_certified_;
        break;
      case CertificationStage::expansion:
        ++expansion_certified_;
        break;
      case CertificationStage::cpu_multiprecision:
        ++cpu_multiprecision_certified_;
        break;
    }
    if (exact_zero) {
      ++exact_zeros_;
    }
  }

  [[nodiscard]] std::uint64_t fp32_proposals() const noexcept {
    return fp32_proposals_;
  }
  [[nodiscard]] std::uint64_t fp64_filtered_certified() const noexcept {
    return fp64_filtered_certified_;
  }
  [[nodiscard]] std::uint64_t expansion_certified() const noexcept {
    return expansion_certified_;
  }
  [[nodiscard]] std::uint64_t cpu_multiprecision_certified() const noexcept {
    return cpu_multiprecision_certified_;
  }
  [[nodiscard]] std::uint64_t exact_zeros() const noexcept { return exact_zeros_; }
  [[nodiscard]] std::uint64_t remaining_unknown() const noexcept {
    return remaining_unknown_;
  }

  [[nodiscard]] std::uint64_t certified_decisions() const noexcept {
    return fp64_filtered_certified_ + expansion_certified_ +
           cpu_multiprecision_certified_;
  }

 private:
  std::uint64_t fp32_proposals_{0};
  std::uint64_t fp64_filtered_certified_{0};
  std::uint64_t expansion_certified_{0};
  std::uint64_t cpu_multiprecision_certified_{0};
  std::uint64_t exact_zeros_{0};
  // Terminally unresolved decisions only; the certified exact-only slice never increments it.
  std::uint64_t remaining_unknown_{0};
};

namespace detail {

[[nodiscard]] inline PredicateDecision multiprecision_decision(
    int sign, PredicateCounters* counters) {
  const PredicateDecision decision{
      predicate_sign(sign), CertificationStage::cpu_multiprecision};
  if (counters != nullptr) {
    counters->record_certification(decision);
  }
  return decision;
}

[[nodiscard]] inline PredicateDecision filtered_decision(
    PredicateSign sign, PredicateCounters* counters) {
  const PredicateDecision decision{sign, CertificationStage::fp64_filtered};
  if (counters != nullptr) {
    counters->record_certification(decision);
  }
  return decision;
}

[[nodiscard]] inline PredicateDecision expansion_decision(
    PredicateSign sign, PredicateCounters* counters) {
  const PredicateDecision decision{sign, CertificationStage::expansion};
  if (counters != nullptr) {
    counters->record_certification(decision);
  }
  return decision;
}

inline void require_filter_policy(PredicateFilterPolicy policy) {
  if (policy != PredicateFilterPolicy::allow_fp64 &&
      policy != PredicateFilterPolicy::allow_adaptive &&
      policy != PredicateFilterPolicy::multiprecision_only) {
    throw std::invalid_argument("predicate filter policy is invalid");
  }
}

[[nodiscard]] inline bool policy_allows_fp64(
    PredicateFilterPolicy policy) noexcept {
  return policy == PredicateFilterPolicy::allow_fp64 ||
         policy == PredicateFilterPolicy::allow_adaptive;
}

[[nodiscard]] inline bool policy_allows_expansion(
    PredicateFilterPolicy policy) noexcept {
  return policy == PredicateFilterPolicy::allow_adaptive;
}

[[nodiscard]] inline PredicateDecision certify_materialized_sign(
    const FilterResult& filtered,
    const ExpansionResult& expanded,
    PredicateSign exact_sign,
    PredicateFilterPolicy policy,
    PredicateCounters* counters) {
  require_filter_policy(policy);
  if (policy_allows_fp64(policy) &&
      filtered.state() == FilterState::certified) {
    if (!filtered.sign().has_value() || *filtered.sign() != exact_sign) {
      throw std::runtime_error(
          "fp64 filter contradicted its exact diagnostic witness");
    }
    return filtered_decision(*filtered.sign(), counters);
  }
  if (policy_allows_expansion(policy) &&
      expanded.state() == FilterState::certified) {
    if (!expanded.sign().has_value() || *expanded.sign() != exact_sign) {
      throw std::runtime_error(
          "floating expansion contradicted its exact diagnostic witness");
    }
    return expansion_decision(*expanded.sign(), counters);
  }
  return multiprecision_decision(static_cast<int>(exact_sign), counters);
}

}  // namespace detail

[[nodiscard]] inline PredicateSign predicate_sign(int value) noexcept {
  if (value < 0) {
    return PredicateSign::negative;
  }
  return value == 0 ? PredicateSign::zero : PredicateSign::positive;
}

[[nodiscard]] inline std::string_view to_string(PredicateSign sign) {
  switch (sign) {
    case PredicateSign::negative:
      return "negative";
    case PredicateSign::zero:
      return "zero";
    case PredicateSign::positive:
      return "positive";
  }
  throw std::invalid_argument("predicate sign is invalid");
}

[[nodiscard]] inline std::string_view to_string(CertificationStage stage) {
  switch (stage) {
    case CertificationStage::fp64_filtered:
      return "fp64_filtered";
    case CertificationStage::expansion:
      return "expansion";
    case CertificationStage::cpu_multiprecision:
      return "cpu_multiprecision";
  }
  throw std::invalid_argument("predicate certification stage is invalid");
}

}  // namespace morsehgp3d::exact
