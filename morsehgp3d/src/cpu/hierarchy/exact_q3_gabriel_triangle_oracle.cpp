#include "morsehgp3d/hierarchy/exact_q3_gabriel_triangle_oracle.hpp"

#include "morsehgp3d/exact/support.hpp"
#include "morsehgp3d/hierarchy/facet_miniball.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_multiply(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (left != 0U && right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::length_error(message);
  }
  return left * right;
}

[[nodiscard]] std::size_t choose_three(std::size_t point_count) {
  if (point_count < 3U) {
    return 0U;
  }
  return checked_multiply(
             checked_multiply(
                 point_count,
                 point_count - 1U,
                 "the q=3 triangle count overflows size_t"),
             point_count - 2U,
             "the q=3 triangle count overflows size_t") /
         6U;
}

[[nodiscard]] std::uint64_t checked_u64(
    std::size_t value,
    const char* message) {
  if constexpr (sizeof(std::size_t) > sizeof(std::uint64_t)) {
    if (value > static_cast<std::size_t>(
                    std::numeric_limits<std::uint64_t>::max())) {
      throw std::length_error(message);
    }
  }
  return static_cast<std::uint64_t>(value);
}

class DigestWriter {
 public:
  explicit DigestWriter(std::string_view domain) { text(domain); }

  void byte(std::uint8_t value) {
    builder_.update(std::span<const std::uint8_t>{&value, 1U});
  }

  void boolean(bool value) { byte(value ? std::uint8_t{1U} : 0U); }

  void u64(std::uint64_t value) {
    std::array<std::uint8_t, 8U> bytes{};
    for (std::size_t index = 0U; index < bytes.size(); ++index) {
      const std::size_t shift = (bytes.size() - 1U - index) * 8U;
      bytes[index] = static_cast<std::uint8_t>(value >> shift);
    }
    builder_.update(bytes);
  }

  void size(std::size_t value) {
    u64(checked_u64(
        value, "a q=3 Gabriel oracle size does not fit uint64"));
  }

  void point_id(PointId value) { u64(value); }

  void text(std::string_view value) {
    size(value.size());
    builder_.update(value);
  }

  void canonical_id(const contract::CanonicalId& value) {
    builder_.update(value.bytes());
  }

  void center(const exact::ExactCenter3& value) {
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      text(exact::canonical_integer_string(value.numerator(axis)));
    }
    text(exact::canonical_integer_string(value.denominator()));
  }

  void level(const exact::ExactLevel& value) { text(value.canonical_key()); }

  void point_ids(std::span<const PointId> values) {
    size(values.size());
    for (const PointId value : values) {
      point_id(value);
    }
  }

  [[nodiscard]] contract::CanonicalId finalize() {
    return builder_.finalize();
  }

 private:
  contract::CanonicalSha256Builder builder_;
};

[[nodiscard]] contract::CanonicalId canonical_cloud_digest(
    const spatial::CanonicalPointCloud& cloud) {
  DigestWriter writer{
      "MorseHGP3D/exact_q3_gabriel_triangle_oracle/cloud/v1"};
  writer.size(cloud.size());
  for (std::size_t index = 0U; index < cloud.size(); ++index) {
    const PointId point_id = static_cast<PointId>(index);
    writer.point_id(point_id);
    for (const std::uint64_t bits : cloud.point(point_id).canonical_input_bits()) {
      writer.u64(bits);
    }
  }
  return writer.finalize();
}

void write_counters(
    DigestWriter& writer,
    const ExactQ3GabrielTriangleOracleCounters& counters) {
  writer.size(counters.point_count);
  writer.size(counters.expected_unordered_triangle_count);
  writer.size(counters.enumerated_unordered_triangle_count);
  writer.size(counters.local_exact_miniball_build_count);
  writer.size(counters.full_cloud_point_classification_count);
  writer.size(counters.strictly_inside_classification_count);
  writer.size(counters.boundary_classification_count);
  writer.size(counters.exterior_classification_count);
  writer.size(counters.minimal_pair_support_triangle_count);
  writer.size(counters.minimal_triangle_support_triangle_count);
  writer.size(counters.rejected_non_gabriel_triangle_count);
  writer.size(counters.pre_dedup_gabriel_triangle_count);
  writer.size(counters.duplicate_gabriel_triangle_occurrence_count);
  writer.size(counters.output_gabriel_triangle_count);
  writer.size(counters.output_support_point_reference_count);
  writer.size(counters.output_strict_interior_point_reference_count);
  writer.size(counters.output_extra_shell_point_reference_count);
  writer.size(counters.output_exterior_point_reference_count);
  writer.size(counters.output_facet_reference_count);
  writer.boolean(counters.complete_triangle_mass_accounting_closed);
  writer.boolean(counters.complete_point_classification_accounting_closed);
  writer.boolean(counters.canonical_sort_and_dedup_certified);
  writer.boolean(counters.bounded_exhaustive_reference_only);
  writer.boolean(counters.future_product_producer_called);
  writer.boolean(counters.delaunay_or_higher_order_mosaic_materialized);
  writer.boolean(counters.gamma2_completeness_claimed);
  writer.boolean(counters.silent_incidence_coverage_certified);
  writer.boolean(counters.hierarchy_reduction_performed);
  writer.boolean(counters.public_status_claimed);
}

void write_record(
    DigestWriter& writer,
    const ExactQ3GabrielTriangleRecord& record) {
  writer.point_ids(record.triangle_point_ids);
  for (const ExactQ3GabrielFacetIds& facet : record.facet_point_ids) {
    writer.point_ids(facet);
  }
  writer.byte(static_cast<std::uint8_t>(record.minimal_support_kind));
  writer.point_ids(record.support_point_ids);
  writer.point_ids(record.strict_interior_point_ids);
  writer.point_ids(record.extra_shell_point_ids);
  writer.point_ids(record.exterior_point_ids);
  writer.center(record.center);
  writer.level(record.squared_level);
}

[[nodiscard]] contract::CanonicalId compute_payload_digest(
    const ExactQ3GabrielTriangleOracleResult& result) {
  DigestWriter writer{
      "MorseHGP3D/exact_q3_gabriel_triangle_oracle/payload/v1"};
  writer.u64(result.schema_version);
  writer.size(result.point_count);
  writer.canonical_id(result.canonical_cloud_digest);
  writer.size(result.triangles.size());
  for (const ExactQ3GabrielTriangleRecord& record : result.triangles) {
    write_record(writer, record);
  }
  write_counters(writer, result.counters);
  return writer.finalize();
}

[[nodiscard]] ExactQ3GabrielTriangleFacets canonical_facets(
    const ExactQ3GabrielTriangleIds& triangle) noexcept {
  return ExactQ3GabrielTriangleFacets{
      ExactQ3GabrielFacetIds{triangle[0], triangle[1]},
      ExactQ3GabrielFacetIds{triangle[0], triangle[2]},
      ExactQ3GabrielFacetIds{triangle[1], triangle[2]}};
}

[[nodiscard]] bool record_less(
    const ExactQ3GabrielTriangleRecord& left,
    const ExactQ3GabrielTriangleRecord& right) {
  if (left.squared_level != right.squared_level) {
    return left.squared_level < right.squared_level;
  }
  return left.triangle_point_ids < right.triangle_point_ids;
}

[[nodiscard]] bool triangle_key_less(
    const ExactQ3GabrielTriangleRecord& left,
    const ExactQ3GabrielTriangleRecord& right) noexcept {
  return left.triangle_point_ids < right.triangle_point_ids;
}

[[nodiscard]] bool valid_point_id(
    PointId point_id,
    std::size_t point_count) noexcept {
  return point_id < static_cast<PointId>(point_count);
}

[[nodiscard]] bool sorted_unique_valid_ids(
    std::span<const PointId> point_ids,
    std::size_t point_count) noexcept {
  for (std::size_t index = 0U; index < point_ids.size(); ++index) {
    if (!valid_point_id(point_ids[index], point_count) ||
        (index != 0U && point_ids[index - 1U] >= point_ids[index])) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool record_structurally_valid(
    const ExactQ3GabrielTriangleRecord& record,
    std::size_t point_count) {
  if (!sorted_unique_valid_ids(record.triangle_point_ids, point_count) ||
      record.facet_point_ids != canonical_facets(record.triangle_point_ids) ||
      !sorted_unique_valid_ids(record.support_point_ids, point_count) ||
      !sorted_unique_valid_ids(
          record.strict_interior_point_ids, point_count) ||
      !sorted_unique_valid_ids(record.extra_shell_point_ids, point_count) ||
      !sorted_unique_valid_ids(record.exterior_point_ids, point_count) ||
      record.squared_level.numerator() <= 0 ||
      (record.support_point_ids.size() != 2U &&
       record.support_point_ids.size() != 3U) ||
      static_cast<std::size_t>(record.minimal_support_kind) !=
          record.support_point_ids.size() ||
      !std::includes(
          record.triangle_point_ids.begin(),
          record.triangle_point_ids.end(),
          record.support_point_ids.begin(),
          record.support_point_ids.end()) ||
      !std::includes(
          record.triangle_point_ids.begin(),
          record.triangle_point_ids.end(),
          record.strict_interior_point_ids.begin(),
          record.strict_interior_point_ids.end())) {
    return false;
  }

  std::vector<PointId> partition;
  partition.reserve(point_count);
  partition.insert(
      partition.end(),
      record.support_point_ids.begin(),
      record.support_point_ids.end());
  partition.insert(
      partition.end(),
      record.strict_interior_point_ids.begin(),
      record.strict_interior_point_ids.end());
  partition.insert(
      partition.end(),
      record.extra_shell_point_ids.begin(),
      record.extra_shell_point_ids.end());
  partition.insert(
      partition.end(),
      record.exterior_point_ids.begin(),
      record.exterior_point_ids.end());
  std::sort(partition.begin(), partition.end());
  if (partition.size() != point_count) {
    return false;
  }
  for (std::size_t index = 0U; index < partition.size(); ++index) {
    if (partition[index] != static_cast<PointId>(index)) {
      return false;
    }
  }
  for (const PointId triangle_point_id : record.triangle_point_ids) {
    if (std::binary_search(
            record.exterior_point_ids.begin(),
            record.exterior_point_ids.end(),
            triangle_point_id)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool triangle_keys_unique(
    const std::vector<ExactQ3GabrielTriangleRecord>& records) {
  std::vector<ExactQ3GabrielTriangleIds> keys;
  keys.reserve(records.size());
  for (const ExactQ3GabrielTriangleRecord& record : records) {
    keys.push_back(record.triangle_point_ids);
  }
  std::sort(keys.begin(), keys.end());
  return std::adjacent_find(keys.begin(), keys.end()) == keys.end();
}

void add_record_reference_counts(
    ExactQ3GabrielTriangleOracleCounters& counters,
    const ExactQ3GabrielTriangleRecord& record) {
  counters.output_support_point_reference_count = checked_add(
      counters.output_support_point_reference_count,
      record.support_point_ids.size(),
      "the q=3 output support reference count overflows size_t");
  counters.output_strict_interior_point_reference_count = checked_add(
      counters.output_strict_interior_point_reference_count,
      record.strict_interior_point_ids.size(),
      "the q=3 output strict-interior reference count overflows size_t");
  counters.output_extra_shell_point_reference_count = checked_add(
      counters.output_extra_shell_point_reference_count,
      record.extra_shell_point_ids.size(),
      "the q=3 output extra-shell reference count overflows size_t");
  counters.output_exterior_point_reference_count = checked_add(
      counters.output_exterior_point_reference_count,
      record.exterior_point_ids.size(),
      "the q=3 output exterior reference count overflows size_t");
  counters.output_facet_reference_count = checked_add(
      counters.output_facet_reference_count,
      record.facet_point_ids.size(),
      "the q=3 output facet reference count overflows size_t");
}

}  // namespace

bool ExactQ3GabrielTriangleOracleResult::locally_structurally_valid() const {
  try {
    if (schema_version != exact_q3_gabriel_triangle_oracle_schema_version ||
        point_count == 0U ||
        point_count > exact_q3_gabriel_triangle_oracle_maximum_point_count ||
        counters.point_count != point_count) {
      return false;
    }
    const std::size_t expected_triangle_count = choose_three(point_count);
    const std::size_t classification_mass = checked_add(
        checked_add(
            counters.strictly_inside_classification_count,
            counters.boundary_classification_count,
            "the q=3 structural classification mass overflows size_t"),
        counters.exterior_classification_count,
        "the q=3 structural classification mass overflows size_t");
    const std::size_t support_kind_mass = checked_add(
        counters.minimal_pair_support_triangle_count,
        counters.minimal_triangle_support_triangle_count,
        "the q=3 structural support-kind mass overflows size_t");
    const std::size_t decision_mass = checked_add(
        counters.rejected_non_gabriel_triangle_count,
        counters.pre_dedup_gabriel_triangle_count,
        "the q=3 structural decision mass overflows size_t");
    const std::size_t dedup_mass = checked_add(
        counters.duplicate_gabriel_triangle_occurrence_count,
        counters.output_gabriel_triangle_count,
        "the q=3 structural dedup mass overflows size_t");
    const std::size_t expected_facet_reference_count = checked_multiply(
        triangles.size(),
        3U,
        "the q=3 structural facet reference count overflows size_t");
    if (triangles.size() > expected_triangle_count ||
        counters.expected_unordered_triangle_count !=
            expected_triangle_count ||
        counters.enumerated_unordered_triangle_count !=
            counters.expected_unordered_triangle_count ||
        counters.local_exact_miniball_build_count !=
            counters.enumerated_unordered_triangle_count ||
        counters.full_cloud_point_classification_count !=
            checked_multiply(
                counters.enumerated_unordered_triangle_count,
                point_count,
                "the q=3 structural classification count overflows size_t") ||
        classification_mass !=
            counters.full_cloud_point_classification_count ||
        support_kind_mass !=
            counters.enumerated_unordered_triangle_count ||
        decision_mass !=
            counters.enumerated_unordered_triangle_count ||
        dedup_mass !=
            counters.pre_dedup_gabriel_triangle_count ||
        counters.output_gabriel_triangle_count != triangles.size() ||
        counters.output_facet_reference_count !=
            expected_facet_reference_count ||
        !counters.complete_triangle_mass_accounting_closed ||
        !counters.complete_point_classification_accounting_closed ||
        !counters.canonical_sort_and_dedup_certified ||
        !counters.bounded_exhaustive_reference_only ||
        counters.future_product_producer_called ||
        counters.delaunay_or_higher_order_mosaic_materialized ||
        counters.gamma2_completeness_claimed ||
        counters.silent_incidence_coverage_certified ||
        counters.hierarchy_reduction_performed ||
        counters.public_status_claimed ||
        !std::is_sorted(triangles.begin(), triangles.end(), record_less) ||
        !triangle_keys_unique(triangles)) {
      return false;
    }

    std::size_t support_reference_count = 0U;
    std::size_t strict_interior_reference_count = 0U;
    std::size_t extra_shell_reference_count = 0U;
    std::size_t exterior_reference_count = 0U;
    for (const ExactQ3GabrielTriangleRecord& record : triangles) {
      if (!record_structurally_valid(record, point_count)) {
        return false;
      }
      support_reference_count = checked_add(
          support_reference_count,
          record.support_point_ids.size(),
          "the q=3 structural support reference count overflows size_t");
      strict_interior_reference_count = checked_add(
          strict_interior_reference_count,
          record.strict_interior_point_ids.size(),
          "the q=3 structural interior reference count overflows size_t");
      extra_shell_reference_count = checked_add(
          extra_shell_reference_count,
          record.extra_shell_point_ids.size(),
          "the q=3 structural shell reference count overflows size_t");
      exterior_reference_count = checked_add(
          exterior_reference_count,
          record.exterior_point_ids.size(),
          "the q=3 structural exterior reference count overflows size_t");
    }
    return support_reference_count ==
               counters.output_support_point_reference_count &&
           strict_interior_reference_count ==
               counters.output_strict_interior_point_reference_count &&
           extra_shell_reference_count ==
               counters.output_extra_shell_point_reference_count &&
           exterior_reference_count ==
               counters.output_exterior_point_reference_count &&
           compute_payload_digest(*this) == payload_digest;
  } catch (const std::exception&) {
    return false;
  }
}

ExactQ3GabrielTriangleOracleResult
build_exact_q3_gabriel_triangle_oracle(
    const spatial::CanonicalPointCloud& cloud) {
  if (cloud.size() == 0U) {
    throw std::invalid_argument(
        "the exhaustive q=3 Gabriel oracle requires a nonempty canonical "
        "point cloud");
  }
  if (cloud.size() > exact_q3_gabriel_triangle_oracle_maximum_point_count) {
    throw std::invalid_argument(
        "the exhaustive q=3 Gabriel oracle is limited to fourteen points");
  }

  ExactQ3GabrielTriangleOracleResult result;
  result.point_count = cloud.size();
  result.canonical_cloud_digest = canonical_cloud_digest(cloud);
  result.counters.point_count = cloud.size();
  result.counters.expected_unordered_triangle_count =
      choose_three(cloud.size());
  std::vector<ExactQ3GabrielTriangleRecord> occurrences;
  occurrences.reserve(result.counters.expected_unordered_triangle_count);

  for (std::size_t first = 0U; first < cloud.size(); ++first) {
    for (std::size_t second = first + 1U; second < cloud.size(); ++second) {
      for (std::size_t third = second + 1U; third < cloud.size(); ++third) {
        const ExactQ3GabrielTriangleIds triangle{
            static_cast<PointId>(first),
            static_cast<PointId>(second),
            static_cast<PointId>(third)};
        ++result.counters.enumerated_unordered_triangle_count;
        const ExactFacetMiniballResult miniball =
            build_exact_facet_miniball(cloud, triangle);
        ++result.counters.local_exact_miniball_build_count;
        if (miniball.support_point_ids.size() == 2U) {
          ++result.counters.minimal_pair_support_triangle_count;
        } else if (miniball.support_point_ids.size() == 3U) {
          ++result.counters.minimal_triangle_support_triangle_count;
        } else {
          throw std::logic_error(
              "a distinct q=3 triplet has neither pair nor triangle support");
        }

        ExactQ3GabrielTriangleRecord record;
        record.triangle_point_ids = triangle;
        record.facet_point_ids = canonical_facets(triangle);
        record.minimal_support_kind =
            miniball.support_point_ids.size() == 2U
                ? ExactQ3GabrielMinimalSupportKind::pair
                : ExactQ3GabrielMinimalSupportKind::triangle;
        record.support_point_ids = miniball.support_point_ids;
        record.center = miniball.center;
        record.squared_level = miniball.squared_radius;

        for (std::size_t point_index = 0U;
             point_index < cloud.size();
             ++point_index) {
          const PointId point_id = static_cast<PointId>(point_index);
          const exact::SpherePointClassification classification =
              exact::classify_sphere_point(
                  record.center,
                  record.squared_level,
                  cloud.point(point_id));
          ++result.counters.full_cloud_point_classification_count;
          switch (classification.location()) {
            case exact::SpherePointLocation::strictly_inside:
              record.strict_interior_point_ids.push_back(point_id);
              ++result.counters.strictly_inside_classification_count;
              break;
            case exact::SpherePointLocation::boundary:
              ++result.counters.boundary_classification_count;
              if (!std::binary_search(
                      record.support_point_ids.begin(),
                      record.support_point_ids.end(),
                      point_id)) {
                record.extra_shell_point_ids.push_back(point_id);
              }
              break;
            case exact::SpherePointLocation::outside:
              record.exterior_point_ids.push_back(point_id);
              ++result.counters.exterior_classification_count;
              break;
          }
        }

        const bool no_strict_interior_omitted = std::includes(
            triangle.begin(),
            triangle.end(),
            record.strict_interior_point_ids.begin(),
            record.strict_interior_point_ids.end());
        if (!no_strict_interior_omitted) {
          ++result.counters.rejected_non_gabriel_triangle_count;
          continue;
        }
        if (!record_structurally_valid(record, cloud.size())) {
          throw std::logic_error(
              "an emitted exact q=3 Gabriel record is structurally invalid");
        }
        occurrences.push_back(std::move(record));
        ++result.counters.pre_dedup_gabriel_triangle_count;
      }
    }
  }

  if (result.counters.enumerated_unordered_triangle_count !=
      result.counters.expected_unordered_triangle_count) {
    throw std::logic_error(
        "the exhaustive q=3 oracle lost an unordered triangle");
  }

  std::sort(occurrences.begin(), occurrences.end(), triangle_key_less);
  result.triangles.reserve(occurrences.size());
  for (ExactQ3GabrielTriangleRecord& occurrence : occurrences) {
    if (!result.triangles.empty() &&
        result.triangles.back().triangle_point_ids ==
            occurrence.triangle_point_ids) {
      if (result.triangles.back() != occurrence) {
        throw std::logic_error(
            "duplicate q=3 Gabriel occurrences disagree on exact payload");
      }
      ++result.counters.duplicate_gabriel_triangle_occurrence_count;
      continue;
    }
    result.triangles.push_back(std::move(occurrence));
  }
  std::sort(result.triangles.begin(), result.triangles.end(), record_less);
  result.counters.output_gabriel_triangle_count = result.triangles.size();
  for (const ExactQ3GabrielTriangleRecord& record : result.triangles) {
    add_record_reference_counts(result.counters, record);
  }

  result.counters.complete_triangle_mass_accounting_closed =
      result.counters.rejected_non_gabriel_triangle_count +
              result.counters.pre_dedup_gabriel_triangle_count ==
          result.counters.enumerated_unordered_triangle_count &&
      result.counters.duplicate_gabriel_triangle_occurrence_count +
              result.counters.output_gabriel_triangle_count ==
          result.counters.pre_dedup_gabriel_triangle_count;
  result.counters.complete_point_classification_accounting_closed =
      result.counters.full_cloud_point_classification_count ==
          checked_multiply(
              result.counters.enumerated_unordered_triangle_count,
              cloud.size(),
              "the q=3 exact classification count overflows size_t") &&
      result.counters.strictly_inside_classification_count +
              result.counters.boundary_classification_count +
              result.counters.exterior_classification_count ==
          result.counters.full_cloud_point_classification_count;
  result.counters.canonical_sort_and_dedup_certified =
      std::is_sorted(
          result.triangles.begin(), result.triangles.end(), record_less) &&
      triangle_keys_unique(result.triangles);
  result.payload_digest = compute_payload_digest(result);
  if (!result.locally_structurally_valid()) {
    throw std::logic_error(
        "the exhaustive q=3 Gabriel oracle failed its local closure");
  }
  return result;
}

ExactQ3GabrielTriangleOracleVerification
verify_exact_q3_gabriel_triangle_oracle(
    const spatial::CanonicalPointCloud& cloud,
    const ExactQ3GabrielTriangleOracleResult& observed) {
  ExactQ3GabrielTriangleOracleVerification verification;
  if (cloud.size() == 0U ||
      cloud.size() > exact_q3_gabriel_triangle_oracle_maximum_point_count) {
    return verification;
  }
  const ExactQ3GabrielTriangleOracleResult expected =
      build_exact_q3_gabriel_triangle_oracle(cloud);
  const bool observed_structurally_valid =
      observed.locally_structurally_valid();
  verification.bounded_input_certified =
      cloud.size() != 0U &&
      cloud.size() <= exact_q3_gabriel_triangle_oracle_maximum_point_count &&
      observed.point_count == cloud.size();
  verification.schema_and_scope_certified =
      observed.schema_version ==
          exact_q3_gabriel_triangle_oracle_schema_version &&
      observed.counters.bounded_exhaustive_reference_only &&
      !observed.counters.public_status_claimed;
  verification.canonical_cloud_binding_certified =
      observed.canonical_cloud_digest == expected.canonical_cloud_digest;
  verification.complete_triangle_mass_certified =
      observed.counters.expected_unordered_triangle_count ==
          expected.counters.expected_unordered_triangle_count &&
      observed.counters.enumerated_unordered_triangle_count ==
          expected.counters.enumerated_unordered_triangle_count &&
      observed.counters.complete_triangle_mass_accounting_closed;
  verification.complete_exact_classification_certified =
      observed.counters.local_exact_miniball_build_count ==
          expected.counters.local_exact_miniball_build_count &&
      observed.counters.full_cloud_point_classification_count ==
          expected.counters.full_cloud_point_classification_count &&
      observed.counters.strictly_inside_classification_count ==
          expected.counters.strictly_inside_classification_count &&
      observed.counters.boundary_classification_count ==
          expected.counters.boundary_classification_count &&
      observed.counters.exterior_classification_count ==
          expected.counters.exterior_classification_count &&
      observed.counters.complete_point_classification_accounting_closed;
  verification.exact_record_payload_certified =
      observed.triangles == expected.triangles;
  verification.canonical_sort_and_dedup_certified =
      observed.counters.canonical_sort_and_dedup_certified &&
      observed_structurally_valid;
  verification.counters_certified =
      observed.counters == expected.counters;
  verification.payload_digest_certified =
      observed.payload_digest == expected.payload_digest &&
      observed_structurally_valid;
  verification.no_product_or_forbidden_global_structure_certified =
      !observed.counters.future_product_producer_called &&
      !observed.counters.delaunay_or_higher_order_mosaic_materialized;
  verification.gamma2_and_hierarchy_not_claimed_certified =
      !observed.counters.gamma2_completeness_claimed &&
      !observed.counters.silent_incidence_coverage_certified &&
      !observed.counters.hierarchy_reduction_performed &&
      !observed.counters.public_status_claimed;
  verification.fresh_replay_certified = observed == expected;
  return verification;
}

}  // namespace morsehgp3d::hierarchy
