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
#include <span>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "parallel/joined_workers.hpp"
#include "parallel/work_reduction.hpp"
#include "pipeline/wspd_q2_parallel.hpp"

namespace {

using mhgp9::gen::Point3;
using mhgp9::gen::Q2AnchorMode;
using mhgp9::gen::Q2CensusConsumer;
using mhgp9::gen::Q2CensusMode;
using mhgp9::gen::Q2SiblingMode;
using mhgp9::gen::Q2WitnessOrder;
using mhgp9::gen::WspdFrontMode;
using mhgp9::gen::u64;
using Points = std::vector<Point3>;
using Pair = std::pair<std::size_t, std::size_t>;

struct Support {
  Pair pair;
  mhgp9::gen::Q2BallKey key;
  std::vector<std::size_t> interior, shell;
  bool operator==(const Support&) const = default;
};
using Output = std::vector<Support>;

struct Options {
  unsigned k{5}, s{8};
  WspdFrontMode front{WspdFrontMode::MidpointSamples};
  Q2CensusMode census{Q2CensusMode::SharedBlocks};
  Q2SiblingMode sibling{Q2SiblingMode::Saturating};
  Q2WitnessOrder order{Q2WitnessOrder::ComplementFirst};
  Q2AnchorMode anchor{Q2AnchorMode::Individual};
  std::size_t pool{};
};

struct Gate {
  u64 checks{}, clouds{}, oracle_pairs{}, oracle_sites{}, mono_runs{}, parallel_runs{};
  u64 supports{}, max_shell{}, empty_job_runs{}, excess_worker_runs{}, multi_worker_runs{};
  u64 pool_selected{}, pool_filtered{}, pool_passthrough{}, selected64{}, filtered64{};
  u64 joint_runs{}, pairwise_runs{}, sibling_tests{}, structural_steps{}, callback_failures{};
  u64 reentrant_runs{}, ownership_resets{}, joined_checks{}, launch_failures{}, invalid_inputs{}, mutants{};
  // The 18-bit twin corpus is counted under its own exact pins so that every
  // u16 pin (clouds, parallel_runs window, max_shell) keeps its value.
  bool wide{};
  u64 clouds18{}, parallel_runs18{}, supports18{}, max_shell18{};

  void require(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
  }
  template <class Exception = std::invalid_argument, class Function>
  void rejects(Function&& function, const char* message) {
    bool caught = false;
    try { function(); } catch (const Exception&) { caught = true; }
    require(caught, message);
    ++invalid_inputs;
  }
};

void sort_output(Output& output) {
  std::sort(output.begin(), output.end(), [](const auto& a, const auto& b) { return a.pair < b.pair; });
}

Support copy_support(const mhgp9::gen::Q2Support& source) {
  Support result{{std::min(source.a_id, source.b_id), std::max(source.a_id, source.b_id)}, source.key,
                 {source.interior.begin(), source.interior.end()}, {source.shell.begin(), source.shell.end()}};
  std::sort(result.interior.begin(), result.interior.end());
  std::sort(result.shell.begin(), result.shell.end());
  return result;
}

// All-pairs/all-sites TEST oracle, bounded here by n<=140. This is fresh
// scalar geometry, not a product front, bound, census, Pool or merge helper.
// Promoted signed dot products obey |H|<=3*262143^2 < 2^38, within int64_t.
Output oracle(Gate& gate, const Points& points) {
  gate.require(!points.empty() && points.size() <= 140, "oracle fixture escaped its declared small domain");
  Output result;
  for (std::size_t a = 0; a < points.size(); ++a) {
    for (std::size_t b = a + 1; b < points.size(); ++b) {
      Support item;
      item.pair = {a, b};
      for (std::size_t axis = 0; axis < 3; ++axis) {
        item.key.center_twice[axis] = static_cast<std::uint32_t>(points[a][axis]) + points[b][axis];
        const auto difference = std::int64_t{points[a][axis]} - points[b][axis];
        item.key.diameter_squared += static_cast<u64>(difference * difference);
      }
      for (std::size_t z = 0; z < points.size(); ++z) {
        std::int64_t h = 0;
        for (std::size_t axis = 0; axis < 3; ++axis) {
          h += (std::int64_t{points[z][axis]} - points[a][axis]) *
               (std::int64_t{points[b][axis]} - points[z][axis]);
        }
        if (h > 0) item.interior.push_back(z);
        if (h == 0) item.shell.push_back(z);
        ++gate.oracle_sites;
      }
      result.push_back(std::move(item));
      ++gate.oracle_pairs;
    }
  }
  return result;
}

Output admitted(const Output& all, unsigned k) {
  Output result;
  for (const auto& item : all) if (item.interior.size() < k) result.push_back(item);
  return result;
}

void check_output(Gate& gate, Output& output, const Output& expected, std::size_t n, unsigned k) {
  sort_output(output);
  gate.require(output == expected, "parallel support/key/interior/full-shell multiset differs from scalar oracle");
  for (const auto& item : output) {
    gate.require(item.pair.first < item.pair.second && item.pair.second < n && item.interior.size() < k,
                 "accepted payload has invalid support IDs or saturated depth");
    for (const auto* ids : {&item.interior, &item.shell}) {
      gate.require(std::is_sorted(ids->begin(), ids->end()) &&
                       std::adjacent_find(ids->begin(), ids->end()) == ids->end() &&
                       (ids->empty() || ids->back() < n), "payload IDs are duplicated or outside the owner");
    }
    gate.require(std::binary_search(item.shell.begin(), item.shell.end(), item.pair.first) &&
                     std::binary_search(item.shell.begin(), item.shell.end(), item.pair.second),
                 "complete shell lost a support endpoint");
    auto& max_shell = gate.wide ? gate.max_shell18 : gate.max_shell;
    max_shell = std::max(max_shell, static_cast<u64>(item.shell.size()));
  }
  (gate.wide ? gate.supports18 : gate.supports) += output.size();
}

struct Mono {
  mhgp9::gen::WspdQ2CensusResult result;
  Output output;
};

Mono mono(Gate& gate, const mhgp9::gen::Q2CensusIndex& index, const Output& all, const Options& options) {
  Mono result;
  result.result = mhgp9::gen::run_wspd_q2_census(index, options.k, options.s, options.front, options.census,
      [&](const mhgp9::gen::Q2Support& item) { result.output.push_back(copy_support(item)); },
      options.sibling, options.order, options.anchor, options.pool);
  check_output(gate, result.output, admitted(all, options.k), index.cloud().points().size(), options.k);
  ++gate.mono_runs;
  return result;
}

mhgp9::gen::Q2PoolWork integer_pool(mhgp9::gen::Q2PoolWork work) {
  work.preparation_ms = 0;
  work.selected_total_ms = 0;
  return work;
}

bool same_work(const mhgp9::gen::WspdQ2ParallelResult& parallel, const mhgp9::gen::WspdQ2CensusResult& reference) {
  return parallel.front.total_unordered_pairs == reference.front.total_unordered_pairs &&
         parallel.front.active_lane_mask == reference.front.active_lane_mask &&
         parallel.front.work == reference.front.work && parallel.census_work == reference.census.work &&
         parallel.sibling_work == reference.sibling_work && parallel.order_work == reference.order_work &&
         parallel.joint_work == reference.joint_work && integer_pool(parallel.pool_work) == integer_pool(reference.pool_work) &&
         parallel.input_rectangles == reference.input_rectangles && parallel.anchor_queries == reference.anchor_queries &&
         parallel.candidate_pairs == reference.census.candidate_pairs &&
         parallel.accepted_pairs == reference.census.accepted_pairs && parallel.rejected_pairs == reference.census.rejected_pairs;
}

bool finite_nonnegative(double value) { return std::isfinite(value) && value >= 0; }
bool near(double a, double b) { return std::abs(a - b) <= 1e-8 * (1 + std::max(std::abs(a), std::abs(b))); }

struct Parallel {
  mhgp9::gen::WspdQ2ParallelResult result;
  Output output;
};

Parallel parallel(Gate& gate, const mhgp9::gen::Q2CensusIndexPtr& index, const Output& all,
                  const Options& options, const Mono& reference, std::size_t workers, std::size_t granularity) {
  // Separate vector object per slot, with all outer allocation complete
  // before launching. No worker touches Gate or another slot's buffer.
  std::vector<Output> slots(workers);
  std::vector<Q2CensusConsumer> consumers;
  for (std::size_t i = 0; i < workers; ++i) {
    consumers.emplace_back([&, i](const mhgp9::gen::Q2Support& item) { slots[i].push_back(copy_support(item)); });
  }
  Parallel capture;
  capture.result = mhgp9::gen::run_wspd_q2_census_parallel(index, options.k, options.s, options.front, options.census,
      consumers, granularity, options.sibling, options.order, options.anchor, options.pool);
  const auto& result = capture.result;
  for (const auto& slot : slots) capture.output.insert(capture.output.end(), slot.begin(), slot.end());
  check_output(gate, capture.output, admitted(all, options.k), index->cloud().points().size(), options.k);
  gate.require(capture.output == reference.output && same_work(result, reference.result),
               "parallel scheduling changed exact output or a mono work field/max/bin");
  gate.require(result.requested_workers == workers && result.target_jobs == workers * granularity &&
                   result.started_workers == std::min<u64>(workers, result.jobs) &&
                   result.workers.size() == result.started_workers && result.completed_jobs == result.jobs &&
                   result.terminal_jobs <= result.jobs &&
                   (result.jobs <= result.target_jobs || result.jobs - result.target_jobs <= 2),
               "worker/job metadata violates bounded preparation or exact completion");
  gate.require(result.jobs == 0 || result.job_storage_bytes > 0, "retained jobs report zero vector storage");
  u64 jobs = 0, products = result.prefix_product_visits, rectangles = 0, visits = 0, supports = 0;
  u64 pool_sum = 0, pool_max = 0;
  double worker_ms = 0, payload_ms = 0;
  for (std::size_t slot = 0; slot < result.workers.size(); ++slot) {
    const auto& work = result.workers[slot];
    jobs += work.jobs; products += work.front_products; rectangles += work.input_rectangles;
    visits += work.count_node_visits; supports += work.supports;
    pool_sum += work.pool_peak_bytes; pool_max = std::max(pool_max, work.pool_peak_bytes);
    worker_ms += work.elapsed_ms; payload_ms += work.payload_ms;
    gate.require(work.supports == slots[slot].size() && finite_nonnegative(work.elapsed_ms) &&
                     finite_nonnegative(work.payload_ms) && work.payload_ms <= work.elapsed_ms + 1e-8,
                 "worker payload ownership or per-slot inclusive times failed");
  }
  for (std::size_t slot = result.workers.size(); slot < slots.size(); ++slot)
    gate.require(slots[slot].empty(), "an unstarted callback slot received an emission");
  gate.require(jobs == result.jobs && products == result.front.work.product_visits &&
                   rectangles == result.input_rectangles && visits == result.census_work.count_node_visits &&
                   supports == result.accepted_pairs && pool_sum == result.pool_peak_bytes_sum &&
                   pool_max == result.pool_work.plan_peak_bytes && near(worker_ms, result.worker_ms_sum) &&
                   near(payload_ms, result.payload_ms_sum), "private-worker reductions lost units or maxima");
  gate.require(finite_nonnegative(result.total_ms) && finite_nonnegative(result.partition_ms) &&
                   result.partition_ms <= result.total_ms + 1e-8 && finite_nonnegative(result.pool_work.preparation_ms) &&
                   finite_nonnegative(result.pool_work.selected_total_ms) &&
                   result.pool_work.preparation_ms <= result.pool_work.selected_total_ms + 1e-8 &&
                   result.pool_work.selected_total_ms <= result.worker_ms_sum + 1e-8,
               "parallel timers are nonfinite, negative, or violate their own enclosing intervals");
  // Worker/payload sums can exceed wall time; do not subtract or compare
  // them to total_ms as if concurrent intervals were disjoint wall time.
  gate.empty_job_runs += static_cast<u64>(result.jobs == 0);
  gate.excess_worker_runs += static_cast<u64>(workers > result.jobs);
  gate.multi_worker_runs += static_cast<u64>(result.started_workers > 1);
  gate.pool_selected += result.pool_work.selected_rectangles;
  gate.pool_filtered += result.pool_work.filtered_pairs;
  gate.pool_passthrough += result.pool_work.passthrough_rectangles;
  if (options.pool == 64) {
    gate.selected64 += result.pool_work.selected_rectangles;
    gate.filtered64 += result.pool_work.filtered_pairs;
  }
  gate.joint_runs += static_cast<u64>(options.anchor != Q2AnchorMode::Individual);
  gate.pairwise_runs += static_cast<u64>(options.census == Q2CensusMode::Pairwise);
  gate.sibling_tests += result.sibling_work.bound_tests;
  gate.structural_steps += result.order_work.structural_splits + result.joint_work.structural_splits;
  ++(gate.wide ? gate.parallel_runs18 : gate.parallel_runs);
  return capture;
}

Points axis_fixture() { return {{0, 0, 0}, {1, 0, 0}, {99, 0, 0}, {100, 0, 0}}; }
using Coordinate = mhgp9::gen::Coordinate;

// The u16 corpus is a pinned recipe (same clouds, same floors); only the cast
// type follows the engine's Coordinate. Its 65535/32767 sites are interior
// points of the 18-bit domain and no longer exercise any bound.
std::vector<Points> small_fixtures() {
  std::vector<Points> result{{{7, 8, 9}}, {{0, 0, 0}, {65535, 65535, 65535}}, axis_fixture()};
  Points cube, shell, rows, random;
  for (unsigned bits = 0; bits < 8; ++bits) {
    cube.push_back({static_cast<Coordinate>((bits & 1U) * 65535),
                    static_cast<Coordinate>(((bits >> 1U) & 1U) * 65535),
                    static_cast<Coordinate>(((bits >> 2U) & 1U) * 65535)});
  }
  cube.push_back({32768, 32767, 32768});
  for (int x = -5; x <= 5; ++x) for (int y = -5; y <= 5; ++y) for (int z = -5; z <= 5; ++z) {
    if (x * x + y * y + z * z == 25)
      shell.push_back({static_cast<Coordinate>(x + 8), static_cast<Coordinate>(y + 8),
                       static_cast<Coordinate>(z + 8)});
  }
  for (unsigned side = 0; side < 2; ++side) for (unsigned y = 0; y < 8; ++y)
    rows.push_back({static_cast<Coordinate>(1000 + side * 59000), static_cast<Coordinate>(y), 0});
  std::uint32_t state = 1749;
  for (unsigned i = 0; i < 13; ++i) {
    state = state * 1664525U + 1013904223U;
    const auto y = static_cast<Coordinate>(state >> 16U);
    state = state * 1664525U + 1013904223U;
    random.push_back({static_cast<Coordinate>(i * 4093), y, static_cast<Coordinate>(state >> 16U)});
  }
  result.push_back(cube); result.push_back(shell); result.push_back(rows); result.push_back(random);
  return result;
}

// 18-bit twins of the extreme fixtures, graved at the engine limit 262143:
// the space diagonal, the eight corners with the near-center site and a
// random cloud whose own recipe draws 18 bits (>> 14U, separate seed). They
// pass the same oracle, work-identity and metadata checks as the u16 corpus.
static_assert(mhgp9::gen::coordinate_limit == 262143, "18-bit fixtures are graved at the engine limit");
std::vector<Points> small_fixtures18() {
  std::vector<Points> result{{{0, 0, 0}, {262143, 262143, 262143}}};
  Points cube, random;
  for (unsigned bits = 0; bits < 8; ++bits) {
    cube.push_back({static_cast<Coordinate>((bits & 1U) * 262143),
                    static_cast<Coordinate>(((bits >> 1U) & 1U) * 262143),
                    static_cast<Coordinate>(((bits >> 2U) & 1U) * 262143)});
  }
  cube.push_back({131072, 131071, 131072});
  std::uint32_t state = 2749;
  for (unsigned i = 0; i < 13; ++i) {
    state = state * 1664525U + 1013904223U;
    const auto y = static_cast<Coordinate>(state >> 14U);
    state = state * 1664525U + 1013904223U;
    random.push_back({static_cast<Coordinate>(i * 16381), y, static_cast<Coordinate>(state >> 14U)});
  }
  result.push_back(cube); result.push_back(random);
  return result;
}

void small_corpus(Gate& gate, const std::vector<Points>& fixtures, bool wide) {
  gate.wide = wide;
  for (std::size_t cloud_id = 0; cloud_id < fixtures.size(); ++cloud_id) {
    const auto& points = fixtures[cloud_id];
    const auto all = oracle(gate, points);
    const auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
    unsigned sample = 0;
    for (const unsigned k : {1U, 5U, 10U}) for (const unsigned s : {8U, 10U, 12U})
      for (const auto front : {WspdFrontMode::Pure, WspdFrontMode::MidpointSamples}) {
        Options options;
        options.k = k; options.s = s; options.front = front;
        options.pool = std::array<std::size_t, 3>{0, 2, 64}[(sample + cloud_id) % 3];
        options.order = sample % 2 == 0 ? Q2WitnessOrder::GlobalDfs : Q2WitnessOrder::ComplementFirst;
        options.sibling = sample % 3 == 0 ? Q2SiblingMode::Disabled : Q2SiblingMode::Saturating;
        const auto reference = mono(gate, *index, all, options);
        static_cast<void>(parallel(gate, index, all, options, reference, 1, 1));
        static_cast<void>(parallel(gate, index, all, options, reference, 2, 4));
        static_cast<void>(parallel(gate, index, all, options, reference, 4, 32));
        ++sample;
      }
    // Orthogonal traversal variants only on three small clouds; avoid a
    // blind Cartesian product over threads, granularity and every mode.
    if (cloud_id == 2 || cloud_id == 3 || cloud_id == 5) {
      for (unsigned variant = 0; variant < 6; ++variant) {
        Options options;
        options.k = variant % 2 == 0 ? 5 : 10;
        options.s = 10;
        options.front = variant % 2 == 0 ? WspdFrontMode::Pure : WspdFrontMode::MidpointSamples;
        options.pool = std::array<std::size_t, 3>{0, 2, 64}[variant % 3];
        if (variant < 2) {
          options.census = Q2CensusMode::Pairwise;
          options.order = Q2WitnessOrder::GlobalDfs;
          options.sibling = Q2SiblingMode::Disabled;
        } else {
          options.anchor = variant < 4 ? Q2AnchorMode::SharedProduct : Q2AnchorMode::SharedAnchors;
          options.order = variant % 2 == 0 ? Q2WitnessOrder::GlobalDfs : Q2WitnessOrder::ComplementFirst;
        }
        const auto reference = mono(gate, *index, all, options);
        static_cast<void>(parallel(gate, index, all, options, reference, 2, 1));
        static_cast<void>(parallel(gate, index, all, options, reference, 4, 4));
      }
    }
    ++(wide ? gate.clouds18 : gate.clouds);
  }
  gate.wide = false;
}

// Two 65-site clusters at the given offset: the u16 recipe (60000) and its
// 18-bit twin whose far cluster ends exactly at the limit (262079 + 64).
void clusters_corpus(Gate& gate, unsigned far_offset, bool wide) {
  gate.wide = wide;
  Points clusters;
  for (unsigned side = 0; side < 2; ++side) for (unsigned i = 0; i < 65; ++i)
    clusters.push_back({static_cast<Coordinate>(side * far_offset + i), 17, 31});
  const auto all = oracle(gate, clusters);
  const auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(clusters));
  for (const unsigned k : {1U, 10U}) for (const auto front : {WspdFrontMode::Pure, WspdFrontMode::MidpointSamples}) {
    Options options;
    options.k = k; options.front = front; options.pool = 64;
    const auto reference = mono(gate, *index, all, options);
    gate.require(reference.result.pool_work.selected_rectangles > 0 && reference.result.pool_work.filtered_pairs > 0 &&
                     reference.result.pool_work.pair_roots > 0,
                 "130-site threshold64 fixture stopped exercising effective Pool and residual census");
    for (const std::size_t workers : {std::size_t{1}, std::size_t{2}, std::size_t{4}})
      static_cast<void>(parallel(gate, index, all, options, reference, workers, 4));
  }
  ++(wide ? gate.clouds18 : gate.clouds);
  gate.wide = false;
}

void corpus(Gate& gate) {
  small_corpus(gate, small_fixtures(), false);
  clusters_corpus(gate, 60000, false);
  small_corpus(gate, small_fixtures18(), true);
  clusters_corpus(gate, 262079, true);
  const auto points = axis_fixture();
  const auto axis_all = oracle(gate, points);
  const auto axis_index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
  Options options;
  options.k = 1; options.pool = 2; options.front = WspdFrontMode::Pure;
  const auto reference = mono(gate, *axis_index, axis_all, options);
  for (const std::size_t workers : {std::size_t{1}, std::size_t{2}, std::size_t{4}})
    for (const std::size_t granularity : {std::size_t{1}, std::size_t{4}, std::size_t{32}})
      static_cast<void>(parallel(gate, axis_index, axis_all, options, reference, workers, granularity));
  const Points pair{{0, 0, 0}, {1, 0, 0}};
  const auto pair_all = oracle(gate, pair);
  const auto pair_index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(pair));
  const auto pair_reference = mono(gate, *pair_index, pair_all, options);
  const auto excess = parallel(gate, pair_index, pair_all, options, pair_reference, 8, 32);
  gate.require(excess.result.jobs == 1 && excess.result.started_workers == 1,
               "eight requested workers did not collapse to the sole terminal pair job");
}

struct CallbackFailure {};

struct Active {
  std::atomic<unsigned>& count;
  explicit Active(std::atomic<unsigned>& value) : count(value) { count.fetch_add(1); }
  ~Active() { count.fetch_sub(1); }
};

void callbacks_and_ownership(Gate& gate) {
  const auto points = axis_fixture();
  const auto all = oracle(gate, points);
  auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
  Options options;
  options.k = 1; options.front = WspdFrontMode::Pure; options.pool = 2;
  const auto reference = mono(gate, *index, all, options);
  gate.require(reference.result.pool_work.filtered_pairs == 3 && reference.result.pool_work.pair_roots == 1,
               "Pool callback fixture no longer has precisely the selected survivor {1,2}");
  std::atomic<unsigned> active{0}, emitted{0}, selected{0};
  std::vector<Q2CensusConsumer> throwing(4, [&](const mhgp9::gen::Q2Support& source) {
    Active guard(active);
    emitted.fetch_add(1);
    if (Pair{std::min(source.a_id, source.b_id), std::max(source.a_id, source.b_id)} == Pair{1, 2}) {
      selected.fetch_add(1);
      throw CallbackFailure{};
    }
  });
  bool caught = false;
  try {
    static_cast<void>(mhgp9::gen::run_wspd_q2_census_parallel(index, 1, 8, WspdFrontMode::Pure,
        Q2CensusMode::SharedBlocks, throwing, 4, options.sibling, options.order, options.anchor, 2));
  } catch (const CallbackFailure&) { caught = true; }
  gate.require(caught && emitted.load() > 0 && selected.load() == 1 && active.load() == 0,
               "effective-Pool callback failure escaped propagation/join or failed to reach selected work");
  ++gate.callback_failures;
  static_cast<void>(parallel(gate, index, all, options, reference, 4, 4));

  const std::weak_ptr<const mhgp9::gen::Q2CensusIndex> weak = index;
  std::vector<Output> slots(4), nested_slots(2);
  std::atomic<unsigned> nested_calls{0};
  std::atomic<bool> nested_entered{false};
  std::vector<Q2CensusConsumer> callbacks;
  for (std::size_t slot = 0; slot < slots.size(); ++slot) callbacks.emplace_back([&, slot](const mhgp9::gen::Q2Support& source) {
    const auto before = copy_support(source);
    if (before.pair == Pair{1, 2}) {
      bool expected = false;
      if (!nested_entered.compare_exchange_strong(expected, true))
        throw std::runtime_error("parallel producer repeated the unique reentrant support");
      index.reset();  // Only this unique support writes the caller handle.
      const auto nested_owner = weak.lock();
      if (!nested_owner) throw std::runtime_error("outer parallel call did not own its exact index");
      std::vector<Q2CensusConsumer> nested_consumers;
      for (std::size_t i = 0; i < nested_slots.size(); ++i)
        nested_consumers.emplace_back([&, i](const mhgp9::gen::Q2Support& item) { nested_slots[i].push_back(copy_support(item)); });
      static_cast<void>(mhgp9::gen::run_wspd_q2_census_parallel(nested_owner, 5, 10, WspdFrontMode::MidpointSamples,
          Q2CensusMode::SharedBlocks, nested_consumers, 4, Q2SiblingMode::Saturating,
          Q2WitnessOrder::ComplementFirst, Q2AnchorMode::SharedAnchors, 2));
      if (!(before == copy_support(source))) throw std::runtime_error("nested engine invalidated outer payload spans");
      nested_calls.fetch_add(1);
    }
    slots[slot].push_back(copy_support(source));
  });
  const auto outer = mhgp9::gen::run_wspd_q2_census_parallel(index, 1, 8, WspdFrontMode::Pure,
      Q2CensusMode::SharedBlocks, callbacks, 4, options.sibling, options.order, options.anchor, 2);
  Output outer_output, nested_output;
  for (const auto& slot : slots) outer_output.insert(outer_output.end(), slot.begin(), slot.end());
  for (const auto& slot : nested_slots) nested_output.insert(nested_output.end(), slot.begin(), slot.end());
  check_output(gate, outer_output, admitted(all, 1), points.size(), 1);
  check_output(gate, nested_output, admitted(all, 5), points.size(), 5);
  gate.require(nested_calls.load() == 1 && !index && weak.expired() && same_work(outer, reference.result),
               "nested parallel Pool call/reset changed work, leaked index ownership, or never ran");
  ++gate.reentrant_runs;
  ++gate.ownership_resets;
}

void joined_workers(Gate& gate) {
  using mhgp9::gen::parallel_detail::run_joined_workers;
  unsigned calls = 0, launches = 0;
  const auto forbidden_launcher = [&](auto&&) -> std::thread {
    ++launches;
    throw std::runtime_error("inline/empty helper launched a thread");
  };
  run_joined_workers(0, [&](std::size_t, const std::atomic<bool>&) { ++calls; }, forbidden_launcher);
  gate.require(calls == 0 && launches == 0, "zero workers performed work or launched a thread");
  const auto caller = std::this_thread::get_id();
  run_joined_workers(1, [&](std::size_t slot, const std::atomic<bool>& cancel) {
    if (slot != 0 || cancel.load() || std::this_thread::get_id() != caller)
      throw std::runtime_error("single-worker helper is not synchronous inline work");
    ++calls;
  }, forbidden_launcher);
  gate.require(calls == 1 && launches == 0, "one-worker helper launched or lost its inline callback");
  gate.joined_checks += 2;

  struct LaunchFailure {};
  std::atomic<unsigned> active{0}, finished{0};
  std::latch entered(1);
  unsigned attempts = 0;
  const auto fail_second = [&](auto&& function) -> std::thread {
    if (++attempts == 2) {
      entered.wait();  // First worker is demonstrably alive before failure.
      throw LaunchFailure{};
    }
    return std::thread(std::forward<decltype(function)>(function));
  };
  bool caught = false;
  try {
    run_joined_workers(4, [&](std::size_t, const std::atomic<bool>& cancel) {
      Active guard(active);
      entered.count_down();
      while (!cancel.load(std::memory_order_relaxed)) std::this_thread::yield();
      finished.fetch_add(1);
    }, fail_second);
  } catch (const LaunchFailure&) { caught = true; }
  gate.require(caught && attempts == 2 && finished.load() == 1 && active.load() == 0,
               "partial thread-launch failure returned without cancelling/joining the launched worker");
  ++gate.launch_failures;
  ++gate.joined_checks;

  active.store(0); finished.store(0);
  std::latch together(4);
  caught = false;
  try {
    run_joined_workers(4, [&](std::size_t slot, const std::atomic<bool>& cancel) {
      Active guard(active);
      together.count_down(); together.wait();
      if (slot == 0) throw CallbackFailure{};
      while (!cancel.load(std::memory_order_relaxed)) std::this_thread::yield();
      finished.fetch_add(1);
    });
  } catch (const CallbackFailure&) { caught = true; }
  gate.require(caught && active.load() == 0 && finished.load() == 3,
               "worker exception was rethrown before its other three active workers joined");
  ++gate.joined_checks;
  caught = false;
  try {
    run_joined_workers(1, [&](std::size_t, const std::atomic<bool>&) { throw CallbackFailure{}; }, forbidden_launcher);
  } catch (const CallbackFailure&) { caught = true; }
  gate.require(caught && launches == 0, "inline worker exception was lost or unexpectedly launched");
  ++gate.joined_checks;
}

void invalid_and_mutants(Gate& gate) {
  const auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(axis_fixture()));
  std::atomic<unsigned> emitted{0};
  const Q2CensusConsumer consumer = [&](const mhgp9::gen::Q2Support&) { emitted.fetch_add(1); };
  const std::vector<Q2CensusConsumer> consumers(2, consumer);
  const auto invoke = [&](const Options& options, std::span<const Q2CensusConsumer> callbacks, std::size_t granularity) {
    return mhgp9::gen::run_wspd_q2_census_parallel(index, options.k, options.s, options.front, options.census,
        callbacks, granularity, options.sibling, options.order, options.anchor, options.pool);
  };
  Options options;
  options.pool = 1;  // Invalid options still fail if Pool would select everything.
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_wspd_q2_census_parallel({}, 5, 8, options.front, options.census, consumers)); },
               "parallel entry accepted null owner");
  gate.rejects([&] { static_cast<void>(invoke(options, {}, 4)); }, "parallel entry accepted no consumers");
  auto empty_last = consumers; empty_last.back() = {};
  gate.rejects([&] { static_cast<void>(invoke(options, empty_last, 4)); }, "parallel entry accepted an empty later slot");
  gate.rejects([&] { static_cast<void>(invoke(options, consumers, 0)); }, "parallel entry accepted zero granularity");
  gate.rejects<std::overflow_error>([&] { static_cast<void>(invoke(options, consumers, std::numeric_limits<std::size_t>::max())); },
                                    "parallel target multiplication wrapped around");
  for (unsigned mutation = 0; mutation < 12; ++mutation) {
    auto changed = options;
    switch (mutation) {
      case 0: changed.k = 0; break;
      case 1: changed.k = 11; break;
      case 2: changed.s = 0; break;
      case 3: changed.front = static_cast<WspdFrontMode>(99); break;
      case 4: changed.census = static_cast<Q2CensusMode>(99); break;
      case 5: changed.sibling = static_cast<Q2SiblingMode>(99); break;
      case 6: changed.order = static_cast<Q2WitnessOrder>(99); break;
      case 7: changed.anchor = static_cast<Q2AnchorMode>(99); break;
      default:
        changed.census = Q2CensusMode::Pairwise;
        changed.sibling = Q2SiblingMode::Disabled;
        changed.order = Q2WitnessOrder::GlobalDfs;
        if (mutation == 8) changed.sibling = Q2SiblingMode::Saturating;
        if (mutation == 9) changed.order = Q2WitnessOrder::ComplementFirst;
        if (mutation == 10) changed.anchor = Q2AnchorMode::SharedProduct;
        if (mutation == 11) changed.anchor = Q2AnchorMode::SharedAnchors;
    }
    gate.rejects([&] { static_cast<void>(invoke(changed, consumers, 4)); }, "parallel entry accepted invalid traversal options");
  }
  gate.require(emitted.load() == 0, "invalid parallel options emitted before complete validation");

  const auto overflow = [&]<class Work>(u64 Work::* field) {
    Work a{}, b{};
    a.*field = std::numeric_limits<u64>::max(); b.*field = 1;
    gate.rejects<std::overflow_error>([&] { mhgp9::gen::parallel_detail::merge_work(a, b); },
                                      "work reduction silently wrapped an additive u64 field");
  };
  overflow(&mhgp9::gen::WspdFrontWork::product_visits);
  overflow(&mhgp9::gen::Q2CensusWork::payload_supports);
  overflow(&mhgp9::gen::Q2SiblingWork::rejected_after_credit);
  overflow(&mhgp9::gen::Q2OrderWork::phase_switches);
  overflow(&mhgp9::gen::Q2JointWork::accepted_pairs);
  overflow(&mhgp9::gen::Q2PoolWork::passthrough_anchors);
  mhgp9::gen::WspdFrontWork a{}, b{};
  a.residual_pair_mass[2] = std::numeric_limits<u64>::max(); b.residual_pair_mass[2] = 1;
  gate.rejects<std::overflow_error>([&] { mhgp9::gen::parallel_detail::merge_work(a, b); }, "lane reduction silently wrapped");
  a = {}; b = {};
  a.max_stack_size = 7; b.max_stack_size = 5;
  mhgp9::gen::parallel_detail::merge_work(a, b);
  gate.require(a.max_stack_size == 7, "logical DFS stack maxima were added instead of maximized");

  const auto all = oracle(gate, axis_fixture());
  options.k = 1; options.pool = 2; options.front = WspdFrontMode::Pure;
  const auto reference = mono(gate, *index, all, options);
  const auto actual = parallel(gate, index, all, options, reference, 2, 4);
  auto altered_output = actual.output;
  altered_output.front().shell.pop_back();
  gate.require(altered_output != reference.output, "truncated shell model escaped complete-payload equality");
  altered_output = actual.output; altered_output.push_back(altered_output.front()); sort_output(altered_output);
  gate.require(altered_output != reference.output, "duplicate support model escaped multiset equality");
  auto altered = actual.result;
  ++altered.front.work.max_stack_size;
  gate.require(!same_work(altered, reference.result), "logical maximum mutant escaped full work equality");
  altered = actual.result; ++altered.census_work.count_node_visits;
  gate.require(!same_work(altered, reference.result), "census work mutant escaped full equality");
  altered = actual.result; ++altered.pool_work.factor_sites;
  gate.require(!same_work(altered, reference.result), "Pool preparation mutant escaped integer work equality");
  altered = actual.result; ++altered.front.total_unordered_pairs;
  gate.require(!same_work(altered, reference.result), "summed global-domain mutant escaped work equality");
  gate.mutants += 6;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp9_gen_wspd_q2_parallel_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    corpus(gate);
    callbacks_and_ownership(gate);
    joined_workers(gate);
    invalid_and_mutants(gate);
    gate.require(gate.clouds == 8 && gate.oracle_sites > 1000000 && gate.mono_runs >= 140 &&
                     gate.parallel_runs >= 400 && gate.parallel_runs < 500 && gate.supports > 1000 &&
                     gate.max_shell == 30 && gate.empty_job_runs > 0 && gate.excess_worker_runs > 0 &&
                     gate.multi_worker_runs > 0 && gate.pool_selected > 0 && gate.pool_filtered > 0 &&
                     gate.pool_passthrough > 0 && gate.selected64 > 0 && gate.filtered64 > 0 &&
                     gate.joint_runs > 0 && gate.pairwise_runs > 0 && gate.sibling_tests > 0 &&
                     gate.structural_steps > 0 && gate.callback_failures == 1 && gate.reentrant_runs == 1 &&
                     gate.ownership_resets == 1 && gate.joined_checks == 5 && gate.launch_failures == 1 &&
                     gate.invalid_inputs >= 24 && gate.mutants == 6,
                 "parallel gate lost a declared non-vacuity floor");
    // Separate floors of the 18-bit twin corpus; max_shell18 is the full
    // shell of the 262143-cube diagonal as reported by the scalar oracle.
    gate.require(gate.clouds18 == 4 && gate.parallel_runs18 > 0 && gate.supports18 > 0 && gate.max_shell18 == 8 &&
                     !gate.wide,
                 "parallel gate lost an 18-bit twin fixture floor");
    std::cout << "{\"schema\":\"mhgp9_gen_wspd_q2_parallel_gate_v1\",\"status\":\"passed\","
              << "\"scope\":\"bounded_cpu_q2_jobs_not_full\",\"public_status\":\"not_claimed\","
              << "\"checks\":" << gate.checks << ",\"clouds\":" << gate.clouds
              << ",\"oracle_pairs\":" << gate.oracle_pairs << ",\"oracle_sites\":" << gate.oracle_sites
              << ",\"mono_runs\":" << gate.mono_runs << ",\"parallel_runs\":" << gate.parallel_runs
              << ",\"supports\":" << gate.supports << ",\"max_shell\":" << gate.max_shell
              << ",\"empty_job_runs\":" << gate.empty_job_runs << ",\"excess_worker_runs\":" << gate.excess_worker_runs
              << ",\"multi_worker_runs\":" << gate.multi_worker_runs << ",\"pool_selected\":" << gate.pool_selected
              << ",\"pool_filtered\":" << gate.pool_filtered << ",\"pool_passthrough\":" << gate.pool_passthrough
              << ",\"selected64\":" << gate.selected64 << ",\"filtered64\":" << gate.filtered64
              << ",\"joint_runs\":" << gate.joint_runs << ",\"pairwise_runs\":" << gate.pairwise_runs
              << ",\"sibling_tests\":" << gate.sibling_tests << ",\"structural_steps\":" << gate.structural_steps
              << ",\"callback_failures\":" << gate.callback_failures << ",\"reentrant_runs\":" << gate.reentrant_runs
              << ",\"ownership_resets\":" << gate.ownership_resets << ",\"joined_checks\":" << gate.joined_checks
              << ",\"launch_failures\":" << gate.launch_failures << ",\"invalid_inputs\":" << gate.invalid_inputs
              << ",\"mutants\":" << gate.mutants << ",\"clouds18\":" << gate.clouds18
              << ",\"parallel_runs18\":" << gate.parallel_runs18 << ",\"supports18\":" << gate.supports18
              << ",\"max_shell18\":" << gate.max_shell18 << "}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp9_gen_wspd_q2_parallel_gate failed: " << error.what() << '\n';
    return 1;
  }
}
