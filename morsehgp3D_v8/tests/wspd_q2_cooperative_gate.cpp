#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <latch>
#include <limits>
#include <memory>
#include <set>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "pipeline/wspd_q2_cooperative.hpp"

namespace {
using mhgp8::Point3;
using mhgp8::Q2SiblingMode;
using mhgp8::Q2WitnessOrder;
using mhgp8::WspdFrontMode;
using mhgp8::WspdQ2CooperativeOptions;
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
  u64 checks{}, clouds{}, oracle_pairs{}, oracle_sites{}, coarse_runs{}, cooperative_runs{}, default_runs{};
  u64 supports{}, max_shell{}, continued{}, donations{}, after_seeds_claimed{}, credit_pauses{}, phase_pauses{};
  u64 empty_runs{}, small_only_runs{}, pool_blocks_continuations{}, selected64{}, filtered64{}, passthrough{};
  u64 callback_failures{}, reentrant_runs{}, index_resets{}, multiple_callback_threads{}, callback_threads_joined{};
  u64 invalid_inputs{}, mutants{};
  // The 18-bit twin corpus is counted under its own exact pins so that every
  // u16 pin (clouds, default_runs, max_shell) keeps its historical value.
  bool wide{};
  u64 clouds18{}, default_runs18{}, supports18{}, max_shell18{};
  void require(bool condition, const char* message) {
    ++checks; if (!condition) throw std::runtime_error(message);
  }
  template<class Exception = std::invalid_argument, class Function>
  void rejects(Function&& function, const char* message) {
    bool caught = false;
    try { function(); } catch (const Exception&) { caught = true; }
    require(caught, message); ++invalid_inputs;
  }
};

Support copy(const mhgp8::Q2Support& value) {
  Support result{std::min(value.a_id, value.b_id), std::max(value.a_id, value.b_id), value.key,
                 {value.interior.begin(), value.interior.end()}, {value.shell.begin(), value.shell.end()}};
  std::sort(result.interior.begin(), result.interior.end());
  std::sort(result.shell.begin(), result.shell.end());
  return result;
}
void normalize(Output& output) {
  std::sort(output.begin(), output.end(), [](const auto& a, const auto& b) {
    return a.a < b.a || (a.a == b.a && a.b < b.b);
  });
}
Output join(const std::vector<Output>& slots) {
  Output result;
  for (const auto& slot : slots) result.insert(result.end(), slot.begin(), slot.end());
  normalize(result); return result;
}

// Explicit reuse of the independent scalar-oracle method in the dynamic
// gate, not its product answers. No box predicate, front, or census is used.
// Each promoted three-axis dot product fits int64: |H| <= 3*262143^2 < 2^38.
Output oracle(Gate& gate, const Points& points) {
  gate.require(!points.empty() && points.size() <= 100, "cooperative oracle exceeded its bounded n<=100 domain");
  Output result;
  for (std::size_t a = 0; a < points.size(); ++a) for (std::size_t b = a + 1; b < points.size(); ++b) {
    Support item; item.a = a; item.b = b;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      item.key.center_twice[axis] = static_cast<std::uint32_t>(points[a][axis]) + points[b][axis];
      const auto delta = std::int64_t{points[a][axis]} - points[b][axis];
      item.key.diameter_squared += static_cast<u64>(delta * delta);
    }
    for (std::size_t z = 0; z < points.size(); ++z) {
      std::int64_t h = 0;
      for (std::size_t axis = 0; axis < 3; ++axis)
        h += (std::int64_t{points[z][axis]} - points[a][axis]) *
             (std::int64_t{points[b][axis]} - points[z][axis]);
      if (h > 0) item.interior.push_back(z);
      if (h == 0) item.shell.push_back(z);
      ++gate.oracle_sites;
    }
    result.push_back(std::move(item)); ++gate.oracle_pairs;
  }
  return result;
}
Output accepted(const Output& all, unsigned k) {
  Output result;
  for (const auto& item : all) if (item.interior.size() < k) result.push_back(item);
  return result;
}
void check_output(Gate& gate, const Output& output, const Output& expected) {
  gate.require(output == expected, "cooperative support/key/interior/full-shell multiset differs from brute-force oracle");
  for (const auto& item : output) {
    gate.require(std::adjacent_find(item.interior.begin(), item.interior.end()) == item.interior.end() &&
                     std::adjacent_find(item.shell.begin(), item.shell.end()) == item.shell.end() &&
                     std::binary_search(item.shell.begin(), item.shell.end(), item.a) &&
                     std::binary_search(item.shell.begin(), item.shell.end(), item.b),
                 "cooperative payload duplicated an ID or removed a support endpoint from shell");
    auto& max_shell = gate.wide ? gate.max_shell18 : gate.max_shell;
    max_shell = std::max(max_shell, static_cast<u64>(item.shell.size()));
  }
  (gate.wide ? gate.supports18 : gate.supports) += output.size();
}

struct GeometryOptions {
  unsigned k{10}, s{8};
  WspdFrontMode front{WspdFrontMode::Pure};
  Q2SiblingMode sibling{Q2SiblingMode::Disabled};
  Q2WitnessOrder order{Q2WitnessOrder::GlobalDfs};
  std::size_t pool{};
};
mhgp8::Q2PoolWork integer_pool(mhgp8::Q2PoolWork work) {
  work.preparation_ms = 0; work.selected_total_ms = 0; return work;
}
bool same_geometry(const mhgp8::WspdQ2ParallelResult& a, const mhgp8::WspdQ2ParallelResult& b) {
  return a.front.work == b.front.work && a.front.total_unordered_pairs == b.front.total_unordered_pairs &&
         a.front.active_lane_mask == b.front.active_lane_mask && a.census_work == b.census_work &&
         a.sibling_work == b.sibling_work && a.order_work == b.order_work && a.joint_work == b.joint_work &&
         integer_pool(a.pool_work) == integer_pool(b.pool_work) && a.input_rectangles == b.input_rectangles &&
         a.anchor_queries == b.anchor_queries && a.candidate_pairs == b.candidate_pairs &&
         a.accepted_pairs == b.accepted_pairs && a.rejected_pairs == b.rejected_pairs;
}
struct Baseline { mhgp8::WspdQ2ParallelResult result; Output output; };
Baseline coarse(Gate& gate, const mhgp8::Q2CensusIndexPtr& index, const GeometryOptions& options, const Output& expected) {
  std::vector<Output> slots(2);
  std::vector<mhgp8::Q2CensusConsumer> consumers;
  for (std::size_t i = 0; i < slots.size(); ++i)
    consumers.emplace_back([&, i](const mhgp8::Q2Support& item) { slots[i].push_back(copy(item)); });
  Baseline result;
  result.result = mhgp8::run_wspd_q2_census_parallel(index, options.k, options.s, options.front,
      mhgp8::Q2CensusMode::SharedBlocks, consumers, 1, options.sibling, options.order,
      mhgp8::Q2AnchorMode::Individual, options.pool, {mhgp8::WspdQ2ScheduleMode::Coarse, 1, 1});
  result.output = join(slots); check_output(gate, result.output, expected); ++gate.coarse_runs;
  return result;
}

auto additive(const mhgp8::Q2CooperativeWork& w) {
  return std::array{w.continued_anchors, w.continued_pairs, w.completed_pairs, w.fragments_started,
                    w.completed_fragments, w.donations, w.donations_after_seeds_exhausted, w.offer_checks,
                    w.offer_busy, w.offer_full, w.offer_no_waiter, w.offer_no_sibling, w.waits, w.wakes};
}
auto resume_additive(const mhgp8::Q2CensusResumeWork& w) {
  return std::array{w.advance_calls, w.transitions, w.entry_steps, w.witness_steps, w.admission_steps,
                    w.payload_steps, w.pauses, w.pauses_after_credit, w.pauses_inside_deferred, w.pauses_during_emission};
}
auto detach_additive(const mhgp8::Q2CensusDetachWork& w) {
  return std::array{w.attempts, w.detached_frames, w.imported_frames, w.transferred_pairs, w.moved_frames};
}
template<std::size_t N> void add(std::array<u64, N>& a, const std::array<u64, N>& b) {
  for (std::size_t i = 0; i < N; ++i) a[i] += b[i];
}
bool close(double a, double b) { return std::abs(a - b) <= 1e-7 * std::max({1.0, std::abs(a), std::abs(b)}); }

void compare(Gate& gate, const mhgp8::WspdQ2CooperativeResult& result, const Baseline& baseline,
             const GeometryOptions& geometry, WspdQ2CooperativeOptions options, const std::vector<Output>& slots) {
  const auto& p = result.pipeline; const auto& w = result.work; const auto& r = w.resume_work; const auto& d = w.detach_work;
  check_output(gate, join(slots), baseline.output);
  gate.require(same_geometry(p, baseline.result), "cooperative routing changed a front bin/mass/logical maximum, census field or Pool counter");
  gate.require(p.requested_workers == slots.size() && p.started_workers == (p.jobs == 0 ? 0 : slots.size()) &&
                   p.workers.size() == p.started_workers && result.workers.size() == p.started_workers &&
                   p.target_jobs == slots.size() * options.jobs_per_worker && p.completed_jobs == p.jobs &&
                   p.terminal_jobs <= p.jobs && p.dispatch_work == mhgp8::WspdFrontDispatchWork{},
               "cooperative workers/seeds metadata or no-front-donation contract failed");
  auto work_sum = additive(w); work_sum.fill(0);
  auto resume_sum = resume_additive(r); resume_sum.fill(0);
  auto detach_sum = detach_additive(d); detach_sum.fill(0);
  u64 max_queue = 0, max_active = 0, max_fragment = 0, max_pending = 0;
  u64 jobs = 0, products = p.prefix_product_visits, rectangles = 0, visits = 0, supports = 0, peak_sum = 0, peak_max = 0;
  double elapsed = 0, payload = 0;
  for (std::size_t i = 0; i < result.workers.size(); ++i) {
    const auto& worker = result.workers[i].work; const auto& pipeline = p.workers[i];
    add(work_sum, additive(worker)); add(resume_sum, resume_additive(worker.resume_work)); add(detach_sum, detach_additive(worker.detach_work));
    max_queue = std::max(max_queue, worker.max_queue_size); max_active = std::max(max_active, worker.max_active_tasks);
    max_fragment = std::max(max_fragment, worker.max_fragment_bytes); max_pending = std::max(max_pending, worker.resume_work.max_pending_tasks);
    jobs += pipeline.jobs; products += pipeline.front_products; rectangles += pipeline.input_rectangles;
    visits += pipeline.count_node_visits; supports += pipeline.supports;
    peak_sum += pipeline.pool_peak_bytes; peak_max = std::max(peak_max, pipeline.pool_peak_bytes);
    elapsed += pipeline.elapsed_ms; payload += pipeline.payload_ms;
    gate.require(pipeline.supports == slots[i].size() && pipeline.dispatch_work == mhgp8::WspdFrontDispatchWork{} &&
                     pipeline.elapsed_ms >= 0 && pipeline.payload_ms >= 0 && pipeline.elapsed_ms <= p.total_ms + 1e-6,
                 "worker used another callback slot, donated front work or misreported its enclosing interval");
  }
  gate.require(work_sum == additive(w) && resume_sum == resume_additive(r) && detach_sum == detach_additive(d) &&
                   max_queue == w.max_queue_size && max_active == w.max_active_tasks && max_fragment == w.max_fragment_bytes &&
                   max_pending == r.max_pending_tasks, "cooperative worker reduction lost a counter or added a maximum");
  gate.require(jobs == p.jobs && products == p.front.work.product_visits && rectangles == p.input_rectangles &&
                   visits == p.census_work.count_node_visits && supports == p.accepted_pairs && peak_sum == p.pool_peak_bytes_sum &&
                   peak_max == p.pool_work.plan_peak_bytes && close(elapsed, p.worker_ms_sum) && close(payload, p.payload_ms_sum),
               "pipeline worker reductions lost geometry, Pool maxima or interval sums");
  gate.require(w.continued_pairs == w.completed_pairs && w.continued_pairs <= p.candidate_pairs &&
                   w.continued_anchors <= p.anchor_queries && w.fragments_started == w.completed_fragments &&
                   w.completed_fragments == w.continued_anchors + w.donations &&
                   w.donations == d.detached_frames && w.donations == d.imported_frames &&
                   w.donations_after_seeds_exhausted <= w.donations &&
                   w.offer_checks == w.offer_busy + w.offer_full + w.offer_no_waiter + d.attempts &&
                   d.attempts == w.donations + w.offer_no_sibling && w.waits == w.wakes &&
                   d.moved_frames >= w.donations && d.transferred_pairs >= w.donations,
               "cooperative root/fragment/offer/import/export/wake accounting is inconsistent");
  gate.require(r.transitions == r.entry_steps + r.witness_steps + r.admission_steps + r.payload_steps &&
                   r.advance_calls == r.pauses + w.completed_fragments && r.entry_steps <= p.census_work.query_tasks &&
                   r.payload_steps <= p.accepted_pairs && r.max_pending_tasks <= mhgp8::index_stack_frames &&
                   r.pauses_after_credit <= r.pauses && r.pauses_inside_deferred <= r.pauses && r.pauses_during_emission <= r.pauses &&
                   w.max_queue_size <= options.queue_capacity && w.max_active_tasks <= p.started_workers,
               "cooperative transition, pause or memory-stack bounds are inconsistent");
  if (p.started_workers != 0)
    gate.require(p.queue_storage_bytes >= options.queue_capacity * sizeof(std::unique_ptr<mhgp8::Q2CensusContinuation>),
                 "cooperative census queue storage was omitted");
  if (w.continued_anchors == 0)
    gate.require(w.continued_pairs == 0 && w.completed_fragments == 0 && r.transitions == 0 &&
                     w.max_fragment_bytes == 0 && r.max_pending_tasks == 0,
                 "small/Pool-only path paid hidden continuation work");
  else gate.require(w.max_fragment_bytes >= 6272 && r.max_pending_tasks > 0, "continued anchor omitted its owned frame capacity");
  if (slots.size() == 1)
    gate.require(w.offer_checks == 0 && w.donations == 0 && d.attempts == 0, "one worker polled or donated to itself");
  if (geometry.pool != 0 && geometry.pool <= options.min_b_size) {
    gate.require(w.continued_anchors == 0, "Pool-selected rectangle including passthrough entered cooperative continuation path");
    ++gate.pool_blocks_continuations;
  }
  if (geometry.pool == 0 && options.min_b_size == 1)
    gate.require(w.continued_pairs == p.candidate_pairs && w.continued_anchors == p.anchor_queries,
                 "all-unfiltered/min1 path omitted an anchor or doubled its candidate mass");
  for (const double value : {p.total_ms, p.partition_ms, p.worker_ms_sum, p.payload_ms_sum,
                            p.pool_work.preparation_ms, p.pool_work.selected_total_ms})
    gate.require(std::isfinite(value) && value >= 0, "cooperative timing is negative or nonfinite");
  gate.continued += w.continued_anchors; gate.donations += w.donations; gate.after_seeds_claimed += w.donations_after_seeds_exhausted;
  gate.credit_pauses += r.pauses_after_credit; gate.phase_pauses += r.pauses_inside_deferred;
  gate.empty_runs += p.jobs == 0; gate.small_only_runs += w.continued_anchors == 0; gate.passthrough += p.pool_work.passthrough_rectangles;
  if (geometry.pool == 64) { gate.selected64 += p.pool_work.selected_rectangles; gate.filtered64 += p.pool_work.filtered_pairs; }
  ++gate.cooperative_runs;
}

mhgp8::WspdQ2CooperativeResult run(Gate& gate, const mhgp8::Q2CensusIndexPtr& index, const Baseline& baseline,
    const GeometryOptions& geometry, std::size_t workers, WspdQ2CooperativeOptions options, bool defaults = false) {
  const auto* nodes = index->spatial_nodes().data(); const auto* order = index->spatial_order().data();
  const auto* coordinates = index->cloud().points().data(); const auto visits = index->work().point_visits;
  std::vector<Output> slots(workers);
  std::vector<mhgp8::Q2CensusConsumer> consumers;
  for (std::size_t i = 0; i < workers; ++i)
    consumers.emplace_back([&, i](const mhgp8::Q2Support& item) { slots[i].push_back(copy(item)); });
  const auto result = defaults
      ? mhgp8::run_wspd_q2_census_cooperative(index, geometry.k, geometry.s, geometry.front, consumers)
      : mhgp8::run_wspd_q2_census_cooperative(index, geometry.k, geometry.s, geometry.front, consumers,
          options, geometry.sibling, geometry.order, geometry.pool);
  compare(gate, result, baseline, geometry, options, slots);
  gate.require(nodes == index->spatial_nodes().data() && order == index->spatial_order().data() &&
                   coordinates == index->cloud().points().data() && visits == index->work().point_visits &&
                   result.pipeline.census_work.query_build_nodes == 0 && result.pipeline.census_work.query_cover_visits == 0,
               "cooperative path changed the immutable input/index or rebuilt a local query factor");
  (gate.wide ? gate.default_runs18 : gate.default_runs) += defaults; return result;
}

Points axis() { return {{0, 0, 0}, {1, 0, 0}, {99, 0, 0}, {100, 0, 0}}; }
using Coordinate = mhgp8::Coordinate;
// The u16 corpus is a pinned recipe (same clouds, same floors); only the cast
// type follows the engine's Coordinate. Its 65535/32767 sites are interior
// points of the 18-bit domain and no longer exercise any bound.
std::vector<Points> fixtures() {
  std::vector<Points> result{{{7, 8, 9}}, {{0, 0, 0}, {65535, 65535, 65535}}, axis()};
  Points shell, cube, rows, random, clusters;
  for (int x = -5; x <= 5; ++x) for (int y = -5; y <= 5; ++y) for (int z = -5; z <= 5; ++z)
    if (x*x + y*y + z*z == 25)
      shell.push_back({static_cast<Coordinate>(8 + x), static_cast<Coordinate>(8 + y), static_cast<Coordinate>(8 + z)});
  for (unsigned bits = 0; bits < 8; ++bits)
    cube.push_back({static_cast<Coordinate>((bits & 1U) ? 65535 : 0), static_cast<Coordinate>((bits & 2U) ? 65535 : 0),
                    static_cast<Coordinate>((bits & 4U) ? 65535 : 0)});
  cube.push_back({32767, 32768, 32767});
  for (unsigned side = 0; side < 2; ++side) for (unsigned i = 0; i < 32; ++i)
    rows.push_back({static_cast<Coordinate>(1000 + side * 59000), static_cast<Coordinate>(i), 0});
  std::uint32_t state = 971;
  for (unsigned i = 0; i < 13; ++i) {
    state = state * 1664525U + 1013904223U; const auto y = static_cast<Coordinate>(state >> 16U);
    state = state * 1664525U + 1013904223U;
    random.push_back({static_cast<Coordinate>(4093 * i), y, static_cast<Coordinate>(state >> 16U)});
  }
  // Unequal factors8x72 exercise a real Pool64 plan with total n80, not a
  // larger hidden oracle or a fabricated spatial node.
  for (unsigned i = 0; i < 8; ++i) clusters.push_back({static_cast<Coordinate>(i), 17, 31});
  for (unsigned i = 0; i < 72; ++i) clusters.push_back({static_cast<Coordinate>(60000 + i), 17, 31});
  result.push_back(shell); result.push_back(cube); result.push_back(rows); result.push_back(random); result.push_back(clusters);
  return result;
}
// 18-bit twins of the extreme fixtures, graved at the engine limit 262143:
// the space diagonal, the eight corners with the near-center site, a random
// cloud whose own recipe draws 18 bits (>> 14U, separate seed) and the 8x72
// clusters whose far factor ends exactly at the limit. They pass the same
// oracle, work-identity and lifetime checks as the u16 corpus.
static_assert(mhgp8::coordinate_limit == 262143, "18-bit fixtures are graved at the engine limit");
std::vector<Points> fixtures18() {
  std::vector<Points> result{{{0, 0, 0}, {262143, 262143, 262143}}};
  Points cube, random, clusters;
  for (unsigned bits = 0; bits < 8; ++bits)
    cube.push_back({static_cast<Coordinate>((bits & 1U) ? 262143 : 0), static_cast<Coordinate>((bits & 2U) ? 262143 : 0),
                    static_cast<Coordinate>((bits & 4U) ? 262143 : 0)});
  cube.push_back({131071, 131072, 131071});
  std::uint32_t state = 1811;
  for (unsigned i = 0; i < 13; ++i) {
    state = state * 1664525U + 1013904223U; const auto y = static_cast<Coordinate>(state >> 14U);
    state = state * 1664525U + 1013904223U;
    random.push_back({static_cast<Coordinate>(16381 * i), y, static_cast<Coordinate>(state >> 14U)});
  }
  for (unsigned i = 0; i < 8; ++i) clusters.push_back({static_cast<Coordinate>(i), 17, 31});
  for (unsigned i = 0; i < 72; ++i) clusters.push_back({static_cast<Coordinate>(262072 + i), 17, 31});
  result.push_back(cube); result.push_back(random); result.push_back(clusters);
  return result;
}
void corpus(Gate& gate, const std::vector<Points>& clouds, bool wide) {
  gate.wide = wide;
  for (std::size_t c = 0; c < clouds.size(); ++c) {
    const auto& points = clouds[c]; const auto all = oracle(gate, points);
    const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
    unsigned variant = 0;
    for (const unsigned k : {1U, 5U, 10U}) for (const unsigned s : {8U, 10U, 12U}) {
      const auto choice = variant++;
      GeometryOptions geometry{k, s, choice % 2 == 0 ? WspdFrontMode::Pure : WspdFrontMode::MidpointSamples,
          choice % 3 == 0 ? Q2SiblingMode::Disabled : Q2SiblingMode::Saturating,
          choice % 2 == 0 ? Q2WitnessOrder::GlobalDfs : Q2WitnessOrder::ComplementFirst,
          std::array<std::size_t, 3>{0, 2, 64}[(choice + c) % 3]};
      const auto baseline = coarse(gate, index, geometry, accepted(all, k));
      static_cast<void>(run(gate, index, baseline, geometry, 1, {1, 1, 1, 1}));
      static_cast<void>(run(gate, index, baseline, geometry, 2, {1, 1, 1, 1}));
      static_cast<void>(run(gate, index, baseline, geometry, 4, {4, 8, 256, 64}));
      if (choice == 0) {
        GeometryOptions defaults{k, s};
        const auto default_base = coarse(gate, index, defaults, accepted(all, k));
        static_cast<void>(run(gate, index, default_base, defaults, 2, {}, true));
      }
    }
    // Dedicated fine-quantum all-unfiltered run provides ample real internal
    // work for idle workers; counters remain observations, not stable hashes.
    GeometryOptions fine{10, 8, WspdFrontMode::Pure, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, 0};
    const auto fine_base = coarse(gate, index, fine, accepted(all, 10));
    static_cast<void>(run(gate, index, fine_base, fine, 4, {1, 1, 1, 1}));
    ++(wide ? gate.clouds18 : gate.clouds);
  }
  gate.wide = false;
}

struct CallbackFailure {};
void callbacks(Gate& gate) {
  const auto points = axis(); const auto all = oracle(gate, points);
  auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
  const WspdQ2CooperativeOptions options{1, 1, 1, 1};
  GeometryOptions geometry;
  const auto baseline = coarse(gate, index, geometry, accepted(all, 10));
  std::vector<Output> slots(4); std::array<bool, 4> first{};
  std::array<std::thread::id, 4> thread_ids;
  std::atomic<unsigned> arrivals{0}; std::latch two_callbacks{2};
  std::vector<mhgp8::Q2CensusConsumer> consumers;
  for (std::size_t i = 0; i < slots.size(); ++i)
    consumers.emplace_back([&, i](const mhgp8::Q2Support& item) {
      slots[i].push_back(copy(item));
      if (!first[i]) {
        first[i] = true; thread_ids[i] = std::this_thread::get_id();
        if (arrivals.fetch_add(1) < 2) { two_callbacks.count_down(); two_callbacks.wait(); }
      }
    });
  const auto paired = mhgp8::run_wspd_q2_census_cooperative(index, 10, 8, WspdFrontMode::Pure, consumers, options);
  compare(gate, paired, baseline, geometry, options, slots);
  std::set<std::thread::id> distinct;
  for (std::size_t i = 0; i < first.size(); ++i) if (first[i]) distinct.insert(thread_ids[i]);
  gate.require(distinct.size() >= 2 && !distinct.contains(std::this_thread::get_id()),
               "productive callbacks did not execute on different real workers");
  gate.multiple_callback_threads += distinct.size();

  // A TLS marker decrements only when each callback-producing worker exits,
  // not merely when its callback returns. No sleeps or concurrent Gate writes.
  struct ThreadExit {
    std::atomic<unsigned>* count;
    explicit ThreadExit(std::atomic<unsigned>& value) : count(&value) { count->fetch_add(1); }
    ~ThreadExit() { count->fetch_sub(1); }
  };
  std::atomic<unsigned> live_threads{0}, throwing_arrivals{0}, failures{0};
  std::array<bool, 4> seen{}; std::latch two_throwing{2};
  std::vector<mhgp8::Q2CensusConsumer> throwing;
  for (std::size_t i = 0; i < 4; ++i)
    throwing.emplace_back([&, i](const mhgp8::Q2Support&) {
      thread_local ThreadExit exit(live_threads);
      if (!seen[i]) {
        seen[i] = true;
        if (throwing_arrivals.fetch_add(1) < 2) { two_throwing.count_down(); two_throwing.wait(); }
      }
      if (failures.fetch_add(1) == 0) throw CallbackFailure{};
    });
  bool caught = false;
  try { static_cast<void>(mhgp8::run_wspd_q2_census_cooperative(index, 10, 8, WspdFrontMode::Pure, throwing, options)); }
  catch (const CallbackFailure&) { caught = true; }
  gate.require(caught && throwing_arrivals.load() >= 2 && live_threads.load() == 0,
               "callback failure did not join all observed callback-producing threads");
  ++gate.callback_failures; gate.callback_threads_joined += throwing_arrivals.load();

  geometry.k = 1; geometry.pool = 2;
  const auto pool_base = coarse(gate, index, geometry, accepted(all, 1));
  gate.require(pool_base.result.pool_work.filtered_pairs == 3 && pool_base.result.pool_work.pair_roots == 1,
               "effective-Pool callback fixture changed");
  const std::weak_ptr<const mhgp8::Q2CensusIndex> weak = index;
  std::vector<Output> outer_slots(4), inner_slots(2);
  std::atomic<unsigned> nested{0};
  consumers.clear();
  for (std::size_t i = 0; i < outer_slots.size(); ++i)
    consumers.emplace_back([&, i](const mhgp8::Q2Support& item) {
      const auto before = copy(item);
      if (before.a == 1 && before.b == 2) {
        if (nested.fetch_add(1) != 0) throw std::runtime_error("duplicate nested Pool support");
        index.reset();
        const auto owner = weak.lock();
        if (!owner) throw std::runtime_error("cooperative callback lost its shared input owner");
        std::vector<mhgp8::Q2CensusConsumer> inner;
        for (std::size_t j = 0; j < inner_slots.size(); ++j)
          inner.emplace_back([&, j](const mhgp8::Q2Support& value) { inner_slots[j].push_back(copy(value)); });
        static_cast<void>(mhgp8::run_wspd_q2_census_cooperative(owner, 5, 10, WspdFrontMode::MidpointSamples,
            inner, options, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst));
        if (!(copy(item) == before)) throw std::runtime_error("nested team invalidated the outer borrowed payload");
      }
      outer_slots[i].push_back(copy(item));
    });
  const auto nested_result = mhgp8::run_wspd_q2_census_cooperative(index, 1, 8, WspdFrontMode::Pure,
      consumers, options, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, 2);
  compare(gate, nested_result, pool_base, geometry, options, outer_slots);
  check_output(gate, join(inner_slots), accepted(all, 5));
  gate.require(nested.load() == 1 && !index && weak.expired(), "nested callback/reset was skipped or leaked the shared index owner");
  ++gate.reentrant_runs; ++gate.index_resets;
}

void invalids(Gate& gate) {
  const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(axis()));
  std::atomic<unsigned> emissions{0};
  const std::vector<mhgp8::Q2CensusConsumer> consumers(2, [&](const mhgp8::Q2Support&) { emissions.fetch_add(1); });
  const auto invoke = [&](mhgp8::Q2CensusIndexPtr owner, unsigned k, unsigned s, WspdFrontMode front,
      const std::vector<mhgp8::Q2CensusConsumer>& slots, WspdQ2CooperativeOptions options = {},
      Q2SiblingMode sibling = Q2SiblingMode::Disabled, Q2WitnessOrder order = Q2WitnessOrder::GlobalDfs) {
    return mhgp8::run_wspd_q2_census_cooperative(std::move(owner), k, s, front, slots, options, sibling, order);
  };
  gate.rejects([&] { static_cast<void>(invoke({}, 1, 8, WspdFrontMode::Pure, consumers)); }, "null cooperative index accepted");
  for (unsigned k : {0U, 11U}) gate.rejects([&] { static_cast<void>(invoke(index, k, 8, WspdFrontMode::Pure, consumers)); }, "invalid K accepted");
  gate.rejects([&] { static_cast<void>(invoke(index, 1, 0, WspdFrontMode::Pure, consumers)); }, "zero separation accepted");
  gate.rejects([&] { static_cast<void>(invoke(index, 1, 8, static_cast<WspdFrontMode>(99), consumers)); }, "invalid front mode accepted");
  gate.rejects([&] { static_cast<void>(invoke(index, 1, 8, WspdFrontMode::Pure, {})); }, "empty worker set accepted");
  auto empty = consumers; empty.back() = {};
  gate.rejects([&] { static_cast<void>(invoke(index, 1, 8, WspdFrontMode::Pure, empty)); }, "empty later callback accepted");
  for (unsigned bad = 0; bad < 4; ++bad) {
    WspdQ2CooperativeOptions options;
    if (bad == 0) options.jobs_per_worker = 0;
    if (bad == 1) options.queue_capacity = 0;
    if (bad == 2) options.quantum = 0;
    if (bad == 3) options.min_b_size = 0;
    gate.rejects([&] { static_cast<void>(invoke(index, 1, 8, WspdFrontMode::Pure, consumers, options)); }, "zero cooperative option accepted");
  }
  gate.rejects([&] { static_cast<void>(invoke(index, 1, 8, WspdFrontMode::Pure, consumers, {}, static_cast<Q2SiblingMode>(99))); }, "invalid sibling mode accepted");
  gate.rejects([&] { static_cast<void>(invoke(index, 1, 8, WspdFrontMode::Pure, consumers, {}, Q2SiblingMode::Disabled, static_cast<Q2WitnessOrder>(99))); }, "invalid witness order accepted");
  gate.rejects<std::overflow_error>([&] { static_cast<void>(invoke(index, 1, 8, WspdFrontMode::Pure, consumers,
      {std::numeric_limits<std::size_t>::max(), 1, 1, 1})); }, "job target multiplication overflow accepted");
  gate.require(emissions.load() == 0, "invalid cooperative options emitted before validation completed");

  const auto all = oracle(gate, axis()); GeometryOptions geometry;
  const auto baseline = coarse(gate, index, geometry, accepted(all, 10));
  const auto result = run(gate, index, baseline, geometry, 2, {1, 1, 1, std::numeric_limits<std::size_t>::max()});
  gate.require(result.work.continued_anchors == 0, "large method-selection threshold was treated as a search/output quota");
  auto changed = result.pipeline; ++changed.front.work.max_stack_size;
  gate.require(!same_geometry(changed, baseline.result), "logical front-stack mutant survived"); ++gate.mutants;
  changed = result.pipeline; ++changed.census_work.count_node_visits;
  gate.require(!same_geometry(changed, baseline.result), "replayed census visit mutant survived"); ++gate.mutants;
  changed = result.pipeline; ++changed.order_work.structural_splits;
  gate.require(!same_geometry(changed, baseline.result), "replayed structural work mutant survived"); ++gate.mutants;
  changed = result.pipeline; ++changed.pool_work.factor_sites;
  gate.require(!same_geometry(changed, baseline.result), "duplicated Pool factor preparation mutant survived"); ++gate.mutants;
  auto lost = baseline.output; lost.front().shell.pop_back();
  gate.require(lost != baseline.output, "truncated global shell mutant survived"); ++gate.mutants;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_wspd_q2_cooperative_gate --selftest\n"; return 2;
  }
  try {
    Gate gate;
    corpus(gate, fixtures(), false); corpus(gate, fixtures18(), true); callbacks(gate); invalids(gate);
    gate.require(gate.clouds == 8 && gate.cooperative_runs >= 230 && gate.coarse_runs >= 80 && gate.default_runs == 8 &&
                     gate.continued > 0 && gate.credit_pauses > 0 && gate.phase_pauses > 0 && gate.max_shell == 30 &&
                     gate.empty_runs > 0 && gate.small_only_runs > 0 && gate.pool_blocks_continuations > 0 &&
                     gate.selected64 > 0 && gate.filtered64 > 0 && gate.passthrough > 0 &&
                     gate.multiple_callback_threads >= 2 && gate.callback_threads_joined >= 2 && gate.callback_failures == 1 &&
                     gate.reentrant_runs == 1 && gate.index_resets == 1 && gate.mutants == 5,
                 "cooperative gate lost a required positive geometry/routing/lifetime fixture");
    // Separate floors of the 18-bit twin corpus; max_shell18 is the full
    // shell of the 262143-cube diagonal as reported by the scalar oracle.
    gate.require(gate.clouds18 == 4 && gate.default_runs18 == 4 && gate.supports18 > 0 && gate.max_shell18 == 8 &&
                     !gate.wide,
                 "cooperative gate lost an 18-bit twin fixture floor");
    // Donation statistics are observed, not exact scheduling signatures.
    // The after-seeds counter means all seeds CLAIMED, not all completed.
    std::cout << "{\"schema\":\"mhgp8_wspd_q2_cooperative_gate_v1\",\"status\":\"passed\",\"public_status\":\"not_claimed\""
              << ",\"checks\":" << gate.checks << ",\"clouds\":" << gate.clouds << ",\"oracle_pairs\":" << gate.oracle_pairs
              << ",\"oracle_sites\":" << gate.oracle_sites << ",\"coarse_runs\":" << gate.coarse_runs
              << ",\"cooperative_runs\":" << gate.cooperative_runs << ",\"default_runs\":" << gate.default_runs
              << ",\"supports\":" << gate.supports << ",\"max_shell\":" << gate.max_shell << ",\"continued_anchors\":" << gate.continued
              << ",\"donations\":" << gate.donations << ",\"donations_after_seeds_claimed\":" << gate.after_seeds_claimed
              << ",\"credit_pauses\":" << gate.credit_pauses << ",\"phase_pauses\":" << gate.phase_pauses
              << ",\"empty_runs\":" << gate.empty_runs << ",\"small_only_runs\":" << gate.small_only_runs
              << ",\"pool_blocks_continuations\":" << gate.pool_blocks_continuations << ",\"selected64\":" << gate.selected64
              << ",\"filtered64\":" << gate.filtered64 << ",\"passthrough\":" << gate.passthrough
              << ",\"multiple_callback_threads\":" << gate.multiple_callback_threads << ",\"callback_threads_joined\":" << gate.callback_threads_joined
              << ",\"callback_failures\":" << gate.callback_failures << ",\"reentrant_runs\":" << gate.reentrant_runs
              << ",\"index_resets\":" << gate.index_resets << ",\"invalid_inputs\":" << gate.invalid_inputs << ",\"mutants\":" << gate.mutants
              << ",\"clouds18\":" << gate.clouds18 << ",\"default_runs18\":" << gate.default_runs18
              << ",\"supports18\":" << gate.supports18 << ",\"max_shell18\":" << gate.max_shell18 << "}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp8 cooperative gate: " << error.what() << '\n'; return 1;
  }
}
