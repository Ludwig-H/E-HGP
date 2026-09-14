// Audit adapter for already quantized inputs; ledger checks are not an oracle.
#include "wspd/front.hpp"
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
  require(b == 0 || a <= std::numeric_limits<u64>::max() / b, "pair mass overflow");
  return a * b;
}
double ms(Clock::time_point a, Clock::time_point b) {
  return std::chrono::duration<double, std::milli>(b - a).count();
}
template <std::size_t N> void array(const char* name, const std::array<u64, N>& values) {
  std::cout << ",\"" << name << "\":[";
  for (std::size_t i = 0; i < N; ++i) { if (i != 0) std::cout << ','; std::cout << values[i]; }
  std::cout << ']';
}
int run(int argc, char** argv) {
  require(argc == 5, "usage: front_input_probe input.u16le Kmax s pure|samples");
  const unsigned k = number(argv[2]), s = number(argv[3]);
  const std::string_view mode(argv[4]);
  require(k >= 1 && k <= 10 && s > 0 && (mode == "pure" || mode == "samples"),
          "unsupported front parameters");
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
  u64 hash = 14695981039346656037ULL;
  for (u64 i = 0; i < n; ++i) {
    std::array<unsigned char, 6> raw{};
    input.read(reinterpret_cast<char*>(raw.data()), 6);
    require(input.gcount() == 6 && !input.bad(), "truncated u16 input");
    const auto word = [&](std::size_t offset) {
      return static_cast<std::uint16_t>(static_cast<unsigned>(raw[offset]) |
                                       (static_cast<unsigned>(raw[offset + 1]) << 8U));
    };
    points.push_back({word(0), word(2), word(4)});
    for (const auto byte : raw) { hash ^= byte; hash *= 1099511628211ULL; }
  }
  require(input.peek() == std::char_traits<char>::eof() && !input.bad(), "input length changed");
  input.close();
  const auto loaded = Clock::now();
  auto cloud = prepare_cloud(points);
  const auto prepared = Clock::now();
  auto index = make_q2_cloud_index(cloud);
  const auto indexed = Clock::now();
  const auto nodes = index->spatial_nodes();
  const unsigned active = (1U << std::min(k, 3U)) - 1U;
  u64 callbacks = 0, factor_sites = 0, union_mass = 0;
  std::array<u64, 3> lane_mass{}, lane_rectangles{};
  const auto result = run_wspd_front(*index, k, s,
      mode == "pure" ? WspdFrontMode::Pure : WspdFrontMode::MidpointSamples,
      [&](const WspdRectangle& rectangle) {
        require(rectangle.a_node < nodes.size() && rectangle.b_node < nodes.size() &&
                rectangle.lane_mask != 0 && (rectangle.lane_mask & ~active) == 0, "invalid descriptor");
        const auto a = nodes[rectangle.a_node].range, b = nodes[rectangle.b_node].range;
        require(a.first < a.last && b.first < b.last && a.last <= n && b.last <= n &&
                (a.last <= b.first || b.last <= a.first), "invalid factor ranges");
        const auto mass = product(a.size(), b.size());
        counter_add(callbacks); counter_add(factor_sites, a.size()); counter_add(factor_sites, b.size());
        counter_add(union_mass, mass);
        for (unsigned q = 0; q < 3; ++q) if ((rectangle.lane_mask & (1U << q)) != 0) {
          counter_add(lane_mass[q], mass); counter_add(lane_rectangles[q]);
        }
      });
  const auto front_done = Clock::now();
  const auto& w = result.work;
  const auto total = n % 2 == 0 ? product(n / 2, n - 1) : product(n, (n - 1) / 2);
  require(result.total_unordered_pairs == total && result.active_lane_mask == active &&
          callbacks == w.emitted_rectangles && factor_sites == w.emitted_factor_sites &&
          lane_mass == w.residual_pair_mass && lane_rectangles == w.lane_rectangles &&
          union_mass <= total, "front callback ledger mismatch");
  for (unsigned q = 0; q < 3; ++q) {
    const auto expected = (active & (1U << q)) != 0 ? total : 0;
    require(w.rejected_pair_mass[q] <= expected && lane_mass[q] == expected - w.rejected_pair_mass[q],
            "front lane mass is not conserved");
    require(mode != "pure" || w.rejected_pair_mass[q] == 0, "pure mode rejected pairs");
  }
  const auto index_work = index->work();
  const auto cloud_bytes = cloud->retained_bytes(), index_bytes = index->retained_bytes();
  const auto validated = Clock::now();
  index.reset(); cloud.reset(); std::vector<Point3>().swap(points);
  const auto finished = Clock::now();
  std::cout.imbue(std::locale::classic());
  std::cout << std::setprecision(17) << "{\"status\":\"completed\",\"schema\":\"mhgp8_lidar_front_audit_v1\","
            << "\"scope\":\"draft_front_only_ledger_not_geometry_oracle\",\"public_status\":\"not_claimed\","
            << "\"threads\":1,\"gcp_used\":false,\"profile\":\"quantized_u16_input_only\","
            << "\"separation_convention\":\"box_gap_diameter_v1\",\"input_format\":\"xyz_u16_little_endian\",\"n\":" << n
            << ",\"kmax\":" << k << ",\"s\":" << s << ",\"front_mode\":\"" << mode << '"'
            << ",\"input_fnv64\":\"" << std::hex << hash << std::dec << '"'
            << ",\"active_lane_mask\":" << active << ",\"total_unordered_pairs\":" << total << ",\"callback_rectangles\":" << callbacks
            << ",\"callback_factor_sites\":" << factor_sites << ",\"callback_union_pair_mass\":" << union_mass;
#define FIELD(name) std::cout << ",\"" #name "\":" << w.name
  FIELD(product_visits); FIELD(diagonal_splits); FIELD(diagonal_leaves); FIELD(disjoint_splits);
  FIELD(separation_tests); FIELD(witness_searches); FIELD(witness_descent_steps); FIELD(witness_box_distance_tests);
  FIELD(proposed_sites); FIELD(proposals_in_factors); FIELD(h_bound_tests); FIELD(xi_bound_tests);
  FIELD(witness_lane_credits); FIELD(fully_rejected_products); FIELD(emitted_rectangles); FIELD(emitted_factor_sites);
  FIELD(max_factor_size); FIELD(leaf_pair_rectangles); FIELD(max_stack_size); FIELD(max_product_depth);
#undef FIELD
  array("size_class_rectangles", w.size_class_rectangles); array("size_class_pair_mass", w.size_class_pair_mass);
  array("rejected_pair_mass", w.rejected_pair_mass); array("residual_pair_mass", w.residual_pair_mass);
  array("lane_rectangles", w.lane_rectangles); array("callback_lane_mass", lane_mass);
  std::cout << ",\"index_point_visits\":" << index_work.point_visits << ",\"index_max_depth\":" << index_work.max_depth
            << ",\"cloud_retained_bytes\":" << cloud_bytes << ",\"index_retained_bytes\":" << index_bytes
            << ",\"load_ms\":" << ms(started, loaded) << ",\"cloud_ms\":" << ms(loaded, prepared)
            << ",\"index_ms\":" << ms(prepared, indexed) << ",\"front_callback_ms\":" << ms(indexed, front_done)
            << ",\"validation_ms\":" << ms(front_done, validated) << ",\"destruction_ms\":" << ms(validated, finished)
            << ",\"total_ms\":" << ms(started, finished) << "}\n";
  return 0;
}
}  // namespace
int main(int argc, char** argv) {
  try { return run(argc, argv); }
  catch (const std::exception& error) { std::cerr << "front audit failed: " << error.what() << '\n'; return 1; }
}
