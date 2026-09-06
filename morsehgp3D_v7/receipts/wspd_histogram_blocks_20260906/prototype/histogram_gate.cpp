// Private bounded histogram comparison; no timing or FULL authority.
// Scalar product reference: generate.hpp 345129a775d430a40e151d3b1adb5cd9efeaf77a6ffb6713bd081c74d40bdd9c.
// Independent point predicate is written below with i128 components; it does
// not call product spindle/H/cross helpers. Continuous authority is the audit.
#include <algorithm>
#include <array>
#include <cstdio>
#include <limits>
#include <string_view>
#include <vector>
#include "src/pipeline/generate.hpp"
#include "histogram_blocks.hpp"

#ifdef MHGP7_TESTING
#error This private gate must exercise nominal source
#endif

namespace {
using namespace mhgp7;
namespace hb = mhgp7::histogram_blocks_private;
struct Failure { const char* why; };
void need(bool ok, const char* why) { if (!ok) throw Failure{why}; }
unsigned long long number(u64 x) { return static_cast<unsigned long long>(x); }

bool independent_inside(Lane lane, P3 a, P3 b, P3 z) {
  const std::array<i128, 3> d{b.x - a.x, b.y - a.y, b.z - a.z};
  const std::array<i128, 3> w{z.x - a.x, z.y - a.y, z.z - a.z};
  i128 h = 0, xi = 0;
  for (unsigned i = 0; i < 3; ++i) h += w[i] * (d[i] - w[i]);
  if (h <= 0) return false;
  if (lane == Lane::kQ2) return true;
  for (unsigned i = 0; i < 3; ++i) {
    const unsigned j = (i + 1) % 3, k = (i + 2) % 3;
    const i128 c = d[j] * w[k] - d[k] * w[j];
    xi += c * c;
  }
  return (lane == Lane::kQ3 ? 3 : 2) * h * h > xi;
}
bool independent_universal(Lane lane, P3 a, AxisBox t, P3 z) {
  for (unsigned bits = 0; bits < 8; ++bits) {
    const P3 b{bits & 1 ? t.hi[0] : t.lo[0], bits & 2 ? t.hi[1] : t.lo[1],
               bits & 4 ? t.hi[2] : t.lo[2]};
    if (!independent_inside(lane, a, b, z)) return false;
  }
  return true;
}
std::vector<u64> independent_histogram(const CloudIndex& ix, Lane lane, NodeRef factor, NodeRef other) {
  const auto range = ix.range_of(factor);
  std::vector<u64> result(static_cast<std::size_t>(range.last - range.first + 1));
  for (i32 a = range.first; a <= range.last; ++a)
    for (i32 z = range.first; z <= range.last; ++z)
      if (a != z && independent_universal(lane, ix.upos[static_cast<std::size_t>(a)], ix.box_of(other),
                                         ix.upos[static_cast<std::size_t>(z)]))
        ++result[static_cast<std::size_t>(a - range.first)];
  return result;
}

struct Fixture { std::vector<InputPoint> points; std::size_t factor_size; };
Fixture fixture(unsigned id) {
  std::vector<P3> a, b;
  if (id == 0) { a = {{0, 0, 0}}; b = {{20, 0, 0}}; }
  if (id == 1) { a = {{0, 0, 0}, {1, 1, 0}, {4, 0, 1}, {5, 1, 1}}; b = {{100, 0, 0}}; }
  if (id == 2) { a = {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}}; b = {{100, 0, 0}}; }
  if (id == 3) { a = {{0, 0, 0}, {3, 3, 0}, {4, 0, 0}}; b = {{100, 100, 0}}; }
  if (id == 4) {
    for (i64 x = 0; x < 32; ++x) a.push_back({x, x % 2, (x / 2) % 2});
    b = {{1000, 0, 0}, {1001, 0, 0}};
  }
  if (id == 5) { a = {{0, 0, 0}, {0, 1, 0}, {0, 2, 0}}; b = {{100, 0, 0}}; }
  if (id == 6) { a = {{65500, 65500, 65500}, {65501, 65501, 65500}}; b = {{0, 0, 0}}; }
  Fixture result{{}, a.size()};
  for (P3 p : a) result.points.push_back({static_cast<PointId>(result.points.size()), p});
  for (P3 p : b) result.points.push_back({static_cast<PointId>(result.points.size()), p});
  return result;
}
NodeRef factor(const CloudIndex& ix, PointId first, PointId last) {
  for (i32 v = -ix.unique_count(); v < static_cast<i32>(ix.nodes.size()); ++v) {
    const auto range = ix.range_of(v);
    if (range.last - range.first + 1 != static_cast<i32>(last - first)) continue;
    bool good = true;
    for (i32 u = range.first; u <= range.last; ++u)
      good = good && first <= ix.point_id(u) && ix.point_id(u) < last;
    if (good) return v;
  }
  throw Failure{"fixture.original_factor_not_radix_node"};
}

void certificate_controls() {
  for (Lane q : kLanes) {
    hb::Work w;
    need(!hb::certifies(q, {1, 1, 1}, hb::point_box({20, 1, 1}), hb::point_box({1, 1, 1}), w),
         "certificate.diagonal");
    need(!hb::certifies(q, {0, 0, 0}, hb::point_box({10, 0, 0}), hb::point_box({11, 0, 0}), w),
         "certificate.negative_H");
    need(hb::certifies(q, {0, 0, 0}, hb::point_box({65535, 65535, 65535}),
                       hb::point_box({32767, 32767, 32767}), w), "certificate.i128_square");
  }
  hb::Work w;
  need(!hb::certifies(Lane::kQ2, {0, 0, 0}, hb::point_box({2, 0, 0}), hb::point_box({1, 1, 0}), w),
       "certificate.q2_shell");
  need(!hb::certifies(Lane::kQ3, {0, 1, 1}, hb::point_box({2, 0, 0}), hb::point_box({1, 1, 0}), w),
       "certificate.q3_strict_boundary");
  need(!hb::certifies(Lane::kQ4, {0, 0, 0}, hb::point_box({2, 2, 2}), hb::point_box({1, 1, 0}), w),
       "certificate.q4_strict_boundary");
  const AxisBox anchors{{0, 0, 0}, {3, 3, 0}}, t = hb::point_box({100, 100, 0});
  need(hmax4_boxes(anchors, t, hb::point_box({4, 0, 0})) == -816, "hmax.variable_anchor_counterexample");
  for (Lane q : {Lane::kQ3, Lane::kQ4})
    need(independent_universal(q, {0, 0, 0}, t, {4, 0, 0}), "hmax.good_anchor_must_survive");
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") return 2;
  try {
    certificate_controls();
    u64 comparisons = 0, baselines = 0, separations = 0, logical = 0, scalar = 0, nodes = 0;
    u64 credits = 0, rejects = 0, diagonal_rejects = 0, default_walks = 0, default_scalars = 0;
    std::array<u64, 3> credit_by_lane{};
    for (unsigned scene = 0; scene < 7; ++scene) {
      auto data = fixture(scene);
      for (unsigned permutation = 0; permutation < 2; ++permutation) {
        if (permutation) std::reverse(data.points.begin(), data.points.end());
        const auto ix = build_cloud_index(data.points);
        need(ix.valid && !ix.has_duplicate_positions(), "fixture.index");
        const WspdRect r{factor(ix, 0, static_cast<PointId>(data.factor_size)),
                         factor(ix, static_cast<PointId>(data.factor_size), static_cast<PointId>(data.points.size()))};
        for (i64 s : {8, 10, 12}) {
          need(wspd_detail::separated(ix.box_of(r.a), ix.box_of(r.b), s, 1), "fixture.separation");
          ++separations;
        }
        if (scene == 5) {
          const i32 anchor = ix.range_of(r.a).first;
          hb::Work w;
          need(hb::fixed_anchor(ix, Lane::kQ3, r.a, ix.box_of(r.b), anchor, w) == 0
               && w.hmax_rejected_pairs == data.factor_size - 1 && w.node_visits == 1,
               "hmax.rejected_original_factor_excludes_diagonal");
          ++diagonal_rejects;
        }
        for (Lane lane : kLanes) {
          std::vector<u64> old_a, old_b;
          generate_detail::corner_histograms(ix, lane, r, &old_a, &old_b);
          need(old_a == independent_histogram(ix, lane, r.a, r.b)
               && old_b == independent_histogram(ix, lane, r.b, r.a), "independent.scalar_reference");
          ++baselines;
          for (std::size_t cutoff : {std::size_t{0}, std::size_t{8}, std::numeric_limits<std::size_t>::max()}) {
            std::vector<u64> a, b;
            const hb::Work w = hb::corner_histograms(ix, lane, r, a, b, cutoff);
            need(a == old_a && b == old_b, "histogram.literal_differential");
            need(w.logical_pairs == w.credited_positions + w.hmax_rejected_pairs + w.scalar_tests,
                 "work.logical_pair_partition");
            if (cutoff == std::numeric_limits<std::size_t>::max())
              need(w.node_visits == 0 && w.scalar_tests == w.logical_pairs, "dispatch.scalar_has_no_walk");
            if (cutoff == 8) { default_walks += w.block_factors; default_scalars += w.scalar_factors; }
            if (cutoff == 0) {
              logical += w.logical_pairs; scalar += w.scalar_tests; nodes += w.node_visits;
              credits += w.credited_positions; rejects += w.hmax_rejected_pairs;
              credit_by_lane[static_cast<std::size_t>(lane_index(lane))] += w.credited_blocks;
            }
            ++comparisons;
          }
        }
      }
    }
    need(comparisons == 126 && baselines == 42 && separations == 42 && diagonal_rejects == 2,
         "nonvacuity.inventory");
    need(credit_by_lane[1] > 0 && credit_by_lane[2] > 0 && credits >= 4 && rejects >= 2
         && scalar < logical && default_walks > 0 && default_scalars > 0, "nonvacuity.block_and_fallback");
    std::printf("{\"schema\":\"mhgp7-private-histogram-blocks-v1\",\"status\":\"passed\","
                "\"public_status\":\"not_claimed\",\"timing_claim\":false,\"comparisons\":%llu,"
                "\"independent_baselines\":%llu,\"separation_checks\":%llu,\"diagonal_block_rejections\":%llu,"
                "\"forced_logical_pairs\":%llu,\"forced_scalar_tests\":%llu,\"forced_node_visits\":%llu,"
                "\"forced_credited_positions\":%llu,\"forced_hmax_rejected_pairs\":%llu,"
                "\"credited_blocks_q2_q3_q4\":[%llu,%llu,%llu]}\n", number(comparisons), number(baselines),
                number(separations), number(diagonal_rejects), number(logical), number(scalar), number(nodes),
                number(credits), number(rejects), number(credit_by_lane[0]), number(credit_by_lane[1]), number(credit_by_lane[2]));
    return 0;
  } catch (const Failure& e) {
    std::fprintf(stderr, "histogram blocks rejected: %s\n", e.why);
  } catch (const std::exception& e) {
    std::fprintf(stderr, "histogram blocks exception: %s\n", e.what());
  }
  return 1;
}
