// Bounded independent audit of the actual v7 spindle helpers.
// 0: normal success; 1: check failed; 2: bad CLI; 3: mutant survived;
// 4: a named, real MHGP7_TESTING injection was detected causally.
#include <algorithm>
#include <array>
#include <cfenv>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "../src/spindle/witness_count.hpp"

using namespace mhgp7;

namespace {

struct Counts {
  u64 roots = 0, spindle = 0, h_zero = 0, q3_equal = 0, q4_equal = 0;
  u64 wide_xi = 0, cores = 0, positive_cores = 0, empty_cores = 0;
  u64 boxes = 0, corners = 0, masks = 0, collections = 0, contacts = 0, orientations = 0;
} counts;

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

// Independent bisection, without sqrt, product isqrt, or floating conversion.
i64 root_ref(i128 value) {
  require(value >= 0 && value <= std::numeric_limits<i64>::max(), "oracle root domain");
  i64 lo = 0, hi = i64{1} << 32;
  while (hi - lo > 1) {
    const i64 middle = lo + (hi - lo) / 2;
    if (static_cast<i128>(middle) * middle <= value) lo = middle;
    else hi = middle;
  }
  return lo;
}

i64 ceil_root_ref(i128 value) {
  const i64 root = root_ref(value);
  return root + (static_cast<i128>(root) * root != value);
}

std::array<i64, 3> coords(const P3& point) { return {point.x, point.y, point.z}; }

i128 distance_ref(const P3& first, const P3& second) {
  const auto a = coords(first), b = coords(second);
  i128 result = 0;
  for (int axis = 0; axis < 3; ++axis) {
    const i128 delta = static_cast<i128>(a[axis]) - b[axis];
    result += delta * delta;
  }
  return result;
}

struct PowerRef { i128 h, xi; };

PowerRef power_ref(const P3& a, const P3& b, const P3& z) {
  const i128 ab = distance_ref(a, b), az = distance_ref(a, z), bz = distance_ref(b, z);
  require((ab - az - bz) % 2 == 0, "distance identity parity");
  const i128 h = (ab - az - bz) / 2;
  const i128 dot = (ab + az - bz) / 2;
  // Gram determinant: no product cross/dot/norm helper is called by this judge.
  return {h, ab * az - dot * dot};
}

bool spindle_ref(Lane q, const P3& a, const P3& b, const P3& z) {
  const auto value = power_ref(a, b, z);
  if (value.h <= 0) return false;
  if (q == Lane::kQ2) return true;
  return (q == Lane::kQ3 ? 3 : 2) * value.h * value.h > value.xi;
}

AxisBox singleton(const P3& point) {
  return {{point.x, point.y, point.z}, {point.x, point.y, point.z}};
}

std::vector<P3> corners_ref(const AxisBox& box) {
  std::vector<P3> result;
  for (i64 x : {box.lo[0], box.hi[0]})
    for (i64 y : {box.lo[1], box.hi[1]})
      for (i64 z : {box.lo[2], box.hi[2]}) {
        const P3 p{x, y, z};
        if (std::find(result.begin(), result.end(), p) == result.end()) result.push_back(p);
      }
  return result;
}

CoreBall core_ref(Lane q, const AxisBox& a, const AxisBox& b) {
  // Independent fixed-contract constants, checked against the actual declarations.
  const i128 scale_a = i128{1} << 30, scale_c = i128{1} << 20;
  const i128 aq = q == Lane::kQ2 ? scale_a : q == Lane::kQ3 ? 619000000 : 555000000;
  const i128 cq = q == Lane::kQ2 ? 2097152 : q == Lane::kQ3 ? 1398102 : 1329545;
  i128 distance = 0, diam_a = 0, diam_b = 0;
  CoreBall result{};
  for (int axis = 0; axis < 3; ++axis) {
    result.center4[axis] = a.lo[axis] + b.lo[axis] + a.hi[axis] + b.hi[axis];
    const i128 delta = static_cast<i128>(a.lo[axis]) + a.hi[axis] - b.lo[axis] - b.hi[axis];
    const i128 wa = static_cast<i128>(a.hi[axis]) - a.lo[axis];
    const i128 wb = static_cast<i128>(b.hi[axis]) - b.lo[axis];
    distance += delta * delta;
    diam_a += wa * wa;
    diam_b += wb * wb;
  }
  const i128 distance_lower = root_ref(distance);
  const i128 ra = ceil_root_ref(diam_a), rb = ceil_root_ref(diam_b), radii = ra + rb;
  i128 first = 0;
  if (distance_lower > radii) first = aq * (distance_lower - radii) / scale_a - radii;
  const i128 numerator = 2 * cq * (ra * ra + rb * rb);
  const i128 rounded = numerator / scale_c + (numerator % scale_c != 0);
  const i128 second = aq * distance_lower / scale_a - ceil_root_ref(rounded);
  result.radius4 = static_cast<i64>(std::max(i128{0}, std::max(first, second)));
  return result;
}

i128 ball_distance_ref(const P3& point, const CoreBall& ball) {
  i128 result = 0;
  const auto value = coords(point);
  for (int axis = 0; axis < 3; ++axis) {
    const i128 delta = i128{4} * value[axis] - ball.center4[axis];
    result += delta * delta;
  }
  return result;
}

bool point_ball_ref(const P3& point, const CoreBall& ball) {
  return ball_distance_ref(point, ball) < static_cast<i128>(ball.radius4) * ball.radius4;
}

int box_ball_ref(const AxisBox& box, const CoreBall& ball) {
  i128 far = 0, near = 0;
  for (const auto& point : corners_ref(box)) far = std::max(far, ball_distance_ref(point, ball));
  for (int axis = 0; axis < 3; ++axis) {
    const i128 center = ball.center4[axis];
    const i128 lo = i128{4} * box.lo[axis], hi = i128{4} * box.hi[axis];
    const i128 projection = std::max(lo, std::min(hi, center));
    near += (projection - center) * (projection - center);
  }
  const i128 radius = static_cast<i128>(ball.radius4) * ball.radius4;
  return far < radius ? 1 : near >= radius ? -1 : 0;
}

void root_cases() {
  require(std::fegetround() == FE_TONEAREST && std::numeric_limits<double>::is_iec559 &&
          std::numeric_limits<double>::digits == 53, "qualified binary64 RN domain");
  std::set<i64> values;
  for (i64 value = 0; value <= 4096; ++value) values.insert(value);
  for (i64 root : {7ll, 113509ll, 113510ll, 227019ll, 321054ll, 321055ll})
    for (i64 delta : std::array<i64, 4>{-1, 0, 1, 2 * root}) {
      const i64 value = root * root + delta;
      if (value >= 0 && value <= 103076160800ll) values.insert(value);
    }
  for (i64 value : {12884508675ll, 51538034700ll, 103076160798ll, 103076160799ll, 103076160800ll})
    values.insert(value);
  for (i64 value : values) {
    const double seed = std::sqrt(static_cast<double>(value));
    require(std::isfinite(seed) && seed >= 0 && seed < 321056, "finite convertible sqrt proposal");
    require(static_cast<i64>(static_cast<double>(value)) == value, "integer-to-double exactness");
    require(floor_sqrt(value) == root_ref(value), "floor_sqrt versus independent bisection");
    require(ceil_sqrt(value) == ceil_root_ref(value), "ceil_sqrt versus independent bisection");
    ++counts.roots;
  }
}

void check_spindle(const P3& a, const P3& b, const P3& z) {
  const auto ref = power_ref(a, b, z);
  require(h_point(a, b, z) == ref.h, "h_point versus distance identity");
  counts.h_zero += ref.h == 0;
  counts.q3_equal += ref.h > 0 && 3 * ref.h * ref.h == ref.xi;
  counts.q4_equal += ref.h > 0 && 2 * ref.h * ref.h == ref.xi;
  counts.wide_xi += ref.h > 0 && ref.xi > std::numeric_limits<i64>::max();
  for (Lane q : kLanes) {
    require(in_spindle(q, a, b, z) == spindle_ref(q, a, b, z), "spindle strict classification");
    ++counts.spindle;
  }
}

void local_geometry_cases() {
  const std::vector<P3> points{{0, 0, 0}, {65535, 65535, 65535}, {65535, 32767, 0},
      {0, 65535, 0}, {65535, 0, 65535}, {1, 1, 0}, {2, 1, 1}, {2, 2, 2},
      {2, 0, 0}, {1, 0, 0}, {0, 1, 0}, {32767, 32768, 32767}};
  for (const auto& a : points)
    for (const auto& b : points)
      for (const auto& z : points) check_spindle(a, b, z);
  const std::vector<AxisBox> boxes{
      {{0, 0, 0}, {0, 0, 0}}, {{65535, 65535, 65535}, {65535, 65535, 65535}},
      {{0, 0, 0}, {65535, 65535, 65535}}, {{1, 2, 3}, {4, 7, 10}},
      {{65530, 65531, 65532}, {65535, 65535, 65535}}, {{0, 0, 0}, {1, 1, 1}},
      {{32767, 32767, 32767}, {32768, 32768, 32768}}, {{10, 0, 0}, {10, 10, 0}},
      {{0, 0, 0}, {0, 65535, 65535}}, {{3, 4, 5}, {3, 4, 5}},
      {{30, 40, 50}, {31, 41, 51}}, {{0, 1, 0}, {1, 1, 0}}};
  require(kSpindleD == 1073741824 && kCoupE == 1048576 && kA2 == 1073741824 &&
          kA3 == 619000000 && kA4 == 555000000 && kC2 == 2097152 &&
          kC3 == 1398102 && kC4 == 1329545, "pinned D E and coefficient declarations");
  for (const auto& a : boxes) {
    const auto ca = corners_ref(a);
    for (const auto& b : boxes) {
      const auto cb = corners_ref(b);
      for (Lane q : kLanes) {
        const auto got = core_ball(q, a, b), expected = core_ref(q, a, b);
        const auto reverse = core_ball(q, b, a);
        require(got.radius4 == expected.radius4 && reverse.radius4 == expected.radius4,
                "core directed radius and endpoint reversal");
        for (int axis = 0; axis < 3; ++axis)
          require(got.center4[axis] == expected.center4[axis] && reverse.center4[axis] == expected.center4[axis],
                  "core exact quarter center");
        AxisBox reflected_a = a, reflected_b = b, rotated_a{}, rotated_b{};
        reflected_a.lo[0] = 65535 - a.hi[0];
        reflected_a.hi[0] = 65535 - a.lo[0];
        reflected_b.lo[0] = 65535 - b.hi[0];
        reflected_b.hi[0] = 65535 - b.lo[0];
        for (int axis = 0; axis < 3; ++axis) {
          rotated_a.lo[axis] = a.lo[(axis + 1) % 3];
          rotated_a.hi[axis] = a.hi[(axis + 1) % 3];
          rotated_b.lo[axis] = b.lo[(axis + 1) % 3];
          rotated_b.hi[axis] = b.hi[(axis + 1) % 3];
        }
        const auto reflected = core_ball(q, reflected_a, reflected_b);
        const auto rotated = core_ball(q, rotated_a, rotated_b);
        require(reflected.radius4 == got.radius4 && rotated.radius4 == got.radius4,
                "core radius axis reflection and cycle");
        for (int axis = 0; axis < 3; ++axis) {
          require(reflected.center4[axis] == (axis == 0 ? 262140 - got.center4[axis] : got.center4[axis]),
                  "core reflected quarter center");
          require(rotated.center4[axis] == got.center4[(axis + 1) % 3], "core rotated quarter center");
        }
        counts.orientations += 2;
        ++counts.cores;
        counts.positive_cores += got.radius4 > 0;
        counts.empty_cores += got.radius4 == 0;
        for (const auto& z : points) {
          require(point_in_ball(z, got) == point_ball_ref(z, expected), "point core comparison");
          if (point_ball_ref(z, expected))
            for (const auto& aa : ca)
              for (const auto& bb : cb)
                require(spindle_ref(q, aa, bb, z), "core inclusion independent strict spindle");
        }
        for (const auto& z : boxes) {
          require(box_vs_ball(z, got) == box_ball_ref(z, expected), "box core comparison");
          ++counts.boxes;
        }
      }
      for (const auto& z : points) {
        bool expected3 = true, expected4 = true;
        for (Lane q : kLanes) {
          bool expected = true;
          for (const auto& aa : ca)
            for (const auto& bb : cb) expected = expected && spindle_ref(q, aa, bb, z);
          require(corner64_universal(q, a, b, z) == expected, "distinct corner authority");
          if (q == Lane::kQ3) expected3 = expected;
          if (q == Lane::kQ4) expected4 = expected;
          ++counts.corners;
        }
        bool all3 = true, all4 = true;
        u64 evaluations = 0;
        corner64_universal_34(a, b, z, &all3, &all4, &evaluations);
        require(all3 == expected3 && all4 == expected4 && evaluations >= 1 && evaluations <= ca.size() * cb.size(),
                "fused corner authority and bounded evaluations");
      }
      const auto& zbox = boxes[(ca.size() + cb.size()) % boxes.size()];
      i128 minimum = static_cast<i128>(std::numeric_limits<i64>::max());
      for (const auto& aa : ca)
        for (const auto& bb : cb)
          for (const auto& zz : corners_ref(zbox)) minimum = std::min(minimum, power_ref(aa, bb, zz).h);
      require(hmin_boxes(a, b, zbox) == minimum, "Hmin versus complete corner product");
      i128 minimax4 = static_cast<i128>(std::numeric_limits<i64>::max());
      for (const auto& aa : ca)
        for (const auto& bb : cb) {
          const auto av = coords(aa), bv = coords(bb);
          i128 maximum4 = 0;
          for (int axis = 0; axis < 3; ++axis) {
            std::vector<i128> candidates{2 * zbox.lo[axis], 2 * zbox.hi[axis]};
            const i128 vertex = static_cast<i128>(av[axis]) + bv[axis];
            if (candidates[0] <= vertex && vertex <= candidates[1]) candidates.push_back(vertex);
            i128 best = -static_cast<i128>(std::numeric_limits<i64>::max());
            for (i128 twice_z : candidates)
              best = std::max(best, (twice_z - 2 * av[axis]) * (2 * bv[axis] - twice_z));
            maximum4 += best;
          }
          minimax4 = std::min(minimax4, maximum4);
        }
      require(hmax4_boxes(a, b, zbox) == minimax4, "Hmax4 versus endpoints and rational vertex");
    }
  }
  const CoreBall contact{{16, 16, 16}, 4};
  for (const auto& pair : std::vector<std::pair<P3, bool>>{{{4, 4, 4}, true}, {{5, 4, 4}, false}, {{6, 4, 4}, false}}) {
    require(point_in_ball(pair.first, contact) == pair.second, "explicit interior shell exterior");
    ++counts.contacts;
  }
  require(box_vs_ball({{4, 4, 4}, {5, 4, 4}}, contact) == 0, "mixed box includes exact shell");
  require(box_vs_ball({{5, 4, 4}, {6, 4, 4}}, contact) == -1, "open ball contact excluded");
  require(box_vs_ball({{4, 4, 4}, {4, 4, 4}}, contact) == 1, "strict interior singleton");
  counts.contacts += 3;
}

CloudIndex count_fixture() {
  return build_cloud_index(std::vector<InputPoint>{{91, {0, 0, 0}}, {7, {4, 0, 0}},
      {0xFFFFFFFFu, {5, 0, 0}}, {33, {6, 0, 0}}, {102, {10, 0, 0}}});
}

void count_cases() {
  const auto ix = count_fixture();
  require(ix.valid && ix.unique_count() == 5, "count fixture index");
  require(ix.upos[0] == P3{0, 0, 0} && ix.upos[4] == P3{10, 0, 0}, "anchor ranks");
  for (i32 a = 0; a < 5; ++a)
    for (i32 b = a + 1; b < 5; ++b) {
      const auto box_a = singleton(ix.upos[static_cast<size_t>(a)]);
      const auto box_b = singleton(ix.upos[static_cast<size_t>(b)]);
      for (u64 h : {0ull, 1ull, 2ull, 10ull}) {
        const u64 thresholds[3] = {h, h, h};
        for (u8 mask = 1; mask < 8; ++mask)
          for (bool with_corners : {false, true}) {
            const auto actual = count_universal_witnesses(ix, leaf_ref(a), leaf_ref(b), thresholds, mask, with_corners);
            for (int lane = 0; lane < 3; ++lane) {
              u64 expected = 0;
              const auto q = kLanes[lane];
              const auto core = core_ref(q, box_a, box_b);
              if (mask & (1u << lane))
                for (i32 point = 0; point < 5; ++point) {
                  if (point == a || point == b) continue;
                  const auto& z = ix.upos[static_cast<size_t>(point)];
                  expected += lane == 0 || with_corners ? spindle_ref(q, ix.upos[static_cast<size_t>(a)], ix.upos[static_cast<size_t>(b)], z)
                                                       : point_ball_ref(z, core);
                }
              require(actual.c[lane] == std::min(expected, h), "fused exact independent identity count");
            }
            require(actual.nodes_visited <= 9 && actual.corner_evals <= 5 * 64, "local traversal bounds");
            ++counts.masks;
          }
      }
      for (Lane q : kLanes) {
        std::set<i32> expected;
        for (i32 point = 0; point < 5; ++point)
          if (point != a && point != b && spindle_ref(q, ix.upos[static_cast<size_t>(a)], ix.upos[static_cast<size_t>(b)], ix.upos[static_cast<size_t>(point)]))
            expected.insert(point);
        require(true_spindle_count(q, ix, a, b, 10) == expected.size(), "true count without threshold truncation");
        for (u64 cap : {0ull, 1ull, 5ull}) {
          std::vector<i32> output(7, -99);
          const u64 written = collect_universal_ids(q, ix, leaf_ref(a), leaf_ref(b), cap, output.data() + 1);
          require(written == std::min<u64>(cap, expected.size()), "collect exact bounded cardinal");
          require(output[0] == -99 && output[static_cast<size_t>(written) + 1] == -99, "collection writable boundary");
          std::set<i32> actual;
          for (size_t i = 1; i <= written; ++i) require(expected.count(output[i]) && actual.insert(output[i]).second,
                                                      "collection identity unique and admissible");
          if (cap == 5) require(actual == expected, "collection complete identity set");
          ++counts.collections;
        }
      }
    }
  const u64 observed = true_spindle_count(Lane::kQ2, ix, 0, 4, 1);
  require(observed == 3, "actual helper threshold overshoot fixture");
  std::printf("helper_true_spindle_count anchor=0/10 h=1 observed=%llu clipped_contract=1 actual_contract=threshold_stop\n",
              (unsigned long long)observed);
}

int mutant_case(const std::string& name) {
  if (name != "core-ball-ceil-distance" && name != "witness-no-lane-mask") return 2;
  require(mutants_enable(name), "real product injection enabled");
  if (name == "core-ball-ceil-distance") {
    const auto ball = core_ball(Lane::kQ2, singleton({0, 0, 0}), singleton({1, 1, 0}));
    const bool false_interior = point_in_ball({0, 1, 0}, ball);
    if (ball.radius4 == 3 && false_interior && !spindle_ref(Lane::kQ2, {0, 0, 0}, {1, 1, 0}, {0, 1, 0})) {
      std::printf("mutant=%s killed actual_radius4=3 oracle_radius4=2 shell_distance4_squared=8 false_strict_inclusion=1\n", name.c_str());
      return 4;
    }
  } else {
    const auto ix = count_fixture();
    const u64 threshold[3] = {10, 10, 10};
    const auto result = count_universal_witnesses(ix, leaf_ref(0), leaf_ref(4), threshold, 1, false);
    if (result.c[0] == 8) {
      std::printf("mutant=%s killed actual_count=8 independent_unique_count=3 threshold=10\n", name.c_str());
      return 4;
    }
  }
  return 3;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 3 && std::string(argv[1]) == "--mutant") return mutant_case(argv[2]);
    if (argc != 1) return 2;
    root_cases();
    local_geometry_cases();
    count_cases();
    require(counts.roots >= 4110 && counts.spindle >= 5000 && counts.h_zero >= 100 &&
            counts.q3_equal > 0 && counts.q4_equal > 0 && counts.wide_xi > 0 &&
            counts.cores == 432 && counts.positive_cores >= 100 && counts.empty_cores >= 100 &&
            counts.boxes == 5184 && counts.corners == 5184 && counts.masks == 560 &&
            counts.collections == 90 && counts.contacts == 6 && counts.orientations == 864, "non-vacuity floors");
    std::printf("roots=%llu spindle=%llu H_zero=%llu q3_equal=%llu q4_equal=%llu wide_Xi_positive_H=%llu "
                "cores=%llu positive_cores=%llu empty_cores=%llu boxes=%llu corners=%llu masks=%llu collections=%llu contacts=%llu orientations=%llu "
                "FLT_EVAL_METHOD=%d RN=%d\n",
                (unsigned long long)counts.roots, (unsigned long long)counts.spindle, (unsigned long long)counts.h_zero,
                (unsigned long long)counts.q3_equal, (unsigned long long)counts.q4_equal, (unsigned long long)counts.wide_xi,
                (unsigned long long)counts.cores, (unsigned long long)counts.positive_cores, (unsigned long long)counts.empty_cores,
                (unsigned long long)counts.boxes, (unsigned long long)counts.corners, (unsigned long long)counts.masks,
                (unsigned long long)counts.collections, (unsigned long long)counts.contacts,
                (unsigned long long)counts.orientations, FLT_EVAL_METHOD,
                std::fegetround() == FE_TONEAREST);
    return 0;
  } catch (const std::exception& error) {
    std::fprintf(stderr, "failure=%s roots=%llu cores=%llu positives=%llu zeros=%llu masks=%llu collections=%llu\n",
                 error.what(), (unsigned long long)counts.roots, (unsigned long long)counts.cores,
                 (unsigned long long)counts.positive_cores, (unsigned long long)counts.empty_cores,
                 (unsigned long long)counts.masks, (unsigned long long)counts.collections);
    return 1;
  }
}
