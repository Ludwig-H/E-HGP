// Standalone overlay qualification. OBig enumeration, no shared argmin/division.
#include <algorithm>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

#include "../src/pipeline/census.hpp"
#include "../oracle/obig.hpp"

namespace {
using namespace mhgp7;
using Big = mhgp7_oracle::OBig<8>;
u64 boxes_seen = 0, axis_points = 0, volume_points = 0, census_queries = 0;
u64 extended_profile_boxes = 0;
int max_bits = 0;

Big axis_value(const BallKey& k, int axis, i64 t) {
  const Big x = Big::from_i64(t);
  return Big::from_i128(k.a) * x * x + Big::from_i128(k.b[axis]) * x;
}
Big point_value(const BallKey& k, i64 x, i64 y, i64 z) {
  return Big::from_i128(k.c) + axis_value(k, 0, x) + axis_value(k, 1, y) + axis_value(k, 2, z);
}

// This is a FIXTURE domain check, not a new product validation API. Invalid
// polynomial/box inputs never reach AxisBounds (which has preconditions).
bool fixture_in_profile(const BallKey& k, const AxisBox& b) {
  if (k.a <= 0 || k.a >= ((i128)1 << 68)) return false;
  if (k.c <= -((i128)1 << 105) || k.c >= ((i128)1 << 105)) return false;
  for (int i = 0; i < 3; ++i) {
    if (k.b[i] <= -((i128)1 << 87) || k.b[i] >= ((i128)1 << 87)) return false;
    if (b.lo[i] < 0 || b.hi[i] > kCoordMax || b.lo[i] > b.hi[i]) return false;
  }
  return true;
}

bool check_box(const BallKey& k, const AxisBox& b, bool full_volume = false) {
  if (!fixture_in_profile(k, b)) return false;
  bool beyond_old_b = false;
  for (int i = 0; i < 3; ++i)
    beyond_old_b = beyond_old_b || k.b[i] <= -((i128)1 << 86) || k.b[i] >= ((i128)1 << 86);
  const bool beyond_old_c = k.c <= -((i128)1 << 104) || k.c >= ((i128)1 << 104);
  if (beyond_old_b && beyond_old_c) ++extended_profile_boxes;
  Big mn = Big::from_i128(k.c), mx = mn;
  for (int i = 0; i < 3; ++i) {
    Big low = axis_value(k, i, b.lo[i]), high = low;
    for (i64 t = b.lo[i]; t <= b.hi[i]; ++t) {
      const Big v = axis_value(k, i, t);
      low = std::min(low, v);
      high = std::max(high, v);
      ++axis_points;
    }
    mn = mn + low;
    mx = mx + high;
  }
  max_bits = std::max({max_bits, mn.bit_length(), mx.bit_length()});
  if (full_volume) {
    Big vmin = point_value(k, b.lo[0], b.lo[1], b.lo[2]), vmax = vmin;
    for (i64 x = b.lo[0]; x <= b.hi[0]; ++x)
      for (i64 y = b.lo[1]; y <= b.hi[1]; ++y)
        for (i64 z = b.lo[2]; z <= b.hi[2]; ++z) {
          const Big v = point_value(k, x, y, z);
          vmin = std::min(vmin, v);
          vmax = std::max(vmax, v);
          ++volume_points;
        }
    if (mn != vmin || mx != vmax) return false;
  }
  i128 actual_min = 0, actual_max = 0;
  census_detail::AxisBounds(k).bounds(b, &actual_min, &actual_max);
  ++boxes_seen;
  if (Big::from_i128(actual_min) != mn || Big::from_i128(actual_max) != mx) {
    std::printf("DIVERGENCE axis_bounds box=%llu min_equal=%d max_equal=%d\n",
                (unsigned long long)boxes_seen, Big::from_i128(actual_min) == mn,
                Big::from_i128(actual_max) == mx);
    return false;
  }
  return !mhgp7_oracle::overflow_seen();
}

bool check_census() {
  std::vector<P3> points;
  for (i64 x = 0; x <= 8; ++x)
    for (i64 y = 0; y <= 8; ++y)
      for (i64 z = 0; z <= 8; ++z) points.push_back({x, y, z});
  const CloudIndex index = build_cloud_index(points);
  if (!index.valid || index.has_duplicate_positions()) return false;
  std::vector<NodeRef> scratch;
  for (i64 radius_squared : {0, 1, 2, 5, 9, 16, 25, 48, 49}) {
    const BallKey k{1, {-8, -8, -8}, 48 - radius_squared};
    std::vector<i32> expected_interior, expected_shell, interior, shell;
    for (i32 i = 0; i < index.unique_count(); ++i) {
      const P3& p = index.upos[(size_t)i];
      const Big v = point_value(k, p.x, p.y, p.z);
      if (v < Big{}) expected_interior.push_back(i);
      else if (v == Big{}) expected_shell.push_back(i);
    }
    if (ball_census(index, k, points.size(), points.size(), &interior, &shell, nullptr, &scratch)
        != CensusStatus::kOk) return false;
    std::sort(interior.begin(), interior.end());
    std::sort(shell.begin(), shell.end());
    if (interior != expected_interior || shell != expected_shell) return false;
    for (u64 threshold : {1ull, 2ull, 10ull, 729ull, 730ull}) {
      u64 count = 0;
      const bool got = ball_depth_at_least(index, k, threshold, &count, nullptr, &scratch);
      if (got != (expected_interior.size() >= threshold)) return false;
      if (!got && count != expected_interior.size()) return false;
      ++census_queries;
    }
    if (!expected_interior.empty() && ball_census(index, k, expected_interior.size() - 1,
        points.size(), &interior, &shell) != CensusStatus::kInteriorOverflow) return false;
    if (!expected_shell.empty() && ball_census(index, k, points.size(), expected_shell.size() - 1,
        &interior, &shell) != CensusStatus::kShellOverflow) return false;
  }
  const std::vector<P3> invalid_points{{-1, 0, 0}, {0, 0, 0}};
  return !build_cloud_index(invalid_points).valid;
}

bool run_gate() {
  const AxisBox small{{0, 0, 0}, {8, 8, 8}};
  // r>A, r<A, r=A, integer centre; all axes and clamping boundaries.
  for (i128 b : {-15, -13, -14, -16, 7, -31})
    if (!check_box(BallKey{2, {b, b + 1, b - 1}, 43}, small, true)) return false;
  if (!check_box(BallKey{2, {-15, -15, -15}, 0}, AxisBox{{6, 0, 2}, {8, 1, 3}}, true)) return false;
  const i128 wide_a = ((i128)1 << 67) + 17;
  if (!check_box(BallKey{wide_a, {-7 * wide_a, -9 * wide_a, -11 * wide_a}, -((i128)1 << 103)}, small, true)) return false;
  // Near profile limits, with centres far beyond u16 or i64. No distant
  // polynomial is evaluated; the gate's enumeration stays in u16.
  const AxisBox entire{{0, 0, 0}, {65535, 65535, 65535}};
  if (!check_box(BallKey{1, {-(((i128)1 << 87) - 1), ((i128)1 << 87) - 1, 0},
                        ((i128)1 << 105) - 1}, entire)) return false;
  if (!check_box(BallKey{((i128)1 << 68) - 1,
                        {-(((i128)1 << 87) - 1), ((i128)1 << 87) - 1, -2},
                        -(((i128)1 << 105) - 1)}, entire)) return false;
  // The historical keys.hpp limits themselves are VALID, not rejections,
  // in the conservative S1 domain. Exercise both signs at these cutoffs.
  if (!check_box(BallKey{wide_a, {(i128)1 << 86, -((i128)1 << 86), 0},
                        (i128)1 << 104}, small, true)) return false;
  if (!check_box(BallKey{wide_a, {-((i128)1 << 86), (i128)1 << 86, 0},
                        -((i128)1 << 104)}, small, true)) return false;
  std::mt19937_64 random(0xa815b0);
  for (int n = 0; n < 1200; ++n) {
    BallKey k{(i128)(1 + random() % 10000), {}, (i128)(random() % 1000000)};
    AxisBox box{};
    for (int i = 0; i < 3; ++i) {
      box.lo[i] = (i64)(random() % 65536);
      box.hi[i] = std::min((i64)65535, box.lo[i] + (i64)(random() % 33));
      const i64 centre = box.lo[i] + (i64)(random() % 65) - 16;
      k.b[i] = -2 * k.a * centre + (i128)(random() % (u64)(2 * k.a));
    }
    if (!check_box(k, box, n < 8)) return false;
  }
  BallKey invalid{1, {0, 0, 0}, 0};
  AxisBox invalid_box = small;
  int rejected = 0;
  invalid.a = 0; rejected += !fixture_in_profile(invalid, small);
  invalid.a = -1; rejected += !fixture_in_profile(invalid, small);
  invalid.a = (i128)1 << 68; rejected += !fixture_in_profile(invalid, small);
  invalid.a = 1; invalid.b[1] = (i128)1 << 87; rejected += !fixture_in_profile(invalid, small);
  invalid.b[1] = -((i128)1 << 87); rejected += !fixture_in_profile(invalid, small);
  invalid.b[1] = 0; invalid.c = -((i128)1 << 105); rejected += !fixture_in_profile(invalid, small);
  invalid.c = (i128)1 << 105; rejected += !fixture_in_profile(invalid, small);
  invalid.c = 0; invalid_box.lo[0] = -1; rejected += !fixture_in_profile(invalid, invalid_box);
  invalid_box = small; invalid_box.hi[1] = 65536; rejected += !fixture_in_profile(invalid, invalid_box);
  invalid_box = small; invalid_box.lo[2] = 9; rejected += !fixture_in_profile(invalid, invalid_box);
  if (rejected != 10 || !check_census()) return false;
  return boxes_seen >= 1212 && axis_points >= 400000 && volume_points >= 5000
         && census_queries == 45 && max_bits >= 106 && extended_profile_boxes >= 4
         && !mhgp7_oracle::overflow_seen();
}
}  // namespace

int main(int argc, char** argv) {
  if (argc > 2 || (argc == 2 && !mhgp7::mutants_enable(argv[1]))) return 2;
  mhgp7_oracle::clear_overflow();
  const bool ok = run_gate();
  if (mhgp7_oracle::overflow_seen()) return 3;
  if (!ok) return mhgp7::mutants_any() ? 4 : 1;
  if (mhgp7::mutants_any()) {
    std::puts("SURVIVED axis_bounds mutant");
    return 1;
  }
  std::printf("OK axis_bounds boxes=%llu axis_points=%llu volume_points=%llu census_queries=%llu max_bits=%d rejected_fixtures=10 extended_profile_boxes=%llu\n",
              (unsigned long long)boxes_seen, (unsigned long long)axis_points,
              (unsigned long long)volume_points, (unsigned long long)census_queries, max_bits,
              (unsigned long long)extended_profile_boxes);
  return 0;
}
