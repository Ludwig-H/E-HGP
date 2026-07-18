#include "morsehgp3d/gpu/spatial_reference.hpp"

#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/point.hpp"

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iomanip>
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

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::gpu::QueryCoordinateProjection;
using morsehgp3d::gpu::SpatialReferenceAudit;
using morsehgp3d::gpu::SpatialReferenceContext;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ClosedBallPartition;
using morsehgp3d::spatial::ExactNeighbor;
using morsehgp3d::spatial::ExclusionSet;
using morsehgp3d::spatial::PointId;
using morsehgp3d::spatial::TopKPartition;

constexpr std::string_view kProtocolHeader =
    "morsehgp3d-spatial-gpu-reference-v1";

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

[[nodiscard]] std::uint64_t next_uint64(
    std::istringstream& input, std::string_view field_name) {
  return parse_unsigned_decimal<std::uint64_t>(
      next_token(input, field_name), field_name);
}

[[nodiscard]] PointId next_point_id(
    std::istringstream& input, std::string_view field_name) {
  return parse_unsigned_decimal<PointId>(
      next_token(input, field_name), field_name);
}

void require_end_of_case(std::istringstream& input) {
  std::string extra;
  if (input >> extra) {
    throw std::invalid_argument(
        "unexpected token after the point payload: " + extra);
  }
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

void write_level(std::ostream& output, const ExactLevel& level) {
  output << "{\"denominator\":\"" << level.denominator_string()
         << "\",\"numerator\":\"" << level.numerator_string() << "\"}";
}

void write_strict_neighbors(
    std::ostream& output, std::span<const ExactNeighbor> neighbors) {
  output << '[';
  for (std::size_t index = 0U; index < neighbors.size(); ++index) {
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
  output << ",\"eligible_point_count\":" << partition.eligible_point_count()
         << ",\"requested_rank\":" << partition.requested_rank()
         << ",\"shell_complete\":"
         << (partition.shell_complete() ? "true" : "false")
         << ",\"strict_below\":";
  write_strict_neighbors(output, partition.strict_below());
  output << '}';
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

void write_projection(
    std::ostream& output, QueryCoordinateProjection projection) {
  switch (projection) {
    case QueryCoordinateProjection::exact:
      output << "\"exact\"";
      return;
    case QueryCoordinateProjection::rounded:
      output << "\"rounded\"";
      return;
    case QueryCoordinateProjection::underflow:
      output << "\"underflow\"";
      return;
    case QueryCoordinateProjection::overflow_clamped:
      output << "\"overflow_clamped\"";
      return;
  }
  throw std::logic_error("unknown query-coordinate projection status");
}

void write_hex_word(std::ostream& output, std::uint64_t bits) {
  const auto flags = output.flags();
  const char fill = output.fill();
  output << '"' << std::hex << std::nouppercase << std::setfill('0')
         << std::setw(16) << bits << '"';
  output.flags(flags);
  output.fill(fill);
}

void write_audit(std::ostream& output, const SpatialReferenceAudit& audit) {
  output << "{\"all_points_enumerated\":"
         << (audit.all_points_enumerated ? "true" : "false")
         << ",\"buffer_epoch\":" << audit.buffer_epoch
         << ",\"cpu_exact_distance_evaluation_count\":"
         << audit.cpu_exact_distance_evaluation_count
         << ",\"cpu_exact_recertification_complete\":"
         << (audit.cpu_exact_recertification_complete ? "true" : "false")
         << ",\"decision_semantics\":\""
         << SpatialReferenceAudit::decision_semantics
         << "\",\"gpu_finite_distance_proposal_count\":"
         << audit.gpu_finite_distance_proposal_count
         << ",\"gpu_infinite_distance_proposal_count\":"
         << audit.gpu_infinite_distance_proposal_count
         << ",\"gpu_input_point_count\":" << audit.gpu_input_point_count
         << ",\"gpu_launch_count\":" << audit.gpu_launch_count
         << ",\"gpu_nan_distance_proposal_count\":"
         << audit.gpu_nan_distance_proposal_count
         << ",\"gpu_output_record_count\":"
         << audit.gpu_output_record_count
         << ",\"gpu_unique_point_id_count\":"
         << audit.gpu_unique_point_id_count
         << ",\"projected_query_bits\":[";
  for (std::size_t axis = 0U; axis < audit.projected_query_bits.size(); ++axis) {
    if (axis != 0U) {
      output << ',';
    }
    write_hex_word(output, audit.projected_query_bits[axis]);
  }
  output << "],\"proposal_digest_fnv1a\":";
  write_hex_word(output, audit.proposal_digest_fnv1a);
  output << ",\"proposal_semantics\":\""
         << SpatialReferenceAudit::proposal_semantics
         << "\",\"query_projection\":[";
  for (std::size_t axis = 0U; axis < audit.query_projection.size(); ++axis) {
    if (axis != 0U) {
      output << ',';
    }
    write_projection(output, audit.query_projection[axis]);
  }
  output << "]}";
}

void process_case(std::string_view line) {
  std::istringstream input{std::string{line}};
  if (next_token(input, "case marker") != "case") {
    throw std::invalid_argument("a protocol payload line must start with 'case'");
  }
  const std::uint64_t case_id = next_uint64(input, "case_id");
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
  for (std::size_t index = 0U; index < exclusion_count; ++index) {
    excluded_ids.push_back(next_point_id(input, "excluded_point_id"));
  }
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
  const ExclusionSet exclusions = ExclusionSet::from_ids(
      excluded_ids, cloud, run_m_star);
  const ExactRational3 query{
      query_x, query_y, query_z, query_denominator};
  SpatialReferenceContext context{cloud};
  const auto top = context.top_k(
      cloud, query, requested_rank, exclusions);
  const auto ball = context.closed_ball(
      cloud, query, top.exact_partition.cutoff_squared_distance());

  std::cout << "{\"case_id\":" << case_id << ",\"closed_ball\":";
  write_closed_ball(std::cout, ball.exact_partition);
  std::cout << ",\"closed_ball_audit\":";
  write_audit(std::cout, ball.audit);
  std::cout << ",\"schema\":\"morsehgp3d.spatial_gpu_reference.v1\""
            << ",\"top_k\":";
  write_top_k(std::cout, top.exact_partition);
  std::cout << ",\"top_k_audit\":";
  write_audit(std::cout, top.audit);
  std::cout << "}\n";
}

}  // namespace

int main() {
  try {
    std::string line;
    if (!std::getline(std::cin, line) || line != kProtocolHeader) {
      throw std::invalid_argument("missing spatial-GPU protocol header");
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
      throw std::invalid_argument("missing spatial-GPU protocol terminator");
    }
    while (std::getline(std::cin, line)) {
      if (!line.empty()) {
        throw std::invalid_argument(
            "data after the protocol terminator is forbidden");
      }
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "gpu_spatial_reference_replay: " << error.what() << '\n';
    return 2;
  }
}
