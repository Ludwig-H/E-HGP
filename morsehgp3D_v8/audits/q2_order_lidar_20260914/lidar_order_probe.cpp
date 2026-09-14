// Explicit adaptation of q2_front_20260914/lidar_q2_probe.cpp, SHA256
// 217b2bca2956e75df52a4ad1877959a5bd3e2078384b4c06faabf7c0123e8f99.
// Adds the published sibling/order options and their counters; digest unchanged.
// Explicit audit adaptation of lidar08_20260914/front_input_probe.cpp:
// SHA256 c4aea05be88d2a07c775a926fdb7f525b3592d6968bcd5f0d54328bc3a553bcb.
// Canonical support encoding and callback checks adapted from
// morsehgp3D_v8/bench/wspd_q2_census_probe.cpp, SHA256
// 04523b566cfa5395f0ac2d23e9925c4ff5bcf18882cd1c5e0f2f0d5a1bae6e0f.
// Checksums compare streams without storing them; they are not an oracle.
#include "pipeline/wspd_q2_census.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
using namespace mhgp8;
using Clock = std::chrono::steady_clock;
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
unsigned number(std::string_view text) {
  unsigned result{};
  const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
  require(!text.empty() && parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size(),
          "invalid unsigned parameter");
  return result;
}
u64 product(u64 a, u64 b) {
  require(b == 0 || a <= std::numeric_limits<u64>::max() / b, "counter product overflow");
  return a * b;
}
double ms(Clock::time_point a, Clock::time_point b) {
  return std::chrono::duration<double, std::milli>(b - a).count();
}
void hash_word(u64& hash, u64 value) {
  for (unsigned byte = 0; byte < 8; ++byte) {
    hash ^= value & 255U; hash *= 1099511628211ULL; value >>= 8U;
  }
}
struct CallbackWork {
  u64 copied_ids{}, sort_calls{}, sort_comparisons{}, validation_ids{};
  u64 adjacent_tests{}, cross_set_comparisons{}, support_key_axis_checks{}, hash_words{};
};
struct OutputDigest {
  u64 supports{}, interior_ids{}, shell_ids{}, sum{}, xor_value{};
  CallbackWork work;
  std::vector<std::size_t> interior, shell;
  void consume(const Q2Support& support, std::span<const Point3> points, unsigned kmax) {
    require(support.a_id < points.size() && support.b_id < points.size() &&
            support.a_id != support.b_id && support.interior.size() < kmax &&
            support.shell.size() >= 2, "invalid materialized q2 support");
    const auto& a = points[support.a_id];
    const auto& b = points[support.b_id];
    u64 diameter = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      counter_add(work.support_key_axis_checks);
      require(support.key.center_twice[axis] == static_cast<unsigned>(a[axis]) + b[axis],
              "support midpoint disagrees with endpoints");
      const i64 delta = static_cast<i64>(a[axis]) - b[axis];
      counter_add(diameter, static_cast<u64>(delta * delta));
    }
    require(diameter == support.key.diameter_squared, "support diameter disagrees with endpoints");
    interior.assign(support.interior.begin(), support.interior.end());
    shell.assign(support.shell.begin(), support.shell.end());
    counter_add(work.copied_ids, interior.size()); counter_add(work.copied_ids, shell.size());
    bool saw_a = false, saw_b = false;
    for (unsigned part = 0; part < 2; ++part) {
      auto& ids = part == 0 ? interior : shell;
      counter_add(work.sort_calls);
      std::sort(ids.begin(), ids.end(), [&](std::size_t left, std::size_t right) {
        counter_add(work.sort_comparisons); return left < right;
      });
      for (std::size_t i = 0; i < ids.size(); ++i) {
        counter_add(work.validation_ids);
        require(ids[i] < points.size(), "materialized ID escaped the cloud");
        require(part != 0 || (ids[i] != support.a_id && ids[i] != support.b_id),
                "support endpoint emitted as strict interior");
        if (i != 0) {
          counter_add(work.adjacent_tests);
          require(ids[i - 1] != ids[i], "duplicate ID inside a support");
        }
        if (part != 0) { saw_a = saw_a || ids[i] == support.a_id; saw_b = saw_b || ids[i] == support.b_id; }
      }
    }
    require(saw_a && saw_b, "shell lost an endpoint");
    std::size_t i = 0, j = 0;
    while (i < interior.size() && j < shell.size()) {
      counter_add(work.cross_set_comparisons);
      require(interior[i] != shell[j], "interior and shell overlap");
      if (interior[i] < shell[j]) ++i; else ++j;
    }
    u64 hash = 14695981039346656037ULL;
    const auto word = [&](u64 value) { counter_add(work.hash_words); hash_word(hash, value); };
    word(2); word(std::min(support.a_id, support.b_id)); word(std::max(support.a_id, support.b_id));
    for (const auto coordinate : support.key.center_twice) word(coordinate);
    word(support.key.diameter_squared);
    word(interior.size()); for (const auto id : interior) word(id);
    word(shell.size()); for (const auto id : shell) word(id);
    counter_add(supports); counter_add(interior_ids, interior.size()); counter_add(shell_ids, shell.size());
    sum += hash; xor_value ^= hash;  // Deliberate modulo arithmetic only for checksums.
  }
};
struct Fields {
  bool first = true;
  void add(const char* name, u64 value) {
    if (!first) std::cout << ',';
    first = false; std::cout << '"' << name << "\":" << value;
  }
};
template <std::size_t N> void array(const char* name, const std::array<u64, N>& values) {
  std::cout << ",\"" << name << "\":[";
  for (std::size_t i = 0; i < N; ++i) { if (i != 0) std::cout << ','; std::cout << values[i]; }
  std::cout << ']';
}
int run(int argc, char** argv) {
  require(argc == 8, "usage: lidar_order_probe input.u16le Kmax s pure|samples shared none|sibling global|complement");
  const unsigned k = number(argv[2]), s = number(argv[3]);
  const std::string_view front_mode(argv[4]), census_mode(argv[5]), sibling_mode(argv[6]), witness_order(argv[7]);
  require(census_mode == "shared" && (sibling_mode == "none" || sibling_mode == "sibling") &&
          (witness_order == "global" || witness_order == "complement"), "unsupported order parameters");
  require(k >= 1 && k <= 10 && s > 0 && (front_mode == "pure" || front_mode == "samples") &&
          (census_mode == "pairwise" || census_mode == "shared"), "unsupported parameters");
  const auto started = Clock::now();
  std::ifstream input(argv[1], std::ios::binary | std::ios::ate);
  require(input.is_open(), "cannot open input");
  const std::streamoff bytes = input.tellg();
  require(bytes >= std::streamoff{12} && bytes % std::streamoff{6} == 0, "invalid u16 input length");
  const auto n = static_cast<u64>(bytes / std::streamoff{6});
  require(n <= std::numeric_limits<std::size_t>::max(), "input count exceeds size_t");
  input.seekg(0);
  std::vector<Point3> points;
  points.reserve(static_cast<std::size_t>(n));
  u64 input_hash = 14695981039346656037ULL;
  for (u64 i = 0; i < n; ++i) {
    std::array<unsigned char, 6> raw{};
    input.read(reinterpret_cast<char*>(raw.data()), 6);
    require(input.gcount() == 6 && !input.bad(), "truncated u16 input");
    const auto word = [&](std::size_t offset) {
      return static_cast<std::uint16_t>(static_cast<unsigned>(raw[offset]) |
                                       (static_cast<unsigned>(raw[offset + 1]) << 8U));
    };
    points.push_back({word(0), word(2), word(4)});
    for (const auto byte : raw) { input_hash ^= byte; input_hash *= 1099511628211ULL; }
  }
  require(input.peek() == std::char_traits<char>::eof() && !input.bad(), "input length changed");
  input.close();
  const auto loaded = Clock::now();
  auto cloud = prepare_cloud(points);
  const auto prepared = Clock::now();
  auto index = make_q2_cloud_index(cloud);
  const auto indexed = Clock::now();
  OutputDigest digest;
  const auto result = run_wspd_q2_census(*index, k, s,
      front_mode == "pure" ? WspdFrontMode::Pure : WspdFrontMode::MidpointSamples,
      census_mode == "pairwise" ? Q2CensusMode::Pairwise : Q2CensusMode::SharedBlocks,
      [&](const Q2Support& support) { digest.consume(support, cloud->points(), k); },
      sibling_mode == "none" ? Q2SiblingMode::Disabled : Q2SiblingMode::Saturating,
      witness_order == "global" ? Q2WitnessOrder::GlobalDfs : Q2WitnessOrder::ComplementFirst);
  const auto processed = Clock::now();
  const auto& f = result.front.work;
  const auto& c = result.census;
  const auto total = n % 2 == 0 ? product(n / 2, n - 1) : product(n, (n - 1) / 2);
  require(result.front.total_unordered_pairs == total && result.front.active_lane_mask == 1 &&
          f.rejected_pair_mass[0] <= total && f.residual_pair_mass[0] == total - f.rejected_pair_mass[0] &&
          c.candidate_pairs == f.residual_pair_mass[0] && c.accepted_pairs <= c.candidate_pairs &&
          c.rejected_pairs == c.candidate_pairs - c.accepted_pairs, "q2 pair ledger mismatch");
  require(result.input_rectangles == f.emitted_rectangles && f.lane_rectangles[0] == f.emitted_rectangles &&
          result.anchor_queries >= result.input_rectangles && result.anchor_queries <= c.candidate_pairs &&
          f.xi_bound_tests == 0 && (front_mode != "pure" || f.rejected_pair_mass[0] == 0), "q2 front scope mismatch");
  for (unsigned q = 1; q < 3; ++q) require(f.rejected_pair_mass[q] == 0 && f.residual_pair_mass[q] == 0 &&
      f.lane_rectangles[q] == 0, "higher lane entered the q2 pipeline");
  require(c.work.query_build_nodes == 0 && c.work.query_build_point_visits == 0 && c.work.query_cover_visits == 0 &&
          c.work.query_build_max_depth == 0 && c.work.frontier_restarts == 0 &&
          c.work.input_descriptors == result.input_rectangles && c.work.count_root_starts ==
          (census_mode == "shared" ? result.anchor_queries : c.candidate_pairs), "continuation or local-index mismatch");
  require(digest.supports == c.accepted_pairs && digest.supports == c.work.payload_supports &&
          digest.interior_ids == c.work.payload_interior_sites && digest.shell_ids == c.work.payload_shell_sites,
          "physical callback ledger mismatch");
  auto ids = digest.interior_ids;
  counter_add(ids, digest.shell_ids);
  auto words = product(9, digest.supports); counter_add(words, ids);
  require(digest.work.copied_ids == ids && digest.work.validation_ids == ids && digest.work.hash_words == words &&
          digest.work.sort_calls == product(2, digest.supports) &&
          digest.work.support_key_axis_checks == product(3, digest.supports), "callback work mismatch");
  require(c.total_ms == result.total_ms && c.query_index_ms == 0 && c.count_ms >= 0 && c.payload_ms >= 0 &&
          c.total_ms + 1e-6 >= c.count_ms + c.payload_ms, "pipeline time contract mismatch");
  const auto index_work = index->work();
  const auto cloud_bytes = cloud->retained_bytes(), index_bytes = index->retained_bytes();
  auto callback_bytes = product(digest.interior.capacity(), sizeof(std::size_t));
  counter_add(callback_bytes, product(digest.shell.capacity(), sizeof(std::size_t)));
  const auto validated = Clock::now();
  std::vector<std::size_t>().swap(digest.interior); std::vector<std::size_t>().swap(digest.shell);
  index.reset(); cloud.reset(); std::vector<Point3>().swap(points);
  const auto finished = Clock::now();
  std::cout.imbue(std::locale::classic());
  std::cout << std::setprecision(17) << "{\"status\":\"completed\",\"schema\":\"mhgp8_lidar_q2_audit_v1\","
            << "\"scope\":\"q2_support_stream_not_full_checksums_not_oracle\",\"public_status\":\"not_claimed\","
            << "\"threads\":1,\"gcp_used\":false,\"profile\":\"quantized_u16_input_only\","
            << "\"separation_convention\":\"box_gap_diameter_v1\",\"input_format\":\"xyz_u16_little_endian\",\"n\":" << n
            << ",\"kmax\":" << k << ",\"s\":" << s << ",\"front_mode\":\"" << front_mode
            << "\",\"census_mode\":\"" << census_mode << "\",\"input_fnv64\":\"" << std::hex << input_hash << std::dec << '"'
            << ",\"total_unordered_pairs\":" << total << ",\"active_lane_mask\":1,\"input_rectangles\":" << result.input_rectangles
            << ",\"anchor_queries\":" << result.anchor_queries << ",\"candidate_pairs\":" << c.candidate_pairs
            << ",\"accepted_pairs\":" << c.accepted_pairs << ",\"rejected_pairs\":" << c.rejected_pairs << ",\"front_work\":{";
#define EMIT(object, name) fields.add(#name, object.name)
  {
    Fields fields;
    EMIT(f, product_visits); EMIT(f, diagonal_splits); EMIT(f, diagonal_leaves); EMIT(f, disjoint_splits);
    EMIT(f, separation_tests); EMIT(f, witness_searches); EMIT(f, witness_descent_steps); EMIT(f, witness_box_distance_tests);
    EMIT(f, proposed_sites); EMIT(f, proposals_in_factors); EMIT(f, h_bound_tests); EMIT(f, xi_bound_tests);
    EMIT(f, witness_lane_credits); EMIT(f, fully_rejected_products); EMIT(f, emitted_rectangles); EMIT(f, emitted_factor_sites);
    EMIT(f, max_factor_size); EMIT(f, leaf_pair_rectangles); EMIT(f, max_stack_size); EMIT(f, max_product_depth);
  }
  array("size_class_rectangles", f.size_class_rectangles); array("size_class_pair_mass", f.size_class_pair_mass);
  array("rejected_pair_mass", f.rejected_pair_mass); array("residual_pair_mass", f.residual_pair_mass);
  array("lane_rectangles", f.lane_rectangles);
  std::cout << "},\"census_work\":{";
  {
    Fields fields;
    EMIT(c.work, query_build_point_visits); EMIT(c.work, query_build_nodes); EMIT(c.work, query_build_max_depth);
    EMIT(c.work, input_descriptors); EMIT(c.work, query_cover_visits); EMIT(c.work, query_tasks); EMIT(c.work, query_splits);
    EMIT(c.work, witness_splits); EMIT(c.work, count_root_starts); EMIT(c.work, shared_splits_after_credit);
    EMIT(c.work, cursor_advances); EMIT(c.work, cursor_reuses); EMIT(c.work, count_node_visits);
    EMIT(c.work, count_bound_tests); EMIT(c.work, count_point_tests); EMIT(c.work, uniform_credited_pairs);
    EMIT(c.work, uniform_rejected_pairs); EMIT(c.work, uniform_accepted_pairs); EMIT(c.work, consumed_witness_sites);
    EMIT(c.work, frontier_restarts); EMIT(c.work, payload_node_visits); EMIT(c.work, payload_bound_tests);
    EMIT(c.work, payload_point_tests); EMIT(c.work, payload_interior_sites); EMIT(c.work, payload_shell_sites); EMIT(c.work, payload_supports);
  }
  std::cout << "},\"sibling_mode\":\"" << sibling_mode << "\",\"witness_order\":\"" << witness_order
            << "\",\"sibling_work\":{";
  {
    Fields fields;
    EMIT(result.sibling_work, proposals); EMIT(result.sibling_work, cardinality_skips);
    EMIT(result.sibling_work, bound_tests); EMIT(result.sibling_work, rejected_tasks);
    EMIT(result.sibling_work, rejected_pairs); EMIT(result.sibling_work, rejected_after_credit);
  }
  std::cout << "},\"order_work\":{";
  {
    Fields fields;
    EMIT(result.order_work, structural_splits); EMIT(result.order_work, deferred_skips);
    EMIT(result.order_work, anchor_skips); EMIT(result.order_work, phase_switches);
  }
  std::cout << "},\"callback_work\":{";
  {
    Fields fields;
    EMIT(digest.work, copied_ids); EMIT(digest.work, sort_calls); EMIT(digest.work, sort_comparisons);
    EMIT(digest.work, validation_ids); EMIT(digest.work, adjacent_tests); EMIT(digest.work, cross_set_comparisons);
    EMIT(digest.work, support_key_axis_checks); EMIT(digest.work, hash_words);
  }
#undef EMIT
  std::cout << "},\"digest\":{\"encoding\":\"canonical_q2_support_v2\",\"supports\":" << digest.supports
            << ",\"interior_ids\":" << digest.interior_ids << ",\"shell_ids\":" << digest.shell_ids
            << ",\"sum\":\"" << std::hex << digest.sum << "\",\"xor\":\"" << digest.xor_value << std::dec << "\"}"
            << ",\"index_point_visits\":" << index_work.point_visits << ",\"index_max_depth\":" << index_work.max_depth
            << ",\"cloud_retained_bytes\":" << cloud_bytes << ",\"index_retained_bytes\":" << index_bytes
            << ",\"callback_buffers_capacity_bytes\":" << callback_bytes
            << ",\"load_ms\":" << ms(started, loaded) << ",\"cloud_ms\":" << ms(loaded, prepared)
            << ",\"index_ms\":" << ms(prepared, indexed) << ",\"pipeline_and_callback_ms\":" << ms(indexed, processed)
            << ",\"validation_ms\":" << ms(processed, validated) << ",\"destruction_ms\":" << ms(validated, finished)
            << ",\"total_ms\":" << ms(started, finished) << ",\"pipeline_total_ms\":" << result.total_ms
            << ",\"front_and_count_ms\":" << c.count_ms << ",\"payload_ms\":" << c.payload_ms
            << ",\"query_index_ms\":" << c.query_index_ms << "}\n";
  return 0;
}
}  // namespace
int main(int argc, char** argv) {
  try { return run(argc, argv); }
  catch (const std::exception& error) { std::cerr << "LiDAR q2 audit failed: " << error.what() << '\n'; return 1; }
}
