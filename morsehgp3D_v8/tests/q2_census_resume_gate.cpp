#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <latch>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "pipeline/q2_census_resume.hpp"

namespace {

using mhgp8::Point3;
using mhgp8::Q2CensusContinuationStatus;
using mhgp8::Q2CensusResumeStage;
using mhgp8::Q2SiblingMode;
using mhgp8::Q2WitnessOrder;
using mhgp8::u64;
using Points = std::vector<Point3>;
constexpr auto unlimited = std::numeric_limits<std::size_t>::max();
static_assert(!std::is_copy_constructible_v<mhgp8::Q2CensusContinuation> &&
              !std::is_move_constructible_v<mhgp8::Q2CensusContinuation> &&
              !std::is_copy_assignable_v<mhgp8::Q2CensusContinuation> &&
              !std::is_move_assignable_v<mhgp8::Q2CensusContinuation>);

struct Support {
  std::size_t a{}, b{};
  mhgp8::Q2BallKey key;
  std::vector<std::size_t> interior, shell;
  bool operator==(const Support&) const = default;
};
using Output = std::vector<Support>;

struct Gate {
  u64 checks{}, fixtures{}, roots{}, oracle_pairs{}, oracle_sites{}, references{}, resumptions{};
  u64 advances{}, supports{}, max_shell{}, credit_pauses{}, split_credit_pauses{}, phase_pauses{}, deferred_child_pauses{};
  u64 emission_pauses{}, partial_range_pauses{}, two_engines{}, thread_transfers{}, owner_resets{};
  u64 callback_failures{}, reentrant_calls{}, overlap_rejections{}, invalid_inputs{}, mutants{};
  void require(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
  }
  template<class Exception = std::invalid_argument, class Function>
  void rejects(Function&& function, const char* message) {
    bool caught = false;
    try { function(); } catch (const Exception&) { caught = true; }
    require(caught, message);
    ++invalid_inputs;
  }
};

Support copy(const mhgp8::Q2Support& support) {
  Support result{std::min(support.a_id, support.b_id), std::max(support.a_id, support.b_id), support.key,
                 {support.interior.begin(), support.interior.end()}, {support.shell.begin(), support.shell.end()}};
  std::sort(result.interior.begin(), result.interior.end());
  std::sort(result.shell.begin(), result.shell.end());
  return result;
}

void sort(Output& output) {
  std::sort(output.begin(), output.end(), [](const auto& a, const auto& b) {
    return a.a < b.a || (a.a == b.a && a.b < b.b);
  });
}

// Independent, test-only scalar oracle. Neither its keys nor H signs call
// a product predicate, front, index traversal or census implementation.
// All differences are promoted before multiplication: |H| <=3*65535^2.
Output oracle(Gate& gate, const Points& points, std::size_t anchor,
              std::vector<std::size_t> targets, unsigned k) {
  gate.require(points.size() <= 140 && anchor < points.size(), "resume oracle exceeded its declared small domain");
  std::sort(targets.begin(), targets.end());
  gate.require(std::adjacent_find(targets.begin(), targets.end()) == targets.end(), "oracle target IDs were duplicated");
  Output output;
  for (const auto b : targets) {
    gate.require(b < points.size() && b != anchor, "oracle target is not a distinct original site");
    Support item;
    item.a = std::min(anchor, b); item.b = std::max(anchor, b);
    for (std::size_t axis = 0; axis < 3; ++axis) {
      item.key.center_twice[axis] = std::uint32_t{points[anchor][axis]} + points[b][axis];
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
  sort(output);
  return output;
}

std::size_t rank_of(const mhgp8::Q2CensusIndex& index, std::size_t id) {
  const auto order = index.spatial_order();
  const auto found = std::find(order.begin(), order.end(), id);
  if (found == order.end()) throw std::runtime_error("fixture original ID is absent from index");
  return static_cast<std::size_t>(found - order.begin());
}

std::vector<std::size_t> ids_of(const mhgp8::Q2CensusIndex& index, std::size_t node) {
  const auto range = index.spatial_nodes()[node].range;
  const auto order = index.spatial_order();
  return {order.begin() + static_cast<std::ptrdiff_t>(range.first),
          order.begin() + static_cast<std::ptrdiff_t>(range.last)};
}

std::size_t node_for(const mhgp8::Q2CensusIndex& index, std::vector<std::size_t> ids) {
  std::sort(ids.begin(), ids.end());
  for (std::size_t node = 0; node < index.spatial_nodes().size(); ++node) {
    if (index.spatial_nodes()[node].range.size() != ids.size()) continue;
    auto actual = ids_of(index, node);
    std::sort(actual.begin(), actual.end());
    if (actual == ids) return node;
  }
  throw std::runtime_error("fixture IDs do not describe a real immutable index node");
}

std::size_t largest_disjoint(const mhgp8::Q2CensusIndex& index, std::size_t rank) {
  auto selected = mhgp8::Q2SpatialNode::absent;
  std::size_t size = 0;
  for (std::size_t node = 0; node < index.spatial_nodes().size(); ++node) {
    const auto range = index.spatial_nodes()[node].range;
    if ((rank < range.first || rank >= range.last) && range.size() > size) {
      selected = node; size = range.size();
    }
  }
  if (selected == mhgp8::Q2SpatialNode::absent) throw std::runtime_error("fixture has no disjoint nonempty B");
  return selected;
}

void check_output(Gate& gate, const Output& output, const Output& expected) {
  auto normalized = output;
  sort(normalized);
  gate.require(normalized == expected, "resumed support/key/interior/full-shell multiset differs from scalar oracle");
  for (const auto& support : output) {
    gate.require(std::adjacent_find(support.interior.begin(), support.interior.end()) == support.interior.end() &&
                     std::adjacent_find(support.shell.begin(), support.shell.end()) == support.shell.end(),
                 "resumed payload contains a duplicate ID");
    gate.require(std::binary_search(support.shell.begin(), support.shell.end(), support.a) &&
                     std::binary_search(support.shell.begin(), support.shell.end(), support.b),
                 "count-only anchor exclusion removed a support endpoint from its shell");
    gate.max_shell = std::max(gate.max_shell, static_cast<u64>(support.shell.size()));
  }
  gate.supports += output.size();
}

template<class Result>
void same_work(Gate& gate, const Result& actual, const mhgp8::Q2CensusAnchorResult& reference) {
  gate.require(actual.census.work == reference.census.work && actual.sibling_work == reference.sibling_work &&
                   actual.order_work == reference.order_work &&
                   actual.census.candidate_pairs == reference.census.candidate_pairs &&
                   actual.census.accepted_pairs == reference.census.accepted_pairs &&
                   actual.census.rejected_pairs == reference.census.rejected_pairs,
               "resumption repeated or lost work relative to the direct recursive anchor traversal");
}

template<class Continuation>
bool advance_checked(Gate& gate, Continuation& continuation, const mhgp8::Q2CensusIndex& index,
                     std::size_t original_b, unsigned k, std::size_t budget, const mhgp8::Q2CensusConsumer& consumer) {
  const auto before = continuation.snapshot();
  const auto done = continuation.advance(budget, consumer);
  const auto after = continuation.snapshot();
  const auto pending = continuation.pending();
  const auto& work = after.resume_work;
  gate.require(work.transitions >= before.resume_work.transitions &&
                   work.transitions - before.resume_work.transitions <= budget &&
                   work.transitions == work.entry_steps + work.witness_steps + work.admission_steps + work.payload_steps,
               "quantum truncated work or did not account for every declared transition");
  gate.require(done == (after.status == Q2CensusContinuationStatus::Done), "advance return and terminal status disagree");
  gate.require(after.census.work.count_root_starts == 1 && after.census.work.frontier_restarts == 0 &&
                   after.census.work.input_descriptors == 1 && after.census.work.query_build_nodes == 0 &&
                   after.census.work.query_build_point_visits == 0 && after.census.work.query_cover_visits == 0,
               "resumption restarted the root or rebuilt/recovered its already certified B factor");
  gate.require(work.entry_steps == after.census.work.query_tasks && work.payload_steps == after.census.work.payload_supports &&
                   after.census.accepted_pairs <= after.census.candidate_pairs &&
                   after.census.rejected_pairs <= after.census.candidate_pairs - after.census.accepted_pairs &&
                   after.census.work.payload_supports <= after.census.accepted_pairs && after.census.query_index_ms == 0,
               "intermediate accounting replayed task entry, admission, payload, or query indexing");
  if (!done) {
    gate.require(after.status == Q2CensusContinuationStatus::Ready && pending.task_count > 0 &&
                     pending.stage != Q2CensusResumeStage::None && work.transitions > before.resume_work.transitions,
                 "a positive quantum left a nonterminal continuation without progress");
    gate.require(pending.original_b_node == original_b && pending.acquired_count < k,
                 "a pending child lost original B or retained a saturated acquired count");
    const auto original = index.spatial_nodes()[original_b].range;
    gate.require(pending.query_node < index.spatial_nodes().size(), "pending query node is not an index node");
    const auto current = index.spatial_nodes()[pending.query_node].range;
    gate.require(original.first <= current.first && current.last <= original.last &&
                     pending.task_count <= work.max_pending_tasks,
                 "pending query escaped its original B or logical stack maximum");
    if (pending.acquired_count > 0) {
      ++gate.credit_pauses;
      if (after.census.work.shared_splits_after_credit > 0) ++gate.split_credit_pauses;
    }
    if (pending.inside_deferred && pending.query_node != original_b) ++gate.deferred_child_pauses;
    if (budget == 1 && after.order_work.phase_switches > before.order_work.phase_switches) {
      gate.require(pending.inside_deferred, "a phase-switch suspension lost its original-B phase");
      ++gate.phase_pauses;
    }
    if (pending.stage == Q2CensusResumeStage::Emit) {
      gate.require(pending.emit_next < pending.emit_end && pending.emit_end == current.last,
                   "pending admission forgot the remaining support positions");
      ++gate.emission_pauses;
      if (current.first < pending.emit_next && pending.emit_next < pending.emit_end) ++gate.partial_range_pauses;
    }
  } else {
    gate.require(pending.task_count == 0 && pending.stage == Q2CensusResumeStage::None,
                 "completed continuation retained unfinished query work");
  }
  const auto memory = continuation.memory();
  gate.require(memory.retained_bytes >= memory.stack_bytes && memory.retained_bytes >= memory.payload_bytes &&
                   memory.payload_bytes >= sizeof(std::size_t) * (memory.interior_capacity + memory.shell_capacity),
               "continuation omitted its owned stack or payload capacities");
  ++gate.advances;
  return done;
}

struct Capture {
  mhgp8::Q2CensusAnchorResult result;
  Output output;
};

Capture reference(Gate& gate, const mhgp8::Q2CensusIndexPtr& index, std::size_t anchor_rank,
                  std::size_t b_node, unsigned k, Q2SiblingMode sibling, Q2WitnessOrder order,
                  const Output& expected) {
  Capture capture;
  capture.result = mhgp8::run_q2_anchor_reference(index, anchor_rank, b_node, k,
      [&](const mhgp8::Q2Support& support) { capture.output.push_back(copy(support)); }, sibling, order);
  check_output(gate, capture.output, expected);
  ++gate.references;
  return capture;
}

void run_case(Gate& gate, const mhgp8::Q2CensusIndexPtr& index, std::size_t anchor_rank, std::size_t b_node,
              unsigned k, Q2SiblingMode sibling, Q2WitnessOrder order, std::size_t budget,
              const Capture& direct, const Output& expected) {
  auto continuation = mhgp8::make_q2_census_continuation(index, anchor_rank, b_node, k, sibling, order);
  Output output;
  const mhgp8::Q2CensusConsumer consume = [&](const mhgp8::Q2Support& support) { output.push_back(copy(support)); };
  const auto initial = continuation->snapshot();
  gate.require(initial.status == Q2CensusContinuationStatus::Ready && initial.resume_work.transitions == 0 &&
                   initial.census.work.query_tasks == 0 && initial.census.candidate_pairs == index->spatial_nodes()[b_node].range.size(),
               "factory performed traversal or changed the declared anchor domain");
  // Test-only hang diagnostic on n<=140, not a product search/output cap.
  std::size_t calls = 0;
  while (!advance_checked(gate, *continuation, *index, b_node, k, budget, consume))
    gate.require(++calls < 1000000, "bounded fixture made too many positive-quantum resumptions");
  const auto final = continuation->snapshot();
  same_work(gate, final, direct.result);
  gate.require(output == direct.output, "quantum changed recursive DFS callback order");
  check_output(gate, output, expected);
  gate.require(final.census.accepted_pairs + final.census.rejected_pairs == final.census.candidate_pairs &&
                   final.census.work.payload_supports == output.size() &&
                   final.census.work.query_tasks == 1 + 2 * final.census.work.query_splits &&
                   final.census.work.cursor_reuses == 2 * final.census.work.query_splits,
               "completed continuation broke the pair/child/cursor/payload ledger");
  for (const double time : {final.census.total_ms, final.census.count_ms, final.census.payload_ms})
    gate.require(std::isfinite(time) && time >= 0, "continuation reported a nonfinite or negative duration");
  const auto calls_before = final.resume_work.advance_calls;
  gate.require(continuation->advance(1, consume) && continuation->snapshot().resume_work.advance_calls == calls_before &&
                   output == direct.output, "advance on Done performed work or emitted a duplicate support");
  ++gate.resumptions;
}

struct Fixture { Points points; std::size_t anchor{}; std::vector<std::size_t> selected; };

std::vector<Fixture> fixtures() {
  std::vector<Fixture> result{
      {{{0, 0, 0}, {65535, 65535, 65535}}, 0, {1}},
      // Explicit port of q2_sibling_gate's split-after-credit counterexample.
      {{{0, 0, 0}, {5, 0, 0}, {10, 0, 0}, {11, 0, 0}}, 0, {2, 3}},
      {{{1000, 0, 0}, {0, 1, 0}, {0, 0, 0}}, 0, {1, 2}},
      // Explicit fractional-center/full-shell fixture from q2_census_gate.
      {{{0, 1, 1}, {3, 2, 2}, {1, 0, 1}, {2, 3, 2}, {1, 1, 1}, {1, 1, 4}}, 0, {1}},
      {{{1000, 1000, 1000}, {60000, 999, 1000}, {60000, 1001, 1000},
        {60000, 998, 1000}, {60000, 1002, 1000}}, 0, {}},
      {{{1000, 0, 0}, {60000, 0, 0}, {60000, 2, 0}, {30000, 1, 0}, {60000, 1, 0}}, 0, {}},
      {{{1, 0, 0}, {5, 0, 0}, {6, 0, 0}, {10, 0, 0}, {11, 0, 0}, {1000, 0, 0}}, 0, {}}};
  Fixture sphere;
  for (int x = -5; x <= 5; ++x) for (int y = -5; y <= 5; ++y) for (int z = -5; z <= 5; ++z)
    if (x*x + y*y + z*z == 25)
      sphere.points.push_back({static_cast<std::uint16_t>(10 + x), static_cast<std::uint16_t>(10 + y),
                               static_cast<std::uint16_t>(10 + z)});
  sphere.selected = {29};
  result.push_back(sphere);
  sphere.points.push_back({10, 10, 10});
  result.push_back(std::move(sphere));
  Fixture extremes;
  for (unsigned bits = 0; bits < 8; ++bits)
    extremes.points.push_back({static_cast<std::uint16_t>((bits & 1U) != 0 ? 65535 : 0),
                               static_cast<std::uint16_t>((bits & 2U) != 0 ? 65535 : 0),
                               static_cast<std::uint16_t>((bits & 4U) != 0 ? 65535 : 0)});
  extremes.points.push_back({32767, 32767, 32767}); extremes.selected = {7};
  result.push_back(std::move(extremes));
  Fixture deferral;
  for (unsigned x = 0; x < 4; ++x) for (unsigned y = 0; y < 4; ++y) for (unsigned z = 0; z < 4; ++z) {
    deferral.selected.push_back(deferral.points.size());
    deferral.points.push_back({static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y), static_cast<std::uint16_t>(z)});
  }
  deferral.anchor = deferral.points.size(); deferral.points.push_back({1000, 1000, 1000});
  for (unsigned x = 997; x < 1000; ++x) for (unsigned y = 998; y < 1000; ++y) for (unsigned z = 998; z < 1000; ++z)
    deferral.points.push_back({static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y), static_cast<std::uint16_t>(z)});
  result.push_back(std::move(deferral));
  Fixture credited{{{0, 0, 0}, {500, 0, 0}}, 0, {}};
  for (unsigned i = 0; i < 64; ++i) {
    credited.selected.push_back(credited.points.size());
    credited.points.push_back({static_cast<std::uint16_t>(1000 + i), 0, 0});
  }
  result.push_back(std::move(credited));
  return result;
}

void matrix(Gate& gate) {
  for (const auto& fixture : fixtures()) {
    const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(fixture.points));
    const auto rank = rank_of(*index, fixture.anchor);
    const auto b = fixture.selected.empty() ? largest_disjoint(*index, rank) : node_for(*index, fixture.selected);
    const auto targets = ids_of(*index, b);
    ++gate.fixtures; ++gate.roots;
    for (const unsigned k : {1U, 2U, 5U, 10U}) {
      const auto expected = oracle(gate, fixture.points, fixture.anchor, targets, k);
      for (const auto sibling : {Q2SiblingMode::Disabled, Q2SiblingMode::Saturating})
        for (const auto order : {Q2WitnessOrder::GlobalDfs, Q2WitnessOrder::ComplementFirst}) {
          const auto direct = reference(gate, index, rank, b, k, sibling, order, expected);
          if (fixture.points == Points{{0, 0, 0}, {5, 0, 0}, {10, 0, 0}, {11, 0, 0}} &&
              k == 2 && order == Q2WitnessOrder::GlobalDfs) {
            gate.require(direct.result.census.work.query_splits == 1 && direct.result.census.work.shared_splits_after_credit == 1,
                         "the exact four-site split-after-credit fixture became vacuous");
          }
          for (const auto budget : {std::size_t{1}, std::size_t{2}, std::size_t{7}, unlimited})
            run_case(gate, index, rank, b, k, sibling, order, budget, direct, expected);
        }
    }
  }
}

void lifetimes(Gate& gate) {
  Points points{{0, 0, 0}, {5, 0, 0}, {10, 0, 0}, {11, 0, 0}};
  auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
  const auto rank = rank_of(*index, 0);
  const auto b = node_for(*index, {2, 3});
  const auto second_rank = rank_of(*index, 3);
  const auto second_b = node_for(*index, {0, 1});
  const auto expected1 = oracle(gate, points, 0, {2, 3}, 2);
  const auto expected2 = oracle(gate, points, 3, {0, 1}, 5);
  const auto direct1 = reference(gate, index, rank, b, 2, Q2SiblingMode::Saturating, Q2WitnessOrder::GlobalDfs, expected1);
  const auto direct2 = reference(gate, index, second_rank, second_b, 5, Q2SiblingMode::Disabled, Q2WitnessOrder::ComplementFirst, expected2);
  auto first = mhgp8::make_q2_census_continuation(index, rank, b, 2, Q2SiblingMode::Saturating);
  auto second = mhgp8::make_q2_census_continuation(index, second_rank, second_b, 5, Q2SiblingMode::Disabled, Q2WitnessOrder::ComplementFirst);
  Output out1, out2;
  const mhgp8::Q2CensusConsumer consume1 = [&](const mhgp8::Q2Support& item) { out1.push_back(copy(item)); };
  const mhgp8::Q2CensusConsumer consume2 = [&](const mhgp8::Q2Support& item) { out2.push_back(copy(item)); };
  const std::weak_ptr<const mhgp8::Q2CensusIndex> weak = index;
  const auto* identity = index.get();
  index.reset(); points.assign(2, Point3{1, 1, 1});
  gate.require(!weak.expired(), "continuation did not retain the original immutable index");
  ++gate.owner_resets;
  bool done1 = false, done2 = false;
  std::size_t turns = 0;
  while (!done1 || !done2) {
    if (!done1) done1 = advance_checked(gate, *first, *identity, b, 2, 1, consume1);
    if (!done2) done2 = advance_checked(gate, *second, *identity, second_b, 5, 2, consume2);
    gate.require(++turns < 1000000, "interleaved small continuations failed to terminate");
  }
  same_work(gate, first->snapshot(), direct1.result); same_work(gate, second->snapshot(), direct2.result);
  gate.require(out1 == direct1.output && out2 == direct2.output, "independent continuations shared mutable state or payload buffers");
  ++gate.two_engines;
  first.reset(); second.reset();
  gate.require(weak.expired(), "destroyed continuations retained their final index owner");

  const Points transferred{{0, 0, 0}, {5, 0, 0}, {10, 0, 0}, {11, 0, 0}};
  index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(transferred));
  auto migrating = mhgp8::make_q2_census_continuation(index, rank, b, 2);
  const auto direct = reference(gate, index, rank, b, 2, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, expected1);
  index.reset();
  Output output;
  const mhgp8::Q2CensusConsumer consume = [&](const mhgp8::Q2Support& item) { output.push_back(copy(item)); };
  std::exception_ptr error;
  std::thread one([&] { try { static_cast<void>(migrating->advance(1, consume)); } catch (...) { error = std::current_exception(); } });
  one.join(); if (error) std::rethrow_exception(error);
  gate.require(migrating->snapshot().status == Q2CensusContinuationStatus::Ready, "first thread did not leave a genuine suspended state");
  std::thread two([&] { try { while (!migrating->advance(2, consume)) {} } catch (...) { error = std::current_exception(); } });
  two.join(); if (error) std::rethrow_exception(error);
  same_work(gate, migrating->snapshot(), direct.result);
  gate.require(output == direct.output, "sequential thread transfer invalidated a stack context or callback lifetime");
  ++gate.thread_transfers;
}

void invalid_and_callbacks(Gate& gate) {
  const Points points{{0, 0, 0}, {5, 0, 0}, {10, 0, 0}, {11, 0, 0}};
  const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
  const auto rank = rank_of(*index, 0); const auto b = node_for(*index, {2, 3});
  const auto make = [&](mhgp8::Q2CensusIndexPtr owner, std::size_t a, std::size_t node, unsigned k,
                        Q2SiblingMode sibling = Q2SiblingMode::Disabled, Q2WitnessOrder order = Q2WitnessOrder::GlobalDfs) {
    return mhgp8::make_q2_census_continuation(std::move(owner), a, node, k, sibling, order);
  };
  gate.rejects([&] { static_cast<void>(make({}, rank, b, 2)); }, "null index owner was accepted");
  gate.rejects([&] { static_cast<void>(make(index, points.size(), b, 2)); }, "out-of-range anchor rank was accepted");
  gate.rejects([&] { static_cast<void>(make(index, rank, index->spatial_nodes().size(), 2)); }, "out-of-range B node was accepted");
  gate.rejects([&] { static_cast<void>(make(index, rank, 0, 2)); }, "B containing the anchor was accepted");
  for (const unsigned k : {0U, 11U})
    gate.rejects([&] { static_cast<void>(make(index, rank, b, k)); }, "invalid K was accepted");
  gate.rejects([&] { static_cast<void>(make(index, rank, b, 2, static_cast<Q2SiblingMode>(99))); }, "invalid sibling mode was accepted");
  gate.rejects([&] { static_cast<void>(make(index, rank, b, 2, Q2SiblingMode::Disabled, static_cast<Q2WitnessOrder>(99))); },
               "invalid witness order was accepted");
  auto valid = make(index, rank, b, 2);
  u64 callbacks = 0;
  const mhgp8::Q2CensusConsumer count = [&](const mhgp8::Q2Support&) { ++callbacks; };
  gate.rejects([&] { static_cast<void>(valid->advance(0, count)); }, "zero quantum was accepted");
  gate.rejects([&] { static_cast<void>(valid->advance(1, {})); }, "empty resume callback was accepted");
  gate.require(callbacks == 0 && valid->snapshot().status == Q2CensusContinuationStatus::Ready &&
                   valid->snapshot().resume_work.transitions == 0, "invalid advance poisoned or executed a Ready continuation");
  gate.rejects([&] { static_cast<void>(mhgp8::run_q2_anchor_reference(index, rank, b, 2, {})); },
               "direct reference accepted an empty callback");
  const auto singleton = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(Points{{7, 11, 13}}));
  gate.rejects([&] { static_cast<void>(make(singleton, 0, 0, 1)); }, "one-point cloud fabricated a disjoint anchor/B root");
  while (!valid->advance(1, count)) {}
  gate.rejects([&] { static_cast<void>(valid->advance(0, count)); }, "Done bypassed the invalid-budget rejection");

  struct CallbackFailure {};
  auto failed = make(index, rank, b, 5);
  u64 failed_callbacks = 0;
  bool caught = false;
  try {
    while (!failed->advance(1, [&](const mhgp8::Q2Support&) { ++failed_callbacks; throw CallbackFailure{}; })) {}
  } catch (const CallbackFailure&) { caught = true; }
  gate.require(caught && failed_callbacks == 1 && failed->snapshot().status == Q2CensusContinuationStatus::Failed,
               "callback exception was swallowed or failed continuation remained resumable");
  gate.rejects<std::logic_error>([&] { static_cast<void>(failed->advance(7, count)); }, "failed continuation retried a partially emitted payload");
  gate.require(failed_callbacks == 1, "failure replayed a callback");
  ++gate.callback_failures;

  const auto expected = oracle(gate, points, 0, {2, 3}, 5);
  auto outer = make(index, rank, b, 5);
  auto inner = make(index, rank, b, 5, Q2SiblingMode::Disabled, Q2WitnessOrder::ComplementFirst);
  Output outer_output, inner_output;
  bool nested_done = false;
  const mhgp8::Q2CensusConsumer consume_inner = [&](const mhgp8::Q2Support& item) { inner_output.push_back(copy(item)); };
  const mhgp8::Q2CensusConsumer consume_outer = [&](const mhgp8::Q2Support& item) {
    outer_output.push_back(copy(item));
    gate.rejects<std::logic_error>([&] { static_cast<void>(outer->advance(1, count)); }, "same-continuation callback reentrance was accepted");
    gate.rejects<std::logic_error>([&] { static_cast<void>(outer->snapshot()); }, "busy snapshot was accepted");
    gate.rejects<std::logic_error>([&] { static_cast<void>(outer->pending()); }, "busy pending observer was accepted");
    gate.rejects<std::logic_error>([&] { static_cast<void>(outer->memory()); }, "busy memory observer was accepted");
    if (!nested_done) { while (!inner->advance(1, consume_inner)) {} nested_done = true; }
    ++gate.reentrant_calls;
  };
  while (!outer->advance(2, consume_outer)) {}
  check_output(gate, outer_output, expected); check_output(gate, inner_output, expected);
  gate.require(outer->snapshot().status == Q2CensusContinuationStatus::Done && nested_done,
               "caught nested rejection poisoned the outer continuation or blocked an independent one");

  // A real overlapping advance, synchronized by callback entry, not sleeps
  // or a lucky scheduling window. Only the main thread changes Gate.
  auto overlapping = make(index, rank, b, 5);
  std::latch entered{1}, release{1};
  Output overlapping_output;
  std::exception_ptr worker_error;
  std::thread worker([&] {
    bool signaled = false;
    try {
      static_cast<void>(overlapping->advance(unlimited, [&](const mhgp8::Q2Support& item) {
        overlapping_output.push_back(copy(item));
        if (!signaled) { signaled = true; entered.count_down(); release.wait(); }
      }));
    } catch (...) { worker_error = std::current_exception(); }
    if (!signaled) entered.count_down();
  });
  entered.wait();
  try {
    gate.rejects<std::logic_error>([&] { static_cast<void>(overlapping->advance(1, count)); }, "overlapping advance was accepted");
    gate.rejects<std::logic_error>([&] { static_cast<void>(overlapping->snapshot()); }, "overlapping snapshot was accepted");
    gate.rejects<std::logic_error>([&] { static_cast<void>(overlapping->pending()); }, "overlapping pending observer was accepted");
    gate.rejects<std::logic_error>([&] { static_cast<void>(overlapping->memory()); }, "overlapping memory observer was accepted");
  } catch (...) { release.count_down(); worker.join(); throw; }
  release.count_down(); worker.join();
  if (worker_error) std::rethrow_exception(worker_error);
  check_output(gate, overlapping_output, expected);
  gate.require(overlapping->snapshot().status == Q2CensusContinuationStatus::Done,
               "rejected overlapping caller poisoned the acquired outer advance");
  ++gate.overlap_rejections;

  auto duplicate = expected; duplicate.push_back(expected.front()); sort(duplicate);
  gate.require(duplicate != expected, "duplicate-support mutant was vacuous"); ++gate.mutants;
  auto wrong_credit = expected;
  wrong_credit.erase(std::remove_if(wrong_credit.begin(), wrong_credit.end(), [](const auto& item) { return item.b == 2; }), wrong_credit.end());
  gate.require(wrong_credit != expected, "repeated/lost-credit support mutant was vacuous"); ++gate.mutants;
  auto shell = expected; shell.front().shell.pop_back();
  gate.require(shell != expected, "truncated-shell mutant was vacuous"); ++gate.mutants;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_q2_census_resume_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    matrix(gate); lifetimes(gate); invalid_and_callbacks(gate);
    gate.require(gate.resumptions >= 700 && gate.credit_pauses > 0 && gate.split_credit_pauses > 0 &&
                     gate.phase_pauses > 0 && gate.deferred_child_pauses > 0 && gate.emission_pauses > 0 && gate.max_shell >= 30 &&
                     gate.owner_resets > 0 && gate.thread_transfers > 0 && gate.callback_failures > 0 && gate.reentrant_calls > 0 &&
                     gate.overlap_rejections > 0,
                 "resume gate lost a required positive suspension/lifetime/exception fixture");
    std::cout << "{\"schema\":\"mhgp8_q2_census_resume_gate_v1\",\"status\":\"pass\",\"scope\":\"shared_anchor_continuations_not_full\""
              << ",\"checks\":" << gate.checks << ",\"fixtures\":" << gate.fixtures << ",\"roots\":" << gate.roots
              << ",\"oracle_pairs\":" << gate.oracle_pairs << ",\"oracle_sites\":" << gate.oracle_sites
              << ",\"references\":" << gate.references << ",\"resumptions\":" << gate.resumptions
              << ",\"advances\":" << gate.advances << ",\"supports\":" << gate.supports << ",\"max_shell\":" << gate.max_shell
              << ",\"credit_pauses\":" << gate.credit_pauses << ",\"split_credit_pauses\":" << gate.split_credit_pauses
              << ",\"phase_pauses\":" << gate.phase_pauses << ",\"emission_pauses\":" << gate.emission_pauses
              << ",\"deferred_child_pauses\":" << gate.deferred_child_pauses
              << ",\"partial_range_pauses\":" << gate.partial_range_pauses << ",\"two_engines\":" << gate.two_engines
              << ",\"thread_transfers\":" << gate.thread_transfers << ",\"owner_resets\":" << gate.owner_resets
              << ",\"callback_failures\":" << gate.callback_failures << ",\"reentrant_calls\":" << gate.reentrant_calls
              << ",\"overlap_rejections\":" << gate.overlap_rejections
              << ",\"invalid_inputs\":" << gate.invalid_inputs << ",\"mutants\":" << gate.mutants << "}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp8 q2 resume gate: " << error.what() << '\n';
    return 1;
  }
}
