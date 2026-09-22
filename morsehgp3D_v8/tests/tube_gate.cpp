#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../oracle/p0_oracle.hpp"
#include "pipeline/local_credits.hpp"

// Independent, bounded judges for the tube certificate, not the FULL tower.
// The direct cpp_int model below has no sorting or monotone suffix sweep.
namespace {

using mhgp8::Box3;
using mhgp8::CreditPlan;
using mhgp8::Lane;
using mhgp8::Point3;
using mhgp8::Range;
using mhgp8::RectangleInput;
using mhgp8::Strategy;
namespace oracle = mhgp8::oracle;
using Integer = oracle::Integer;
using Vector = std::array<Integer, 3>;
using Key = std::array<mhgp8::Coordinate, 3>;
constexpr std::array<Lane, 3> lanes{Lane::Q2, Lane::Q3, Lane::Q4};

struct Gate {
  std::uint64_t checks{};
  std::uint64_t small_plans{};
  std::uint64_t permutations{};
  std::uint64_t geometric_rejections{};
  std::uint64_t retained_pairs{};
  std::uint64_t missed_credits{};
  std::uint64_t mutants{};
  std::uint64_t fallback_cases{};

  void require(bool condition, const std::string& message) {
    ++checks;
    if (!condition) {
      throw std::runtime_error(message);
    }
  }
};

[[nodiscard]] Key key(const Point3& point) {
  return {point.x, point.y, point.z};
}

[[nodiscard]] Vector direction(const Box3& a, const Box3& b) {
  Vector result;
  for (std::size_t j = 0; j < 3; ++j) {
    result[j] = Integer(b.low[j]) + b.high[j] - a.low[j] - a.high[j];
  }
  return result;
}

[[nodiscard]] Integer norm_squared(const Vector& value) {
  Integer result = 0;
  for (const auto& component : value) {
    result += component * component;
  }
  return result;
}

[[nodiscard]] Integer diagonal_squared(const Box3& box) {
  Vector difference;
  for (std::size_t j = 0; j < 3; ++j) {
    difference[j] = Integer(box.high[j]) - box.low[j];
  }
  return norm_squared(difference);
}

[[nodiscard]] bool separated(const Box3& a, const Box3& b,
                              unsigned coefficient = 100) {
  const auto axis = direction(a, b);
  const Integer diameter = std::max(diagonal_squared(a), diagonal_squared(b));
  return norm_squared(axis) > 0 && norm_squared(axis) >= coefficient * diameter;
}

enum class Mutation { None, RankOnly, AllowSelf };

[[nodiscard]] std::vector<std::uint8_t> direct_tubes(
    const RectangleInput& input, Range own, const Box3& a, const Box3& b,
    Lane lane, unsigned cap, Mutation mutation = Mutation::None,
    unsigned separation_coefficient = 100) {
  std::vector<std::uint8_t> result(own.size(), 0);
  if (cap == 0 || !separated(a, b, separation_coefficient)) {
    return result;
  }
  const auto d = direction(a, b);
  Integer width = 0;
  for (const auto& value : d) {
    const Integer absolute = value < 0 ? -value : value;
    width = std::max(width, absolute);
  }
  width *= 4;
  struct Record { Integer projection; Vector transverse; Vector cell; };
  std::vector<Record> records(own.size());
  for (std::size_t i = 0; i < own.size(); ++i) {
    const auto& p = input.points[own.first + i];
    for (std::size_t j = 0; j < 3; ++j) {
      records[i].projection += d[j] * p[j];
      const auto k = (j + 1) % 3;
      const auto l = (j + 2) % 3;
      records[i].transverse[j] = d[k] * p[l] - d[l] * p[k];
    }
  }
  Vector origin = records.front().transverse;
  for (const auto& row : records) {
    for (std::size_t j = 0; j < 3; ++j) {
      origin[j] = std::min(origin[j], row.transverse[j]);
    }
  }
  for (auto& row : records) {
    for (std::size_t j = 0; j < 3; ++j) {
      row.cell[j] = (row.transverse[j] - origin[j]) / width;
    }
  }
  const unsigned left = lane == Lane::Q4 ? 16 : 1;
  const unsigned right = lane == Lane::Q3 ? 1 : 9;
  for (std::size_t i = 0; i < records.size(); ++i) {
    Vector low = records[i].transverse;
    Vector high = low;
    for (const auto& other : records) {
      if (other.cell == records[i].cell) {
        for (std::size_t j = 0; j < 3; ++j) {
          low[j] = std::min(low[j], other.transverse[j]);
          high[j] = std::max(high[j], other.transverse[j]);
        }
      }
    }
    Vector span;
    for (std::size_t j = 0; j < 3; ++j) {
      span[j] = high[j] - low[j];
    }
    const Integer bound = mutation == Mutation::RankOnly ? Integer(0) : norm_squared(span);
    unsigned count = 0;
    for (const auto& other : records) {
      if (other.cell == records[i].cell) {
        const Integer delta = other.projection - records[i].projection;
        const bool positive = mutation == Mutation::AllowSelf ? delta >= 0 : delta > 0;
        if (positive && left * bound <= right * delta * delta) {
          ++count;
        }
      }
    }
    result[i] = static_cast<std::uint8_t>(std::min(cap, count));
  }
  return result;
}

struct Signature {
  std::map<Key, unsigned> a;
  std::map<Key, unsigned> b;
  std::set<std::pair<Key, Key>> pairs;
  bool operator==(const Signature&) const = default;
};

[[nodiscard]] Signature check_small(Gate& gate, const RectangleInput& input,
                                     unsigned kmax, unsigned s, Lane lane) {
  const auto owner = mhgp8::prepare_rectangle(input, kmax, s);
  const auto plan = mhgp8::make_credit_plan(owner, lane, Strategy::Tubes);
  ++gate.small_plans;
  const auto a = oracle::bounds(input.points, input.a.first, input.a.last);
  const auto b = oracle::bounds(input.points, input.b.first, input.b.last);
  const auto h = oracle::threshold(kmax, lane);
  const auto core = oracle::core_credit(input.points, input.core_candidates, a, b, lane, h);
  const auto need = h - core;
  const auto expected_a = direct_tubes(input, input.a, a, b, lane, need);
  const auto expected_b = direct_tubes(input, input.b, b, a, lane, need);
  const auto full_a = oracle::local_credits(input.points, input.a.first,
                                             input.a.last, b, lane, need);
  const auto full_b = oracle::local_credits(input.points, input.b.first,
                                             input.b.last, a, lane, need);
  gate.require(plan.threshold() == h && plan.core_credit() == core,
                "tube plan changed lane threshold or core authority");
  gate.require(plan.a_credits().size() == expected_a.size() &&
                   plan.b_credits().size() == expected_b.size(),
                "tube plan changed original factor sizes");
  Signature signature;
  for (std::size_t i = 0; i < expected_a.size(); ++i) {
    gate.require(plan.a_credits()[i] == expected_a[i],
                  "A tube count differs from direct cpp_int cell/cone model");
    gate.require(plan.a_credits()[i] <= full_a[i],
                  "A tube credit exceeds exact universal Wq histogram");
    gate.missed_credits += full_a[i] - plan.a_credits()[i];
    signature.a.emplace(key(input.points[input.a.first + i]), plan.a_credits()[i]);
  }
  for (std::size_t i = 0; i < expected_b.size(); ++i) {
    gate.require(plan.b_credits()[i] == expected_b[i],
                  "B tube count differs from direct cpp_int cell/cone model");
    gate.require(plan.b_credits()[i] <= full_b[i],
                  "B tube credit exceeds exact universal Wq histogram");
    gate.missed_credits += full_b[i] - plan.b_credits()[i];
    signature.b.emplace(key(input.points[input.b.first + i]), plan.b_credits()[i]);
  }
  std::set<std::pair<std::size_t, std::size_t>> emitted;
  plan.for_each_candidate([&](std::size_t aid, std::size_t bid) {
    gate.require(input.a.first <= aid && aid < input.a.last &&
                     input.b.first <= bid && bid < input.b.last,
                  "tube expansion changed original IDs");
    gate.require(emitted.emplace(aid, bid).second, "tube emitted a pair twice");
    signature.pairs.emplace(key(input.points[aid]), key(input.points[bid]));
  });
  gate.require(emitted.size() == plan.candidate_pairs(),
                "tube descriptor count disagrees with expansion");
  for (std::size_t aid = input.a.first; aid < input.a.last; ++aid) {
    for (std::size_t bid = input.b.first; bid < input.b.last; ++bid) {
      const auto credit = core + expected_a[aid - input.a.first] +
                          expected_b[bid - input.b.first];
      const bool keep = h != 0 && credit < h;
      gate.require(plan.keeps(aid, bid) == keep && emitted.contains({aid, bid}) == keep,
                    "tube residual does not match its disjoint credit sum");
      if (keep) {
        ++gate.retained_pairs;
      } else if (h != 0) {
        gate.require(oracle::point_credit(input.points, aid, bid, lane, h) >= h,
                      "tube rejected a pair without h distinct exact Wq witnesses");
        ++gate.geometric_rejections;
      }
    }
  }
  const auto& work = plan.work();
  gate.require(work.tube_sweep_tests <= 2 * work.tube_records,
                "tube sweep exceeded its linear comparison bound");
  if (need != 0 && !separated(a, b)) {
    gate.require(work.tube_separation_fallbacks == 2 && work.tube_records == 0 &&
                     work.tube_cells == 0 && plan.candidate_pairs() == plan.total_pairs(),
                  "failed cone premise did not preserve the complete residual");
    ++gate.fallback_cases;
  }
  return signature;
}

[[nodiscard]] RectangleInput tube_fixture(unsigned count, unsigned orientation) {
  RectangleInput result;
  result.a = {0, count};
  result.b = {count, 2 * count};
  for (unsigned side = 0; side < 2; ++side) {
    for (unsigned i = 0; i < count; ++i) {
      const unsigned x = 1000 + 2 * i + side * 30000;
      const unsigned y = 1000 + i % 2;
      const unsigned z = 1000 + (i / 2) % 2;
      std::array<unsigned, 3> values{x, y, z};
      switch (orientation) {
        case 0: break;
        case 1: values = {y, x, z}; break;
        case 2: values = {65535 - x, y, z}; break;
        case 3: values = {x + y - 1000, x + 1000 - y, z}; break;
        case 4: values = {z, 65535 - y, x}; break;
        default: throw std::invalid_argument("test orientation");
      }
      result.points.push_back({static_cast<std::uint16_t>(values[0]),
                                static_cast<std::uint16_t>(values[1]),
                                static_cast<std::uint16_t>(values[2])});
    }
  }
  return result;
}

void regular_gate(Gate& gate) {
  for (unsigned orientation = 0; orientation < 5; ++orientation) {
    const auto input = tube_fixture(12, orientation);
    auto permuted = input;
    std::reverse(permuted.points.begin(), permuted.points.begin() + 12);
    std::rotate(permuted.points.begin() + 12, permuted.points.begin() + 17,
                 permuted.points.end());
    for (const auto s : {8U, 10U, 12U}) {
      for (const auto kmax : {1U, 5U, 10U}) {
        for (const auto lane : lanes) {
          const auto original = check_small(gate, input, kmax, s, lane);
          const auto reordered = check_small(gate, permuted, kmax, s, lane);
          gate.require(original == reordered, "tube credits changed under input permutation");
          ++gate.permutations;
          const unsigned h = oracle::threshold(kmax, lane);
          gate.require(original.pairs.size() == h * (h + 1) / 2,
                        "well-spaced tube did not retain the triangular residual");
        }
      }
    }
  }
  // Global offsets exercise negative cross coordinates and wide products.
  RectangleInput wide{{{0, 0, 0}, {6000, 6000, 6000},
                       {59535, 59535, 59535}, {65535, 65535, 65535}},
                      {0, 2}, {2, 4}, {}};
  const auto a = oracle::bounds(wide.points, 0, 2);
  const auto b = oracle::bounds(wide.points, 2, 4);
  const auto d = direction(a, b);
  Integer delta = 0;
  for (std::size_t j = 0; j < 3; ++j) {
    delta += d[j] * 6000;
  }
  gate.require(9 * delta * delta > std::numeric_limits<std::uint64_t>::max(),
                "wide tube fixture does not exceed 64-bit products");
  for (const auto lane : lanes) {
    static_cast<void>(check_small(gate, wide, 5, 8, lane));
  }
  for (auto& point : wide.points) {
    point.y = static_cast<std::uint16_t>(65535 - point.y);
  }
  for (const auto lane : lanes) {
    static_cast<void>(check_small(gate, wide, 5, 8, lane));
  }
  auto with_core = tube_fixture(6, 0);
  with_core.core_candidates.push_back(with_core.points.size());
  with_core.points.push_back({15000, 1000, 1000});
  for (const auto lane : lanes) {
    static_cast<void>(check_small(gate, with_core, 5, 8, lane));
    static_cast<void>(check_small(gate, with_core, 1, 8, lane));
  }
}

void equalities_and_mutants(Gate& gate) {
  // Cone-bound equalities have a separate strict geometric margin. Delta=0
  // does not: it must still exclude the anchor and coincident projections.
  const std::array<Point3, 3> steps{Point3{1, 3, 0}, Point3{1, 1, 0},
                                     Point3{4, 3, 0}};
  for (std::size_t i = 0; i < lanes.size(); ++i) {
    const auto z = steps[i];
    const RectangleInput input{{{0, 0, 0}, z, {1000, 0, 0},
                                 {static_cast<std::uint16_t>(1000 + z.x), z.y, z.z}},
                                {0, 2}, {2, 4}, {}};
    const auto signature = check_small(gate, input, static_cast<unsigned>(lanes[i]) - 1,
                                       8, lanes[i]);
    gate.require(signature.a.at({0, 0, 0}) == 1,
                  "cone equality was incorrectly replaced by strict inequality");
  }
  const RectangleInput singleton{{{0, 0, 0}, {100, 0, 0}}, {0, 1}, {1, 2}, {}};
  const auto one_a = oracle::bounds(singleton.points, 0, 1);
  const auto one_b = oracle::bounds(singleton.points, 1, 2);
  const auto self_mutant = direct_tubes(singleton, singleton.a, one_a, one_b,
                                         Lane::Q2, 1, Mutation::AllowSelf);
  gate.require(self_mutant[0] == 1 &&
                   !oracle::point_witness(Lane::Q2, singleton.points[0],
                                           singleton.points[1], singleton.points[0]),
                "self-boundary mutation did not expose a false credit");
  ++gate.mutants;
  for (const auto lane : lanes) {
    static_cast<void>(check_small(gate, singleton, 5, 8, lane));
  }
  const RectangleInput rank{{{0, 0, 0}, {1, 3, 0}, {1000, 0, 0}, {1001, 3, 0}},
                             {0, 2}, {2, 4}, {}};
  const auto rank_a = oracle::bounds(rank.points, 0, 2);
  const auto rank_b = oracle::bounds(rank.points, 2, 4);
  for (const auto lane : {Lane::Q3, Lane::Q4}) {
    const auto mutant = direct_tubes(rank, rank.a, rank_a, rank_b, lane, 1,
                                       Mutation::RankOnly);
    const auto exact = oracle::local_credits(rank.points, 0, 2, rank_b, lane, 1);
    gate.require(mutant[0] == 1 && exact[0] == 0,
                  "rank-without-transverse-margin mutation was not refuted");
    ++gate.mutants;
    static_cast<void>(check_small(gate, rank, 5, 8, lane));
  }
  const auto input = tube_fixture(12, 0);
  const auto owner = mhgp8::prepare_rectangle(input, 10, 8);
  const auto plan = mhgp8::make_credit_plan(owner, Lane::Q2, Strategy::Tubes);
  bool double_loses_valid_pair = false;
  for (std::size_t aid = 0; aid < 12; ++aid) {
    for (std::size_t bid = 12; bid < 24; ++bid) {
      const unsigned credits = plan.a_credits()[aid] + plan.b_credits()[bid - 12];
      if (credits < 10 && 2 * credits >= 10 &&
          oracle::point_credit(input.points, aid, bid, Lane::Q2, 10) < 10) {
        double_loses_valid_pair = true;
        gate.require(plan.keeps(aid, bid), "nominal path already added overlapping minorants");
      }
    }
  }
  gate.require(double_loses_valid_pair, "adding identical minorants mutant was not refuted");
  ++gate.mutants;

  // D/R = 5.2, hence 25*diag^2 passes but the required 100*diag^2 fails.
  // The wrong factor four actually credits a non-W4 site, not just a model mismatch.
  const RectangleInput wrong_scale{
      {{0, 2, 0}, {4, 5, 0}, {15, 1, 0}, {15, 6, 0}}, {0, 2}, {2, 4}, {}};
  const auto scale_a = oracle::bounds(wrong_scale.points, 0, 2);
  const auto scale_b = oracle::bounds(wrong_scale.points, 2, 4);
  gate.require(separated(scale_a, scale_b, 25) && !separated(scale_a, scale_b),
                "factor-four separation fixture is vacuous");
  const auto bad_scale = direct_tubes(wrong_scale, wrong_scale.a, scale_a, scale_b,
                                        Lane::Q4, 1, Mutation::None, 25);
  gate.require(bad_scale[0] == 1 &&
                   !oracle::point_witness(Lane::Q4, wrong_scale.points[0],
                                           wrong_scale.points[2], wrong_scale.points[1]),
                "wrong separation scaling did not expose a false Q4 credit");
  ++gate.mutants;
  for (const auto lane : lanes) {
    static_cast<void>(check_small(gate, wrong_scale, 5, 1, lane));
  }
}

void missed_witness_gate(Gate& gate) {
  RectangleInput cube;
  cube.a = {0, 8};
  cube.b = {8, 16};
  for (unsigned side = 0; side < 2; ++side) {
    for (unsigned x : {0U, 10U}) {
      for (unsigned y : {0U, 10U}) {
        for (unsigned z : {0U, 10U}) {
          cube.points.push_back({static_cast<std::uint16_t>(200 * side + x),
                                  static_cast<std::uint16_t>(y),
                                  static_cast<std::uint16_t>(z)});
        }
      }
    }
  }
  const auto before = gate.missed_credits;
  const auto result = check_small(gate, cube, 4, 8, Lane::Q4);
  gate.require(gate.missed_credits > before && !result.pairs.empty(),
                "tube limitation fixture lost its true uncredited W4 witnesses or residual");
}

[[nodiscard]] RectangleInput rails_fixture(unsigned length) {
  constexpr unsigned rails = 9;
  const unsigned shift = 48 * rails * length;
  RectangleInput result;
  const unsigned factor = rails * (length + 1);
  result.a = {0, factor};
  result.b = {factor, 2 * factor};
  for (unsigned side = 0; side < 2; ++side) {
    for (unsigned rail = 0; rail < rails; ++rail) {
      for (unsigned x = 0; x <= length; ++x) {
        result.points.push_back({static_cast<std::uint16_t>(side * shift + x),
                                  static_cast<std::uint16_t>(4 * length * rail), 0});
      }
    }
  }
  return result;
}

void rails_gate(Gate& gate) {
  // Exhaustive geometric judges are confined to this small member.
  static_cast<void>(check_small(gate, rails_fixture(8), 10, 8, Lane::Q4));
  // The 2718-point member is judged by the independently proved rail formula.
  // No A*A, B*B or A*B geometric enumeration is performed in this large test.
  const auto input = rails_fixture(150);
  const auto owner = mhgp8::prepare_rectangle(input, 10, 12);
  const auto tubes = mhgp8::make_credit_plan(owner, Lane::Q4, Strategy::Tubes);
  const auto pool = mhgp8::make_credit_plan(owner, Lane::Q4, Strategy::Pool);
  const auto dual = mhgp8::make_credit_plan(owner, Lane::Q4, Strategy::DualBlocks);
  for (std::size_t i = 0; i < input.a.size(); ++i) {
    const auto x = static_cast<unsigned>(input.points[i].x);
    gate.require(tubes.a_credits()[i] == std::min(8U, 150U - x) &&
                     tubes.b_credits()[i] == std::min(8U, x),
                  "large rails disagree with the proven per-coordinate credit formula");
    gate.require(dual.a_credits()[i] == tubes.a_credits()[i] &&
                     dual.b_credits()[i] == tubes.b_credits()[i],
                  "large rails dual/tubes credit arrays disagree");
  }
  gate.require(tubes.total_pairs() == 1846881 && tubes.candidate_pairs() == 2916 &&
                   dual.candidate_pairs() == 2916 && pool.candidate_pairs() == 1846881,
                "large rails residual comparison changed");
  gate.require(tubes.work().tube_records == 2718 && tubes.work().tube_cells == 18 &&
                   tubes.work().tube_sweep_tests <= 5436 &&
                   tubes.work().tube_credited_sites > 0 &&
                   tubes.work().tube_separation_fallbacks == 0,
                "large rails did not exercise the expected linear tube sweep");
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_tube_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    regular_gate(gate);
    equalities_and_mutants(gate);
    missed_witness_gate(gate);
    rails_gate(gate);
    gate.require(gate.small_plans >= 280 && gate.permutations == 135 &&
                     gate.geometric_rejections > 1000 && gate.retained_pairs > 100 &&
                     gate.missed_credits > 0 && gate.fallback_cases == 3 && gate.mutants == 5,
                  "tube gate non-vacuity failed");
    std::cout << "mhgp8_tube_gate passed checks=" << gate.checks
              << " small_plans=" << gate.small_plans
              << " permutations=" << gate.permutations
              << " geometric_rejections=" << gate.geometric_rejections
              << " retained_pairs=" << gate.retained_pairs
              << " missed_credits=" << gate.missed_credits
              << " mutants=" << gate.mutants
              << " fallback_cases=" << gate.fallback_cases
              << " large_rails_residual=2916\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_tube_gate failed: " << error.what() << '\n';
    return 1;
  }
}
