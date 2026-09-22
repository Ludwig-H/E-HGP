// Existing APIs are exercised unchanged. Deliberately false migration rules
// below are scalar models, not mutations of a nonexistent CloudOwner product.
#include "pipeline/q2_census.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
using Pair = std::pair<std::size_t, std::size_t>;
using mhgp8::AxisQ2Mode;
using mhgp8::Lane;
using mhgp8::Q2CensusMode;
using mhgp8::RectangleInput;
using mhgp8::Strategy;
using mhgp8::u64;
struct Stats { u64 runs{}, payloads{}, scalar_site_tests{}, guard_rejections{}, wrong_transfers{}; };
void require(bool yes, const char* reason) {
  if (!yes) throw std::runtime_error(reason);
}

unsigned direct_depth(const mhgp8::PreparedRectangle& owner, Pair pair, Stats& stats) {
  const auto a = owner.points()[pair.first], b = owner.points()[pair.second];
  unsigned depth = 0;
  for (const auto z : owner.points()) {
    std::int64_t h = 0;
    for (unsigned axis = 0; axis < 3; ++axis)
      h += (std::int64_t(z[axis]) - a[axis]) * (std::int64_t(b[axis]) - z[axis]);
    depth += h > 0;
    ++stats.scalar_site_tests;
  }
  return depth;
}

std::vector<Pair> census(const mhgp8::Q2CensusIndex& index, const mhgp8::AxisQ2Plan& plan,
                         Stats& stats) {
  std::vector<Pair> first;
  bool has_first = false;
  for (auto mode : {Q2CensusMode::Pairwise, Q2CensusMode::SharedBlocks}) {
    std::vector<Pair> pairs;
    const auto result = mhgp8::run_q2_census(index, plan, mode, [&](const mhgp8::Q2Support& s) {
      const Pair pair{s.a_id, s.b_id};
      require(direct_depth(plan.rectangle(), pair, stats) == s.interior.size(),
              "census interior differs from scalar all-site depth");
      require(s.interior.size() < plan.rectangle().kmax(), "census emitted a saturated support");
      require(std::find(s.shell.begin(), s.shell.end(), s.a_id) != s.shell.end() &&
              std::find(s.shell.begin(), s.shell.end(), s.b_id) != s.shell.end(),
              "census shell lost a support endpoint");
      pairs.push_back(pair); ++stats.payloads;
    });
    std::sort(pairs.begin(), pairs.end());
    require(std::adjacent_find(pairs.begin(), pairs.end()) == pairs.end(), "duplicate support");
    require(result.accepted_pairs == pairs.size() && result.candidate_pairs == plan.candidate_pairs() &&
            result.accepted_pairs + result.rejected_pairs == result.candidate_pairs,
            "census support accounting mismatch");
    if (has_first) require(first == pairs, "census modes disagree");
    else { first = pairs; has_first = true; }
    ++stats.runs;
  }
  return first;
}

template <class Function>
void rejected(Function&& action, Stats& stats) {
  bool caught = false;
  try { action(); } catch (const std::invalid_argument&) { caught = true; }
  require(caught, "current owner guard accepted the wrong rectangle");
  ++stats.guard_rejections;
}

void rectangle_scope(Stats& stats) {
  const std::vector<mhgp8::Point3> input{{100, 0, 0}, {101, 0, 0}, {200, 0, 0}, {0, 0, 0}};
  // Same original data/ID namespace; the current factory makes two private owners.
  const auto r1 = mhgp8::prepare_rectangle(RectangleInput{input, {0, 2}, {2, 3}, {}}, 1, 12);
  const auto r2 = mhgp8::prepare_rectangle(RectangleInput{input, {0, 2}, {3, 4}, {}}, 1, 12);
  const auto index1 = mhgp8::make_q2_census_index(r1);
  const auto index2 = mhgp8::make_q2_census_index(r2);
  require(r1->core_credit(Lane::Q2) == 0 && r2->core_credit(Lane::Q2) == 0, "unexpected core");
  for (auto strategy : {Strategy::Pool, Strategy::DualBlocks, Strategy::Tubes}) {
    const auto c1 = mhgp8::make_credit_plan(r1, Lane::Q2, strategy);
    const auto c2 = mhgp8::make_credit_plan(r2, Lane::Q2, strategy);
    require(c1.a_credits().size() == 2 && c1.a_credits()[0] == 1 && c1.a_credits()[1] == 0 &&
            c2.a_credits().size() == 2 && c2.a_credits()[0] == 0 && c2.a_credits()[1] == 1 &&
            c1.b_credits()[0] == 0 && c2.b_credits()[0] == 0, "unexpected local credits");
    const auto p1 = mhgp8::make_axis_q2_plan(r1, AxisQ2Mode::Additive, &c1);
    const auto p2 = mhgp8::make_axis_q2_plan(r2, AxisQ2Mode::Additive, &c2);
    require(census(*index1, p1, stats) == std::vector<Pair>{{1, 2}} &&
            census(*index2, p2, stats) == std::vector<Pair>{{0, 3}}, "unexpected true supports");
    // FALSE MODEL: equal cloud and equal cardinalities permit reusing c1 on r2.
    const bool wrong_keeps_required = c1.a_credits()[0] + c1.b_credits()[0] < r2->kmax();
    const bool wrong_keeps_other = c1.a_credits()[1] + c1.b_credits()[0] < r2->kmax();
    require(!wrong_keeps_required && wrong_keeps_other && c2.keeps(0, 3) &&
            direct_depth(*r2, {0, 3}, stats) == 0 && direct_depth(*r2, {1, 3}, stats) == 1,
            "false rectangle-transfer model did not lose the valid support");
    ++stats.wrong_transfers;
    rejected([&] { static_cast<void>(mhgp8::make_axis_q2_plan(r2, AxisQ2Mode::Additive, &c1)); }, stats);
    if (strategy == Strategy::Pool)
      rejected([&] { static_cast<void>(mhgp8::run_q2_census(*index1, p2, Q2CensusMode::Pairwise,
                                      [](const auto&) {})); }, stats);
  }
}

void threshold_scope(Stats& stats) {
  const RectangleInput input{{{0, 0, 0}, {100, 0, 0}, {50, 0, 0}}, {0, 1}, {1, 2}, {}};
  const auto r1 = mhgp8::prepare_rectangle(input, 1, 12);
  const auto r2 = mhgp8::prepare_rectangle(input, 2, 12);
  const auto index1 = mhgp8::make_q2_census_index(r1), index2 = mhgp8::make_q2_census_index(r2);
  const auto p1 = mhgp8::make_axis_q2_plan(r1), p2 = mhgp8::make_axis_q2_plan(r2);
  require(p1.candidate_pairs() == 1 && p2.candidate_pairs() == 1, "threshold fixture was prefiltered");
  require(census(*index1, p1, stats).empty() && census(*index2, p2, stats) == std::vector<Pair>{{0, 1}},
          "threshold fixture census mismatch");
  const auto depth = direct_depth(*r2, {0, 1}, stats);
  // FALSE MODEL: a globally reused geometric index still chooses its first rectangle's Kmax.
  require(depth == 1 && !(depth < index1->rectangle().kmax()) && depth < p2.rectangle().kmax(),
          "false index-threshold model did not lose the valid support");
  rejected([&] { static_cast<void>(mhgp8::run_q2_census(*index1, p2, Q2CensusMode::SharedBlocks,
                                  [](const auto&) {})); }, stats);
}

void permutation_scope(Stats& stats) {
  const RectangleInput input{{{0, 0, 0}, {0, 1, 0}, {0, 0, 1},
                              {100, 2, 2}, {100, 2, 1}}, {0, 3}, {3, 5}, {}};
  const auto owner = mhgp8::prepare_rectangle(input, 2, 12);
  const auto index = mhgp8::make_q2_census_index(owner);
  const auto independent = mhgp8::make_axis_q2_plan(owner, AxisQ2Mode::Independent);
  const auto additive = mhgp8::make_axis_q2_plan(owner, AxisQ2Mode::Additive);
  require(independent.b_order().size() == 2 && additive.b_order().size() == 2 &&
          independent.b_order()[0] == 3 && independent.b_order()[1] == 4 &&
          additive.b_order()[0] == 4 && additive.b_order()[1] == 3,
          "fixture did not produce different actual B permutations");
  require(independent.work().tree_nodes == 0 && additive.work().tree_nodes > 0,
          "fixture did not exercise optional spatial reordering");
  const auto actual = census(*index, additive, stats);
  require(actual == census(*index, independent, stats) && actual.size() == 5 &&
          std::find(actual.begin(), actual.end(), Pair{0, 4}) != actual.end(), "wrong true permutation output");
  bool selected = false;
  for (const auto& block : additive.blocks())
    if (block.a_id == 0 && block.b.first == 0 && block.b.last == 1) selected = true;
  require(selected, "required additive range [0,1) absent");
  // FALSE MODEL: reuse the singleton query box at rank zero from the other plan.
  const auto intended_depth = direct_depth(*owner, {0, additive.b_order()[0]}, stats);
  const auto wrong_box_depth = direct_depth(*owner, {0, independent.b_order()[0]}, stats);
  require(intended_depth == 1 && wrong_box_depth == 3 && intended_depth < owner->kmax() &&
          !(wrong_box_depth < owner->kmax()), "false B-order cache model did not reject the valid range");
}
}  // namespace

int main() {
  try {
    Stats stats;
    rectangle_scope(stats); threshold_scope(stats); permutation_scope(stats);
    require(stats.runs == 20 && stats.payloads == 34 && stats.guard_rejections == 5 &&
            stats.wrong_transfers == 3 && stats.scalar_site_tests > 100, "vacuous identity campaign");
    std::cout << "{\"census_runs\":" << stats.runs << ",\"payloads_checked\":" << stats.payloads
              << ",\"scalar_site_tests\":" << stats.scalar_site_tests
              << ",\"current_owner_guard_rejections\":" << stats.guard_rejections
              << ",\"false_rectangle_models_refuted\":" << stats.wrong_transfers
              << ",\"false_threshold_model_refuted\":true,\"false_permutation_model_refuted\":true"
              << ",\"r1_a_credits\":[1,0],\"r2_a_credits\":[0,1]"
              << ",\"r1_true_support\":[1,2],\"r2_true_support\":[0,3]"
              << ",\"independent_b_order\":[3,4],\"additive_b_order\":[4,3]}\n";
  } catch (const std::exception& error) {
    std::cerr << "cloud/rectangle identity audit: " << error.what() << '\n';
    return 1;
  }
}
