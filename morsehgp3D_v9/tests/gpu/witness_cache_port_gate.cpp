// Host/device cache arithmetic judged here against the generator, its
// dynamic trace, and a pointwise exact witness count. No GPU claim.
#include "../../src/gpu/flat_index.hpp"
#include "../../src/gpu/witness_cache.hpp"
#include "../../src/gen/lanes/q34_witness_search.hpp"
#include "../gen/front_fixtures.hpp"

#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace mhgp9;
struct Counts {
  std::uint64_t queries = 0, traces = 0, cache_tests = 0, full = 0, partial = 0, open = 0;
  std::uint64_t baseline_visits = 0, variant_visits = 0, contacts = 0, checks = 0;
};
void need(bool value, const char* cause) {
  if (!value) throw std::runtime_error(cause);
}

gpu::u8 pointwise(const std::vector<gen::Point3>& points, gen::Point3 a, gen::Point3 b,
                  unsigned k, gpu::u8 mask, Counts& totals) {
  unsigned count[2] = {0, 0};
  for (const auto& z : points) {
    gpu::i64 u[3], v[3], h = 0;
    for (int axis = 0; axis < 3; ++axis) {
      u[axis] = static_cast<gpu::i64>(z[axis]) - a[axis];
      v[axis] = static_cast<gpu::i64>(b[axis]) - z[axis];
      h += u[axis] * v[axis];
    }
    if (h == 0) ++totals.contacts;
    if (h <= 0) continue;
    gpu::i128 xi = 0;
    for (int axis = 0; axis < 3; ++axis) {
      const int j = (axis + 1) % 3, l = (axis + 2) % 3;
      const gpu::i64 cross = u[j] * v[l] - u[l] * v[j];
      xi += static_cast<gpu::i128>(cross) * cross;
    }
    for (unsigned lane = 0; lane < 2; ++lane) {
      const gpu::i128 lhs = static_cast<gpu::i128>(3 - lane) * h * h;
      if (lhs == xi) ++totals.contacts;
      if (lhs > xi) ++count[lane];
    }
  }
  gpu::u8 result = mask;
  if (count[0] >= k - 1) result = static_cast<gpu::u8>(result & ~2U);
  if (count[1] >= k - 2) result = static_cast<gpu::u8>(result & ~4U);
  return result;
}

void endpoint_fixture(Counts& totals) {
  const std::vector<gen::Point3> points{{0,0,0},{10,0,0},{1,0,0},{5,0,0},{6,0,0}};
  const auto index = gen::make_q2_cloud_index(gen::prepare_cloud(points));
  const auto nodes = gpu::flatten_nodes(*index);
  const auto a = gpu::flat_point(points[0]), first = gpu::flat_point(points[1]), next = gpu::flat_point(points[2]);
  gpu::FixedWitnessTrace trace{};
  std::uint64_t visits = 0, tests = 0;
  need(gpu::filter<true,true>(nodes.data(), a, first, 3, 6, visits, &trace) == 0 && trace.size != 0,
       "cause=witness_cache.first_nonvacuous");
  need(gpu::cached_witness_rejections(nodes.data(), a, first, 3, 6, trace, tests) == 6,
       "cause=witness_cache.self_trace");
  auto rejected = gpu::cached_witness_rejections(nodes.data(), a, next, 3, 6, trace, tests);
#ifdef MHGP9_WITNESS_CACHE_MUTANT_NO_RETEST
  rejected = 6;  // Wrong reuse of the representative's rejected lanes.
#endif
  need(rejected == 0, "cause=witness_cache.endpoint_retest");
  need(gpu::filter<true>(nodes.data(), a, next, 3, 6, visits) == 6,
       "cause=witness_cache.endpoint_reference");
  const auto count = static_cast<gpu::u32>(nodes.size());
  need(gpu::validate_witness_trace(nodes.data(), count, trace), "cause=witness_cache.trace_valid");
  auto invalid = trace;
  invalid.nodes[invalid.size] = invalid.nodes[0];
  invalid.lanes[invalid.size++] = invalid.lanes[0];
  need(!gpu::validate_witness_trace(nodes.data(), count, invalid), "cause=witness_cache.overlap_refused");
  invalid = trace; invalid.nodes[0] = count;
  need(!gpu::validate_witness_trace(nodes.data(), count, invalid), "cause=witness_cache.index_refused");
  invalid = trace; invalid.lanes[0] = 8;
  need(!gpu::validate_witness_trace(nodes.data(), count, invalid), "cause=witness_cache.lanes_refused");
  invalid = trace; invalid.size = 18;
  need(!gpu::validate_witness_trace(nodes.data(), count, invalid), "cause=witness_cache.capacity_refused");
  invalid = trace; invalid.lanes[0] = 0;
  need(!gpu::validate_witness_trace(nodes.data(), count, invalid), "cause=witness_cache.empty_lane_refused");
  // Same node on DISJOINT lanes is sound; validation must not refuse it.
  invalid.size = 2; invalid.nodes[0] = invalid.nodes[1] = trace.nodes[0];
  invalid.lanes[0] = 2; invalid.lanes[1] = 4;
  need(gpu::validate_witness_trace(nodes.data(), count, invalid), "cause=witness_cache.disjoint_lanes");
  gpu::FixedWitnessTrace empty{};
  std::uint64_t empty_tests = 0, base_visits = 0, fallback_visits = 0;
  for (std::size_t b = 1; b < points.size(); ++b) {
    const auto fb = gpu::flat_point(points[b]);
    const auto dead = gpu::cached_witness_rejections(nodes.data(), a, fb, 3, 6, empty, empty_tests);
    const auto reference = gpu::filter<true>(nodes.data(), a, fb, 3, 6, base_visits);
    const auto observed = gpu::filter<true>(nodes.data(), a, fb, 3, static_cast<gpu::u8>(6 & ~dead), fallback_visits);
    need(dead == 0 && observed == reference, "cause=witness_cache.empty_object");
  }
  need(empty_tests == 0 && fallback_visits == base_visits, "cause=witness_cache.empty_work");
  need(gpu::filter<true,true>(nodes.data(), a, first, 3, 6, visits, nullptr) == gpu::stack_failure,
       "cause=witness_cache.null_trace");
  trace.size = 1;
  need(gpu::filter<true,true>(nodes.data(), a, first, 1, 6, visits, &trace) == 0 && trace.size == 0,
       "cause=witness_cache.inactive_trace_reset");
  need(gpu::filter<true,true>(nodes.data(), a, first, 2, 6, visits, &trace) == 0 && trace.size == 1 && trace.lanes[0] == 2,
       "cause=witness_cache.k2_trace");
  const std::vector<gen::Point3> tangent{{0,0,0},{2,2,4},{2,0,2}};
  const auto tangent_index = gen::make_q2_cloud_index(gen::prepare_cloud(tangent));
  const auto tangent_nodes = gpu::flatten_nodes(*tangent_index);
  const auto ta = gpu::flat_point(tangent[0]), tb = gpu::flat_point(tangent[1]);
  need(gpu::filter<true,true>(tangent_nodes.data(), ta, tb, 2, 2, visits, &trace) == 2 && trace.size == 0,
       "cause=witness_cache.strict_q3_tangent");
  bool found_tangent = false;
  for (std::size_t i = 0; i < tangent_nodes.size(); ++i) {
    const auto& node = tangent_nodes[i];
    if (node.left != gpu::absent32 || node.box.low[0] != 2 || node.box.low[1] != 0 || node.box.low[2] != 2) continue;
    found_tangent = true;
    trace.size = 1; trace.nodes[0] = static_cast<gpu::u32>(i); trace.lanes[0] = 2;
    need(gpu::cached_witness_rejections(tangent_nodes.data(), ta, tb, 2, 2, trace, tests) == 0,
         "cause=witness_cache.strict_cached_q3_tangent");
  }
  need(found_tangent, "cause=witness_cache.tangent_fixture_nonvacuous");
  totals.checks += 22;  // Includes the four empty-cache query comparisons.
}

void compare_cloud(const std::vector<gen::Point3>& points, Counts& totals) {
  const auto index = gen::make_q2_cloud_index(gen::prepare_cloud(points));
  const auto nodes = gpu::flatten_nodes(*index);
  const std::size_t anchors = std::min<std::size_t>(16, points.size());
  for (unsigned k = 3; k <= 10; ++k)
    for (std::size_t a = 0; a < anchors; ++a)
      for (std::size_t start = 0; start < points.size(); start += 32) {
        const std::size_t end = std::min(start + 32, points.size());
        const std::size_t representative = start == a ? start + 1 : start;
        if (representative == end) continue;
        const auto fa = gpu::flat_point(points[a]), fb = gpu::flat_point(points[representative]);
        gen::Q34WitnessSearchWork work;
        gen::Q34WitnessBoundsWork bounds;
        std::vector<gen::Q34WitnessNode> product_trace;
        const auto expected = gen::filter_q34_witnesses(*index, points[a], points[representative],
            static_cast<std::uint8_t>(k), 6, work, bounds, product_trace);
        gpu::FixedWitnessTrace trace{};
        std::uint64_t trace_visits = 0, default_visits = 0;
        const auto observed = gpu::filter<true,true>(nodes.data(), fa, fb, k, 6, trace_visits, &trace);
        const auto old = gpu::filter<true>(nodes.data(), fa, fb, k, 6, default_visits);
        need(observed == expected && observed == old && trace_visits == work.node_visits && trace_visits == default_visits,
             "cause=witness_cache.producer_mask_visits");
        need(trace.size == product_trace.size() && trace.size <= 2 * k - 3,
             "cause=witness_cache.producer_size");
        need(gpu::validate_witness_trace(nodes.data(), static_cast<gpu::u32>(nodes.size()), trace),
             "cause=witness_cache.producer_antichain");
        for (unsigned i = 0; i < trace.size; ++i)
          need(trace.nodes[i] == product_trace[i].node && trace.lanes[i] == product_trace[i].lanes,
               "cause=witness_cache.producer_trace");
        ++totals.traces;
        totals.variant_visits += trace_visits;
        for (std::size_t b = start; b < end; ++b) {
          if (a == b) continue;
          const auto target = gpu::flat_point(points[b]);
          const auto reference = gpu::filter<true>(nodes.data(), fa, target, k, 6, totals.baseline_visits);
          need(reference == pointwise(points, points[a], points[b], k, 6, totals), "cause=witness_cache.pointwise");
          ++totals.queries;
          if (b == representative) continue;
          const std::uint64_t before = totals.cache_tests;
          const auto dead = gpu::cached_witness_rejections(nodes.data(), fa, target, k, 6, trace, totals.cache_tests);
          gen::Q34WitnessCacheWork cached;
          const auto product = gen::q34_cached_witness_rejections(*index, points[a], points[b],
              static_cast<std::uint8_t>(k), 6, product_trace, cached);
          need(dead == product && totals.cache_tests - before == cached.node_tests, "cause=witness_cache.product_cache");
          need((dead & reference) == 0, "cause=witness_cache.unsound_dead_lane");
          const auto remainder = static_cast<gpu::u8>(6 & ~dead);
          const auto final = gpu::filter<true>(nodes.data(), fa, target, k, remainder, totals.variant_visits);
          need(final == reference, "cause=witness_cache.fallback_mask");
          if (dead == 6) ++totals.full;
          else if (dead != 0) ++totals.partial;
          else ++totals.open;
          // Also exercise independent lane requests and inactive requests.
          for (const gpu::u8 subset : {gpu::u8{0}, gpu::u8{2}, gpu::u8{4}}) {
            std::uint64_t ignored = 0;
            const auto sub = gpu::cached_witness_rejections(nodes.data(), fa, target, k, subset, trace, ignored);
            need(sub == (dead & subset), "cause=witness_cache.lane_subset");
          }
        }
      }
}
}  // namespace

int main(int argc, char**) {
  if (argc != 1) return 2;
  try {
    Counts totals;
    endpoint_fixture(totals);
    for (const std::string family : {"uniform", "terrain", "clusters"}) {
      auto fixture = mhgp9::gen::bench::make_front_fixture(128, family, 3);
      compare_cloud(fixture.points, totals);
      std::reverse(fixture.points.begin(), fixture.points.end());
      compare_cloud(fixture.points, totals);
    }
    // Exact H=0 endpoints and q3 tangency: a=(0,0,0), b=(2,2,4),
    // z=(2,0,2), H=4 and Xi=48=3*H^2. Also exercise the u18 extremes.
    const std::vector<mhgp9::gen::Point3> edge_cases{{0,0,0},{2,2,4},{2,0,2},
      {262143,262143,262143},{262143,0,0},{0,262143,0},{0,0,262143},
      {131071,131072,131071},{1,1,1},{1,0,0},{5,0,0},{6,0,0},{10,0,0}};
    compare_cloud(edge_cases, totals);
    need(totals.queries > 90000 && totals.traces > 3000 && totals.full > 100 && totals.partial > 100 &&
         totals.open > 100 && totals.contacts > 1000, "cause=witness_cache.floor");
    std::printf("witness_cache_port_gate queries=%llu traces=%llu full=%llu partial=%llu open=%llu "
                "contacts=%llu checks=%llu baseline_visits=%llu representative_and_fallback_visits=%llu cache_tests=%llu\n",
      static_cast<unsigned long long>(totals.queries), static_cast<unsigned long long>(totals.traces),
      static_cast<unsigned long long>(totals.full), static_cast<unsigned long long>(totals.partial),
      static_cast<unsigned long long>(totals.open), static_cast<unsigned long long>(totals.contacts),
      static_cast<unsigned long long>(totals.checks), static_cast<unsigned long long>(totals.baseline_visits),
      static_cast<unsigned long long>(totals.variant_visits), static_cast<unsigned long long>(totals.cache_tests));
    return 0;
  } catch (const std::exception& e) {
    std::printf("%s\n", e.what());
    return 1;
  }
}
