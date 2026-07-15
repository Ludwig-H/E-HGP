#pragma once

#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/predicate.hpp"
#include "morsehgp3d/exact/support.hpp"

#include <algorithm>
#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::exact {

struct ExactLevelOrderResult {
  PredicateDecision decision;
  BigInt cross_product_difference;
};

namespace level_order_detail {

[[nodiscard]] inline int bigint_sign(const BigInt& value) noexcept {
  if (value < 0) {
    return -1;
  }
  return value == 0 ? 0 : 1;
}

[[nodiscard]] inline PredicateDecision record_multiprecision_decision(
    const BigInt& witness, PredicateCounters* counters) {
  const PredicateDecision decision{
      predicate_sign(bigint_sign(witness)),
      CertificationStage::cpu_multiprecision};
  if (counters != nullptr) {
    counters->record_certification(decision);
  }
  return decision;
}

template <std::size_t SupportSize>
[[nodiscard]] inline ExactLevel exact_support_level(
    const std::array<CertifiedPoint3, SupportSize>& support) {
  const auto exact_support = support_detail::exact_support(support);
  const CircumcenterResult center = support_detail::circumcenter_of(exact_support);
  if (center.kind() != CircumcenterKind::unique ||
      !center.squared_level().has_value()) {
    throw std::invalid_argument(
        "support-level comparison requires affinely independent supports");
  }
  return *center.squared_level();
}

[[nodiscard]] inline ExactLevelOrderResult certify_support_level_order(
    const ExactLevel& left,
    const ExactLevel& right,
    const FilterResult& filtered,
    const ExpansionResult& expanded,
    PredicateFilterPolicy filter_policy,
    PredicateCounters* counters) {
  BigInt witness = exact_level_cross_product_difference(left, right);
  const PredicateSign exact_sign = predicate_sign(bigint_sign(witness));
  const PredicateDecision decision = detail::certify_materialized_sign(
      filtered, expanded, exact_sign, filter_policy, counters);
  return ExactLevelOrderResult{decision, std::move(witness)};
}

template <typename Function>
[[nodiscard]] decltype(auto) visit_binary64_support(
    std::span<const CertifiedPoint3> support, Function&& function) {
  switch (support.size()) {
    case 1U:
      return std::forward<Function>(function)(
          std::array<CertifiedPoint3, 1>{support[0]});
    case 2U:
      return std::forward<Function>(function)(
          std::array<CertifiedPoint3, 2>{support[0], support[1]});
    case 3U:
      return std::forward<Function>(function)(
          std::array<CertifiedPoint3, 3>{
              support[0], support[1], support[2]});
    case 4U:
      return std::forward<Function>(function)(
          std::array<CertifiedPoint3, 4>{
              support[0], support[1], support[2], support[3]});
    default:
      throw std::invalid_argument(
          "binary64 level provenance must contain one to four points");
  }
}

}  // namespace level_order_detail

// Returns the sign of left - right and records exactly one explicit
// CPU-multiprecision certification when counters are provided.
[[nodiscard]] inline PredicateDecision decide_exact_level_order(
    const ExactLevel& left,
    const ExactLevel& right,
    PredicateCounters* counters = nullptr) {
  const BigInt witness =
      exact_level_cross_product_difference(left, right);
  return level_order_detail::record_multiprecision_decision(
      witness, counters);
}

// Diagnostic variant retaining the homogeneous cross-product witness.
[[nodiscard]] inline ExactLevelOrderResult compare_exact_levels(
    const ExactLevel& left,
    const ExactLevel& right,
    PredicateCounters* counters = nullptr) {
  BigInt witness = exact_level_cross_product_difference(left, right);
  const PredicateDecision decision =
      level_order_detail::record_multiprecision_decision(witness, counters);
  return ExactLevelOrderResult{decision, std::move(witness)};
}

// Compares the squared radii of the two circumspheres defined by independent
// supports. This low-level overload does not prove that either support is a
// minimal enclosing-ball support. Use analyzed SupportLevelEmission values
// when miniball minimality/reduction is part of the caller's invariant.
template <std::size_t LeftSize, std::size_t RightSize>
[[nodiscard]] ExactLevelOrderResult compare_support_levels(
    const std::array<CertifiedPoint3, LeftSize>& left_support,
    const std::array<CertifiedPoint3, RightSize>& right_support,
    PredicateCounters* counters = nullptr,
    PredicateFilterPolicy filter_policy = PredicateFilterPolicy::allow_adaptive) {
  static_assert(LeftSize >= 1U && LeftSize <= 4U);
  static_assert(RightSize >= 1U && RightSize <= 4U);
  detail::require_filter_policy(filter_policy);
  const ExactLevel left =
      level_order_detail::exact_support_level(left_support);
  const ExactLevel right =
      level_order_detail::exact_support_level(right_support);
  const auto canonical_left =
      detail::support_polynomial::canonicalize_support(left_support);
  const auto canonical_right =
      detail::support_polynomial::canonicalize_support(right_support);
  FilterResult filtered = FilterResult::uncertain();
  if (detail::policy_allows_fp64(filter_policy)) {
    filtered = detail::support_polynomial::filter_level_order(
        canonical_left, canonical_right);
  }
  ExpansionResult expanded = ExpansionResult::uncertain();
  if (detail::policy_allows_expansion(filter_policy) &&
      filtered.state() != FilterState::certified) {
    expanded = detail::support_polynomial::expansion_level_order(
        canonical_left, canonical_right);
  }
  return level_order_detail::certify_support_level_order(
      left, right, filtered, expanded, filter_policy, counters);
}

// Point identifiers are serialized as exactly representable JSON integers in
// the v2 contract. They must already belong to one canonical point namespace;
// this class canonicalizes a support label, not the point table itself.
class CanonicalSupportIds {
 public:
  static constexpr std::uint64_t maximum_point_id =
      (std::uint64_t{1} << 53U) - std::uint64_t{1};

  [[nodiscard]] static CanonicalSupportIds from_ids(
      std::span<const std::uint64_t> ids) {
    if (ids.empty() || ids.size() > 4U) {
      throw std::invalid_argument(
          "a canonical support must contain between one and four identifiers");
    }
    std::array<std::uint64_t, 4> canonical{};
    for (std::size_t index = 0; index < ids.size(); ++index) {
      if (ids[index] > maximum_point_id) {
        throw std::out_of_range(
            "a canonical support identifier exceeds the v2 PointId domain");
      }
      canonical[index] = ids[index];
    }
    // The domain has at most four entries. An explicit insertion sort avoids
    // platform-dependent iterator difference conversions in strict builds.
    for (std::size_t index = 1U; index < ids.size(); ++index) {
      const std::uint64_t value = canonical[index];
      std::size_t position = index;
      while (position > 0U && value < canonical[position - 1U]) {
        canonical[position] = canonical[position - 1U];
        --position;
      }
      canonical[position] = value;
    }
    for (std::size_t index = 1U; index < ids.size(); ++index) {
      if (canonical[index - 1U] == canonical[index]) {
        throw std::invalid_argument(
            "canonical support identifiers must be unique");
      }
    }
    return CanonicalSupportIds{canonical, ids.size()};
  }

  [[nodiscard]] std::size_t size() const noexcept { return size_; }

  [[nodiscard]] const std::uint64_t& id(std::size_t index) const {
    if (index >= size_) {
      throw std::out_of_range(
          "canonical support identifier index is outside the support");
    }
    return ids_[index];
  }

  [[nodiscard]] std::span<const std::uint64_t> ids() const noexcept {
    return std::span<const std::uint64_t>{ids_.data(), size_};
  }

  [[nodiscard]] bool contains(std::uint64_t identifier) const noexcept {
    for (std::size_t index = 0; index < size_; ++index) {
      if (ids_[index] == identifier) {
        return true;
      }
    }
    return false;
  }

  friend bool operator==(
      const CanonicalSupportIds& left,
      const CanonicalSupportIds& right) noexcept {
    if (left.size_ != right.size_) {
      return false;
    }
    for (std::size_t index = 0; index < left.size_; ++index) {
      if (left.ids_[index] != right.ids_[index]) {
        return false;
      }
    }
    return true;
  }

  // The canonical local-support tie-break is support size, then numeric IDs.
  friend std::strong_ordering operator<=>(
      const CanonicalSupportIds& left,
      const CanonicalSupportIds& right) noexcept {
    if (left.size_ < right.size_) {
      return std::strong_ordering::less;
    }
    if (left.size_ > right.size_) {
      return std::strong_ordering::greater;
    }
    for (std::size_t index = 0; index < left.size_; ++index) {
      if (left.ids_[index] < right.ids_[index]) {
        return std::strong_ordering::less;
      }
      if (left.ids_[index] > right.ids_[index]) {
        return std::strong_ordering::greater;
      }
    }
    return std::strong_ordering::equal;
  }

 private:
  CanonicalSupportIds(
      std::array<std::uint64_t, 4> ids, std::size_t size) noexcept
      : ids_(ids), size_(size) {}

  std::array<std::uint64_t, 4> ids_{};
  std::size_t size_;
};

// Structural input for ordering. create() validates only the closed level and
// support/provenance relation; it does not certify any geometric construction,
// ambient enclosure or enumeration completeness. Geometry-producing callers
// should prefer support_level_emission_from_analysis() below.
class SupportLevelEmission {
 public:
  [[nodiscard]] static SupportLevelEmission create(
      ExactLevel squared_level,
      CanonicalSupportIds minimal_support_ids,
      CanonicalSupportIds source_support_ids) {
    for (const std::uint64_t identifier : minimal_support_ids.ids()) {
      if (!source_support_ids.contains(identifier)) {
        throw std::invalid_argument(
            "a minimal support must be contained in its source support");
      }
    }
    return SupportLevelEmission{
        std::move(squared_level),
        std::move(minimal_support_ids),
        std::move(source_support_ids),
        {}};
  }

  [[nodiscard]] const ExactLevel& squared_level() const noexcept {
    return squared_level_;
  }

  [[nodiscard]] const CanonicalSupportIds& minimal_support_ids()
      const noexcept {
    return minimal_support_ids_;
  }

  [[nodiscard]] const CanonicalSupportIds& source_support_ids()
      const noexcept {
    return source_support_ids_;
  }

  // Minimal binary64 support from a certified geometric analysis. Structural
  // emissions built from arbitrary ExactLevel values intentionally leave it
  // empty and therefore cannot use adaptive level predicates.
  [[nodiscard]] std::span<const CertifiedPoint3> binary64_level_support()
      const noexcept {
    return binary64_level_support_;
  }

  friend bool operator==(
      const SupportLevelEmission& left,
      const SupportLevelEmission& right) noexcept {
    return left.squared_level_ == right.squared_level_ &&
           left.minimal_support_ids_ == right.minimal_support_ids_ &&
           left.source_support_ids_ == right.source_support_ids_;
  }

 private:
  friend SupportLevelEmission support_level_emission_from_analysis(
      std::span<const std::uint64_t> positional_source_ids,
      const CircumcenterSupportAnalysis& analysis);

  SupportLevelEmission(
      ExactLevel squared_level,
      CanonicalSupportIds minimal_support_ids,
      CanonicalSupportIds source_support_ids,
      std::vector<CertifiedPoint3> binary64_level_support)
      : squared_level_(std::move(squared_level)),
        minimal_support_ids_(std::move(minimal_support_ids)),
        source_support_ids_(std::move(source_support_ids)),
        binary64_level_support_(std::move(binary64_level_support)) {}

  ExactLevel squared_level_;
  CanonicalSupportIds minimal_support_ids_;
  CanonicalSupportIds source_support_ids_;
  std::vector<CertifiedPoint3> binary64_level_support_;
};

[[nodiscard]] inline FilterResult filter_support_level_order(
    const SupportLevelEmission& left,
    const SupportLevelEmission& right) {
  if (left.binary64_level_support().empty() ||
      right.binary64_level_support().empty()) {
    return FilterResult::uncertain();
  }
  return level_order_detail::visit_binary64_support(
      left.binary64_level_support(),
      [&right](const auto& left_support) {
        const auto canonical_left =
            detail::support_polynomial::canonicalize_support(left_support);
        return level_order_detail::visit_binary64_support(
            right.binary64_level_support(),
            [&canonical_left](const auto& right_support) {
              const auto canonical_right =
                  detail::support_polynomial::canonicalize_support(
                      right_support);
              return detail::support_polynomial::filter_level_order(
                  canonical_left, canonical_right);
            });
      });
}

[[nodiscard]] inline ExpansionResult expansion_support_level_order(
    const SupportLevelEmission& left,
    const SupportLevelEmission& right) {
  if (left.binary64_level_support().empty() ||
      right.binary64_level_support().empty()) {
    return ExpansionResult::uncertain();
  }
  return level_order_detail::visit_binary64_support(
      left.binary64_level_support(),
      [&right](const auto& left_support) {
        const auto canonical_left =
            detail::support_polynomial::canonicalize_support(left_support);
        return level_order_detail::visit_binary64_support(
            right.binary64_level_support(),
            [&canonical_left](const auto& right_support) {
              const auto canonical_right =
                  detail::support_polynomial::canonicalize_support(
                      right_support);
              return detail::support_polynomial::expansion_level_order(
                  canonical_left, canonical_right);
            });
      });
}

[[nodiscard]] inline ExactLevelOrderResult compare_support_levels(
    const SupportLevelEmission& left,
    const SupportLevelEmission& right,
    PredicateCounters* counters = nullptr,
    PredicateFilterPolicy filter_policy = PredicateFilterPolicy::allow_adaptive) {
  detail::require_filter_policy(filter_policy);
  FilterResult filtered = FilterResult::uncertain();
  if (detail::policy_allows_fp64(filter_policy)) {
    filtered = filter_support_level_order(left, right);
  }
  ExpansionResult expanded = ExpansionResult::uncertain();
  if (detail::policy_allows_expansion(filter_policy) &&
      filtered.state() != FilterState::certified) {
    expanded = expansion_support_level_order(left, right);
  }
  return level_order_detail::certify_support_level_order(
      left.squared_level(),
      right.squared_level(),
      filtered,
      expanded,
      filter_policy,
      counters);
}

[[nodiscard]] inline PredicateDecision decide_support_level_order(
    const SupportLevelEmission& left,
    const SupportLevelEmission& right,
    PredicateCounters* counters = nullptr,
    PredicateFilterPolicy filter_policy = PredicateFilterPolicy::allow_adaptive) {
  return compare_support_levels(left, right, counters, filter_policy).decision;
}

// Converts the certified local minimality result from 2A.6 into a leveled
// support emission. Exterior and dependent circumcentres require the later
// exhaustive sub-support enumeration and are deliberately rejected here. The
// exact centre remains a checked witness of the analysis; it is not a further
// tie-break after a canonical minimal support has been established.
[[nodiscard]] inline SupportLevelEmission support_level_emission_from_analysis(
    std::span<const std::uint64_t> positional_source_ids,
    const CircumcenterSupportAnalysis& analysis) {
  const CircumcenterSupportStatus status = analysis.status();
  if (status != CircumcenterSupportStatus::minimal &&
      status != CircumcenterSupportStatus::boundary_reduced) {
    throw std::invalid_argument(
        "only a minimal or boundary-reduced analysis defines a local support sphere item");
  }
  const CircumcenterResult& circumcenter = analysis.circumcenter_result();
  if (positional_source_ids.size() != circumcenter.support_size()) {
    throw std::invalid_argument(
        "support identifiers must match the analyzed support positions");
  }
  if (circumcenter.kind() != CircumcenterKind::unique ||
      !circumcenter.center().has_value() ||
      !circumcenter.squared_level().has_value() ||
      !analysis.reduced_support_mask().has_value()) {
    throw std::logic_error(
        "a local support sphere item omitted an exact witness or reduced support");
  }

  // The caller must pair each identifier with the point at the same position
  // in the support that produced analysis. Validate and canonicalize the full
  // provenance without changing that positional input before applying the
  // reduction mask below.
  CanonicalSupportIds source_support =
      CanonicalSupportIds::from_ids(positional_source_ids);
  std::array<std::uint64_t, 4> reduced_ids{};
  std::size_t reduced_size = 0U;
  for (std::size_t index = 0; index < positional_source_ids.size(); ++index) {
    if (analysis.reduced_support_contains(index)) {
      reduced_ids[reduced_size] = positional_source_ids[index];
      ++reduced_size;
    }
  }
  if (reduced_size == 0U ||
      !analysis.reduced_support_size().has_value() ||
      reduced_size != *analysis.reduced_support_size()) {
    throw std::logic_error(
        "a local support sphere item has an inconsistent reduced support mask");
  }
  const CanonicalSupportIds minimal_support =
      CanonicalSupportIds::from_ids(
          std::span<const std::uint64_t>{reduced_ids.data(), reduced_size});
  SupportLevelEmission emission = SupportLevelEmission::create(
      *circumcenter.squared_level(),
      minimal_support,
      std::move(source_support));
  const std::span<const CertifiedPoint3> binary64_support =
      analysis.binary64_support();
  if (!binary64_support.empty()) {
    if (binary64_support.size() != positional_source_ids.size()) {
      throw std::logic_error(
          "binary64 support provenance does not match its positional identifiers");
    }
    emission.binary64_level_support_.reserve(reduced_size);
    for (std::size_t index = 0; index < binary64_support.size(); ++index) {
      if (analysis.reduced_support_contains(index)) {
        emission.binary64_level_support_.push_back(binary64_support[index]);
      }
    }
    if (emission.binary64_level_support_.size() != reduced_size) {
      throw std::logic_error(
          "binary64 level provenance does not match the reduced support");
    }
  }
  return emission;
}

struct SupportEmissionProvenance {
  CanonicalSupportIds source_support_ids;
  std::size_t emission_count;

  friend bool operator==(
      const SupportEmissionProvenance&,
      const SupportEmissionProvenance&) noexcept = default;
};

struct CanonicalSupportLevel {
  ExactLevel squared_level;
  CanonicalSupportIds minimal_support_ids;
  std::vector<SupportEmissionProvenance> source_provenance;
  std::size_t emission_count;

  friend bool operator==(
      const CanonicalSupportLevel&,
      const CanonicalSupportLevel&) noexcept = default;
};

struct EqualLevelSupportBatch {
  ExactLevel squared_level;
  std::vector<CanonicalSupportLevel> supports;
  std::size_t emission_count;

  friend bool operator==(
      const EqualLevelSupportBatch&,
      const EqualLevelSupportBatch&) noexcept = default;
};

struct CanonicalLevelBatchResult {
  std::vector<EqualLevelSupportBatch> batches;
  std::size_t emission_count;
  std::size_t unique_emission_count;
  std::size_t duplicate_emission_count;

  friend bool operator==(
      const CanonicalLevelBatchResult&,
      const CanonicalLevelBatchResult&) noexcept = default;
};

namespace level_order_detail {

[[nodiscard]] inline bool support_then_level_then_source_less(
    const SupportLevelEmission& left,
    const SupportLevelEmission& right) {
  if (left.minimal_support_ids() != right.minimal_support_ids()) {
    return left.minimal_support_ids() < right.minimal_support_ids();
  }
  if (left.squared_level() != right.squared_level()) {
    return left.squared_level() < right.squared_level();
  }
  return left.source_support_ids() < right.source_support_ids();
}

[[nodiscard]] inline bool level_then_support_then_source_less(
    const SupportLevelEmission& left,
    const SupportLevelEmission& right) {
  if (left.squared_level() != right.squared_level()) {
    return left.squared_level() < right.squared_level();
  }
  if (left.minimal_support_ids() != right.minimal_support_ids()) {
    return left.minimal_support_ids() < right.minimal_support_ids();
  }
  return left.source_support_ids() < right.source_support_ids();
}

}  // namespace level_order_detail

// Canonicalizes already-verified structural emissions from one point namespace.
// Equal levels form one batch; distinct minimal supports remain distinct.
// Repeated source emissions are counted, while different source supports are
// retained as sorted provenance for the same reduced support. This operation
// never promotes its inputs to geometrically certified or publicly exact data.
[[nodiscard]] inline CanonicalLevelBatchResult canonical_level_batches(
    std::span<const SupportLevelEmission> emissions) {
  std::vector<SupportLevelEmission> ordered{emissions.begin(), emissions.end()};

  // A canonical minimal support denotes one deterministic sphere in a point
  // namespace. Different levels for the same support are therefore a hard
  // invariant violation, not values to order by a further tie-break.
  std::sort(
      ordered.begin(),
      ordered.end(),
      level_order_detail::support_then_level_then_source_less);
  for (std::size_t index = 1U; index < ordered.size(); ++index) {
    if (ordered[index - 1U].minimal_support_ids() ==
            ordered[index].minimal_support_ids() &&
        ordered[index - 1U].squared_level() !=
            ordered[index].squared_level()) {
      throw std::invalid_argument(
          "one canonical minimal support cannot have different exact levels");
    }
  }

  std::sort(
      ordered.begin(),
      ordered.end(),
      level_order_detail::level_then_support_then_source_less);

  std::vector<EqualLevelSupportBatch> batches;
  std::size_t unique_emission_count = 0U;
  std::size_t duplicate_emission_count = 0U;
  for (const SupportLevelEmission& emission : ordered) {
    if (batches.empty() ||
        batches.back().squared_level != emission.squared_level()) {
      batches.push_back(EqualLevelSupportBatch{
          emission.squared_level(), {}, 0U});
    }
    EqualLevelSupportBatch& batch = batches.back();
    if (batch.supports.empty() ||
        batch.supports.back().minimal_support_ids !=
            emission.minimal_support_ids()) {
      batch.supports.push_back(CanonicalSupportLevel{
          emission.squared_level(), emission.minimal_support_ids(), {}, 0U});
    }
    CanonicalSupportLevel& support = batch.supports.back();
    if (support.source_provenance.empty() ||
        support.source_provenance.back().source_support_ids !=
            emission.source_support_ids()) {
      support.source_provenance.push_back(
          SupportEmissionProvenance{emission.source_support_ids(), 1U});
      ++unique_emission_count;
    } else {
      ++support.source_provenance.back().emission_count;
      ++duplicate_emission_count;
    }
    ++support.emission_count;
    ++batch.emission_count;
  }

  return CanonicalLevelBatchResult{
      std::move(batches),
      ordered.size(),
      unique_emission_count,
      duplicate_emission_count};
}

}  // namespace morsehgp3d::exact
