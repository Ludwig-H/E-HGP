#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "pipeline/q2_joint_bounds.hpp"

namespace {

using mhgp9::gen::Box3;
using mhgp9::gen::Point3;
using mhgp9::gen::Q2Bounds;
using mhgp9::gen::Q2JointPreparedBounds;
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
  std::uint64_t swaps{};
  std::uint64_t singleton_reductions{};
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
  return {static_cast<mhgp9::gen::Coordinate>(values[0]), static_cast<mhgp9::gen::Coordinate>(values[1]),
          static_cast<mhgp9::gen::Coordinate>(values[2])};
}

bool exceeds_u16(const Box3& box) {
  return box.high.x > 65535 || box.high.y > 65535 || box.high.z > 65535;
}

Box3 enclosure(const Point3& first, const Point3& second) {
  return {{std::min(first.x, second.x), std::min(first.y, second.y), std::min(first.z, second.z)},
          {std::max(first.x, second.x), std::max(first.y, second.y), std::max(first.z, second.z)}};
}

// Direct polynomial on doubled rational coordinates, independently of the
// production center/diameter representation: 4H=sum(2z-2a)(2b-2z).
Integer polynomial4(const Doubled& a, const Doubled& b, const Doubled& z) {
  Integer result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    result += (Integer(z[axis]) - Integer(a[axis])) *
              (Integer(b[axis]) - Integer(z[axis]));
  }
  return result;
}

void include(ExactBounds& bounds, bool& first, const Integer& value) {
  if (first || value < bounds.minimum4) bounds.minimum4 = value;
  if (first || value > bounds.maximum4) bounds.maximum4 = value;
  first = false;
}

// Full 3D oracle: independently enumerate 64 A/B corner pairs, and the
// Cartesian product of Z endpoints and every in-box half-integral summit.
// The direct cpp_int polynomial never calls production geometry.
ExactBounds oracle_bounds(Gate& gate, const Box3& a, const Box3& b, const Box3& z) {
  ExactBounds result{};
  bool first = true;
  for (unsigned a_bits = 0; a_bits < 8; ++a_bits) {
    for (unsigned b_bits = 0; b_bits < 8; ++b_bits) {
      Doubled a2{}, b2{};
      std::array<std::vector<std::int64_t>, 3> choices;
      for (std::size_t axis = 0; axis < 3; ++axis) {
        const auto av = (a_bits & (1U << axis)) != 0 ? a.high[axis] : a.low[axis];
        const auto bv = (b_bits & (1U << axis)) != 0 ? b.high[axis] : b.low[axis];
        a2[axis] = 2 * static_cast<std::int64_t>(av);
        b2[axis] = 2 * static_cast<std::int64_t>(bv);
        choices[axis].push_back(2 * static_cast<std::int64_t>(z.low[axis]));
        choices[axis].push_back(2 * static_cast<std::int64_t>(z.high[axis]));
        const std::int64_t summit = static_cast<std::int64_t>(av) + bv;
        if (choices[axis][0] <= summit && summit <= choices[axis][1]) {
          choices[axis].push_back(summit);
        }
      }
      for (const auto x : choices[0]) {
        for (const auto y : choices[1]) {
          for (const auto zz : choices[2]) {
            include(result, first, polynomial4(a2, b2, {x, y, zz}));
            ++gate.oracle_values;
          }
        }
      }
    }
  }
  return result;
}

ExactBounds dense_half_grid(Gate& gate, const Box3& a, const Box3& b, const Box3& z) {
  // Small boxes only. This includes interior A and B values, not just their
  // corners. Integer endpoint A/B pairs have half-integral Z summits, so
  // all exact continuous extrema occur in this independently swept grid.
  const std::array<Box3, 3> boxes{a, b, z};
  std::array<std::int64_t, 9> lows{}, highs{};
  for (std::size_t group = 0; group < boxes.size(); ++group) {
    for (std::size_t axis = 0; axis < 3; ++axis) {
      lows[3 * group + axis] = 2 * static_cast<std::int64_t>(boxes[group].low[axis]);
      highs[3 * group + axis] = 2 * static_cast<std::int64_t>(boxes[group].high[axis]);
    }
  }
  auto values = lows;
  ExactBounds result{};
  bool first = true;
  for (;;) {
    include(result, first, polynomial4({values[0], values[1], values[2]},
                                      {values[3], values[4], values[5]},
                                      {values[6], values[7], values[8]}));
    ++gate.dense_values;
    std::size_t axis = 0;
    while (axis < values.size() && values[axis] == highs[axis]) {
      values[axis] = lows[axis];
      ++axis;
    }
    if (axis == values.size()) break;
    ++values[axis];
  }
  return result;
}

Q2Bounds compare(Gate& gate, const Box3& a, const Box3& b, const Box3& z,
                 const ExactBounds& expected) {
  const auto actual = Q2JointPreparedBounds(a, b).bounds(z);
  gate.require(Integer(actual.minimum4) == expected.minimum4 &&
                   Integer(actual.maximum4) == expected.maximum4,
               "joint bounds differ from independent exact continuous extrema");
  // Proved range for the input width: -12 M^2 <= 4H <= 3 M^2 with M = 65535
  // when every box fits the historical u16 profile (unchanged check) and
  // M = coordinate_limit = 262143 otherwise.
  const bool wide = exceeds_u16(a) || exceeds_u16(b) || exceeds_u16(z);
  const Integer width = wide ? Integer(mhgp9::gen::coordinate_limit) : Integer(65535);
  const Integer square = width * width;
  gate.require(-12 * square <= actual.minimum4 && actual.minimum4 <= actual.maximum4 &&
                   actual.maximum4 <= 3 * square,
               wide ? "joint bounds exceed their proved signed 18-bit range"
                    : "joint bounds exceed their proved signed-u16 range");
  if (wide) ++gate.cases18;
  const auto swapped = Q2JointPreparedBounds(b, a).bounds(z);
  gate.require(swapped.minimum4 == actual.minimum4 && swapped.maximum4 == actual.maximum4,
               "exchanging A and B changed joint bounds");
  ++gate.swaps;
  for (const bool reverse : {false, true}) {
    const auto& fixed = reverse ? b : a;
    const auto& other = reverse ? a : b;
    if (fixed.low == fixed.high) {
      const auto singleton = mhgp9::gen::Q2PreparedBounds(fixed.low, other).bounds(z);
      gate.require(singleton.minimum4 == actual.minimum4 && singleton.maximum4 == actual.maximum4,
                   "singleton joint box disagrees with prepared anchor bounds");
      ++gate.singleton_reductions;
    }
  }
  if (actual.minimum4 > 0) ++gate.interior;
  else if (actual.maximum4 < 0) ++gate.exterior;
  else if (actual.minimum4 == 0 && actual.maximum4 == 0) ++gate.shell;
  else ++gate.mixed;
  ++gate.cases;
  return actual;
}

Q2Bounds check(Gate& gate, const Box3& a, const Box3& b, const Box3& z, bool dense = false) {
  const auto expected = oracle_bounds(gate, a, b, z);
  if (dense) {
    const auto observed = dense_half_grid(gate, a, b, z);
    gate.require(observed.minimum4 == expected.minimum4 && observed.maximum4 == expected.maximum4,
                 "full half-grid extrema differ from the cpp_int corner/summit oracle");
  }
  return compare(gate, a, b, z, expected);
}

void interval_exhaustion(Gate& gate) {
  std::vector<Box3> intervals;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    intervals.clear();
    for (std::int64_t low = 0; low < 4; ++low) {
      for (std::int64_t high = low; high < 4; ++high) {
        Doubled lows{}, highs{};
        lows[axis] = low;
        highs[axis] = high;
        intervals.push_back({point(lows), point(highs)});
      }
    }
    for (const auto& a : intervals) {
      for (const auto& b : intervals) {
        for (const auto& z : intervals) {
          const auto expected = dense_half_grid(gate, a, b, z);
          static_cast<void>(compare(gate, a, b, z, expected));
        }
      }
    }
  }
}

// Exact lattice isometry: axis permutation then reflection x -> side - x,
// with side = 65535 (u16 profile) or coordinate_limit (18-bit profile).
Point3 transformed(const Point3& input, const std::array<unsigned, 3>& axes,
                    unsigned reflection, mhgp9::gen::Coordinate side) {
  Doubled values{};
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const auto value = input[axes[axis]];
    values[axis] = (reflection & (1U << axis)) != 0 ? side - value : value;
  }
  return point(values);
}

Box3 transformed(const Box3& input, const std::array<unsigned, 3>& axes,
                  unsigned reflection, mhgp9::gen::Coordinate side) {
  return enclosure(transformed(input.low, axes, reflection, side),
                   transformed(input.high, axes, reflection, side));
}

void three_dimensional(Gate& gate) {
  static_cast<void>(check(gate, {{0, 0, 0}, {1, 1, 1}}, {{10, 10, 10}, {11, 11, 11}},
                               {{4, 4, 4}, {6, 6, 6}}));
  static_cast<void>(check(gate, {{0, 0, 0}, {1, 1, 1}}, {{10, 10, 10}, {11, 11, 11}},
                               {{20, 20, 20}, {21, 21, 21}}));
  static_cast<void>(check(gate, {{0, 0, 0}, {1, 1, 1}}, {{0, 0, 0}, {1, 1, 1}},
                               {{0, 0, 0}, {1, 1, 1}}, true));
  static_cast<void>(check(gate, {{0, 0, 0}, {1, 1, 1}}, {{2, 1, 0}, {3, 2, 1}},
                               {{1, 0, 0}, {2, 1, 1}}, true));

  const Box3 a{{1000, 1001, 1002}, {1003, 1004, 1005}};
  const Box3 b{{1004, 1002, 1000}, {1008, 1007, 1005}};
  const Box3 z{{999, 1000, 1001}, {1009, 1004, 1006}};
  const auto baseline = check(gate, a, b, z);
  std::array<unsigned, 3> axes{0, 1, 2};
  do {
    for (unsigned reflection = 0; reflection < 8; ++reflection) {
      const auto actual = check(gate, transformed(a, axes, reflection, 65535),
                                     transformed(b, axes, reflection, 65535),
                                     transformed(z, axes, reflection, 65535));
      gate.require(actual.minimum4 == baseline.minimum4 && actual.maximum4 == baseline.maximum4,
                   "axis permutation or exact u16 reflection changed joint bounds");
      ++gate.symmetries;
    }
  } while (std::next_permutation(axes.begin(), axes.end()));

  // 18-bit twin: the same boxes translated by 260000 next to the far corner
  // (4H depends on differences only, so the extrema are equal), then every
  // axis permutation and exact reflection x -> 262143 - x.
  const Box3 a18{{261000, 261001, 261002}, {261003, 261004, 261005}};
  const Box3 b18{{261004, 261002, 261000}, {261008, 261007, 261005}};
  const Box3 z18{{260999, 261000, 261001}, {261009, 261004, 261006}};
  const auto baseline18 = check(gate, a18, b18, z18);
  gate.require(baseline18.minimum4 == baseline.minimum4 && baseline18.maximum4 == baseline.maximum4,
               "translating the symmetry fixture to 18-bit coordinates changed joint bounds");
  axes = {0, 1, 2};
  do {
    for (unsigned reflection = 0; reflection < 8; ++reflection) {
      const auto actual = check(gate, transformed(a18, axes, reflection, mhgp9::gen::coordinate_limit),
                                     transformed(b18, axes, reflection, mhgp9::gen::coordinate_limit),
                                     transformed(z18, axes, reflection, mhgp9::gen::coordinate_limit));
      gate.require(actual.minimum4 == baseline.minimum4 && actual.maximum4 == baseline.maximum4,
                   "axis permutation or exact 18-bit reflection changed joint bounds");
      ++gate.symmetries18;
    }
  } while (std::next_permutation(axes.begin(), axes.end()));

  std::uint64_t state = UINT64_C(0x83e751b920f6a4cd);
  const auto coordinate = [&state]() {
    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;
    return static_cast<std::uint16_t>((state * UINT64_C(2685821657736338717)) >> 48);
  };
  for (unsigned iteration = 0; iteration < 128; ++iteration) {
    std::array<Box3, 3> boxes;
    for (auto& box : boxes) {
      const Point3 first{coordinate(), coordinate(), coordinate()};
      const Point3 second{coordinate(), coordinate(), coordinate()};
      box = enclosure(first, second);
    }
    static_cast<void>(check(gate, boxes[0], boxes[1], boxes[2]));
  }

  // Separate stable pseudo-random full-18-bit family (own state and own
  // floor); the u16 recipe above is pinned and unchanged.
  std::uint64_t state18 = UINT64_C(0x2b7e9d0c4a61f358);
  const auto coordinate18 = [&state18]() {
    state18 ^= state18 >> 12;
    state18 ^= state18 << 25;
    state18 ^= state18 >> 27;
    return static_cast<mhgp9::gen::Coordinate>((state18 * UINT64_C(2685821657736338717)) >> 46);
  };
  for (unsigned iteration = 0; iteration < 128; ++iteration) {
    std::array<Box3, 3> boxes;
    for (auto& box : boxes) {
      const Point3 first{coordinate18(), coordinate18(), coordinate18()};
      const Point3 second{coordinate18(), coordinate18(), coordinate18()};
      box = enclosure(first, second);
      gate.require(box.high.x <= mhgp9::gen::coordinate_limit && box.high.y <= mhgp9::gen::coordinate_limit &&
                       box.high.z <= mhgp9::gen::coordinate_limit,
                   "18-bit pseudo-random recipe left the coordinate range");
    }
    static_cast<void>(check(gate, boxes[0], boxes[1], boxes[2]));
    ++gate.random18;
  }
}

void model_counterexamples(Gate& gate) {
  // Arithmetic counter-models only, not mutated product binaries.
  const auto crossed = check(gate, {{0, 0, 0}, {2, 0, 0}}, {{0, 0, 0}, {2, 0, 0}},
                                  {{1, 0, 0}, {1, 0, 0}});
  gate.require(crossed.minimum4 == -4 && crossed.maximum4 == 4 &&
                   polynomial4({0, 0, 0}, {0, 0, 0}, {2, 0, 0}) == -4 &&
                   polynomial4({4, 0, 0}, {4, 0, 0}, {2, 0, 0}) == -4,
               "matching A/B endpoints alone did not miss the positive crossed extrema");
  ++gate.model_mutants;

  const auto corners = check(gate, {{0, 2, 2}, {0, 2, 2}}, {{4, 2, 2}, {4, 2, 2}},
                                  {{2, 0, 0}, {2, 4, 4}});
  gate.require(corners.minimum4 == -16 && corners.maximum4 == 16,
               "Z-corner-only maximum did not miss a strict center interior");
  ++gate.model_mutants;
  const auto half = check(gate, {{0, 0, 0}, {0, 0, 0}}, {{3, 0, 0}, {3, 0, 0}},
                               {{1, 0, 0}, {2, 0, 0}});
  gate.require(half.minimum4 == 8 && half.maximum4 == 9 &&
                   polynomial4({0, 0, 0}, {6, 0, 0}, {2, 0, 0}) == 8,
               "integer-rounded summit did not lose the exact half-integer maximum");
  ++gate.model_mutants;
  const auto varying_a = check(gate, {{0, 0, 0}, {2, 0, 0}}, {{4, 0, 0}, {4, 0, 0}},
                                    {{1, 0, 0}, {1, 0, 0}});
  gate.require(varying_a.minimum4 == -12 && varying_a.maximum4 == 12,
               "a single anchor or NoCredit excluded mixed queries in A");
  ++gate.model_mutants;
  const auto endpoints = check(gate, {{0, 0, 0}, {0, 0, 0}}, {{2, 0, 0}, {2, 0, 0}},
                                    {{0, 0, 0}, {2, 0, 0}});
  gate.require(endpoints.minimum4 == 0 && endpoints.maximum4 == 4,
               "nearest-Z distance in the minimum incorrectly credited endpoints");
  ++gate.model_mutants;
  const auto shell = check(gate, {{0, 0, 0}, {0, 0, 0}}, {{2, 0, 0}, {2, 0, 0}},
                                {{1, 1, 0}, {1, 1, 0}});
  gate.require(shell.minimum4 == 0 && shell.maximum4 == 0 && !(shell.minimum4 > 0),
               "non-strict positivity changed a pure shell to an interior");
  ++gate.model_mutants;

  const Integer square = Integer(65535) * 65535;
  const Box3 full{{0, 0, 0}, {65535, 65535, 65535}};
  const auto extremes = check(gate, full, full, full);
  gate.require(Integer(extremes.minimum4) == -12 * square &&
                   Integer(extremes.maximum4) == 3 * square,
               "full-u16 joint boxes did not attain both proved extrema");
  const auto wide = check(gate, {{0, 0, 0}, {0, 0, 0}}, {{65535, 0, 0}, {65535, 0, 0}},
                               {{0, 0, 0}, {65535, 0, 0}});
  Integer wrapped32 = Integer(wide.maximum4) % (Integer(1) << 32);
  if (wrapped32 >= (Integer(1) << 31)) wrapped32 -= Integer(1) << 32;
  gate.require(Integer(wide.maximum4) == square && wrapped32 < 0,
               "signed-i32 overflow did not invert a wide positive maximum");
  ++gate.model_mutants;
  gate.require(extremes.minimum4 < 0 && (Integer(1) << 64) + extremes.minimum4 > 0,
               "unsigned arithmetic did not turn a negative minimum into positive credit");
  ++gate.model_mutants;
  const auto summed = check(gate, {{0, 0, 0}, {0, 0, 0}}, {{2, 2, 2}, {2, 2, 2}},
                                 {{1, 1, 1}, {1, 1, 1}});
  gate.require(summed.minimum4 == 12 && summed.maximum4 == 12 && summed.minimum4 != 4,
               "one coordinate extremum incorrectly replaced the sum over three axes");
  ++gate.model_mutants;

  // 18-bit twins of the extremal fixtures (M = coordinate_limit = 262143):
  // the same identities with M^2 = 68718952449 > 2^32, counted separately.
  constexpr mhgp9::gen::Coordinate m = mhgp9::gen::coordinate_limit;
  const Integer square18 = Integer(m) * m;
  const Box3 full18{{0, 0, 0}, {m, m, m}};
  const auto extremes18 = check(gate, full18, full18, full18);
  gate.require(Integer(extremes18.minimum4) == -12 * square18 &&
                   Integer(extremes18.maximum4) == 3 * square18,
               "full-18-bit joint boxes did not attain both proved extrema");
  const auto wide18 = check(gate, {{0, 0, 0}, {0, 0, 0}}, {{m, 0, 0}, {m, 0, 0}},
                                 {{0, 0, 0}, {m, 0, 0}});
  Integer wrapped32_18 = Integer(wide18.maximum4) % (Integer(1) << 32);
  if (wrapped32_18 >= (Integer(1) << 31)) wrapped32_18 -= Integer(1) << 32;
  gate.require(Integer(wide18.maximum4) == square18 && wrapped32_18 < 0,
               "signed-i32 overflow did not invert a wide 18-bit maximum");
  ++gate.model_mutants18;
  // A u32 store of the prepared square D = (b - a)^2 (the historical layout)
  // would keep only D mod 2^32 at 18 bits and silently shrink the maximum.
  const Integer truncated32 = square18 % (Integer(1) << 32);
  gate.require(square18 >= (Integer(1) << 32) && truncated32 < square18 &&
                   Integer(wide18.maximum4) != truncated32,
               "u32 square-storage mutant did not lose the wide 18-bit maximum");
  ++gate.model_mutants18;
  gate.require(extremes18.minimum4 < 0 && (Integer(1) << 64) + extremes18.minimum4 > 0,
               "unsigned arithmetic did not turn a negative 18-bit minimum into positive credit");
  ++gate.model_mutants18;
  // Varying-A fixture translated to the far corner: the same mixed extrema
  // (-12, 12) with C = a + b close to 2^19 and every product promoted to i64.
  const auto varying_a18 = check(gate, {{m - 4, 0, 0}, {m - 2, 0, 0}}, {{m, 0, 0}, {m, 0, 0}},
                                      {{m - 3, 0, 0}, {m - 3, 0, 0}});
  gate.require(varying_a18.minimum4 == -12 && varying_a18.maximum4 == 12,
               "far-corner varying-A fixture lost its mixed extrema");
  ++gate.model_mutants18;
}

void ownership_and_rejections(Gate& gate) {
  Box3 a{{1, 2, 3}, {3, 4, 5}};
  Box3 b{{5, 6, 7}, {9, 10, 11}};
  const Box3 z{{0, 1, 2}, {10, 11, 12}};
  Q2JointPreparedBounds original(a, b);
  const auto expected = oracle_bounds(gate, a, b, z);
  const Q2JointPreparedBounds copied(original);
  Q2JointPreparedBounds assigned({{0, 0, 0}, {0, 0, 0}}, {{1, 1, 1}, {1, 1, 1}});
  assigned = original;
  a = {{65535, 65535, 65535}, {0, 0, 0}};
  b = a;
  for (const auto* prepared : std::array<const Q2JointPreparedBounds*, 3>{&original, &copied, &assigned}) {
    const auto actual = prepared->bounds(z);
    gate.require(Integer(actual.minimum4) == expected.minimum4 &&
                     Integer(actual.maximum4) == expected.maximum4,
                 "joint constants retained a mutable alias to A or B");
    ++gate.copies;
  }
  original = Q2JointPreparedBounds({{0, 0, 0}, {0, 0, 0}}, {{2, 0, 0}, {2, 0, 0}});
  gate.require(copied.bounds(z).minimum4 == assigned.bounds(z).minimum4 &&
                   copied.bounds(z).maximum4 == assigned.bounds(z).maximum4,
               "assignment changed a distinct prepared object's constants");
  const Box3 valid{{0, 0, 0}, {1, 1, 1}};
  for (std::size_t axis = 0; axis < 3; ++axis) {
    Doubled low{}, high{};
    low[axis] = 1;
    const Box3 inverted{point(low), point(high)};
    gate.rejects([&] { static_cast<void>(Q2JointPreparedBounds(inverted, valid)); },
                  "joint constructor accepted an inverted A interval");
    gate.rejects([&] { static_cast<void>(Q2JointPreparedBounds(valid, inverted)); },
                  "joint constructor accepted an inverted B interval");
    gate.rejects([&] { static_cast<void>(copied.bounds(inverted)); },
                  "joint query accepted an inverted Z interval");
  }
  const auto after_rejections = copied.bounds(z);
  gate.require(Integer(after_rejections.minimum4) == expected.minimum4 &&
                   Integer(after_rejections.maximum4) == expected.maximum4,
               "invalid Z queries modified prepared constants");
}

static_assert(std::is_trivially_copyable_v<Q2JointPreparedBounds>);
static_assert(std::is_trivially_copy_constructible_v<Q2JointPreparedBounds>);
static_assert(std::is_trivially_copy_assignable_v<Q2JointPreparedBounds>);
static_assert(std::is_standard_layout_v<Q2JointPreparedBounds>);
static_assert(!std::is_default_constructible_v<Q2JointPreparedBounds>);
static_assert(sizeof(Q2JointPreparedBounds) == 192);  // u64 constants since the 18-bit widening
static_assert(sizeof(Q2Bounds) == 2 * sizeof(std::int64_t));

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp9_gen_q2_joint_bounds_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    interval_exhaustion(gate);
    three_dimensional(gate);
    model_counterexamples(gate);
    ownership_and_rejections(gate);
    gate.require(gate.cases >= 3100 && gate.oracle_values > 50000 && gate.dense_values > 100000 &&
                     gate.interior > 0 && gate.exterior > 0 && gate.shell > 0 && gate.mixed > 0 &&
                     gate.swaps == gate.cases && gate.singleton_reductions > 0 &&
                     gate.symmetries == 48 && gate.model_mutants == 9 &&
                     gate.invalid_inputs == 9 && gate.copies == 3,
                 "joint bounds gate non-vacuity failed");
    // cases18 = 1 translated baseline + 42 symmetries (the six fully
    // reflected images fall back under 65535) + 128 random + 3 model-twin
    // checks (extremes, wide, varying-A).
    gate.require(gate.cases18 == 174 && gate.random18 == 128 && gate.symmetries18 == 48 &&
                     gate.model_mutants18 == 4,
                 "joint bounds gate 18-bit non-vacuity failed");
    std::cout << "mhgp9_gen_q2_joint_bounds_gate passed checks=" << gate.checks
              << " cases=" << gate.cases << " oracle_values=" << gate.oracle_values
              << " dense_values=" << gate.dense_values << " interior=" << gate.interior
              << " exterior=" << gate.exterior << " shell=" << gate.shell << " mixed=" << gate.mixed
              << " swaps=" << gate.swaps << " singleton_reductions=" << gate.singleton_reductions
              << " symmetries=" << gate.symmetries << " model_mutants=" << gate.model_mutants
              << " invalid_inputs=" << gate.invalid_inputs << " copies=" << gate.copies
              << " cases18=" << gate.cases18 << " random18=" << gate.random18
              << " symmetries18=" << gate.symmetries18 << " model_mutants18=" << gate.model_mutants18
              << " prepared_bytes=" << sizeof(Q2JointPreparedBounds) << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp9_gen_q2_joint_bounds_gate failed: " << error.what() << '\n';
    return 1;
  }
}
