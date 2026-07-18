#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/spatial/brute_force.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"
#include "morsehgp3d/spatial/query.hpp"

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
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ClosedBallPartition;
using morsehgp3d::spatial::ExactNeighbor;
using morsehgp3d::spatial::ExclusionSet;
using morsehgp3d::spatial::PointId;
using morsehgp3d::spatial::TopKPartition;

constexpr std::string_view protocol_header = "morsehgp3d-spatial-query-v1";

template <typename Integer>
[[nodiscard]] Integer parse_unsigned_decimal(
    std::string_view text, std::string_view field_name) {
  static_assert(std::numeric_limits<Integer>::is_integer);
  static_assert(!std::numeric_limits<Integer>::is_signed);
  if (text.empty() || (text.size() > 1U && text.front() == '0')) {
    throw std::invalid_argument(
        std::string{field_name} + " must be a canonical unsigned decimal integer");
  }
  Integer value{};
  const char* const begin = text.data();
  const char* const end = begin + text.size();
  const auto [position, error] = std::from_chars(begin, end, value, 10);
  if (error != std::errc{} || position != end) {
    throw std::invalid_argument(
        std::string{field_name} + " must be a canonical unsigned decimal integer");
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
  return parse_unsigned_decimal<std::size_t>(next_token(input, field_name), field_name);
}

[[nodiscard]] PointId next_point_id(
    std::istringstream& input, std::string_view field_name) {
  return parse_unsigned_decimal<PointId>(next_token(input, field_name), field_name);
}

void require_end_of_case(std::istringstream& input) {
  std::string extra;
  if (input >> extra) {
    throw std::invalid_argument("unexpected token after the point payload: " + extra);
  }
}

void write_id_array(std::ostream& output, std::span<const PointId> ids) {
  output << '[';
  for (std::size_t index = 0; index < ids.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    output << ids[index];
  }
  output << ']';
}

void write_level(std::ostream& output, const ExactLevel& level) {
  output << "{\"denominator\":\"" << level.denominator_string()
         << "\",\"numerator\":\"" << level.numerator_string() << "\"}";
}

void write_query(std::ostream& output, const ExactRational3& query) {
  const auto record = query.to_record();
  output << "{\"denominator\":\"" << record.denominator
         << "\",\"x_numerator\":\"" << record.x_numerator
         << "\",\"y_numerator\":\"" << record.y_numerator
         << "\",\"z_numerator\":\"" << record.z_numerator << "\"}";
}

void write_canonical_points(
    std::ostream& output, const CanonicalPointCloud& cloud) {
  output << '[';
  for (std::size_t index = 0; index < cloud.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    const PointId id = static_cast<PointId>(index);
    const std::array<std::uint64_t, 3> bits = cloud.point(id).canonical_input_bits();
    output << "{\"id\":" << id << ",\"input_bits\":[\""
           << morsehgp3d::exact::binary64_hex(bits[0]) << "\",\""
           << morsehgp3d::exact::binary64_hex(bits[1]) << "\",\""
           << morsehgp3d::exact::binary64_hex(bits[2])
           << "\"],\"source_index\":" << cloud.source_index(id) << '}';
  }
  output << ']';
}

void write_closed_ball(
    std::ostream& output, const ClosedBallPartition& partition) {
  output << "{\"closed_rank\":" << partition.closed_rank()
         << ",\"evaluation_count\":" << partition.evaluation_count()
         << ",\"exterior_ids\":";
  write_id_array(output, partition.exterior_ids());
  output << ",\"interior_ids\":";
  write_id_array(output, partition.interior_ids());
  output << ",\"partition_complete\":"
         << (partition.partition_complete() ? "true" : "false")
         << ",\"shell_ids\":";
  write_id_array(output, partition.shell_ids());
  output << ",\"squared_radius\":";
  write_level(output, partition.squared_radius());
  output << '}';
}

void write_strict_neighbors(
    std::ostream& output, std::span<const ExactNeighbor> neighbors) {
  output << '[';
  for (std::size_t index = 0; index < neighbors.size(); ++index) {
    if (index != 0U) {
      output << ',';
    }
    output << "{\"point_id\":" << neighbors[index].point_id
           << ",\"squared_distance\":";
    write_level(output, neighbors[index].squared_distance);
    output << '}';
  }
  output << ']';
}

void write_top_k(std::ostream& output, const TopKPartition& partition) {
  output << "{\"canonical_choice_ids\":";
  write_id_array(output, partition.canonical_choice_ids());
  output << ",\"cutoff_shell_ids\":";
  write_id_array(output, partition.cutoff_shell_ids());
  output << ",\"cutoff_squared_distance\":";
  write_level(output, partition.cutoff_squared_distance());
  output << ",\"distance_evaluation_count\":"
         << partition.distance_evaluation_count()
         << ",\"eligible_point_count\":" << partition.eligible_point_count()
         << ",\"requested_rank\":" << partition.requested_rank()
         << ",\"shell_complete\":"
         << (partition.shell_complete() ? "true" : "false")
         << ",\"strict_below\":";
  write_strict_neighbors(output, partition.strict_below());
  output << '}';
}

void process_case(std::string_view line) {
  std::istringstream input{std::string{line}};
  if (next_token(input, "case marker") != "case") {
    throw std::invalid_argument("a protocol payload line must start with 'case'");
  }

  const std::uint64_t case_id = next_point_id(input, "case_id");
  const std::size_t point_count = next_size(input, "point_count");
  const std::size_t requested_rank = next_size(input, "requested_rank");
  const std::size_t run_m_star = next_size(input, "run_m_star");
  const std::size_t exclusion_count = next_size(input, "exclusion_count");
  const BigInt query_x = morsehgp3d::exact::parse_canonical_integer(
      next_token(input, "query_x_numerator"));
  const BigInt query_y = morsehgp3d::exact::parse_canonical_integer(
      next_token(input, "query_y_numerator"));
  const BigInt query_z = morsehgp3d::exact::parse_canonical_integer(
      next_token(input, "query_z_numerator"));
  const BigInt query_denominator =
      morsehgp3d::exact::parse_canonical_positive_integer(
          next_token(input, "query_denominator"));

  std::vector<PointId> excluded_ids;
  excluded_ids.reserve(exclusion_count);
  for (std::size_t index = 0; index < exclusion_count; ++index) {
    excluded_ids.push_back(next_point_id(input, "excluded_point_id"));
  }

  std::vector<CertifiedPoint3> source_points;
  source_points.reserve(point_count);
  for (std::size_t index = 0; index < point_count; ++index) {
    std::array<std::uint64_t, 3> bits{};
    for (std::size_t axis = 0; axis < bits.size(); ++axis) {
      bits[axis] = morsehgp3d::exact::parse_binary64_hex(
          next_token(input, "point_binary64_word"), false);
    }
    source_points.push_back(CertifiedPoint3::from_binary64_bits(bits));
  }
  require_end_of_case(input);

  const CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(source_points);
  const ExclusionSet exclusions = ExclusionSet::from_ids(
      excluded_ids, cloud, run_m_star);
  const ExactRational3 query{
      query_x, query_y, query_z, query_denominator};
  const TopKPartition top_k = morsehgp3d::spatial::brute_force_top_k(
      cloud, query, requested_rank, exclusions);
  const ClosedBallPartition closed_ball =
      morsehgp3d::spatial::brute_force_closed_ball(
          cloud, query, top_k.cutoff_squared_distance());

  std::cout << "{\"canonical_points\":";
  write_canonical_points(std::cout, cloud);
  std::cout << ",\"case_id\":" << case_id << ",\"closed_ball\":";
  write_closed_ball(std::cout, closed_ball);
  std::cout << ",\"exclusions\":{\"ids\":";
  write_id_array(std::cout, exclusions.ids());
  std::cout << ",\"point_count\":" << exclusions.point_count()
            << ",\"run_m_star\":" << exclusions.run_m_star()
            << "},\"query\":";
  write_query(std::cout, query);
  std::cout << ",\"schema\":\"morsehgp3d.spatial_query_dump.v1\",\"top_k\":";
  write_top_k(std::cout, top_k);
  std::cout << "}\n";
}

}  // namespace

int main() {
  try {
    std::string line;
    if (!std::getline(std::cin, line) || line != protocol_header) {
      throw std::invalid_argument("missing spatial-query protocol header");
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
      throw std::invalid_argument("missing spatial-query protocol terminator");
    }
    while (std::getline(std::cin, line)) {
      if (!line.empty()) {
        throw std::invalid_argument("data after the protocol terminator is forbidden");
      }
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "spatial_query_dump: " << error.what() << '\n';
    return 2;
  }
}
