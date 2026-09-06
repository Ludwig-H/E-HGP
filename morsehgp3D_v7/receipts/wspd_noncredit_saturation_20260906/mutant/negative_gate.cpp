// Independent integer predicates and bounded differential checks. No timings.
#include <algorithm>
#include <array>
#include <cstdio>
#include <exception>
#include <limits>
#include <string_view>
#include <vector>
#include "src/pipeline/generate.hpp"
#include "histogram_negative.hpp"
#ifdef MHGP7_TESTING
#error Nominal product only
#endif
namespace {
using namespace mhgp7;
namespace neg = histogram_negative_private;
namespace pos = histogram_blocks_private;
struct Failure { const char* why; };
void need(bool ok, const char* why) { if (!ok) throw Failure{why}; }
struct Values { i128 h, xi; };
Values exact(const P3& a, const P3& b, const P3& z) {
  const std::array<i128, 3> d{b.x - a.x, b.y - a.y, b.z - a.z};
  const std::array<i128, 3> v{z.x - a.x, z.y - a.y, z.z - a.z};
  Values out{};
  for (unsigned i = 0; i < 3; ++i) {
    out.h += v[i] * (d[i] - v[i]);
    const i128 cross = d[(i + 1) % 3] * v[(i + 2) % 3] - d[(i + 2) % 3] * v[(i + 1) % 3];
    out.xi += cross * cross;
  }
  return out;
}
bool inside(Lane lane, const P3& a, const P3& b, const P3& z) {
  const auto v = exact(a, b, z);
  return v.h > 0 && (lane == Lane::kQ2 || (lane == Lane::kQ3 ? 3 : 2) * v.h * v.h > v.xi);
}
bool universal(Lane lane, const P3& a, const AxisBox& b, const P3& z) {
  for (unsigned mask = 0; mask < 8; ++mask) {
    const P3 corner{mask & 1 ? b.hi[0] : b.lo[0], mask & 2 ? b.hi[1] : b.lo[1],
                    mask & 4 ? b.hi[2] : b.lo[2]};
    if (!inside(lane, a, corner, z)) return false;
  }
  return true;
}
std::vector<u64> independent_histogram(const CloudIndex& ix, Lane lane, NodeRef factor, NodeRef other) {
  const auto range = ix.range_of(factor);
  std::vector<u64> result(static_cast<std::size_t>(range.last - range.first + 1), 0);
  for (i32 a = range.first; a <= range.last; ++a)
    for (i32 z = range.first; z <= range.last; ++z)
      if (z != a && universal(lane, ix.upos[static_cast<std::size_t>(a)], ix.box_of(other),
                              ix.upos[static_cast<std::size_t>(z)]))
        ++result[static_cast<std::size_t>(a - range.first)];
  return result;
}
void partition(const neg::Work& w) {
  need(w.base.logical_pairs == w.base.scalar_tests + w.base.credited_positions + w.base.hmax_rejected_pairs
       + w.negative_rejected_pairs + w.saturation_unvisited_pairs, "work.full_logical_partition");
}
void primitive() {
  const P3 a{0, 0, 0}, b{100, 0, 0};
  const AxisBox outside{{1, 4, 0}, {2, 5, 1}};
  need(hmax4_boxes(pos::point_box(a), pos::point_box(b), outside) == 720
       && neg::cross_lower(a, b, outside) == 160000, "audit.bounds_literals");
  for (Lane lane : {Lane::kQ3, Lane::kQ4}) {
    neg::Work w;
    need(neg::rejects_fixed_pair(lane, a, b, outside, w) && w.angular_rejected_nodes == 1,
         "audit.angular_rejection_nonvacuous");
    for (i64 x = 1; x <= 2; ++x) for (i64 y = 4; y <= 5; ++y) for (i64 z = 0; z <= 1; ++z) {
      need(inside(Lane::kQ2, a, b, {x, y, z}) && !inside(lane, a, b, {x, y, z}), "audit.independent_W2_not_W34");
    }
    // Contains a true witness. Replacing Xi_min by Xi_max falsely rejects it.
    const AxisBox mixed{{1, 0, 0}, {2, 5, 1}};
    const i128 m = hmax4_boxes(pos::point_box(a), pos::point_box(b), mixed);
    need(inside(lane, a, b, {1, 0, 0}) && !neg::rejects_fixed_pair(lane, a, b, mixed, w),
         "negative.true_witness_preserved");
    need((lane == Lane::kQ3 ? 3 : 2) * m * m <= 16 * pos::cross_upper(a, pos::point_box(b), mixed),
         "mutant.Xi_upper_causal_false_reject");
    const P3 huge_b{65535, 65535, 65535}, huge_z{32767, 32767, 32767};
    const i128 huge_m = hmax4_boxes(pos::point_box(a), pos::point_box(huge_b), pos::point_box(huge_z));
    need(huge_m * huge_m > static_cast<i128>(std::numeric_limits<u64>::max())
         && inside(lane, a, huge_b, huge_z)
         && !neg::rejects_fixed_pair(lane, a, huge_b, pos::point_box(huge_z), w), "negative.i128_large_square");
    need(neg::rejects_fixed_pair(lane, a, b, pos::point_box(a), w), "negative.diagonal_zero");
  }
  for (Lane lane : {Lane::kQ3, Lane::kQ4}) {
    const P3 sa = lane == Lane::kQ3 ? P3{0, 1, 1} : P3{0, 0, 0};
    const P3 sb = lane == Lane::kQ3 ? P3{2, 0, 0} : P3{2, 2, 2}, sz{1, 1, 0};
    const auto v = exact(sa, sb, sz);
    neg::Work w;
    need(v.h > 0 && (lane == Lane::kQ3 ? 3 : 2) * v.h * v.h == v.xi
         && neg::rejects_fixed_pair(lane, sa, sb, pos::point_box(sz), w), "negative.strict_equality_rejects");
  }
}
std::vector<InputPoint> cloud(unsigned scene) {
  std::vector<P3> p{{0, 0, 0}};
  if (scene == 0 || scene == 3)
    for (i64 x = 1; x <= 2; ++x) for (i64 y = 4; y <= 5; ++y) for (i64 z = 0; z <= 1; ++z)
      p.push_back({x, y, z});
  if (scene == 1) for (i64 x = 1; x < 9; ++x) p.push_back({x, 0, 0});
  if (scene == 2) for (i64 y = 1; y < 9; ++y) p.push_back({0, y, 0});
  p.push_back({100, 0, 0});
  if (scene == 3) p.push_back({102, 2, 2});
  std::vector<InputPoint> input;
  for (std::size_t i = 0; i < p.size(); ++i) input.push_back({static_cast<PointId>(i), p[i]});
  return input;
}
int run() {
  primitive();
  u64 comparisons = 0, negative_pairs = 0, skipped_pairs = 0, clipped_blocks = 0, non_site_lanes = 0;
  for (unsigned scene = 0; scene < 4; ++scene) for (unsigned reverse = 0; reverse < 2; ++reverse) {
    auto input = cloud(scene);
    if (reverse) std::reverse(input.begin(), input.end());
    const auto ix = build_cloud_index(input);
    need(ix.valid && !ix.has_duplicate_positions() && !ix.nodes.empty(), "fixture.index");
    const auto root = ix.nodes[static_cast<std::size_t>(ix.root())];
    const WspdRect r{root.left, root.right};
    need(wspd_detail::separated(ix.box_of(r.a), ix.box_of(r.b), 8, 1), "fixture.s8_separation");
    if (scene == 3) {
      const AxisBox box = ix.box_of(r.b);
      const P3 center = neg::box_representative(box);
      need(center.x == 101 && center.y == 1 && center.z == 1, "non_site.literal_center");
      const std::array<i64, 3> value{center.x, center.y, center.z};
      for (unsigned axis = 0; axis < 3; ++axis)
        need(box.lo[axis] < value[axis] && value[axis] < box.hi[axis], "non_site.strict_interior");
      for (const auto& p : input)
        need(p.position.x != center.x || p.position.y != center.y || p.position.z != center.z,
             "non_site.absent_from_input");
    }
    for (Lane lane : {Lane::kQ2, Lane::kQ3, Lane::kQ4}) {
      u64 lane_negative_pairs = 0;
      std::vector<u64> old_a, old_b;
      generate_detail::corner_histograms(ix, lane, r, &old_a, &old_b);
      need(old_a == independent_histogram(ix, lane, r.a, r.b)
           && old_b == independent_histogram(ix, lane, r.b, r.a), "independent.histogram_reference");
      for (u64 threshold : {u64{0}, u64{1}, u64{2}, u64{8}, u64{10}, std::numeric_limits<u64>::max()})
        for (std::size_t cutoff : {std::size_t{0}, std::size_t{8}, std::numeric_limits<std::size_t>::max()}) {
          auto expected_a = old_a, expected_b = old_b;
          for (auto& value : expected_a) value = std::min(value, threshold);
          for (auto& value : expected_b) value = std::min(value, threshold);
          std::vector<u64> ha, hb;
          const auto w = neg::corner_histograms(ix, lane, r, ha, hb, cutoff, threshold);
          need(ha == expected_a && hb == expected_b, "histogram.clipped_literal_difference");
          partition(w);
          if (threshold == std::numeric_limits<u64>::max())
            need(w.saturated_anchors == 0 && w.saturation_unvisited_pairs == 0 && w.clipped_credit_blocks == 0,
                 "saturation.MAX_is_exhaustive");
          if (threshold == 0)
            need(w.base.scalar_tests == 0 && w.base.node_visits == 0
                 && w.saturation_unvisited_pairs == w.base.logical_pairs, "saturation.zero_no_work");
          if (lane == Lane::kQ2) need(w.negative_tests == 0 && w.negative_rejected_pairs == 0, "negative.q2_unchanged");
          if (cutoff == std::numeric_limits<std::size_t>::max())
            need(w.base.node_visits == 0, "saturation.scalar_fallback_no_walk");
          negative_pairs += w.negative_rejected_pairs;
          lane_negative_pairs += w.negative_rejected_pairs;
          skipped_pairs += w.saturation_unvisited_pairs;
          clipped_blocks += w.clipped_credit_blocks;
          ++comparisons;
        }
      if (scene == 3 && lane != Lane::kQ2) {
        need(lane_negative_pairs > 0, "non_site.q34_negative_nonvacuous");
        ++non_site_lanes;
      }
    }
  }
  need(comparisons == 432 && non_site_lanes == 4 && negative_pairs > 0 && skipped_pairs > 0 && clipped_blocks > 0,
       "nonvacuity.negative_and_saturation");
  std::printf("{\"schema\":\"mhgp7-private-negative-histogram-gate-v1\",\"status\":\"passed\","
              "\"public_status\":\"not_claimed\",\"comparisons\":%llu,\"negative_pairs\":%llu,"
              "\"saturation_unvisited_pairs\":%llu,\"clipped_credit_blocks\":%llu,\"non_site_lanes\":%llu}\n",
              static_cast<unsigned long long>(comparisons), static_cast<unsigned long long>(negative_pairs),
              static_cast<unsigned long long>(skipped_pairs), static_cast<unsigned long long>(clipped_blocks),
              static_cast<unsigned long long>(non_site_lanes));
  return 0;
}
}  // namespace
int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") return 2;
  try { return run(); }
  catch (const Failure& e) { std::fprintf(stderr, "negative histogram rejected: %s\n", e.why); }
  catch (const std::exception& e) { std::fprintf(stderr, "negative histogram exception: %s\n", e.what()); }
  return 1;
}
