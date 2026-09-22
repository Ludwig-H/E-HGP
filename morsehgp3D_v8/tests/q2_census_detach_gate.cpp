#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <iterator>
#include <latch>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

#include "pipeline/q2_census_resume.hpp"

// Explicit adaptation of plan_assignment_gate's thread-local fault injector.
// It is armed ONLY for detach_pending, never for a callback or assertion.
// Ordinary and over-aligned C++ allocation paths are both intercepted.
namespace allocation_failure {
struct State { bool armed{}, hit{}; std::size_t remaining{}, calls{}; };
thread_local State state;
void disarm() noexcept { state.armed = false; }
[[gnu::noinline]] void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) {
  if (state.armed) {
    ++state.calls;
    if (state.remaining == 0) { state.hit = true; disarm(); throw std::bad_alloc(); }
    --state.remaining;
  }
  void* pointer = nullptr;
  if (alignment <= alignof(std::max_align_t)) pointer = std::malloc(size == 0 ? 1 : size);
  else if (posix_memalign(&pointer, alignment, size == 0 ? 1 : size) != 0) pointer = nullptr;
  if (pointer == nullptr) throw std::bad_alloc();
  return pointer;
}
[[gnu::noinline]] void release(void* pointer) noexcept { std::free(pointer); }
struct Attempt { bool caught{}, injected{}; std::size_t calls{}; };
template<class Function> Attempt attempt(std::size_t position, Function&& function) {
  state = {true, false, position, 0};
  bool caught = false;
  try { function(); } catch (const std::bad_alloc&) { caught = true; }
  catch (...) { disarm(); throw; }
  disarm();
  return {caught, state.hit, state.calls};
}
}  // namespace allocation_failure

void* operator new(std::size_t size) { return allocation_failure::allocate(size); }
void* operator new[](std::size_t size) { return allocation_failure::allocate(size); }
void* operator new(std::size_t size, std::align_val_t alignment) { return allocation_failure::allocate(size, static_cast<std::size_t>(alignment)); }
void* operator new[](std::size_t size, std::align_val_t alignment) { return allocation_failure::allocate(size, static_cast<std::size_t>(alignment)); }
void operator delete(void* pointer) noexcept { allocation_failure::release(pointer); }
void operator delete[](void* pointer) noexcept { allocation_failure::release(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { allocation_failure::release(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { allocation_failure::release(pointer); }
void operator delete(void* pointer, std::align_val_t) noexcept { allocation_failure::release(pointer); }
void operator delete[](void* pointer, std::align_val_t) noexcept { allocation_failure::release(pointer); }
void operator delete(void* pointer, std::size_t, std::align_val_t) noexcept { allocation_failure::release(pointer); }
void operator delete[](void* pointer, std::size_t, std::align_val_t) noexcept { allocation_failure::release(pointer); }

namespace {
using mhgp8::Point3;
using mhgp8::Q2CensusContinuation;
using mhgp8::Q2CensusContinuationStatus;
using mhgp8::Q2CensusResumeStage;
using mhgp8::Q2SiblingMode;
using mhgp8::Q2WitnessOrder;
using mhgp8::u64;
using Owner = std::unique_ptr<Q2CensusContinuation>;
using Points = std::vector<Point3>;

struct Gate {
  u64 checks{}, fixtures{}, runs{}, oracle_pairs{}, oracle_sites{}, supports{}, max_shell{};
  u64 detaches{}, imported_credit{}, imported_phase{}, emit_donors{}, recursive_detaches{}, no_child{};
  u64 destroyed_parents{}, owner_resets{}, parallel_pairs{}, distinct_threads{}, callback_failures{}, reentrant_rejections{};
  u64 allocation_failures{}, allocation_successes{}, injected_allocations{}, invalid_inputs{}, mutants{};
  void require(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
  }
  template<class Exception = std::logic_error, class Function>
  void rejects(Function&& function, const char* message) {
    bool caught = false;
    try { function(); } catch (const Exception&) { caught = true; }
    require(caught, message); ++invalid_inputs;
  }
};

struct Support {
  std::size_t a{}, b{};
  mhgp8::Q2BallKey key;
  std::vector<std::size_t> interior, shell;
  bool operator==(const Support&) const = default;
};
using Output = std::vector<Support>;
Support copy(const mhgp8::Q2Support& item) {
  Support result{std::min(item.a_id, item.b_id), std::max(item.a_id, item.b_id), item.key,
                 {item.interior.begin(), item.interior.end()}, {item.shell.begin(), item.shell.end()}};
  std::sort(result.interior.begin(), result.interior.end());
  std::sort(result.shell.begin(), result.shell.end());
  return result;
}
void sort(Output& output) {
  std::sort(output.begin(), output.end(), [](const auto& a, const auto& b) { return std::tie(a.a, a.b) < std::tie(b.a, b.b); });
}

// Test-only scalar oracle, explicitly following the independent formula in
// q2_census_resume_gate. No product bounds, census or front serve as oracle.
Output oracle(Gate& gate, const Points& points, std::size_t anchor,
              const std::vector<std::size_t>& targets, unsigned k) {
  gate.require(points.size() <= 140, "detach oracle exceeded its declared small domain");
  Output output;
  for (const auto b : targets) {
    gate.require(anchor < points.size() && b < points.size() && anchor != b, "oracle received an invalid support pair");
    Support item; item.a = std::min(anchor, b); item.b = std::max(anchor, b);
    for (std::size_t axis = 0; axis < 3; ++axis) {
      item.key.center_twice[axis] = static_cast<std::uint32_t>(points[anchor][axis]) + points[b][axis];
      const auto delta = std::int64_t{points[anchor][axis]} - points[b][axis];
      item.key.diameter_squared += static_cast<u64>(delta * delta);
    }
    for (std::size_t z = 0; z < points.size(); ++z) {
      std::int64_t h = 0;
      for (std::size_t axis = 0; axis < 3; ++axis)
        h += (std::int64_t{points[z][axis]} - points[anchor][axis]) *
             (std::int64_t{points[b][axis]} - points[z][axis]);
      if (h > 0) item.interior.push_back(z);
      if (h == 0) item.shell.push_back(z);
      ++gate.oracle_sites;
    }
    if (item.interior.size() < k) output.push_back(std::move(item));
    ++gate.oracle_pairs;
  }
  sort(output); return output;
}

// Independent literal reduction of all36 geometric work fields. The query
// tree depth is always zero on this global-index API; it is checked below.
// No product merge helper, object layout, padding or reinterpret_cast is used.
template<class Snapshot> std::array<u64, 36> geometry(const Snapshot& snapshot) {
  const auto& c = snapshot.census.work; const auto& s = snapshot.sibling_work; const auto& o = snapshot.order_work;
  return {c.query_build_point_visits, c.query_build_nodes, c.query_build_max_depth, c.input_descriptors,
          c.query_cover_visits, c.query_tasks, c.query_splits, c.witness_splits, c.count_root_starts,
          c.shared_splits_after_credit, c.cursor_advances, c.cursor_reuses, c.count_node_visits,
          c.count_bound_tests, c.count_point_tests, c.uniform_credited_pairs, c.uniform_rejected_pairs,
          c.uniform_accepted_pairs, c.consumed_witness_sites, c.frontier_restarts, c.payload_node_visits,
          c.payload_bound_tests, c.payload_point_tests, c.payload_interior_sites, c.payload_shell_sites,
          c.payload_supports, s.proposals, s.cardinality_skips, s.bound_tests, s.rejected_tasks,
          s.rejected_pairs, s.rejected_after_credit, o.structural_splits, o.deferred_skips,
          o.anchor_skips, o.phase_switches};
}
template<class Work> std::array<u64, 5> detach_values(const Work& work) {
  return {work.attempts, work.detached_frames, work.imported_frames, work.transferred_pairs, work.moved_frames};
}
auto pending_values(const mhgp8::Q2CensusResumePending& p) {
  return std::tuple{p.task_count, p.stage, p.query_node, p.cursor, p.sibling_node, p.original_b_node,
                    p.acquired_count, p.inside_deferred, p.emit_next, p.emit_end};
}
auto memory_values(const mhgp8::Q2CensusResumeMemory& m) {
  return std::tuple{m.stack_capacity, m.stack_bytes, m.interior_capacity, m.shell_capacity, m.payload_bytes, m.retained_bytes};
}
bool same_snapshot(const mhgp8::Q2CensusResumeSnapshot& a, const mhgp8::Q2CensusResumeSnapshot& b) {
  const auto metadata = [](const auto& s) {
    return std::tuple{s.census.candidate_pairs, s.census.accepted_pairs, s.census.rejected_pairs,
                      s.census.query_index_ms, s.census.count_ms, s.census.payload_ms, s.census.total_ms, s.status};
  };
  return geometry(a) == geometry(b) && metadata(a) == metadata(b) && a.resume_work == b.resume_work &&
         detach_values(a.detach_work) == detach_values(b.detach_work);
}

std::size_t rank_of(const mhgp8::Q2CensusIndex& index, std::size_t id) {
  const auto order = index.spatial_order(); const auto found = std::find(order.begin(), order.end(), id);
  if (found == order.end()) throw std::runtime_error("fixture original ID absent from index");
  return static_cast<std::size_t>(found - order.begin());
}
std::vector<std::size_t> ids_of(const mhgp8::Q2CensusIndex& index, std::size_t node) {
  const auto range = index.spatial_nodes()[node].range; const auto order = index.spatial_order();
  return {order.begin() + static_cast<std::ptrdiff_t>(range.first), order.begin() + static_cast<std::ptrdiff_t>(range.last)};
}
std::size_t largest_disjoint(const mhgp8::Q2CensusIndex& index, std::size_t rank) {
  std::size_t best = mhgp8::Q2SpatialNode::absent, size = 0;
  for (std::size_t node = 0; node < index.spatial_nodes().size(); ++node) {
    const auto range = index.spatial_nodes()[node].range;
    if ((rank < range.first || rank >= range.last) && range.size() > size) { best = node; size = range.size(); }
  }
  if (best == mhgp8::Q2SpatialNode::absent) throw std::runtime_error("fixture has no disjoint B node");
  return best;
}

struct Fixture { Points points; std::size_t anchor{}; };
Fixture line(unsigned population = 8) {
  Fixture fixture{{{0, 0, 0}}, 0};
  for (unsigned i = 0; i < population; ++i) fixture.points.push_back({static_cast<std::uint16_t>(1000 + i), 0, 0});
  return fixture;
}
std::vector<Fixture> fixtures() {
  std::vector<Fixture> result{
      // Explicitly ported four-site inherited-credit fixture, plus a longer
      // line whose K10 descendants genuinely split again after detachment.
      {{{0, 0, 0}, {5, 0, 0}, {10, 0, 0}, {11, 0, 0}}, 0}, line(),
      {{{1000, 0, 0}, {0, 1, 0}, {0, 0, 0}}, 0}};
  Fixture sphere;
  for (int x = -5; x <= 5; ++x) for (int y = -5; y <= 5; ++y) for (int z = -5; z <= 5; ++z)
    if (x*x + y*y + z*z == 25)
      sphere.points.push_back({static_cast<std::uint16_t>(10 + x), static_cast<std::uint16_t>(10 + y), static_cast<std::uint16_t>(10 + z)});
  result.push_back(std::move(sphere));
  Fixture extreme;
  for (unsigned bits = 0; bits < 8; ++bits)
    extreme.points.push_back({static_cast<std::uint16_t>((bits & 1U) ? 65535 : 0),
                              static_cast<std::uint16_t>((bits & 2U) ? 65535 : 0),
                              static_cast<std::uint16_t>((bits & 4U) ? 65535 : 0)});
  extreme.points.push_back({32767, 32767, 32767}); result.push_back(std::move(extreme));
  Fixture deferral;
  for (unsigned x = 0; x < 4; ++x) for (unsigned y = 0; y < 4; ++y) for (unsigned z = 0; z < 4; ++z)
    deferral.points.push_back({static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y), static_cast<std::uint16_t>(z)});
  deferral.anchor = deferral.points.size(); deferral.points.push_back({1000, 1000, 1000});
  for (unsigned x = 997; x < 1000; ++x) for (unsigned y = 998; y < 1000; ++y) for (unsigned z = 998; z < 1000; ++z)
    deferral.points.push_back({static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y), static_cast<std::uint16_t>(z)});
  result.push_back(std::move(deferral)); return result;
}

struct Reference { mhgp8::Q2CensusAnchorResult result; Output output, expected; };
Reference reference(Gate& gate, const Fixture& fixture, const mhgp8::Q2CensusIndexPtr& index,
                    std::size_t rank, std::size_t b, unsigned k, Q2SiblingMode sibling, Q2WitnessOrder order) {
  Reference direct;
  direct.expected = oracle(gate, fixture.points, fixture.anchor, ids_of(*index, b), k);
  direct.result = mhgp8::run_q2_anchor_reference(index, rank, b, k,
      [&](const mhgp8::Q2Support& item) { direct.output.push_back(copy(item)); }, sibling, order);
  sort(direct.output);
  gate.require(direct.output == direct.expected, "direct recursive reference disagrees with independent scalar oracle");
  return direct;
}

struct Total {
  std::array<u64, 36> work{};
  u64 candidates{}, accepted{}, rejected{}, detached{}, imported{}, shifted{};
  void add(Gate& gate, const mhgp8::Q2CensusResumeSnapshot& part) {
    gate.require(part.status == Q2CensusContinuationStatus::Done, "an incomplete branch entered the completed reduction");
    const auto values = geometry(part);
    for (std::size_t i = 0; i < values.size(); ++i) work[i] += values[i];
    candidates += part.census.candidate_pairs; accepted += part.census.accepted_pairs; rejected += part.census.rejected_pairs;
    detached += part.detach_work.detached_frames; imported += part.detach_work.imported_frames; shifted += part.detach_work.moved_frames;
    gate.require(part.census.accepted_pairs + part.census.rejected_pairs == part.census.candidate_pairs &&
                     part.census.query_index_ms == 0 && part.census.work.query_build_max_depth == 0,
                 "a detached domain lost its completed pair partition or rebuilt an index");
  }
};

void check_total(Gate& gate, const Total& total, Output output, const Reference& direct) {
  sort(output);
  gate.require(output == direct.expected && output == direct.output, "detached branches lost/duplicated exact supports, interiors or full shells");
  gate.require(total.work == geometry(direct.result) && total.candidates == direct.result.census.candidate_pairs &&
                   total.accepted == direct.result.census.accepted_pairs && total.rejected == direct.result.census.rejected_pairs,
               "sum of36 detached geometric counters or pair masses differs from direct recursive reference");
  gate.require(total.detached == total.imported && total.shifted >= total.detached && total.work[8] == 1 && total.work[3] == 1,
               "detached import/export ledger or historical root/descriptor ownership was duplicated");
  for (const auto& item : output) gate.max_shell = std::max(gate.max_shell, static_cast<u64>(item.shell.size()));
  gate.supports += output.size(); ++gate.runs;
}

Owner detach_checked(Gate& gate, Q2CensusContinuation& parent) {
  const auto before = parent.snapshot(); const auto pending = parent.pending();
  auto child = parent.detach_pending();
  const auto after = parent.snapshot();
  if (!child) {
    if (before.status == Q2CensusContinuationStatus::Done)
      gate.require(same_snapshot(before, after), "detach on Done changed a completed snapshot");
    else {
      auto expected = before; ++expected.detach_work.attempts;
      gate.require(pending.task_count <= 1 && same_snapshot(expected, after), "null detach lost work or modified more than attempts");
    }
    ++gate.no_child; return {};
  }
  gate.require(pending.task_count > 1, "detach fabricated a frame without a waiting sibling");
  const auto adopted = child->snapshot(); const auto cp = child->pending(); const auto pp = parent.pending();
  const auto mass = child->index().spatial_nodes()[cp.query_node].range.size();
  auto expected = before;
  expected.census.candidate_pairs -= static_cast<u64>(mass);
  ++expected.detach_work.attempts; ++expected.detach_work.detached_frames;
  expected.detach_work.transferred_pairs += static_cast<u64>(mass);
  expected.detach_work.moved_frames += static_cast<u64>(pending.task_count - 1);
  gate.require(same_snapshot(expected, after), "successful detach changed parent history, geometry, timing or wrong pair mass");
  auto expected_pending = pending; --expected_pending.task_count;
  gate.require(pending_values(pp) == pending_values(expected_pending), "detach disturbed the active top frame while removing a waiting bottom sibling");
  gate.require(&parent.index() == &child->index() && cp.task_count == 1 && cp.stage == Q2CensusResumeStage::Entry &&
                   cp.original_b_node == pending.original_b_node && adopted.status == Q2CensusContinuationStatus::Ready,
               "detached frame lost its immutable index, Entry state or original-B context");
  const auto child_range = child->index().spatial_nodes()[cp.query_node].range;
  const auto active_range = parent.index().spatial_nodes()[pending.query_node].range;
  gate.require(child_range.last <= active_range.first || active_range.last <= child_range.first,
               "detach exported a domain overlapping the retained active query");
  gate.require(adopted.census.candidate_pairs == mass && adopted.census.accepted_pairs == 0 && adopted.census.rejected_pairs == 0 &&
                   geometry(adopted) == std::array<u64, 36>{} &&
                   adopted.census.query_index_ms == 0 && adopted.census.count_ms == 0 &&
                   adopted.census.payload_ms == 0 && adopted.census.total_ms == 0 &&
                   detach_values(adopted.detach_work) == std::array<u64, 5>{0, 0, 1, 0, 0},
               "import copied historical counters, started a new root or lost its candidate mass");
  auto empty_resume = adopted.resume_work; empty_resume.max_pending_tasks = 0;
  gate.require(empty_resume == mhgp8::Q2CensusResumeWork{} && adopted.resume_work.max_pending_tasks == 1 &&
                   child->memory().payload_bytes == 0, "child copied scheduling history or mutable payload buffers");
  if (cp.acquired_count > 0) ++gate.imported_credit;
  if (cp.inside_deferred) ++gate.imported_phase;
  if (pending.stage == Q2CensusResumeStage::Emit) ++gate.emit_donors;
  if (before.detach_work.imported_frames != 0) ++gate.recursive_detaches;
  ++gate.detaches;
  return child;
}

enum class Policy { ParentFirst, ChildFirst, EmitDonor };
void forest_run(Gate& gate, const mhgp8::Q2CensusIndexPtr& index, std::size_t rank, std::size_t b,
                unsigned k, Q2SiblingMode sibling, Q2WitnessOrder order, Policy policy, const Reference& direct) {
  std::vector<Owner> tasks;
  tasks.push_back(mhgp8::make_q2_census_continuation(index, rank, b, k, sibling, order));
  Output output; Total total;
  const mhgp8::Q2CensusConsumer consume = [&](const mhgp8::Q2Support& item) { output.push_back(copy(item)); };
  static_cast<void>(detach_checked(gate, *tasks.front()));
  std::size_t turns = 0;
  while (!tasks.empty()) {
    // Both orders have the same geometry but intentionally different output
    // order. Completed owners are destroyed immediately after their reduction.
    const auto current = policy == Policy::ChildFirst ? tasks.size() - 1 : std::size_t{0};
    auto& owner = tasks[current];
    if (owner->advance(1, consume)) {
      static_cast<void>(detach_checked(gate, *owner));
      total.add(gate, owner->snapshot());
      if (tasks.size() > 1 && owner->snapshot().detach_work.detached_frames != 0) ++gate.destroyed_parents;
      tasks.erase(tasks.begin() + static_cast<std::ptrdiff_t>(current));
    } else {
      const auto p = owner->pending();
      if (p.task_count > 1 && (policy != Policy::EmitDonor || p.stage == Q2CensusResumeStage::Emit)) {
        auto child = detach_checked(gate, *owner);
        gate.require(static_cast<bool>(child), "eligible nonempty pending stack refused a detach");
        tasks.push_back(std::move(child));
      }
    }
    // This is a diagnostic for these <=77-site tests, never a product quota.
    gate.require(++turns < 1000000, "bounded detach fixture failed to make progress");
  }
  check_total(gate, total, std::move(output), direct);
}

void matrix(Gate& gate) {
  for (const auto& fixture : fixtures()) {
    const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(fixture.points));
    const auto rank = rank_of(*index, fixture.anchor); const auto b = largest_disjoint(*index, rank);
    ++gate.fixtures;
    for (const unsigned k : {1U, 2U, 5U, 10U})
      for (const auto sibling : {Q2SiblingMode::Disabled, Q2SiblingMode::Saturating})
        for (const auto order : {Q2WitnessOrder::GlobalDfs, Q2WitnessOrder::ComplementFirst}) {
          const auto direct = reference(gate, fixture, index, rank, b, k, sibling, order);
          for (const auto policy : {Policy::ParentFirst, Policy::ChildFirst, Policy::EmitDonor})
            forest_run(gate, index, rank, b, k, sibling, order, policy, direct);
        }
  }
}

void seek(Gate& gate, Q2CensusContinuation& owner, const mhgp8::Q2CensusConsumer& consume, bool emit) {
  for (std::size_t turn = 0; turn < 1000000; ++turn) {
    const auto p = owner.pending();
    if (p.task_count > 1 && (!emit || p.stage == Q2CensusResumeStage::Emit)) return;
    gate.require(!owner.advance(1, consume), "targeted fixture completed before its required detachable state");
  }
  throw std::runtime_error("targeted fixture never reached the requested detach state");
}
void finish(Q2CensusContinuation& owner, Output& output) {
  const mhgp8::Q2CensusConsumer consume = [&](const mhgp8::Q2Support& item) { output.push_back(copy(item)); };
  while (!owner.advance(7, consume)) {}
}

void allocation_guarantee(Gate& gate) {
  for (unsigned scenario = 0; scenario < 3; ++scenario) {
    const bool credited = scenario == 2;
    const bool emit = scenario == 1;
    const auto fixture = credited ? Fixture{{{0, 0, 0}, {5, 0, 0}, {10, 0, 0}, {11, 0, 0}}, 0} : line();
    const unsigned k = credited ? 2 : 10;
    const auto order = credited ? Q2WitnessOrder::GlobalDfs : Q2WitnessOrder::ComplementFirst;
    const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(fixture.points));
    const auto rank = rank_of(*index, 0); const auto b = largest_disjoint(*index, rank);
    const auto direct = reference(gate, fixture, index, rank, b, k, Q2SiblingMode::Disabled, order);
    std::size_t failures = 0;
    for (std::size_t position = 0; ; ++position) {
      auto parent = mhgp8::make_q2_census_continuation(index, rank, b, k, Q2SiblingMode::Disabled, order);
      Output output;
      const mhgp8::Q2CensusConsumer consume = [&](const mhgp8::Q2Support& item) { output.push_back(copy(item)); };
      seek(gate, *parent, consume, emit);
      const auto before = parent->snapshot(); const auto p = parent->pending(); const auto memory = parent->memory();
      if (credited) gate.require(p.acquired_count == 1, "allocation fixture lost its nonzero inherited credit");
      Owner child;
      const auto attempt = allocation_failure::attempt(position, [&] { child = parent->detach_pending(); });
      gate.require(!allocation_failure::state.armed, "allocation injector leaked into gate assertions or callbacks");
      if (attempt.caught) {
        gate.require(attempt.injected && !child && same_snapshot(before, parent->snapshot()) &&
                         pending_values(p) == pending_values(parent->pending()) && memory_values(memory) == memory_values(parent->memory()),
                     "failed detach violated the strong guarantee for parent work/state/memory/attempts");
        ++gate.allocation_failures; ++failures;
        child = detach_checked(gate, *parent);  // Same object, not a fresh retry.
        gate.require(static_cast<bool>(child), "strong-guarantee parent could not detach after allocation failure");
      } else {
        gate.require(!attempt.injected && child && failures == attempt.calls && position == attempt.calls && attempt.calls > 0,
                     "fault-injection loop did not exercise every successful detach allocation position");
        ++gate.allocation_successes; gate.injected_allocations += attempt.calls;
      }
      // This fixture has performed no previous detach. Both its immediate
      // first split and first Emit still retain the original right child at
      // stack.front, even when several more recent siblings are above it.
      gate.require(child->pending().query_node == index->spatial_nodes()[b].right,
                   "detach selected a newer pending sibling instead of stack.front");
      if (!emit) {
        const auto adopted = child->pending();
        gate.require(p.task_count == 2 && adopted.cursor == p.cursor && adopted.acquired_count == p.acquired_count &&
                         adopted.inside_deferred == p.inside_deferred && adopted.sibling_node == p.query_node,
                     "first split detached an altered inherited cursor/count/phase/opposite sibling");
      }
      finish(*child, output); finish(*parent, output);
      Total total; total.add(gate, child->snapshot()); total.add(gate, parent->snapshot());
      check_total(gate, total, std::move(output), direct);
      if (!attempt.caught) break;
    }
  }
}

void lifetime_and_threads(Gate& gate) {
  const auto fixture = line();
  auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(fixture.points));
  const auto rank = rank_of(*index, 0); const auto b = largest_disjoint(*index, rank);
  const auto direct = reference(gate, fixture, index, rank, b, 10, Q2SiblingMode::Disabled, Q2WitnessOrder::ComplementFirst);
  auto parent = mhgp8::make_q2_census_continuation(index, rank, b, 10, Q2SiblingMode::Disabled, Q2WitnessOrder::ComplementFirst);
  Output output;
  const mhgp8::Q2CensusConsumer consume = [&](const mhgp8::Q2Support& item) { output.push_back(copy(item)); };
  seek(gate, *parent, consume, false); auto child1 = detach_checked(gate, *parent);
  seek(gate, *parent, consume, false); auto child2 = detach_checked(gate, *parent);
  gate.require(child1 && child2, "parallel fixture failed to create two detached children");
  const std::weak_ptr<const mhgp8::Q2CensusIndex> weak = index;
  index.reset(); ++gate.owner_resets;
  finish(*parent, output);
  Total total; total.add(gate, parent->snapshot()); parent.reset(); ++gate.destroyed_parents;
  gate.require(!weak.expired(), "destroyed completed parent invalidated its still-pending children");
  std::array<Output, 2> private_output;
  std::array<std::exception_ptr, 2> errors;
  std::array<std::thread::id, 2> ids;
  std::latch ready{2}, start{1};
  const auto work = [&](std::size_t slot, Q2CensusContinuation& child) {
    ids[slot] = std::this_thread::get_id(); ready.count_down(); start.wait();
    try { finish(child, private_output[slot]); } catch (...) { errors[slot] = std::current_exception(); }
  };
  std::thread one([&] { work(0, *child1); });
  std::thread two;
  try { two = std::thread([&] { work(1, *child2); }); }
  catch (...) { start.count_down(); one.join(); throw; }
  ready.wait(); start.count_down(); one.join(); two.join();
  for (const auto& error : errors) if (error) std::rethrow_exception(error);
  gate.require(ids[0] != ids[1] && ids[0] != std::this_thread::get_id() && ids[1] != std::this_thread::get_id(),
               "parallel children did not execute on two distinct concurrently alive worker threads");
  ++gate.parallel_pairs; gate.distinct_threads += 2;
  total.add(gate, child1->snapshot()); total.add(gate, child2->snapshot());
  for (auto& part : private_output) output.insert(output.end(), std::make_move_iterator(part.begin()), std::make_move_iterator(part.end()));
  child1.reset(); child2.reset();
  gate.require(weak.expired(), "completed child owners leaked the last immutable index reference");
  check_total(gate, total, std::move(output), direct);
}

void rejections(Gate& gate) {
  const auto fixture = line();
  const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(fixture.points));
  const auto rank = rank_of(*index, 0); const auto b = largest_disjoint(*index, rank);
  const auto direct = reference(gate, fixture, index, rank, b, 10, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs);
  gate.rejects<std::invalid_argument>([&] { static_cast<void>(mhgp8::make_q2_census_continuation({}, rank, b, 10)); }, "null owner was accepted");
  gate.rejects<std::invalid_argument>([&] { static_cast<void>(mhgp8::make_q2_census_continuation(index, rank, 0, 10)); }, "anchor inside B was accepted");
  gate.rejects<std::invalid_argument>([&] { static_cast<void>(mhgp8::make_q2_census_continuation(index, rank, b, 0)); }, "invalid K was accepted");
  auto outer = mhgp8::make_q2_census_continuation(index, rank, b, 10);
  Output output;
  const mhgp8::Q2CensusConsumer consume = [&](const mhgp8::Q2Support& item) {
    output.push_back(copy(item));
    gate.rejects([&] { static_cast<void>(outer->detach_pending()); }, "callback detached its own busy continuation");
    ++gate.reentrant_rejections;
  };
  while (!outer->advance(2, consume)) {}
  Total total; total.add(gate, outer->snapshot()); check_total(gate, total, output, direct);
  static_cast<void>(detach_checked(gate, *outer));

  auto failed = mhgp8::make_q2_census_continuation(index, rank, b, 10);
  bool caught = false;
  try {
    while (!failed->advance(1, [&](const mhgp8::Q2Support&) { static_cast<void>(failed->detach_pending()); })) {}
  } catch (const std::logic_error&) { caught = true; }
  gate.require(caught && failed->snapshot().status == Q2CensusContinuationStatus::Failed,
               "uncaught nested detach did not fail its outer advance");
  const auto failure = failed->snapshot();
  gate.rejects([&] { static_cast<void>(failed->detach_pending()); }, "Failed continuation exported partial work");
  gate.require(same_snapshot(failure, failed->snapshot()), "failed detach changed poisoned history");
  ++gate.callback_failures;

  auto altered = geometry(direct.result); ++altered[8];
  gate.require(altered != geometry(direct.result), "new-root work mutant survived"); ++gate.mutants;
  auto duplicate = direct.expected; duplicate.push_back(duplicate.front()); sort(duplicate);
  gate.require(duplicate != direct.expected, "duplicated detached domain mutant survived"); ++gate.mutants;
  auto lost = direct.expected; lost.front().shell.pop_back();
  gate.require(lost != direct.expected, "truncated child shell mutant survived"); ++gate.mutants;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_q2_census_detach_gate --selftest\n"; return 2;
  }
  try {
    Gate gate;
    matrix(gate); allocation_guarantee(gate); lifetime_and_threads(gate); rejections(gate);
    gate.require(gate.runs >= 288 && gate.detaches > 0 && gate.imported_credit > 0 && gate.imported_phase > 0 &&
                     gate.emit_donors > 0 && gate.recursive_detaches > 0 && gate.no_child > 0 && gate.max_shell >= 30 &&
                     gate.destroyed_parents > 0 && gate.owner_resets > 0 && gate.parallel_pairs == 1 && gate.distinct_threads == 2 &&
                     gate.allocation_failures > 0 && gate.allocation_successes == 3 && gate.injected_allocations == gate.allocation_failures &&
                     gate.callback_failures > 0 && gate.reentrant_rejections > 0,
                 "detach gate lost a required positive transfer/allocation/lifetime fixture");
    std::cout << "{\"schema\":\"mhgp8_q2_census_detach_gate_v1\",\"status\":\"pass\",\"scope\":\"owned_shared_anchor_detach_not_full\""
              << ",\"checks\":" << gate.checks << ",\"fixtures\":" << gate.fixtures << ",\"runs\":" << gate.runs
              << ",\"oracle_pairs\":" << gate.oracle_pairs << ",\"oracle_sites\":" << gate.oracle_sites
              << ",\"supports\":" << gate.supports << ",\"max_shell\":" << gate.max_shell << ",\"detaches\":" << gate.detaches
              << ",\"imported_credit\":" << gate.imported_credit << ",\"imported_phase\":" << gate.imported_phase
              << ",\"emit_donors\":" << gate.emit_donors << ",\"recursive_detaches\":" << gate.recursive_detaches
              << ",\"no_child\":" << gate.no_child << ",\"destroyed_parents\":" << gate.destroyed_parents
              << ",\"owner_resets\":" << gate.owner_resets << ",\"parallel_pairs\":" << gate.parallel_pairs
              << ",\"distinct_threads\":" << gate.distinct_threads << ",\"allocation_failures\":" << gate.allocation_failures
              << ",\"allocation_successes\":" << gate.allocation_successes << ",\"injected_allocations\":" << gate.injected_allocations
              << ",\"callback_failures\":" << gate.callback_failures << ",\"reentrant_rejections\":" << gate.reentrant_rejections
              << ",\"invalid_inputs\":" << gate.invalid_inputs << ",\"mutants\":" << gate.mutants << "}\n";
    return 0;
  } catch (const std::exception& error) {
    allocation_failure::disarm();
    std::cerr << "mhgp8 q2 detach gate: " << error.what() << '\n'; return 1;
  }
}
