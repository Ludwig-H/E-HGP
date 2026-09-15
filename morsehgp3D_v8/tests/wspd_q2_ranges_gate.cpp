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

#include "pipeline/wspd_q2_ranges.hpp"

namespace {
using mhgp8::Point3;
using mhgp8::Q2SiblingMode;
using mhgp8::Q2WitnessOrder;
using mhgp8::WspdFrontMode;
using mhgp8::WspdQ2RangeOptions;
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
  u64 checks{}, clouds{}, oracle_pairs{}, oracle_sites{}, coarse_runs{}, range_runs{}, default_runs{};
  u64 supports{}, max_shell{}, donations{}, after_seeds_claimed{}, empty_runs{}, surplus_workers{};
  u64 selected64{}, filtered64{}, passthrough{}, pool_donations{}, passthrough_donations{};
  u64 callback_failures{}, reentrant_runs{}, index_resets{}, multiple_callback_threads{}, callback_threads_joined{};
  u64 wide_pool_cases{};
  u64 invalid_inputs{}, mutants{};
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
  normalize(result);
  return result;
}

// Explicit reuse of the independent scalar-oracle methodology of the
// cooperative gate, not of its answers or qualification. Neither the front
// nor product box/census predicates enter this exhaustive bounded judge.
// All promoted dot products fit int64 over the quantized u16 domain.
Output oracle(Gate& gate, const Points& points) {
  gate.require(!points.empty() && points.size() <= 100, "range oracle exceeded its bounded n<=100 domain");
  Output result;
  for (std::size_t a = 0; a < points.size(); ++a) for (std::size_t b = a + 1; b < points.size(); ++b) {
    Support item;
    item.a = a;
    item.b = b;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      item.key.center_twice[axis] = std::uint32_t{points[a][axis]} + points[b][axis];
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
    result.push_back(std::move(item));
    ++gate.oracle_pairs;
  }
  return result;
}
Output accepted(const Output& all, unsigned k) {
  Output result;
  for (const auto& item : all) if (item.interior.size() < k) result.push_back(item);
  return result;
}
void check_output(Gate& gate, const Output& output, const Output& expected) {
  gate.require(output == expected, "range support/key/interior/full-shell multiset differs from brute-force oracle");
  for (const auto& item : output) {
    gate.require(std::adjacent_find(item.interior.begin(), item.interior.end()) == item.interior.end() &&
                     std::adjacent_find(item.shell.begin(), item.shell.end()) == item.shell.end() &&
                     std::binary_search(item.shell.begin(), item.shell.end(), item.a) &&
                     std::binary_search(item.shell.begin(), item.shell.end(), item.b),
                 "range payload duplicated an ID or omitted an endpoint from the full shell");
    gate.max_shell = std::max(gate.max_shell, static_cast<u64>(item.shell.size()));
  }
  gate.supports += output.size();
}

struct GeometryOptions {
  unsigned k{10}, s{8};
  WspdFrontMode front{WspdFrontMode::Pure};
  Q2SiblingMode sibling{Q2SiblingMode::Disabled};
  Q2WitnessOrder order{Q2WitnessOrder::GlobalDfs};
  std::size_t pool{};
};
mhgp8::Q2PoolWork integer_pool(mhgp8::Q2PoolWork work) {
  work.preparation_ms = 0;
  work.selected_total_ms = 0;
  return work;
}
bool same_geometry(const mhgp8::WspdQ2ParallelResult& a, const mhgp8::WspdQ2ParallelResult& b) {
  return a.front.work == b.front.work && a.front.total_unordered_pairs == b.front.total_unordered_pairs &&
         a.front.active_lane_mask == b.front.active_lane_mask && a.census_work == b.census_work &&
         a.sibling_work == b.sibling_work && a.order_work == b.order_work && a.joint_work == b.joint_work &&
         integer_pool(a.pool_work) == integer_pool(b.pool_work) && a.input_rectangles == b.input_rectangles &&
         a.anchor_queries == b.anchor_queries && a.candidate_pairs == b.candidate_pairs &&
         a.accepted_pairs == b.accepted_pairs && a.rejected_pairs == b.rejected_pairs;
}
struct Baseline {
  mhgp8::WspdQ2ParallelResult result;
  Output output;
};
Baseline coarse(Gate& gate, const mhgp8::Q2CensusIndexPtr& index, const GeometryOptions& options, const Output& expected) {
  std::vector<Output> slots(2);
  std::vector<mhgp8::Q2CensusConsumer> consumers;
  for (std::size_t i = 0; i < slots.size(); ++i)
    consumers.emplace_back([&, i](const mhgp8::Q2Support& item) { slots[i].push_back(copy(item)); });
  Baseline result;
  result.result = mhgp8::run_wspd_q2_census_parallel(index, options.k, options.s, options.front,
      mhgp8::Q2CensusMode::SharedBlocks, consumers, 1, options.sibling, options.order,
      mhgp8::Q2AnchorMode::Individual, options.pool, {mhgp8::WspdQ2ScheduleMode::Coarse, 1, 1});
  result.output = join(slots);
  check_output(gate, result.output, expected);
  ++gate.coarse_runs;
  return result;
}

template<std::size_t N> void add(std::array<u64, N>& a, const std::array<u64, N>& b) {
  for (std::size_t i = 0; i < N; ++i) a[i] += b[i];
}
bool close(double a, double b) {
  return std::abs(a - b) <= 1e-7 * std::max({1.0, std::abs(a), std::abs(b)});
}

auto additive(const mhgp8::Q2RangeWork& w) {
  return std::array{w.initial_ranges, w.completed_ranges, w.received_ranges,
                    w.initial_anchors, w.completed_anchors, w.initial_pairs, w.completed_pairs,
                    w.initial_shared_ranges, w.initial_pool_ranges, w.initial_passthrough_ranges,
                    w.donations, w.donated_anchors, w.donated_pairs, w.donations_after_seeds_exhausted,
                    w.shared_donations, w.pool_donations, w.passthrough_donations,
                    w.offer_checks, w.offer_busy, w.offer_full, w.offer_no_waiter, w.waits, w.wakes};
}

void check_accounting(Gate& gate, const mhgp8::WspdQ2RangeResult& result, WspdQ2RangeOptions options) {
  const auto& p = result.pipeline;
  const auto& w = result.work;
  gate.require(p.started_workers == (p.jobs == 0 ? 0 : p.requested_workers) &&
                   p.workers.size() == p.started_workers && result.workers.size() == p.started_workers &&
                   p.target_jobs == p.requested_workers * options.jobs_per_worker && p.completed_jobs == p.jobs &&
                   p.terminal_jobs <= p.jobs && p.dispatch_work == mhgp8::WspdFrontDispatchWork{},
               "range workers/seeds metadata or no-front-donation contract failed");
  auto work_sum = additive(w);
  work_sum.fill(0);
  u64 max_queue = 0, max_active = 0;
  u64 jobs = 0, products = p.prefix_product_visits, rectangles = 0, visits = 0, supports = 0;
  u64 peak_sum = 0, peak_max = 0;
  double elapsed = 0, payload = 0;
  for (std::size_t i = 0; i < result.workers.size(); ++i) {
    const auto& worker = result.workers[i].work;
    const auto& pipeline = p.workers[i];
    add(work_sum, additive(worker));
    max_queue = std::max(max_queue, worker.max_queue_size);
    max_active = std::max(max_active, worker.max_active_tasks);
    jobs += pipeline.jobs;
    products += pipeline.front_products;
    rectangles += pipeline.input_rectangles;
    visits += pipeline.count_node_visits;
    supports += pipeline.supports;
    peak_sum += pipeline.pool_peak_bytes;
    peak_max = std::max(peak_max, pipeline.pool_peak_bytes);
    elapsed += pipeline.elapsed_ms;
    payload += pipeline.payload_ms;
    gate.require(pipeline.dispatch_work == mhgp8::WspdFrontDispatchWork{} &&
                     std::isfinite(pipeline.elapsed_ms) && std::isfinite(pipeline.payload_ms) &&
                     pipeline.elapsed_ms >= 0 && pipeline.payload_ms >= 0 && pipeline.elapsed_ms <= p.total_ms + 1e-6,
                 "range worker donated front work or misreported its enclosing interval");
    gate.require(worker.completed_ranges == worker.initial_ranges + worker.received_ranges &&
                     worker.donations == worker.shared_donations + worker.pool_donations + worker.passthrough_donations &&
                     worker.donations_after_seeds_exhausted <= worker.donations &&
                     worker.offer_checks == worker.offer_busy + worker.offer_full + worker.offer_no_waiter + worker.donations &&
                     worker.waits == worker.wakes,
                 "per-worker range obligations/offers/wakes are inconsistent");
  }
  gate.require(work_sum == additive(w) && max_queue == w.max_queue_size && max_active == w.max_active_tasks,
               "range worker reduction lost an additive counter or added a maximum");
  gate.require(jobs == p.jobs && products == p.front.work.product_visits && rectangles == p.input_rectangles &&
                   visits == p.census_work.count_node_visits && supports == p.accepted_pairs && peak_sum == p.pool_peak_bytes_sum &&
                   peak_max == p.pool_work.plan_peak_bytes && close(elapsed, p.worker_ms_sum) && close(payload, p.payload_ms_sum),
               "range pipeline worker reductions lost geometry, creator Pool maxima or interval sums");
  gate.require(w.initial_ranges == w.initial_shared_ranges + w.initial_pool_ranges + w.initial_passthrough_ranges &&
                   w.completed_ranges == w.initial_ranges + w.donations && w.received_ranges == w.donations &&
                   w.donations == w.shared_donations + w.pool_donations + w.passthrough_donations &&
                   w.initial_anchors == w.completed_anchors && w.initial_pairs == w.completed_pairs &&
                   w.initial_pairs == p.candidate_pairs && w.donations_after_seeds_exhausted <= w.donations &&
                   w.offer_checks == w.offer_busy + w.offer_full + w.offer_no_waiter + w.donations && w.waits == w.wakes &&
                   w.donated_anchors >= w.donations && w.donated_pairs >= w.donated_anchors,
               "range initial/completed/transferred mass accounting is inconsistent");
  gate.require(w.max_queue_size <= options.queue_capacity && w.max_active_tasks <= p.started_workers &&
                   result.max_live_pool_parents <= options.queue_capacity + p.started_workers &&
                   ((w.donations == 0) == (w.max_queue_size == 0)),
               "range queue/activity/distinct-Pool-parent bounds are inconsistent");
  gate.require(result.range_task_bytes > 0 && result.range_task_bytes < 6272,
               "anchor range is absent or stores a full49-frame census continuation");
  if (p.started_workers != 0)
    gate.require(p.queue_storage_bytes >= options.queue_capacity * result.range_task_bytes,
                 "preallocated value queue storage was omitted");
  if (p.pool_work.selected_rectangles == 0)
    gate.require(result.max_live_pool_parents == 0 && result.max_live_pool_bytes == 0 &&
                     w.initial_pool_ranges == 0 && w.initial_passthrough_ranges == 0,
                 "Pool-disabled execution created a hidden Pool parent or range");
  else
    gate.require(result.max_live_pool_parents > 0 && result.max_live_pool_bytes >= p.pool_work.plan_peak_bytes,
                 "selected Pool parent omitted its unique live storage");
  if (p.requested_workers == 1)
    gate.require(w.offer_checks == 0 && w.donations == 0 && w.waits == 0,
                 "one worker polled, donated to itself or waited");
  for (const double value : {p.total_ms, p.partition_ms, p.worker_ms_sum, p.payload_ms_sum,
                            p.pool_work.preparation_ms, p.pool_work.selected_total_ms})
    gate.require(std::isfinite(value) && value >= 0, "range timing is negative or nonfinite");
}

void compare(Gate& gate, const mhgp8::WspdQ2RangeResult& result, const Baseline& baseline,
             const GeometryOptions& geometry, WspdQ2RangeOptions options, const std::vector<Output>& slots) {
  check_output(gate, join(slots), baseline.output);
  gate.require(same_geometry(result.pipeline, baseline.result),
               "range routing changed a front bin/mass/logical maximum, census field or Pool counter");
  check_accounting(gate, result, options);
  const auto& p = result.pipeline;
  const auto& w = result.work;
  gate.require(p.requested_workers == slots.size(), "range engine changed the number of callback slots");
  for (std::size_t i = 0; i < p.workers.size(); ++i)
    gate.require(p.workers[i].supports == slots[i].size(), "range worker used another callback slot");
  gate.donations += w.donations;
  gate.pool_donations += w.pool_donations;
  gate.passthrough_donations += w.passthrough_donations;
  gate.after_seeds_claimed += w.donations_after_seeds_exhausted;
  gate.empty_runs += p.jobs == 0;
  gate.surplus_workers += p.jobs > 0 && p.started_workers > p.jobs;
  gate.passthrough += p.pool_work.passthrough_rectangles;
  if (geometry.pool == 64) {
    gate.selected64 += p.pool_work.selected_rectangles;
    gate.filtered64 += p.pool_work.filtered_pairs;
  }
  ++gate.range_runs;
}

mhgp8::WspdQ2RangeResult run(Gate& gate, const mhgp8::Q2CensusIndexPtr& index, const Baseline& baseline,
    const GeometryOptions& geometry, std::size_t workers, WspdQ2RangeOptions options, bool defaults = false) {
  const auto* nodes = index->spatial_nodes().data();
  const auto* order = index->spatial_order().data();
  const auto* coordinates = index->cloud().points().data();
  const auto visits = index->work().point_visits;
  std::vector<Output> slots(workers);
  std::vector<mhgp8::Q2CensusConsumer> consumers;
  for (std::size_t i = 0; i < workers; ++i)
    consumers.emplace_back([&, i](const mhgp8::Q2Support& item) { slots[i].push_back(copy(item)); });
  const auto result = defaults
      ? mhgp8::run_wspd_q2_census_ranges(index, geometry.k, geometry.s, geometry.front, consumers)
      : mhgp8::run_wspd_q2_census_ranges(index, geometry.k, geometry.s, geometry.front, consumers,
          options, geometry.sibling, geometry.order, geometry.pool);
  compare(gate, result, baseline, geometry, options, slots);
  gate.require(nodes == index->spatial_nodes().data() && order == index->spatial_order().data() &&
                   coordinates == index->cloud().points().data() && visits == index->work().point_visits &&
                   result.pipeline.census_work.query_build_nodes == 0 && result.pipeline.census_work.query_cover_visits == 0,
               "range path changed the immutable input/index or rebuilt a local query factor");
  gate.default_runs += defaults;
  return result;
}

Points axis() { return {{0, 0, 0}, {1, 0, 0}, {99, 0, 0}, {100, 0, 0}}; }
std::vector<Points> fixtures() {
  std::vector<Points> result{{{7, 8, 9}}, {{0, 0, 0}, {65535, 65535, 65535}}, axis()};
  Points shell, cube, rows, random, clusters, planes;
  for (int x = -5; x <= 5; ++x) for (int y = -5; y <= 5; ++y) for (int z = -5; z <= 5; ++z)
    if (x*x + y*y + z*z == 25)
      shell.push_back({static_cast<std::uint16_t>(8 + x), static_cast<std::uint16_t>(8 + y), static_cast<std::uint16_t>(8 + z)});
  for (unsigned bits = 0; bits < 8; ++bits)
    cube.push_back({static_cast<std::uint16_t>((bits & 1U) ? 65535 : 0), static_cast<std::uint16_t>((bits & 2U) ? 65535 : 0),
                    static_cast<std::uint16_t>((bits & 4U) ? 65535 : 0)});
  cube.push_back({32767, 32768, 32767});
  for (unsigned side = 0; side < 2; ++side) for (unsigned i = 0; i < 32; ++i)
    rows.push_back({static_cast<std::uint16_t>(1000 + side * 59000), static_cast<std::uint16_t>(i), 0});
  std::uint32_t state = 971;
  for (unsigned i = 0; i < 13; ++i) {
    state = state * 1664525U + 1013904223U;
    const auto y = static_cast<std::uint16_t>(state >> 16U);
    state = state * 1664525U + 1013904223U;
    random.push_back({static_cast<std::uint16_t>(4093 * i), y, static_cast<std::uint16_t>(state >> 16U)});
  }
  // Unequal factors8x72 exercise a real Pool64 plan while the independent
  // oracle remains within n<=100. Spatial ranks are not original IDs.
  for (unsigned i = 0; i < 8; ++i) clusters.push_back({static_cast<std::uint16_t>(i), 17, 31});
  for (unsigned i = 0; i < 72; ++i) clusters.push_back({static_cast<std::uint16_t>(60000 + i), 17, 31});
  result.push_back(shell);
  result.push_back(cube);
  result.push_back(rows);
  result.push_back(random);
  result.push_back(clusters);
  // Different original IDs for the same geometry force the Pool projection
  // ties and all shell IDs to be evaluated against a fresh independent oracle.
  std::reverse(clusters.begin(), clusters.end());
  std::rotate(clusters.begin(), clusters.begin() + 11, clusters.end());
  result.push_back(clusters);
  // Two thin two-layer clouds: both facing layers retain16 credit-zero
  // anchors while the rear layers receive local credits. A filtered Pool
  // parent therefore has a wide surviving range, unlike a collinear cloud
  // whose surviving local credit classes may each contain just one anchor.
  for (const unsigned x : {100U, 200U, 60000U, 60100U})
    for (unsigned y = 0; y < 16; ++y)
      planes.push_back({static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y), 0});
  result.push_back(planes);
  return result;
}

void corpus(Gate& gate) {
  const auto clouds = fixtures();
  for (std::size_t c = 0; c < clouds.size(); ++c) {
    const auto all = oracle(gate, clouds[c]);
    const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(clouds[c]));
    unsigned variant = 0;
    for (const unsigned k : {1U, 5U, 10U}) for (const unsigned s : {8U, 10U, 12U}) {
      const auto choice = variant++;
      const GeometryOptions geometry{k, s,
          choice % 2 == 0 ? WspdFrontMode::Pure : WspdFrontMode::MidpointSamples,
          choice % 3 == 0 ? Q2SiblingMode::Disabled : Q2SiblingMode::Saturating,
          choice % 2 == 0 ? Q2WitnessOrder::GlobalDfs : Q2WitnessOrder::ComplementFirst,
          std::array<std::size_t, 3>{0, 2, 64}[(choice + c) % 3]};
      const auto baseline = coarse(gate, index, geometry, accepted(all, k));
      static_cast<void>(run(gate, index, baseline, geometry, 1, {1, 1, 1}));
      static_cast<void>(run(gate, index, baseline, geometry, 2, {1, 1, 1}));
      static_cast<void>(run(gate, index, baseline, geometry, 4, {4, 8, 8}));
      static_cast<void>(run(gate, index, baseline, geometry, 8, {1, 1, 64}));
      if (choice == 0) {
        const GeometryOptions defaults{k, s};
        const auto default_base = coarse(gate, index, defaults, accepted(all, k));
        static_cast<void>(run(gate, index, default_base, defaults, 2, {}, true));
      }
    }
    // Pool selection, including its no-filter fallback, must remain exactly
    // the same when small unstarted anchor ranges can be transferred. There
    // is no scheduling-dependent minimum donation count in the test.
    for (const auto pool : {std::size_t{0}, std::size_t{2}, std::size_t{64}}) {
      const GeometryOptions fine{10, 8, WspdFrontMode::Pure, Q2SiblingMode::Saturating,
                                 Q2WitnessOrder::ComplementFirst, pool};
      const auto baseline = coarse(gate, index, fine, accepted(all, 10));
      static_cast<void>(run(gate, index, baseline, fine, 4, {1, 1, 1}));
    }
    if (c + 1 == clouds.size()) {
      for (unsigned k : {1U, 2U}) {
        const GeometryOptions wide{k, 8, WspdFrontMode::Pure, Q2SiblingMode::Disabled,
                                   Q2WitnessOrder::GlobalDfs, 2};
        const auto baseline = coarse(gate, index, wide, accepted(all, k));
        gate.require(baseline.result.pool_work.filtered_pairs >= 3 * 16 * 16 &&
                         baseline.result.pool_work.selected_anchors >= 16 && baseline.result.pool_work.pair_roots >= 16 * 16,
                     "four-layer fixture lost its wide, genuinely filtered Pool residue");
        for (unsigned repetition = 0; repetition < 3; ++repetition) {
          static_cast<void>(run(gate, index, baseline, wide, 8, {1, 8, 1}));
          static_cast<void>(run(gate, index, baseline, wide, 32, {1, 8, 1}));
          gate.wide_pool_cases += 2;
        }
      }
    }
    ++gate.clouds;
  }
}

struct CallbackFailure {};
void callbacks(Gate& gate) {
  // Explicit adaptation of cooperative-gate lifetime tests. The two seed
  // subtrees on this axis fixture can emit independently: this latch proves
  // distinct productive callback threads, never a successful range donation.
  const auto points = axis();
  const auto all = oracle(gate, points);
  auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
  const WspdQ2RangeOptions options{1, 1, 1};
  GeometryOptions geometry;
  const auto baseline = coarse(gate, index, geometry, accepted(all, 10));
  std::vector<Output> slots(4);
  std::array<bool, 4> first{};
  std::array<std::thread::id, 4> thread_ids;
  std::atomic<unsigned> arrivals{0};
  std::latch two_callbacks{2};
  std::vector<mhgp8::Q2CensusConsumer> consumers;
  for (std::size_t i = 0; i < slots.size(); ++i)
    consumers.emplace_back([&, i](const mhgp8::Q2Support& item) {
      slots[i].push_back(copy(item));
      if (!first[i]) {
        first[i] = true;
        thread_ids[i] = std::this_thread::get_id();
        if (arrivals.fetch_add(1) < 2) { two_callbacks.count_down(); two_callbacks.wait(); }
      }
    });
  const auto paired = mhgp8::run_wspd_q2_census_ranges(index, 10, 8, WspdFrontMode::Pure, consumers, options);
  compare(gate, paired, baseline, geometry, options, slots);
  std::set<std::thread::id> distinct;
  for (std::size_t i = 0; i < first.size(); ++i) if (first[i]) distinct.insert(thread_ids[i]);
  gate.require(distinct.size() >= 2 && !distinct.contains(std::this_thread::get_id()),
               "range callbacks did not execute on different real worker threads");
  gate.multiple_callback_threads += distinct.size();

  struct ThreadExit {
    std::atomic<unsigned>* count;
    explicit ThreadExit(std::atomic<unsigned>& value) : count(&value) { count->fetch_add(1); }
    ~ThreadExit() { count->fetch_sub(1); }
  };
  std::atomic<unsigned> live_threads{0}, throwing_arrivals{0}, failures{0};
  std::array<bool, 4> seen{};
  std::latch two_throwing{2};
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
  try { static_cast<void>(mhgp8::run_wspd_q2_census_ranges(index, 10, 8, WspdFrontMode::Pure, throwing, options)); }
  catch (const CallbackFailure&) { caught = true; }
  gate.require(caught && throwing_arrivals.load() >= 2 && live_threads.load() == 0,
               "range callback failure returned before observed callback-producing workers exited");
  ++gate.callback_failures;
  gate.callback_threads_joined += throwing_arrivals.load();

  geometry.k = 1;
  geometry.pool = 2;
  const auto pool_base = coarse(gate, index, geometry, accepted(all, 1));
  gate.require(pool_base.result.pool_work.filtered_pairs == 3 && pool_base.result.pool_work.pair_roots == 1,
               "effective-Pool nested callback fixture changed");
  const std::weak_ptr<const mhgp8::Q2CensusIndex> weak = index;
  std::vector<Output> outer_slots(4), inner_slots(2);
  std::atomic<unsigned> nested{0};
  consumers.clear();
  for (std::size_t i = 0; i < outer_slots.size(); ++i)
    consumers.emplace_back([&, i](const mhgp8::Q2Support& item) {
      const auto before = copy(item);
      if (before.a == 1 && before.b == 2) {
        if (nested.fetch_add(1) != 0) throw std::runtime_error("duplicate nested Pool range support");
        index.reset();
        const auto owner = weak.lock();
        if (!owner) throw std::runtime_error("range callback lost its shared input owner");
        std::vector<mhgp8::Q2CensusConsumer> inner;
        for (std::size_t j = 0; j < inner_slots.size(); ++j)
          inner.emplace_back([&, j](const mhgp8::Q2Support& value) { inner_slots[j].push_back(copy(value)); });
        static_cast<void>(mhgp8::run_wspd_q2_census_ranges(owner, 5, 10, WspdFrontMode::MidpointSamples,
            inner, options, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst));
        if (!(copy(item) == before)) throw std::runtime_error("nested range team invalidated the outer borrowed payload");
      }
      outer_slots[i].push_back(copy(item));
    });
  const auto nested_result = mhgp8::run_wspd_q2_census_ranges(index, 1, 8, WspdFrontMode::Pure,
      consumers, options, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, 2);
  compare(gate, nested_result, pool_base, geometry, options, outer_slots);
  check_output(gate, join(inner_slots), accepted(all, 5));
  gate.require(nested.load() == 1 && !index && weak.expired(),
               "nested range callback/reset was skipped or leaked the shared index/Pool owner");
  ++gate.reentrant_runs;
  ++gate.index_resets;
}

void invalids(Gate& gate) {
  const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(axis()));
  std::atomic<unsigned> emissions{0};
  const std::vector<mhgp8::Q2CensusConsumer> consumers(2, [&](const mhgp8::Q2Support&) { emissions.fetch_add(1); });
  const auto invoke = [&](mhgp8::Q2CensusIndexPtr owner, unsigned k, unsigned s, WspdFrontMode front,
      const std::vector<mhgp8::Q2CensusConsumer>& slots, WspdQ2RangeOptions options = {},
      Q2SiblingMode sibling = Q2SiblingMode::Disabled, Q2WitnessOrder order = Q2WitnessOrder::GlobalDfs) {
    return mhgp8::run_wspd_q2_census_ranges(std::move(owner), k, s, front, slots, options, sibling, order);
  };
  gate.rejects([&] { static_cast<void>(invoke({}, 1, 8, WspdFrontMode::Pure, consumers)); }, "null range index accepted");
  for (unsigned k : {0U, 11U})
    gate.rejects([&] { static_cast<void>(invoke(index, k, 8, WspdFrontMode::Pure, consumers)); }, "invalid range K accepted");
  gate.rejects([&] { static_cast<void>(invoke(index, 1, 0, WspdFrontMode::Pure, consumers)); }, "zero range separation accepted");
  gate.rejects([&] { static_cast<void>(invoke(index, 1, 8, static_cast<WspdFrontMode>(99), consumers)); }, "invalid range front accepted");
  gate.rejects([&] { static_cast<void>(invoke(index, 1, 8, WspdFrontMode::Pure, {})); }, "empty range worker set accepted");
  auto empty = consumers;
  empty.back() = {};
  gate.rejects([&] { static_cast<void>(invoke(index, 1, 8, WspdFrontMode::Pure, empty)); }, "empty later range callback accepted");
  for (unsigned bad = 0; bad < 3; ++bad) {
    WspdQ2RangeOptions options;
    if (bad == 0) options.jobs_per_worker = 0;
    if (bad == 1) options.queue_capacity = 0;
    if (bad == 2) options.anchor_grain = 0;
    gate.rejects([&] { static_cast<void>(invoke(index, 1, 8, WspdFrontMode::Pure, consumers, options)); }, "zero range option accepted");
  }
  gate.rejects([&] { static_cast<void>(invoke(index, 1, 8, WspdFrontMode::Pure, consumers, {}, static_cast<Q2SiblingMode>(99))); },
               "invalid range sibling mode accepted");
  gate.rejects([&] { static_cast<void>(invoke(index, 1, 8, WspdFrontMode::Pure, consumers, {}, Q2SiblingMode::Disabled,
      static_cast<Q2WitnessOrder>(99))); }, "invalid range witness order accepted");
  gate.rejects<std::overflow_error>([&] { static_cast<void>(invoke(index, 1, 8, WspdFrontMode::Pure, consumers,
      {std::numeric_limits<std::size_t>::max(), 1, 1})); }, "range job target overflow accepted");
  gate.rejects<std::overflow_error>([&] { static_cast<void>(invoke(index, 1, 8, WspdFrontMode::Pure, consumers,
      {1, std::numeric_limits<std::size_t>::max(), 1})); }, "range queue-byte overflow accepted");
  gate.require(emissions.load() == 0, "invalid range options emitted before validation completed");

  const auto all = oracle(gate, axis());
  const GeometryOptions geometry;
  const auto baseline = coarse(gate, index, geometry, accepted(all, 10));
  const auto result = run(gate, index, baseline, geometry, 2, {1, 1, std::numeric_limits<std::size_t>::max()});
  gate.require(result.work.offer_checks == 0 && result.work.donations == 0,
               "huge scheduling grain truncated search/output or made an ineligible offer");
  auto changed = result.pipeline;
  ++changed.front.work.max_stack_size;
  gate.require(!same_geometry(changed, baseline.result), "logical front-stack mutant survived"); ++gate.mutants;
  changed = result.pipeline; ++changed.census_work.count_node_visits;
  gate.require(!same_geometry(changed, baseline.result), "replayed census visit mutant survived"); ++gate.mutants;
  changed = result.pipeline; ++changed.order_work.structural_splits;
  gate.require(!same_geometry(changed, baseline.result), "replayed structural work mutant survived"); ++gate.mutants;
  changed = result.pipeline; ++changed.pool_work.factor_sites;
  gate.require(!same_geometry(changed, baseline.result), "duplicated Pool factor preparation mutant survived"); ++gate.mutants;
  auto lost = baseline.output; lost.front().shell.pop_back();
  gate.require(lost != baseline.output, "truncated global shell mutant survived"); ++gate.mutants;
  auto mass_mutant = result; ++mass_mutant.work.completed_pairs;
  bool rejected = false;
  try { check_accounting(gate, mass_mutant, {1, 1, std::numeric_limits<std::size_t>::max()}); }
  catch (const std::runtime_error&) { rejected = true; }
  gate.require(rejected, "completed anchor-range mass mutant survived"); ++gate.mutants;
  auto donation_mutant = result; ++donation_mutant.work.pool_donations;
  rejected = false;
  try { check_accounting(gate, donation_mutant, {1, 1, std::numeric_limits<std::size_t>::max()}); }
  catch (const std::runtime_error&) { rejected = true; }
  gate.require(rejected, "donation-type mutant survived"); ++gate.mutants;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_wspd_q2_ranges_gate --selftest\n"; return 2;
  }
  try {
    Gate gate;
    corpus(gate); callbacks(gate); invalids(gate);
    gate.require(gate.clouds == 10 && gate.range_runs >= 410 && gate.coarse_runs >= 130 && gate.default_runs == 10 &&
                     gate.max_shell == 30 && gate.empty_runs > 0 && gate.surplus_workers > 0 &&
                     gate.selected64 > 0 && gate.filtered64 > 0 && gate.passthrough > 0 && gate.wide_pool_cases == 12 &&
                     gate.multiple_callback_threads >= 2 && gate.callback_threads_joined >= 2 && gate.callback_failures == 1 &&
                     gate.reentrant_runs == 1 && gate.index_resets == 1 && gate.invalid_inputs == 14 && gate.mutants == 7,
                 "range gate lost a required positive geometry/routing/lifetime fixture");
    // Donation counts are observations, not deterministic signatures or
    // nonvacuity promises. Seeds-exhausted means CLAIMED, not completed.
    std::cout << "{\"schema\":\"mhgp8_wspd_q2_ranges_gate_v1\",\"status\":\"passed\",\"public_status\":\"not_claimed\""
              << ",\"checks\":" << gate.checks << ",\"clouds\":" << gate.clouds << ",\"oracle_pairs\":" << gate.oracle_pairs
              << ",\"oracle_sites\":" << gate.oracle_sites << ",\"coarse_runs\":" << gate.coarse_runs
              << ",\"range_runs\":" << gate.range_runs << ",\"default_runs\":" << gate.default_runs
              << ",\"supports\":" << gate.supports << ",\"max_shell\":" << gate.max_shell
              << ",\"donations\":" << gate.donations << ",\"donations_after_seeds_claimed\":" << gate.after_seeds_claimed
              << ",\"pool_donations\":" << gate.pool_donations << ",\"passthrough_donations\":" << gate.passthrough_donations
              << ",\"wide_pool_cases\":" << gate.wide_pool_cases
              << ",\"empty_runs\":" << gate.empty_runs << ",\"surplus_workers\":" << gate.surplus_workers
              << ",\"selected64\":" << gate.selected64 << ",\"filtered64\":" << gate.filtered64 << ",\"passthrough\":" << gate.passthrough
              << ",\"multiple_callback_threads\":" << gate.multiple_callback_threads
              << ",\"callback_threads_joined\":" << gate.callback_threads_joined
              << ",\"callback_failures\":" << gate.callback_failures << ",\"reentrant_runs\":" << gate.reentrant_runs
              << ",\"index_resets\":" << gate.index_resets << ",\"invalid_inputs\":" << gate.invalid_inputs
              << ",\"mutants\":" << gate.mutants << "}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp8 range gate: " << error.what() << '\n'; return 1;
  }
}
