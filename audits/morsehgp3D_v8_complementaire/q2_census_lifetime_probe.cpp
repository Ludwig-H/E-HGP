// Bounded exception/lifetime audit, separate from the geometric census oracle.
#include "pipeline/q2_census.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace allocation {
bool armed{}, injected{};
std::size_t remaining{}, calls{}, live{};
void arm(std::size_t position) noexcept {
  armed = true; injected = false; remaining = position; calls = 0;
}
void disarm() noexcept { armed = false; }
[[gnu::noinline]] void* allocate(std::size_t size) {
  if (armed) {
    ++calls;
    if (remaining == 0) {
      armed = false; injected = true;
      throw std::bad_alloc();
    }
    --remaining;
  }
  if (void* p = std::malloc(size == 0 ? 1 : size)) { ++live; return p; }
  throw std::bad_alloc();
}
[[gnu::noinline]] void release(void* p) noexcept {
  if (p != nullptr) { --live; std::free(p); }
}
}  // namespace allocation
void* operator new(std::size_t n) { return allocation::allocate(n); }
void* operator new[](std::size_t n) { return allocation::allocate(n); }
void operator delete(void* p) noexcept { allocation::release(p); }
void operator delete[](void* p) noexcept { allocation::release(p); }
void operator delete(void* p, std::size_t) noexcept { allocation::release(p); }
void operator delete[](void* p, std::size_t) noexcept { allocation::release(p); }

namespace {
using mhgp8::u64;
using Mode = mhgp8::Q2CensusMode;
struct Failure { const char* reason; };
struct CallbackFailure { unsigned token; };
void require(bool condition, const char* reason) {
  if (!condition) throw Failure{reason};
}
static_assert(!std::is_copy_constructible_v<mhgp8::Q2CensusIndex>);
static_assert(!std::is_move_constructible_v<mhgp8::Q2CensusIndex>);
static_assert(!std::is_copy_assignable_v<mhgp8::Q2CensusIndex>);
static_assert(!std::is_move_assignable_v<mhgp8::Q2CensusIndex>);

auto index_work(const mhgp8::Q2IndexWork& w) {
  return std::array{w.point_visits, w.nodes, w.max_depth, w.escape_links};
}
auto census_work(const mhgp8::Q2CensusWork& w) {
  return std::array{w.query_build_point_visits, w.query_build_nodes, w.query_build_max_depth,
      w.input_descriptors, w.query_cover_visits, w.query_tasks, w.query_splits,
      w.witness_splits, w.count_root_starts, w.shared_splits_after_credit, w.cursor_advances,
      w.cursor_reuses, w.count_node_visits, w.count_bound_tests, w.count_point_tests,
      w.uniform_credited_pairs, w.uniform_rejected_pairs, w.uniform_accepted_pairs,
      w.consumed_witness_sites, w.frontier_restarts, w.payload_node_visits,
      w.payload_bound_tests, w.payload_point_tests, w.payload_interior_sites,
      w.payload_shell_sites, w.payload_supports};
}
auto axis_work(const mhgp8::AxisQ2Work& w) {
  return std::array{w.sort_passes, w.sorted_sites, w.sort_comparisons, w.columns,
      w.constrained_anchors, w.slab_bound_updates, w.tree_point_visits, w.tree_nodes,
      w.query_nodes, w.contained_nodes, w.disjoint_nodes, w.whole_factor_accepts,
      w.whole_factor_rejects, w.emitted_blocks, w.max_tree_depth, w.axis_bound_queries,
      w.axis_count_queries, w.axis_rank_comparisons, w.axis_pruned_nodes,
      w.axis_slab_rejects, w.restriction_credit_copies, w.restriction_credit_visits,
      w.restriction_bound_queries, w.restriction_pruned_nodes, w.coalesced_blocks};
}

struct Record {
  std::size_t a{}, b{}, interior_size{}, shell_size{};
  mhgp8::Q2BallKey key;
  std::array<std::size_t, 32> interior{}, shell{};
  bool operator==(const Record&) const = default;
};
struct Output {
  std::array<Record, 128> records{};
  std::size_t size{};
  // This callback storage never allocates: injected failures belong to census.
  void copy(const mhgp8::Q2Support& support) {
    require(size < records.size() && support.interior.size() <= 32 && support.shell.size() <= 32,
            "bounded callback storage exceeded");
    auto& record = records[size++];
    record = Record{};
    record.a = support.a_id; record.b = support.b_id; record.key = support.key;
    record.interior_size = support.interior.size(); record.shell_size = support.shell.size();
    std::copy(support.interior.begin(), support.interior.end(), record.interior.begin());
    std::copy(support.shell.begin(), support.shell.end(), record.shell.begin());
  }
};
void equal_output(const Output& a, const Output& b) {
  require(a.size == b.size, "retry changed output cardinal");
  require(std::equal(a.records.begin(), a.records.begin() + static_cast<std::ptrdiff_t>(a.size),
                     b.records.begin()), "retry changed output IDs, key, or payload");
}
void prefix_output(const Output& prefix, const Output& full) {
  require(prefix.size <= full.size &&
          std::equal(prefix.records.begin(), prefix.records.begin() +
                         static_cast<std::ptrdiff_t>(prefix.size), full.records.begin()),
          "exception left a changed output prefix");
}
void equal_result(const mhgp8::Q2CensusResult& a, const mhgp8::Q2CensusResult& b) {
  // Wall-clock readings are intentionally not compared or qualified.
  require(a.candidate_pairs == b.candidate_pairs && a.accepted_pairs == b.accepted_pairs &&
          a.rejected_pairs == b.rejected_pairs && census_work(a.work) == census_work(b.work),
          "retry changed census counts or work");
}

mhgp8::RectangleInput fixture(bool saturated) {
  mhgp8::RectangleInput input;
  for (unsigned side = 0; side < 2; ++side)
    for (unsigned y = 0; y < 3; ++y)
      for (unsigned z = 0; z < 3; ++z)
        input.points.push_back({static_cast<std::uint16_t>(1000 + 49000 * side),
                                static_cast<std::uint16_t>(100 + y),
                                static_cast<std::uint16_t>(100 + z)});
  input.a = {0, 9}; input.b = {9, 18};
  input.points.push_back({25500, 101, 101});
  if (saturated) input.core_candidates = {18};
  return input;
}
struct Stats {
  u64 index_failures{}, index_successes{}, census_failures{}, census_successes{};
  u64 late_allocation_failures{}, callback_exceptions{}, callback_partial_supports{};
  u64 complete_retries{}, empty_runs{}, api_rejections{}, oracle_records{};
  u64 accepted_pairwise{}, accepted_shared{}, rejected_pairwise{}, rejected_shared{};
};

void validate_reference(const Output& output, const mhgp8::PreparedRectangle& owner, Stats& stats) {
  require(output.size > 3, "too few accepted supports for delayed callback failure");
  for (std::size_t i = 0; i < output.size; ++i) {
    const auto& r = output.records[i];
    require(r.a < 9 && r.b >= 9 && r.b < 18 && r.interior_size > 0 && r.shell_size >= 2,
            "reference did not exercise original IDs and both payload populations");
    std::array<bool, 19> inside{}, shell{};
    for (std::size_t j = 0; j < r.interior_size; ++j) {
      require(r.interior[j] < 19 && !inside[r.interior[j]], "invalid interior ID");
      inside[r.interior[j]] = true;
    }
    for (std::size_t j = 0; j < r.shell_size; ++j) {
      require(r.shell[j] < 19 && !shell[r.shell[j]], "invalid shell ID");
      shell[r.shell[j]] = true;
    }
    const auto a = owner.points()[r.a], b = owner.points()[r.b];
    for (std::size_t z = 0; z < 19; ++z) {
      std::int64_t h = 0;
      for (unsigned axis = 0; axis < 3; ++axis)
        h += (std::int64_t(owner.points()[z][axis]) - a[axis]) *
             (std::int64_t(b[axis]) - owner.points()[z][axis]);
      require(inside[z] == (h > 0) && shell[z] == (h == 0), "reference payload geometry mismatch");
    }
    require(inside[18] && shell[r.a] && shell[r.b], "global witness or endpoint missing");
    ++stats.oracle_records;
  }
}

void index_failures(const mhgp8::RectanglePtr& owner, const mhgp8::Q2CensusIndexPtr& retained,
                    Stats& stats) {
  const auto work = index_work(retained->work());
  for (std::size_t position = 0; ; ++position) {
    require(position < 128, "index allocation loop exceeded");
    const auto refs = owner.use_count(), retained_refs = retained.use_count();
    const auto live = allocation::live;
    mhgp8::Q2CensusIndexPtr created;
    bool caught = false;
    allocation::arm(position);
    try { created = mhgp8::make_q2_census_index(owner); }
    catch (const std::bad_alloc&) { caught = true; }
    catch (...) { allocation::disarm(); throw; }
    allocation::disarm();
    require(caught == allocation::injected, "index allocation failure not propagated");
    if (!caught) require(created && &created->rectangle() == owner.get() &&
                         index_work(created->work()) == work, "index retry changed its owner or work");
    created.reset();
    require(allocation::live == live && owner.use_count() == refs &&
            retained.use_count() == retained_refs && index_work(retained->work()) == work,
            "index construction changed retained state or leaked resources");
    if (caught) ++stats.index_failures;
    else { ++stats.index_successes; break; }
  }
}

void census_failures(const mhgp8::RectanglePtr& owner, const mhgp8::Q2CensusIndexPtr& index,
                     const mhgp8::AxisQ2Plan& plan, Mode mode, Stats& stats) {
  Output reference;
  const mhgp8::Q2CensusConsumer baseline_consumer = [&](const auto& s) { reference.copy(s); };
  const auto expected = mhgp8::run_q2_census(*index, plan, mode, baseline_consumer);
  validate_reference(reference, *owner, stats);
  if (mode == Mode::Pairwise) {
    stats.accepted_pairwise = expected.accepted_pairs; stats.rejected_pairwise = expected.rejected_pairs;
  } else {
    stats.accepted_shared = expected.accepted_pairs; stats.rejected_shared = expected.rejected_pairs;
  }
  const auto iw = index_work(index->work());
  const auto aw = axis_work(plan.work());
  const auto check_retained = [&] {
    require(index_work(index->work()) == iw && axis_work(plan.work()) == aw &&
            &index->rectangle() == owner.get() && &plan.rectangle() == owner.get(),
            "census exception changed input owner or counters");
  };
  for (std::size_t position = 0; ; ++position) {
    require(position < 128, "census allocation loop exceeded");
    Output partial;
    const mhgp8::Q2CensusConsumer consumer = [&](const auto& s) { partial.copy(s); };
    const auto live = allocation::live;
    const auto refs = owner.use_count(), index_refs = index.use_count();
    bool caught = false;
    mhgp8::Q2CensusResult result;
    allocation::arm(position);
    try { result = mhgp8::run_q2_census(*index, plan, mode, consumer); }
    catch (const std::bad_alloc&) { caught = true; }
    catch (...) { allocation::disarm(); throw; }
    allocation::disarm();
    require(caught == allocation::injected, "census allocation failure not propagated");
    require(allocation::live == live && owner.use_count() == refs && index.use_count() == index_refs,
            "census failure leaked temporary storage or references");
    check_retained();
    prefix_output(partial, reference);
    if (!caught) { equal_output(partial, reference); equal_result(result, expected); }
    if (caught) {
      ++stats.census_failures;
      if (partial.size > 0) ++stats.late_allocation_failures;
    } else ++stats.census_successes;
    partial.size = 0;  // Explicitly discard this incomplete output before retry.
    const auto retry = mhgp8::run_q2_census(*index, plan, mode, consumer);
    equal_output(partial, reference); equal_result(retry, expected); check_retained();
    ++stats.complete_retries;
    if (!caught) break;
  }
  Output partial;
  const mhgp8::Q2CensusConsumer throwing = [&](const auto& support) {
    partial.copy(support);
    if (partial.size == 3) throw CallbackFailure{731};
  };
  const auto live = allocation::live;
  const auto refs = owner.use_count(), index_refs = index.use_count();
  bool propagated = false;
  try { static_cast<void>(mhgp8::run_q2_census(*index, plan, mode, throwing)); }
  catch (const CallbackFailure& e) { propagated = e.token == 731; }
  require(propagated, "callback exception was swallowed");
  require(partial.size == 3, "callback failure did not retain three copied supports");
  prefix_output(partial, reference);
  require(allocation::live == live && owner.use_count() == refs && index.use_count() == index_refs,
          "callback exception leaked census storage or references");
  check_retained();
  ++stats.callback_exceptions; stats.callback_partial_supports += partial.size;
  partial.size = 0;
  const mhgp8::Q2CensusConsumer retry_consumer = [&](const auto& support) { partial.copy(support); };
  const auto retry = mhgp8::run_q2_census(*index, plan, mode, retry_consumer);
  equal_output(partial, reference); equal_result(retry, expected); check_retained();
  ++stats.complete_retries;
}

template <class Exception = std::invalid_argument, class Function>
void refuses(Function&& function, Stats& stats) {
  bool caught = false;
  try { function(); } catch (const Exception&) { caught = true; }
  require(caught, "empty-residual validation was bypassed");
  ++stats.api_rejections;
}

void empty_validation(Stats& stats) {
  const auto owner = mhgp8::prepare_rectangle(fixture(true), 1, 12);
  const auto foreign_owner = mhgp8::prepare_rectangle(fixture(true), 1, 12);
  const auto index = mhgp8::make_q2_census_index(owner);
  const auto foreign = mhgp8::make_q2_census_index(foreign_owner);
  auto plan = mhgp8::make_axis_q2_plan(owner, mhgp8::AxisQ2Mode::Additive);
  require(plan.candidate_pairs() == 0, "empty fixture not saturated");
  unsigned called = 0;
  const mhgp8::Q2CensusConsumer consumer = [&](const auto&) { ++called; };
  for (Mode mode : {Mode::Pairwise, Mode::SharedBlocks}) {
    refuses([&] { static_cast<void>(mhgp8::run_q2_census(*foreign, plan, mode, consumer)); }, stats);
    refuses([&] { static_cast<void>(mhgp8::run_q2_census(*index, plan,
                                  static_cast<Mode>(255), consumer)); }, stats);
    refuses([&] { static_cast<void>(mhgp8::run_q2_census(*index, plan, mode, {})); }, stats);
    allocation::arm(0);
    const auto result = mhgp8::run_q2_census(*index, plan, mode, consumer);
    allocation::disarm();
    require(allocation::calls == 0 && called == 0 && result.candidate_pairs == 0 &&
            result.accepted_pairs == 0 && result.rejected_pairs == 0 &&
            census_work(result.work) == census_work(mhgp8::Q2CensusWork{}),
            "valid empty run allocated, emitted, or retained traversal state");
    ++stats.empty_runs;
  }
  const auto moved = std::move(plan);
  for (Mode mode : {Mode::Pairwise, Mode::SharedBlocks})
    refuses<std::logic_error>([&] { static_cast<void>(mhgp8::run_q2_census(*index, plan, mode, consumer)); }, stats);
  require(moved.candidate_pairs() == 0, "empty plan move changed cardinal");
}
}  // namespace

int main() {
  try {
    Stats s;
    const auto owner = mhgp8::prepare_rectangle(fixture(false), 4, 12);
    const auto index = mhgp8::make_q2_census_index(owner);
    const auto plan = mhgp8::make_axis_q2_plan(owner, mhgp8::AxisQ2Mode::Additive);
    index_failures(owner, index, s);
    for (Mode mode : {Mode::Pairwise, Mode::SharedBlocks}) census_failures(owner, index, plan, mode, s);
    empty_validation(s);
    require(s.index_failures >= 5 && s.index_successes == 1 && s.census_failures >= 5 &&
            s.census_successes == 2 && s.late_allocation_failures >= 2 &&
            s.callback_exceptions == 2 && s.callback_partial_supports == 6 && s.complete_retries >= 7 &&
            s.empty_runs == 2 && s.api_rejections == 8 && s.oracle_records > 6 &&
            s.accepted_pairwise == s.accepted_shared && s.rejected_pairwise == s.rejected_shared &&
            s.accepted_pairwise > 3 && s.rejected_pairwise > 0, "vacuous exception campaign");
    std::cout << "{\"index_allocation_failures\":" << s.index_failures
              << ",\"index_constructions\":" << s.index_successes
              << ",\"census_allocation_failures\":" << s.census_failures
              << ",\"census_completed_attempts\":" << s.census_successes
              << ",\"allocation_failures_after_output\":" << s.late_allocation_failures
              << ",\"callback_exceptions\":" << s.callback_exceptions
              << ",\"partial_supports_retained\":" << s.callback_partial_supports
              << ",\"complete_retries\":" << s.complete_retries
              << ",\"empty_runs_without_allocation\":" << s.empty_runs
              << ",\"empty_api_rejections\":" << s.api_rejections
              << ",\"reference_payloads_checked\":" << s.oracle_records
              << ",\"accepted_per_mode\":" << s.accepted_pairwise
              << ",\"rejected_per_mode\":" << s.rejected_pairwise << "}\n";
  } catch (const Failure& e) {
    allocation::disarm(); std::cerr << "census lifetime audit: " << e.reason << '\n'; return 1;
  } catch (const std::exception& e) {
    allocation::disarm(); std::cerr << "census lifetime unexpected: " << e.what() << '\n'; return 2;
  }
}
