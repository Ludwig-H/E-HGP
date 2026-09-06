// Private single-rectangle experiment. No full WSPD/front/completion/FULL run.
// Reuses the already qualified fixed-anchor prototype, unchanged.
#include <array>
#include <chrono>
#include <cstdio>
#include <exception>
#include <string_view>
#include <vector>
#include "src/core/sha256.hpp"
#include "src/pipeline/generate.hpp"
#include "../v7_wspd_histogram_blocks_20260906/histogram_blocks.hpp"

#ifdef MHGP7_TESTING
#error Nominal product only
#endif

namespace {
using namespace mhgp7;
namespace blocks = histogram_blocks_private;
using Clock = std::chrono::steady_clock;
struct Failure { const char* why; };
void need(bool ok, const char* why) { if (!ok) throw Failure{why}; }
unsigned long long num(u64 value) { return static_cast<unsigned long long>(value); }
double elapsed(Clock::time_point start) {
  return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}
void print_work(const blocks::Work& w) {
  std::printf("{\"logical_pairs\":%llu,\"scalar_tests\":%llu,\"node_visits\":%llu,"
              "\"hmax_tests\":%llu,\"hmin_tests\":%llu,\"ximax_tests\":%llu,"
              "\"credited_blocks\":%llu,\"credited_positions\":%llu,\"hmax_rejected_pairs\":%llu,"
              "\"scalar_true\":%llu,\"block_factors\":%llu}",
              num(w.logical_pairs), num(w.scalar_tests), num(w.node_visits), num(w.hmax_tests),
              num(w.hmin_tests), num(w.ximax_tests), num(w.credited_blocks), num(w.credited_positions),
              num(w.hmax_rejected_pairs), num(w.scalar_true), num(w.block_factors));
}
int run(u64 n) {
  const u64 m = n / 2;
  std::vector<InputPoint> input;
  input.reserve(static_cast<std::size_t>(n));
  for (u64 side = 0; side < 2; ++side)
    for (u64 j = 0; j < m; ++j) {
      const P3 p{static_cast<i64>(side * 64512 + (j % 32) * 32),
                 static_cast<i64>(((j / 32) % 32) * 32), static_cast<i64>((j / 1024) * 32)};
      input.push_back({static_cast<PointId>(side * m + j), p});
    }
  const auto ix = build_cloud_index(input);
  need(ix.valid && !ix.has_duplicate_positions() && ix.upos.size() == n && !ix.nodes.empty(), "input.index");
  const auto& root = ix.nodes[static_cast<std::size_t>(ix.root())];
  const WspdRect rectangle{root.left, root.right};
  const auto a = ix.range_of(rectangle.a), b = ix.range_of(rectangle.b);
  need(a.first == 0 && a.last == static_cast<i32>(m - 1) && b.first == static_cast<i32>(m)
       && b.last == static_cast<i32>(n - 1), "root.original_factors_partition_X");
  const AxisBox box_a = ix.box_of(rectangle.a), box_b = ix.box_of(rectangle.b);
  need(box_a.lo[0] == 0 && box_a.hi[0] <= 1024 && box_b.lo[0] == 64512 && box_b.hi[0] <= 65535,
       "root.two_disjoint_cubes");
  for (unsigned axis = 0; axis < 3; ++axis)
    need(box_a.hi[axis] - box_a.lo[axis] <= 1024 && box_b.hi[axis] - box_b.lo[axis] <= 1024,
         "root.cube_side");
  for (i64 s : {8, 10, 12}) need(wspd_detail::separated(box_a, box_b, s, 1), "root.separated_s8_s10_s12");
  // The core counts only X\(A union B), which is empty, independently of s.
  const u64 h[3]{10, 9, 8};
  const auto core = count_universal_witnesses(ix, rectangle.a, rectangle.b, h, 7, true);
  need(core.c[0] == 0 && core.c[1] == 0 && core.c[2] == 0, "root.empty_external_core");
  Sha256 input_hash;
  input_hash.tag("mhgp7-private-two-cubes-input-v1");
  for (const auto& p : input) {
    input_hash.u64le(p.id); input_hash.i64le(p.position.x);
    input_hash.i64le(p.position.y); input_hash.i64le(p.position.z);
  }
  std::printf("{\"event\":\"config\",\"schema\":\"mhgp7-private-large-factor-histograms-v1\","
              "\"public_status\":\"not_claimed\",\"n\":%llu,\"factor_size\":%llu,"
              "\"shape\":\"two_lattice_cubes_spacing32_offset64512\",\"separated_s\":[8,10,12],"
              "\"core\":[0,0,0],\"core_nodes\":%llu,\"threads\":1,\"input_digest\":\"%s\","
              "\"timed_scope\":\"one_instrumented_histogram_call\","
              "\"s_not_a_histogram_parameter\":true,\"full_front_run\":false}\n",
              num(n), num(m), num(core.nodes_visited), input_hash.hex().c_str());
  std::fflush(stdout);
  const u64 logical = 2 * m * (m - 1);
  std::array<std::array<double, 3>, 3> durations{};
  u64 compared = 0;
  Sha256 output_hash;
  output_hash.tag("mhgp7-private-two-cubes-histograms-v1"); output_hash.u64le(n);
  for (unsigned q = 0; q < 3; ++q) {
    const Lane lane = q == 0 ? Lane::kQ2 : q == 1 ? Lane::kQ3 : Lane::kQ4;
    std::vector<u64> expected_a, expected_b, ha, hb;
    expected_a.reserve(m); expected_b.reserve(m); ha.reserve(m); hb.reserve(m);
    for (unsigned arm = 0; arm < 3; ++arm) {
      auto& result_a = arm == 0 ? expected_a : ha;
      auto& result_b = arm == 0 ? expected_b : hb;
      blocks::Work work;
      const auto start = Clock::now();
      if (arm == 0) generate_detail::corner_histograms(ix, lane, rectangle, &result_a, &result_b);
      else work = blocks::corner_histograms(ix, lane, rectangle, result_a, result_b, arm == 1 ? 0 : 8);
      durations[q][arm] = elapsed(start);
      need(result_a.size() == m && result_b.size() == m, "histogram.size");
      if (arm == 0) {
        work.logical_pairs = logical; work.scalar_tests = logical; work.scalar_factors = 2;
        output_hash.u64le(q + 2);
        for (u64 value : result_a) { output_hash.u64le(value); work.scalar_true += value; }
        for (u64 value : result_b) { output_hash.u64le(value); work.scalar_true += value; }
      } else {
        need(result_a == expected_a && result_b == expected_b, "histogram.literal_difference");
        need(work.block_factors == 2 && work.node_visits > 0, "histogram.block_path_nonvacuous");
        compared += n;
      }
      need(work.logical_pairs == logical && logical == work.scalar_tests + work.credited_positions
           + work.hmax_rejected_pairs, "histogram.work_partition");
      std::printf("{\"event\":\"histogram\",\"q\":%u,\"arm\":\"%s\",\"elapsed_ms\":%.6f,\"work\":",
                  q + 2, arm == 0 ? "scalar" : arm == 1 ? "block0" : "hybrid8", durations[q][arm]);
      print_work(work); std::printf("}\n"); std::fflush(stdout);
    }
  }
  need(compared == 6 * n, "histogram.comparisons_nonvacuous");
  std::printf("{\"event\":\"terminal\",\"status\":\"completed\",\"public_status\":\"not_claimed\","
              "\"all_histograms_literal_equal\":true,\"compared_values\":%llu,"
              "\"P_factor_logical\":%llu,\"reference_digest\":\"%s\",\"total_ms\":[%.6f,%.6f,%.6f]}\n",
              num(compared), num(3 * logical), output_hash.hex().c_str(),
              durations[0][0] + durations[1][0] + durations[2][0],
              durations[0][1] + durations[1][1] + durations[2][1],
              durations[0][2] + durations[1][2] + durations[2][2]);
  return 0;
}
}  // namespace
int main(int argc, char** argv) {
  if (argc != 2) return 2;
  const std::string_view option(argv[1]);
  const u64 n = option == "--n=8000" ? 8000 : option == "--n=16000" ? 16000 : option == "--n=32000" ? 32000 : 0;
  if (n == 0) return 2;
  try { return run(n); }
  catch (const Failure& error) { std::fprintf(stderr, "large-factor histogram rejected: %s\n", error.why); }
  catch (const std::exception& error) { std::fprintf(stderr, "large-factor histogram exception: %s\n", error.what()); }
  return 1;
}
