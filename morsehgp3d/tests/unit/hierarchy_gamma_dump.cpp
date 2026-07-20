#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/hierarchy/gamma.hpp"
#include "morsehgp3d/hierarchy/gamma_transition.hpp"
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
using morsehgp3d::hierarchy::ExactGammaTransitionDecision;
using morsehgp3d::hierarchy::ExactGammaTransitionGroupKind;
using morsehgp3d::hierarchy::ExactGammaTransitionResult;
using morsehgp3d::hierarchy::ExactStrictGammaCofaceWitness;
using morsehgp3d::hierarchy::ExactStrictGammaComponentWitness;
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

struct TransitionFixedCase {
  std::string_view name;
  std::vector<CertifiedPoint3> input_points;
  std::vector<std::string_view> input_labels;
  std::size_t order{};
  ExactLevel squared_level;
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

void write_indices(
    std::ostream& output,
    std::span<const std::size_t> indices) {
  output << '[';
  for (std::size_t index = 0U; index < indices.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    output << indices[index];
  }
  output << ']';
}

void write_canonical_points(
    std::ostream& output,
    const CanonicalPointCloud& cloud,
    std::span<const std::string_view> input_labels) {
  output << '[';
  for (std::size_t index = 0U; index < cloud.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    const PointId point_id = static_cast<PointId>(index);
    const std::size_t source_index = cloud.source_index(point_id);
    const auto input_bits = cloud.point(point_id).canonical_input_bits();
    output << "{\"id\":" << point_id << ",\"input_bits\":[\""
           << morsehgp3d::exact::binary64_hex(input_bits[0]) << "\",\""
           << morsehgp3d::exact::binary64_hex(input_bits[1]) << "\",\""
           << morsehgp3d::exact::binary64_hex(input_bits[2])
           << "\"],\"label\":\""
           << input_labels[source_index]
           << "\",\"source_index\":" << source_index << '}';
  }
  output << ']';
}

void write_components(
    std::ostream& output,
    std::span<const ExactStrictGammaComponentWitness> components) {
  output << '[';
  for (std::size_t index = 0U; index < components.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    const ExactStrictGammaComponentWitness& component = components[index];
    output << "{\"canonical_representative_facet_point_ids\":";
    write_point_ids(
        output, component.canonical_representative_facet_point_ids);
    output << ",\"facet_point_ids\":";
    write_facets(output, component.facet_point_ids);
    output << '}';
  }
  output << ']';
}

void write_cofaces(
    std::ostream& output,
    std::span<const ExactStrictGammaCofaceWitness> cofaces) {
  output << '[';
  for (std::size_t index = 0U; index < cofaces.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    const ExactStrictGammaCofaceWitness& coface = cofaces[index];
    output << "{\"facet_point_ids\":";
    write_facets(output, coface.facet_point_ids);
    output << ",\"point_ids\":";
    write_point_ids(output, coface.coface_point_ids);
    output << ",\"squared_level\":";
    write_level(output, coface.squared_level);
    output << '}';
  }
  output << ']';
}

template <typename FacetWitness>
void write_facet_witnesses(
    std::ostream& output,
    std::span<const FacetWitness> facets) {
  output << '[';
  for (std::size_t index = 0U; index < facets.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    output << "{\"point_ids\":";
    write_point_ids(output, facets[index].facet_point_ids);
    output << ",\"squared_level\":";
    write_level(output, facets[index].squared_level);
    output << '}';
  }
  output << ']';
}

[[nodiscard]] std::string_view transition_group_kind_name(
    ExactGammaTransitionGroupKind kind) {
  switch (kind) {
    case ExactGammaTransitionGroupKind::
        new_closed_component_without_strict_component:
      return "new_closed_component_without_strict_component";
    case ExactGammaTransitionGroupKind::
        one_strict_component_continuation:
      return "one_strict_component_continuation";
    case ExactGammaTransitionGroupKind::
        multiple_strict_component_coalescence:
      return "multiple_strict_component_coalescence";
  }
  throw std::logic_error("an unknown Gamma transition group kind was dumped");
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
  std::cout << "],\"canonical_points\":";
  write_canonical_points(std::cout, cloud, fixed_case.input_labels);
  std::cout << ",\"case\":\"" << fixed_case.name
            << "\",\"closed\":false,\"components\":";
  write_components(std::cout, result.components);
  std::cout << ",\"order\":" << result.order
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

void write_transition_case(const TransitionFixedCase& fixed_case) {
  if (fixed_case.input_points.size() != fixed_case.input_labels.size()) {
    throw std::logic_error(
        "a fixed Gamma-transition case has mismatched point labels");
  }
  const CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(fixed_case.input_points);
  const ExactGammaTransitionResult result =
      morsehgp3d::hierarchy::build_exact_gamma_equal_level_transition(
          cloud,
          fixed_case.order,
          fixed_case.squared_level,
          exhaustive_budget());
  if (result.decision != ExactGammaTransitionDecision::
                             complete_exhaustive_open_to_closed_transition) {
    throw std::logic_error(
        "a fixed Gamma-transition differential did not complete");
  }

  std::cout << "{\"canonical_points\":";
  write_canonical_points(std::cout, cloud, fixed_case.input_labels);
  std::cout << ",\"case\":\"" << fixed_case.name
            << "\",\"closed_components\":";
  write_components(std::cout, result.closed_components);
  std::cout << ",\"equal_level_cofaces\":";
  write_cofaces(std::cout, result.equal_level_cofaces);
  std::cout << ",\"equal_level_facets\":";
  write_facet_witnesses(
      std::cout,
      std::span{result.equal_level_facets});
  std::cout << ",\"equal_level_incidences\":[";
  for (std::size_t index = 0U;
       index < result.equal_level_incidences.size();
       ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    const auto& incidence = result.equal_level_incidences[index];
    std::cout << "{\"coface_point_ids\":";
    write_point_ids(std::cout, incidence.coface_point_ids);
    std::cout << ",\"facet_point_ids\":";
    write_point_ids(std::cout, incidence.facet_point_ids);
    std::cout << ",\"newly_active_at_level\":"
              << (incidence.newly_active_at_level ? "true" : "false")
              << ",\"strict_component_index\":";
    if (incidence.strict_component_index.has_value()) {
      std::cout << *incidence.strict_component_index;
    } else {
      std::cout << "null";
    }
    std::cout << '}';
  }
  std::cout << "],\"order\":" << result.order
            << ",\"squared_level\":";
  write_level(std::cout, result.squared_level);
  std::cout << ",\"strict_active_cofaces\":";
  write_cofaces(std::cout, result.strict_gamma.active_cofaces);
  std::cout << ",\"strict_active_facets\":";
  write_facet_witnesses(
      std::cout,
      std::span{result.strict_gamma.active_facets});
  std::cout << ",\"strict_component_to_closed_component_index\":";
  write_indices(
      std::cout,
      result.strict_component_to_closed_component_index);
  std::cout << ",\"strict_components\":";
  write_components(std::cout, result.strict_gamma.components);
  std::cout << ",\"transition_groups\":[";
  for (std::size_t index = 0U;
       index < result.transition_groups.size();
       ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    const auto& group = result.transition_groups[index];
    std::cout << "{\"canonical_representative_facet_point_ids\":";
    write_point_ids(
        std::cout, group.canonical_representative_facet_point_ids);
    std::cout << ",\"closed_component_index\":"
              << group.closed_component_index
              << ",\"equal_level_coface_point_ids\":";
    write_facets(std::cout, group.equal_level_coface_point_ids);
    std::cout << ",\"kind\":\""
              << transition_group_kind_name(group.kind)
              << "\",\"newly_active_facet_point_ids\":";
    write_facets(std::cout, group.newly_active_facet_point_ids);
    std::cout << ",\"strict_component_indices\":";
    write_indices(std::cout, group.strict_component_indices);
    std::cout << '}';
  }
  std::cout << "]}\n";
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

[[nodiscard]] std::vector<TransitionFixedCase> transition_fixed_cases() {
  std::vector<TransitionFixedCase> cases;
  cases.push_back(TransitionFixedCase{
      "isolated_equal_facet",
      {point(0.0, 0.0), point(2.0, 0.0), point(10.0, 0.0)},
      {"A", "B", "C"},
      2U,
      ExactLevel{BigInt{1}}});
  cases.push_back(TransitionFixedCase{
      "silent_equal_coface",
      {point(0.0, 0.0), point(1.0, 0.0), point(2.0, 0.0)},
      {"A", "B", "C"},
      1U,
      ExactLevel{BigInt{1}}});
  cases.push_back(TransitionFixedCase{
      "equal_facet_coalescence",
      {point(-2.0, 0.0), point(0.0, 1.0), point(2.0, 0.0)},
      {"A", "B", "C"},
      2U,
      ExactLevel{BigInt{4}}});
  cases.push_back(TransitionFixedCase{
      "overlapping_equal_cofaces",
      {point(-2.0, 0.0),
       point(0.0, -3.0),
       point(0.0, 3.0),
       point(2.0, 0.0)},
      {"A", "B", "C", "D"},
      2U,
      ExactLevel{BigInt{169}, BigInt{36}}});
  cases.push_back(TransitionFixedCase{
      "disconnected_q1_q0_q2",
      {point(-4.0, -2.0, 0.0),
       point(-4.0, 2.0, 0.0),
       point(0.0, 0.0, 0.0),
       point(6.0, 4.0, 0.0),
       point(8.0, -2.0, 0.0),
       point(40.0, 0.0, 0.0),
       point(46.0, 6.0, 8.0),
       point(80.0, 0.0, 0.0),
       point(83.0, 3.0, 4.0),
       point(86.0, 6.0, 8.0)},
      {"A0", "A1", "A2", "A3", "A4",
       "B0", "B1", "C0", "C1", "C2"},
      2U,
      ExactLevel{BigInt{34}}});
  cases.push_back(TransitionFixedCase{
      "order_ten_equal_coface",
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
      ExactLevel{BigInt{25}}});
  return cases;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 2 && std::string_view{argv[1]} == "--transitions") {
      for (const TransitionFixedCase& fixed_case :
           transition_fixed_cases()) {
        write_transition_case(fixed_case);
      }
      return 0;
    }
    if (argc != 1) {
      throw std::invalid_argument(
          "usage: hierarchy_gamma_dump [--transitions]");
    }
    for (const FixedCase& fixed_case : fixed_cases()) {
      write_case(fixed_case);
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "hierarchy Gamma dump failed: " << error.what() << '\n';
    return 1;
  }
}
