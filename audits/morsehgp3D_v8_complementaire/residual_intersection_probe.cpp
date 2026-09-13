// Audit-only composition of existing plans. No product implementation.
#include "pipeline/axis_q2.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
using mhgp8::u64;
void require(bool value, const char* reason) {
  if (!value) throw std::runtime_error(reason);
}

struct Fragment {
  std::size_t a_id;
  unsigned threshold;
  mhgp8::Range range;
};
struct Result {
  std::vector<std::vector<std::size_t>> selected;
  std::vector<Fragment> fragments;
  u64 candidates{};
  u64 rank_tests{};
  u64 rank_entries{};
  u64 stored_ids{};
  u64 input_fragments{};
};

Result intersect(const mhgp8::RectanglePtr& owner, const mhgp8::CreditPlan& credit,
                 const mhgp8::AxisQ2Plan& axis) {
  // Call rectangle() first: moved-from input access must fail closed.
  const auto& c = credit.rectangle();
  const auto& x = axis.rectangle();
  require(owner && owner.get() == &c && owner.get() == &x, "different owner");
  require(credit.lane() == mhgp8::Lane::Q2, "intersection requires q2");
  const unsigned h = credit.threshold() - credit.core_credit();
  require(h == axis.need(), "inconsistent need");
  Result result;
  if (h == 0 || credit.candidate_pairs() == 0 || axis.candidate_pairs() == 0)
    return result;
  const auto a = owner->a_range();
  const auto b = owner->b_range();
  require(credit.a_credits().size() == a.size() &&
          credit.b_credits().size() == b.size() && axis.b_order().size() == b.size(),
          "inconsistent input arrays");
  result.selected.resize(h + 1);
  std::vector<std::vector<std::size_t>> rank(h + 1);
  for (unsigned t = 1; t <= h; ++t) {
    rank[t].push_back(0);
    ++result.rank_entries;
    for (std::size_t i = 0; i < b.size(); ++i) {
      const auto id = axis.b_order()[i];
      require(id >= b.first && id < b.last, "foreign input ID");
      const auto value = credit.b_credits()[id - b.first];
      ++result.rank_tests;
      if (value < t) {  // MUTATION_STRICT_BOUNDARY
        result.selected[t].push_back(id);
        ++result.stored_ids;
      }
      rank[t].push_back(result.selected[t].size());
      ++result.rank_entries;
    }
  }
  for (const auto& block : axis.blocks()) {
    ++result.input_fragments;
    require(block.a_id >= a.first && block.a_id < a.last &&
            block.b.first <= block.b.last && block.b.last <= b.size(),
            "invalid input fragment");
    const unsigned a_credit = credit.a_credits()[block.a_id - a.first];
    if (a_credit >= h) continue;
    const unsigned t = h - a_credit;
    const mhgp8::Range selected{rank[t][block.b.first], rank[t][block.b.last]};
    if (selected.size() != 0) {
      result.fragments.push_back({block.a_id, t, selected});
      mhgp8::counter_add(result.candidates, selected.size());
    }
  }
  require(result.fragments.size() <= axis.blocks().size(), "fragment expansion");
  return result;
}

mhgp8::RectangleInput fixture(bool reverse, bool core) {
  mhgp8::RectangleInput input;
  input.points = {{65000, 65000, 65000}, {65001, 65000, 65000},
                  {65002, 65000, 65000}};
  const auto add_factor = [&](unsigned side) {
    const auto first = input.points.size();
    constexpr unsigned m = 60;
    for (unsigned i = 0; i < m; ++i) {
      // Deliberately permute original IDs, independently of geometric sorting.
      const unsigned k = (17 * i + 5) % m;
      input.points.push_back({static_cast<std::uint16_t>(1000 + side * 40000 + k / 20),
                              static_cast<std::uint16_t>(1000 + (k / 5) % 4),
                              static_cast<std::uint16_t>(1000 + k % 5)});
    }
    return mhgp8::Range{first, input.points.size()};
  };
  if (reverse) { input.b = add_factor(1); input.a = add_factor(0); }
  else { input.a = add_factor(0); input.b = add_factor(1); }
  if (core) {
    input.core_candidates.push_back(input.points.size());
    input.points.push_back({21000, 1000, 1000});
  }
  return input;
}

template <class Function>
void must_reject(Function function, u64& rejections) {
  bool rejected = false;
  try { function(); } catch (const std::exception&) { rejected = true; }
  require(rejected, "invalid plan combination accepted");
  ++rejections;
}

void run() {
  u64 plans = 0, pairs_checked = 0, expanded_pairs = 0, improvements_credit = 0;
  u64 improvements_axis = 0, improvements_both = 0, empty = 0, rejections = 0;
  u64 rank_tests = 0, stored_ids = 0, fragments = 0, input_fragments = 0;
  for (const bool reverse : {false, true})
    for (const bool core : {false, true})
      for (const unsigned h : {1U, 2U, 5U, 10U}) {
        const auto owner = mhgp8::prepare_rectangle(fixture(reverse, core), h, 12);
        const auto axis = mhgp8::make_axis_q2_plan(owner);
        for (const auto strategy : {mhgp8::Strategy::Pool, mhgp8::Strategy::DualBlocks,
                                    mhgp8::Strategy::Tubes}) {
          const auto credit = mhgp8::make_credit_plan(owner, mhgp8::Lane::Q2, strategy);
          const auto result = intersect(owner, credit, axis);
          const auto a = owner->a_range();
          const auto b = owner->b_range();
          std::vector<bool> coverage(a.size() * b.size(), false);
          u64 expanded = 0;
          for (const auto& fragment : result.fragments) {
            require(fragment.threshold < result.selected.size() &&
                    fragment.range.first < fragment.range.last &&
                    fragment.range.last <= result.selected[fragment.threshold].size(),
                    "invalid output fragment");
            for (std::size_t i = fragment.range.first; i < fragment.range.last; ++i) {
              const auto bid = result.selected[fragment.threshold][i];
              require(fragment.a_id >= a.first && fragment.a_id < a.last &&
                      bid >= b.first && bid < b.last, "foreign output ID");
              const auto key = (fragment.a_id - a.first) * b.size() + bid - b.first;
              require(!coverage[key], "duplicated intersection pair");
              coverage[key] = true;
              ++expanded;
            }
          }
          for (std::size_t aid = a.first; aid < a.last; ++aid)
            for (std::size_t bid = b.first; bid < b.last; ++bid) {
              require(coverage[(aid - a.first) * b.size() + bid - b.first] ==
                      (credit.keeps(aid, bid) && axis.keeps(aid, bid)),
                      "intersection differs from conjunction");
              ++pairs_checked;
            }
          require(expanded == result.candidates, "candidate count mismatch");
          const bool better_c = result.candidates < credit.candidate_pairs();
          const bool better_x = result.candidates < axis.candidate_pairs();
          improvements_credit += better_c;
          improvements_axis += better_x;
          improvements_both += better_c && better_x;
          empty += result.candidates == 0;
          rank_tests += result.rank_tests;
          stored_ids += result.stored_ids;
          fragments += result.fragments.size();
          input_fragments += result.input_fragments;
          expanded_pairs += expanded;
          ++plans;
        }
      }
  const auto owner = mhgp8::prepare_rectangle(fixture(false, false), 2, 12);
  const auto twin = mhgp8::prepare_rectangle(fixture(false, false), 2, 12);
  auto credit = mhgp8::make_credit_plan(owner, mhgp8::Lane::Q2, mhgp8::Strategy::Pool);
  auto axis = mhgp8::make_axis_q2_plan(owner);
  const auto foreign = mhgp8::make_axis_q2_plan(twin);
  const auto q3 = mhgp8::make_credit_plan(owner, mhgp8::Lane::Q3, mhgp8::Strategy::Pool);
  must_reject([&] { static_cast<void>(intersect(owner, credit, foreign)); }, rejections);
  must_reject([&] { static_cast<void>(intersect(twin, credit, axis)); }, rejections);
  must_reject([&] { static_cast<void>(intersect(owner, q3, axis)); }, rejections);
  const auto moved_credit = std::move(credit);
  must_reject([&] { static_cast<void>(intersect(owner, credit, axis)); }, rejections);
  const auto moved_axis = std::move(axis);
  must_reject([&] { static_cast<void>(intersect(owner, moved_credit, axis)); }, rejections);
  require(moved_axis.candidate_pairs() > 0, "nonvacuous move fixture");
  require(plans == 48 && pairs_checked == 172800 && improvements_both > 0 && empty > 0 &&
          rejections == 5 && fragments > 0 && expanded_pairs > 0, "vacuous campaign");
  std::cout << "{\"plans\":" << plans << ",\"pairs_checked\":" << pairs_checked
            << ",\"expanded_pairs\":" << expanded_pairs << ",\"empty_plans\":" << empty
            << ",\"improvements_credit\":" << improvements_credit
            << ",\"improvements_axis\":" << improvements_axis
            << ",\"improvements_both\":" << improvements_both
            << ",\"rank_tests\":" << rank_tests << ",\"stored_ids\":" << stored_ids
            << ",\"input_fragments\":" << input_fragments
            << ",\"output_fragments\":" << fragments
            << ",\"invalid_combinations_rejected\":" << rejections << "}\n";
}
}  // namespace

int main() {
  try { run(); }
  catch (const std::exception& error) {
    std::cerr << "residual intersection audit: " << error.what() << '\n';
    return 1;
  }
}
