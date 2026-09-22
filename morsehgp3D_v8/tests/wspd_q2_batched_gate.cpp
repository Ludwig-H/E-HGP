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

#include "pipeline/wspd_q2_batched.hpp"

namespace {
using mhgp8::Point3;
using mhgp8::Q2SiblingMode;
using mhgp8::Q2WitnessOrder;
using mhgp8::WspdFrontMode;
using mhgp8::WspdQ2BatchOptions;
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
  u64 checks{}, clouds{}, oracle_pairs{}, oracle_sites{}, coarse_runs{}, batch_runs{}, default_runs{};
  u64 supports{}, max_shell{}, empty_runs{}, surplus_requests{};
  u64 selected64{}, filtered64{}, passthrough{}, compact_tasks{}, inherited_credit{}, phase2_tasks{}, siblings_due{};
  u64 callback_failures{}, reentrant_runs{}, index_resets{}, multiple_callback_threads{}, callback_threads_joined{};
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

// Explicit adaptation of the ranges gate at2741d614: oracle methodology,
// fixtures and comparison structure only, not its answers or qualification. Neither the front
// nor product box/census predicates enter this exhaustive bounded judge.
// All promoted dot products fit int64 over the quantized u16 domain.
Output oracle(Gate& gate, const Points& points) {
  gate.require(!points.empty() && points.size() <= 100, "batch oracle exceeded its bounded n<=100 domain");
  Output result;
  for (std::size_t a = 0; a < points.size(); ++a) for (std::size_t b = a + 1; b < points.size(); ++b) {
    Support item;
    item.a = a;
    item.b = b;
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
  gate.require(output == expected, "batch support/key/interior/full-shell multiset differs from brute-force oracle");
  for (const auto& item : output) {
    gate.require(std::adjacent_find(item.interior.begin(), item.interior.end()) == item.interior.end() &&
                     std::adjacent_find(item.shell.begin(), item.shell.end()) == item.shell.end() &&
                     std::binary_search(item.shell.begin(), item.shell.end(), item.a) &&
                     std::binary_search(item.shell.begin(), item.shell.end(), item.b),
                 "batch payload duplicated an ID or omitted an endpoint from the full shell");
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
auto additive(const mhgp8::Q2BatchWork& w) {
  return std::array{w.enqueued, w.completed, w.completed_accepted, w.completed_rejected,
      w.batch_passes, w.lane_visits, w.transitions, w.entry_steps, w.witness_steps,
      w.admission_steps, w.payload_steps, w.sibling_due, w.entry_after_credit,
      w.entry_inside_deferred, w.key_preparations, w.sibling_bound_tests, w.sibling_rejected,
      w.sibling_rejected_after_credit, w.full_drains, w.seed_flushes, w.pool_flushes};
}
void work_contract(Gate& gate, const mhgp8::Q2BatchWork& w, WspdQ2BatchOptions options) {
  gate.require(w.enqueued == w.completed && w.completed == w.completed_accepted + w.completed_rejected &&
                   w.enqueued == w.entry_steps && w.key_preparations == w.enqueued && w.admission_steps == w.completed_accepted &&
                   w.payload_steps == w.completed_accepted &&
                   w.transitions == w.entry_steps + w.witness_steps + w.admission_steps + w.payload_steps,
               "batched state lineage, entry, admission or emission is inconsistent");
  gate.require(w.entry_after_credit <= w.entry_steps && w.entry_inside_deferred <= w.entry_steps &&
                   w.sibling_due <= w.entry_steps && w.sibling_bound_tests <= w.sibling_due &&
                   w.sibling_rejected <= w.sibling_bound_tests && w.sibling_rejected_after_credit <= w.sibling_rejected &&
                   w.sibling_rejected_after_credit <= w.entry_after_credit && w.full_drains <= w.enqueued &&
                   w.max_active <= options.lanes && ((w.enqueued == 0) == (w.max_active == 0)) &&
                   w.batch_passes <= w.lane_visits && w.lane_visits <= w.transitions,
               "batched active-state bounds or positive work accounting is inconsistent");
  const auto needed_visits = w.transitions / options.quantum + (w.transitions % options.quantum != 0);
  gate.require(needed_visits <= w.lane_visits, "batched lane exceeded its positive transition quantum");
}
void check_accounting(Gate& gate, const mhgp8::WspdQ2BatchResult& result, WspdQ2BatchOptions options) {
  const auto& p = result.pipeline;
  const auto& w = result.work;
  gate.require(p.started_workers == std::min(p.requested_workers, p.jobs) &&
                   p.workers.size() == p.started_workers && result.workers.size() == p.started_workers &&
                   p.target_jobs == p.requested_workers * options.jobs_per_worker && p.completed_jobs == p.jobs &&
                   p.terminal_jobs <= p.jobs && p.dispatch_work == mhgp8::WspdFrontDispatchWork{} && p.queue_storage_bytes == 0,
               "batched Coarse worker/seed metadata or no-shared-queue contract failed");
  auto work_sum = additive(w);
  work_sum.fill(0);
  u64 maximum = 0, capacity = 0, storage = 0;
  u64 jobs = 0, products = p.prefix_product_visits, rectangles = 0, visits = 0, supports = 0, peak_sum = 0, peak_max = 0;
  double elapsed = 0, payload = 0;
  for (std::size_t i = 0; i < result.workers.size(); ++i) {
    const auto& batch = result.workers[i];
    const auto& worker = batch.work;
    const auto& pipeline = p.workers[i];
    add(work_sum, additive(worker));
    maximum = std::max(maximum, worker.max_active);
    capacity += batch.state_capacity;
    storage += batch.state_storage_bytes;
    jobs += pipeline.jobs;
    products += pipeline.front_products;
    rectangles += pipeline.input_rectangles;
    visits += pipeline.count_node_visits;
    supports += pipeline.supports;
    peak_sum += pipeline.pool_peak_bytes;
    peak_max = std::max(peak_max, pipeline.pool_peak_bytes);
    elapsed += pipeline.elapsed_ms;
    payload += pipeline.payload_ms;
    work_contract(gate, worker, options);
    gate.require(batch.state_capacity >= options.lanes &&
                     batch.state_storage_bytes == batch.state_capacity * result.state_bytes && worker.seed_flushes == pipeline.jobs,
                 "batched worker omitted reusable state capacity or completed a seed before flushing it");
    gate.require(pipeline.dispatch_work == mhgp8::WspdFrontDispatchWork{} &&
                     std::isfinite(pipeline.elapsed_ms) && std::isfinite(pipeline.payload_ms) &&
                     pipeline.elapsed_ms >= 0 && pipeline.payload_ms >= 0 && pipeline.elapsed_ms <= p.total_ms + 1e-6,
                 "batched worker donated front work or misreported its enclosing interval");
  }
  gate.require(work_sum == additive(w) && maximum == w.max_active && capacity == result.state_capacity_sum &&
                   storage == result.state_storage_bytes_sum,
               "batched worker reduction lost state storage, an additive counter or a maximum");
  gate.require(jobs == p.jobs && products == p.front.work.product_visits && rectangles == p.input_rectangles &&
                   visits == p.census_work.count_node_visits && supports == p.accepted_pairs && peak_sum == p.pool_peak_bytes_sum &&
                   peak_max == p.pool_work.plan_peak_bytes && close(elapsed, p.worker_ms_sum) && close(payload, p.payload_ms_sum),
               "batched worker reduction lost geometry, Pool maxima or interval sums");
  work_contract(gate, w, options);
  gate.require(w.seed_flushes == p.jobs && w.pool_flushes == p.pool_work.selected_rectangles &&
                   w.entry_steps <= p.census_work.query_tasks && w.completed_accepted <= p.accepted_pairs &&
                   w.completed_rejected <= p.rejected_pairs && result.state_bytes > 0 && result.state_bytes < 6272,
               "batched route omitted a Pool flush or duplicated query/pair responsibilities");
  for (const double value : {p.total_ms, p.partition_ms, p.worker_ms_sum, p.payload_ms_sum,
                            p.pool_work.preparation_ms, p.pool_work.selected_total_ms})
    gate.require(std::isfinite(value) && value >= 0, "batched timing is negative or nonfinite");
}

void compare(Gate& gate, const mhgp8::WspdQ2BatchResult& result, const Baseline& baseline,
             const GeometryOptions& geometry, WspdQ2BatchOptions options, const std::vector<Output>& slots) {
  check_output(gate, join(slots), baseline.output);
  gate.require(same_geometry(result.pipeline, baseline.result),
               "batch changed an old front/census/Pool integer counter or exact output");
  check_accounting(gate, result, options);
  const auto& p = result.pipeline;
  const auto& w = result.work;
  gate.require(p.requested_workers == slots.size(), "batch changed the number of callback slots");
  for (std::size_t i = 0; i < p.workers.size(); ++i)
    gate.require(p.workers[i].supports == slots[i].size(), "batch worker used another callback slot");
  for (std::size_t i = p.workers.size(); i < slots.size(); ++i)
    gate.require(slots[i].empty(), "batch invoked an unstarted worker's callback slot");
  gate.compact_tasks += w.enqueued;
  gate.inherited_credit += w.entry_after_credit;
  gate.phase2_tasks += w.entry_inside_deferred;
  gate.siblings_due += w.sibling_due;
  gate.empty_runs += p.jobs == 0;
  gate.surplus_requests += p.jobs > 0 && p.requested_workers > p.jobs;
  gate.passthrough += p.pool_work.passthrough_rectangles;
  if (geometry.pool == 64) {
    gate.selected64 += p.pool_work.selected_rectangles;
    gate.filtered64 += p.pool_work.filtered_pairs;
  }
  ++gate.batch_runs;
}

mhgp8::WspdQ2BatchResult run(Gate& gate, const mhgp8::Q2CensusIndexPtr& index, const Baseline& baseline,
    const GeometryOptions& geometry, std::size_t workers, WspdQ2BatchOptions options, bool defaults = false) {
  const auto* nodes = index->spatial_nodes().data();
  const auto* order = index->spatial_order().data();
  const auto* coordinates = index->cloud().points().data();
  const auto visits = index->work().point_visits;
  std::vector<Output> slots(workers);
  std::vector<mhgp8::Q2CensusConsumer> consumers;
  for (std::size_t i = 0; i < workers; ++i)
    consumers.emplace_back([&, i](const mhgp8::Q2Support& item) { slots[i].push_back(copy(item)); });
  const auto result = defaults
      ? mhgp8::run_wspd_q2_census_batched(index, geometry.k, geometry.s, geometry.front, consumers)
      : mhgp8::run_wspd_q2_census_batched(index, geometry.k, geometry.s, geometry.front, consumers,
          options, geometry.sibling, geometry.order, geometry.pool);
  compare(gate, result, baseline, geometry, options, slots);
  gate.require(nodes == index->spatial_nodes().data() && order == index->spatial_order().data() &&
                   coordinates == index->cloud().points().data() && visits == index->work().point_visits &&
                   result.pipeline.census_work.query_build_nodes == 0 && result.pipeline.census_work.query_cover_visits == 0,
               "batch changed the immutable index or rebuilt a query factor");
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
  result.push_back({{0, 0, 0}, {5, 0, 0}, {10, 0, 0}, {11, 0, 0}});
  result.push_back({{0, 0, 0}, {500, 0, 0}, {1000, 0, 0}, {1001, 0, 0}, {1030, 0, 0}});
  result.push_back({{1000, 0, 0}, {0, 1, 0}, {0, 0, 0}});
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
      static_cast<void>(run(gate, index, baseline, geometry, 2, {1, 4, 7}));
      static_cast<void>(run(gate, index, baseline, geometry, 4, {4, 8, 256}));
      static_cast<void>(run(gate, index, baseline, geometry, 8, {1, 16, 1}));
      if (choice == 0) {
        const GeometryOptions defaults{k, s};
        const auto default_base = coarse(gate, index, defaults, accepted(all, k));
        static_cast<void>(run(gate, index, default_base, defaults, 2, {}, true));
      }
    }
    // All Pool routes, including passthrough, remain synchronous. The exact
    // comparison includes preparation counts and all historical clock-free
    // counters; compact tasks must flush before any temporary B order.
    for (const auto pool : {std::size_t{0}, std::size_t{2}, std::size_t{64}}) {
      const GeometryOptions fine{10, 8, WspdFrontMode::Pure, Q2SiblingMode::Saturating,
                                 Q2WitnessOrder::ComplementFirst, pool};
      const auto baseline = coarse(gate, index, fine, accepted(all, 10));
      static_cast<void>(run(gate, index, baseline, fine, 4, {1, 16, 1}));
    }
    ++gate.clouds;
  }
}

void inherited_fixtures(Gate& gate) {
  const std::array<Points, 3> points{{
      {{0, 0, 0}, {5, 0, 0}, {10, 0, 0}, {11, 0, 0}},
      {{0, 0, 0}, {500, 0, 0}, {1000, 0, 0}, {1001, 0, 0}, {1030, 0, 0}},
      {{1000, 0, 0}, {0, 1, 0}, {0, 0, 0}}}};
  for (std::size_t fixture = 0; fixture < points.size(); ++fixture) {
    const auto all = oracle(gate, points[fixture]);
    const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points[fixture]));
    for (const auto order : {Q2WitnessOrder::GlobalDfs, Q2WitnessOrder::ComplementFirst}) {
      const unsigned k = fixture == 2 ? 1U : 2U;
      const GeometryOptions geometry{k, 8, WspdFrontMode::Pure, Q2SiblingMode::Saturating, order, 0};
      const auto baseline = coarse(gate, index, geometry, accepted(all, k));
      for (const auto lanes : {std::size_t{1}, std::size_t{4}, std::size_t{8}, std::size_t{16}})
        for (const auto quantum : {std::size_t{1}, std::size_t{7}, std::size_t{256}}) {
          const auto result = run(gate, index, baseline, geometry, 2, {1, lanes, quantum});
          gate.require(result.work.sibling_due > 0, "targeted singleton fixture lost its due sibling entry");
          if (fixture < 2)
            gate.require(result.work.entry_after_credit > 0, "targeted singleton fixture lost its inherited credit");
          if (fixture < 2 && order == Q2WitnessOrder::ComplementFirst)
            gate.require(result.work.entry_inside_deferred > 0, "targeted singleton fixture lost its inherited original-B phase");
          if (fixture == 1)
            gate.require(result.work.sibling_bound_tests > 0 && result.work.sibling_rejected > 0 &&
                             result.work.sibling_rejected_after_credit > 0,
                         "singleton sibling certificate no longer rejects autonomously after inherited credit");
          if (fixture == 2)
            gate.require(result.work.sibling_bound_tests == 2 && result.work.sibling_rejected == 0,
                         "tangent singleton sibling was not tested twice or was incorrectly credited");
        }
    }
  }
}

struct CallbackFailure {};
void callbacks(Gate& gate) {
  // Explicit adaptation of cooperative-gate lifetime tests. The two seed
  // subtrees on this axis fixture can emit independently: this latch proves
  // distinct productive callback threads, never a successful batch donation.
  const auto points = axis();
  const auto all = oracle(gate, points);
  auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
  const WspdQ2BatchOptions options{1, 1, 1};
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
  const auto paired = mhgp8::run_wspd_q2_census_batched(index, 10, 8, WspdFrontMode::Pure, consumers, options);
  compare(gate, paired, baseline, geometry, options, slots);
  std::set<std::thread::id> distinct;
  for (std::size_t i = 0; i < first.size(); ++i) if (first[i]) distinct.insert(thread_ids[i]);
  gate.require(distinct.size() >= 2 && !distinct.contains(std::this_thread::get_id()),
               "batch callbacks did not execute on different real worker threads");
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
  try { static_cast<void>(mhgp8::run_wspd_q2_census_batched(index, 10, 8, WspdFrontMode::Pure, throwing, options)); }
  catch (const CallbackFailure&) { caught = true; }
  gate.require(caught && throwing_arrivals.load() >= 2 && live_threads.load() == 0,
               "batch callback failure returned before observed callback-producing workers exited");
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
        if (nested.fetch_add(1) != 0) throw std::runtime_error("duplicate nested Pool batch support");
        index.reset();
        const auto owner = weak.lock();
        if (!owner) throw std::runtime_error("batch callback lost its shared input owner");
        std::vector<mhgp8::Q2CensusConsumer> inner;
        for (std::size_t j = 0; j < inner_slots.size(); ++j)
          inner.emplace_back([&, j](const mhgp8::Q2Support& value) { inner_slots[j].push_back(copy(value)); });
        static_cast<void>(mhgp8::run_wspd_q2_census_batched(owner, 5, 10, WspdFrontMode::MidpointSamples,
            inner, options, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst));
        if (!(copy(item) == before)) throw std::runtime_error("nested batch team invalidated the outer borrowed payload");
      }
      outer_slots[i].push_back(copy(item));
    });
  const auto nested_result = mhgp8::run_wspd_q2_census_batched(index, 1, 8, WspdFrontMode::Pure,
      consumers, options, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, 2);
  compare(gate, nested_result, pool_base, geometry, options, outer_slots);
  check_output(gate, join(inner_slots), accepted(all, 5));
  gate.require(nested.load() == 1 && !index && weak.expired(),
               "nested batch callback/reset was skipped or leaked the shared index/Pool owner");
  ++gate.reentrant_runs;
  ++gate.index_resets;
}
void invalids(Gate& gate) {
  const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(axis()));
  std::atomic<unsigned> emissions{0};
  const std::vector<mhgp8::Q2CensusConsumer> consumers(2, [&](const mhgp8::Q2Support&) { emissions.fetch_add(1); });
  const auto invoke = [&](mhgp8::Q2CensusIndexPtr owner, unsigned k, unsigned s, WspdFrontMode front,
      const std::vector<mhgp8::Q2CensusConsumer>& slots, WspdQ2BatchOptions options = {},
      Q2SiblingMode sibling = Q2SiblingMode::Disabled, Q2WitnessOrder order = Q2WitnessOrder::GlobalDfs) {
    return mhgp8::run_wspd_q2_census_batched(std::move(owner), k, s, front, slots, options, sibling, order);
  };
  gate.rejects([&] { static_cast<void>(invoke({}, 1, 8, WspdFrontMode::Pure, consumers)); }, "null batch index accepted");
  for (unsigned k : {0U, 11U})
    gate.rejects([&] { static_cast<void>(invoke(index, k, 8, WspdFrontMode::Pure, consumers)); }, "invalid batch K accepted");
  gate.rejects([&] { static_cast<void>(invoke(index, 1, 0, WspdFrontMode::Pure, consumers)); }, "zero batch separation accepted");
  gate.rejects([&] { static_cast<void>(invoke(index, 1, 8, static_cast<WspdFrontMode>(99), consumers)); }, "invalid batch front accepted");
  gate.rejects([&] { static_cast<void>(invoke(index, 1, 8, WspdFrontMode::Pure, {})); }, "empty batch worker set accepted");
  auto empty = consumers;
  empty.back() = {};
  gate.rejects([&] { static_cast<void>(invoke(index, 1, 8, WspdFrontMode::Pure, empty)); }, "empty later batch callback accepted");
  for (unsigned bad = 0; bad < 3; ++bad) {
    WspdQ2BatchOptions options;
    if (bad == 0) options.jobs_per_worker = 0;
    if (bad == 1) options.lanes = 0;
    if (bad == 2) options.quantum = 0;
    gate.rejects([&] { static_cast<void>(invoke(index, 1, 8, WspdFrontMode::Pure, consumers, options)); }, "zero batch option accepted");
  }
  gate.rejects([&] { static_cast<void>(invoke(index, 1, 8, WspdFrontMode::Pure, consumers, {}, static_cast<Q2SiblingMode>(99))); },
               "invalid batch sibling mode accepted");
  gate.rejects([&] { static_cast<void>(invoke(index, 1, 8, WspdFrontMode::Pure, consumers, {}, Q2SiblingMode::Disabled,
      static_cast<Q2WitnessOrder>(99))); }, "invalid batch witness order accepted");
  gate.rejects<std::overflow_error>([&] { static_cast<void>(invoke(index, 1, 8, WspdFrontMode::Pure, consumers,
      {std::numeric_limits<std::size_t>::max(), 1, 1})); }, "batch job target overflow accepted");
  gate.rejects<std::overflow_error>([&] { static_cast<void>(invoke(index, 1, 8, WspdFrontMode::Pure, consumers,
      {1, std::numeric_limits<std::size_t>::max(), 1})); }, "batch queue-byte overflow accepted");
  gate.require(emissions.load() == 0, "invalid batch options emitted before validation completed");

  const auto all = oracle(gate, axis());
  const GeometryOptions geometry;
  const auto baseline = coarse(gate, index, geometry, accepted(all, 10));
  const auto result = run(gate, index, baseline, geometry, 2, {1, 1, std::numeric_limits<std::size_t>::max()});
  gate.require(result.work.enqueued > 0 && result.work.completed == result.work.enqueued,
               "huge scheduling quantum truncated search/output");
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
  auto mass_mutant = result; ++mass_mutant.work.completed;
  bool rejected = false;
  try { check_accounting(gate, mass_mutant, {1, 1, std::numeric_limits<std::size_t>::max()}); }
  catch (const std::runtime_error&) { rejected = true; }
  gate.require(rejected, "completed singleton mass mutant survived"); ++gate.mutants;
  auto donation_mutant = result; ++donation_mutant.work.key_preparations;
  rejected = false;
  try { check_accounting(gate, donation_mutant, {1, 1, std::numeric_limits<std::size_t>::max()}); }
  catch (const std::runtime_error&) { rejected = true; }
  gate.require(rejected, "key-preparation replay mutant survived"); ++gate.mutants;
}


}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_wspd_q2_batched_gate --selftest\n"; return 2;
  }
  try {
    Gate gate;
    corpus(gate); inherited_fixtures(gate); callbacks(gate); invalids(gate);
    gate.require(gate.clouds == 13 && gate.batch_runs >= 590 && gate.coarse_runs >= 170 && gate.default_runs == 13 &&
                     gate.max_shell == 30 && gate.empty_runs > 0 && gate.surplus_requests > 0 &&
                     gate.selected64 > 0 && gate.filtered64 > 0 && gate.passthrough > 0 &&
                     gate.compact_tasks > 0 && gate.inherited_credit > 0 && gate.phase2_tasks > 0 && gate.siblings_due > 0 &&
                     gate.multiple_callback_threads >= 2 && gate.callback_threads_joined >= 2 && gate.callback_failures == 1 &&
                     gate.reentrant_runs == 1 && gate.index_resets == 1 && gate.invalid_inputs == 14 && gate.mutants == 7,
                 "batch gate lost a required positive geometry/routing/lifetime fixture");
    std::cout << "{\"schema\":\"mhgp8_wspd_q2_batched_gate_v1\",\"status\":\"passed\",\"public_status\":\"not_claimed\""
              << ",\"checks\":" << gate.checks << ",\"clouds\":" << gate.clouds << ",\"oracle_pairs\":" << gate.oracle_pairs
              << ",\"oracle_sites\":" << gate.oracle_sites << ",\"coarse_runs\":" << gate.coarse_runs
              << ",\"batch_runs\":" << gate.batch_runs << ",\"default_runs\":" << gate.default_runs
              << ",\"supports\":" << gate.supports << ",\"max_shell\":" << gate.max_shell
              << ",\"compact_tasks\":" << gate.compact_tasks << ",\"inherited_credit\":" << gate.inherited_credit
              << ",\"phase2_tasks\":" << gate.phase2_tasks << ",\"siblings_due\":" << gate.siblings_due
              << ",\"empty_runs\":" << gate.empty_runs << ",\"surplus_requests\":" << gate.surplus_requests
              << ",\"selected64\":" << gate.selected64 << ",\"filtered64\":" << gate.filtered64 << ",\"passthrough\":" << gate.passthrough
              << ",\"multiple_callback_threads\":" << gate.multiple_callback_threads
              << ",\"callback_threads_joined\":" << gate.callback_threads_joined
              << ",\"callback_failures\":" << gate.callback_failures << ",\"reentrant_runs\":" << gate.reentrant_runs
              << ",\"index_resets\":" << gate.index_resets << ",\"invalid_inputs\":" << gate.invalid_inputs
              << ",\"mutants\":" << gate.mutants << "}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp8 batched gate: " << error.what() << '\n'; return 1;
  }
}
