#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/integer.hpp"
#include "morsehgp3d/exact/predicate.hpp"
#include "morsehgp3d/exact/rational.hpp"
#include "morsehgp3d/gpu/predicate_filter.hpp"

#include <array>
#include <bit>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

namespace {

using morsehgp3d::exact::binary64_hex;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::exact::greatest_common_divisor;
using morsehgp3d::exact::parse_binary64_hex;
using morsehgp3d::exact::parse_canonical_integer;
using morsehgp3d::exact::parse_canonical_positive_integer;
using morsehgp3d::exact::to_string;
using morsehgp3d::gpu::FilterSign;
using morsehgp3d::gpu::Orientation3DBatchOptions;
using morsehgp3d::gpu::Orientation3DBatchResult;
using morsehgp3d::gpu::Orientation3DFilterInput;
using morsehgp3d::gpu::PowerBisectorBatchOptions;
using morsehgp3d::gpu::PowerBisectorBatchResult;
using morsehgp3d::gpu::PowerBisectorFilterInput;
using morsehgp3d::gpu::PowerBisectorLabelPoint;
using morsehgp3d::gpu::maximum_power_bisector_cardinality;
using morsehgp3d::gpu::SquaredDistanceBatchOptions;
using morsehgp3d::gpu::SquaredDistanceBatchResult;
using morsehgp3d::gpu::SquaredDistanceFilterInput;

constexpr std::size_t kMaximumBatchSize = std::size_t{1} << 20U;

enum class ReplayPredicate {
  squared_distance,
  orientation_3d,
  power_bisector_side,
};

using ReplayInput =
    std::variant<
        SquaredDistanceFilterInput,
        Orientation3DFilterInput,
        PowerBisectorFilterInput>;

struct ReplayRecord {
  ReplayPredicate predicate{ReplayPredicate::squared_distance};
  ReplayInput input{SquaredDistanceFilterInput{}};
  std::string replay_command;
};

[[nodiscard]] std::string encode_json_string(std::string_view value) {
  static constexpr char kHex[] = "0123456789abcdef";
  std::string encoded;
  encoded.reserve(value.size() + 2U);
  encoded.push_back('"');
  for (const char raw_character : value) {
    const auto character = static_cast<unsigned char>(raw_character);
    switch (character) {
      case '"':
        encoded += "\\\"";
        break;
      case '\\':
        encoded += "\\\\";
        break;
      case '\b':
        encoded += "\\b";
        break;
      case '\f':
        encoded += "\\f";
        break;
      case '\n':
        encoded += "\\n";
        break;
      case '\r':
        encoded += "\\r";
        break;
      case '\t':
        encoded += "\\t";
        break;
      default:
        if (character < 0x20U || character >= 0x7fU) {
          encoded += "\\u00";
          encoded.push_back(kHex[(character >> 4U) & 0x0fU]);
          encoded.push_back(kHex[character & 0x0fU]);
        } else {
          encoded.push_back(static_cast<char>(character));
        }
        break;
    }
  }
  encoded.push_back('"');
  return encoded;
}

[[nodiscard]] std::uint64_t parse_replay_id(std::string_view text) {
  if (text.empty() || text.front() == '+' ||
      (text.size() > 1U && text.front() == '0')) {
    throw std::invalid_argument("a replay identifier must be canonical decimal");
  }
  std::uint64_t value = 0U;
  const auto conversion =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (conversion.ec != std::errc{} ||
      conversion.ptr != text.data() + text.size()) {
    throw std::invalid_argument("a replay identifier must be uint64 decimal");
  }
  return value;
}

[[nodiscard]] std::size_t parse_size(
    std::string_view text, std::string_view label) {
  if (text.empty() || text.front() == '+' || text.front() == '-' ||
      (text.size() > 1U && text.front() == '0')) {
    throw std::invalid_argument(
        std::string{label} + " must be canonical nonnegative decimal");
  }
  std::size_t value = 0U;
  const auto conversion =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (conversion.ec != std::errc{} ||
      conversion.ptr != text.data() + text.size()) {
    throw std::invalid_argument(
        std::string{label} + " must fit the platform size domain");
  }
  return value;
}

[[nodiscard]] std::uint32_t parse_point_id(
    std::string_view text, std::size_t point_count) {
  const std::size_t value = parse_size(text, "a power-bisector point identifier");
  if (value >= point_count ||
      value > std::numeric_limits<std::uint32_t>::max()) {
    throw std::invalid_argument(
        "a power-bisector point identifier is outside the point table");
  }
  return static_cast<std::uint32_t>(value);
}

[[nodiscard]] std::uint64_t exact_integer_binary64_bits(
    const BigInt& value, std::string_view label) {
  const double candidate = value.convert_to<double>();
  if (!std::isfinite(candidate)) {
    throw std::invalid_argument(
        std::string{label} + " is not a finite binary64 integer");
  }
  const std::uint64_t bits = std::bit_cast<std::uint64_t>(candidate);
  if (ExactRational::from_binary64_bits(bits) != ExactRational{value}) {
    throw std::invalid_argument(
        std::string{label} + " is not exactly representable as binary64");
  }
  return bits;
}

[[nodiscard]] std::vector<std::string> split_tokens(const std::string& line) {
  std::istringstream input{line};
  std::vector<std::string> tokens;
  std::string token;
  while (input >> token) {
    tokens.push_back(std::move(token));
  }
  return tokens;
}

[[nodiscard]] ReplayRecord parse_record(
    const std::string& line, std::size_t line_number) {
  const std::vector<std::string> tokens = split_tokens(line);
  if (tokens.size() >= 2U && tokens[1] == "compare_squared_distances") {
    if (tokens.size() != 11U) {
      throw std::invalid_argument(
          "Phase 2B input line " + std::to_string(line_number) +
          " must be: REPLAY_ID compare_squared_distances followed by 9 binary64 words");
    }
    SquaredDistanceFilterInput input;
    input.replay_id = parse_replay_id(tokens[0]);
    std::array<std::uint64_t, 9> words{};
    for (std::size_t index = 0U; index < words.size(); ++index) {
      words[index] = parse_binary64_hex(tokens[index + 2U], false);
    }
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      input.witness_bits[axis] = words[axis];
      input.left_bits[axis] = words[axis + 3U];
      input.right_bits[axis] = words[axis + 6U];
    }
    ReplayRecord record{
        ReplayPredicate::squared_distance,
        input,
        "compare_squared_distances"};
    for (const std::uint64_t word : words) {
      record.replay_command += " " + binary64_hex(word);
    }
    return record;
  }
  if (tokens.size() >= 2U && tokens[1] == "orientation_3d") {
    if (tokens.size() != 14U) {
      throw std::invalid_argument(
          "Phase 2B input line " + std::to_string(line_number) +
          " must be: REPLAY_ID orientation_3d followed by 12 binary64 words");
    }
    Orientation3DFilterInput input;
    input.replay_id = parse_replay_id(tokens[0]);
    std::array<std::uint64_t, 12> words{};
    for (std::size_t index = 0U; index < words.size(); ++index) {
      words[index] = parse_binary64_hex(tokens[index + 2U], false);
    }
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      input.a_bits[axis] = words[axis];
      input.b_bits[axis] = words[axis + 3U];
      input.c_bits[axis] = words[axis + 6U];
      input.d_bits[axis] = words[axis + 9U];
    }
    ReplayRecord record{
        ReplayPredicate::orientation_3d, input, "orientation_3d"};
    for (const std::uint64_t word : words) {
      record.replay_command += " " + binary64_hex(word);
    }
    return record;
  }
  if (tokens.size() >= 2U && tokens[1] == "power_bisector_side") {
    if (tokens.size() < 11U) {
      throw std::invalid_argument(
          "Phase 2B input line " + std::to_string(line_number) +
          " contains an incomplete power-bisector replay");
    }
    PowerBisectorFilterInput input;
    input.replay_id = parse_replay_id(tokens[0]);
    std::array<BigInt, 3> witness_numerators{};
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      witness_numerators[axis] = parse_canonical_integer(tokens[2U + axis]);
      input.witness_numerator_bits[axis] = exact_integer_binary64_bits(
          witness_numerators[axis],
          "a power-bisector witness numerator");
    }
    const BigInt witness_denominator =
        parse_canonical_positive_integer(tokens[5U]);
    input.witness_denominator_bits = exact_integer_binary64_bits(
        witness_denominator,
        "the power-bisector witness denominator");
    BigInt common_divisor = witness_denominator;
    for (const BigInt& numerator : witness_numerators) {
      common_divisor = greatest_common_divisor(
          std::move(common_divisor), numerator);
    }
    if (common_divisor != 1) {
      throw std::invalid_argument(
          "the power-bisector homogeneous witness must be reduced canonically");
    }

    std::size_t cursor = 6U;
    const std::size_t point_count =
        parse_size(tokens[cursor], "the power-bisector point table size");
    ++cursor;
    constexpr std::size_t kMaximumPointTableSize =
        2U * maximum_power_bisector_cardinality;
    if (point_count == 0U || point_count > kMaximumPointTableSize ||
        point_count > (tokens.size() - cursor) / 3U) {
      throw std::invalid_argument(
          "a GPU power-bisector point table must contain between one and twenty complete points");
    }
    std::vector<std::array<std::uint64_t, 3>> point_table(point_count);
    for (std::size_t point = 0U; point < point_count; ++point) {
      for (std::size_t axis = 0U; axis < 3U; ++axis) {
        point_table[point][axis] =
            parse_binary64_hex(tokens[cursor], false);
        ++cursor;
      }
    }

    if (cursor >= tokens.size()) {
      throw std::invalid_argument("the power-bisector R label is missing");
    }
    const std::size_t r_count =
        parse_size(tokens[cursor], "the power-bisector R label size");
    ++cursor;
    if (r_count == 0U ||
        r_count > maximum_power_bisector_cardinality ||
        r_count > tokens.size() - cursor) {
      throw std::invalid_argument(
          "the power-bisector R label must contain between one and ten identifiers");
    }
    for (std::size_t index = 0U; index < r_count; ++index) {
      const std::uint32_t point_id =
          parse_point_id(tokens[cursor], point_count);
      ++cursor;
      input.r_points[index] =
          PowerBisectorLabelPoint{point_id, point_table[point_id]};
    }

    if (cursor >= tokens.size()) {
      throw std::invalid_argument("the power-bisector Q label is missing");
    }
    const std::size_t q_count =
        parse_size(tokens[cursor], "the power-bisector Q label size");
    ++cursor;
    if (q_count != r_count || q_count != tokens.size() - cursor) {
      throw std::invalid_argument(
          "power-bisector labels must have equal declared and serialized sizes");
    }
    for (std::size_t index = 0U; index < q_count; ++index) {
      const std::uint32_t point_id =
          parse_point_id(tokens[cursor], point_count);
      ++cursor;
      input.q_points[index] =
          PowerBisectorLabelPoint{point_id, point_table[point_id]};
    }
    input.cardinality = static_cast<std::uint32_t>(r_count);

    ReplayRecord record{
        ReplayPredicate::power_bisector_side,
        input,
        "power_bisector_side"};
    for (std::size_t index = 2U; index < tokens.size(); ++index) {
      record.replay_command += " " + tokens[index];
    }
    return record;
  }
  throw std::invalid_argument(
      "Phase 2B input line " + std::to_string(line_number) +
      " must name compare_squared_distances, orientation_3d or power_bisector_side");
}

[[nodiscard]] const char* filter_sign_name(FilterSign sign) {
  switch (sign) {
    case FilterSign::negative:
      return "negative";
    case FilterSign::unknown:
      return "unknown";
    case FilterSign::positive:
      return "positive";
  }
  throw std::invalid_argument("the GPU filter sign is invalid");
}

void write_decision(
    const ReplayRecord& record,
    const morsehgp3d::gpu::SquaredDistanceDecision& decision) {
  std::cout << "{\"certification_stage\":"
            << encode_json_string(to_string(decision.certification_stage))
            << ",\"gpu_filter_sign\":"
            << encode_json_string(filter_sign_name(decision.gpu_filter_sign))
            << ",\"kind\":\"decision\",\"predicate\":\"compare_squared_distances\""
            << ",\"replay_command\":"
            << encode_json_string(record.replay_command)
            << ",\"replay_id\":" << decision.replay_id
            << ",\"sign\":" << encode_json_string(to_string(decision.sign))
            << "}\n";
}

void write_decision(
    const ReplayRecord& record,
    const morsehgp3d::gpu::Orientation3DDecision& decision) {
  std::cout << "{\"certification_stage\":"
            << encode_json_string(to_string(decision.certification_stage))
            << ",\"gpu_filter_sign\":"
            << encode_json_string(filter_sign_name(decision.gpu_filter_sign))
            << ",\"kind\":\"decision\",\"predicate\":\"orientation_3d\""
            << ",\"replay_command\":"
            << encode_json_string(record.replay_command)
            << ",\"replay_id\":" << decision.replay_id
            << ",\"sign\":" << encode_json_string(to_string(decision.sign))
            << "}\n";
}

void write_decision(
    const ReplayRecord& record,
    const morsehgp3d::gpu::PowerBisectorDecision& decision) {
  std::cout << "{\"certification_stage\":"
            << encode_json_string(to_string(decision.certification_stage))
            << ",\"gpu_filter_sign\":"
            << encode_json_string(filter_sign_name(decision.gpu_filter_sign))
            << ",\"kind\":\"decision\",\"predicate\":\"power_bisector_side\""
            << ",\"replay_command\":"
            << encode_json_string(record.replay_command)
            << ",\"replay_id\":" << decision.replay_id
            << ",\"sign\":" << encode_json_string(to_string(decision.sign))
            << "}\n";
}

template <typename BatchResult>
void write_summary(
    const BatchResult& result,
    bool audit_known,
    std::string_view schema) {
  const auto& counters = result.counters;
  std::cout
      << "{\"audit_gpu_signs\":" << (audit_known ? "true" : "false")
      << ",\"counters\":{\"async_fallback_batches\":"
      << counters.async_fallback_batches
      << ",\"cpu_expansion_certified\":" << counters.cpu_expansion_certified
      << ",\"cpu_fp64_filtered_certified\":"
      << counters.cpu_fp64_filtered_certified
      << ",\"cpu_multiprecision_certified\":"
      << counters.cpu_multiprecision_certified
      << ",\"exact_zeros\":" << counters.exact_zeros
      << ",\"gpu_fp64_certified\":" << counters.gpu_fp64_certified
      << ",\"gpu_inputs\":" << counters.gpu_inputs
      << ",\"gpu_known_audited\":" << counters.gpu_known_audited
      << ",\"gpu_unknown_forwarded\":" << counters.gpu_unknown_forwarded
      << ",\"remaining_unknown\":" << counters.remaining_unknown
      << "},\"kind\":\"summary\",\"schema\":"
      << encode_json_string(schema) << "}\n";
}

int run_squared_distance(
    const std::vector<ReplayRecord>& records,
    bool audit_known,
    bool summary_only) {
  std::vector<SquaredDistanceFilterInput> inputs;
  inputs.reserve(records.size());
  for (const ReplayRecord& record : records) {
    inputs.push_back(std::get<SquaredDistanceFilterInput>(record.input));
  }
  const SquaredDistanceBatchResult result =
      morsehgp3d::gpu::decide_squared_distance_batch_async(
          std::move(inputs), SquaredDistanceBatchOptions{audit_known})
          .get();
  if (result.decisions.size() != records.size()) {
    throw std::runtime_error("Phase 2B resolved decision count changed");
  }
  if (!summary_only) {
    for (std::size_t index = 0U; index < records.size(); ++index) {
      write_decision(records[index], result.decisions[index]);
    }
  }
  write_summary(
      result, audit_known, "morsehgp3d.phase2b.distance_filter.v1");
  return 0;
}

int run_orientation_3d(
    const std::vector<ReplayRecord>& records,
    bool audit_known,
    bool summary_only) {
  std::vector<Orientation3DFilterInput> inputs;
  inputs.reserve(records.size());
  for (const ReplayRecord& record : records) {
    inputs.push_back(std::get<Orientation3DFilterInput>(record.input));
  }
  const Orientation3DBatchResult result =
      morsehgp3d::gpu::decide_orientation_3d_batch_async(
          std::move(inputs), Orientation3DBatchOptions{audit_known})
          .get();
  if (result.decisions.size() != records.size()) {
    throw std::runtime_error(
        "Phase 2B orientation resolved decision count changed");
  }
  if (!summary_only) {
    for (std::size_t index = 0U; index < records.size(); ++index) {
      write_decision(records[index], result.decisions[index]);
    }
  }
  write_summary(
      result,
      audit_known,
      "morsehgp3d.phase2b.orientation_3d_filter.v1");
  return 0;
}

int run_power_bisector_side(
    const std::vector<ReplayRecord>& records,
    bool audit_known,
    bool summary_only) {
  std::vector<PowerBisectorFilterInput> inputs;
  inputs.reserve(records.size());
  for (const ReplayRecord& record : records) {
    inputs.push_back(std::get<PowerBisectorFilterInput>(record.input));
  }
  const PowerBisectorBatchResult result =
      morsehgp3d::gpu::decide_power_bisector_batch_async(
          std::move(inputs), PowerBisectorBatchOptions{audit_known})
          .get();
  if (result.decisions.size() != records.size()) {
    throw std::runtime_error(
        "Phase 2B power-bisector resolved decision count changed");
  }
  if (!summary_only) {
    for (std::size_t index = 0U; index < records.size(); ++index) {
      write_decision(records[index], result.decisions[index]);
    }
  }
  write_summary(
      result,
      audit_known,
      "morsehgp3d.phase2b.power_bisector_side_filter.v1");
  return 0;
}

int run(bool audit_known, bool summary_only) {
  std::vector<ReplayRecord> records;
  std::string line;
  std::size_t line_number = 0U;
  while (std::getline(std::cin, line)) {
    ++line_number;
    if (line.empty()) {
      throw std::invalid_argument(
          "Phase 2B input line " + std::to_string(line_number) + " is empty");
    }
    if (records.size() == kMaximumBatchSize) {
      throw std::length_error("Phase 2B batch exceeds 1048576 records");
    }
    records.push_back(parse_record(line, line_number));
  }
  if (!std::cin.eof()) {
    throw std::runtime_error("Phase 2B batch input could not be read completely");
  }

  const ReplayPredicate predicate =
      records.empty() ? ReplayPredicate::squared_distance
                      : records.front().predicate;
  for (const ReplayRecord& record : records) {
    if (record.predicate != predicate) {
      throw std::invalid_argument(
          "a Phase 2B replay batch must contain a single predicate");
    }
  }
  int status = 0;
  switch (predicate) {
    case ReplayPredicate::squared_distance:
      status = run_squared_distance(records, audit_known, summary_only);
      break;
    case ReplayPredicate::orientation_3d:
      status = run_orientation_3d(records, audit_known, summary_only);
      break;
    case ReplayPredicate::power_bisector_side:
      status = run_power_bisector_side(records, audit_known, summary_only);
      break;
  }
  if (!std::cout) {
    throw std::runtime_error("Phase 2B JSONL output could not be written");
  }
  return status;
}

void usage(const char* executable) {
  std::cerr
      << "usage: " << executable << " [--audit-known] [--summary-only]\n"
      << "stdin: a homogeneous batch of either\n"
      << "  REPLAY_ID compare_squared_distances HEX_X HEX_Y HEX_Z ... (3 points)\n"
      << "  REPLAY_ID orientation_3d HEX_X HEX_Y HEX_Z ... (4 points)\n"
      << "  REPLAY_ID power_bisector_side XN YN ZN D POINT_COUNT ... R_COUNT ... Q_COUNT ...\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    bool audit_known = false;
    bool summary_only = false;
    for (int index = 1; index < argc; ++index) {
      const std::string_view argument{argv[index]};
      if (argument == "--audit-known") {
        audit_known = true;
      } else if (argument == "--summary-only") {
        summary_only = true;
      } else if (argument == "--help" || argument == "-h") {
        usage(argv[0]);
        return 0;
      } else {
        throw std::invalid_argument("unsupported Phase 2B runner option");
      }
    }
    return run(audit_known, summary_only);
  } catch (const std::exception& error) {
    std::cerr << "Phase 2B GPU predicate replay failed: " << error.what() << '\n';
    return 1;
  }
}
