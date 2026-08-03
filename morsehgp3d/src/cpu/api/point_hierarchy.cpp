#include "morsehgp3d/api/point_hierarchy.hpp"

#include "morsehgp3d/exact/integer.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace morsehgp3d::api {
namespace {

using exact::BigInt;
using exact::ExactLevel;
using exact::ExactRational;

[[nodiscard]] std::size_t checked_size(std::uint64_t value, std::string_view label) {
  if (value > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    throw std::length_error(std::string{label} + " does not fit size_t");
  }
  return static_cast<std::size_t>(value);
}

[[nodiscard]] bool is_zero_id(const contract::CanonicalId& id) noexcept {
  return std::all_of(id.bytes().begin(), id.bytes().end(), [](std::uint8_t byte) {
    return byte == 0U;
  });
}

[[nodiscard]] BigInt positive_power(BigInt base, std::uint32_t exponent) {
  BigInt result = 1;
  while (exponent != 0U) {
    if ((exponent & 1U) != 0U) {
      result *= base;
    }
    exponent >>= 1U;
    if (exponent != 0U) {
      base *= base;
    }
  }
  return result;
}

[[nodiscard]] std::uint32_t unsigned_gcd(
    std::uint32_t left, std::uint32_t right) noexcept {
  while (right != 0U) {
    const std::uint32_t remainder = left % right;
    left = right;
    right = remainder;
  }
  return left;
}

[[nodiscard]] PositiveRationalExponent normalize_exponent(
    PositiveRationalExponent exponent) {
  if (exponent.numerator == 0U || exponent.denominator == 0U) {
    throw std::invalid_argument("exp_z must be a strictly positive rational");
  }
  constexpr std::uint32_t maximum_component = 64U;
  if (exponent.numerator > maximum_component ||
      exponent.denominator > maximum_component) {
    throw std::invalid_argument("exp_z numerator and denominator must not exceed 64");
  }
  const std::uint32_t divisor =
      unsigned_gcd(exponent.numerator, exponent.denominator);
  exponent.numerator /= divisor;
  exponent.denominator /= divisor;
  return exponent;
}

[[nodiscard]] int compare_density_impl(
    const DensityLevel& left,
    const DensityLevel& right,
    const PositiveRationalExponent exponent) {
  if (left.order == 0U || right.order == 0U) {
    throw std::invalid_argument("a density order must be strictly positive");
  }
  const bool left_infinite = left.squared_radius.numerator() == 0;
  const bool right_infinite = right.squared_radius.numerator() == 0;
  if (left_infinite || right_infinite) {
    if (left_infinite && right_infinite) {
      return 0;
    }
    return left_infinite ? 1 : -1;
  }

  const std::uint32_t order_exponent = 2U * exponent.denominator;
  const BigInt left_value =
      positive_power(BigInt{left.order}, order_exponent) *
      positive_power(right.squared_radius.numerator(), exponent.numerator) *
      positive_power(left.squared_radius.denominator(), exponent.numerator);
  const BigInt right_value =
      positive_power(BigInt{right.order}, order_exponent) *
      positive_power(left.squared_radius.numerator(), exponent.numerator) *
      positive_power(right.squared_radius.denominator(), exponent.numerator);
  if (left_value < right_value) {
    return -1;
  }
  if (left_value > right_value) {
    return 1;
  }
  return 0;
}

struct RationalInterval {
  ExactRational lower;
  ExactRational upper;
};

[[nodiscard]] RationalInterval add_intervals(
    const RationalInterval& left, const RationalInterval& right) {
  return RationalInterval{left.lower + right.lower, left.upper + right.upper};
}

[[nodiscard]] RationalInterval multiply_interval(
    const RationalInterval& interval, const ExactRational& positive_factor) {
  if (positive_factor.sign() < 0) {
    throw std::logic_error("an interval scale must be nonnegative");
  }
  return RationalInterval{
      interval.lower * positive_factor, interval.upper * positive_factor};
}

[[nodiscard]] RationalInterval divide_positive_intervals(
    const RationalInterval& numerator, const RationalInterval& denominator) {
  if (denominator.lower.sign() <= 0) {
    throw std::domain_error("a certified interval denominator must be positive");
  }
  return RationalInterval{
      numerator.lower / denominator.upper,
      numerator.upper / denominator.lower};
}

[[nodiscard]] BigInt integer_nth_root_floor(
    const BigInt& value, const std::uint32_t degree) {
  if (value < 0 || degree == 0U) {
    throw std::invalid_argument("integer_nth_root_floor has an invalid argument");
  }
  if (value <= 1 || degree == 1U) {
    return value;
  }

  const std::size_t most_significant_bit = boost::multiprecision::msb(value);
  const std::size_t root_bits =
      most_significant_bit / static_cast<std::size_t>(degree) + 2U;
  if (root_bits > static_cast<std::size_t>(
                      std::numeric_limits<unsigned int>::max())) {
    throw std::length_error("the integer root estimate is too wide");
  }
  BigInt estimate =
      BigInt{1} << static_cast<unsigned int>(root_bits);
  const BigInt degree_value = degree;
  for (;;) {
    const BigInt divisor = positive_power(estimate, degree - 1U);
    const BigInt next =
        ((degree_value - 1) * estimate + value / divisor) / degree_value;
    if (next >= estimate) {
      break;
    }
    estimate = next;
  }
  while (positive_power(estimate, degree) > value) {
    --estimate;
  }
  while (positive_power(estimate + 1, degree) <= value) {
    ++estimate;
  }
  return estimate;
}

[[nodiscard]] RationalInterval inverse_radius_power_interval(
    const ExactLevel& squared_radius,
    const PositiveRationalExponent exponent,
    const std::uint32_t precision_bits) {
  if (squared_radius.numerator() == 0) {
    throw std::domain_error("inverse-radius weights are undefined at radius zero");
  }

  std::uint32_t radical_numerator = exponent.numerator;
  std::uint32_t radical_denominator = 2U * exponent.denominator;
  const std::uint32_t divisor =
      unsigned_gcd(radical_numerator, radical_denominator);
  radical_numerator /= divisor;
  radical_denominator /= divisor;

  const BigInt base_numerator = positive_power(
      squared_radius.denominator(), radical_numerator);
  const BigInt base_denominator = positive_power(
      squared_radius.numerator(), radical_numerator);
  if (radical_denominator == 1U) {
    const ExactRational exact_value{base_numerator, base_denominator};
    return RationalInterval{exact_value, exact_value};
  }

  const std::uint64_t shift_width =
      static_cast<std::uint64_t>(precision_bits) * radical_denominator;
  if (shift_width > static_cast<std::uint64_t>(
                        std::numeric_limits<unsigned int>::max())) {
    throw std::length_error("the radical enclosure shift is too large");
  }
  const BigInt scaled_numerator =
      base_numerator << static_cast<unsigned int>(shift_width);
  const BigInt scaled_floor = scaled_numerator / base_denominator;
  const BigInt root =
      integer_nth_root_floor(scaled_floor, radical_denominator);
  const BigInt scale = exact::power_of_two(precision_bits);
  const bool exact_root =
      positive_power(root, radical_denominator) * base_denominator ==
      scaled_numerator;
  const ExactRational lower{root, scale};
  const ExactRational upper =
      exact_root ? lower : ExactRational{root + 1, scale};
  return RationalInterval{lower, upper};
}

struct DisjointSet {
  explicit DisjointSet(std::size_t size)
      : parent(size), rank(size, 0U) {
    std::iota(parent.begin(), parent.end(), std::size_t{0U});
  }

  [[nodiscard]] std::size_t find(std::size_t value) {
    std::size_t root = value;
    while (parent[root] != root) {
      root = parent[root];
    }
    while (parent[value] != value) {
      const std::size_t next = parent[value];
      parent[value] = root;
      value = next;
    }
    return root;
  }

  [[nodiscard]] std::size_t unite(std::size_t left, std::size_t right) {
    left = find(left);
    right = find(right);
    if (left == right) {
      return left;
    }
    if (rank[left] < rank[right]) {
      std::swap(left, right);
    }
    parent[right] = left;
    if (rank[left] == rank[right]) {
      ++rank[left];
    }
    return left;
  }

  std::vector<std::size_t> parent;
  std::vector<std::uint8_t> rank;
};

struct ValidatedInput {
  PositiveRationalExponent exponent;
  std::unordered_map<SourceNodeId, std::size_t> source_node_index;
  std::vector<ExactRational> order_weights;
};

[[nodiscard]] contract::CanonicalId compute_tower_payload_id_impl(
    const CertifiedTowerInput& input) {
  contract::CanonicalSha256Builder builder;
  builder.update("MorseHGP3D/v1/certified-tower-payload/");
  builder.update("n=");
  builder.update(std::to_string(input.point_count));
  builder.update("/K=");
  builder.update(std::to_string(input.maximum_order));
  builder.update("/forest_receipt=");
  builder.update(input.receipts.all_orders_forest_receipt_id.to_lower_hex());
  builder.update("/vertical_receipt=");
  builder.update(input.receipts.vertical_maps_receipt_id.to_lower_hex());
  builder.update("/incidence_receipt=");
  builder.update(input.receipts.projectable_incidences_receipt_id.to_lower_hex());
  builder.update(input.receipts.all_orders_complete ? "/forests=complete" :
                                                       "/forests=incomplete");
  builder.update(input.receipts.vertical_maps_complete ? "/vertical=complete" :
                                                        "/vertical=incomplete");
  builder.update(input.receipts.projectable_incidences_complete
                     ? "/incidences=complete"
                     : "/incidences=incomplete");
  builder.update(input.receipts.exact_source_certified ? "/source=certified" :
                                                       "/source=uncertified");
  builder.update(input.receipts.surrogate_source_used ? "/surrogate=yes" :
                                                      "/surrogate=no");

  std::vector<const CertifiedTowerNode*> nodes;
  nodes.reserve(input.nodes.size());
  for (const CertifiedTowerNode& node : input.nodes) {
    nodes.push_back(&node);
  }
  std::sort(nodes.begin(), nodes.end(), [](const auto* left, const auto* right) {
    return left->source_node_id < right->source_node_id;
  });
  for (const CertifiedTowerNode* node : nodes) {
    builder.update("/node=");
    builder.update(std::to_string(node->source_node_id));
    builder.update(":");
    builder.update(std::to_string(node->order));
    builder.update("@");
    builder.update(node->squared_radius.canonical_key());
  }

  struct CanonicalEdgeRecord {
    SourceNodeId first{0U};
    SourceNodeId second{0U};
    TowerEdgeKind kind{TowerEdgeKind::horizontal_forest};
    DensityLevel activation{};
  };
  std::vector<CanonicalEdgeRecord> edges;
  edges.reserve(input.edges.size());
  for (const CertifiedTowerEdge& edge : input.edges) {
    SourceNodeId first = edge.first_source_node_id;
    SourceNodeId second = edge.second_source_node_id;
    if (edge.kind == TowerEdgeKind::horizontal_forest && second < first) {
      std::swap(first, second);
    }
    edges.push_back(CanonicalEdgeRecord{
        first, second, edge.kind, edge.activation_level});
  }
  std::sort(edges.begin(), edges.end(), [](const auto& left, const auto& right) {
    if (left.kind != right.kind) {
      return left.kind < right.kind;
    }
    if (left.first != right.first) {
      return left.first < right.first;
    }
    if (left.second != right.second) {
      return left.second < right.second;
    }
    if (left.activation.order != right.activation.order) {
      return left.activation.order < right.activation.order;
    }
    return left.activation.squared_radius < right.activation.squared_radius;
  });
  for (const CanonicalEdgeRecord& edge : edges) {
    builder.update("/edge=");
    builder.update(std::to_string(static_cast<std::uint8_t>(edge.kind)));
    builder.update(":");
    builder.update(std::to_string(edge.first));
    builder.update("->");
    builder.update(std::to_string(edge.second));
    builder.update("@");
    builder.update(std::to_string(edge.activation.order));
    builder.update(":");
    builder.update(edge.activation.squared_radius.canonical_key());
  }

  std::vector<const CertifiedProjectableSimplex*> simplices;
  simplices.reserve(input.projectable_simplices.size());
  for (const CertifiedProjectableSimplex& simplex : input.projectable_simplices) {
    simplices.push_back(&simplex);
  }
  std::sort(
      simplices.begin(), simplices.end(), [](const auto* left, const auto* right) {
        return left->source_simplex_id < right->source_simplex_id;
      });
  for (const CertifiedProjectableSimplex* simplex : simplices) {
    builder.update("/simplex=");
    builder.update(std::to_string(simplex->source_simplex_id));
    builder.update(":");
    builder.update(std::to_string(simplex->order));
    builder.update("@carrier=");
    builder.update(std::to_string(simplex->carrier_source_node_id));
    for (const PointId point : simplex->point_ids) {
      builder.update(",p=");
      builder.update(std::to_string(point));
    }
    std::vector<ExactLevel> atoms = simplex->incident_coface_squared_radii;
    std::sort(atoms.begin(), atoms.end());
    for (const ExactLevel& atom : atoms) {
      builder.update(",a=");
      builder.update(atom.canonical_key());
    }
    builder.update(",exit=");
    builder.update(std::to_string(simplex->terminal_exit_level.order));
    builder.update("@");
    builder.update(simplex->terminal_exit_level.squared_radius.canonical_key());
  }
  return builder.finalize();
}

void require_receipt_declarations(const CertifiedTowerInput& input) {
  const TowerSourceReceipts& receipts = input.receipts;
  if (is_zero_id(receipts.all_orders_forest_receipt_id) ||
      is_zero_id(receipts.vertical_maps_receipt_id) ||
      is_zero_id(receipts.projectable_incidences_receipt_id)) {
    throw ExactCertificationError("all three upstream receipt identifiers are required");
  }
  if (!receipts.all_orders_complete || !receipts.vertical_maps_complete ||
      !receipts.projectable_incidences_complete ||
      !receipts.exact_source_certified) {
    throw ExactCertificationError(
        "the tower source is not declared complete and exact by its producer");
  }
  if (receipts.surrogate_source_used) {
    throw ExactCertificationError("a surrogate source is forbidden by the exact API");
  }
}

void verify_payload_binding(const CertifiedTowerInput& input) {
  const TowerSourceReceipts& receipts = input.receipts;
  if (is_zero_id(receipts.tower_payload_id) ||
      receipts.tower_payload_id != compute_tower_payload_id_impl(input)) {
    throw ExactCertificationError(
        "the upstream receipts are not bound to the canonical tower payload");
  }
}

[[nodiscard]] ValidatedInput validate_input(
    const CertifiedTowerInput& input, const PointHierarchyOptions& options) {
  require_receipt_declarations(input);
  if (input.point_count == 0U) {
    throw std::invalid_argument("point_count must be strictly positive");
  }
  static_cast<void>(checked_size(input.point_count, "point_count"));
  if (input.maximum_order == 0U) {
    throw std::invalid_argument("maximum_order must be strictly positive");
  }
  if (input.maximum_order > input.point_count ||
      input.maximum_order > input.nodes.size()) {
    throw std::invalid_argument("maximum_order cannot exceed points or source nodes");
  }
  if (input.nodes.empty()) {
    throw std::invalid_argument("the certified tower must contain nodes");
  }
  const auto size_as_u64 = [](const std::size_t value,
                              const std::string_view label) {
    if (value > static_cast<std::size_t>(
                    std::numeric_limits<std::uint64_t>::max())) {
      throw std::length_error(std::string{label} + " does not fit uint64");
    }
    return static_cast<std::uint64_t>(value);
  };
  const std::uint64_t source_node_count =
      size_as_u64(input.nodes.size(), "source node count");
  const std::uint64_t source_edge_count =
      size_as_u64(input.edges.size(), "source edge count");
  const std::uint64_t projectable_simplex_count = size_as_u64(
      input.projectable_simplices.size(), "projectable simplex count");
  if (input.point_count > options.budget.maximum_points ||
      source_node_count > options.budget.maximum_source_nodes ||
      source_edge_count > options.budget.maximum_source_edges ||
      projectable_simplex_count >
          options.budget.maximum_projectable_simplices) {
    throw std::length_error("the point-hierarchy build budget is exceeded");
  }
  if (options.budget.maximum_hierarchy_nodes == 0U ||
      source_node_count > options.budget.maximum_hierarchy_nodes - 1U ||
      source_edge_count > options.budget.maximum_hierarchy_nodes - 1U -
                              source_node_count) {
    throw std::length_error("the hierarchy-node preflight budget is exceeded");
  }
  if (options.budget.initial_certification_bits < 8U ||
      options.budget.maximum_certification_bits <
          options.budget.initial_certification_bits ||
      options.budget.maximum_certification_bits > 4096U) {
    throw std::invalid_argument("the adaptive certification bit budget is invalid");
  }
  if (options.budget.maximum_exact_integer_bits == 0U ||
      options.budget.maximum_exact_integer_bits > 1'000'000U) {
    throw std::invalid_argument("the exact-integer bit budget is invalid");
  }
  if (options.simplex_point_weighting !=
          SimplexPointWeighting::inverse_radius &&
      options.simplex_point_weighting != SimplexPointWeighting::uniform) {
    throw std::invalid_argument("the simplex-to-point weighting mode is invalid");
  }

  const auto require_bounded_integer = [&](const BigInt& value) {
    const BigInt magnitude = value < 0 ? -value : value;
    const std::size_t width =
        magnitude == 0 ? 1U : boost::multiprecision::msb(magnitude) + 1U;
    if (width > options.budget.maximum_exact_integer_bits) {
      throw std::length_error("an exact scalar exceeds the configured bit budget");
    }
  };
  const auto require_bounded_level = [&](const ExactLevel& value) {
    require_bounded_integer(value.numerator());
    require_bounded_integer(value.denominator());
  };
  std::uint64_t preflight_incidences = 0U;
  std::uint64_t preflight_atoms = 0U;
  for (const CertifiedTowerNode& node : input.nodes) {
    require_bounded_level(node.squared_radius);
  }
  for (const CertifiedTowerEdge& edge : input.edges) {
    require_bounded_level(edge.activation_level.squared_radius);
  }
  for (const CertifiedProjectableSimplex& simplex : input.projectable_simplices) {
    const std::uint64_t simplex_incidences =
        size_as_u64(simplex.point_ids.size(), "simplex incidence count");
    const std::uint64_t simplex_atoms = size_as_u64(
        simplex.incident_coface_squared_radii.size(), "coface atom count");
    if (simplex_incidences >
            options.budget.maximum_point_simplex_incidences ||
        preflight_incidences >
            options.budget.maximum_point_simplex_incidences -
                simplex_incidences ||
        simplex_atoms > options.budget.maximum_coface_radius_atoms ||
        preflight_atoms > options.budget.maximum_coface_radius_atoms -
                              simplex_atoms) {
      throw std::length_error("the simplex payload budget is exceeded");
    }
    preflight_incidences += simplex_incidences;
    preflight_atoms += simplex_atoms;
    for (const ExactLevel& atom : simplex.incident_coface_squared_radii) {
      require_bounded_level(atom);
    }
    require_bounded_level(simplex.terminal_exit_level.squared_radius);
  }
  for (const OrderWeight& order_weight : options.order_weights) {
    require_bounded_integer(order_weight.weight.numerator());
    require_bounded_integer(order_weight.weight.denominator());
  }
  verify_payload_binding(input);

  ValidatedInput validated;
  validated.exponent = normalize_exponent(options.exp_z);
  validated.source_node_index.reserve(input.nodes.size());
  std::vector<bool> seen_order(
      static_cast<std::size_t>(input.maximum_order) + 1U, false);
  for (std::size_t index = 0U; index < input.nodes.size(); ++index) {
    const CertifiedTowerNode& node = input.nodes[index];
    if (node.order == 0U || node.order > input.maximum_order) {
      throw std::invalid_argument("a tower node order lies outside 1,...,K");
    }
    if (!validated.source_node_index.emplace(node.source_node_id, index).second) {
      throw std::invalid_argument("source_node_id values must be unique");
    }
    seen_order[node.order] = true;
  }
  for (std::uint32_t order = 1U; order <= input.maximum_order; ++order) {
    if (!seen_order[order]) {
      throw ExactCertificationError("the source omits one of the trees T_1,...,T_K");
    }
  }

  // Horizontal node domains are disjoint across orders, hence one DSU detects
  // every within-order cycle without allocating O(K * node_count) storage.
  DisjointSet horizontal_forests(input.nodes.size());
  std::vector<std::optional<ExactLevel>> carrier_death_radius(input.nodes.size());
  std::vector<std::size_t> vertical_edge_indices;
  std::set<std::tuple<TowerEdgeKind, SourceNodeId, SourceNodeId, std::uint32_t, std::string>>
      seen_edges;
  for (const CertifiedTowerEdge& edge : input.edges) {
    const auto first_iterator =
        validated.source_node_index.find(edge.first_source_node_id);
    const auto second_iterator =
        validated.source_node_index.find(edge.second_source_node_id);
    if (first_iterator == validated.source_node_index.end() ||
        second_iterator == validated.source_node_index.end()) {
      throw std::invalid_argument("a tower edge references an unknown source node");
    }
    if (first_iterator->second == second_iterator->second) {
      throw std::invalid_argument("tower self-edges are forbidden");
    }
    const std::uint32_t first_order = input.nodes[first_iterator->second].order;
    const std::uint32_t second_order = input.nodes[second_iterator->second].order;
    if (edge.activation_level.order == 0U ||
        edge.activation_level.order > input.maximum_order) {
      throw std::invalid_argument("a tower edge has an invalid activation order");
    }
    if (edge.kind == TowerEdgeKind::horizontal_forest) {
      if (first_order != second_order ||
          edge.activation_level.order != first_order) {
        throw ExactCertificationError("a horizontal edge crosses two orders");
      }
      if (horizontal_forests.find(first_iterator->second) ==
          horizontal_forests.find(second_iterator->second)) {
        throw ExactCertificationError("a horizontal Hartigan source contains a cycle");
      }
      static_cast<void>(horizontal_forests.unite(
          first_iterator->second, second_iterator->second));
      for (const std::size_t endpoint :
           std::array<std::size_t, 2U>{
               first_iterator->second, second_iterator->second}) {
        const ExactLevel& birth = input.nodes[endpoint].squared_radius;
        const ExactLevel& activation = edge.activation_level.squared_radius;
        if (birth < activation) {
          if (carrier_death_radius[endpoint].has_value() &&
              *carrier_death_radius[endpoint] != activation) {
            throw ExactCertificationError(
                "a horizontal carrier has more than one later absorption level");
          }
          carrier_death_radius[endpoint] = activation;
        }
      }
    } else if (edge.kind == TowerEdgeKind::adjacent_order_vertical) {
      const std::uint32_t difference = first_order > second_order
                                           ? first_order - second_order
                                           : second_order - first_order;
      if (difference != 1U) {
        throw ExactCertificationError("a vertical edge must join adjacent orders");
      }
      if (first_order != second_order + 1U ||
          edge.activation_level.order != second_order) {
        throw ExactCertificationError(
            "a vertical edge must be oriented from order k+1 to order k");
      }
      vertical_edge_indices.push_back(
          static_cast<std::size_t>(&edge - input.edges.data()));
    } else {
      throw std::invalid_argument("the tower edge kind is invalid");
    }
    const DensityLevel first_level{
        first_order, input.nodes[first_iterator->second].squared_radius};
    const DensityLevel second_level{
        second_order, input.nodes[second_iterator->second].squared_radius};
    if (compare_density_impl(
            edge.activation_level, first_level, validated.exponent) > 0 ||
        compare_density_impl(
            edge.activation_level, second_level, validated.exponent) > 0) {
      throw ExactCertificationError(
          "a tower edge activates before one of its endpoints");
    }
    SourceNodeId canonical_first = edge.first_source_node_id;
    SourceNodeId canonical_second = edge.second_source_node_id;
    if (edge.kind == TowerEdgeKind::horizontal_forest &&
        canonical_second < canonical_first) {
      std::swap(canonical_first, canonical_second);
    }
    if (!seen_edges
             .emplace(
                 edge.kind,
                 canonical_first,
                 canonical_second,
                 edge.activation_level.order,
                 edge.activation_level.squared_radius.canonical_key())
             .second) {
      throw ExactCertificationError("a canonical tower edge is duplicated");
    }
  }
  for (const std::size_t edge_index : vertical_edge_indices) {
    const CertifiedTowerEdge& edge = input.edges[edge_index];
    const std::array<std::size_t, 2U> endpoints{
        validated.source_node_index.at(edge.first_source_node_id),
        validated.source_node_index.at(edge.second_source_node_id)};
    for (const std::size_t endpoint : endpoints) {
      const ExactLevel& cut = edge.activation_level.squared_radius;
      if (cut < input.nodes[endpoint].squared_radius) {
        throw ExactCertificationError(
            "a vertical edge precedes the raw birth radius of a carrier");
      }
      if (carrier_death_radius[endpoint].has_value() &&
          !(cut < *carrier_death_radius[endpoint])) {
        throw ExactCertificationError(
            "a vertical edge does not lie in the raw lifetime of a carrier");
      }
    }
  }

  validated.order_weights.assign(
      static_cast<std::size_t>(input.maximum_order) + 1U,
      ExactRational{BigInt{1}});
  std::vector<bool> explicit_weight(
      static_cast<std::size_t>(input.maximum_order) + 1U, false);
  for (const OrderWeight& order_weight : options.order_weights) {
    if (order_weight.order == 0U || order_weight.order > input.maximum_order) {
      throw std::invalid_argument("an order weight lies outside 1,...,K");
    }
    if (order_weight.weight.sign() <= 0) {
      throw std::invalid_argument("order weights must be strictly positive");
    }
    if (explicit_weight[order_weight.order]) {
      throw std::invalid_argument("an order weight is specified twice");
    }
    explicit_weight[order_weight.order] = true;
    validated.order_weights[order_weight.order] = order_weight.weight;
  }

  std::unordered_set<SourceSimplexId> simplex_ids;
  simplex_ids.reserve(input.projectable_simplices.size());
  std::vector<bool> order_one_point_seen(
      checked_size(input.point_count, "point_count"), false);
  std::uint64_t incidence_count = 0U;
  for (const CertifiedProjectableSimplex& simplex : input.projectable_simplices) {
    if (!simplex_ids.insert(simplex.source_simplex_id).second) {
      throw std::invalid_argument("source_simplex_id values must be unique");
    }
    if (simplex.order == 0U || simplex.order > input.maximum_order ||
        simplex.point_ids.size() != simplex.order) {
      throw std::invalid_argument("a projectable simplex has an invalid order");
    }
    const auto carrier =
        validated.source_node_index.find(simplex.carrier_source_node_id);
    if (carrier == validated.source_node_index.end() ||
        input.nodes[carrier->second].order != simplex.order) {
      throw ExactCertificationError("a simplex carrier is absent or has the wrong order");
    }
    if (simplex.incident_coface_squared_radii.empty()) {
      throw ExactCertificationError("a projectable simplex has no certified coface radius");
    }
    if (simplex.terminal_exit_level.order != simplex.order) {
      throw ExactCertificationError(
          "a projectable simplex exit level has the wrong order");
    }
    const DensityLevel carrier_level{
        simplex.order, input.nodes[carrier->second].squared_radius};
    if (compare_density_impl(
            simplex.terminal_exit_level,
            carrier_level,
            validated.exponent) != 0) {
      throw ExactCertificationError(
          "V1 requires a projectable simplex exit at its carrier density");
    }
    if (!std::is_sorted(simplex.point_ids.begin(), simplex.point_ids.end()) ||
        std::adjacent_find(simplex.point_ids.begin(), simplex.point_ids.end()) !=
            simplex.point_ids.end()) {
      throw std::invalid_argument("simplex point_ids must be strictly increasing");
    }
    for (const PointId point_id : simplex.point_ids) {
      if (point_id >= input.point_count) {
        throw std::invalid_argument("a simplex point_id lies outside the point cloud");
      }
      if (simplex.order == 1U) {
        const std::size_t point_index = checked_size(point_id, "point id");
        if (order_one_point_seen[point_index]) {
          throw ExactCertificationError(
              "a T1 singleton is attached to more than one carrier");
        }
        order_one_point_seen[point_index] = true;
      }
    }
    for (const ExactLevel& radius : simplex.incident_coface_squared_radii) {
      if (options.simplex_point_weighting ==
              SimplexPointWeighting::inverse_radius &&
          radius.numerator() == 0) {
        throw ExactCertificationError(
            "inverse-radius simplex weights require strictly positive radii");
      }
    }
    if (simplex.point_ids.size() >
        static_cast<std::size_t>(std::numeric_limits<std::uint64_t>::max())) {
      throw std::length_error("a simplex incidence count does not fit uint64");
    }
    const std::uint64_t simplex_incidence_count =
        static_cast<std::uint64_t>(simplex.point_ids.size());
    if (simplex_incidence_count >
            options.budget.maximum_point_simplex_incidences ||
        incidence_count > options.budget.maximum_point_simplex_incidences -
                              simplex_incidence_count) {
      throw std::length_error("the point-simplex incidence budget is exceeded");
    }
    incidence_count += simplex_incidence_count;
  }
  if (std::find(order_one_point_seen.begin(), order_one_point_seen.end(), false) !=
      order_one_point_seen.end()) {
    throw ExactCertificationError(
        "the complete T1 projectable stream must cover every point exactly once");
  }
  std::vector<std::size_t> canonical_simplex_indices(
      input.projectable_simplices.size());
  std::iota(
      canonical_simplex_indices.begin(),
      canonical_simplex_indices.end(),
      std::size_t{0U});
  std::sort(
      canonical_simplex_indices.begin(),
      canonical_simplex_indices.end(),
      [&](std::size_t left, std::size_t right) {
        return input.projectable_simplices[left].point_ids <
               input.projectable_simplices[right].point_ids;
      });
  for (std::size_t index = 1U; index < canonical_simplex_indices.size(); ++index) {
    if (input.projectable_simplices[canonical_simplex_indices[index - 1U]].point_ids ==
        input.projectable_simplices[canonical_simplex_indices[index]].point_ids) {
      throw ExactCertificationError(
          "a projectable simplex point set occurs more than once");
    }
  }
  return validated;
}

struct TopologyResult {
  std::vector<PointHierarchyNode> nodes;
  PointHierarchyNodeId virtual_root{0U};
  std::vector<PointHierarchyNodeId> carrier_by_source_index;
  std::vector<std::uint64_t> topology_dfs_begin;
  std::vector<std::uint64_t> topology_dfs_end;
};

struct FiltrationEvent {
  DensityLevel level;
  bool is_vertex{false};
  std::size_t index{0U};
};

[[nodiscard]] TopologyResult build_tower_topology(
    const CertifiedTowerInput& input,
    const ValidatedInput& validated) {
  std::vector<FiltrationEvent> events;
  events.reserve(input.nodes.size() + input.edges.size());
  for (std::size_t index = 0U; index < input.nodes.size(); ++index) {
    events.push_back(FiltrationEvent{
        DensityLevel{input.nodes[index].order, input.nodes[index].squared_radius},
        true,
        index});
  }
  for (std::size_t index = 0U; index < input.edges.size(); ++index) {
    events.push_back(
        FiltrationEvent{input.edges[index].activation_level, false, index});
  }
  std::sort(events.begin(), events.end(), [&](const FiltrationEvent& left,
                                              const FiltrationEvent& right) {
    const int comparison =
        compare_density_impl(left.level, right.level, validated.exponent);
    if (comparison != 0) {
      return comparison > 0;
    }
    if (left.level.order != right.level.order) {
      return left.level.order < right.level.order;
    }
    if (left.level.squared_radius != right.level.squared_radius) {
      return left.level.squared_radius < right.level.squared_radius;
    }
    if (left.is_vertex != right.is_vertex) {
      return left.is_vertex;
    }
    return left.index < right.index;
  });

  DisjointSet components(input.nodes.size());
  std::vector<bool> active(input.nodes.size(), false);
  constexpr PointHierarchyNodeId no_node =
      std::numeric_limits<PointHierarchyNodeId>::max();
  std::vector<PointHierarchyNodeId> component_tree(input.nodes.size(), no_node);
  std::vector<std::size_t> new_vertex_batch(
      input.nodes.size(), std::numeric_limits<std::size_t>::max());
  TopologyResult topology;
  topology.carrier_by_source_index.assign(input.nodes.size(), no_node);

  std::size_t event_begin = 0U;
  while (event_begin < events.size()) {
    std::size_t event_end = event_begin + 1U;
    while (event_end < events.size() &&
           compare_density_impl(
               events[event_begin].level,
               events[event_end].level,
               validated.exponent) == 0) {
      ++event_end;
    }

    std::vector<std::size_t> new_vertices;
    std::vector<std::size_t> batch_edges;
    for (std::size_t event_index = event_begin; event_index < event_end;
         ++event_index) {
      if (events[event_index].is_vertex) {
        const std::size_t vertex = events[event_index].index;
        if (active[vertex]) {
          throw std::logic_error("a source vertex was activated twice");
        }
        active[vertex] = true;
        new_vertex_batch[vertex] = event_begin;
        new_vertices.push_back(vertex);
      } else {
        batch_edges.push_back(events[event_index].index);
      }
    }

    std::map<std::size_t, std::size_t> old_component_representative;
    for (const std::size_t edge_index : batch_edges) {
      const CertifiedTowerEdge& edge = input.edges[edge_index];
      const std::array<std::size_t, 2U> endpoints{
          validated.source_node_index.at(edge.first_source_node_id),
          validated.source_node_index.at(edge.second_source_node_id)};
      for (const std::size_t endpoint : endpoints) {
        if (!active[endpoint]) {
          throw ExactCertificationError(
              "an edge is active before one of its source vertices");
        }
        if (new_vertex_batch[endpoint] != event_begin) {
          const std::size_t old_root = components.find(endpoint);
          old_component_representative.emplace(old_root, endpoint);
        }
      }
    }
    for (const std::size_t edge_index : batch_edges) {
      const CertifiedTowerEdge& edge = input.edges[edge_index];
      static_cast<void>(components.unite(
          validated.source_node_index.at(edge.first_source_node_id),
          validated.source_node_index.at(edge.second_source_node_id)));
    }

    struct AffectedComponent {
      std::set<PointHierarchyNodeId> children;
      std::vector<std::size_t> direct_vertices;
    };
    std::map<std::size_t, AffectedComponent> affected;
    for (const std::size_t vertex : new_vertices) {
      affected[components.find(vertex)].direct_vertices.push_back(vertex);
    }
    for (const auto& [old_root, representative] : old_component_representative) {
      const PointHierarchyNodeId child = component_tree[old_root];
      if (child == no_node) {
        throw std::logic_error("an active component has no hierarchy carrier");
      }
      affected[components.find(representative)].children.insert(child);
    }

    std::vector<std::pair<std::size_t, AffectedComponent>> canonical_groups;
    canonical_groups.reserve(affected.size());
    for (auto& entry : affected) {
      std::sort(
          entry.second.direct_vertices.begin(),
          entry.second.direct_vertices.end(),
          [&](std::size_t left, std::size_t right) {
            return input.nodes[left].source_node_id < input.nodes[right].source_node_id;
          });
      canonical_groups.emplace_back(entry.first, std::move(entry.second));
    }
    std::sort(
        canonical_groups.begin(),
        canonical_groups.end(),
        [&](const auto& left, const auto& right) {
          const auto& left_children = left.second.children;
          const auto& right_children = right.second.children;
          if (left_children != right_children) {
            return std::lexicographical_compare(
                left_children.begin(),
                left_children.end(),
                right_children.begin(),
                right_children.end());
          }
          return std::lexicographical_compare(
              left.second.direct_vertices.begin(),
              left.second.direct_vertices.end(),
              right.second.direct_vertices.begin(),
              right.second.direct_vertices.end(),
              [&](std::size_t left_vertex, std::size_t right_vertex) {
                return input.nodes[left_vertex].source_node_id <
                       input.nodes[right_vertex].source_node_id;
              });
        });
    for (auto& [root, group] : canonical_groups) {
      if (group.direct_vertices.empty() && group.children.size() == 1U) {
        component_tree[root] = *group.children.begin();
        continue;
      }
      PointHierarchyNode node;
      node.node_id = static_cast<PointHierarchyNodeId>(topology.nodes.size());
      node.event_level = events[event_begin].level;
      node.child_node_ids.assign(group.children.begin(), group.children.end());
      for (const std::size_t vertex : group.direct_vertices) {
        node.direct_source_node_ids.push_back(input.nodes[vertex].source_node_id);
        topology.carrier_by_source_index[vertex] = node.node_id;
      }
      topology.nodes.push_back(std::move(node));
      const PointHierarchyNodeId parent = topology.nodes.back().node_id;
      for (const PointHierarchyNodeId child : topology.nodes.back().child_node_ids) {
        topology.nodes[checked_size(child, "hierarchy child")].parent_node_id = parent;
      }
      component_tree[root] = parent;
    }
    event_begin = event_end;
  }

  std::set<PointHierarchyNodeId> natural_roots;
  for (std::size_t vertex = 0U; vertex < input.nodes.size(); ++vertex) {
    if (!active[vertex]) {
      throw std::logic_error("a source vertex was not activated");
    }
    const std::size_t root = components.find(vertex);
    if (component_tree[root] == no_node) {
      throw std::logic_error("a final source component has no hierarchy node");
    }
    natural_roots.insert(component_tree[root]);
  }
  for (const PointHierarchyNodeId carrier : topology.carrier_by_source_index) {
    if (carrier == no_node) {
      throw std::logic_error("a source vertex has no point-hierarchy carrier");
    }
  }

  PointHierarchyNode virtual_root;
  virtual_root.node_id = static_cast<PointHierarchyNodeId>(topology.nodes.size());
  virtual_root.child_node_ids.assign(natural_roots.begin(), natural_roots.end());
  topology.nodes.push_back(std::move(virtual_root));
  topology.virtual_root = topology.nodes.back().node_id;
  for (const PointHierarchyNodeId root : topology.nodes.back().child_node_ids) {
    topology.nodes[checked_size(root, "natural root")].parent_node_id =
        topology.virtual_root;
  }

  topology.topology_dfs_begin.assign(topology.nodes.size(), 0U);
  topology.topology_dfs_end.assign(topology.nodes.size(), 0U);
  std::uint64_t clock = 0U;
  struct DfsFrame {
    PointHierarchyNodeId node_id{0U};
    std::size_t next_child{0U};
  };
  std::vector<DfsFrame> stack;
  stack.push_back(DfsFrame{topology.virtual_root, 0U});
  topology.topology_dfs_begin[checked_size(
      topology.virtual_root, "virtual root")] = clock++;
  while (!stack.empty()) {
    DfsFrame& frame = stack.back();
    const std::size_t node_index = checked_size(frame.node_id, "hierarchy node");
    if (frame.next_child < topology.nodes[node_index].child_node_ids.size()) {
      const PointHierarchyNodeId child =
          topology.nodes[node_index].child_node_ids[frame.next_child++];
      topology.topology_dfs_begin[checked_size(child, "hierarchy child")] = clock++;
      stack.push_back(DfsFrame{child, 0U});
    } else {
      topology.topology_dfs_end[node_index] = clock;
      stack.pop_back();
    }
  }
  return topology;
}

struct FaceData {
  SourceSimplexId source_simplex_id{0U};
  std::uint32_t order{0U};
  PointHierarchyNodeId carrier{0U};
  std::vector<ExactLevel> atom_levels;
  DensityLevel terminal_exit_level{};
};

[[nodiscard]] bool is_descendant(
    const TopologyResult& topology,
    const PointHierarchyNodeId ancestor,
    const PointHierarchyNodeId candidate) {
  const std::size_t ancestor_index = checked_size(ancestor, "ancestor");
  const std::size_t candidate_index = checked_size(candidate, "candidate");
  return topology.topology_dfs_begin[ancestor_index] <=
             topology.topology_dfs_begin[candidate_index] &&
         topology.topology_dfs_end[candidate_index] <=
             topology.topology_dfs_end[ancestor_index];
}

[[nodiscard]] PointHierarchyNodeId immediate_child_for_carrier(
    const TopologyResult& topology,
    const PointHierarchyNodeId node,
    const PointHierarchyNodeId carrier) {
  const auto& children =
      topology.nodes[checked_size(node, "routing node")].child_node_ids;
  const std::uint64_t carrier_begin = topology.topology_dfs_begin[checked_size(
      carrier, "routing carrier")];
  auto candidate = std::upper_bound(
      children.begin(),
      children.end(),
      carrier_begin,
      [&](const std::uint64_t value, const PointHierarchyNodeId child) {
        return value < topology.topology_dfs_begin[checked_size(
                           child, "routing child")];
      });
  if (candidate != children.begin()) {
    --candidate;
    if (is_descendant(topology, *candidate, carrier)) {
      return *candidate;
    }
  }
  throw std::logic_error("a contribution carrier is outside the routing subtree");
}

struct Candidate {
  bool stay{false};
  PointHierarchyNodeId destination{0U};
  std::vector<std::size_t> face_indices;
};

struct PointWeightContext {
  std::map<std::uint32_t, std::vector<std::size_t>> faces_by_order;
  std::map<std::uint32_t, ExactRational> alpha_by_order;
};

[[nodiscard]] RationalInterval face_score_interval(
    const FaceData& face,
    const PointHierarchyOptions& options,
    const PositiveRationalExponent exponent,
    const std::uint32_t precision_bits) {
  if (options.simplex_point_weighting == SimplexPointWeighting::uniform) {
    const ExactRational one{BigInt{1}};
    return RationalInterval{one, one};
  }
  RationalInterval score{ExactRational{}, ExactRational{}};
  for (const ExactLevel& atom : face.atom_levels) {
    score = add_intervals(
        score,
        inverse_radius_power_interval(atom, exponent, precision_bits));
  }
  return score;
}

[[nodiscard]] std::vector<RationalInterval> candidate_score_intervals(
    const std::vector<Candidate>& candidates,
    const PointWeightContext& context,
    const std::vector<FaceData>& faces,
    const PointHierarchyOptions& options,
    const PositiveRationalExponent exponent,
    const std::uint32_t precision_bits) {
  std::map<std::size_t, RationalInterval> face_scores;
  std::map<std::uint32_t, RationalInterval> denominator_by_order;
  for (const auto& [order, order_faces] : context.faces_by_order) {
    RationalInterval denominator{ExactRational{}, ExactRational{}};
    for (const std::size_t face_index : order_faces) {
      const RationalInterval score =
          face_score_interval(faces[face_index], options, exponent, precision_bits);
      face_scores.emplace(face_index, score);
      denominator = add_intervals(denominator, score);
    }
    denominator_by_order.emplace(order, std::move(denominator));
  }

  std::vector<RationalInterval> results(
      candidates.size(), RationalInterval{ExactRational{}, ExactRational{}});
  for (std::size_t candidate_index = 0U;
       candidate_index < candidates.size();
       ++candidate_index) {
    std::map<std::uint32_t, RationalInterval> numerator_by_order;
    for (const std::size_t face_index : candidates[candidate_index].face_indices) {
      const FaceData& face = faces[face_index];
      auto [iterator, inserted] = numerator_by_order.emplace(
          face.order,
          RationalInterval{ExactRational{}, ExactRational{}});
      static_cast<void>(inserted);
      iterator->second =
          add_intervals(iterator->second, face_scores.at(face_index));
    }
    for (const auto& [order, numerator] : numerator_by_order) {
      results[candidate_index] = add_intervals(
          results[candidate_index],
          multiply_interval(
              divide_positive_intervals(
                  numerator, denominator_by_order.at(order)),
              context.alpha_by_order.at(order)));
    }
  }
  return results;
}

using FormalNumerator =
    std::map<std::uint32_t, std::map<std::string, std::uint64_t>>;

[[nodiscard]] FormalNumerator formal_numerator(
    const Candidate& candidate,
    const std::vector<FaceData>& faces,
    const SimplexPointWeighting weighting) {
  FormalNumerator result;
  for (const std::size_t face_index : candidate.face_indices) {
    const FaceData& face = faces[face_index];
    if (weighting == SimplexPointWeighting::uniform) {
      ++result[face.order]["uniform"];
      continue;
    }
    for (const ExactLevel& level : face.atom_levels) {
      ++result[face.order][level.canonical_key()];
    }
  }
  return result;
}

[[nodiscard]] bool tie_precedes(const Candidate& left, const Candidate& right) {
  if (left.stay != right.stay) {
    return left.stay;
  }
  return left.destination < right.destination;
}

[[nodiscard]] std::size_t select_candidate_exact(
    const std::vector<Candidate>& candidates,
    const PointWeightContext& context,
    const std::vector<FaceData>& faces,
    const PointHierarchyOptions& options,
    const PositiveRationalExponent exponent) {
  if (candidates.empty()) {
    throw std::logic_error("an exact candidate selection cannot be empty");
  }
  if (candidates.size() == 1U) {
    return 0U;
  }
  std::vector<FormalNumerator> formal_scores;
  formal_scores.reserve(candidates.size());
  for (const Candidate& candidate : candidates) {
    formal_scores.push_back(
        formal_numerator(candidate, faces, options.simplex_point_weighting));
  }

  std::uint32_t precision = options.budget.initial_certification_bits;
  for (;;) {
    std::vector<RationalInterval> scores;
    try {
      scores = candidate_score_intervals(
          candidates, context, faces, options, exponent, precision);
    } catch (const std::domain_error&) {
      if (precision >= options.budget.maximum_certification_bits) {
        break;
      }
      precision = std::min(
          options.budget.maximum_certification_bits,
          static_cast<std::uint32_t>(precision * 2U));
      continue;
    }
    for (std::size_t candidate = 0U; candidate < candidates.size(); ++candidate) {
      bool dominates = true;
      for (std::size_t other = 0U; other < candidates.size(); ++other) {
        if (candidate == other) {
          continue;
        }
        const bool formally_equal = formal_scores[candidate] == formal_scores[other];
        const bool point_exact_equal =
            scores[candidate].lower == scores[candidate].upper &&
            scores[other].lower == scores[other].upper &&
            scores[candidate].lower == scores[other].lower;
        if (formally_equal || point_exact_equal) {
          if (tie_precedes(candidates[other], candidates[candidate])) {
            dominates = false;
            break;
          }
          continue;
        }
        if (!(scores[candidate].lower > scores[other].upper)) {
          dominates = false;
          break;
        }
      }
      if (dominates) {
        return candidate;
      }
    }
    if (precision >= options.budget.maximum_certification_bits) {
      break;
    }
    precision = std::min(
        options.budget.maximum_certification_bits,
        static_cast<std::uint32_t>(precision * 2U));
  }
  throw ExactCertificationError(
      "a simplex-to-point weight comparison remained uncertified");
}

struct RoutedPoints {
  std::vector<PointHierarchyNodeId> terminal_by_point;
  std::vector<std::optional<DensityLevel>> terminal_density_by_point;
};

[[nodiscard]] RoutedPoints route_points(
    const CertifiedTowerInput& input,
    const PointHierarchyOptions& options,
    const ValidatedInput& validated,
    const TopologyResult& topology) {
  std::vector<FaceData> faces;
  faces.reserve(input.projectable_simplices.size());
  std::vector<std::uint64_t> face_offsets_by_point(
      checked_size(input.point_count, "point_count") + 1U, 0U);
  for (const CertifiedProjectableSimplex& simplex : input.projectable_simplices) {
    const std::size_t source_index =
        validated.source_node_index.at(simplex.carrier_source_node_id);
    faces.push_back(FaceData{
        simplex.source_simplex_id,
        simplex.order,
        topology.carrier_by_source_index[source_index],
        simplex.incident_coface_squared_radii,
        simplex.terminal_exit_level});
    for (const PointId point : simplex.point_ids) {
      ++face_offsets_by_point[checked_size(point, "point id") + 1U];
    }
  }
  for (std::size_t point = 0U;
       point + 1U < face_offsets_by_point.size();
       ++point) {
    face_offsets_by_point[point + 1U] += face_offsets_by_point[point];
  }
  std::vector<std::size_t> face_indices_by_point(
      checked_size(face_offsets_by_point.back(), "point-face incidence count"));
  std::vector<std::uint64_t> face_cursors = face_offsets_by_point;
  for (std::size_t face_index = 0U;
       face_index < input.projectable_simplices.size();
       ++face_index) {
    for (const PointId point :
         input.projectable_simplices[face_index].point_ids) {
      const std::size_t point_index = checked_size(point, "point id");
      face_indices_by_point[checked_size(
          face_cursors[point_index]++, "point-face incidence")] = face_index;
    }
  }

  RoutedPoints routed;
  routed.terminal_by_point.assign(
      checked_size(input.point_count, "point_count"), topology.virtual_root);
  routed.terminal_density_by_point.resize(
      checked_size(input.point_count, "point_count"));

  for (PointId point = 0U; point < input.point_count; ++point) {
    const std::size_t point_index = checked_size(point, "point id");
    const std::uint64_t incident_begin = face_offsets_by_point[point_index];
    const std::uint64_t incident_end = face_offsets_by_point[point_index + 1U];
    if (incident_begin == incident_end) {
      continue;
    }
    const std::span<const std::size_t> incident_faces{
        face_indices_by_point.data() +
            checked_size(incident_begin, "point-face begin"),
        checked_size(incident_end - incident_begin, "point-face count")};

    PointWeightContext context;
    for (const std::size_t face_index : incident_faces) {
      context.faces_by_order[faces[face_index].order].push_back(face_index);
    }
    ExactRational active_order_weight_sum;
    for (const auto& [order, unused] : context.faces_by_order) {
      static_cast<void>(unused);
      active_order_weight_sum =
          active_order_weight_sum + validated.order_weights[order];
    }
    for (const auto& [order, unused] : context.faces_by_order) {
      static_cast<void>(unused);
      context.alpha_by_order.emplace(
          order, validated.order_weights[order] / active_order_weight_sum);
    }

    PointHierarchyNodeId current = topology.virtual_root;
    std::vector<std::size_t> current_faces(
        incident_faces.begin(), incident_faces.end());
    for (;;) {
      std::map<PointHierarchyNodeId, std::vector<std::size_t>> child_faces;
      std::vector<std::size_t> stay_faces;
      for (const std::size_t face_index : current_faces) {
        const PointHierarchyNodeId carrier = faces[face_index].carrier;
        if (carrier == current) {
          stay_faces.push_back(face_index);
        } else {
          child_faces[immediate_child_for_carrier(topology, current, carrier)]
              .push_back(face_index);
        }
      }

      std::vector<Candidate> candidates;
      candidates.reserve(child_faces.size() + (stay_faces.empty() ? 0U : 1U));
      if (!stay_faces.empty()) {
        candidates.push_back(Candidate{true, current, std::move(stay_faces)});
      }
      for (auto& [child, indices] : child_faces) {
        candidates.push_back(Candidate{false, child, std::move(indices)});
      }
      if (candidates.empty()) {
        throw std::logic_error("point routing produced no candidate");
      }

      const std::size_t best = select_candidate_exact(
          candidates, context, faces, options, validated.exponent);
      Candidate selected = std::move(candidates[best]);
      if (selected.stay) {
        routed.terminal_by_point[checked_size(point, "point id")] = current;
        DensityLevel terminal_density =
            faces[selected.face_indices.front()].terminal_exit_level;
        for (const std::size_t face_index : selected.face_indices) {
          const DensityLevel& candidate_density =
              faces[face_index].terminal_exit_level;
          const int comparison = compare_density_impl(
              candidate_density, terminal_density, validated.exponent);
          if (comparison > 0 ||
              (comparison == 0 &&
               (candidate_density.order < terminal_density.order ||
                (candidate_density.order == terminal_density.order &&
                 candidate_density.squared_radius <
                     terminal_density.squared_radius)))) {
            terminal_density = candidate_density;
          }
        }
        const PointHierarchyNode& terminal_node =
            topology.nodes[checked_size(current, "terminal node")];
        if (terminal_node.event_level.has_value() &&
            compare_density_impl(
                terminal_density,
                *terminal_node.event_level,
                validated.exponent) < 0) {
          throw ExactCertificationError(
              "a point terminal density precedes its carrier event");
        }
        routed.terminal_density_by_point[checked_size(point, "point id")] =
            terminal_density;
        break;
      }
      current = selected.destination;
      current_faces = std::move(selected.face_indices);
    }
  }
  return routed;
}

using DensityExpression = std::map<std::string, std::pair<ExactLevel, BigInt>>;

void add_density_term(
    DensityExpression& expression,
    const DensityLevel& density,
    BigInt coefficient) {
  // All zero-radius levels belong to the same exact top batch.  Represent
  // that batch by one formal symbol Lambda_infinity, consistently with
  // compare_density_impl(), instead of attempting to distinguish k / 0.
  // Finite levels retain their actual k / r^exp_z coefficient.
  if (density.squared_radius.numerator() != 0) {
    coefficient *= density.order;
  }
  if (coefficient == 0) {
    return;
  }
  const std::string key = density.squared_radius.canonical_key();
  auto [iterator, inserted] = expression.emplace(
      key, std::make_pair(density.squared_radius, BigInt{0}));
  static_cast<void>(inserted);
  iterator->second.second += coefficient;
  if (iterator->second.second == 0) {
    expression.erase(iterator);
  }
}

[[nodiscard]] DensityExpression add_expressions(
    DensityExpression left, const DensityExpression& right) {
  for (const auto& [key, term] : right) {
    auto [iterator, inserted] =
        left.emplace(key, std::make_pair(term.first, BigInt{0}));
    static_cast<void>(inserted);
    iterator->second.second += term.second;
    if (iterator->second.second == 0) {
      left.erase(iterator);
    }
  }
  return left;
}

[[nodiscard]] DensityExpression consume_expressions(
    DensityExpression left, DensityExpression& right) {
  left.merge(right);
  for (const auto& [key, term] : right) {
    auto iterator = left.find(key);
    if (iterator == left.end()) {
      throw std::logic_error("a consumed density term was not transferred");
    }
    iterator->second.second += term.second;
    if (iterator->second.second == 0) {
      left.erase(iterator);
    }
  }
  right.clear();
  return left;
}

[[nodiscard]] DensityExpression subtract_expressions(
    DensityExpression left, const DensityExpression& right) {
  for (const auto& [key, term] : right) {
    auto [iterator, inserted] =
        left.emplace(key, std::make_pair(term.first, BigInt{0}));
    static_cast<void>(inserted);
    iterator->second.second -= term.second;
    if (iterator->second.second == 0) {
      left.erase(iterator);
    }
  }
  return left;
}

[[nodiscard]] int sign_density_expression_exact(
    const DensityExpression& expression,
    const PositiveRationalExponent exponent,
    const PointHierarchyBudget& budget) {
  if (expression.empty()) {
    return 0;
  }
  for (const auto& [key, term] : expression) {
    static_cast<void>(key);
    if (term.first.numerator() == 0 && term.second != 0) {
      // DensityExpression is ordered lexicographically in the formal common
      // symbol Lambda_infinity and its finite remainder.  Equal zero-level
      // mass cancels exactly; otherwise the leading integer coefficient alone
      // determines the sign.  No finite surrogate or epsilon is introduced.
      return term.second > 0 ? 1 : -1;
    }
  }

  std::uint32_t precision = budget.initial_certification_bits;
  for (;;) {
    ExactRational lower;
    ExactRational upper;
    for (const auto& [key, term] : expression) {
      static_cast<void>(key);
      const RationalInterval atom =
          inverse_radius_power_interval(term.first, exponent, precision);
      if (term.second > 0) {
        lower = lower + atom.lower * ExactRational{term.second};
        upper = upper + atom.upper * ExactRational{term.second};
      } else {
        lower = lower + atom.upper * ExactRational{term.second};
        upper = upper + atom.lower * ExactRational{term.second};
      }
    }
    if (lower.sign() > 0) {
      return 1;
    }
    if (upper.sign() < 0) {
      return -1;
    }
    if (lower.is_zero() && upper.is_zero()) {
      return 0;
    }
    if (precision >= budget.maximum_certification_bits) {
      break;
    }
    precision = std::min(
        budget.maximum_certification_bits,
        static_cast<std::uint32_t>(precision * 2U));
  }
  throw ExactCertificationError("an EOM stability comparison remained uncertified");
}

[[nodiscard]] contract::CanonicalId make_reduction_receipt(
    const CertifiedTowerInput& input,
    const PointHierarchyOptions& options,
    const PositiveRationalExponent exponent,
    const std::vector<ExactRational>& order_weights,
    const TopologyResult& topology,
    const RoutedPoints& routed) {
  contract::CanonicalSha256Builder builder;
  builder.update("MorseHGP3D/v1/exact-multi-order-point-hierarchy/");
  builder.update(input.receipts.all_orders_forest_receipt_id.to_lower_hex());
  builder.update(input.receipts.vertical_maps_receipt_id.to_lower_hex());
  builder.update(input.receipts.projectable_incidences_receipt_id.to_lower_hex());
  builder.update("/tower_payload=");
  builder.update(input.receipts.tower_payload_id.to_lower_hex());
  builder.update("/n=");
  builder.update(std::to_string(input.point_count));
  builder.update("/K=");
  builder.update(std::to_string(input.maximum_order));
  builder.update("/exp=");
  builder.update(std::to_string(exponent.numerator));
  builder.update("/");
  builder.update(std::to_string(exponent.denominator));
  builder.update("/weighting=");
  builder.update(options.simplex_point_weighting == SimplexPointWeighting::inverse_radius
                     ? "inverse_radius"
                     : "uniform");
  builder.update("/certification_bits=");
  builder.update(std::to_string(options.budget.initial_certification_bits));
  builder.update(":");
  builder.update(std::to_string(options.budget.maximum_certification_bits));
  for (std::uint32_t order = 1U; order <= input.maximum_order; ++order) {
    builder.update("/eta_");
    builder.update(std::to_string(order));
    builder.update("=");
    builder.update(order_weights[order].canonical_key());
  }
  for (const PointHierarchyNode& node : topology.nodes) {
    builder.update("/node=");
    builder.update(std::to_string(node.node_id));
    builder.update(":");
    if (node.event_level.has_value()) {
      builder.update(std::to_string(node.event_level->order));
      builder.update("@");
      builder.update(node.event_level->squared_radius.canonical_key());
    } else {
      builder.update("virtual");
    }
    for (const PointHierarchyNodeId child : node.child_node_ids) {
      builder.update(",c=");
      builder.update(std::to_string(child));
    }
    for (const SourceNodeId source : node.direct_source_node_ids) {
      builder.update(",s=");
      builder.update(std::to_string(source));
    }
  }
  for (PointId point = 0U; point < input.point_count; ++point) {
    builder.update("/p=");
    builder.update(std::to_string(point));
    builder.update("@");
    builder.update(std::to_string(
        routed.terminal_by_point[checked_size(point, "point id")]));
  }
  return builder.finalize();
}

[[nodiscard]] FlatClustering labels_from_antichain(
    const std::vector<PointHierarchyNode>& nodes,
    const std::vector<PointId>& point_dfs_order,
    const std::uint64_t point_count,
    std::vector<PointHierarchyNodeId> selected,
    const FlatSelectionMethod method,
    const std::uint64_t minimum_cluster_size,
    const contract::CanonicalId& hierarchy_reduction_receipt_id) {
  std::sort(selected.begin(), selected.end(), [&](PointHierarchyNodeId left,
                                                   PointHierarchyNodeId right) {
    const PointHierarchyNode& left_node = nodes[checked_size(left, "selected node")];
    const PointHierarchyNode& right_node = nodes[checked_size(right, "selected node")];
    if (left_node.point_dfs_begin != right_node.point_dfs_begin) {
      return left_node.point_dfs_begin < right_node.point_dfs_begin;
    }
    return left < right;
  });
  FlatClustering result;
  result.method = method;
  result.labels.assign(checked_size(point_count, "point_count"), -1);
  result.selected_node_ids = selected;
  std::uint64_t prior_end = 0U;
  bool first = true;
  for (std::size_t label = 0U; label < selected.size(); ++label) {
    const PointHierarchyNode& node =
        nodes[checked_size(selected[label], "selected node")];
    if (!first && node.point_dfs_begin < prior_end) {
      throw std::logic_error("a flat selection is not an antichain");
    }
    first = false;
    prior_end = node.point_dfs_end;
    for (std::uint64_t position = node.point_dfs_begin;
         position < node.point_dfs_end;
         ++position) {
      const PointId point =
          point_dfs_order[checked_size(position, "point DFS position")];
      std::int64_t& point_label = result.labels[checked_size(point, "point id")];
      if (point_label != -1) {
        throw std::logic_error("a point occurs in two selected clusters");
      }
      if (label > static_cast<std::size_t>(
                      std::numeric_limits<std::int64_t>::max())) {
        throw std::length_error("the cluster label does not fit int64");
      }
      point_label = static_cast<std::int64_t>(label);
    }
  }
  result.noise_point_count = static_cast<std::uint64_t>(std::count(
      result.labels.begin(), result.labels.end(), std::int64_t{-1}));
  result.clusters_are_pairwise_disjoint = true;
  result.minimum_cluster_size = minimum_cluster_size;
  result.hierarchy_reduction_receipt_id = hierarchy_reduction_receipt_id;
  return result;
}

void finalize_flat_selection_receipt(FlatClustering& result) {
  contract::CanonicalSha256Builder builder;
  builder.update("MorseHGP3D/v1/exact-flat-point-clustering/");
  builder.update(result.hierarchy_reduction_receipt_id.to_lower_hex());
  builder.update("/method=");
  builder.update(std::to_string(static_cast<std::uint8_t>(result.method)));
  builder.update("/minimum_cluster_size=");
  builder.update(std::to_string(result.minimum_cluster_size));
  if (result.lambda_threshold.has_value()) {
    builder.update("/lambda=");
    builder.update(std::to_string(result.lambda_threshold->order));
    builder.update("@");
    builder.update(result.lambda_threshold->squared_radius.canonical_key());
  }
  if (result.dbscan_epsilon_squared.has_value()) {
    builder.update("/epsilon_squared=");
    builder.update(result.dbscan_epsilon_squared->canonical_key());
    builder.update("/reference_order=");
    builder.update(std::to_string(result.dbscan_reference_order));
  }
  if (result.method == FlatSelectionMethod::excess_of_mass) {
    builder.update(result.eom_allow_single_cluster
                       ? "/allow_single_cluster=true"
                       : "/allow_single_cluster=false");
  }
  for (const PointHierarchyNodeId node : result.selected_node_ids) {
    builder.update("/selected=");
    builder.update(std::to_string(node));
  }
  result.selection_receipt_id = builder.finalize();
}

}  // namespace

struct PointHierarchy::Implementation {
  std::uint64_t point_count{0U};
  std::uint32_t maximum_order{0U};
  PointHierarchyNodeId virtual_root{0U};
  PositiveRationalExponent exponent{};
  PointHierarchyBudget budget{};
  std::vector<PointHierarchyNode> nodes;
  std::vector<PointHierarchyNodeId> terminal_by_point;
  std::vector<std::optional<DensityLevel>> terminal_density_by_point;
  std::vector<PointId> point_dfs_order;
  std::vector<DensityExpression> direct_exit_expression_by_node;
  ExactPointHierarchyReceipt receipt;
};

PointHierarchyBudget PointHierarchyBudget::large_resident_30m() noexcept {
  PointHierarchyBudget budget;
  budget.maximum_points = 32'000'000U;
  budget.maximum_source_nodes = 80'000'000U;
  budget.maximum_source_edges = 160'000'000U;
  budget.maximum_projectable_simplices = 80'000'000U;
  budget.maximum_point_simplex_incidences = 640'000'000U;
  budget.maximum_coface_radius_atoms = 640'000'000U;
  budget.maximum_hierarchy_nodes = 240'000'001U;
  return budget;
}

int compare_density_levels_exact(
    const DensityLevel& left,
    const DensityLevel& right,
    PositiveRationalExponent exp_z) {
  return compare_density_impl(left, right, normalize_exponent(exp_z));
}

contract::CanonicalId compute_tower_payload_id(
    const CertifiedTowerInput& input) {
  return compute_tower_payload_id_impl(input);
}

PointHierarchy::PointHierarchy(
    std::shared_ptr<const Implementation> implementation)
    : implementation_(std::move(implementation)) {
  if (!implementation_) {
    throw std::invalid_argument("a PointHierarchy implementation cannot be null");
  }
}

PointHierarchy::PointHierarchy(PointHierarchy&& other) noexcept
    : implementation_(other.implementation_) {}

PointHierarchy& PointHierarchy::operator=(PointHierarchy&& other) noexcept {
  if (this != &other) {
    implementation_ = other.implementation_;
  }
  return *this;
}

std::uint64_t PointHierarchy::point_count() const noexcept {
  return implementation_->point_count;
}

std::uint32_t PointHierarchy::maximum_order() const noexcept {
  return implementation_->maximum_order;
}

PointHierarchyNodeId PointHierarchy::virtual_root_node_id() const noexcept {
  return implementation_->virtual_root;
}

const std::vector<PointHierarchyNode>& PointHierarchy::nodes() const noexcept {
  return implementation_->nodes;
}

const std::vector<PointHierarchyNodeId>&
PointHierarchy::terminal_node_by_point() const noexcept {
  return implementation_->terminal_by_point;
}

const std::vector<PointId>& PointHierarchy::point_ids_in_dfs_order() const noexcept {
  return implementation_->point_dfs_order;
}

const ExactPointHierarchyReceipt& PointHierarchy::receipt() const noexcept {
  return implementation_->receipt;
}

FlatClustering PointHierarchy::select_lambda_cut(
    const DensityLevel& threshold,
    const std::uint64_t minimum_cluster_size) const {
  if (threshold.order == 0U || threshold.order > implementation_->maximum_order) {
    throw std::invalid_argument("the lambda-cut reference order lies outside 1,...,K");
  }
  if (minimum_cluster_size == 0U) {
    throw std::invalid_argument("minimum_cluster_size must be strictly positive");
  }
  std::vector<PointHierarchyNodeId> selected;
  for (const PointHierarchyNode& node : implementation_->nodes) {
    if (!node.event_level.has_value() ||
        node.subtree_point_count < minimum_cluster_size ||
        compare_density_impl(
            *node.event_level, threshold, implementation_->exponent) < 0) {
      continue;
    }
    const PointHierarchyNodeId parent = *node.parent_node_id;
    const PointHierarchyNode& parent_node =
        implementation_->nodes[checked_size(parent, "parent node")];
    if (!parent_node.event_level.has_value() ||
        compare_density_impl(
            *parent_node.event_level, threshold, implementation_->exponent) < 0) {
      selected.push_back(node.node_id);
    }
  }
  FlatClustering result = labels_from_antichain(
      implementation_->nodes,
      implementation_->point_dfs_order,
      implementation_->point_count,
      std::move(selected),
      FlatSelectionMethod::lambda_cut,
      minimum_cluster_size,
      implementation_->receipt.reduction_receipt_id);
  result.lambda_threshold = threshold;
  finalize_flat_selection_receipt(result);
  return result;
}

FlatClustering PointHierarchy::select_dbscan_radius(
    const ExactLevel& epsilon_squared,
    std::uint32_t reference_order,
    const std::uint64_t minimum_cluster_size) const {
  if (epsilon_squared.numerator() == 0) {
    throw std::invalid_argument("a DBSCAN radius must be strictly positive");
  }
  if (reference_order == 0U) {
    reference_order = implementation_->maximum_order;
  }
  FlatClustering result =
      select_lambda_cut(DensityLevel{reference_order, epsilon_squared},
                        minimum_cluster_size);
  result.method = FlatSelectionMethod::dbscan_radius;
  result.lambda_threshold = DensityLevel{reference_order, epsilon_squared};
  result.dbscan_epsilon_squared = epsilon_squared;
  result.dbscan_reference_order = reference_order;
  finalize_flat_selection_receipt(result);
  return result;
}

FlatClustering PointHierarchy::select_excess_of_mass(
    const std::uint64_t minimum_cluster_size,
    const bool allow_single_cluster) const {
  if (minimum_cluster_size == 0U) {
    throw std::invalid_argument("minimum_cluster_size must be strictly positive");
  }
  struct CondensedCluster {
    PointHierarchyNodeId raw_start{0U};
    std::optional<DensityLevel> birth_level;
    DensityExpression stability;
    std::vector<std::size_t> child_cluster_indices;
  };
  std::vector<CondensedCluster> condensed;
  std::vector<std::size_t> condensed_roots;
  const auto append_cluster = [&](PointHierarchyNodeId raw_start,
                                  std::optional<DensityLevel> birth_level) {
    const std::size_t index = condensed.size();
    condensed.push_back(
        CondensedCluster{raw_start, std::move(birth_level), {}, {}});
    return index;
  };
  const auto& raw_roots = implementation_->nodes[checked_size(
      implementation_->virtual_root, "virtual root")]
                              .child_node_ids;
  for (const PointHierarchyNodeId root : raw_roots) {
    if (implementation_->nodes[checked_size(root, "natural root")]
            .subtree_point_count >= minimum_cluster_size) {
      condensed_roots.push_back(append_cluster(root, std::nullopt));
    }
  }

  for (std::size_t cluster_index = 0U;
       cluster_index < condensed.size();
       ++cluster_index) {
    const PointHierarchyNodeId raw_start = condensed[cluster_index].raw_start;
    DensityExpression stability;
    if (condensed[cluster_index].birth_level.has_value()) {
      add_density_term(
          stability,
          *condensed[cluster_index].birth_level,
          -BigInt{implementation_->nodes[checked_size(
              raw_start, "condensed start")]
                      .subtree_point_count});
    }

    PointHierarchyNodeId current = raw_start;
    std::vector<PointHierarchyNodeId> split_children;
    for (;;) {
      const std::size_t current_index = checked_size(current, "condensed raw node");
      const PointHierarchyNode& raw_node = implementation_->nodes[current_index];
      if (!raw_node.event_level.has_value()) {
        throw std::logic_error("a condensed cluster reached the virtual root");
      }
      stability = add_expressions(
          std::move(stability),
          implementation_->direct_exit_expression_by_node[current_index]);

      std::vector<PointHierarchyNodeId> eligible_children;
      for (const PointHierarchyNodeId child : raw_node.child_node_ids) {
        if (implementation_->nodes[checked_size(child, "raw child")]
                .subtree_point_count >= minimum_cluster_size) {
          eligible_children.push_back(child);
        }
      }
      if (eligible_children.size() == 1U) {
        for (const PointHierarchyNodeId child : raw_node.child_node_ids) {
          if (child != eligible_children.front()) {
            add_density_term(
                stability,
                *raw_node.event_level,
                BigInt{implementation_->nodes[checked_size(child, "small child")]
                           .subtree_point_count});
          }
        }
        current = eligible_children.front();
        continue;
      }

      for (const PointHierarchyNodeId child : raw_node.child_node_ids) {
        add_density_term(
            stability,
            *raw_node.event_level,
            BigInt{implementation_->nodes[checked_size(child, "raw child")]
                       .subtree_point_count});
      }
      if (eligible_children.size() >= 2U) {
        split_children = std::move(eligible_children);
      }
      break;
    }

    if (sign_density_expression_exact(
            stability,
            implementation_->exponent,
            implementation_->budget) < 0) {
      throw ExactCertificationError("a condensed-cluster stability is negative");
    }
    condensed[cluster_index].stability = std::move(stability);
    if (!split_children.empty()) {
      const DensityLevel birth_level = *implementation_->nodes[checked_size(
          current, "true split node")]
                                             .event_level;
      for (const PointHierarchyNodeId child : split_children) {
        // append_cluster may reallocate `condensed`; do not keep a reference
        // to its parent element across that call.
        const std::size_t child_cluster = append_cluster(child, birth_level);
        condensed[cluster_index].child_cluster_indices.push_back(child_cluster);
      }
    }
  }

  std::vector<bool> choose_self(condensed.size(), false);
  std::vector<std::unique_ptr<DensityExpression>> best_stability(
      condensed.size());
  for (std::size_t reverse = condensed.size(); reverse > 0U; --reverse) {
    const std::size_t cluster_index = reverse - 1U;
    CondensedCluster& cluster = condensed[cluster_index];
    if (cluster.child_cluster_indices.empty()) {
      choose_self[cluster_index] = true;
      best_stability[cluster_index] =
          std::make_unique<DensityExpression>(std::move(cluster.stability));
      continue;
    }
    DensityExpression descendants;
    for (const std::size_t child : cluster.child_cluster_indices) {
      if (!best_stability[child]) {
        throw std::logic_error("a condensed child has no EOM optimum");
      }
      descendants = consume_expressions(
          std::move(descendants), *best_stability[child]);
      best_stability[child].reset();
    }
    const int comparison = sign_density_expression_exact(
        subtract_expressions(cluster.stability, descendants),
        implementation_->exponent,
        implementation_->budget);
    if (comparison >= 0) {
      choose_self[cluster_index] = true;
      best_stability[cluster_index] =
          std::make_unique<DensityExpression>(std::move(cluster.stability));
    } else {
      best_stability[cluster_index] =
          std::make_unique<DensityExpression>(std::move(descendants));
    }
  }

  std::vector<PointHierarchyNodeId> selected;
  std::vector<std::size_t> pending(condensed_roots.rbegin(), condensed_roots.rend());
  while (!pending.empty()) {
    const std::size_t cluster_index = pending.back();
    pending.pop_back();
    const bool forbidden_sole_root =
        !allow_single_cluster && condensed_roots.size() == 1U &&
        cluster_index == condensed_roots.front();
    if (choose_self[cluster_index] && !forbidden_sole_root) {
      selected.push_back(condensed[cluster_index].raw_start);
      continue;
    }
    const auto& children = condensed[cluster_index].child_cluster_indices;
    pending.insert(pending.end(), children.rbegin(), children.rend());
  }
  FlatClustering result = labels_from_antichain(
      implementation_->nodes,
      implementation_->point_dfs_order,
      implementation_->point_count,
      std::move(selected),
      FlatSelectionMethod::excess_of_mass,
      minimum_cluster_size,
      implementation_->receipt.reduction_receipt_id);
  result.eom_allow_single_cluster = allow_single_cluster;
  finalize_flat_selection_receipt(result);
  return result;
}

bool PointHierarchy::validate_partition_invariants() const {
  const std::vector<PointHierarchyNode>& hierarchy_nodes = implementation_->nodes;
  if (hierarchy_nodes.empty() ||
      implementation_->virtual_root >= hierarchy_nodes.size() ||
      implementation_->terminal_by_point.size() != implementation_->point_count ||
      implementation_->point_dfs_order.size() != implementation_->point_count) {
    return false;
  }
  std::vector<std::uint64_t> direct_counts(hierarchy_nodes.size(), 0U);
  for (const PointHierarchyNodeId terminal : implementation_->terminal_by_point) {
    if (terminal >= hierarchy_nodes.size()) {
      return false;
    }
    ++direct_counts[checked_size(terminal, "terminal")];
  }
  std::vector<bool> seen(checked_size(implementation_->point_count, "point_count"), false);
  for (const PointId point : implementation_->point_dfs_order) {
    if (point >= implementation_->point_count ||
        seen[checked_size(point, "point id")]) {
      return false;
    }
    seen[checked_size(point, "point id")] = true;
  }
  for (std::size_t index = 0U; index < hierarchy_nodes.size(); ++index) {
    const PointHierarchyNode& node = hierarchy_nodes[index];
    if (node.node_id != index || node.direct_point_count != direct_counts[index] ||
        node.point_dfs_begin > node.point_dfs_end ||
        node.point_dfs_end > implementation_->point_count ||
        node.point_dfs_end - node.point_dfs_begin != node.subtree_point_count) {
      return false;
    }
    std::uint64_t expected = node.direct_point_count;
    std::uint64_t cursor = node.point_dfs_begin + node.direct_point_count;
    for (const PointHierarchyNodeId child_id : node.child_node_ids) {
      if (child_id >= hierarchy_nodes.size()) {
        return false;
      }
      const PointHierarchyNode& child = hierarchy_nodes[checked_size(child_id, "child")];
      if (!child.parent_node_id.has_value() || *child.parent_node_id != node.node_id ||
          child.point_dfs_begin != cursor) {
        return false;
      }
      expected += child.subtree_point_count;
      cursor = child.point_dfs_end;
    }
    if (expected != node.subtree_point_count || cursor != node.point_dfs_end) {
      return false;
    }
  }
  return hierarchy_nodes[checked_size(
             implementation_->virtual_root, "virtual root")]
             .subtree_point_count == implementation_->point_count;
}

PointHierarchy build_exact_point_hierarchy(
    const CertifiedTowerInput& input, const PointHierarchyOptions& options) {
  const ValidatedInput validated = validate_input(input, options);
  TopologyResult topology = build_tower_topology(input, validated);
  if (topology.nodes.size() > options.budget.maximum_hierarchy_nodes) {
    throw std::length_error("the hierarchy-node build budget is exceeded");
  }
  RoutedPoints routed = route_points(input, options, validated, topology);

  std::vector<std::uint64_t> direct_point_offsets(
      topology.nodes.size() + 1U, 0U);
  for (const PointHierarchyNodeId terminal : routed.terminal_by_point) {
    ++direct_point_offsets[checked_size(terminal, "terminal node") + 1U];
  }
  for (std::size_t node = 0U; node < topology.nodes.size(); ++node) {
    direct_point_offsets[node + 1U] += direct_point_offsets[node];
    topology.nodes[node].direct_point_count =
        direct_point_offsets[node + 1U] - direct_point_offsets[node];
  }
  std::vector<PointId> direct_points(
      checked_size(input.point_count, "point_count"));
  std::vector<std::uint64_t> direct_point_cursors = direct_point_offsets;
  for (PointId point = 0U; point < input.point_count; ++point) {
    const PointHierarchyNodeId terminal =
        routed.terminal_by_point[checked_size(point, "point id")];
    direct_points[checked_size(
        direct_point_cursors[checked_size(terminal, "terminal node")]++,
        "direct point position")] = point;
  }
  for (std::size_t node = 0U; node < topology.nodes.size(); ++node) {
    PointHierarchyNode& hierarchy_node = topology.nodes[node];
    hierarchy_node.subtree_point_count = hierarchy_node.direct_point_count;
    for (const PointHierarchyNodeId child : hierarchy_node.child_node_ids) {
      hierarchy_node.subtree_point_count +=
          topology.nodes[checked_size(child, "child node")].subtree_point_count;
    }
  }

  std::vector<PointId> point_dfs_order;
  point_dfs_order.reserve(checked_size(input.point_count, "point_count"));
  struct PointDfsFrame {
    PointHierarchyNodeId node_id{0U};
    bool exit{false};
  };
  std::vector<PointDfsFrame> point_stack;
  point_stack.push_back(PointDfsFrame{topology.virtual_root, false});
  while (!point_stack.empty()) {
    const PointDfsFrame frame = point_stack.back();
    point_stack.pop_back();
    PointHierarchyNode& node =
        topology.nodes[checked_size(frame.node_id, "hierarchy node")];
    if (frame.exit) {
      node.point_dfs_end = point_dfs_order.size();
      continue;
    }
    node.point_dfs_begin = point_dfs_order.size();
    const std::size_t node_index =
        checked_size(frame.node_id, "hierarchy node");
    const PointId* const direct_begin =
        direct_points.data() + checked_size(
                                   direct_point_offsets[node_index],
                                   "direct point begin");
    const PointId* const direct_end =
        direct_points.data() + checked_size(
                                   direct_point_offsets[node_index + 1U],
                                   "direct point end");
    point_dfs_order.insert(
        point_dfs_order.end(), direct_begin, direct_end);
    point_stack.push_back(PointDfsFrame{frame.node_id, true});
    for (auto child = node.child_node_ids.rbegin();
         child != node.child_node_ids.rend();
         ++child) {
      point_stack.push_back(PointDfsFrame{*child, false});
    }
  }

  auto implementation = std::make_shared<PointHierarchy::Implementation>();
  const contract::CanonicalId reduction_receipt_id =
      make_reduction_receipt(
          input,
          options,
          validated.exponent,
          validated.order_weights,
          topology,
          routed);
  implementation->point_count = input.point_count;
  implementation->maximum_order = input.maximum_order;
  implementation->virtual_root = topology.virtual_root;
  implementation->exponent = validated.exponent;
  implementation->budget = options.budget;
  implementation->nodes = topology.nodes;
  implementation->terminal_by_point = std::move(routed.terminal_by_point);
  implementation->terminal_density_by_point =
      std::move(routed.terminal_density_by_point);
  implementation->point_dfs_order = std::move(point_dfs_order);
  implementation->direct_exit_expression_by_node.resize(
      implementation->nodes.size());
  for (PointId point = 0U; point < input.point_count; ++point) {
    const std::size_t point_index = checked_size(point, "point id");
    if (!implementation->terminal_density_by_point[point_index].has_value()) {
      continue;
    }
    add_density_term(
        implementation->direct_exit_expression_by_node[checked_size(
            implementation->terminal_by_point[point_index], "terminal node")],
        *implementation->terminal_density_by_point[point_index],
        BigInt{1});
  }
  std::vector<OrderWeight> normalized_order_weights;
  normalized_order_weights.reserve(input.maximum_order);
  for (std::uint32_t order = 1U; order <= input.maximum_order; ++order) {
    normalized_order_weights.push_back(
        OrderWeight{order, validated.order_weights[order]});
  }
  implementation->receipt = ExactPointHierarchyReceipt{
      input.receipts.all_orders_forest_receipt_id,
      input.receipts.vertical_maps_receipt_id,
      input.receipts.projectable_incidences_receipt_id,
      input.receipts.tower_payload_id,
      reduction_receipt_id,
      validated.exponent,
      options.simplex_point_weighting,
      std::move(normalized_order_weights),
      options.budget.initial_certification_bits,
      options.budget.maximum_certification_bits,
      true,
      false,
      false,
      true,
      false};

  PointHierarchy result{std::move(implementation)};
  if (!result.validate_partition_invariants()) {
    throw std::logic_error("the constructed point hierarchy violates its partition invariant");
  }
  return result;
}

}  // namespace morsehgp3d::api
