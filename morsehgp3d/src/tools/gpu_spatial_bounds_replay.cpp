#include "morsehgp3d/gpu/spatial_bounds.hpp"

#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/exact/level.hpp"
#include "morsehgp3d/exact/point.hpp"

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
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::gpu::DirectedEnclosureStatus;
using morsehgp3d::gpu::SpatialBoundsAudit;
using morsehgp3d::gpu::SpatialBoundsContext;
using morsehgp3d::gpu::SpatialBoundsDecision;
using morsehgp3d::spatial::ExactDyadicAabb3;

constexpr std::string_view kProtocolHeader =
    "morsehgp3d-spatial-gpu-bounds-v1";
constexpr std::size_t kMaximumCaseCount = 256U;
constexpr std::size_t kMaximumBoxCount = 4096U;
constexpr std::size_t kMaximumLineBytes = 1U << 20U;

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

void require_end_of_case(std::istringstream& input) {
  std::string extra;
  if (input >> extra) {
    throw std::invalid_argument(
        "unexpected token after the AABB payload: " + extra);
  }
}

void write_hex_word(std::ostream& output, std::uint64_t bits) {
  const auto flags = output.flags();
  const char fill = output.fill();
  output << '"' << std::hex << std::nouppercase << std::setfill('0')
         << std::setw(16) << bits << '"';
  output.flags(flags);
  output.fill(fill);
}

void write_level(std::ostream& output, const ExactLevel& level) {
  output << "{\"denominator\":\"" << level.denominator_string()
         << "\",\"numerator\":\"" << level.numerator_string() << "\"}";
}

void write_enclosure_status(
    std::ostream& output, DirectedEnclosureStatus status) {
  switch (status) {
    case DirectedEnclosureStatus::exact:
      output << "\"exact\"";
      return;
    case DirectedEnclosureStatus::enclosed:
      output << "\"enclosed\"";
      return;
    case DirectedEnclosureStatus::unsupported_range:
      output << "\"unsupported_range\"";
      return;
  }
  throw std::logic_error("unknown directed-enclosure status");
}

void write_decision(std::ostream& output, SpatialBoundsDecision decision) {
  switch (decision) {
    case SpatialBoundsDecision::prune:
      output << "\"prune\"";
      return;
    case SpatialBoundsDecision::visit:
      output << "\"visit\"";
      return;
    case SpatialBoundsDecision::unknown:
      output << "\"unknown\"";
      return;
  }
  throw std::logic_error("unknown spatial-bounds decision");
}

void write_enclosure_array(
    std::ostream& output,
    const std::array<DirectedEnclosureStatus, 3>& statuses) {
  output << '[';
  for (std::size_t axis = 0U; axis < statuses.size(); ++axis) {
    if (axis != 0U) {
      output << ',';
    }
    write_enclosure_status(output, statuses[axis]);
  }
  output << ']';
}

void write_word_array(
    std::ostream& output, const std::array<std::uint64_t, 3>& words) {
  output << '[';
  for (std::size_t axis = 0U; axis < words.size(); ++axis) {
    if (axis != 0U) {
      output << ',';
    }
    write_hex_word(output, words[axis]);
  }
  output << ']';
}

void write_audit(std::ostream& output, const SpatialBoundsAudit& audit) {
  output << "{\"all_boxes_classified\":"
         << (audit.all_boxes_classified ? "true" : "false")
         << ",\"buffer_epoch\":" << audit.buffer_epoch
         << ",\"certified_prune_count\":" << audit.certified_prune_count
         << ",\"cpu_exact_prune_recertification_count\":"
         << audit.cpu_exact_prune_recertification_count
         << ",\"cpu_exact_recertification_complete\":"
         << (audit.cpu_exact_recertification_complete ? "true" : "false")
         << ",\"cutoff_enclosure\":";
  write_enclosure_status(output, audit.cutoff_enclosure);
  output << ",\"cutoff_lower_bits\":";
  write_hex_word(output, audit.cutoff_lower_bits);
  output << ",\"cutoff_upper_bits\":";
  write_hex_word(output, audit.cutoff_upper_bits);
  output << ",\"decision_semantics\":\""
         << SpatialBoundsAudit::decision_semantics
         << "\",\"gpu_input_box_count\":" << audit.gpu_input_box_count
         << ",\"gpu_launch_count\":" << audit.gpu_launch_count
         << ",\"gpu_output_record_count\":"
         << audit.gpu_output_record_count
         << ",\"gpu_prune_proposal_count\":"
         << audit.gpu_prune_proposal_count
         << ",\"gpu_unique_box_index_count\":"
         << audit.gpu_unique_box_index_count
         << ",\"gpu_unknown_proposal_count\":"
         << audit.gpu_unknown_proposal_count
         << ",\"gpu_visit_proposal_count\":"
         << audit.gpu_visit_proposal_count
         << ",\"minimum_certified_strict_margin\":";
  if (audit.minimum_certified_strict_margin.has_value()) {
    write_level(output, *audit.minimum_certified_strict_margin);
  } else {
    output << "null";
  }
  output << ",\"proposal_digest_fnv1a\":";
  write_hex_word(output, audit.proposal_digest_fnv1a);
  output << ",\"proposal_permutation_complete\":"
         << (audit.proposal_permutation_complete ? "true" : "false")
         << ",\"proposal_semantics\":\""
         << SpatialBoundsAudit::proposal_semantics
         << "\",\"query_enclosure\":";
  write_enclosure_array(output, audit.query_enclosure);
  output << ",\"query_lower_bits\":";
  write_word_array(output, audit.query_lower_bits);
  output << ",\"query_upper_bits\":";
  write_word_array(output, audit.query_upper_bits);
  output << ",\"unsupported_range_fallback_count\":"
         << audit.unsupported_range_fallback_count << '}';
}

void process_case(std::string_view line) {
  std::istringstream input{std::string{line}};
  if (next_token(input, "case marker") != "case") {
    throw std::invalid_argument("a protocol payload line must start with 'case'");
  }
  const std::uint64_t case_id = next_uint64(input, "case_id");
  const std::size_t box_count = next_size(input, "box_count");
  if (box_count == 0U || box_count > kMaximumBoxCount) {
    throw std::invalid_argument("box_count is outside the bounded protocol");
  }
  const BigInt query_x = morsehgp3d::exact::parse_canonical_integer(
      next_token(input, "query_x_numerator"));
  const BigInt query_y = morsehgp3d::exact::parse_canonical_integer(
      next_token(input, "query_y_numerator"));
  const BigInt query_z = morsehgp3d::exact::parse_canonical_integer(
      next_token(input, "query_z_numerator"));
  const BigInt query_denominator =
      morsehgp3d::exact::parse_canonical_positive_integer(
          next_token(input, "query_denominator"));
  const BigInt cutoff_numerator =
      morsehgp3d::exact::parse_canonical_nonnegative_integer(
          next_token(input, "cutoff_numerator"));
  const BigInt cutoff_denominator =
      morsehgp3d::exact::parse_canonical_positive_integer(
          next_token(input, "cutoff_denominator"));

  std::vector<ExactDyadicAabb3> boxes;
  boxes.reserve(box_count);
  for (std::size_t box_index = 0U; box_index < box_count; ++box_index) {
    ExactDyadicAabb3 box{};
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      box.lower_binary64_bits[axis] =
          morsehgp3d::exact::parse_binary64_hex(
              next_token(input, "box_lower_binary64_word"), false);
    }
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      box.upper_binary64_bits[axis] =
          morsehgp3d::exact::parse_binary64_hex(
              next_token(input, "box_upper_binary64_word"), false);
    }
    boxes.push_back(box);
  }
  require_end_of_case(input);

  const ExactRational3 query{
      query_x, query_y, query_z, query_denominator};
  const ExactLevel cutoff{cutoff_numerator, cutoff_denominator};
  SpatialBoundsContext context{boxes};
  const auto result = context.classify_strict_prune(query, cutoff);

  std::cout << "{\"audit\":";
  write_audit(std::cout, result.audit);
  std::cout << ",\"case_id\":" << case_id << ",\"decisions\":[";
  for (std::size_t index = 0U; index < result.decisions.size(); ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    write_decision(std::cout, result.decisions[index]);
  }
  std::cout << "],\"schema\":\"morsehgp3d.spatial_gpu_bounds.v1\"}\n";
}

}  // namespace

int main() {
  try {
    std::string line;
    if (!std::getline(std::cin, line) || line != kProtocolHeader) {
      throw std::invalid_argument("missing spatial-GPU-bounds protocol header");
    }
    bool saw_end = false;
    std::size_t case_count = 0U;
    while (std::getline(std::cin, line)) {
      if (line == "end") {
        saw_end = true;
        break;
      }
      if (line.empty()) {
        throw std::invalid_argument("blank protocol lines are forbidden");
      }
      if (line.size() > kMaximumLineBytes) {
        throw std::invalid_argument("a protocol line exceeds the byte bound");
      }
      if (case_count == kMaximumCaseCount) {
        throw std::invalid_argument("the protocol exceeds its case-count bound");
      }
      process_case(line);
      ++case_count;
    }
    if (!saw_end) {
      throw std::invalid_argument("missing spatial-GPU-bounds protocol terminator");
    }
    if (case_count == 0U) {
      throw std::invalid_argument("the spatial-GPU-bounds protocol is empty");
    }
    while (std::getline(std::cin, line)) {
      if (!line.empty()) {
        throw std::invalid_argument(
            "data after the protocol terminator is forbidden");
      }
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "gpu_spatial_bounds_replay: " << error.what() << '\n';
    return 2;
  }
}
