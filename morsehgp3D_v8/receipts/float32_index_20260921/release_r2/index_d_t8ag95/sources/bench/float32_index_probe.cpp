#include "spatial/float32_index.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using Words = mhgp8::Float32Words;
using Clock = std::chrono::steady_clock;

struct Options {
  std::string input;
  std::string synthetic;
  std::size_t n{};
  std::size_t repeat{3};
};

void require(bool condition, const char* message) {
  if (!condition) throw std::invalid_argument(message);
}

std::size_t positive_size(std::string_view text) {
  std::size_t value{};
  const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
  require(result.ec == std::errc{} && result.ptr == text.data() + text.size() && value != 0,
          "expected a positive, addressable decimal integer");
  return value;
}

Options options(int argc, char** argv) {
  Options result;
  bool has_input = false, has_synthetic = false, has_n = false, has_repeat = false;
  for (int i = 1; i < argc; ++i) {
    const std::string_view name(argv[i]);
    require(name == "--input" || name == "--synthetic" || name == "--n" || name == "--repeat",
            "usage: --input PATH | --synthetic uniform|terrain|clusters --n N; [--repeat R]");
    require(i + 1 < argc, "option requires a value");
    const std::string_view value(argv[++i]);
    if (name == "--input") {
      require(!has_input && !value.empty(), "duplicate or empty --input");
      has_input = true;
      result.input = value;
    } else if (name == "--synthetic") {
      require(!has_synthetic, "duplicate --synthetic");
      require(value == "uniform" || value == "terrain" || value == "clusters",
              "unknown synthetic cloud");
      has_synthetic = true;
      result.synthetic = value;
    } else if (name == "--n") {
      require(!has_n, "duplicate --n");
      has_n = true;
      result.n = positive_size(value);
    } else {
      require(!has_repeat, "duplicate --repeat");
      has_repeat = true;
      result.repeat = positive_size(value);
    }
  }
  require(has_input != has_synthetic, "select exactly one input source");
  require(has_n == has_synthetic, "--n is required only for synthetic input");
  return result;
}

std::uint32_t little_word(const unsigned char* bytes) noexcept {
  return static_cast<std::uint32_t>(bytes[0]) |
         (static_cast<std::uint32_t>(bytes[1]) << 8) |
         (static_cast<std::uint32_t>(bytes[2]) << 16) |
         (static_cast<std::uint32_t>(bytes[3]) << 24);
}

std::vector<Words> read_input(const std::string& path) {
  // This format has EXACTLY three little-endian binary32 words per site:
  // no intensity field, header, quantization, or implicit sampling.
  std::ifstream stream(path, std::ios::binary | std::ios::ate);
  require(static_cast<bool>(stream), "cannot open input file");
  const auto end = stream.tellg();
  require(end != std::ifstream::pos_type(-1), "cannot determine input size");
  const auto bytes = static_cast<std::streamoff>(end);
  require(bytes > 0 && bytes % 12 == 0, "input must contain nonempty, complete 12-byte XYZ sites");
  const auto count = static_cast<std::uintmax_t>(bytes / 12);
  require(count <= std::numeric_limits<std::size_t>::max(), "input site count is not addressable");
  const auto n = static_cast<std::size_t>(count);
  std::vector<Words> points;
  points.reserve(n);
  stream.seekg(0, std::ios::beg);
  require(static_cast<bool>(stream), "cannot seek to input start");
  // A bounded I/O buffer is a transfer grain, never a point/search limit.
  constexpr std::size_t block_sites = 4096;
  std::array<unsigned char, 12 * block_sites> buffer{};
  while (points.size() != n) {
    const auto sites = std::min(block_sites, n - points.size());
    const auto wanted = static_cast<std::streamsize>(12 * sites);
    stream.read(reinterpret_cast<char*>(buffer.data()), wanted);
    require(stream.gcount() == wanted && !stream.bad(), "input was truncated or could not be read");
    for (std::size_t i = 0; i != sites; ++i) {
      const auto* first = buffer.data() + 12 * i;
      points.push_back({little_word(first), little_word(first + 4), little_word(first + 8)});
    }
  }
  require(stream.peek() == std::char_traits<char>::eof() && !stream.bad(),
          "input grew while being read or had a read error");
  return points;
}

std::uint64_t permute(std::uint64_t id) noexcept {
  // SplitMix64 is a bijection: addition, odd multiplications and xor/right
  // shifts are invertible on 64-bit words. Distinct IDs give distinct triples.
  auto value = id + 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31);
}

std::uint32_t scaled_bits(std::int64_t numerator, int exponent) {
  static_assert(sizeof(float) == sizeof(std::uint32_t) &&
                std::numeric_limits<float>::is_iec559 &&
                std::numeric_limits<float>::radix == 2 &&
                std::numeric_limits<float>::digits == 24 &&
                std::numeric_limits<float>::max_exponent == 128,
                "synthetic input requires native IEEE binary32");
  // All callers have |numerator| < 2^24 and exact normal-or-zero results.
  return std::bit_cast<std::uint32_t>(std::ldexp(static_cast<float>(numerator), exponent));
}

std::vector<Words> synthetic_input(std::string_view kind, std::size_t n) {
  static_assert(std::numeric_limits<std::size_t>::digits <= 64);
  std::vector<Words> points;
  points.reserve(n);
  for (std::size_t id = 0; id != n; ++id) {
    const auto mixed = permute(static_cast<std::uint64_t>(id));
    const auto field_x = mixed & ((std::uint64_t{1} << 22) - 1);
    const auto field_y = (mixed >> 22) & ((std::uint64_t{1} << 21) - 1);
    const auto field_z = mixed >> 43;
    const auto x = static_cast<std::int64_t>(field_x) - (std::int64_t{1} << 21);
    const auto y = static_cast<std::int64_t>(field_y) - (std::int64_t{1} << 20);
    const auto z = static_cast<std::int64_t>(field_z) - (std::int64_t{1} << 20);
    if (kind == "clusters") {
      // Three upper x bits select eight disjoint boxes centred at (+/-4)^3.
      // Remaining x/y/z bits map injectively into each box with half-width1.
      const auto cluster = field_x >> 19;
      const auto fine_x = static_cast<std::int64_t>(field_x & ((std::uint64_t{1} << 19) - 1)) -
                          (std::int64_t{1} << 18);
      const auto cx = (cluster & 1U) != 0 ? std::int64_t{1} << 20 : -(std::int64_t{1} << 20);
      const auto cy = (cluster & 2U) != 0 ? std::int64_t{1} << 22 : -(std::int64_t{1} << 22);
      const auto cz = (cluster & 4U) != 0 ? std::int64_t{1} << 22 : -(std::int64_t{1} << 22);
      points.push_back({scaled_bits(cx + fine_x, -18), scaled_bits(cy + y, -20),
                        scaled_bits(cz + z, -20)});
    } else if (kind == "terrain") {
      // Keep x/y unchanged, then add an integer relief to z. Given x/y this
      // is an invertible translation; thickness2 is small against width64.
      const auto height = (x < 0 ? -x : x) / 2 + (y < 0 ? -y : y);
      points.push_back({scaled_bits(x, -16), scaled_bits(y, -15), scaled_bits(z + height, -20)});
    } else {
      // Exact dyadic cube [-32,32)^3; 22/21/21 bits retain all 64 ID bits.
      points.push_back({scaled_bits(x, -16), scaled_bits(y, -15), scaled_bits(z, -15)});
    }
  }
  return points;
}

double milliseconds(Clock::time_point begin, Clock::time_point end) {
  return std::chrono::duration<double, std::milli>(end - begin).count();
}

void hash_word(std::uint64_t& hash, std::uint64_t value, unsigned bytes) noexcept {
  // FNV-1a, explicitly little-endian bytes, modulo 2^64 (not cryptographic).
  for (unsigned i = 0; i != bytes; ++i) {
    hash ^= (value >> (8U * i)) & 0xffU;
    hash *= 1099511628211ULL;
  }
}

std::string hex64(std::uint64_t value) {
  std::ostringstream output;
  output << std::hex << std::setfill('0') << std::setw(16) << value;
  return output.str();
}

struct Record {
  double build_ms{}, query_ms{};
  std::size_t nodes{}, max_depth{}, retained_bytes{}, construction_peak_vector_bytes{};
  mhgp8::Float32IndexWork work{};
  mhgp8::Float32BoxQueryWork query_work{};
  std::uint64_t point_checksum{14695981039346656037ULL};
  std::uint64_t permutation_checksum{14695981039346656037ULL};
  std::uint64_t query_id_sum{}, query_id_xor{};
};

Record measure(const std::vector<Words>& input) {
  Record record;
  const auto begin = Clock::now();
  const auto index = mhgp8::prepare_float32_index(input);
  const auto built = Clock::now();
  record.build_ms = milliseconds(begin, built);
  record.nodes = index->nodes().size();
  record.max_depth = index->max_depth();
  record.retained_bytes = index->retained_bytes();
  record.construction_peak_vector_bytes = index->construction_peak_vector_bytes();
  record.work = index->work();
  // Checksums of owned point words/original IDs are OUTSIDE build_ms.
  // Both start with n as u64LE; points then use three u32LE words/site,
  // permutation uses one u64LE original ID per rank. Signed-zero bits survive.
  hash_word(record.point_checksum, static_cast<std::uint64_t>(input.size()), 8);
  for (const auto& point : index->points())
    for (const auto word : point.bits()) hash_word(record.point_checksum, word, 4);
  hash_word(record.permutation_checksum, static_cast<std::uint64_t>(input.size()), 8);
  for (const auto id : index->permutation())
    hash_word(record.permutation_checksum, static_cast<std::uint64_t>(id), 8);

  // Query payload checksum work IS included in query_ms. These are closed
  // AABBs selected from index nodes, not q2/q3/q4 predicates or a FULL tower.
  const std::function<void(std::span<const std::size_t>)> emit = [&](auto ids) {
    for (const auto id : ids) {
      record.query_id_sum += static_cast<std::uint64_t>(id);
      record.query_id_xor ^= static_cast<std::uint64_t>(id);
    }
  };
  const auto query_begin = Clock::now();
  const auto last = index->nodes().size() - 1;
  for (std::size_t q = 0; q != 16; ++q) {
    // Exactly floor(q*last/15), avoiding an overflowing intermediate product.
    const auto selected = q * (last / 15) + q * (last % 15) / 15;
    index->visit_box(index->nodes()[selected].box, emit, record.query_work);
  }
  record.query_ms = milliseconds(query_begin, Clock::now());
  return record;  // The immutable owner stays alive throughout all callbacks.
}

void dump(const mhgp8::Float32IndexWork& work) {
  std::cout << '{';
#define FIELD(name) std::cout << '\"' << #name << "\":" << work.name
  FIELD(input_word_triples_copied); std::cout << ',';
  FIELD(finite_points_validated); std::cout << ',';
  FIELD(point_objects_constructed); std::cout << ',';
  FIELD(presort_comparisons); std::cout << ',';
  FIELD(duplicate_adjacent_tests); std::cout << ',';
  FIELD(box_endpoint_reads); std::cout << ',';
  FIELD(partition_rank_writes); std::cout << ',';
  FIELD(partition_id_reads); std::cout << ',';
  FIELD(partition_id_writes); std::cout << ',';
  FIELD(inverse_rank_writes); std::cout << ',';
  FIELD(nodes); std::cout << ',';
  FIELD(leaves);
#undef FIELD
  std::cout << '}';
}

void dump(const mhgp8::Float32BoxQueryWork& work) {
  std::cout << '{';
#define FIELD(name) std::cout << '\"' << #name << "\":" << work.name
  FIELD(node_visits); std::cout << ',';
  FIELD(axis_tests); std::cout << ',';
  FIELD(rejected_nodes); std::cout << ',';
  FIELD(accepted_nodes); std::cout << ',';
  FIELD(refined_nodes); std::cout << ',';
  FIELD(emitted_sites); std::cout << ',';
  FIELD(callbacks);
#undef FIELD
  std::cout << '}';
}

void dump(const Record& record) {
  std::cout << "{\"build_ms\":" << record.build_ms << ",\"query_ms\":" << record.query_ms
            << ",\"nodes\":" << record.nodes << ",\"max_depth\":" << record.max_depth
            << ",\"retained_bytes\":" << record.retained_bytes
            << ",\"construction_peak_vector_bytes\":" << record.construction_peak_vector_bytes
            << ",\"work\":";
  dump(record.work);
  std::cout << ",\"query_work\":";
  dump(record.query_work);
  std::cout << ",\"point_checksum\":\"" << hex64(record.point_checksum)
            << "\",\"permutation_checksum\":\"" << hex64(record.permutation_checksum)
            << "\",\"query_id_sum\":\"" << hex64(record.query_id_sum)
            << "\",\"query_id_xor\":\"" << hex64(record.query_id_xor) << "\"}";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const auto selected = options(argc, argv);
    const auto input_begin = Clock::now();
    const auto input = selected.input.empty() ? synthetic_input(selected.synthetic, selected.n)
                                              : read_input(selected.input);
    const double input_ms = milliseconds(input_begin, Clock::now());
    std::vector<Record> records;
    records.reserve(selected.repeat);
    for (std::size_t repeat = 0; repeat != selected.repeat; ++repeat)
      records.push_back(measure(input));
    std::cout << std::setprecision(17)
              << "{\"schema\":\"mhgp8_float32_index_bench_v1\",\"status\":\"passed\",\"n\":"
              << input.size() << ",\"input_ms\":" << input_ms
              << ",\"point_bytes\":" << sizeof(mhgp8::Float32Point3)
              << ",\"node_bytes\":" << sizeof(mhgp8::Float32IndexNode) << ",\"records\":[";
    for (std::size_t i = 0; i != records.size(); ++i) {
      if (i != 0) std::cout << ',';
      dump(records[i]);
    }
    std::cout << "]}\n";
    require(static_cast<bool>(std::cout), "output stream failed");
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "float32 index probe: " << error.what() << '\n';
    return 1;
  }
}
