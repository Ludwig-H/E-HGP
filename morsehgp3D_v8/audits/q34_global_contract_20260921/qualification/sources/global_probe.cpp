// Independent audit adapter, not a product gate or geometric oracle.
// Uses only the public product entry points. The external Python judge
// enumerates supports and verifies geometry; the runner pins all sources.
#include "pipeline/wspd_q34.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
using namespace mhgp8;
void require(bool condition, std::string_view message) {
  if (!condition) throw std::runtime_error(std::string(message));
}
u64 number(std::string_view text) {
  u64 value{};
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
  require(!text.empty() && error == std::errc{} && end == text.data() + text.size(),
          "expected an unsigned decimal integer");
  return value;
}
std::string decimal(i128 value) {
  if (value == 0) return "0";
  // Keep the working value nonpositive: this also represents signed MIN.
  const bool negative = value < 0;
  if (!negative) value = -value;
  std::string result;
  while (value != 0) {
    result.push_back(static_cast<char>('0' - static_cast<int>(value % 10)));
    value /= 10;
  }
  if (negative) result.push_back('-');
  std::reverse(result.begin(), result.end());
  return result;
}
void hash_word(u64& hash, u64 value) {
  for (unsigned byte = 0; byte != 8; ++byte) {
    hash ^= value & 255U;
    hash *= 1099511628211ULL;  // FNV-1a, intentional modulo 2^64.
    value >>= 8;
  }
}

// The snapshot layouts contain only these u64 aggregates. Every nested
// inventory is checked: these arrays neither omit fields nor read padding.
#define WORDS(type, count) static_assert(std::is_trivially_copyable_v<type> && \
  std::is_standard_layout_v<type> && sizeof(type) == (count) * sizeof(u64))
static_assert(sizeof(u64) == 8 && std::numeric_limits<u64>::digits == 64);
WORDS(Q34EdgeCoverWork, 10); WORDS(WspdQ3Work, 23);
WORDS(Q4PositiveDomainWork, 12); WORDS(Q4LocalGeometryQueryWork, 2);
WORDS(Q4LocalGeometryWork, 27); WORDS(Q4LocalPartitionWork, 20);
WORDS(Q4LocalAtlasWork, 38); WORDS(Q4LocalSweepWork, 41);
WORDS(Q4LocalEdgeWork, 117); WORDS(Q4ShallowSetWork, 25);
WORDS(Q4FamilyWork, 13); WORDS(Q4ShallowSweepWork, 32);
WORDS(Q4WindowSelectionWork, 25); WORDS(Q4WindowSweepWork, 57);
WORDS(Q4WindowEdgeWork, 116); WORDS(WspdQ34Work, 279);
WORDS(WspdFrontWork, 49); WORDS(WspdQ34ParallelWork, 11);
WORDS(WspdQ34WorkerWork, 7);
#undef WORDS
template<std::size_t Count, class T> auto words(const T& value) {
  static_assert(sizeof(T) == Count * sizeof(u64));
  return std::bit_cast<std::array<u64, Count>>(value);
}
template<class Range> void dump_numbers(const Range& values) {
  std::cout << '[';
  bool first = true;
  for (const auto value : values) {
    if (!first) std::cout << ',';
    first = false;
    std::cout << value;
  }
  std::cout << ']';
}

struct Record {
  unsigned arity{};
  std::vector<std::size_t> support;
  std::array<i128, 5> coefficients{};
  std::size_t depth{};
  std::vector<std::size_t> shell;
  bool operator<(const Record& other) const {
    return std::tie(arity, support, coefficients, depth, shell) <
           std::tie(other.arity, other.support, other.coefficients, other.depth, other.shell);
  }
};
Record copy(const Q34SeedCandidate& candidate, std::size_t n, unsigned k, unsigned mask) {
  require(candidate.arity == 3 || candidate.arity == 4, "invalid candidate arity");
  require(k + 2 > candidate.arity && candidate.depth < k + 2 - candidate.arity,
          "invalid candidate depth");
  require((mask & (candidate.arity == 3 ? 2U : 4U)) != 0, "inactive lane emitted");
  Record result;
  result.arity = candidate.arity;
  result.depth = candidate.depth;
  result.coefficients = candidate.ball.coefficients();
  require(result.coefficients[0] > 0, "nonpositive leading ball coefficient");
  for (unsigned i = 0; i < candidate.arity; ++i) {
    const auto id = candidate.support_ids[i];
    require(id < n && (i == 0 || candidate.support_ids[i - 1] < id), "invalid support IDs");
    result.support.push_back(id);
  }
  for (const auto part : {candidate.shell_first, candidate.shell_second})
    require(std::is_sorted(part.begin(), part.end()), "unsorted shell part");
  std::merge(candidate.shell_first.begin(), candidate.shell_first.end(),
             candidate.shell_second.begin(), candidate.shell_second.end(),
             std::back_inserter(result.shell));
  for (std::size_t i = 0; i != result.shell.size(); ++i)
    require(result.shell[i] < n && (i == 0 || result.shell[i - 1] < result.shell[i]),
            "duplicate or invalid shell ID");
  for (const auto id : result.support)
    require(std::binary_search(result.shell.begin(), result.shell.end(), id), "support absent from shell");
  return result;
}
void dump_record(const Record& record) {
  std::cout << "{\"arity\":" << record.arity << ",\"support\":";
  dump_numbers(record.support);
  std::cout << ",\"coefficients\":[";
  for (std::size_t i = 0; i != record.coefficients.size(); ++i) {
    if (i != 0) std::cout << ',';
    std::cout << '"' << decimal(record.coefficients[i]) << '"';
  }
  std::cout << "],\"depth\":" << record.depth << ",\"shell\":";
  dump_numbers(record.shell);
  std::cout << '}';
}

int run(int argc, char** argv) {
  require(argc == 8, "usage: global_probe file K s mask backend(28|30) mode(pure|samples) workers(0|1|4)");
  const auto k = number(argv[2]), s = number(argv[3]), mask = number(argv[4]);
  const auto backend = number(argv[5]), workers = number(argv[7]);
  const std::string_view mode = argv[6];
  require(k >= 1 && k <= 10 && s > 0 && s <= std::numeric_limits<unsigned>::max() &&
          (mask == 2 || mask == 4 || mask == 6) && (backend == 28 || backend == 30) &&
          (mode == "pure" || mode == "samples") && (workers == 0 || workers == 1 || workers == 4),
          "invalid probe options");
  std::ifstream input(argv[1]);
  require(input.is_open(), "cannot open input");
  std::string token;
  require(static_cast<bool>(input >> token), "missing point count");
  const auto count = number(token);
  require(count > 0 && count <= std::numeric_limits<std::size_t>::max(), "invalid point count");
  const auto n = static_cast<std::size_t>(count);
  std::vector<Point3> points;
  points.reserve(n);
  u64 input_hash = 14695981039346656037ULL;
  hash_word(input_hash, count);
  for (std::size_t i = 0; i != n; ++i) {
    std::array<std::uint16_t, 3> xyz{};
    for (auto& coordinate : xyz) {
      require(static_cast<bool>(input >> token), "truncated input coordinates");
      const auto value = number(token);
      require(value <= 65535, "coordinate outside u16");
      coordinate = static_cast<std::uint16_t>(value);
      hash_word(input_hash, value);
    }
    points.push_back({xyz[0], xyz[1], xyz[2]});
  }
  require(!(input >> token) && input.eof(), "extra input or input read error");
  const auto cloud = prepare_cloud(points);
  const auto index = make_q2_cloud_index(cloud);
  WspdQ34Options options;
  options.requested_lane_mask = static_cast<std::uint8_t>(mask);
  options.q4_backend = backend == 28 ? WspdQ4Backend::Local28 : WspdQ4Backend::Window30;
  options.front_mode = mode == "pure" ? WspdFrontMode::Pure : WspdFrontMode::MidpointSamples;
  // Keep all Local28 options at their public defaults in every mode.
  const auto slot_count = workers == 0 ? std::size_t{1} : static_cast<std::size_t>(workers);
  std::vector<std::vector<Record>> slots(slot_count);
  WspdQ34Result result{};
  WspdQ34ParallelWork parallel{};
  std::vector<WspdQ34WorkerWork> worker_work;
  if (workers == 0) {
    result = run_wspd_q34_candidates(index, static_cast<unsigned>(k), static_cast<unsigned>(s), options,
        [&](const Q34SeedCandidate& candidate) {
          slots[0].push_back(copy(candidate, n, static_cast<unsigned>(k), static_cast<unsigned>(mask)));
        });
  } else {
    auto joined = run_wspd_q34_parallel(index, static_cast<unsigned>(k), static_cast<unsigned>(s),
        options, static_cast<std::size_t>(workers), [&](std::size_t slot, const Q34SeedCandidate& candidate) {
          require(slot < slots.size(), "callback slot outside private buffers");
          slots[slot].push_back(copy(candidate, n, static_cast<unsigned>(k), static_cast<unsigned>(mask)));
        });
    result = joined.pipeline;
    parallel = joined.parallel;
    worker_work = std::move(joined.workers);
  }
  std::vector<Record> records;
  for (auto& slot : slots)
    for (auto& record : slot) records.push_back(std::move(record));
  std::sort(records.begin(), records.end());  // Keep multiplicities.
  u64 q3 = 0, q4 = 0, shell_ids = 0;
  for (const auto& record : records) {
    counter_add(record.arity == 3 ? q3 : q4);
    counter_add(shell_ids, static_cast<u64>(record.shell.size()));
  }
  require(q3 == result.work.q3_emitted && q4 == result.work.q4_emitted &&
          shell_ids == result.work.payload_shell_ids, "output ledger mismatch");
  auto logical = result.work;
  logical.q3.peak_shell_bytes = 0;
  logical.peak_edge_buffer_bytes = 0;
  std::cout << "{\"schema\":\"audit_q34_global_contract_probe_v1\",\"status\":\"completed\","
      "\"scope\":\"global_canonical_presentations_not_all_incidences_or_FULL\","
      "\"validation\":\"structure_and_ledger_only_external_exhaustive_oracle_required\","
      "\"source_snapshot\":\"external_runner_manifest\",\"layout\":\"v31_front49_work279_parallel11_worker7\","
      "\"n\":" << n << ",\"kmax\":" << k << ",\"s\":" << s << ",\"mask\":" << mask
      << ",\"q4_backend\":" << backend << ",\"front_mode\":\"" << mode << "\",\"workers\":" << workers
      << ",\"input_fnv1a_u64_le\":" << input_hash << ",\"front\":{\"total_unordered_pairs\":"
      << result.front.total_unordered_pairs << ",\"active_lane_mask\":"
      << static_cast<unsigned>(result.front.active_lane_mask) << ",\"rejected_pair_mass\":";
  dump_numbers(result.front.work.rejected_pair_mass);
  std::cout << ",\"residual_pair_mass\":"; dump_numbers(result.front.work.residual_pair_mass);
  std::cout << ",\"lane_rectangles\":"; dump_numbers(result.front.work.lane_rectangles);
  std::cout << ",\"words\":"; dump_numbers(words<49>(result.front.work));
  std::cout << "},\"ledger\":{";
#define FIELD(name) std::cout << "\"" #name "\":" << result.work.name
  FIELD(input_rectangles); std::cout << ','; FIELD(expanded_pairs); std::cout << ',';
  FIELD(q3_edges); std::cout << ','; FIELD(q4_edges); std::cout << ','; FIELD(both_edges); std::cout << ',';
  FIELD(cover_builds); std::cout << ','; FIELD(cover_sites); std::cout << ',';
  FIELD(max_cover_sites); std::cout << ','; FIELD(q3_emitted); std::cout << ',';
  FIELD(q4_emitted); std::cout << ','; FIELD(payload_shell_ids);
#undef FIELD
  std::cout << "},\"capacities\":{\"peak_cover_bytes\":" << result.work.peak_cover_bytes
      << ",\"q3_peak_shell_bytes\":" << result.work.q3.peak_shell_bytes
      << ",\"peak_edge_buffer_bytes\":" << result.work.peak_edge_buffer_bytes
      << "},\"work_words\":";
  dump_numbers(words<279>(result.work));
  std::cout << ",\"logical_work_words\":"; dump_numbers(words<279>(logical));
  std::cout << ",\"parallel_words\":"; dump_numbers(words<11>(parallel));
  std::cout << ",\"worker_words\":[";
  for (std::size_t i = 0; i != worker_work.size(); ++i) {
    if (i != 0) std::cout << ',';
    dump_numbers(words<7>(worker_work[i]));
  }
  std::cout << "],\"records\":[";
  for (std::size_t i = 0; i != records.size(); ++i) {
    if (i != 0) std::cout << ',';
    dump_record(records[i]);
  }
  std::cout << "]}\n";
  require(static_cast<bool>(std::cout), "output write failed");
  return 0;
}
}  // namespace

int main(int argc, char** argv) {
  try { return run(argc, argv); }
  catch (const std::exception& error) {
    std::cerr << "audit global probe: " << error.what() << '\n';
    return 1;
  }
}
