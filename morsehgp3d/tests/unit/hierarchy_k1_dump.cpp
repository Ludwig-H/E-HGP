#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/rational.hpp"
#include "morsehgp3d/hierarchy/emst.hpp"
#include "morsehgp3d/hierarchy/gabriel.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::hierarchy::ExactEmstEdge;
using morsehgp3d::hierarchy::K1Cut;
using morsehgp3d::hierarchy::K1CutClosure;
using morsehgp3d::hierarchy::K1CutEdgeSource;
using morsehgp3d::hierarchy::K1EmstResult;
using morsehgp3d::hierarchy::K1ExactAnchorCertificate;
using morsehgp3d::hierarchy::K1ExactAnchorResult;
using morsehgp3d::hierarchy::K1HierarchyNode;
using morsehgp3d::hierarchy::K1Multifusion;
using morsehgp3d::hierarchy::K1PairSphereCatalog;
using morsehgp3d::hierarchy::K1PairSphereRecord;
using morsehgp3d::hierarchy::K1RankTwoCutEdgeSource;
using morsehgp3d::hierarchy::K1RankTwoReductionResult;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::PointId;

constexpr std::string_view protocol_header =
    "morsehgp3d-hierarchy-k1-v1";
constexpr std::string_view output_schema =
    "morsehgp3d.hierarchy_k1_dump.v1";

template <typename Integer>
[[nodiscard]] Integer parse_unsigned_decimal(
    std::string_view text, std::string_view field_name) {
  static_assert(std::numeric_limits<Integer>::is_integer);
  static_assert(!std::numeric_limits<Integer>::is_signed);
  if (text.empty() || (text.size() > 1U && text.front() == '0')) {
    throw std::invalid_argument(
        std::string{field_name} +
        " must be a canonical unsigned decimal integer");
  }
  Integer value{};
  const char* const begin = text.data();
  const char* const end = begin + text.size();
  const auto [position, error] = std::from_chars(begin, end, value, 10);
  if (error != std::errc{} || position != end) {
    throw std::invalid_argument(
        std::string{field_name} +
        " must be a canonical unsigned decimal integer");
  }
  return value;
}

[[nodiscard]] std::string next_token(
    std::istringstream& input, std::string_view field_name) {
  std::string token;
  if (!(input >> token)) {
    throw std::invalid_argument("missing " + std::string{field_name});
  }
  return token;
}

[[nodiscard]] std::size_t next_size(
    std::istringstream& input, std::string_view field_name) {
  return parse_unsigned_decimal<std::size_t>(
      next_token(input, field_name), field_name);
}

[[nodiscard]] std::uint64_t next_case_id(
    std::istringstream& input) {
  constexpr std::string_view field_name = "case_id";
  return parse_unsigned_decimal<std::uint64_t>(
      next_token(input, field_name), field_name);
}

void require_end_of_case(std::istringstream& input) {
  std::string extra;
  if (input >> extra) {
    throw std::invalid_argument(
        "unexpected token after the point payload: " + extra);
  }
}

void write_bool(std::ostream& output, bool value) {
  output << (value ? "true" : "false");
}

void write_ratio(std::ostream& output, const ExactRational& value) {
  output << "{\"denominator\":\""
         << morsehgp3d::exact::canonical_integer_string(value.denominator())
         << "\",\"numerator\":\""
         << morsehgp3d::exact::canonical_integer_string(value.numerator())
         << "\"}";
}

void write_ratio(std::ostream& output, const ExactLevel& value) {
  output << "{\"denominator\":\"" << value.denominator_string()
         << "\",\"numerator\":\"" << value.numerator_string() << "\"}";
}

void write_center(std::ostream& output, const ExactRational3& center) {
  output << "{\"x\":";
  write_ratio(output, center.coordinate(0U));
  output << ",\"y\":";
  write_ratio(output, center.coordinate(1U));
  output << ",\"z\":";
  write_ratio(output, center.coordinate(2U));
  output << '}';
}

void write_id_array(std::ostream& output, std::span<const PointId> ids) {
  output << '[';
  for (std::size_t index = 0U; index < ids.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    output << ids[index];
  }
  output << ']';
}

void write_components(std::ostream& output, const K1Cut& components) {
  output << '[';
  for (std::size_t index = 0U; index < components.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    write_id_array(output, components[index]);
  }
  output << ']';
}

void write_canonical_points(
    std::ostream& output, const CanonicalPointCloud& cloud) {
  output << '[';
  for (std::size_t index = 0U; index < cloud.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    const PointId id = static_cast<PointId>(index);
    const std::array<std::uint64_t, 3> bits =
        cloud.point(id).canonical_input_bits();
    output << "{\"id\":" << id << ",\"input_bits\":[\""
           << morsehgp3d::exact::binary64_hex(bits[0]) << "\",\""
           << morsehgp3d::exact::binary64_hex(bits[1]) << "\",\""
           << morsehgp3d::exact::binary64_hex(bits[2])
           << "\"],\"source_index\":" << cloud.source_index(id) << '}';
  }
  output << ']';
}

void write_edge(std::ostream& output, const ExactEmstEdge& edge) {
  output << "{\"level\":";
  write_ratio(output, edge.merge_level);
  output << ",\"squared_length\":";
  write_ratio(output, edge.squared_length);
  output << ",\"u\":" << edge.u << ",\"v\":" << edge.v << '}';
}

void write_edges(
    std::ostream& output, std::span<const ExactEmstEdge> edges) {
  output << '[';
  for (std::size_t index = 0U; index < edges.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    write_edge(output, edges[index]);
  }
  output << ']';
}

void write_node(std::ostream& output, const K1HierarchyNode& node) {
  output << "{\"children\":[";
  for (std::size_t index = 0U; index < node.children.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    output << node.children[index];
  }
  output << "],\"id\":" << node.id << ",\"level\":";
  write_ratio(output, node.level);
  output << ",\"point_ids\":";
  write_id_array(output, node.point_ids);
  output << '}';
}

void write_nodes(
    std::ostream& output, std::span<const K1HierarchyNode> nodes) {
  output << '[';
  for (std::size_t index = 0U; index < nodes.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    write_node(output, nodes[index]);
  }
  output << ']';
}

void write_multifusion(
    std::ostream& output,
    const ExactLevel& level,
    const K1Multifusion& multifusion) {
  output << "{\"child_components\":";
  write_components(output, multifusion.child_components);
  output << ",\"level\":";
  write_ratio(output, level);
  output << ",\"merged_component\":";
  write_id_array(output, multifusion.merged_component);
  output << '}';
}

template <typename Batch>
void write_multifusions(
    std::ostream& output, std::span<const Batch> batches) {
  output << '[';
  bool first = true;
  for (const Batch& batch : batches) {
    for (const K1Multifusion& multifusion : batch.multifusions) {
      if (!first) {
        output << ',';
      }
      first = false;
      write_multifusion(output, batch.level, multifusion);
    }
  }
  output << ']';
}

void write_pair(
    std::ostream& output, const K1PairSphereRecord& pair) {
  output << "{\"center\":";
  write_center(output, pair.center);
  output << ",\"classification\":\""
         << morsehgp3d::hierarchy::to_string(pair.classification)
         << "\",\"closed_rank\":" << pair.closed_rank
         << ",\"exterior_count\":" << pair.exterior_count
         << ",\"interior_ids\":";
  write_id_array(output, pair.interior_ids);
  output << ",\"level\":";
  write_ratio(output, pair.level);
  output << ",\"shell_ids\":";
  write_id_array(output, pair.shell_ids);
  output << ",\"squared_length\":";
  write_ratio(output, pair.squared_length);
  output << ",\"u\":" << pair.u << ",\"v\":" << pair.v << '}';
}

void write_catalog(
    std::ostream& output, const K1PairSphereCatalog& catalog) {
  output << "{\"catalog_status\":\""
         << morsehgp3d::hierarchy::to_string(catalog.catalog_status)
         << "\",\"gabriel_edges\":";
  write_edges(output, catalog.gabriel_diagnostic_edges);
  output << ",\"pairs\":[";
  for (std::size_t index = 0U; index < catalog.pairs.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    write_pair(output, catalog.pairs[index]);
  }
  output << "],\"rank_two_edges\":";
  write_edges(output, catalog.rank_two_edges);
  output << '}';
}

void write_certificate(
    std::ostream& output, const K1ExactAnchorCertificate& certificate) {
  output << "{\"all_selected_witness_edges_are_rank_two\":";
  write_bool(output, certificate.all_selected_witness_edges_are_rank_two);
  output << ",\"anchor_equivalence_certified\":";
  write_bool(output, certificate.anchor_equivalence_certified);
  output << ",\"closed_cuts_match\":";
  write_bool(output, certificate.closed_cuts_match);
  output << ",\"comparison_level_count\":"
         << certificate.comparison_level_count
         << ",\"exact_pair_decisions_complete\":";
  write_bool(output, certificate.exact_pair_decisions_complete);
  output << ",\"multifusions_match\":";
  write_bool(output, certificate.multifusions_match);
  output << ",\"pair_universe_matches_emst\":";
  write_bool(output, certificate.pair_universe_matches_emst);
  output << ",\"selected_tree_edges_match\":";
  write_bool(output, certificate.selected_tree_edges_match);
  output << ",\"selected_tree_hgp_weight_matches\":";
  write_bool(output, certificate.selected_tree_hgp_weight_matches);
  output << ",\"selected_tree_hierarchy_matches\":";
  write_bool(output, certificate.selected_tree_hierarchy_matches);
  output << ",\"selected_tree_squared_weight_matches\":";
  write_bool(output, certificate.selected_tree_squared_weight_matches);
  output << ",\"strict_cuts_match\":";
  write_bool(output, certificate.strict_cuts_match);
  output << '}';
}

void write_emst(std::ostream& output, const K1EmstResult& emst) {
  output << "{\"multifusions\":";
  write_multifusions(output, std::span{emst.equal_level_batches});
  output << ",\"nodes\":";
  write_nodes(output, emst.nodes);
  output << ",\"root_node_id\":" << emst.root_node_id
         << ",\"selected_edges\":";
  write_edges(output, emst.emst_edges);
  output << ",\"total_hgp_weight\":";
  write_ratio(output, emst.total_hgp_weight);
  output << ",\"total_squared_weight\":";
  write_ratio(output, emst.total_squared_weight);
  output << '}';
}

void write_rank_two(
    std::ostream& output, const K1RankTwoReductionResult& rank_two) {
  output << "{\"multifusions\":";
  write_multifusions(output, std::span{rank_two.equal_level_batches});
  output << ",\"nodes\":";
  write_nodes(output, rank_two.nodes);
  output << ",\"root_node_id\":" << rank_two.root_node_id
         << ",\"selected_edges\":";
  write_edges(output, rank_two.selected_witness_edges);
  output << ",\"total_hgp_weight\":";
  write_ratio(output, rank_two.total_selected_hgp_weight);
  output << ",\"total_squared_weight\":";
  write_ratio(output, rank_two.total_selected_squared_weight);
  output << '}';
}

[[nodiscard]] std::vector<ExactLevel> comparison_levels(
    const K1PairSphereCatalog& catalog) {
  std::vector<ExactLevel> levels;
  levels.reserve(catalog.pairs.size() + 1U);
  levels.emplace_back();
  for (const K1PairSphereRecord& pair : catalog.pairs) {
    levels.push_back(pair.level);
  }
  std::sort(levels.begin(), levels.end());
  levels.erase(std::unique(levels.begin(), levels.end()), levels.end());
  return levels;
}

void write_cut(
    std::ostream& output,
    const K1ExactAnchorResult& anchor,
    const ExactLevel& level,
    K1CutClosure closure) {
  output << "{\"closed\":";
  write_bool(output, closure == K1CutClosure::closed);
  output << ",\"complete_graph\":";
  write_components(
      output,
      anchor.emst.cut(level, closure, K1CutEdgeSource::complete_graph));
  output << ",\"gabriel_diagnostic_graph\":";
  write_components(
      output,
      anchor.rank_two_reduction.cut(
          level,
          closure,
          K1RankTwoCutEdgeSource::gabriel_diagnostic_graph));
  output << ",\"level\":";
  write_ratio(output, level);
  output << ",\"rank_two_graph\":";
  write_components(
      output,
      anchor.rank_two_reduction.cut(
          level, closure, K1RankTwoCutEdgeSource::rank_two_graph));
  output << ",\"selected_emst\":";
  write_components(
      output,
      anchor.emst.cut(level, closure, K1CutEdgeSource::selected_emst));
  output << ",\"selected_rank_two_witness\":";
  write_components(
      output,
      anchor.rank_two_reduction.cut(
          level, closure, K1RankTwoCutEdgeSource::selected_witness_tree));
  output << '}';
}

void write_cuts(
    std::ostream& output, const K1ExactAnchorResult& anchor) {
  const std::vector<ExactLevel> levels =
      comparison_levels(anchor.pair_catalog);
  output << '[';
  bool first = true;
  for (const ExactLevel& level : levels) {
    for (const K1CutClosure closure :
         {K1CutClosure::strict, K1CutClosure::closed}) {
      if (!first) {
        output << ',';
      }
      first = false;
      write_cut(output, anchor, level, closure);
    }
  }
  output << ']';
}

void process_case(std::string_view line) {
  std::istringstream input{std::string{line}};
  if (next_token(input, "case marker") != "case") {
    throw std::invalid_argument(
        "a protocol payload line must start with 'case'");
  }

  const std::uint64_t case_id = next_case_id(input);
  const std::size_t point_count = next_size(input, "point_count");
  std::vector<CertifiedPoint3> source_points;
  source_points.reserve(point_count);
  for (std::size_t index = 0U; index < point_count; ++index) {
    std::array<std::uint64_t, 3> bits{};
    for (std::size_t axis = 0U; axis < bits.size(); ++axis) {
      bits[axis] = morsehgp3d::exact::parse_binary64_hex(
          next_token(input, "point_binary64_word"), false);
    }
    source_points.push_back(CertifiedPoint3::from_binary64_bits(bits));
  }
  require_end_of_case(input);

  const CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(source_points);
  const K1ExactAnchorResult anchor =
      morsehgp3d::hierarchy::build_exact_k1_anchor(cloud);

  std::cout << "{\"canonical_points\":";
  write_canonical_points(std::cout, cloud);
  std::cout << ",\"case_id\":" << case_id << ",\"catalog\":";
  write_catalog(std::cout, anchor.pair_catalog);
  std::cout << ",\"certificate\":";
  write_certificate(std::cout, anchor.certificate);
  std::cout << ",\"cuts\":";
  write_cuts(std::cout, anchor);
  std::cout << ",\"emst\":";
  write_emst(std::cout, anchor.emst);
  std::cout << ",\"locally_supported\":";
  write_bool(std::cout, anchor.locally_supported());
  std::cout << ",\"rank_two\":";
  write_rank_two(std::cout, anchor.rank_two_reduction);
  std::cout << ",\"schema\":\"" << output_schema << "\"}\n";
}

}  // namespace

int main() {
  try {
    std::string line;
    if (!std::getline(std::cin, line) || line != protocol_header) {
      throw std::invalid_argument("missing hierarchy-k1 protocol header");
    }

    bool saw_end = false;
    while (std::getline(std::cin, line)) {
      if (line == "end") {
        saw_end = true;
        break;
      }
      if (line.empty()) {
        throw std::invalid_argument("blank protocol lines are forbidden");
      }
      process_case(line);
    }
    if (!saw_end) {
      throw std::invalid_argument("missing hierarchy-k1 protocol terminator");
    }
    while (std::getline(std::cin, line)) {
      if (!line.empty()) {
        throw std::invalid_argument(
            "data after the protocol terminator is forbidden");
      }
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "hierarchy_k1_dump: " << error.what() << '\n';
    return 2;
  }
}
