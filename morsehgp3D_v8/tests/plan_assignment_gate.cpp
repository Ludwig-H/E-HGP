#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "pipeline/local_credits.hpp"

// Explicit v8 port of the failure mechanism documented by the complementary
// auditor in audits/morsehgp3D_v8_complementaire/P0_PLAN_ASSIGNMENT.md.
// This new gate visits every allocating copy position, including batch lanes;
// it never dereferences a retained vector using a changed owner's dimensions.
namespace allocation_failure {

struct State {
  bool armed{};
  bool hit{};
  std::size_t remaining{};
  std::size_t calls{};
};

thread_local State state;

void arm(std::size_t position) noexcept { state = {true, false, position, 0}; }
void disarm() noexcept { state.armed = false; }

// The tested vectors have ordinary alignment. All ordinary scalar/array
// allocation replacements share this one counter; no aligned storage is
// claimed to be exercised by this bounded fixture.
[[gnu::noinline]] void* allocate(std::size_t size) {
  if (state.armed) {
    ++state.calls;
    if (state.remaining == 0) {
      state.hit = true;
      state.armed = false;
      throw std::bad_alloc();
    }
    --state.remaining;
  }
  if (void* result = std::malloc(size == 0 ? 1 : size)) return result;
  throw std::bad_alloc();
}

[[gnu::noinline]] void release(void* pointer) noexcept { std::free(pointer); }

struct Attempt {
  bool caught{};
  bool injected{};
  std::size_t calls{};
};

template <class Function>
Attempt attempt(std::size_t position, Function&& action) {
  arm(position);
  bool caught = false;
  try {
    action();
  } catch (const std::bad_alloc&) {
    caught = true;
  } catch (...) {
    disarm();
    throw;
  }
  disarm();  // Verification, diagnostics and fixtures may allocate after here.
  return {caught, state.hit, state.calls};
}

}  // namespace allocation_failure

void* operator new(std::size_t size) { return allocation_failure::allocate(size); }
void* operator new[](std::size_t size) { return allocation_failure::allocate(size); }
void operator delete(void* pointer) noexcept { allocation_failure::release(pointer); }
void operator delete[](void* pointer) noexcept { allocation_failure::release(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { allocation_failure::release(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { allocation_failure::release(pointer); }

namespace {

using mhgp8::CreditBatch;
using mhgp8::CreditPlan;
using mhgp8::Lane;
using mhgp8::RectangleInput;
using mhgp8::Strategy;
constexpr std::array<Lane, 3> lanes{Lane::Q2, Lane::Q3, Lane::Q4};

struct Gate {
  std::uint64_t checks{};
  std::uint64_t physical_comparisons{};
  std::uint64_t plan_failures{};
  std::uint64_t batch_failures{};
  std::uint64_t successful_assignments{};
  std::uint64_t self_operations{};
  std::uint64_t move_operations{};
  std::uint64_t moved_rejections{};

  void require(bool condition, const char* cause) {
    ++checks;
    if (!condition) throw std::runtime_error(cause);
  }
};

[[nodiscard]] auto work_fields(const mhgp8::Work& w) {
  return std::tie(w.validation_points, w.uniqueness_comparisons,
                  w.pool_selection_tests, w.pool_selected, w.tree_nodes,
                  w.tree_point_visits, w.dual_tasks, w.credited_blocks,
                  w.noncredit_blocks, w.leaf_pairs, w.saturated_tasks,
                  w.max_tree_depth, w.max_task_depth, w.tube_records,
                  w.tube_cells, w.tube_sort_comparisons, w.tube_sweep_tests,
                  w.tube_credited_sites, w.tube_separation_fallbacks,
                  w.predicates.point_tests, w.predicates.universal_queries,
                  w.predicates.q2_axis_terms, w.predicates.corner_tests,
                  w.predicates.block_bound_tests, w.predicates.negative_probes);
}

template <class Left, class Right>
bool same_sequence(const Left& left, const Right& right) {
  return left.size() == right.size() && std::equal(left.begin(), left.end(), right.begin());
}

void same(Gate& gate, const CreditPlan& actual, const CreditPlan& expected) {
  gate.require(!allocation_failure::state.armed, "verification ran while allocation injection was armed");
  gate.require(&actual.rectangle() == &expected.rectangle() &&
                   actual.lane() == expected.lane() && actual.strategy() == expected.strategy() &&
                   actual.threshold() == expected.threshold() &&
                   actual.core_credit() == expected.core_credit() &&
                   actual.total_pairs() == expected.total_pairs() &&
                   actual.candidate_pairs() == expected.candidate_pairs(),
                "assignment mixed owner and scalar plan metadata");
  gate.require(same_sequence(actual.a_credits(), expected.a_credits()) &&
                   same_sequence(actual.b_credits(), expected.b_credits()) &&
                   same_sequence(actual.a_order(), expected.a_order()) &&
                   same_sequence(actual.b_order(), expected.b_order()) &&
                   actual.blocks().size() == expected.blocks().size(),
                "assignment changed only a prefix of the physical plan arrays");
  gate.require(work_fields(actual.work()) == work_fields(expected.work()),
                "assignment changed the work counters independently of its plan");
  const auto ar = actual.rectangle().a_range();
  const auto br = actual.rectangle().b_range();
  gate.require(actual.a_credits().size() == ar.size() && actual.b_credits().size() == br.size(),
                "owner dimensions disagree with retained credit vector lengths");
  for (std::size_t i = 0; i < actual.blocks().size(); ++i) {
    const auto& a = actual.blocks()[i];
    const auto& b = expected.blocks()[i];
    gate.require(a.a.first == b.a.first && a.a.last == b.a.last &&
                     a.b.first == b.b.first && a.b.last == b.b.last &&
                     a.a.first < a.a.last && a.a.last <= actual.a_order().size() &&
                     a.b.first < a.b.last && a.b.last <= actual.b_order().size(),
                  "assignment retained inconsistent residual block bounds");
  }
  std::vector<std::pair<std::size_t, std::size_t>> emitted;
  std::vector<std::pair<std::size_t, std::size_t>> wanted;
  actual.for_each_candidate([&](std::size_t a, std::size_t b) {
    gate.require(ar.first <= a && a < ar.last && br.first <= b && b < br.last,
                  "assignment emitted IDs outside its immutable owner");
    emitted.emplace_back(a, b);
  });
  expected.for_each_candidate([&](std::size_t a, std::size_t b) { wanted.emplace_back(a, b); });
  gate.require(emitted == wanted && emitted.size() == actual.candidate_pairs(),
                "assignment changed the physical candidate stream");
  // Safe only after the checks above establish array lengths and owner IDs.
  for (std::size_t a = ar.first; a < ar.last; ++a) {
    for (std::size_t b = br.first; b < br.last; ++b) {
      gate.require(actual.keeps(a, b) == expected.keeps(a, b),
                    "assignment changed membership in a valid owner range");
    }
  }
  ++gate.physical_comparisons;
}

void same(Gate& gate, const CreditBatch& actual, const CreditBatch& expected) {
  gate.require(work_fields(actual.shared_work()) == work_fields(expected.shared_work()),
                "batch assignment changed shared work before all lanes were copied");
  for (const auto lane : lanes) same(gate, actual.plan(lane), expected.plan(lane));
}

template <class Value>
std::uint64_t strong_copy(Gate& gate, const Value& initial, const Value& source) {
  std::uint64_t failures = 0;
  // Stop at the first successful whole copy. Every earlier allocation in
  // this fixed copy path has therefore been interrupted once, without a cap.
  for (std::size_t position = 0;; ++position) {
    auto target = initial;
    const auto result = allocation_failure::attempt(position, [&] { target = source; });
    if (result.caught) {
      gate.require(result.injected && result.calls == position + 1,
                    "copy failed outside the requested allocation position");
      same(gate, target, initial);
      ++failures;
      continue;
    }
    gate.require(!result.injected && result.calls == position,
                  "allocation sweep did not stop at the first complete copy");
    same(gate, target, source);
    ++gate.successful_assignments;
    break;
  }
  return failures;
}

template <class Function>
void rejects_moved(Gate& gate, Function&& action) {
  bool refused = false;
  try { action(); } catch (const std::logic_error&) { refused = true; }
  gate.require(refused, "moved-from plan exposed geometry without an owner");
  ++gate.moved_rejections;
}

void moved_state(Gate& gate, const CreditPlan& value) {
  gate.require(value.threshold() == 0 && value.core_credit() == 0 &&
                   value.total_pairs() == 0 && value.candidate_pairs() == 0 &&
                   value.a_credits().empty() && value.b_credits().empty() &&
                   value.a_order().empty() && value.b_order().empty() && value.blocks().empty(),
                "moved-from credit plan retained live payload metadata");
  rejects_moved(gate, [&] { static_cast<void>(value.rectangle()); });
  rejects_moved(gate, [&] { static_cast<void>(value.keeps(0, 2)); });
  rejects_moved(gate, [&] { value.for_each_candidate([](std::size_t, std::size_t) {}); });
}

void moved_state(Gate& gate, const CreditBatch& value) {
  for (const auto lane : lanes) moved_state(gate, value.plan(lane));
}

template <class Value>
void self_and_moves(Gate& gate, const Value& initial, const Value& source) {
  auto target = initial;
  auto* const alias = &target;
  const auto copy_self = allocation_failure::attempt(0, [&] { target = *alias; });
  gate.require(!copy_self.caught && copy_self.calls == 0, "self-copy unexpectedly allocated or failed");
  same(gate, target, initial);
  ++gate.self_operations;
  const auto move_self = allocation_failure::attempt(0, [&] { target = std::move(*alias); });
  gate.require(!move_self.caught && move_self.calls == 0, "self-move unexpectedly allocated or failed");
  same(gate, target, initial);
  ++gate.self_operations;

  auto moving = source;
  allocation_failure::arm(0);
  Value moved(std::move(moving));
  allocation_failure::disarm();
  gate.require(allocation_failure::state.calls == 0, "noexcept move construction allocated");
  same(gate, moved, source);
  moved_state(gate, moving);
  ++gate.move_operations;
  const auto result = allocation_failure::attempt(0, [&] { target = std::move(moved); });
  gate.require(!result.caught && result.calls == 0, "noexcept move assignment allocated");
  same(gate, target, source);
  moved_state(gate, moved);
  ++gate.move_operations;
}

void run(Gate& gate) {
  gate.require(std::is_copy_constructible_v<CreditPlan> && std::is_copy_assignable_v<CreditPlan> &&
                   std::is_nothrow_move_constructible_v<CreditPlan> &&
                   std::is_nothrow_move_assignable_v<CreditPlan> &&
                   std::is_copy_constructible_v<CreditBatch> && std::is_copy_assignable_v<CreditBatch> &&
                   std::is_nothrow_move_constructible_v<CreditBatch> &&
                   std::is_nothrow_move_assignable_v<CreditBatch>,
                "plan/batch value semantics or noexcept moves changed");
  const RectangleInput small{
      {{0, 0, 0}, {1, 0, 0}, {100, 0, 0}, {101, 0, 0}}, {0, 2}, {2, 4}, {}};
  const RectangleInput large{
      {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {1000, 0, 0}, {1001, 0, 0}, {1002, 0, 0}},
      {0, 3}, {3, 6}, {}};
  const auto small_owner = mhgp8::prepare_rectangle(small, 1, 8);
  const auto large_owner = mhgp8::prepare_rectangle(large, 10, 8);
  const auto initial = mhgp8::make_credit_plan(small_owner, Lane::Q2, Strategy::DualBlocks);
  const auto source = mhgp8::make_credit_plan(large_owner, Lane::Q4, Strategy::Tubes);
  gate.require(initial.candidate_pairs() == 1 && source.candidate_pairs() == 9 &&
                   &initial.rectangle() != &source.rectangle(),
                "exceptional assignment geometry fixture is vacuous");
  gate.plan_failures = strong_copy(gate, initial, source);
  self_and_moves(gate, initial, source);
  const auto initial_batch = mhgp8::make_credit_batch(small_owner, Strategy::DualBlocks);
  const auto source_batch = mhgp8::make_credit_batch(large_owner, Strategy::Tubes);
  gate.require(initial_batch.shared_work().tube_records == 0 &&
                   source_batch.shared_work().tube_records == 6,
                "batch shared-work transition is vacuous");
  gate.batch_failures = strong_copy(gate, initial_batch, source_batch);
  self_and_moves(gate, initial_batch, source_batch);
  gate.require(gate.plan_failures >= 5 && gate.batch_failures >= 15 &&
                   gate.successful_assignments == 2 && gate.self_operations == 4 &&
                   gate.move_operations == 4 && gate.moved_rejections == 24,
                "exception-safety gate failed its allocating-vector/lane non-vacuity");
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_plan_assignment_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    run(gate);
    std::cout << "mhgp8_plan_assignment_gate passed checks=" << gate.checks
              << " plan_allocation_failures=" << gate.plan_failures
              << " batch_allocation_failures=" << gate.batch_failures
              << " physical_comparisons=" << gate.physical_comparisons
              << " successful_assignments=" << gate.successful_assignments
              << " self_operations=" << gate.self_operations
              << " move_operations=" << gate.move_operations
              << " moved_rejections=" << gate.moved_rejections << '\n';
    return 0;
  } catch (const std::exception& error) {
    allocation_failure::disarm();
    std::cerr << "mhgp8_plan_assignment_gate failed: " << error.what() << '\n';
    return 1;
  }
}
