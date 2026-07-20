#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/hierarchy/gamma.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::hierarchy::ExactStrictGammaBudget;
using morsehgp3d::hierarchy::ExactStrictGammaResult;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::PointId;

struct FixedCase {
  std::string_view name;
  std::vector<CertifiedPoint3> input_points;
  std::vector<std::string_view> input_labels;
  std::size_t order{};
  ExactLevel strict_cut_squared_level;
  std::vector<std::vector<std::size_t>> source_input_indices;
};

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] ExactStrictGammaBudget exhaustive_budget() {
  return ExactStrictGammaBudget{
      ExactStrictGammaBudget::maximum_supported_facet_count,
      ExactStrictGammaBudget::maximum_supported_coface_count,
      ExactStrictGammaBudget::maximum_supported_union_attempt_count};
}

void write_level(std::ostream& output, const ExactLevel& level) {
  output << "{\"denominator\":\"" << level.denominator_string()
         << "\",\"numerator\":\"" << level.numerator_string() << "\"}";
}

void write_point_ids(
    std::ostream& output,
    std::span<const PointId> point_ids) {
  output << '[';
  for (std::size_t index = 0U; index < point_ids.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    output << point_ids[index];
  }
  output << ']';
}

void write_facets(
    std::ostream& output,
    std::span<const std::vector<PointId>> facets) {
  output << '[';
  for (std::size_t index = 0U; index < facets.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    write_point_ids(output, facets[index]);
  }
  output << ']';
}

[[nodiscard]] std::vector<PointId> point_ids_by_source_index(
    const CanonicalPointCloud& cloud) {
  std::vector<PointId> result(cloud.size());
  for (std::size_t index = 0U; index < cloud.size(); ++index) {
    const PointId point_id = static_cast<PointId>(index);
    result.at(cloud.source_index(point_id)) = point_id;
  }
  return result;
}

[[nodiscard]] std::vector<std::vector<PointId>> canonical_sources(
    const FixedCase& fixed_case,
    std::span<const PointId> point_id_by_source_index) {
  std::vector<std::vector<PointId>> sources;
  sources.reserve(fixed_case.source_input_indices.size());
  for (const std::vector<std::size_t>& input_indices :
       fixed_case.source_input_indices) {
    std::vector<PointId> source;
    source.reserve(input_indices.size());
    for (const std::size_t input_index : input_indices) {
      if (input_index >= point_id_by_source_index.size()) {
        throw std::logic_error(
            "a fixed Gamma source index is outside its point cloud");
      }
      source.push_back(point_id_by_source_index[input_index]);
    }
    std::sort(source.begin(), source.end());
    sources.push_back(std::move(source));
  }
  return sources;
}

void write_case(const FixedCase& fixed_case) {
  if (fixed_case.input_points.size() != fixed_case.input_labels.size()) {
    throw std::logic_error("a fixed Gamma case has mismatched point labels");
  }
  const CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(fixed_case.input_points);
  const std::vector<PointId> point_id_by_source_index =
      point_ids_by_source_index(cloud);
  const std::vector<std::vector<PointId>> sources = canonical_sources(
      fixed_case, point_id_by_source_index);
  const ExactStrictGammaResult result =
      morsehgp3d::hierarchy::
          build_exact_strict_gamma_source_classification(
              cloud,
              fixed_case.order,
              fixed_case.strict_cut_squared_level,
              std::span<const std::vector<PointId>>{sources},
              exhaustive_budget());
  if (!result.all_sources_active_and_classified) {
    throw std::logic_error(
        "a fixed Gamma differential source is not active at its open cut");
  }

  std::cout << "{\"active_cofaces\":[";
  for (std::size_t index = 0U; index < result.active_cofaces.size(); ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    const auto& coface = result.active_cofaces[index];
    std::cout << "{\"facet_point_ids\":";
    write_facets(std::cout, coface.facet_point_ids);
    std::cout << ",\"point_ids\":";
    write_point_ids(std::cout, coface.coface_point_ids);
    std::cout << ",\"squared_level\":";
    write_level(std::cout, coface.squared_level);
    std::cout << '}';
  }
  std::cout << "],\"active_facets\":[";
  for (std::size_t index = 0U; index < result.active_facets.size(); ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    const auto& facet = result.active_facets[index];
    std::cout << "{\"point_ids\":";
    write_point_ids(std::cout, facet.facet_point_ids);
    std::cout << ",\"squared_level\":";
    write_level(std::cout, facet.squared_level);
    std::cout << '}';
  }
  std::cout << "],\"canonical_points\":[";
  for (std::size_t index = 0U; index < cloud.size(); ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    const PointId point_id = static_cast<PointId>(index);
    const std::size_t source_index = cloud.source_index(point_id);
    const auto input_bits = cloud.point(point_id).canonical_input_bits();
    std::cout << "{\"id\":" << point_id << ",\"input_bits\":[\""
              << morsehgp3d::exact::binary64_hex(input_bits[0]) << "\",\""
              << morsehgp3d::exact::binary64_hex(input_bits[1]) << "\",\""
              << morsehgp3d::exact::binary64_hex(input_bits[2])
              << "\"],\"label\":\""
              << fixed_case.input_labels.at(source_index)
              << "\",\"source_index\":" << source_index << '}';
  }
  std::cout << "],\"case\":\"" << fixed_case.name
            << "\",\"closed\":false,\"components\":[";
  for (std::size_t index = 0U; index < result.components.size(); ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    const auto& component = result.components[index];
    std::cout << "{\"canonical_representative_facet_point_ids\":";
    write_point_ids(
        std::cout, component.canonical_representative_facet_point_ids);
    std::cout << ",\"facet_point_ids\":";
    write_facets(std::cout, component.facet_point_ids);
    std::cout << '}';
  }
  std::cout << "],\"order\":" << result.order
            << ",\"source_classifications\":[";
  for (std::size_t index = 0U;
       index < result.source_classifications.size();
       ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    const auto& classification = result.source_classifications[index];
    std::cout << "{\"active_strictly_below_cut\":"
              << (classification.active_strictly_below_cut
                      ? "true"
                      : "false")
              << ",\"component_index\":";
    if (classification.component_index.has_value()) {
      std::cout << *classification.component_index;
    } else {
      std::cout << "null";
    }
    std::cout << ",\"source_facet_point_ids\":";
    write_point_ids(std::cout, classification.source_facet_point_ids);
    std::cout << ",\"squared_level\":";
    write_level(std::cout, classification.squared_level);
    std::cout << '}';
  }
  std::cout << "],\"strict_cut_squared_level\":";
  write_level(std::cout, result.strict_cut_squared_level);
  std::cout << "}\n";
}

[[nodiscard]] std::vector<FixedCase> fixed_cases() {
  std::vector<FixedCase> cases;
  cases.push_back(FixedCase{
      "binary_ac",
      {point(-2.0, 0.0), point(2.0, 0.0)},
      {"A", "C"},
      1U,
      ExactLevel{BigInt{4}},
      {{0U}, {1U}}});
  cases.push_back(FixedCase{
      "mediated_apc",
      {point(-2.0, 0.0), point(0.0, 3.0), point(2.0, 0.0)},
      {"A", "P", "C"},
      1U,
      ExactLevel{BigInt{4}},
      {{0U}, {2U}}});
  cases.push_back(FixedCase{
      "ternary_abcpq",
      {point(-2.0, 0.0),
       point(0.0, 3.0),
       point(2.0, 0.0),
       point(2.0, 2.0),
       point(-2.0, 2.0)},
      {"A", "B", "C", "P", "Q"},
      2U,
      ExactLevel{BigInt{169}, BigInt{36}},
      {{1U, 2U}, {0U, 2U}, {0U, 1U}}});
  cases.push_back(FixedCase{
      "gabriel_silent_incidences",
      {point(0.0, 0.0, 7.0),
       point(0.0, 9.0, 6.0),
       point(1.0, 4.0, 0.0),
       point(0.0, 0.0, 1.0),
       point(4.0, 1.0, 2.0)},
      {"A", "B", "C", "D", "E"},
      2U,
      ExactLevel{BigInt{83886}, BigInt{3563}},
      {{0U, 2U}, {3U, 4U}, {0U, 1U}, {1U, 2U}}});
  cases.push_back(FixedCase{
      "order_ten_eleven_point_coface",
      {point(0.0, 0.0),
       point(1.0, 0.0),
       point(2.0, 0.0),
       point(3.0, 0.0),
       point(4.0, 0.0),
       point(5.0, 0.0),
       point(6.0, 0.0),
       point(7.0, 0.0),
       point(8.0, 0.0),
       point(9.0, 0.0),
       point(10.0, 0.0)},
      {"P0", "P1", "P2", "P3", "P4", "P5",
       "P6", "P7", "P8", "P9", "P10"},
      10U,
      ExactLevel{BigInt{26}},
      {{0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U}}});
  return cases;
}

}  // namespace

int main() {
  try {
    for (const FixedCase& fixed_case : fixed_cases()) {
      write_case(fixed_case);
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "hierarchy Gamma dump failed: " << error.what() << '\n';
    return 1;
  }
}
