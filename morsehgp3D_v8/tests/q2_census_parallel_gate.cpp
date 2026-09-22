#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <latch>
#include <limits>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include "pipeline/q2_census_parallel.hpp"

namespace {
using mhgp8::Point3;
using mhgp8::Q2SiblingMode;
using mhgp8::Q2WitnessOrder;
using mhgp8::u64;
using Points = std::vector<Point3>;

struct Support {
  std::size_t a{}, b{};
  mhgp8::Q2BallKey key;
  std::vector<std::size_t> interior, shell;
  bool operator==(const Support&) const = default;
};
using Output = std::vector<Support>;

struct Gate {
  u64 checks{}, fixtures{}, references{}, resumptions{}, parallel_calls{}, supports{};
  u64 donations{}, multi_slot_outputs{}, multi_active_runs{}, callback_failures{}, invalid_inputs{};
  u64 max_shell{};
  void require(bool value, const char* message) {
    ++checks;
    if (!value) throw std::runtime_error(message);
  }
  template<class Function>
  void invalid(Function&& function) {
    bool caught = false;
    try { function(); } catch (const std::invalid_argument&) { caught = true; }
    require(caught, "invalid parallel anchor input was accepted");
    ++invalid_inputs;
  }
};

Support copy(const mhgp8::Q2Support& value) {
  Support result{std::min(value.a_id, value.b_id), std::max(value.a_id, value.b_id),
                 value.key, {value.interior.begin(), value.interior.end()},
                 {value.shell.begin(), value.shell.end()}};
  std::sort(result.interior.begin(), result.interior.end());
  std::sort(result.shell.begin(), result.shell.end());
  return result;
}

void normalize(Output& output) {
  std::sort(output.begin(), output.end(), [](const Support& a, const Support& b) {
    return a.a < b.a || (a.a == b.a && a.b < b.b);
  });
}

std::size_t rank_of(const mhgp8::Q2CensusIndex& index, std::size_t id) {
  const auto order = index.spatial_order();
  const auto found = std::find(order.begin(), order.end(), id);
  if (found == order.end()) throw std::runtime_error("fixture original ID missing");
  return static_cast<std::size_t>(found - order.begin());
}

std::vector<std::size_t> ids_of(const mhgp8::Q2CensusIndex& index, std::size_t node) {
  const auto range = index.spatial_nodes()[node].range;
  const auto order = index.spatial_order();
  return {order.begin() + static_cast<std::ptrdiff_t>(range.first),
          order.begin() + static_cast<std::ptrdiff_t>(range.last)};
}

std::size_t select_node(const mhgp8::Q2CensusIndex& index, std::size_t rank,
                        std::vector<std::size_t> wanted) {
  std::sort(wanted.begin(), wanted.end());
  auto best = mhgp8::Q2SpatialNode::absent;
  std::size_t best_size = 0;
  for (std::size_t id = 0; id < index.spatial_nodes().size(); ++id) {
    const auto range = index.spatial_nodes()[id].range;
    if (range.first <= rank && rank < range.last) continue;
    if (wanted.empty()) {
      if (range.size() > best_size) { best = id; best_size = range.size(); }
    } else if (range.size() == wanted.size()) {
      auto actual = ids_of(index, id);
      std::sort(actual.begin(), actual.end());
      if (actual == wanted) return id;
    }
  }
  if (best == mhgp8::Q2SpatialNode::absent)
    throw std::runtime_error("fixture target is not a disjoint real index node");
  return best;
}

// Independent scalar H/keys/full-shell oracle, limited to these tiny fixtures.
// Neither the recursive reference nor a product bound supplies its answer.
Output oracle(Gate& gate, const Points& points, std::size_t anchor,
              const std::vector<std::size_t>& targets, unsigned k) {
  gate.require(points.size() <= 100, "parallel gate oracle exceeded declared small domain");
  Output result;
  for (const auto b : targets) {
    Support item;
    item.a = std::min(anchor, b); item.b = std::max(anchor, b);
    for (std::size_t axis = 0; axis < 3; ++axis) {
      item.key.center_twice[axis] = static_cast<std::uint32_t>(points[anchor][axis]) + points[b][axis];
      const auto d = std::int64_t{points[anchor][axis]} - points[b][axis];
      item.key.diameter_squared += static_cast<u64>(d * d);
    }
    for (std::size_t z = 0; z < points.size(); ++z) {
      std::int64_t h = 0;
      for (std::size_t axis = 0; axis < 3; ++axis)
        h += (std::int64_t{points[z][axis]} - points[anchor][axis]) *
             (std::int64_t{points[b][axis]} - points[z][axis]);
      if (h > 0) item.interior.push_back(z);
      if (h == 0) item.shell.push_back(z);
    }
    if (item.interior.size() < k) result.push_back(std::move(item));
  }
  normalize(result);
  return result;
}

// Explicit independent enumeration also checks the worker reduction.
// The build-depth maximum is always zero on this no-rebuild API.
auto geometric(const mhgp8::Q2CensusResumeSnapshot& s) {
  const auto& w = s.census.work;
  const auto& b = s.sibling_work;
  const auto& o = s.order_work;
  return std::array{
      s.census.candidate_pairs, s.census.accepted_pairs, s.census.rejected_pairs,
      w.query_build_point_visits, w.query_build_nodes, w.query_build_max_depth,
      w.input_descriptors, w.query_cover_visits, w.query_tasks, w.query_splits,
      w.witness_splits, w.count_root_starts, w.shared_splits_after_credit,
      w.cursor_advances, w.cursor_reuses, w.count_node_visits, w.count_bound_tests,
      w.count_point_tests, w.uniform_credited_pairs, w.uniform_rejected_pairs,
      w.uniform_accepted_pairs, w.consumed_witness_sites, w.frontier_restarts,
      w.payload_node_visits, w.payload_bound_tests, w.payload_point_tests,
      w.payload_interior_sites, w.payload_shell_sites, w.payload_supports,
      b.proposals, b.cardinality_skips, b.bound_tests, b.rejected_tasks,
      b.rejected_pairs, b.rejected_after_credit, o.structural_splits,
      o.deferred_skips, o.anchor_skips, o.phase_switches};
}

auto steps(const mhgp8::Q2CensusResumeWork& w) {
  return std::array{w.transitions, w.entry_steps, w.witness_steps, w.admission_steps, w.payload_steps};
}

auto additive_schedule(const mhgp8::Q2CensusScheduleWork& w) {
  return std::array{w.offer_checks, w.offer_busy, w.offer_full, w.offer_no_sibling,
                    w.donations, w.fragments_started, w.fragments_completed, w.waits, w.wakes};
}

auto additive_resume(const mhgp8::Q2CensusResumeWork& w) {
  return std::array{w.advance_calls, w.transitions, w.entry_steps, w.witness_steps,
                    w.admission_steps, w.payload_steps, w.pauses, w.pauses_after_credit,
                    w.pauses_inside_deferred, w.pauses_during_emission};
}

auto additive_detach(const mhgp8::Q2CensusResumeSnapshot& s) {
  const auto& d = s.detach_work;
  return std::array{d.attempts, d.detached_frames, d.imported_frames, d.transferred_pairs, d.moved_frames};
}

template<std::size_t N>
void add(std::array<u64, N>& out, const std::array<u64, N>& value) {
  for (std::size_t i = 0; i < N; ++i) out[i] += value[i];
}

bool close(double a, double b) {
  return std::abs(a - b) <= 1e-7 * std::max({1.0, std::abs(a), std::abs(b)});
}

void times(Gate& gate, const mhgp8::Q2CensusResult& census) {
  for (const double value : {census.query_index_ms, census.total_ms, census.count_ms, census.payload_ms})
    gate.require(std::isfinite(value) && value >= 0, "parallel census has invalid clocks");
  gate.require(census.query_index_ms == 0 && close(census.total_ms, census.count_ms + census.payload_ms),
               "parallel fragment clocks lost active-count/payload partition");
}

void compare(Gate& gate, const mhgp8::Q2CensusParallelResult& result,
             const mhgp8::Q2CensusAnchorResult& reference,
             const mhgp8::Q2CensusResumeSnapshot& mono, mhgp8::Q2CensusParallelOptions options,
             const std::vector<Output>& output, const Output& expected) {
  const auto& s = result.sum;
  gate.require(s.status == mhgp8::Q2CensusContinuationStatus::Done &&
                   s.census.work == reference.census.work && s.sibling_work == reference.sibling_work &&
                   s.order_work == reference.order_work && geometric(s) == geometric(mono),
               "parallel anchor changed geometric work, masses, sibling certificate or witness order");
  gate.require(steps(s.resume_work) == steps(mono.resume_work),
               "parallel anchor replayed or omitted a geometric/payload transition");
  gate.require(result.workers.size() == options.workers && output.size() == options.workers,
               "parallel result omitted a callback slot");
  auto sum_geo = geometric(s); sum_geo.fill(0);
  auto sum_schedule = additive_schedule(result.schedule); sum_schedule.fill(0);
  auto sum_resume = additive_resume(s.resume_work); sum_resume.fill(0);
  auto sum_detach = additive_detach(s); sum_detach.fill(0);
  u64 max_queue = 0, max_active = 0, max_memory = 0, max_pending = 0, nonempty = 0;
  double sum_total = 0, sum_count = 0, sum_payload = 0;
  Output all;
  for (std::size_t slot = 0; slot < options.workers; ++slot) {
    const auto& w = result.workers[slot];
    gate.require(w.sum.status == mhgp8::Q2CensusContinuationStatus::Done &&
                     w.sum.census.work.query_build_max_depth == 0,
                 "worker ended unfinished or rebuilt its query tree");
    add(sum_geo, geometric(w.sum)); add(sum_schedule, additive_schedule(w.schedule));
    add(sum_resume, additive_resume(w.sum.resume_work)); add(sum_detach, additive_detach(w.sum));
    max_queue = std::max(max_queue, w.schedule.max_queue_size);
    max_active = std::max(max_active, w.schedule.max_active_fragments);
    max_memory = std::max(max_memory, w.max_fragment_bytes);
    max_pending = std::max(max_pending, w.sum.resume_work.max_pending_tasks);
    sum_total += w.sum.census.total_ms; sum_count += w.sum.census.count_ms;
    sum_payload += w.sum.census.payload_ms;
    times(gate, w.sum.census);
    gate.require(w.sum.census.total_ms <= result.total_ms + 1e-6,
                 "one worker active intervals exceed the enclosing wall interval");
    gate.require(w.sum.census.candidate_pairs == w.sum.census.accepted_pairs + w.sum.census.rejected_pairs &&
                     w.sum.census.work.payload_supports == output[slot].size() &&
                     w.sum.census.work.query_tasks + w.sum.detach_work.detached_frames ==
                         w.sum.census.work.count_root_starts + 2 * w.sum.census.work.query_splits +
                         w.sum.detach_work.imported_frames,
                 "worker fragment mass/import/export/task ledger is inconsistent");
    if (!output[slot].empty()) ++nonempty;
    all.insert(all.end(), output[slot].begin(), output[slot].end());
  }
  gate.require(sum_geo == geometric(s) && sum_schedule == additive_schedule(result.schedule) &&
                   sum_resume == additive_resume(s.resume_work) && sum_detach == additive_detach(s),
               "parallel reduction lost or doubled a worker counter");
  gate.require(max_queue == result.schedule.max_queue_size && max_active == result.schedule.max_active_fragments &&
                   max_memory == result.max_fragment_bytes && max_pending == s.resume_work.max_pending_tasks &&
                   close(sum_total, s.census.total_ms) && close(sum_count, s.census.count_ms) &&
                   close(sum_payload, s.census.payload_ms),
               "parallel reduction confused maxima, active intervals and sums");
  times(gate, s.census);
  const auto& work = result.schedule;
  const auto& d = s.detach_work;
  gate.require(work.offer_checks == work.offer_busy + work.offer_full + work.offer_no_sibling + work.donations &&
                   work.fragments_started == work.fragments_completed &&
                   work.fragments_completed == 1 + work.donations &&
                   work.waits == work.wakes && d.detached_frames == d.imported_frames &&
                   d.detached_frames == work.donations && d.attempts == work.offer_no_sibling + work.donations &&
                   d.detached_frames <= s.census.work.query_splits,
               "scheduler success ledger omitted a queue refusal, donation, wake or fragment");
  gate.require(work.max_queue_size >= 1 && work.max_queue_size <= options.queue_capacity &&
                   work.max_active_fragments >= 1 && work.max_active_fragments <= options.workers &&
                   result.queue_storage_bytes >= options.queue_capacity * sizeof(std::unique_ptr<mhgp8::Q2CensusContinuation>) &&
                   s.resume_work.max_pending_tasks >= 1 && s.resume_work.max_pending_tasks <= mhgp8::index_stack_frames &&
                   result.max_fragment_bytes > 0,
               "scheduler memory/activity/pending peaks omit retained obligations");
  gate.require(std::isfinite(result.total_ms) && result.total_ms >= 0,
               "scheduler wall time is not finite and nonnegative");
  if (options.workers == 1)
    gate.require(work.donations == 0 && work.offer_checks == 0 && work.fragments_completed == 1,
                 "single worker performed queue offers or fabricated detached roots");
  normalize(all);
  gate.require(all == expected, "parallel supports/keys/interiors/full shells differ from independent oracle");
  for (const auto& item : all) {
    gate.require(std::adjacent_find(item.interior.begin(), item.interior.end()) == item.interior.end() &&
                     std::adjacent_find(item.shell.begin(), item.shell.end()) == item.shell.end() &&
                     std::binary_search(item.shell.begin(), item.shell.end(), item.a) &&
                     std::binary_search(item.shell.begin(), item.shell.end(), item.b),
                 "parallel payload lost endpoints or duplicated shell/interior IDs");
    gate.max_shell = std::max(gate.max_shell, static_cast<u64>(item.shell.size()));
  }
  gate.supports += all.size(); gate.donations += work.donations;
  if (nonempty > 1) ++gate.multi_slot_outputs;
  if (work.max_active_fragments > 1) ++gate.multi_active_runs;
  ++gate.parallel_calls;
}

struct Fixture { Points points; std::size_t anchor{}; std::vector<std::size_t> targets; };

std::vector<Fixture> fixtures() {
  std::vector<Fixture> result{
      {{{0, 0, 0}, {65535, 65535, 65535}}, 0, {1}},
      {{{0, 0, 0}, {5, 0, 0}, {10, 0, 0}, {11, 0, 0}}, 0, {2, 3}},
      {{{1000, 0, 0}, {0, 1, 0}, {0, 0, 0}}, 0, {1, 2}},
      {{{0, 1, 1}, {3, 2, 2}, {1, 0, 1}, {2, 3, 2}, {1, 1, 1}, {1, 1, 4}}, 0, {1}}};
  Fixture sphere;
  for (int x = -5; x <= 5; ++x) for (int y = -5; y <= 5; ++y) for (int z = -5; z <= 5; ++z)
    if (x*x + y*y + z*z == 25)
      sphere.points.push_back({static_cast<std::uint16_t>(10 + x), static_cast<std::uint16_t>(10 + y),
                               static_cast<std::uint16_t>(10 + z)});
  sphere.targets = {sphere.points.size() - 1}; result.push_back(std::move(sphere));
  Fixture credited{{{0, 0, 0}, {500, 0, 0}}, 0, {}};
  for (unsigned x = 0; x < 64; ++x) {
    credited.targets.push_back(credited.points.size());
    credited.points.push_back({static_cast<std::uint16_t>(1000 + x), 0, 0});
  }
  result.push_back(std::move(credited));
  Fixture deferred;
  for (unsigned x = 0; x < 4; ++x) for (unsigned y = 0; y < 4; ++y) for (unsigned z = 0; z < 4; ++z) {
    deferred.targets.push_back(deferred.points.size());
    deferred.points.push_back({static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                               static_cast<std::uint16_t>(z)});
  }
  deferred.anchor = deferred.points.size(); deferred.points.push_back({1000, 1000, 1000});
  for (unsigned x = 997; x < 1000; ++x) for (unsigned y = 998; y < 1000; ++y)
    deferred.points.push_back({static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y), 999});
  result.push_back(std::move(deferred));
  Fixture mixed;
  for (unsigned i = 0; i < 24; ++i)
    mixed.points.push_back({static_cast<std::uint16_t>(137 * i),
                            static_cast<std::uint16_t>((379 * i + 83) % 5000),
                            static_cast<std::uint16_t>((811 * i + 97) % 4000)});
  result.push_back(std::move(mixed));
  return result;
}

void matrix(Gate& gate) {
  for (const auto& fixture : fixtures()) {
    const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(fixture.points));
    const auto rank = rank_of(*index, fixture.anchor);
    const auto b = select_node(*index, rank, fixture.targets);
    ++gate.fixtures;
    for (const unsigned k : {1U, 2U, 5U, 10U}) {
      const auto expected = oracle(gate, fixture.points, fixture.anchor, ids_of(*index, b), k);
      for (const auto sibling : {Q2SiblingMode::Disabled, Q2SiblingMode::Saturating})
        for (const auto order : {Q2WitnessOrder::GlobalDfs, Q2WitnessOrder::ComplementFirst}) {
          Output direct_output;
          const auto direct = mhgp8::run_q2_anchor_reference(index, rank, b, k,
              [&](const mhgp8::Q2Support& item) { direct_output.push_back(copy(item)); }, sibling, order);
          normalize(direct_output);
          gate.require(direct_output == expected, "recursive reference differs from scalar oracle");
          ++gate.references;
          auto continuation = mhgp8::make_q2_census_continuation(index, rank, b, k, sibling, order);
          const auto done = continuation->advance(std::numeric_limits<std::size_t>::max(),
              [](const mhgp8::Q2Support&) {});
          gate.require(done, "unlimited tiny reference continuation did not complete");
          const auto mono = continuation->snapshot();
          ++gate.resumptions;
          for (const std::size_t workers : {1U, 2U, 4U})
            for (const std::size_t queue : {1U, 8U})
              for (const std::size_t quantum : {1U, 7U, 256U}) {
                const mhgp8::Q2CensusParallelOptions options{workers, quantum, queue};
                std::vector<Output> outputs(workers);
                std::array<std::atomic_flag, 4> busy{};
                std::atomic<bool> overlap{false};
                std::vector<mhgp8::Q2CensusConsumer> consumers;
                consumers.reserve(workers);
                for (std::size_t slot = 0; slot < workers; ++slot)
                  consumers.emplace_back([&, slot](const mhgp8::Q2Support& item) {
                    if (busy[slot].test_and_set(std::memory_order_acquire)) {
                      overlap.store(true, std::memory_order_relaxed);
                      throw std::runtime_error("same callback slot entered concurrently");
                    }
                    try { outputs[slot].push_back(copy(item)); }
                    catch (...) { busy[slot].clear(std::memory_order_release); throw; }
                    busy[slot].clear(std::memory_order_release);
                  });
                const auto actual = mhgp8::run_q2_anchor_parallel(index, rank, b, k, options, consumers, sibling, order);
                gate.require(!overlap.load(std::memory_order_relaxed), "one callback slot was shared by simultaneous workers");
                compare(gate, actual, direct, mono, options, outputs, expected);
              }
        }
    }
  }
}

void invalid_inputs(Gate& gate) {
  const Points points{{0, 0, 0}, {5, 0, 0}, {10, 0, 0}, {11, 0, 0}};
  const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
  const auto rank = rank_of(*index, 0);
  const auto node = select_node(*index, rank, {2, 3});
  std::atomic<unsigned> calls{0};
  const mhgp8::Q2CensusConsumer callback = [&](const mhgp8::Q2Support&) { ++calls; };
  std::vector<mhgp8::Q2CensusConsumer> two{callback, callback};
  const auto run = [&](mhgp8::Q2CensusIndexPtr owner, std::size_t a, std::size_t b, unsigned k,
                       mhgp8::Q2CensusParallelOptions options, std::span<const mhgp8::Q2CensusConsumer> callbacks,
                       Q2SiblingMode sibling = Q2SiblingMode::Disabled,
                       Q2WitnessOrder order = Q2WitnessOrder::GlobalDfs) {
    static_cast<void>(mhgp8::run_q2_anchor_parallel(std::move(owner), a, b, k, options, callbacks, sibling, order));
  };
  for (const auto options : {mhgp8::Q2CensusParallelOptions{0, 1, 1},
                             mhgp8::Q2CensusParallelOptions{2, 0, 1},
                             mhgp8::Q2CensusParallelOptions{2, 1, 0}})
    gate.invalid([&] { run(index, rank, node, 2, options, two); });
  gate.invalid([&] { run(index, rank, node, 2, {1, 1, 1}, two); });
  gate.invalid([&] { run(index, rank, node, 2, {2, 1, 1}, {}); });
  auto empty_slot = two; empty_slot[1] = {};
  gate.invalid([&] { run(index, rank, node, 2, {2, 1, 1}, empty_slot); });
  gate.invalid([&] { run({}, rank, node, 2, {2, 1, 1}, two); });
  gate.invalid([&] { run(index, points.size(), node, 2, {2, 1, 1}, two); });
  gate.invalid([&] { run(index, rank, index->spatial_nodes().size(), 2, {2, 1, 1}, two); });
  gate.invalid([&] { run(index, rank, 0, 2, {2, 1, 1}, two); });
  for (const unsigned k : {0U, 11U}) gate.invalid([&] { run(index, rank, node, k, {2, 1, 1}, two); });
  gate.invalid([&] { run(index, rank, node, 2, {2, 1, 1}, two, static_cast<Q2SiblingMode>(99)); });
  gate.invalid([&] { run(index, rank, node, 2, {2, 1, 1}, two,
                          Q2SiblingMode::Disabled, static_cast<Q2WitnessOrder>(99)); });
  gate.require(calls.load() == 0, "invalid options emitted callbacks before rejection");
}

void failures_and_join(Gate& gate) {
  struct CallbackFailure {};
  const Points points{{0, 0, 0}, {5, 0, 0}, {10, 0, 0}, {11, 0, 0}};
  const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
  const auto rank = rank_of(*index, 0);
  const auto node = select_node(*index, rank, {2, 3});
  for (const std::size_t workers : {1U, 2U, 4U}) {
    std::atomic<unsigned> calls{0}, active{0};
    std::vector<mhgp8::Q2CensusConsumer> callbacks;
    for (std::size_t slot = 0; slot < workers; ++slot) {
      static_cast<void>(slot);
      callbacks.emplace_back([&](const mhgp8::Q2Support&) {
        active.fetch_add(1);
        calls.fetch_add(1);
        active.fetch_sub(1);
        throw CallbackFailure{};
      });
    }
    bool caught = false;
    try {
      static_cast<void>(mhgp8::run_q2_anchor_parallel(index, rank, node, 10, {workers, 1, 1}, callbacks));
    } catch (const CallbackFailure&) { caught = true; }
    gate.require(caught && calls.load() > 0 && calls.load() <= 2 && active.load() == 0,
                 "callback failure was swallowed, replayed, or returned before callback exit");
    ++gate.callback_failures;
  }
  // Main explicitly releases a real blocked callback before joining. No
  // callback waits for a donation or for another callback, so there is no
  // circular scheduler dependency even if all other workers are still idle.
  std::latch entered{1}, release{1};
  std::atomic<bool> claimed{false}, returned{false};
  std::atomic<unsigned> active{0}, calls{0};
  std::exception_ptr error;
  std::vector<mhgp8::Q2CensusConsumer> callbacks;
  for (unsigned slot = 0; slot < 4; ++slot) {
    static_cast<void>(slot);
    callbacks.emplace_back([&](const mhgp8::Q2Support&) {
      active.fetch_add(1); calls.fetch_add(1);
      if (!claimed.exchange(true)) { entered.count_down(); release.wait(); }
      active.fetch_sub(1);
      throw CallbackFailure{};
    });
  }
  std::thread invocation([&] {
    try {
      static_cast<void>(mhgp8::run_q2_anchor_parallel(index, rank, node, 10, {4, 1, 1}, callbacks));
    } catch (...) { error = std::current_exception(); }
    returned.store(true, std::memory_order_release);
    // Also release the observer if an unexpected pre-callback failure (or
    // missing-output bug) terminates the invocation: report failure, not hang.
    if (!claimed.exchange(true)) entered.count_down();
  });
  entered.wait();
  const bool premature = returned.load(std::memory_order_acquire);
  release.count_down();
  invocation.join();
  bool correct = false;
  try { if (error) std::rethrow_exception(error); } catch (const CallbackFailure&) { correct = true; }
  gate.require(!premature && returned.load() && active.load() == 0 && correct && calls.load() > 0,
               "exception path failed to wait for a real in-flight callback and join every worker");
  ++gate.callback_failures;
}
}  // namespace

int main() {
  try {
    Gate gate;
    matrix(gate);
    invalid_inputs(gate);
    failures_and_join(gate);
    gate.require(gate.donations > 0 && gate.multi_slot_outputs > 0 && gate.multi_active_runs > 0 && gate.max_shell >= 30,
                 "parallel gate was vacuous for donations, distinct callback workers, concurrent fragments or full shells");
    std::cout << "{\"status\":\"passed\",\"scope\":\"small_q2_anchor_parallel_not_full\","
              << "\"checks\":" << gate.checks << ",\"fixtures\":" << gate.fixtures
              << ",\"references\":" << gate.references << ",\"resumptions\":" << gate.resumptions
              << ",\"parallel_calls\":" << gate.parallel_calls << ",\"supports\":" << gate.supports
              << ",\"donations\":" << gate.donations << ",\"multi_slot_outputs\":" << gate.multi_slot_outputs
              << ",\"multi_active_runs\":" << gate.multi_active_runs << ",\"max_shell\":" << gate.max_shell
              << ",\"callback_failures\":" << gate.callback_failures << ",\"invalid_inputs\":" << gate.invalid_inputs
              << ",\"partial_launch_failure_injected\":false}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "q2_census_parallel_gate: " << error.what() << '\n';
    return 1;
  } catch (...) {
    std::cerr << "q2_census_parallel_gate: unexpected nonstandard exception\n";
    return 1;
  }
}
