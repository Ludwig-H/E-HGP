// New audit of the published f5430f57 API. No inherited qualification.
#include "pipeline/axis_q2.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <new>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace allocation {
bool armed = false;
bool injected = false;
std::size_t remaining = 0;
std::size_t calls = 0;
std::size_t live = 0;
void arm(std::size_t position) noexcept {
  armed = true; injected = false; remaining = position; calls = 0;
}
void disarm() noexcept { armed = false; }
[[gnu::noinline]] void* allocate(std::size_t bytes) {
  if (armed) {
    ++calls;
    if (remaining == 0) {
      armed = false; injected = true;
      throw std::bad_alloc();
    }
    --remaining;
  }
  if (void* pointer = std::malloc(bytes == 0 ? 1 : bytes)) { ++live; return pointer; }
  throw std::bad_alloc();
}
[[gnu::noinline]] void release(void* pointer) noexcept {
  if (pointer != nullptr) { --live; std::free(pointer); }
}
}  // namespace allocation
void* operator new(std::size_t n) { return allocation::allocate(n); }
void* operator new[](std::size_t n) { return allocation::allocate(n); }
void operator delete(void* p) noexcept { allocation::release(p); }
void operator delete[](void* p) noexcept { allocation::release(p); }
void operator delete(void* p, std::size_t) noexcept { allocation::release(p); }
void operator delete[](void* p, std::size_t) noexcept { allocation::release(p); }

namespace {
using mhgp8::AxisQ2Mode;
using mhgp8::AxisQ2Plan;
using mhgp8::CreditPlan;
using mhgp8::Lane;
using mhgp8::RectanglePtr;
using mhgp8::Strategy;
using mhgp8::u64;
void require(bool yes, const char* cause) {
  if (!yes) throw std::runtime_error(cause);
}
static_assert(!std::is_copy_constructible_v<AxisQ2Plan>);
static_assert(!std::is_copy_assignable_v<AxisQ2Plan>);
static_assert(!std::is_move_assignable_v<AxisQ2Plan>);
static_assert(std::is_nothrow_move_constructible_v<AxisQ2Plan>);

auto work_fields(const mhgp8::AxisQ2Work& w) {
  return std::array{w.sort_passes, w.sorted_sites, w.sort_comparisons, w.columns,
      w.constrained_anchors, w.slab_bound_updates, w.tree_point_visits, w.tree_nodes,
      w.query_nodes, w.contained_nodes, w.disjoint_nodes, w.whole_factor_accepts,
      w.whole_factor_rejects, w.emitted_blocks, w.max_tree_depth, w.axis_bound_queries,
      w.axis_count_queries, w.axis_rank_comparisons, w.axis_pruned_nodes,
      w.axis_slab_rejects, w.restriction_credit_copies, w.restriction_credit_visits,
      w.restriction_bound_queries, w.restriction_pruned_nodes, w.coalesced_blocks};
}

mhgp8::RectangleInput fixture(bool reverse, bool core) {
  mhgp8::RectangleInput input;
  input.points = {{65000, 65000, 65000}, {65001, 65000, 65000},
                  {65002, 65000, 65000}};
  const auto factor = [&](unsigned side) {
    const auto first = input.points.size();
    for (unsigned i = 0; i < 60; ++i) {
      const unsigned p = (17 * i + 5) % 60;
      input.points.push_back({static_cast<std::uint16_t>(1000 + side * 40000 + p / 20),
                             static_cast<std::uint16_t>(1000 + p / 5 % 4),
                             static_cast<std::uint16_t>(1000 + p % 5)});
    }
    return mhgp8::Range{first, input.points.size()};
  };
  if (reverse) { input.b = factor(1); input.a = factor(0); }
  else { input.a = factor(0); input.b = factor(1); }
  if (core) {
    input.core_candidates.push_back(input.points.size());
    input.points.push_back({21000, 1000, 1000});
  }
  return input;
}

std::int64_t distance2(mhgp8::Point3 a, mhgp8::Point3 b) {
  std::int64_t sum = 0;
  for (unsigned i = 0; i < 3; ++i) {
    const std::int64_t d = std::int64_t(a[i]) - b[i];
    sum += d * d;
  }
  return sum;
}

using Bits = std::vector<std::uint8_t>;
Bits coverage(const AxisQ2Plan& plan) {
  const auto& r = plan.rectangle();
  const auto a = r.a_range(), b = r.b_range();
  Bits bits(a.size() * b.size(), 0);
  u64 count = 0;
  // Inspect ranges first, so no invalid output index is dereferenced.
  for (const auto& block : plan.blocks()) {
    require(block.a_id >= a.first && block.a_id < a.last &&
            block.b.first < block.b.last && block.b.last <= plan.b_order().size(),
            "invalid output fragment");
  }
  plan.for_each_candidate([&](std::size_t aid, std::size_t bid) {
    require(aid >= a.first && aid < a.last && bid >= b.first && bid < b.last,
            "foreign output ID");
    auto& bit = bits[(aid - a.first) * b.size() + bid - b.first];
    require(bit == 0, "duplicate output pair");
    bit = 1; ++count;
  });
  require(count == plan.candidate_pairs(), "descriptor cardinal mismatch");
  for (std::size_t aid = a.first; aid < a.last; ++aid)
    for (std::size_t bid = b.first; bid < b.last; ++bid)
      require(plan.keeps(aid, bid) == bool(bits[(aid - a.first) * b.size() + bid - b.first]),
              "keeps differs from fragments");
  return bits;
}

struct Stats {
  u64 owners{}, minima_distance_tests{}, internal_witness_tests{}, q2_plans{};
  u64 positive_need_plans{}, zero_need_plans{}, api_plans{}, membership_checks{};
  u64 factory_failures{}, factory_successes{}, fault_positions{}, moves{};
  u64 moved_rejections{}, baseline_query_nodes{}, baseline_coalesced{};
};

void minimum_gate(Stats& stats) {
  for (bool reverse : {false, true}) for (bool core : {false, true})
    for (unsigned kmax : {1U, 2U, 5U, 10U}) {
      const auto owner = mhgp8::prepare_rectangle(fixture(reverse, core), kmax, 12);
      const auto a = owner->a_range(), b = owner->b_range();
      std::pair<std::size_t, std::size_t> nearest{a.first, b.first};
      auto minimum = distance2(owner->points()[a.first], owner->points()[b.first]);
      for (std::size_t aid = a.first; aid < a.last; ++aid)
        for (std::size_t bid = b.first; bid < b.last; ++bid) {
          const auto value = distance2(owner->points()[aid], owner->points()[bid]);
          if (value < minimum) { minimum = value; nearest = {aid, bid}; }
          ++stats.minima_distance_tests;
        }
      const auto pa = owner->points()[nearest.first], pb = owner->points()[nearest.second];
      for (const auto range : {a, b}) for (std::size_t z = range.first; z < range.last; ++z) {
        if (z == nearest.first || z == nearest.second) continue;
        // 2H = |a-b|^2 - |a-z|^2 - |b-z|^2, using independent distances.
        require(minimum - distance2(pa, owner->points()[z]) -
                    distance2(pb, owner->points()[z]) <= 0,
                "minimum transversal has an internal strict witness");
        ++stats.internal_witness_tests;
      }
      const unsigned need = owner->threshold(Lane::Q2) - owner->core_credit(Lane::Q2);
      for (const auto strategy : {Strategy::Pool, Strategy::DualBlocks, Strategy::Tubes}) {
        const auto credit = mhgp8::make_credit_plan(owner, Lane::Q2, strategy);
        require(credit.a_credits()[nearest.first - a.first] == 0 &&
                credit.b_credits()[nearest.second - b.first] == 0,
                "minimum transversal receives an internal local credit");
        require(credit.keeps(nearest.first, nearest.second) == (need > 0) &&
                (credit.candidate_pairs() > 0) == (need > 0),
                "local empty residual is not equivalent to zero need");
        if (need > 0) ++stats.positive_need_plans; else ++stats.zero_need_plans;
        ++stats.q2_plans;
        for (auto mode : {AxisQ2Mode::Independent, AxisQ2Mode::Additive}) {
          auto axis = mhgp8::make_axis_q2_plan(owner, mode, &credit);
          require(axis.keeps(nearest.first, nearest.second) == (need > 0) &&
                  (axis.candidate_pairs() > 0) == (need > 0),
                  "axial intersection loses minimum transversal");
          static_cast<void>(coverage(axis));
          ++stats.api_plans;
          stats.membership_checks += a.size() * b.size();
        }
      }
      ++stats.owners;
    }
}

template <class Function>
void moved_reject(Function&& action, Stats& stats) {
  bool refused = false;
  try { action(); } catch (const std::logic_error&) { refused = true; }
  require(refused, "moved source still exposes geometry");
  ++stats.moved_rejections;
}

void factory_gate(Stats& stats, AxisQ2Mode mode, bool saturated) {
  const auto owner = mhgp8::prepare_rectangle(fixture(false, saturated), saturated ? 1 : 2, 12);
  const auto credit = mhgp8::make_credit_plan(owner, Lane::Q2, Strategy::Pool);
  auto baseline = mhgp8::make_axis_q2_plan(owner, mode, &credit);
  const auto expected = coverage(baseline);
  const auto expected_work = work_fields(baseline.work());
  const std::vector<std::uint8_t> ca(credit.a_credits().begin(), credit.a_credits().end());
  const std::vector<std::uint8_t> cb(credit.b_credits().begin(), credit.b_credits().end());
  stats.baseline_query_nodes += baseline.work().query_nodes;
  stats.baseline_coalesced += baseline.work().coalesced_blocks;
  std::size_t failures = 0;
  for (std::size_t position = 0; ; ++position) {
    require(position < 256, "bounded factory allocation loop exceeded");
    const auto references = owner.use_count();
    const auto live = allocation::live;
    std::optional<AxisQ2Plan> created;
    bool caught = false;
    allocation::arm(position);
    try { created.emplace(mhgp8::make_axis_q2_plan(owner, mode, &credit)); }
    catch (const std::bad_alloc&) { caught = true; }
    catch (...) { allocation::disarm(); throw; }
    allocation::disarm();
    require(caught == allocation::injected, "allocation injection was not observed");
    ++stats.fault_positions;
    if (caught) { ++failures; ++stats.factory_failures; }
    else {
      require(created.has_value() && coverage(*created) == expected &&
              work_fields(created->work()) == expected_work, "successful construction changed plan");
      ++stats.factory_successes;
    }
    created.reset();
    require(owner.use_count() == references && allocation::live == live,
            "failed construction leaked storage or owner reference");
    require(std::equal(ca.begin(), ca.end(), credit.a_credits().begin()) &&
            std::equal(cb.begin(), cb.end(), credit.b_credits().begin()) &&
            &credit.rectangle() == owner.get() && coverage(baseline) == expected &&
            work_fields(baseline.work()) == expected_work,
            "construction changed a preexisting plan");
    if (!caught) break;
  }
  require(saturated ? failures == 0 : failures >= 8, "unexercised factory allocations");
  allocation::arm(0);
  const auto moved = std::move(baseline);
  allocation::disarm();
  require(allocation::calls == 0 && moved.mode() == mode && moved.has_restriction() &&
          &moved.rectangle() == owner.get() && coverage(moved) == expected &&
          work_fields(moved.work()) == expected_work, "move changed destination state");
  require(baseline.mode() == AxisQ2Mode::Independent && !baseline.has_restriction() &&
          baseline.need() == 0 && baseline.candidate_pairs() == 0 && baseline.total_pairs() == 0 &&
          baseline.blocks().empty() && baseline.b_order().empty() &&
          work_fields(baseline.work()) == work_fields(mhgp8::AxisQ2Work{}),
          "move did not clear source metadata");
  moved_reject([&] { static_cast<void>(baseline.rectangle()); }, stats);
  moved_reject([&] { static_cast<void>(baseline.keeps(3, 63)); }, stats);
  moved_reject([&] { baseline.for_each_candidate([](auto, auto) {}); }, stats);
  ++stats.moves;
}
}  // namespace

int main() {
  try {
    Stats s;
    minimum_gate(s);
    for (auto mode : {AxisQ2Mode::Independent, AxisQ2Mode::Additive})
      for (bool saturated : {false, true}) factory_gate(s, mode, saturated);
    require(s.owners == 16 && s.q2_plans == 48 && s.positive_need_plans == 42 &&
            s.zero_need_plans == 6 && s.api_plans == 96 && s.membership_checks == 345600 &&
            s.factory_failures >= 16 && s.factory_successes == 4 && s.moves == 4 &&
            s.moved_rejections == 12 && s.baseline_query_nodes > 0 && s.baseline_coalesced > 0,
            "vacuous lifetime/minimum campaign");
    std::cout << "{\"owners\":" << s.owners << ",\"q2_plans\":" << s.q2_plans
              << ",\"positive_need_plans\":" << s.positive_need_plans
              << ",\"zero_need_plans\":" << s.zero_need_plans
              << ",\"minima_distance_tests\":" << s.minima_distance_tests
              << ",\"internal_witness_tests\":" << s.internal_witness_tests
              << ",\"axis_plans\":" << s.api_plans
              << ",\"membership_checks\":" << s.membership_checks
              << ",\"factory_failures\":" << s.factory_failures
              << ",\"factory_successes\":" << s.factory_successes
              << ",\"fault_positions\":" << s.fault_positions
              << ",\"moves_without_allocation\":" << s.moves
              << ",\"moved_rejections\":" << s.moved_rejections
              << ",\"baseline_query_nodes\":" << s.baseline_query_nodes
              << ",\"baseline_coalesced_blocks\":" << s.baseline_coalesced << "}\n";
  } catch (const std::exception& error) {
    allocation::disarm();
    std::cerr << "axis lifetime audit: " << error.what() << '\n';
    return 1;
  }
}
