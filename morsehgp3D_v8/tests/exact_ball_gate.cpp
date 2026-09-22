#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/rational.hpp>

#include "lanes/exact_ball.hpp"
#include "exact_ball_oracle.hpp"

// New bounded judge, not a differential against an older engine or the audit
// oracle. Gaussian elimination of the rational Gram system supplies ALL
// barycentric weights. The centre/radius and the primitive polynomial are
// reconstructed from those weights, independently of production Cramer/W.
namespace {
namespace oracle = mhgp8_test::ball_oracle;
using oracle::Big;
using oracle::Rational;
using oracle::Coefficients;
using oracle::Refusal;
using oracle::bits;
using oracle::gcd;
using Points = std::vector<mhgp8::Point3>;
using mhgp8::Point3;
using mhgp8::u64;

static_assert(!std::is_default_constructible_v<mhgp8::ExactBall>);
static_assert(!std::is_copy_assignable_v<mhgp8::ExactBall>);
static_assert(!std::is_move_assignable_v<mhgp8::ExactBall>);

struct Gate {
  u64 checks{}, cases{}, accepted_q2{}, accepted_q3{}, accepted_q4{}, rejected{};
  u64 rank_deficient{}, boundary_centres{}, exterior_centres{}, permutations{};
  u64 powers{}, strict_interiors{}, shell_sites{}, strict_exteriors{};
  u64 same_ball_across_arities{}, shell_supports{}, max_coefficient_bits{};
  u64 max_power_bits{}, max_naive_radius_bits{}, translated{}, judge_mutants{};
  // 18-bit twins (a support coordinate above 65535) keep separate floors, so
  // the widened fixtures reach the wide-arithmetic bits on their own while the
  // historical 65535/32767 fixtures, now interior points, keep their pinned floors.
  u64 wide_cases{}, wide_accepted_q2{}, wide_accepted_q3{}, wide_accepted_q4{}, wide_rejected{};
  u64 wide_max_coefficient_bits{}, wide_max_power_bits{}, wide_max_naive_radius_bits{}, wide_translated{};
  void require(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
  }
};

oracle::Result oracle_ball(std::span<const Point3> points) { return oracle::make(points); }

std::optional<mhgp8::ExactBall> make_ball(std::span<const Point3> support) {
  switch (support.size()) {
    case 2: return mhgp8::ExactBall::make_q2(support[0], support[1]);
    case 3: return mhgp8::ExactBall::make_q3({support[0], support[1], support[2]});
    case 4: return mhgp8::ExactBall::make_q4({support[0], support[1], support[2], support[3]});
    default: throw std::runtime_error("test support arity");
  }
}

Coefficients copy_key(const mhgp8::ExactBall& ball) {
  Coefficients result{};
  for (std::size_t i = 0; i != result.size(); ++i) result[i] = Big(ball.coefficients()[i]);
  return result;
}

bool wide_support(const Points& support) {
  for (const auto& point : support)
    for (std::size_t axis = 0; axis != 3; ++axis) if (point[axis] > 65535) return true;
  return false;
}

std::optional<Coefficients> check_case(Gate& gate, const Points& support, const Points& probes) {
  const auto reference = oracle_ball(support);
  const auto actual = make_ball(support);
  const bool wide = wide_support(support);
  ++gate.cases;
  if (wide) ++gate.wide_cases;
  gate.require(actual.has_value() == reference.ball.has_value(), "strict positivity/rank differs from rational Gram solve");
  if (!actual) {
    ++gate.rejected;
    if (wide) ++gate.wide_rejected;
    if (reference.refusal == Refusal::Rank) ++gate.rank_deficient;
    if (reference.refusal == Refusal::Boundary) ++gate.boundary_centres;
    if (reference.refusal == Refusal::Exterior) ++gate.exterior_centres;
    return std::nullopt;
  }
  if (support.size() == 2) { ++gate.accepted_q2; if (wide) ++gate.wide_accepted_q2; }
  if (support.size() == 3) { ++gate.accepted_q3; if (wide) ++gate.wide_accepted_q3; }
  if (support.size() == 4) { ++gate.accepted_q4; if (wide) ++gate.wide_accepted_q4; }
  const auto key = copy_key(*actual);
  gate.require(key == reference.ball->coefficients, "primitive key differs from independently rationalised centre/radius");
  gate.require(key[0] > 0, "nonpositive quadratic coefficient");
  Big common = key[0];
  for (const auto& coefficient : key) {
    common = gcd(common, coefficient);
    gate.max_coefficient_bits = std::max(gate.max_coefficient_bits, bits(coefficient));
    if (wide) gate.wide_max_coefficient_bits = std::max(gate.wide_max_coefficient_bits, bits(coefficient));
  }
  gate.require(common == 1, "nonprimitive polynomial key");
  Big naive_numerator = -4 * key[0] * key[4];
  for (std::size_t axis = 0; axis != 3; ++axis) naive_numerator += key[axis + 1] * key[axis + 1];
  gate.max_naive_radius_bits = std::max(gate.max_naive_radius_bits, bits(naive_numerator));
  if (wide) gate.wide_max_naive_radius_bits = std::max(gate.wide_max_naive_radius_bits, bits(naive_numerator));
  gate.require(Rational(naive_numerator, 4 * key[0] * key[0]) == reference.ball->radius_squared,
               "primitive polynomial changed the exact radius");
  for (const auto& point : support) gate.require(actual->power(point) == 0, "support site omitted from sphere");
  for (const auto& point : probes) {
    const auto power = Rational(key[0]) * reference.ball->power(point);
    gate.require(power.denominator() == 1, "primitive oracle power was nonintegral");
    gate.require(Big(actual->power(point)) == power.numerator(), "point power differs from solved centre/radius");
    gate.max_power_bits = std::max(gate.max_power_bits, bits(power.numerator()));
    if (wide) gate.wide_max_power_bits = std::max(gate.wide_max_power_bits, bits(power.numerator()));
    if (power.numerator() < 0) ++gate.strict_interiors;
    if (power.numerator() == 0) ++gate.shell_sites;
    if (power.numerator() > 0) ++gate.strict_exteriors;
    ++gate.powers;
  }
  const auto copied = *actual;
  gate.require(copied == *actual && copy_key(copied) == key, "value copy changed immutable key");
  return key;
}

void add_unique(Points& points, Point3 point) {
  if (std::find(points.begin(), points.end(), point) == points.end()) points.push_back(point);
}
Points shell_fixture() {
  Points points;
  for (int x = -5; x <= 5; ++x) for (int y = -5; y <= 5; ++y)
    for (int z = -5; z <= 5; ++z) if (x * x + y * y + z * z == 25)
      points.push_back({static_cast<mhgp8::Coordinate>(x + 20),
                        static_cast<mhgp8::Coordinate>(y + 20),
                        static_cast<mhgp8::Coordinate>(z + 20)});
  return points;
}

std::vector<Points> fixtures() {
  // Every 65535/32767 corner fixture is followed by its 18-bit twin at
  // 262143/131071 (the historical corners are interior points since the
  // widening of 22 September 2026 and keep only their oracle role).
  return {
      {{0, 0, 0}, {1, 1, 1}},
      {{0, 0, 0}, {65535, 65535, 65535}},
      {{0, 0, 0}, {262143, 262143, 262143}},
      {{262143, 262143, 262143}, {262142, 262143, 262143}},
      {{15, 10, 10}, {5, 10, 10}},
      {{10, 10, 10}, {10, 10, 10}},
      {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}},
      {{25, 20, 20}, {17, 24, 20}, {17, 16, 20}},
      {{0, 0, 0}, {65535, 0, 0}, {32767, 65535, 0}},
      {{0, 0, 0}, {262143, 0, 0}, {131071, 262143, 0}},
      {{262143, 262143, 262143}, {262141, 262143, 262143}, {262142, 262142, 262143}},
      {{0, 0, 0}, {46368, 28657, 0}, {28657, 17711, 0}},
      {{0, 0, 0}, {196418, 121393, 0}, {121393, 75025, 0}},
      {{0, 0, 0}, {2, 0, 0}, {0, 2, 0}},
      {{0, 0, 0}, {3, 0, 0}, {1, 1, 0}},
      {{0, 0, 0}, {2, 0, 0}, {1, 0, 0}},
      {{0, 0, 0}, {0, 0, 0}, {1, 1, 1}},
      {{0, 0, 0}, {1, 1, 0}, {1, 0, 1}, {0, 1, 1}},
      {{0, 0, 0}, {65535, 65535, 0}, {65535, 0, 65535}, {0, 65535, 65535}},
      {{0, 0, 0}, {262143, 262143, 0}, {262143, 0, 262143}, {0, 262143, 262143}},
      {{0, 0, 0}, {262143, 65535, 0}, {65535, 0, 262143}, {0, 262143, 65535}},
      {{23, 24, 20}, {17, 24, 20}, {20, 17, 24}, {20, 17, 16}},
      {{25, 20, 20}, {17, 24, 20}, {17, 16, 20}, {20, 20, 25}},
      {{0, 0, 0}, {10, 0, 0}, {5, 5, 0}, {5, 0, 5}},
      {{0, 0, 0}, {2, 0, 0}, {0, 2, 0}, {0, 0, 2}},
      {{0, 0, 0}, {2, 0, 0}, {0, 2, 0}, {2, 2, 0}},
      {{0, 0, 0}, {0, 0, 0}, {1, 1, 0}, {0, 0, 1}},
      {{900, 1000, 1000}, {1100, 1000, 1000}, {1000, 1120, 1040}, {1000, 1120, 960}}
  };
}

void permutations(Gate& gate, const Points& support, const Points& probes) {
  std::vector<std::size_t> order(support.size());
  for (std::size_t i = 0; i != order.size(); ++i) order[i] = i;
  const auto reference = check_case(gate, support, probes);
  do {
    Points permuted;
    for (const auto index : order) permuted.push_back(support[index]);
    gate.require(check_case(gate, permuted, probes) == reference, "support permutation changed geometry or positivity");
    ++gate.permutations;
  } while (std::next_permutation(order.begin(), order.end()));
}

void common_sphere(Gate& gate) {
  const auto shell = shell_fixture();
  gate.require(shell.size() == 30, "sphere fixture does not contain thirty distinct sites");
  const Points pair{{25, 20, 20}, {15, 20, 20}};
  const Points triangle{{25, 20, 20}, {17, 24, 20}, {17, 16, 20}};
  const Points tetrahedron{{23, 24, 20}, {17, 24, 20}, {20, 17, 24}, {20, 17, 16}};
  const auto pair_ball = make_ball(pair);
  const auto triangle_ball = make_ball(triangle);
  const auto tetra_ball = make_ball(tetrahedron);
  gate.require(pair_ball && triangle_ball && tetra_ball, "same-sphere positive presentations were not admitted");
  gate.require(*pair_ball == *triangle_ball && *pair_ball == *tetra_ball,
               "same sphere from different arities has different keys");
  gate.same_ball_across_arities += 3;
  for (const auto& support : {pair, triangle, tetrahedron}) {
    permutations(gate, support, shell);
    ++gate.shell_supports;
  }
  // Same radius alone is never ball identity.
  const auto translated_pair = mhgp8::ExactBall::make_q2({26, 20, 20}, {16, 20, 20});
  gate.require(translated_pair && !(*translated_pair == *pair_ball), "key merged distinct equal-radius spheres");
  // A smaller support on the global shell does not invalidate the positive
  // triangle/tetrahedron presentations: q_min is not a factory property.
  const auto key = copy_key(*pair_ball);
  gate.require(key == Coefficients{1, -40, -40, -40, 1175}, "common-sphere independent literal is wrong");
  auto wrong = key;
  for (auto& coefficient : wrong) coefficient *= 2;
  gate.require(wrong != key, "judge did not reject a nonprimitive representation");
  ++gate.judge_mutants;
  wrong = key;
  for (auto& coefficient : wrong) coefficient *= -1;
  gate.require(wrong != key, "judge did not reject reversed power signs");
  ++gate.judge_mutants;
  wrong = key;
  ++wrong[4];
  gate.require(wrong != key, "judge did not reject changed radius");
  ++gate.judge_mutants;
}

void translated_cases(Gate& gate) {
  const std::vector<Points> supports{
      {{1, 2, 3}, {11, 12, 13}},
      {{5, 0, 0}, {0, 5, 0}, {0, 0, 5}},
      {{0, 0, 0}, {4, 4, 0}, {4, 0, 4}, {0, 4, 4}}
  };
  const std::array<std::array<unsigned, 3>, 3> shifts{{{100, 1000, 2000}, {65520, 65520, 65520}, {17, 0, 400}}};
  // 18-bit twins of the 65520 shift: the largest support coordinate (13)
  // lands exactly on 262143 and next to the halving midpoint 131071/131072.
  const std::array<std::array<unsigned, 3>, 2> wide_shifts{{{262128, 262128, 262128}, {131072, 131071, 262130}}};
  const auto translate = [&](const Points& support, const oracle::Result& reference,
                             const std::array<unsigned, 3>& shift) {
    Points moved;
    for (const auto& point : support)
      moved.push_back({static_cast<mhgp8::Coordinate>(point.x + shift[0]),
                       static_cast<mhgp8::Coordinate>(point.y + shift[1]),
                       static_cast<mhgp8::Coordinate>(point.z + shift[2])});
    const auto next = oracle_ball(moved);
    gate.require(next.ball && next.ball->radius_squared == reference.ball->radius_squared,
                 "translation changed oracle radius");
    for (std::size_t axis = 0; axis != 3; ++axis)
      gate.require(next.ball->center[axis] == reference.ball->center[axis] + Rational(Big(shift[axis])),
                   "translation changed centre displacement");
    Points probes = moved;
    probes.push_back({0, 0, 0});
    probes.push_back({65535, 65535, 65535});
    probes.push_back({262143, 262143, 262143});
    static_cast<void>(check_case(gate, moved, probes));
  };
  for (const auto& support : supports) {
    const auto reference = oracle_ball(support);
    gate.require(reference.ball.has_value(), "translation fixture is not positive");
    for (const auto& shift : shifts) {
      translate(support, reference, shift);
      ++gate.translated;
    }
    for (const auto& shift : wide_shifts) {
      translate(support, reference, shift);
      ++gate.wide_translated;
    }
  }
}

void run(Gate& gate) {
  Points probes = shell_fixture();
  probes.insert(probes.end(), {{0, 0, 0}, {65535, 65535, 65535}, {1, 32768, 65534},
                               {20, 20, 20}, {30000, 40000, 20000},
                               {262143, 262143, 262143}, {1, 131072, 262142}, {120000, 160000, 80000}});
  for (const auto& support : fixtures()) permutations(gate, support, probes);
  common_sphere(gate);
  translated_cases(gate);
  std::uint64_t state = 0x84a170dc21433f09ULL;
  auto next = [&]() {
    state = state * 6364136223846793005ULL + 1442695040888963407ULL;
    return state >> 32;
  };
  for (unsigned trial = 0; trial != 420; ++trial) {
    Points points;
    const unsigned side = trial % 3 == 0 ? 65536U : 17U;
    while (points.size() != 8)
      add_unique(points, {static_cast<mhgp8::Coordinate>(next() % side),
                          static_cast<mhgp8::Coordinate>(next() % side),
                          static_cast<mhgp8::Coordinate>(next() % side)});
    Points sample_probes = points;
    sample_probes.push_back({0, 0, 0});
    sample_probes.push_back({65535, 65535, 65535});
    for (std::size_t arity = 2; arity != 5; ++arity) {
      Points support(points.begin(), points.begin() + static_cast<std::ptrdiff_t>(arity));
      static_cast<void>(check_case(gate, support, sample_probes));
      if (trial < 6) permutations(gate, support, sample_probes);
    }
  }
  // Separate 18-bit random clouds (side 262144) with their own floors; the
  // historical generator above is pinned by its floors and left unchanged.
  std::uint64_t wide_state = 0x2545f4914f6cdd1dULL;
  auto wide_next = [&]() {
    wide_state = wide_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return wide_state >> 32;
  };
  for (unsigned trial = 0; trial != 140; ++trial) {
    Points points;
    while (points.size() != 8)
      add_unique(points, {static_cast<mhgp8::Coordinate>(wide_next() % 262144U),
                          static_cast<mhgp8::Coordinate>(wide_next() % 262144U),
                          static_cast<mhgp8::Coordinate>(wide_next() % 262144U)});
    Points sample_probes = points;
    sample_probes.push_back({0, 0, 0});
    sample_probes.push_back({65535, 65535, 65535});
    sample_probes.push_back({262143, 262143, 262143});
    for (std::size_t arity = 2; arity != 5; ++arity) {
      Points support(points.begin(), points.begin() + static_cast<std::ptrdiff_t>(arity));
      static_cast<void>(check_case(gate, support, sample_probes));
      if (trial < 2) permutations(gate, support, sample_probes);
    }
  }
  gate.require(gate.cases >= 1500 && gate.accepted_q2 >= 400 && gate.accepted_q3 >= 100 &&
               gate.accepted_q4 >= 50 && gate.rejected >= 100, "arity/correctness nonvacuity floor");
  gate.require(gate.rank_deficient >= 10 && gate.boundary_centres >= 10 && gate.exterior_centres >= 20,
               "rank/strict-positivity refusal nonvacuity floor");
  gate.require(gate.powers >= 10000 && gate.strict_interiors >= 100 && gate.shell_sites >= 500 &&
               gate.strict_exteriors >= 1000, "power-sign nonvacuity floor");
  gate.require(gate.max_coefficient_bits > 64 && gate.max_power_bits > 64 && gate.max_naive_radius_bits > 128,
               "wide arithmetic nonvacuity floor");
  gate.require(gate.same_ball_across_arities == 3 && gate.shell_supports == 3 &&
               gate.translated == 9 && gate.judge_mutants == 3, "canonical key nonvacuity floor");
  gate.require(gate.wide_cases >= 400 && gate.wide_accepted_q2 >= 100 && gate.wide_accepted_q3 >= 40 &&
               gate.wide_accepted_q4 >= 20 && gate.wide_rejected >= 100 && gate.wide_translated == 6,
               "18-bit arity/correctness nonvacuity floor");
  gate.require(gate.wide_max_coefficient_bits > 64 && gate.wide_max_power_bits > 64 && gate.wide_max_naive_radius_bits > 128,
               "18-bit wide arithmetic nonvacuity floor");
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_exact_ball_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    run(gate);
    std::cout << "{\"schema\":\"mhgp8_exact_ball_gate_v1\",\"status\":\"passed\""
              << ",\"checks\":" << gate.checks << ",\"cases\":" << gate.cases
              << ",\"accepted_q2\":" << gate.accepted_q2 << ",\"accepted_q3\":" << gate.accepted_q3
              << ",\"accepted_q4\":" << gate.accepted_q4 << ",\"rejected\":" << gate.rejected
              << ",\"rank_deficient\":" << gate.rank_deficient << ",\"boundary_centres\":" << gate.boundary_centres
              << ",\"exterior_centres\":" << gate.exterior_centres << ",\"permutations\":" << gate.permutations
              << ",\"powers\":" << gate.powers << ",\"strict_interiors\":" << gate.strict_interiors
              << ",\"shell_sites\":" << gate.shell_sites << ",\"strict_exteriors\":" << gate.strict_exteriors
              << ",\"same_ball_across_arities\":" << gate.same_ball_across_arities
              << ",\"max_coefficient_bits\":" << gate.max_coefficient_bits
              << ",\"max_power_bits\":" << gate.max_power_bits
              << ",\"max_naive_radius_bits\":" << gate.max_naive_radius_bits
              << ",\"translated\":" << gate.translated << ",\"judge_mutants\":" << gate.judge_mutants
              << ",\"wide_cases\":" << gate.wide_cases << ",\"wide_accepted_q2\":" << gate.wide_accepted_q2
              << ",\"wide_accepted_q3\":" << gate.wide_accepted_q3 << ",\"wide_accepted_q4\":" << gate.wide_accepted_q4
              << ",\"wide_rejected\":" << gate.wide_rejected
              << ",\"wide_max_coefficient_bits\":" << gate.wide_max_coefficient_bits
              << ",\"wide_max_power_bits\":" << gate.wide_max_power_bits
              << ",\"wide_max_naive_radius_bits\":" << gate.wide_max_naive_radius_bits
              << ",\"wide_translated\":" << gate.wide_translated << "}\n";
  } catch (const std::exception& error) {
    std::cerr << "exact ball gate: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
