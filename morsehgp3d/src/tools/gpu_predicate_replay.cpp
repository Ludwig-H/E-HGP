#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/predicate.hpp"
#include "morsehgp3d/gpu/predicate_filter.hpp"

#include <array>
#include <charconv>
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
using morsehgp3d::exact::parse_binary64_hex;
using morsehgp3d::exact::to_string;
using morsehgp3d::gpu::FilterSign;
using morsehgp3d::gpu::Orientation3DBatchOptions;
using morsehgp3d::gpu::Orientation3DBatchResult;
using morsehgp3d::gpu::Orientation3DFilterInput;
using morsehgp3d::gpu::SquaredDistanceBatchOptions;
using morsehgp3d::gpu::SquaredDistanceBatchResult;
using morsehgp3d::gpu::SquaredDistanceFilterInput;

constexpr std::size_t kMaximumBatchSize = std::size_t{1} << 20U;

enum class ReplayPredicate {
  squared_distance,
  orientation_3d,
};

using ReplayInput =
    std::variant<SquaredDistanceFilterInput, Orientation3DFilterInput>;

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
  throw std::invalid_argument(
      "Phase 2B input line " + std::to_string(line_number) +
      " must name compare_squared_distances or orientation_3d");
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
  const int status =
      predicate == ReplayPredicate::squared_distance
          ? run_squared_distance(records, audit_known, summary_only)
          : run_orientation_3d(records, audit_known, summary_only);
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
      << "  REPLAY_ID orientation_3d HEX_X HEX_Y HEX_Z ... (4 points)\n";
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
