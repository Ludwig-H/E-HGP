// Bounded reproduction of exceptional copy assignment in commit 3589a2c9.
// No invalid subscript is executed. Inspecting sizes and emitted IDs suffices.
// A temporary copy-and-swap repair is checked by --expect-strong.
#include "pipeline/local_credits.hpp"
#include <cstdlib>
#include <iostream>
#include <new>
#include <type_traits>
#include <string_view>

namespace {
bool fail_next_allocation = false;
}
void* operator new(std::size_t count) {
  if (fail_next_allocation) {
    fail_next_allocation = false;
    throw std::bad_alloc();
  }
  if (void* pointer = std::malloc(count == 0 ? 1 : count)) return pointer;
  throw std::bad_alloc();
}
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }

int main(int argc, char** argv) {
  if (argc != 2) return 64;
  const bool expect_strong = std::string_view(argv[1]) == "--expect-strong";
  if (!expect_strong && std::string_view(argv[1]) != "--expect-vulnerable") return 64;
  using namespace mhgp8;
  static_assert(!std::is_copy_assignable_v<PreparedRectangle>);
  static_assert(std::is_copy_assignable_v<CreditPlan>);
  const RectangleInput small{{{0,0,0},{1,0,0},{100,0,0},{101,0,0}}, {0,2}, {2,4}, {}};
  const RectangleInput large{{{0,0,0},{1,0,0},{2,0,0},{1000,0,0},{1001,0,0},{1002,0,0}}, {0,3}, {3,6}, {}};
  const auto small_owner = prepare_rectangle(small,1,8);
  const auto large_owner = prepare_rectangle(large,10,8);
  auto target = make_credit_plan(small_owner,Lane::Q2,Strategy::DualBlocks);
  const auto source = make_credit_plan(large_owner,Lane::Q4,Strategy::Tubes);
  auto positive = target;
  positive = source;
  if (&positive.rectangle() != large_owner.get() || positive.a_credits().size() != 3 || positive.b_credits().size() != 3) return 2;
  const auto old_owner = &target.rectangle();
  const auto old_a_size = target.a_credits().size();
  const auto old_b_size = target.b_credits().size();
  const auto old_candidates = target.candidate_pairs();
  bool caught = false;
  fail_next_allocation = true;
  try { target = source; } catch (const std::bad_alloc&) { caught = true; }
  fail_next_allocation = false;
  const bool changed_owner = &target.rectangle() != old_owner;
  const bool wrong_lengths = target.a_credits().size() != target.rectangle().a_range().size() || target.b_credits().size() != target.rectangle().b_range().size();
  unsigned invalid_emissions = 0;
  target.for_each_candidate([&](std::size_t a, std::size_t b) {
    const auto ar = target.rectangle().a_range();
    const auto br = target.rectangle().b_range();
    if (a < ar.first || a >= ar.last || b < br.first || b >= br.last) ++invalid_emissions;
  });
  std::cout << "{\"allocation_failure_caught\":" << caught
            << ",\"owner_changed\":" << changed_owner
            << ",\"new_owner_a_size\":" << target.rectangle().a_range().size()
            << ",\"retained_a_credits_size\":" << target.a_credits().size()
            << ",\"new_owner_b_size\":" << target.rectangle().b_range().size()
            << ",\"retained_b_credits_size\":" << target.b_credits().size()
            << ",\"retained_candidates\":" << target.candidate_pairs()
            << ",\"source_candidates\":" << source.candidate_pairs()
            << ",\"invalid_emissions\":" << invalid_emissions
            << ",\"wrong_lengths\":" << wrong_lengths << "}\n";
  // Do not call keeps(2,3): it would index a_[2] outside the retained size 2.
  if (!caught || old_a_size != 2 || old_b_size != 2 || old_candidates != 1) return 3;
  if (expect_strong) {
    if (changed_owner || wrong_lengths || invalid_emissions != 0 ||
        target.candidate_pairs() != old_candidates || target.a_credits().size() != old_a_size ||
        target.b_credits().size() != old_b_size) return 4;
  } else if (!changed_owner || !wrong_lengths || invalid_emissions != 1) return 5;
  return 0;
}
