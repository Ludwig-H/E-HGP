#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "spindle/q2_prepared_bounds.hpp"

namespace {

using mhgp8::Box3;
using mhgp8::Point3;
using mhgp8::Q2Bounds;
using mhgp8::Q2PreparedBounds;
using Integer = boost::multiprecision::cpp_int;
using Doubled = std::array<std::int64_t, 3>;

struct Gate {
  std::uint64_t checks{};
  std::uint64_t cases{};
  std::uint64_t oracle_values{};
  std::uint64_t dense_values{};
  std::uint64_t interior{};
  std::uint64_t exterior{};
  std::uint64_t shell{};
  std::uint64_t mixed{};
  std::uint64_t symmetries{};
  std::uint64_t model_mutants{};
  std::uint64_t invalid_inputs{};
  std::uint64_t copies{};
  // 18-bit coverage (coordinate_limit = 262143): counted separately so that
  // every historical u16 pin above keeps its exact value.
  std::uint64_t cases18{};
  std::uint64_t random18{};
  std::uint64_t symmetries18{};
  std::uint64_t model_mutants18{};

  void require(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
  }
  template <class Function>
  void rejects(Function&& function, const char* message) {
    bool rejected = false;
    try { function(); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, message);
    ++invalid_inputs;
  }
};

struct ExactBounds {
  Integer minimum4;
  Integer maximum4;
};

Point3 point(const Doubled& values) {
  return {static_cast<mhgp8::Coordinate>(values[0]), static_cast<mhgp8::Coordinate>(values[1]),
          static_cast<mhgp8::Coordinate>(values[2])};
}

bool exceeds_u16(const Point3& value) {
  return value.x > 65535 || value.y > 65535 || value.z > 65535;
}

// Direct polynomial, not the product's stored center/diameter constants:
// for possibly half-integral b and z, 4H = sum (2z-2a)(2b-2z).
Integer polynomial4(const Point3& a, const Doubled& b2, const Doubled& z2) {
  Integer result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    result += (Integer(z2[axis]) - 2 * Integer(a[axis])) *
              (Integer(b2[axis]) - Integer(z2[axis]));
  }
  return result;
}

// Independent exact continuous extrema: b is separately affine, so enumerate
// its eight corners. At each corner, enumerate Z's endpoints and every
// in-box half-integral parabola summit. No production geometry is called.
ExactBounds oracle_bounds(Gate& gate, const Point3& a, const Box3& b, const Box3& z) {
  ExactBounds result{};
  bool first = true;
  for (unsigned bits = 0; bits < 8; ++bits) {
    Doubled b2{};
    std::array<std::vector<std::int64_t>, 3> choices;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const auto bv = (bits & (1U << axis)) != 0 ? b.high[axis] : b.low[axis];
      b2[axis] = 2 * static_cast<std::int64_t>(bv);
      choices[axis].push_back(2 * static_cast<std::int64_t>(z.low[axis]));
      choices[axis].push_back(2 * static_cast<std::int64_t>(z.high[axis]));
      const std::int64_t summit = static_cast<std::int64_t>(a[axis]) + bv;
      if (choices[axis][0] <= summit && summit <= choices[axis][1]) {
        choices[axis].push_back(summit);
      }
    }
    for (const auto x : choices[0]) {
      for (const auto y : choices[1]) {
        for (const auto zz : choices[2]) {
          const Integer value = polynomial4(a, b2, {x, y, zz});
          if (first || value < result.minimum4) result.minimum4 = value;
          if (first || value > result.maximum4) result.maximum4 = value;
          first = false;
          ++gate.oracle_values;
        }
      }
    }
  }
  return result;
}

void dense_half_grid(Gate& gate, const Point3& a, const Box3& b,
                     const Box3& z, const ExactBounds& expected) {
  // Small fixtures only. B interior values are included too; all extrema
  // are attained in this grid because integer B endpoints have half-integer
  // Z summits. This judge never enters the production census path.
  std::array<std::int64_t, 6> lows{};
  std::array<std::int64_t, 6> highs{};
  for (std::size_t axis = 0; axis < 3; ++axis) {
    lows[axis] = 2 * static_cast<std::int64_t>(b.low[axis]);
    highs[axis] = 2 * static_cast<std::int64_t>(b.high[axis]);
    lows[axis + 3] = 2 * static_cast<std::int64_t>(z.low[axis]);
    highs[axis + 3] = 2 * static_cast<std::int64_t>(z.high[axis]);
  }
  auto values = lows;
  ExactBounds observed{};
  bool first = true;
  for (;;) {
    const Integer value = polynomial4(a, {values[0], values[1], values[2]},
                                         {values[3], values[4], values[5]});
    gate.require(expected.minimum4 <= value && value <= expected.maximum4,
                 "continuous bounds omit a half-grid point of B cross Z");
    if (first || value < observed.minimum4) observed.minimum4 = value;
    if (first || value > observed.maximum4) observed.maximum4 = value;
    first = false;
    ++gate.dense_values;
    std::size_t axis = 0;
    while (axis < values.size() && values[axis] == highs[axis]) {
      values[axis] = lows[axis];
      ++axis;
    }
    if (axis == values.size()) break;
    ++values[axis];
  }
  gate.require(observed.minimum4 == expected.minimum4 && observed.maximum4 == expected.maximum4,
               "half-grid extrema disagree with the independent continuous oracle");
}

Q2Bounds check(Gate& gate, const Point3& a, const Box3& b, const Box3& z,
                bool dense = false) {
  const Q2PreparedBounds prepared(a, b);
  const auto actual = prepared.bounds(z);
  const auto expected = oracle_bounds(gate, a, b, z);
  gate.require(Integer(actual.minimum4) == expected.minimum4 &&
                   Integer(actual.maximum4) == expected.maximum4,
               "prepared q2 bounds differ from independent cpp_int extrema");
  // Proved range for the input width: -12 M^2 <= 4H <= 3 M^2 with M = 65535
  // when every coordinate fits the historical u16 profile (unchanged check)
  // and M = coordinate_limit = 262143 otherwise.
  const bool wide = exceeds_u16(a) || exceeds_u16(b.low) || exceeds_u16(b.high) ||
                    exceeds_u16(z.low) || exceeds_u16(z.high);
  const Integer width = wide ? Integer(mhgp8::coordinate_limit) : Integer(65535);
  const Integer square = width * width;
  gate.require(-12 * square <= actual.minimum4 && actual.minimum4 <= actual.maximum4 &&
                   actual.maximum4 <= 3 * square,
               wide ? "prepared q2 bounds exceeded their exact 18-bit range"
                    : "prepared q2 bounds exceeded their exact u16 range");
  if (wide) ++gate.cases18;
  if (actual.minimum4 > 0) ++gate.interior;
  else if (actual.maximum4 < 0) ++gate.exterior;
  else if (actual.minimum4 == 0 && actual.maximum4 == 0) ++gate.shell;
  else ++gate.mixed;
  if (dense) dense_half_grid(gate, a, b, z, expected);
  ++gate.cases;
  return actual;
}

void interval_exhaustion(Gate& gate) {
  for (std::size_t axis = 0; axis < 3; ++axis) {
    for (std::int64_t anchor = 0; anchor < 4; ++anchor) {
      for (std::int64_t bl = 0; bl < 4; ++bl) {
        for (std::int64_t bh = bl; bh < 4; ++bh) {
          for (std::int64_t zl = 0; zl < 4; ++zl) {
            for (std::int64_t zh = zl; zh < 4; ++zh) {
              Doubled av{}, bv_low{}, bv_high{}, zv_low{}, zv_high{};
              av[axis] = anchor;
              bv_low[axis] = bl;
              bv_high[axis] = bh;
              zv_low[axis] = zl;
              zv_high[axis] = zh;
              static_cast<void>(check(gate, point(av), {point(bv_low), point(bv_high)},
                                        {point(zv_low), point(zv_high)}, true));
            }
          }
        }
      }
    }
  }
}

// Exact lattice isometry: axis permutation then reflection x -> side - x,
// with side = 65535 (u16 profile) or coordinate_limit (18-bit profile).
Point3 transformed(const Point3& input, const std::array<unsigned, 3>& axes,
                    unsigned reflection, mhgp8::Coordinate side) {
  Doubled result{};
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const auto value = input[axes[axis]];
    result[axis] = (reflection & (1U << axis)) != 0 ? side - value : value;
  }
  return point(result);
}

Box3 transformed(const Box3& box, const std::array<unsigned, 3>& axes,
                  unsigned reflection, mhgp8::Coordinate side) {
  const auto first = transformed(box.low, axes, reflection, side);
  const auto second = transformed(box.high, axes, reflection, side);
  return {{std::min(first.x, second.x), std::min(first.y, second.y), std::min(first.z, second.z)},
          {std::max(first.x, second.x), std::max(first.y, second.y), std::max(first.z, second.z)}};
}

void three_dimensional(Gate& gate) {
  static_cast<void>(check(gate, {0, 0, 0}, {{10, 10, 10}, {11, 11, 11}},
                            {{4, 4, 4}, {6, 6, 6}}));
  static_cast<void>(check(gate, {0, 0, 0}, {{10, 10, 10}, {11, 11, 11}},
                            {{20, 20, 20}, {21, 21, 21}}));
  static_cast<void>(check(gate, {0, 0, 0}, {{2, 0, 0}, {2, 4, 0}},
                            {{1, 0, 1}, {1, 0, 1}}));
  for (const Point3& a : {Point3{0, 0, 0}, {1, 2, 0}, {3, 1, 2}}) {
    static_cast<void>(check(gate, a, {{0, 0, 0}, {2, 2, 2}},
                              {{0, 0, 0}, {2, 2, 2}}, true));
  }

  const Point3 a{1000, 1001, 1002};
  const Box3 b{{1004, 1002, 1000}, {1008, 1007, 1005}};
  const Box3 z{{999, 1000, 1001}, {1009, 1004, 1006}};
  const auto baseline = check(gate, a, b, z);
  std::array<unsigned, 3> axes{0, 1, 2};
  do {
    for (unsigned reflection = 0; reflection < 8; ++reflection) {
      const auto result = check(gate, transformed(a, axes, reflection, 65535),
                                  transformed(b, axes, reflection, 65535),
                                  transformed(z, axes, reflection, 65535));
      gate.require(result.minimum4 == baseline.minimum4 && result.maximum4 == baseline.maximum4,
                   "axis permutation or u16 reflection changed q2 extrema");
      ++gate.symmetries;
    }
  } while (std::next_permutation(axes.begin(), axes.end()));

  // 18-bit twin: the same local geometry translated by 260000 next to the
  // far corner (4H depends on differences only, so the extrema are equal),
  // then every axis permutation and exact reflection x -> 262143 - x.
  const Point3 a18{261000, 261001, 261002};
  const Box3 b18{{261004, 261002, 261000}, {261008, 261007, 261005}};
  const Box3 z18{{260999, 261000, 261001}, {261009, 261004, 261006}};
  const auto baseline18 = check(gate, a18, b18, z18);
  gate.require(baseline18.minimum4 == baseline.minimum4 && baseline18.maximum4 == baseline.maximum4,
               "translating the symmetry fixture to 18-bit coordinates changed q2 extrema");
  axes = {0, 1, 2};
  do {
    for (unsigned reflection = 0; reflection < 8; ++reflection) {
      const auto result = check(gate, transformed(a18, axes, reflection, mhgp8::coordinate_limit),
                                  transformed(b18, axes, reflection, mhgp8::coordinate_limit),
                                  transformed(z18, axes, reflection, mhgp8::coordinate_limit));
      gate.require(result.minimum4 == baseline.minimum4 && result.maximum4 == baseline.maximum4,
                   "axis permutation or 18-bit reflection changed q2 extrema");
      ++gate.symmetries18;
    }
  } while (std::next_permutation(axes.begin(), axes.end()));

  // Stable pseudo-random full-u16 inputs; no product fixture generator is used.
  std::uint64_t state = UINT64_C(0x429d840bf9a26317);
  const auto coordinate = [&state]() {
    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;
    return static_cast<std::uint16_t>((state * UINT64_C(2685821657736338717)) >> 48);
  };
  for (unsigned index = 0; index < 160; ++index) {
    const Point3 anchor{coordinate(), coordinate(), coordinate()};
    const Point3 b0{coordinate(), coordinate(), coordinate()};
    const Point3 b1{coordinate(), coordinate(), coordinate()};
    const Point3 z0{coordinate(), coordinate(), coordinate()};
    const Point3 z1{coordinate(), coordinate(), coordinate()};
    const Box3 bv{{std::min(b0.x, b1.x), std::min(b0.y, b1.y), std::min(b0.z, b1.z)},
                 {std::max(b0.x, b1.x), std::max(b0.y, b1.y), std::max(b0.z, b1.z)}};
    const Box3 zv{{std::min(z0.x, z1.x), std::min(z0.y, z1.y), std::min(z0.z, z1.z)},
                 {std::max(z0.x, z1.x), std::max(z0.y, z1.y), std::max(z0.z, z1.z)}};
    static_cast<void>(check(gate, anchor, bv, zv));
  }

  // Separate stable pseudo-random full-18-bit family (own state and own
  // floor); the u16 recipe above is pinned and unchanged.
  std::uint64_t state18 = UINT64_C(0x5c1f0b3a9e72d641);
  const auto coordinate18 = [&state18]() {
    state18 ^= state18 >> 12;
    state18 ^= state18 << 25;
    state18 ^= state18 >> 27;
    return static_cast<mhgp8::Coordinate>((state18 * UINT64_C(2685821657736338717)) >> 46);
  };
  for (unsigned index = 0; index < 160; ++index) {
    const Point3 anchor{coordinate18(), coordinate18(), coordinate18()};
    const Point3 b0{coordinate18(), coordinate18(), coordinate18()};
    const Point3 b1{coordinate18(), coordinate18(), coordinate18()};
    const Point3 z0{coordinate18(), coordinate18(), coordinate18()};
    const Point3 z1{coordinate18(), coordinate18(), coordinate18()};
    const Box3 bv{{std::min(b0.x, b1.x), std::min(b0.y, b1.y), std::min(b0.z, b1.z)},
                 {std::max(b0.x, b1.x), std::max(b0.y, b1.y), std::max(b0.z, b1.z)}};
    const Box3 zv{{std::min(z0.x, z1.x), std::min(z0.y, z1.y), std::min(z0.z, z1.z)},
                 {std::max(z0.x, z1.x), std::max(z0.y, z1.y), std::max(z0.z, z1.z)}};
    gate.require(anchor.x <= mhgp8::coordinate_limit && bv.high.x <= mhgp8::coordinate_limit &&
                     zv.high.x <= mhgp8::coordinate_limit,
                 "18-bit pseudo-random recipe left the coordinate range");
    static_cast<void>(check(gate, anchor, bv, zv));
    ++gate.random18;
  }
}

void model_counterexamples(Gate& gate) {
  // Explicit ports of the auditors' maximum-at-interior and NoCredit fixtures.
  // Counter-models below are arithmetic models, not mutated product binaries.
  const auto corners = check(gate, {0, 2, 2}, {{4, 2, 2}, {4, 2, 2}},
                               {{2, 0, 0}, {2, 4, 4}});
  gate.require(corners.minimum4 == -16 && corners.maximum4 == 16,
               "corner-exterior / center-interior fixture changed");
  for (unsigned bits = 0; bits < 8; ++bits) {
    const Integer value = polynomial4({0, 2, 2}, {8, 4, 4},
        {4, (bits & 2U) != 0 ? 8 : 0, (bits & 4U) != 0 ? 8 : 0});
    gate.require(value == -16 && value < corners.maximum4,
                 "corner-only maximum was not refuted by a strict interior");
  }
  ++gate.model_mutants;

  const auto half = check(gate, {0, 0, 0}, {{3, 0, 0}, {3, 0, 0}},
                            {{1, 0, 0}, {2, 0, 0}});
  const Integer rounded_summit = polynomial4({0, 0, 0}, {6, 0, 0}, {2, 0, 0});
  gate.require(half.minimum4 == 8 && half.maximum4 == 9 && rounded_summit == 8,
               "integer-rounded summit did not lose the continuous half-integer maximum");
  ++gate.model_mutants;

  const auto no_credit = check(gate, {1000, 1, 0}, {{60000, 1, 0}, {60000, 3, 0}},
                                 {{1000, 2, 0}, {1000, 2, 0}});
  gate.require(no_credit.minimum4 == -4 && no_credit.maximum4 == 4,
               "one refuting B endpoint incorrectly excluded every query in the B box");
  ++gate.model_mutants;

  const auto endpoint = check(gate, {0, 0, 0}, {{2, 0, 0}, {2, 0, 0}},
                                {{0, 0, 0}, {2, 0, 0}});
  gate.require(endpoint.minimum4 == 0 && endpoint.maximum4 == 4 &&
                   endpoint.maximum4 > endpoint.minimum4,
               "using the nearest instead of farthest Z value did not falsely credit endpoints");
  ++gate.model_mutants;
  const auto shell = check(gate, {0, 0, 0}, {{2, 0, 0}, {2, 0, 0}},
                             {{1, 1, 0}, {1, 1, 0}});
  gate.require(shell.minimum4 == 0 && shell.maximum4 == 0 && !(shell.minimum4 > 0),
               "non-strict positivity incorrectly promoted a pure shell to interior");
  ++gate.model_mutants;

  const Integer square = Integer(65535) * 65535;
  const auto maximum = check(gate, {0, 0, 0}, {{65535, 65535, 65535}, {65535, 65535, 65535}},
                               {{0, 0, 0}, {65535, 65535, 65535}});
  const auto minimum = check(gate, {0, 0, 0}, {{0, 0, 0}, {0, 0, 0}},
                               {{65535, 65535, 65535}, {65535, 65535, 65535}});
  gate.require(Integer(maximum.maximum4) == 3 * square &&
                   Integer(minimum.minimum4) == -12 * square &&
                   minimum.minimum4 == minimum.maximum4,
               "the exact positive or negative u16 extrema were not attained");
  const auto wide = check(gate, {0, 0, 0}, {{65535, 0, 0}, {65535, 0, 0}},
                            {{0, 0, 0}, {65535, 0, 0}});
  Integer wrapped32 = Integer(wide.maximum4) % (Integer(1) << 32);
  if (wrapped32 >= (Integer(1) << 31)) wrapped32 -= Integer(1) << 32;
  gate.require(Integer(wide.maximum4) == square && wrapped32 < 0,
               "signed-i32 arithmetic mutant did not invert a wide positive maximum");
  ++gate.model_mutants;
  const Integer unsigned_negative = (Integer(1) << 64) + minimum.minimum4;
  gate.require(minimum.minimum4 < 0 && unsigned_negative > 0,
               "unsigned residual mutation did not turn exterior power into positive credit");
  ++gate.model_mutants;
  const auto all_axes = check(gate, {0, 0, 0}, {{2, 2, 2}, {2, 2, 2}},
                                 {{1, 1, 1}, {1, 1, 1}});
  gate.require(all_axes.minimum4 == 12 && all_axes.maximum4 == 12 && all_axes.minimum4 != 4,
               "an axiswise extremum replaced the sum of all three coordinate contributions");
  ++gate.model_mutants;

  // 18-bit twins of the extremal fixtures (M = coordinate_limit = 262143):
  // the same identities with M^2 = 68718952449 > 2^32, counted separately.
  constexpr mhgp8::Coordinate m = mhgp8::coordinate_limit;
  const Integer square18 = Integer(m) * m;
  const auto maximum18 = check(gate, {0, 0, 0}, {{m, m, m}, {m, m, m}}, {{0, 0, 0}, {m, m, m}});
  const auto minimum18 = check(gate, {0, 0, 0}, {{0, 0, 0}, {0, 0, 0}}, {{m, m, m}, {m, m, m}});
  gate.require(Integer(maximum18.maximum4) == 3 * square18 &&
                   Integer(minimum18.minimum4) == -12 * square18 &&
                   minimum18.minimum4 == minimum18.maximum4,
               "the exact positive or negative 18-bit extrema were not attained");
  const auto wide18 = check(gate, {0, 0, 0}, {{m, 0, 0}, {m, 0, 0}}, {{0, 0, 0}, {m, 0, 0}});
  Integer wrapped32_18 = Integer(wide18.maximum4) % (Integer(1) << 32);
  if (wrapped32_18 >= (Integer(1) << 31)) wrapped32_18 -= Integer(1) << 32;
  gate.require(Integer(wide18.maximum4) == square18 && wrapped32_18 < 0,
               "signed-i32 arithmetic mutant did not invert a wide 18-bit maximum");
  ++gate.model_mutants18;
  // A u32 store of the prepared square D = (e - a)^2 (the historical layout)
  // would keep only D mod 2^32 at 18 bits and silently shrink the maximum.
  const Integer truncated32 = square18 % (Integer(1) << 32);
  gate.require(square18 >= (Integer(1) << 32) && truncated32 < square18 &&
                   Integer(wide18.maximum4) != truncated32,
               "u32 square-storage mutant did not lose the wide 18-bit maximum");
  ++gate.model_mutants18;
  const Integer unsigned_negative18 = (Integer(1) << 64) + minimum18.minimum4;
  gate.require(minimum18.minimum4 < 0 && unsigned_negative18 > 0,
               "unsigned residual mutation did not turn 18-bit exterior power into positive credit");
  ++gate.model_mutants18;
  // NoCredit twin with a far B endpoint: D = 261000^2 > 2^32 must cancel
  // exactly against (2z - C)^2 on the x axis, leaving only the y contribution.
  const auto no_credit18 = check(gate, {1000, 1, 0}, {{262000, 1, 0}, {262000, 3, 0}},
                                   {{1000, 2, 0}, {1000, 2, 0}});
  gate.require(no_credit18.minimum4 == -4 && no_credit18.maximum4 == 4,
               "one far refuting B endpoint incorrectly excluded every query in the B box");
  ++gate.model_mutants18;
}

void ownership_and_rejections(Gate& gate) {
  Point3 a{1, 2, 3};
  Box3 b{{5, 6, 7}, {9, 10, 11}};
  const Box3 z{{0, 1, 2}, {10, 11, 12}};
  Q2PreparedBounds original(a, b);
  const auto expected = oracle_bounds(gate, a, b, z);
  const Q2PreparedBounds copied(original);
  Q2PreparedBounds assigned({0, 0, 0}, {{1, 1, 1}, {1, 1, 1}});
  assigned = original;
  a = {65535, 65535, 65535};
  b = {{65535, 65535, 65535}, {0, 0, 0}};  // Mutate live sources, not borrowed memory.
  for (const auto* prepared : std::array<const Q2PreparedBounds*, 3>{&original, &copied, &assigned}) {
    const auto value = prepared->bounds(z);
    gate.require(Integer(value.minimum4) == expected.minimum4 &&
                     Integer(value.maximum4) == expected.maximum4,
                 "prepared constants retained a mutable alias to a or B");
    ++gate.copies;
  }
  original = Q2PreparedBounds({0, 0, 0}, {{2, 0, 0}, {2, 0, 0}});
  gate.require(copied.bounds(z).minimum4 == assigned.bounds(z).minimum4 &&
                   copied.bounds(z).maximum4 == assigned.bounds(z).maximum4,
               "replacing one prepared object changed another object's constants");
  for (std::size_t axis = 0; axis < 3; ++axis) {
    Doubled low{}, high{};
    low[axis] = 1;
    const Box3 inverted{point(low), point(high)};
    gate.rejects([&] { static_cast<void>(Q2PreparedBounds({0, 0, 0}, inverted)); },
                  "prepared q2 factory accepted an inverted B interval");
    gate.rejects([&] { static_cast<void>(copied.bounds(inverted)); },
                  "prepared q2 query accepted an inverted Z interval");
  }
  const auto after_rejections = copied.bounds(z);
  gate.require(Integer(after_rejections.minimum4) == expected.minimum4 &&
                   Integer(after_rejections.maximum4) == expected.maximum4,
               "invalid queries mutated the prepared constants");
}

// Fixed arithmetic state only. These layout properties do NOT qualify a
// CUDA kernel, a GPU backend, end-to-end performance or the HGP FULL tower.
static_assert(std::is_trivially_copyable_v<Q2PreparedBounds>);
static_assert(std::is_trivially_copy_constructible_v<Q2PreparedBounds>);
static_assert(std::is_trivially_copy_assignable_v<Q2PreparedBounds>);
static_assert(std::is_standard_layout_v<Q2PreparedBounds>);
static_assert(!std::is_default_constructible_v<Q2PreparedBounds>);
static_assert(sizeof(Q2PreparedBounds) <= 96);  // u64 constants since the 18-bit widening
static_assert(sizeof(Q2Bounds) == 2 * sizeof(std::int64_t));

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_q2_prepared_bounds_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    interval_exhaustion(gate);
    three_dimensional(gate);
    model_counterexamples(gate);
    ownership_and_rejections(gate);
    gate.require(gate.cases >= 1400 && gate.oracle_values > 100000 && gate.dense_values > 50000 &&
                     gate.interior > 0 && gate.exterior > 0 && gate.shell > 0 && gate.mixed > 0 &&
                     gate.symmetries == 48 && gate.model_mutants == 8 &&
                     gate.invalid_inputs == 6 && gate.copies == 3,
                 "prepared q2 bounds gate non-vacuity failed");
    // cases18 = 1 translated baseline + 42 symmetries (the six fully
    // reflected images fall back under 65535) + 160 random + 4 model twins.
    gate.require(gate.cases18 == 207 && gate.random18 == 160 && gate.symmetries18 == 48 &&
                     gate.model_mutants18 == 4,
                 "prepared q2 bounds gate 18-bit non-vacuity failed");
    std::cout << "mhgp8_q2_prepared_bounds_gate passed checks=" << gate.checks
              << " cases=" << gate.cases << " oracle_values=" << gate.oracle_values
              << " dense_values=" << gate.dense_values << " interior=" << gate.interior
              << " exterior=" << gate.exterior << " shell=" << gate.shell << " mixed=" << gate.mixed
              << " symmetries=" << gate.symmetries << " model_mutants=" << gate.model_mutants
              << " invalid_inputs=" << gate.invalid_inputs << " copies=" << gate.copies
              << " cases18=" << gate.cases18 << " random18=" << gate.random18
              << " symmetries18=" << gate.symmetries18 << " model_mutants18=" << gate.model_mutants18
              << " prepared_bytes=" << sizeof(Q2PreparedBounds) << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_q2_prepared_bounds_gate failed: " << error.what() << '\n';
    return 1;
  }
}
