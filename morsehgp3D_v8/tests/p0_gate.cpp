#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "../oracle/p0_oracle.hpp"
#include "pipeline/local_credits.hpp"
#include "spindle/predicates.hpp"

namespace {

using mhgp8::BlockDecision;
using mhgp8::Box3;
using mhgp8::Lane;
using mhgp8::Point3;
using mhgp8::PredicateWork;
using mhgp8::RectangleInput;
using mhgp8::Strategy;
namespace oracle = mhgp8::oracle;

constexpr std::array<Lane, 3> lanes{Lane::Q2, Lane::Q3, Lane::Q4};
constexpr std::array<Strategy, 3> strategies{Strategy::Pool, Strategy::DualBlocks, Strategy::Tubes};

struct Gate {
  std::uint64_t checks{};
  std::uint64_t rejections{};
  std::uint64_t strict_mutants{};
  std::uint64_t positive_points{};
  std::uint64_t negative_points{};
  std::array<std::uint64_t, 3> block_decisions{};
  std::uint64_t plans{};
  std::uint64_t killed_pairs{};
  std::uint64_t retained_pairs{};
  std::uint64_t credited_blocks{};
  std::uint64_t noncredit_blocks{};
  std::uint64_t leaf_pairs{};
  std::uint64_t saturated_tasks{};

  void require(bool condition, const std::string& description) {
    ++checks;
    if (!condition) {
      throw std::runtime_error(description);
    }
  }

  template <class Exception, class Function>
  void rejects(Function&& function, const std::string& description) {
    bool rejected = false;
    try {
      function();
    } catch (const Exception&) {
      rejected = true;
    }
    require(rejected, description);
    ++rejections;
  }
};

class Generator {
 public:
  [[nodiscard]] std::uint32_t next() {
    state_ ^= state_ << 13;
    state_ ^= state_ >> 17;
    state_ ^= state_ << 5;
    return state_;
  }

  [[nodiscard]] std::uint16_t coordinate(unsigned bound = 65536) {
    return static_cast<std::uint16_t>(next() % bound);
  }

  [[nodiscard]] Point3 point(unsigned bound = 65536) {
    return {coordinate(bound), coordinate(bound), coordinate(bound)};
  }

 private:
  std::uint32_t state_{0x91d352a7U};
};

void compare_point(Gate& gate, Lane lane, const Point3& a, const Point3& b,
                   const Point3& z) {
  PredicateWork work;
  const bool expected = oracle::point_witness(lane, a, b, z);
  gate.require(mhgp8::point_witness(lane, a, b, z, work) == expected,
               "point predicate disagrees with cpp_int Gram oracle");
  gate.require(work.point_tests == 1, "point test is not counted once");
  if (expected) {
    ++gate.positive_points;
  } else {
    ++gate.negative_points;
  }
}

void predicate_gate(Gate& gate) {
  Generator random;
  for (unsigned sample = 0; sample < 1024; ++sample) {
    const unsigned bound = sample % 2 == 0 ? 16 : 65536;
    const Point3 a = random.point(bound);
    const Point3 b = random.point(bound);
    const Point3 z = random.point(bound);
    for (const auto lane : lanes) {
      compare_point(gate, lane, a, b, z);
      PredicateWork work;
      gate.require(mhgp8::point_witness(lane, a, b, z, work) ==
                       mhgp8::point_witness(lane, b, a, z, work),
                   "endpoint reversal changed a witness");
    }
  }

  const Point3 zero{};
  const std::array<std::pair<Point3, Point3>, 3> boundaries{
      std::pair{Point3{2, 0, 0}, Point3{1, 1, 0}},
      std::pair{Point3{2, 1, 1}, Point3{1, 1, 0}},
      std::pair{Point3{2, 1, 1}, Point3{1, 0, 0}}};
  for (std::size_t i = 0; i < lanes.size(); ++i) {
    const auto [b, z] = boundaries[i];
    const auto value = oracle::metrics(zero, b, z);
    const unsigned multiplier = lanes[i] == Lane::Q3 ? 3 : 2;
    const bool non_strict_mutant = lanes[i] == Lane::Q2
        ? value.h >= 0
        : value.h > 0 && multiplier * value.h * value.h >= value.xi;
    gate.require(non_strict_mutant, "strict-sign mutant fixture is vacuous");
    gate.require(!oracle::point_witness(lanes[i], zero, b, z),
                 "oracle accepts boundary witness");
    compare_point(gate, lanes[i], zero, b, z);
    ++gate.strict_mutants;
    for (const unsigned scale : {1U, 32767U}) {
      const Point3 scaled_b{
          static_cast<std::uint16_t>(b.x * scale),
          static_cast<std::uint16_t>(b.y * scale),
          static_cast<std::uint16_t>(b.z * scale)};
      const Point3 scaled_z{
          static_cast<std::uint16_t>(z.x * scale),
          static_cast<std::uint16_t>(z.y * scale),
          static_cast<std::uint16_t>(z.z * scale)};
      compare_point(gate, lanes[i], zero, scaled_b, scaled_z);
    }
  }

  const Point3 maximum{65535, 65535, 65535};
  const Point3 middle{32768, 32768, 32768};
  const auto large = oracle::metrics(zero, maximum, middle);
  gate.require(3 * large.h * large.h > std::numeric_limits<std::uint64_t>::max(),
               "wide-product fixture does not exceed 64 bits");
  for (const auto lane : lanes) {
    compare_point(gate, lane, zero, maximum, middle);
    compare_point(gate, lane, zero, maximum, zero);
    compare_point(gate, lane, zero, maximum, maximum);
    compare_point(gate, lane, maximum, maximum, middle);
  }

  for (unsigned sample = 0; sample < 192; ++sample) {
    Point3 first = random.point();
    Point3 last = random.point();
    const Box3 box{
        {std::min(first.x, last.x), std::min(first.y, last.y),
         std::min(first.z, last.z)},
        {std::max(first.x, last.x), std::max(first.y, last.y),
         std::max(first.z, last.z)}};
    const Point3 a = random.point();
    const Point3 z = random.point();
    for (const auto lane : lanes) {
      PredicateWork work;
      gate.require(mhgp8::universal_witness(lane, a, box, z, work) ==
                       oracle::universal_witness(lane, a, box, z),
                   "universal predicate disagrees with independent corners");
      if (lane == Lane::Q2) {
        gate.require(work.q2_axis_terms == 3 && work.corner_tests == 0,
                     "Q2 box minimum did not use exactly three affine terms");
      }
    }
  }

  // Both stored B sites admit z, but their box has a corner which does not.
  const Point3 z{1, 1, 0};
  const Point3 b0{0, 3, 0};
  const Point3 b1{3, 0, 0};
  const Box3 box{{0, 0, 0}, {3, 3, 0}};
  PredicateWork work;
  gate.require(oracle::point_witness(Lane::Q2, zero, b0, z) &&
                   oracle::point_witness(Lane::Q2, zero, b1, z),
               "discrete-sites/box fixture is vacuous");
  gate.require(!mhgp8::universal_witness(Lane::Q2, zero, box, z, work),
               "universal-over-box contract weakened to stored sites");
  for (const auto lane : lanes) {
    PredicateWork positive_work;
    const Box3 far_box{{100, 0, 0}, {101, 1, 1}};
    gate.require(mhgp8::universal_witness(lane, zero, far_box, {30, 0, 0},
                                          positive_work) &&
                     oracle::universal_witness(lane, zero, far_box, {30, 0, 0}),
                 "positive universal-over-box fixture failed");
    if (lane != Lane::Q2) {
      gate.require(positive_work.corner_tests == 8,
                   "positive Q3/Q4 box fixture did not exercise all eight corners");
    }
  }
}

[[nodiscard]] std::vector<Point3> lattice(const Box3& box) {
  std::vector<Point3> points;
  for (auto x = box.low.x; x <= box.high.x; ++x) {
    for (auto y = box.low.y; y <= box.high.y; ++y) {
      for (auto z = box.low.z; z <= box.high.z; ++z) {
        points.push_back({static_cast<std::uint16_t>(x),
                          static_cast<std::uint16_t>(y),
                          static_cast<std::uint16_t>(z)});
      }
    }
  }
  return points;
}

void check_block(Gate& gate, Lane lane, const Box3& a, const Box3& b,
                 const Box3& z) {
  PredicateWork work;
  const auto decision = mhgp8::classify_witness_block(lane, a, b, z, work);
  gate.require(work.block_bound_tests == 1, "missing block classification count");
  const auto ordinal = static_cast<std::size_t>(decision);
  gate.require(ordinal < gate.block_decisions.size(), "invalid block decision");
  ++gate.block_decisions[ordinal];
  if (decision == BlockDecision::Uncertain) {
    return;
  }
  const auto anchor_points = lattice(a);
  const auto witness_points = lattice(z);
  const auto opposite_points = lattice(b);
  for (const auto& anchor : anchor_points) {
    for (const auto& witness : witness_points) {
      if (decision == BlockDecision::NoCredit) {
        gate.require(!oracle::universal_witness(lane, anchor, b, witness),
                     "NoCredit excludes a genuinely universal witness");
      } else {
        for (const auto& opposite : opposite_points) {
          gate.require(oracle::point_witness(lane, anchor, opposite, witness),
                       "Credit block includes a non-witness lattice point");
        }
      }
    }
  }
}

void block_gate(Gate& gate) {
  for (const auto lane : lanes) {
    check_block(gate, lane, {{0, 0, 0}, {1, 0, 0}},
                 {{100, 0, 0}, {101, 0, 0}}, {{30, 0, 0}, {31, 0, 0}});
    check_block(gate, lane, {{10, 0, 0}, {11, 0, 0}},
                 {{100, 0, 0}, {101, 0, 0}}, {{0, 0, 0}, {1, 0, 0}});
    check_block(gate, lane, {{0, 0, 0}, {2, 0, 0}},
                 {{100, 0, 0}, {100, 0, 0}}, {{1, 0, 0}, {1, 0, 0}});
    // The maximum in z is at 1.5, not at either integer endpoint.
    check_block(gate, lane, {{0, 0, 0}, {0, 0, 0}},
                 {{3, 0, 0}, {3, 0, 0}}, {{1, 0, 0}, {2, 0, 0}});
  }
  Generator random;
  const auto random_box = [&random](unsigned base, unsigned spread) {
    const Point3 low{static_cast<std::uint16_t>(base + random.coordinate(spread)),
                     random.coordinate(6), random.coordinate(6)};
    const Point3 high{
        static_cast<std::uint16_t>(low.x + random.coordinate(3)),
        static_cast<std::uint16_t>(low.y + random.coordinate(3)),
        static_cast<std::uint16_t>(low.z + random.coordinate(3))};
    return Box3{low, high};
  };
  for (unsigned sample = 0; sample < 96; ++sample) {
    const auto a = random_box(0, 6);
    const auto b = random_box(8, 6);
    const auto z = random_box(0, 16);
    for (const auto lane : lanes) {
      check_block(gate, lane, a, b, z);
    }
  }
  for (const auto count : gate.block_decisions) {
    gate.require(count > 0, "block decision branch has no non-vacuous fixture");
  }
}

[[nodiscard]] RectangleInput line_input(std::size_t count_a,
                                        std::size_t count_b) {
  RectangleInput result;
  result.a = {0, count_a};
  result.b = {count_a, count_a + count_b};
  for (std::size_t i = 0; i < count_a; ++i) {
    result.points.push_back({static_cast<std::uint16_t>(1000 + i), 1000, 1000});
  }
  for (std::size_t i = 0; i < count_b; ++i) {
    result.points.push_back({static_cast<std::uint16_t>(60000 + i), 1000, 1000});
  }
  return result;
}

[[nodiscard]] RectangleInput clusters_input(Generator& random,
                                            std::size_t count_a,
                                            std::size_t count_b) {
  RectangleInput result;
  result.a = {0, count_a};
  result.b = {count_a, count_a + count_b};
  for (std::size_t i = 0; i < count_a + count_b; ++i) {
    const unsigned base = i < count_a ? 1000 : 60000;
    Point3 point;
    do {
      point = {static_cast<std::uint16_t>(base + random.coordinate(100)),
               static_cast<std::uint16_t>(1000 + random.coordinate(100)),
               static_cast<std::uint16_t>(1000 + random.coordinate(100))};
    } while (std::find(result.points.begin(), result.points.end(), point) !=
             result.points.end());
    result.points.push_back(point);
  }
  return result;
}

void check_plan(Gate& gate, const RectangleInput& input, unsigned kmax,
                unsigned separation_s, Lane lane, Strategy strategy) {
  const auto rectangle = mhgp8::prepare_rectangle(input, kmax, separation_s);
  const auto plan = mhgp8::make_credit_plan(rectangle, lane, strategy);
  ++gate.plans;
  const auto box_a = oracle::bounds(input.points, input.a.first, input.a.last);
  const auto box_b = oracle::bounds(input.points, input.b.first, input.b.last);
  gate.require(rectangle->a_box().low == box_a.low &&
                   rectangle->a_box().high == box_a.high &&
                   rectangle->b_box().low == box_b.low &&
                   rectangle->b_box().high == box_b.high,
               "prepared boxes differ from independent bounds");
  const unsigned threshold = oracle::threshold(kmax, lane);
  const unsigned core = oracle::core_credit(input.points, input.core_candidates,
                                             box_a, box_b, lane, threshold);
  const unsigned need = threshold - core;
  gate.require(plan.threshold() == threshold && plan.core_credit() == core &&
                   rectangle->threshold(lane) == threshold &&
                   rectangle->core_credit(lane) == core,
               "lane threshold or independently certified core differs");
  gate.require(plan.lane() == lane && plan.strategy() == strategy &&
                   &plan.rectangle() == rectangle.get(),
               "plan lost its identity or immutable owner");
  const auto expected_a = oracle::local_credits(input.points, input.a.first,
                                                input.a.last, box_b, lane, need);
  const auto expected_b = oracle::local_credits(input.points, input.b.first,
                                                input.b.last, box_a, lane, need);
  gate.require(plan.a_credits().size() == expected_a.size() &&
                   plan.b_credits().size() == expected_b.size(),
               "credit arrays do not preserve factor sizes");
  for (std::size_t i = 0; i < expected_a.size(); ++i) {
    gate.require(plan.a_credits()[i] <= expected_a[i], "overcredited A endpoint");
    if (strategy == Strategy::DualBlocks) {
      gate.require(plan.a_credits()[i] == expected_a[i],
                   "dual traversal lost a qualified A witness before saturation");
    }
  }
  for (std::size_t i = 0; i < expected_b.size(); ++i) {
    gate.require(plan.b_credits()[i] <= expected_b[i], "overcredited B endpoint");
    if (strategy == Strategy::DualBlocks) {
      gate.require(plan.b_credits()[i] == expected_b[i],
                   "dual traversal lost a qualified B witness before saturation");
    }
  }
  const auto pair_count = static_cast<std::uint64_t>(input.a.size()) * input.b.size();
  gate.require(plan.total_pairs() == pair_count, "wrong total rectangle mass");
  for (const auto& block : plan.blocks()) {
    gate.require(block.a.first < block.a.last && block.b.first < block.b.last &&
                     block.a.last <= plan.a_order().size() &&
                     block.b.last <= plan.b_order().size(),
                 "candidate block has invalid ranges");
  }
  std::set<std::pair<std::size_t, std::size_t>> expanded;
  plan.for_each_candidate([&](std::size_t a, std::size_t b) {
    gate.require(input.a.first <= a && a < input.a.last &&
                     input.b.first <= b && b < input.b.last,
                 "candidate expansion did not preserve original point IDs");
    gate.require(expanded.emplace(a, b).second, "candidate pair emitted twice");
  });
  gate.require(expanded.size() == plan.candidate_pairs(),
               "descriptor count disagrees with explicit expansion");
  for (std::size_t a = input.a.first; a < input.a.last; ++a) {
    for (std::size_t b = input.b.first; b < input.b.last; ++b) {
      const unsigned credits = core + plan.a_credits()[a - input.a.first] +
                               plan.b_credits()[b - input.b.first];
      const bool keep = threshold != 0 && credits < threshold;
      gate.require(plan.keeps(a, b) == keep && expanded.contains({a, b}) == keep,
                   "keeps/expansion disagrees with the certified credit sum");
      if (keep) {
        ++gate.retained_pairs;
      } else if (threshold != 0) {
        ++gate.killed_pairs;
        gate.require(oracle::point_credit(input.points, a, b, lane, threshold) >=
                         threshold,
                     "rejected pair lacks enough distinct exact Wq witnesses");
      }
    }
  }
  gate.credited_blocks += plan.work().credited_blocks;
  gate.noncredit_blocks += plan.work().noncredit_blocks;
  gate.leaf_pairs += plan.work().leaf_pairs;
  gate.saturated_tasks += plan.work().saturated_tasks;
}

void plan_gate(Gate& gate) {
  const auto line = line_input(64, 64);
  for (const auto lane : lanes) {
    for (const auto strategy : strategies) {
      check_plan(gate, line, 10, 8, lane, strategy);
      const auto plan = mhgp8::make_credit_plan(
          mhgp8::prepare_rectangle(line, 10, 8), lane, strategy);
      const auto h = oracle::threshold(10, lane);
      gate.require(plan.candidate_pairs() == h * (h + 1) / 2,
                   "64+64 collinear residual is not the exact triangular count");
    }
  }
  Generator random;
  for (unsigned sample = 0; sample < 12; ++sample) {
    const auto input = clusters_input(random, 2 + sample, 3 + sample % 7);
    for (const auto kmax : {1U, 5U, 10U}) {
      for (const auto lane : lanes) {
        for (const auto strategy : strategies) {
          check_plan(gate, input, kmax, std::array{8U, 10U, 12U}[sample % 3],
                       lane, strategy);
        }
      }
    }
  }
  // Offset ranges, a gap, reversed factor order and a non-witness proposal.
  auto offset = line_input(5, 7);
  offset.points.insert(offset.points.begin(), {0, 65535, 0});
  offset.points.insert(offset.points.begin() + 6, {30000, 1000, 1000});
  offset.a = {1, 6};
  offset.b = {7, 14};
  offset.core_candidates = {0, 6};
  auto core = line_input(7, 9);
  for (unsigned i = 0; i < 10; ++i) {
    core.core_candidates.push_back(core.points.size());
    core.points.push_back({static_cast<std::uint16_t>(30000 + i), 1000, 1000});
  }
  for (const auto lane : lanes) {
    for (const auto strategy : strategies) {
      check_plan(gate, offset, 10, 12, lane, strategy);
      auto reversed = offset;
      std::swap(reversed.a, reversed.b);
      check_plan(gate, reversed, 5, 10, lane, strategy);
      check_plan(gate, core, 10, 8, lane, strategy);
      auto partial = core;
      partial.core_candidates.resize(2);
      check_plan(gate, partial, 10, 8, lane, strategy);
      check_plan(gate, line_input(1, 1), 1, 1, lane, strategy);
      RectangleInput sheets;
      sheets.a = {0, 9};
      sheets.b = {9, 18};
      for (unsigned i = 0; i < 18; ++i) {
        sheets.points.push_back({static_cast<std::uint16_t>(i < 9 ? 1000 : 60000),
                                  static_cast<std::uint16_t>(1000 + i % 9), 1000});
      }
      check_plan(gate, sheets, 10, 8, lane, strategy);
      const auto sheet_plan = mhgp8::make_credit_plan(
          mhgp8::prepare_rectangle(sheets, 10, 8), lane, strategy);
      gate.require(sheet_plan.candidate_pairs() == 81,
                   "negative witness blocks incorrectly killed sheet anchors");
    }
  }
  // Identical local witness populations must not be counted again as core.
  const auto single_core = mhgp8::prepare_rectangle(
      RectangleInput{{{0, 0, 0}, {100, 0, 0}, {50, 0, 0}}, {0, 1}, {1, 2}, {2}},
      2, 8);
  const auto one_credit = mhgp8::make_credit_plan(single_core, Lane::Q2, Strategy::Pool);
  gate.require(one_credit.core_credit() == 1 && one_credit.keeps(0, 1),
               "one identified core site counted twice at threshold two");

  auto owned = mhgp8::prepare_rectangle(line_input(3, 3), 5, 8);
  std::weak_ptr<const mhgp8::PreparedRectangle> weak = owned;
  const auto persistent = mhgp8::make_credit_plan(owned, Lane::Q2, Strategy::Pool);
  const Point3 first_point = owned->points()[0];
  owned.reset();
  gate.require(!weak.expired() && persistent.rectangle().points()[0] == first_point,
               "plan did not retain its immutable input owner");
  gate.require(!std::is_default_constructible_v<mhgp8::PreparedRectangle> &&
                   !std::is_default_constructible_v<mhgp8::CreditPlan>,
               "an uncertified default rectangle or plan is publicly constructible");
  gate.require(!std::is_copy_constructible_v<mhgp8::PreparedRectangle> &&
                   !std::is_copy_assignable_v<mhgp8::PreparedRectangle> &&
                   !std::is_move_constructible_v<mhgp8::PreparedRectangle> &&
                   !std::is_move_assignable_v<mhgp8::PreparedRectangle>,
               "mutable copied owner can be reassigned behind certified credits");
  // Auditor's physical exploit: the old line has one residual pair at K=1,
  // but rebinding that same owner to transverse sites would require four.
  // Shared immutable factory owners remain usable; copying/reassignment of
  // the underlying object is forbidden, not just its default construction.
  const RectangleInput owner_line{
      {{0, 0, 0}, {1, 0, 0}, {100, 0, 0}, {101, 0, 0}}, {0, 2}, {2, 4}, {}};
  const RectangleInput owner_sheet{
      {{0, 0, 0}, {0, 10, 0}, {1000, 0, 0}, {1000, 10, 0}}, {0, 2}, {2, 4}, {}};
  check_plan(gate, owner_line, 1, 8, Lane::Q2, Strategy::DualBlocks);
  check_plan(gate, owner_sheet, 1, 8, Lane::Q2, Strategy::DualBlocks);
  const auto first_owner = mhgp8::prepare_rectangle(owner_line, 1, 8);
  auto owner_alias = first_owner;
  const auto first_plan = mhgp8::make_credit_plan(first_owner, Lane::Q2, Strategy::DualBlocks);
  owner_alias = mhgp8::prepare_rectangle(owner_sheet, 1, 8);
  const auto second_plan = mhgp8::make_credit_plan(owner_alias, Lane::Q2, Strategy::DualBlocks);
  gate.require(first_plan.candidate_pairs() == 1 && second_plan.candidate_pairs() == 4 &&
                   &first_plan.rectangle() != &second_plan.rectangle(),
               "reassigning a shared pointer changed an existing plan owner");
  // A vector move preserves existing mutable element aliases. The public
  // trust boundary must copy before certifying, even when called with move.
  // Check the non-consuming signature before retaining/writing any alias:
  // an old by-value consuming factory could otherwise destroy that buffer.
  using CopyingFactory = mhgp8::RectanglePtr (*)(const RectangleInput&, unsigned, unsigned);
  // Select the coordinate-owning adapter explicitly now that a second
  // overload accepts an already certified cloud. This cast only compiles
  // for the required const-reference (non-consuming) function signature.
  gate.require(std::is_same_v<decltype(static_cast<CopyingFactory>(
                   &mhgp8::prepare_rectangle)), CopyingFactory>,
               "rectangle factory can consume the source buffer behind mutable aliases");
  for (const auto strategy : {Strategy::Pool, Strategy::DualBlocks, Strategy::Tubes}) {
    auto source = owner_line;
    Point3* const source_alias = source.points.data();
    const auto copied_owner = mhgp8::prepare_rectangle(std::move(source), 1, 8);
    gate.require(source.points.data() == source_alias && source.points.size() == 4 &&
                     source_alias != copied_owner->points().data(),
                 "factory did not create distinct storage while preserving its source");
    const auto copied_plan = mhgp8::make_credit_plan(copied_owner, Lane::Q2, strategy);
    const auto before_a = copied_owner->a_box();
    const auto before_b = copied_owner->b_box();
    gate.require(copied_plan.candidate_pairs() == 1,
                 "input-buffer-alias positive control is vacuous");
    for (std::size_t i = 0; i < source.points.size(); ++i) {
      source_alias[i] = owner_sheet.points[i];
    }
    gate.require(source.points == owner_sheet.points &&
                     std::equal(copied_owner->points().begin(), copied_owner->points().end(),
                                owner_line.points.begin(), owner_line.points.end()) &&
                     copied_owner->a_box().low == before_a.low &&
                     copied_owner->a_box().high == before_a.high &&
                     copied_owner->b_box().low == before_b.low &&
                     copied_owner->b_box().high == before_b.high,
                 "source-buffer mutation changed the already certified geometry");
    const auto after_mutation = mhgp8::make_credit_plan(copied_owner, Lane::Q2, strategy);
    const auto fresh_owner = mhgp8::prepare_rectangle(source, 1, 8);
    const auto fresh_plan = mhgp8::make_credit_plan(fresh_owner, Lane::Q2, strategy);
    unsigned alias_mutant_losses = 0;
    for (std::size_t a = 0; a < 2; ++a) {
      for (std::size_t b = 2; b < 4; ++b) {
        gate.require(copied_plan.keeps(a, b) == after_mutation.keeps(a, b),
                     "source mutation changed credits of a plan from the private owner");
        if (oracle::point_credit(copied_owner->points(), a, b, Lane::Q2, 1) == 0) {
          gate.require(copied_plan.keeps(a, b),
                       "old private plan lost a valid q2 pair after source mutation");
        }
        if (oracle::point_credit(source.points, a, b, Lane::Q2, 1) == 0) {
          gate.require(fresh_plan.keeps(a, b),
                       "newly certified transverse input lost a valid q2 pair");
          // This is exactly the invalid result of sharing source_alias with
          // the old plan: old credits applied to the new transverse geometry.
          alias_mutant_losses += static_cast<unsigned>(!copied_plan.keeps(a, b));
        }
      }
    }
    gate.require(copied_plan.candidate_pairs() == 1 && fresh_plan.candidate_pairs() == 4 &&
                     alias_mutant_losses == 3,
                 "retained-buffer-alias mutant did not expose its three lost pairs");
  }
  auto unordered_core = owner_line;
  unordered_core.points.push_back({50, 0, 0});
  unordered_core.points.push_back({60, 0, 0});
  unordered_core.core_candidates = {5, 4};
  const auto core_copy = mhgp8::prepare_rectangle(unordered_core, 2, 8);
  gate.require(unordered_core.core_candidates == std::vector<std::size_t>{5, 4} &&
                   core_copy->core_credit(Lane::Q2) == 2,
               "factory sorted the caller's core proposals instead of a private copy");
  const RectangleInput equality{
      {{0, 0, 0}, {1, 0, 0}, {9, 0, 0}, {10, 0, 0}}, {0, 2}, {2, 4}, {}};
  check_plan(gate, equality, 5, 8, Lane::Q2, Strategy::DualBlocks);
  gate.rejects<std::invalid_argument>(
      [&] { static_cast<void>(mhgp8::prepare_rectangle(equality, 5, 10)); },
      "s=10 accepted a rectangle separated only at s=8");
}

void rejection_gate(Gate& gate) {
  const auto valid = line_input(2, 3);
  const auto bad_input = [&](RectangleInput input, const std::string& message) {
    gate.rejects<std::invalid_argument>(
        [&] { static_cast<void>(mhgp8::prepare_rectangle(input, 10, 8)); }, message);
  };
  for (const auto kmax : {0U, 11U, std::numeric_limits<unsigned>::max()}) {
    gate.rejects<std::invalid_argument>(
        [&] { static_cast<void>(mhgp8::prepare_rectangle(valid, kmax, 8)); },
        "unsupported Kmax accepted");
  }
  gate.rejects<std::invalid_argument>(
      [&] { static_cast<void>(mhgp8::prepare_rectangle(valid, 10, 0)); },
      "zero separation accepted");
  auto changed = valid;
  changed.points[0] = changed.points[1];
  bad_input(changed, "duplicate coordinates accepted");
  changed = valid;
  changed.a = {0, 0};
  bad_input(changed, "empty factor accepted");
  changed.a = {2, 1};
  bad_input(changed, "inverted factor accepted");
  changed = valid;
  changed.b.last = changed.points.size() + 1;
  bad_input(changed, "out-of-range factor accepted");
  changed = valid;
  changed.b.first = 1;
  bad_input(changed, "overlapping factors accepted");
  changed = valid;
  changed.core_candidates = {0};
  bad_input(changed, "A endpoint adopted as a core site");
  changed.core_candidates = {3};
  bad_input(changed, "B endpoint adopted as a core site");
  changed.core_candidates = {valid.points.size()};
  bad_input(changed, "out-of-range core site accepted");
  changed.points.push_back({30000, 1000, 1000});
  changed.core_candidates = {5, 5};
  bad_input(changed, "duplicate core identity accepted");
  changed = RectangleInput{{{0, 0, 0}, {2, 0, 0}, {3, 0, 0}, {5, 0, 0}},
                            {0, 2}, {2, 4}, {}};
  bad_input(changed, "insufficient separation accepted");
  bad_input({}, "empty input accepted");

  const auto rectangle = mhgp8::prepare_rectangle(valid, 10, 8);
  const auto bad_lane = static_cast<Lane>(255);
  gate.rejects<std::invalid_argument>(
      [&] { static_cast<void>(rectangle->threshold(bad_lane)); },
      "invalid threshold lane accepted");
  gate.rejects<std::invalid_argument>(
      [&] { static_cast<void>(rectangle->core_credit(bad_lane)); },
      "invalid core lane accepted");
  gate.rejects<std::invalid_argument>(
      [&] { static_cast<void>(mhgp8::make_credit_plan(rectangle, bad_lane, Strategy::Pool)); },
      "invalid plan lane accepted");
  gate.rejects<std::invalid_argument>(
      [&] { static_cast<void>(mhgp8::make_credit_plan(rectangle, Lane::Q2,
                                                     static_cast<Strategy>(255))); },
      "invalid strategy accepted");
  gate.rejects<std::invalid_argument>(
      [&] { static_cast<void>(mhgp8::make_credit_plan({}, Lane::Q2, Strategy::Pool)); },
      "null owner accepted");
  const auto plan = mhgp8::make_credit_plan(rectangle, Lane::Q2, Strategy::Pool);
  for (const auto& ids : {std::pair<std::size_t, std::size_t>{2, 3},
                          std::pair<std::size_t, std::size_t>{0, 0},
                          std::pair<std::size_t, std::size_t>{0, 5},
                          std::pair<std::size_t, std::size_t>{
                              std::numeric_limits<std::size_t>::max(), 3}}) {
    gate.rejects<std::invalid_argument>(
        [&] { static_cast<void>(plan.keeps(ids.first, ids.second)); },
        "keeps accepted an ID outside its declared factor");
  }
  PredicateWork work;
  const Box3 inverted{{2, 0, 0}, {1, 0, 0}};
  const Box3 box{{0, 0, 0}, {1, 1, 1}};
  gate.rejects<std::invalid_argument>(
      [&] { static_cast<void>(mhgp8::point_witness(bad_lane, {}, {}, {}, work)); },
      "point predicate accepted invalid lane");
  gate.rejects<std::invalid_argument>(
      [&] { static_cast<void>(mhgp8::universal_witness(Lane::Q2, {}, inverted, {}, work)); },
      "universal predicate accepted inverted box");
  gate.rejects<std::invalid_argument>(
      [&] { static_cast<void>(mhgp8::box_witness(Lane::Q3, inverted, box, {}, work)); },
      "box predicate accepted inverted anchors");
  gate.rejects<std::invalid_argument>(
      [&] { static_cast<void>(mhgp8::classify_witness_block(Lane::Q4, box, box,
                                                           inverted, work)); },
      "block predicate accepted inverted witnesses");
  gate.rejects<std::out_of_range>(
      [] { static_cast<void>(Point3{}[3]); }, "invalid coordinate axis accepted");
  gate.rejects<std::out_of_range>(
      [&] { static_cast<void>(mhgp8::box_corner(box, 8)); },
      "invalid box corner accepted");
  std::uint64_t counter = std::numeric_limits<std::uint64_t>::max();
  gate.rejects<std::overflow_error>([&] { mhgp8::counter_add(counter); },
                                     "work counter silently overflowed");
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_p0_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    predicate_gate(gate);
    block_gate(gate);
    plan_gate(gate);
    rejection_gate(gate);
    gate.require(gate.positive_points > 100 && gate.negative_points > 100,
                 "point predicate population is vacuous");
    gate.require(gate.plans >= 200 && gate.killed_pairs > 100 &&
                     gate.retained_pairs > 100,
                 "plan population is vacuous");
    gate.require(gate.credited_blocks > 0 && gate.noncredit_blocks > 0,
                 "dual positive/negative blocks were not exercised");
    gate.require(gate.leaf_pairs > 0 && gate.saturated_tasks > 0,
                 "dual leaf fallback or lazy saturation was not exercised");
    gate.require(gate.strict_mutants == 3 && gate.rejections >= 20,
                 "strict-sign or input rejection coverage is vacuous");
    std::cout << "mhgp8_p0_gate passed checks=" << gate.checks
              << " plans=" << gate.plans << " rejections=" << gate.rejections
              << " strict_mutants=" << gate.strict_mutants
              << " killed_pairs=" << gate.killed_pairs
              << " retained_pairs=" << gate.retained_pairs
              << " credited_blocks=" << gate.credited_blocks
              << " noncredit_blocks=" << gate.noncredit_blocks
              << " leaf_pairs=" << gate.leaf_pairs
              << " saturated_tasks=" << gate.saturated_tasks << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_p0_gate failed: " << error.what() << '\n';
    return 1;
  }
}
