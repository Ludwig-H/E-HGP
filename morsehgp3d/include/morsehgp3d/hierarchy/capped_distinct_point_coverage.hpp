#pragma once

#include "morsehgp3d/spatial/point_cloud.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

// This is a downstream cardinality summary for an already complete source
// component.  It is not a source filter: discarded identifiers are irrelevant
// only to the predicate |coverage| >= threshold and must never be used to drop
// facets, cofaces, silent incidences, or equal-level source work.
//
// For a threshold m >= 1, the summary stores exactly the numerically
// smallest min(m, |coverage|) distinct PointId values.  Thus reaching m stored
// identifiers decides |coverage| >= m exactly.  Once the threshold is reached,
// the summary deliberately exposes no exact cardinality beyond that lower
// bound.
class CappedDistinctPointCoverage {
 public:
  static constexpr std::string_view backend = "reference_cpu";
  static constexpr std::string_view profile = "hgp_reduced";
  static constexpr std::string_view mode =
      "downstream_capped_distinct_point_coverage";
  static constexpr std::string_view relation = "at_least";
  static constexpr std::string_view deployment_status =
      "architecture_only";
  static constexpr std::string_view public_status = "not_claimed";
  static constexpr bool source_pruning_authority = false;
  static constexpr bool hierarchy_authority = false;
  static constexpr bool public_exactness_claimed = false;

  explicit CappedDistinctPointCoverage(std::size_t threshold);
  CappedDistinctPointCoverage(const CappedDistinctPointCoverage&) = default;
  CappedDistinctPointCoverage& operator=(
      const CappedDistinctPointCoverage&) = default;
  CappedDistinctPointCoverage(CappedDistinctPointCoverage&& other) noexcept;
  CappedDistinctPointCoverage& operator=(
      CappedDistinctPointCoverage&& other) noexcept;

  [[nodiscard]] std::size_t threshold() const noexcept {
    return threshold_;
  }

  [[nodiscard]] std::size_t retained_distinct_count() const noexcept {
    return smallest_distinct_point_ids_.size();
  }

  [[nodiscard]] std::span<const spatial::PointId>
  smallest_distinct_point_ids() const & noexcept {
    return smallest_distinct_point_ids_;
  }
  [[nodiscard]] std::span<const spatial::PointId>
  smallest_distinct_point_ids() const && = delete;

  [[nodiscard]] bool coverage_size_at_least_threshold() const noexcept {
    return smallest_distinct_point_ids_.size() == threshold_;
  }

  // A value is returned only while the complete supplied coverage is known to
  // contain fewer than threshold distinct identifiers.  At the threshold this
  // returns nullopt even when the true size happens to equal threshold, because
  // the capped summary cannot distinguish equality from a larger cardinality.
  [[nodiscard]] std::optional<std::size_t>
  exact_coverage_size_if_below_threshold() const noexcept;

  // Returns whether the canonical retained summary changed.
  bool insert_point_id(spatial::PointId point_id);

  void insert_facet(std::span<const spatial::PointId> facet_point_ids);

  void insert_facets(
      std::span<const std::vector<spatial::PointId>> facet_point_ids);

  // This complete delta is staged transactionally: allocation failure leaves
  // the existing summary unchanged.  Both arenas are accepted because an
  // exact coverage_delta may retain the
  // newly acquired facets for source replay in addition to its point-set
  // difference.  Their union is summarized; overlaps and duplicates are
  // intentionally harmless.
  void insert_coverage_delta(
      std::span<const std::vector<spatial::PointId>>
          added_facet_point_ids,
      std::span<const spatial::PointId> added_point_ids);

  // Summaries must use the same runtime threshold.  The operation implements
  // cap_m(A union B) from cap_m(A) and cap_m(B), so repeated and regrouped
  // merges are idempotent, commutative, and associative at summary level.
  void merge_from(const CappedDistinctPointCoverage& other);

  friend bool operator==(
      const CappedDistinctPointCoverage&,
      const CappedDistinctPointCoverage&) = default;

 private:
  std::size_t threshold_;
  std::vector<spatial::PointId> smallest_distinct_point_ids_;
};

}  // namespace morsehgp3d::hierarchy
