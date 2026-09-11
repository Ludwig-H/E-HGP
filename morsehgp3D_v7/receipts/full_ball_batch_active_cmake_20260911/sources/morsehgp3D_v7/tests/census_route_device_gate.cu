// Device gate for the reusable route. A test-only allocation exception is
// compiled here, never in the product wrapper. The host stub may execute this
// same file, but it explicitly reports stub-hote and no device qualification.
#ifndef MHGP7_TESTING
#define MHGP7_TESTING 1
#endif

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/gpu/census_route.cuh"
#include "../src/lanes/q2.hpp"
#include "../src/pipeline/generate.hpp"

using namespace mhgp7;

#if !defined(__CUDACC__) && !defined(MHGP7_FAKE_DEVICE)
int main() { std::fprintf(stderr, "REFUS : no CUDA compiler\n"); return 2; }
#else

namespace {
u64 checks = 0, failures = 0, compared = 0, extra_shells = 0, rejected = 0;
u64 arities[5] = {};

void expect(bool value, const char* reason) {
  ++checks;
  if (!value) { ++failures; std::printf("failure=%s\n", reason); }
}

std::vector<i32> sorted(std::span<const i32> ids) {
  std::vector<i32> result(ids.begin(), ids.end());
  std::sort(result.begin(), result.end());
  return result;
}

void compare(const std::vector<Survivor>& expected, const std::vector<BallData>& expected_balls,
             const std::vector<Survivor>& actual, const std::vector<BallData>& actual_balls) {
  expect(expected.size() == actual.size() && expected_balls.size() == actual_balls.size(), "sizes");
  if (expected.size() != actual.size() || expected_balls.size() != actual_balls.size()) return;
  for (size_t i = 0; i < expected.size(); ++i) {
    const auto& a = expected_balls[i];
    const auto& b = actual_balls[i];
    expect(expected[i].idx == actual[i].idx && expected[i].depth == actual[i].depth, "survivor");
    expect(a.key == b.key && a.level == b.level && a.arity == b.arity, "candidate_identity");
    // Membership is judged explicitly independent of traversal order; the
    // existing kernel gate additionally judges bit-identical DFS lists.
    expect(sorted(a.interior()) == sorted(b.interior()), "strict_interior");
    expect(sorted(a.shell()) == sorted(b.shell()), "closed_shell");
    ++compared; ++arities[b.arity];
    if (b.n_shell > b.arity) ++extra_shells;
  }
}

bool exercise(const CloudIndex& ix, const std::vector<BallCandidate>& candidates, u64 smax) {
  std::vector<Survivor> expected;
  std::vector<BallData> expected_balls;
  ExpandStats reference;
  prefilter_balls(ix, candidates, smax, 1, &expected, &reference);
  if (census_balls(ix, candidates, expected, smax, 12, 1, &expected_balls, &reference) !=
      PipelineStatus::kCompleteRegular) return false;
  for (const size_t lot : {size_t{1}, size_t{17}, candidates.size() + 1}) {
    gpu::CensusRouteContext context;
    gpu::CensusRouteCosts costs;
    const std::string prepare = context.prepare(ix, lot, &costs);
    expect(prepare.empty() && context.ready(), "prepare");
    if (!prepare.empty()) { std::printf("reason=%s\n", prepare.c_str()); return false; }
    expect(context.index_digest() == gpu::build_index_wire(ix).host_wire_digest, "index_snapshot_digest");
    std::vector<Survivor> actual{{0, 999}};
    std::vector<BallData> actual_balls(1);
    ExpandStats stats;
    const std::string error = context.run(candidates, smax, 12, &actual, &actual_balls, &stats, &costs);
    expect(error.empty(), "run");
    if (!error.empty()) { std::printf("reason=%s\n", error.c_str()); return false; }
    compare(expected, expected_balls, actual, actual_balls);
    expect(stats.dead_depth == reference.dead_depth && stats.survivors == reference.survivors &&
           stats.census_interior == reference.census_interior && stats.census_shell == reference.census_shell,
           "stats");
    expect(costs.h2d_ball_bytes == candidates.size() * 112 &&
           costs.h2d_sentinel_bytes == candidates.size() * 100 &&
           costs.d2h_bytes == candidates.size() * 100, "physical_bytes");
    expect(costs.lots_launched == (candidates.size() + lot - 1) / lot &&
           costs.lots_reconstructed == costs.lots_launched, "lot_counts");
    const u64 transfers = costs.h2d_ball_bytes + costs.h2d_sentinel_bytes + costs.d2h_bytes;
    expect(context.run({}, smax, 12, &actual, &actual_balls, &stats, &costs).empty(), "empty_after_full");
    expect(actual.empty() && actual_balls.empty() && stats.survivors == 0 && stats.dead_depth == 0,
           "empty_clears_outputs");
    expect(transfers == costs.h2d_ball_bytes + costs.h2d_sentinel_bytes + costs.d2h_bytes,
           "empty_no_candidate_transfers");
    expect(context.close(&costs).empty() && !context.ready(), "close");
    expect(costs.freed_bytes == costs.allocated_bytes && costs.device_resident_bytes == 0 &&
           costs.failed_cuda_frees == 0, "release_all");
  }
  return true;
}

void refusals(const CloudIndex& ix, const std::vector<BallCandidate>& candidates) {
  expect(candidates.size() > 17, "fault_nonvacuum");
  if (candidates.size() <= 17) return;
  for (u64 ordinal = 1; ordinal <= 15; ++ordinal) {
    gpu::CensusRouteContext context;
    gpu::CensusRouteCosts costs;
    context.test_fail_allocation_at(ordinal);
    const std::string error = context.prepare(ix, 17, &costs);
    expect(error == "test : CUDA allocation failure", "allocation_fault_rejected");
    expect(!context.ready() && costs.allocations == ordinal - 1 &&
           costs.freed_bytes == costs.allocated_bytes && costs.device_resident_bytes == 0 &&
           costs.failed_cuda_frees == 0, "allocation_fault_cleanup");
    ++rejected;
  }
  for (const bool late_bad_arity : {true, false}) {
    gpu::CensusRouteContext context;
    gpu::CensusRouteCosts costs;
    expect(context.prepare(ix, 17, &costs).empty(), "refusal_prepare");
    std::vector<BallCandidate> bad = candidates;
    if (late_bad_arity) bad[17].arity = 0;
    else context.test_kernel_mutant(gpu::kMutSkipBallWrite);
    std::vector<Survivor> survivors{{0, 999}};
    std::vector<BallData> balls(1);
    ExpandStats stats;
    stats.dead_depth = 71; stats.survivors = 72; stats.census_interior = 73; stats.census_shell = 74;
    const std::string error = context.run(bad, 11, 12, &survivors, &balls, &stats, &costs);
    expect(!error.empty() && !context.ready(), "refusal_invalidates_context");
    expect(survivors.empty() && balls.empty(), "no_partial_publication");
    expect(stats.dead_depth == 71 && stats.survivors == 72 && stats.census_interior == 73 &&
           stats.census_shell == 74, "refusal_stats_unchanged");
    expect(costs.allocated_bytes == costs.freed_bytes && costs.failed_cuda_frees == 0, "refusal_cleanup");
    if (late_bad_arity) expect(costs.lots_reconstructed == 1, "late_refusal_after_private_prefix");
    else expect(error.find("ecriture device omise") != std::string::npos, "sentinel_refusal_reason");
    ++rejected;
  }
  gpu::CensusRouteCosts costs;
  std::vector<Survivor> survivors{{0, 999}};
  std::vector<BallData> balls(1);
  ExpandStats stats;
  expect(gpu::device_prefilter_census_route(ix, {}, 11, 12, &survivors, &balls, &stats, 17, &costs).empty(),
         "cold_empty");
  expect(survivors.empty() && balls.empty() && costs.allocated_bytes == costs.freed_bytes, "cold_empty_cleanup");
  {
    gpu::CensusRouteContext context;
    expect(context.prepare(ix, 17).empty(), "prepare_without_sink");
    gpu::CensusRouteCosts fresh;
    expect(context.close(&fresh).empty() && fresh.freed_bytes > 0 &&
           fresh.device_resident_bytes == 0 && fresh.failed_cuda_frees == 0,
           "new_release_sink_no_underflow");
  }
  {
    gpu::CensusRouteContext context;
    gpu::CensusRouteCosts prepare_costs, fresh;
    expect(context.prepare(ix, 17, &prepare_costs).empty(), "prepare_other_sink");
    expect(context.run({}, 11, 12, &survivors, &balls, &stats, &fresh).empty(), "run_other_sink");
    expect(fresh.device_resident_bytes == prepare_costs.allocated_bytes, "new_run_sink_owned_bytes");
    expect(context.close(&fresh).empty() && fresh.freed_bytes == prepare_costs.allocated_bytes &&
           fresh.device_resident_bytes == 0, "other_sink_closed");
  }
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string(argv[1]) != "--selftest") return 2;
  int count = 0;
  cudaDeviceProp prop{};
  if (cudaGetDeviceCount(&count) != cudaSuccess || count == 0 ||
      cudaGetDeviceProperties(&prop, 0) != cudaSuccess) return 2;
#if defined(MHGP7_FAKE_DEVICE)
  const char* backend = "stub-hote";
#else
  const char* backend = "CUDA";
  if (prop.major != 12 || prop.minor != 0) return 2;
#endif
  std::printf("backend=%s device=%s sm=%d.%d\n", backend, prop.name, prop.major, prop.minor);
  for (const int n : {8, 24}) {
    const auto input = make_family_input(CloudFamily::kUniform, n, 65536, 3);
    const CloudIndex ix = build_cloud_index(input);
    GenerateOptions options;
    options.s = 8; options.smax = 11; options.threads = 1;
    std::vector<BallCandidate> candidates;
    GenerateStats stats;
    generate_candidates(ix, options, &candidates, &stats);
    if (stats.cap_refus != kCapRefusNone) return 2;
    rle_candidates(&candidates, 1);
    expect(!candidates.empty(), "generated_nonvacuum");
    if (!exercise(ix, candidates, 11)) return 1;
    if (n == 24) refusals(ix, candidates);
  }
  // Deliberately non-Morton external IDs. The two diagonals have the same
  // positive ball with four shell sites and minimal support cardinality two.
  const std::vector<InputPoint> square{{91, {0, 0, 0}}, {7, {2, 0, 0}},
                                       {42, {2, 2, 0}}, {3, {0, 2, 0}}};
  const CloudIndex square_ix = build_cloud_index(square);
  std::vector<BallCandidate> square_candidates;
  for (size_t a = 0; a < square.size(); ++a)
    for (size_t b = a + 1; b < square.size(); ++b) {
      const P3 x = square[a].position, y = square[b].position;
      square_candidates.push_back({q2_ball_key(x, y), promote_level(q2_exact_level(p3_norm2(p3_sub(x, y)))), 2});
    }
  rle_candidates(&square_candidates, 1);
  if (!exercise(square_ix, square_candidates, 4)) return 1;
  expect(compared > 100 && extra_shells >= 3 && arities[2] > 0 && arities[3] > 0 && arities[4] > 0,
         "coverage_nonvacuum");
  expect(rejected == 17, "rejections_nonvacuum");
  std::printf("census_route_gate=%s backend=%s checks=%llu compared=%llu extra_shells=%llu "
              "q2=%llu q3=%llu q4=%llu rejections=%llu failures=%llu\n",
              failures == 0 ? "passed" : "failed", backend,
              static_cast<unsigned long long>(checks), static_cast<unsigned long long>(compared),
              static_cast<unsigned long long>(extra_shells), static_cast<unsigned long long>(arities[2]),
              static_cast<unsigned long long>(arities[3]), static_cast<unsigned long long>(arities[4]),
              static_cast<unsigned long long>(rejected), static_cast<unsigned long long>(failures));
  return failures == 0 ? 0 : 1;
}
#endif
