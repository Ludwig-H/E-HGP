#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "pipeline/wspd_q2_parallel.hpp"

namespace {

using mhgp8::Point3;
using mhgp8::Q2AnchorMode;
using mhgp8::Q2CensusMode;
using mhgp8::Q2SiblingMode;
using mhgp8::Q2WitnessOrder;
using mhgp8::WspdFrontMode;
using mhgp8::WspdQ2Schedule;
using mhgp8::WspdQ2ScheduleMode;
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
  u64 checks{}, clouds{}, oracle_pairs{}, oracle_sites{}, mono_runs{}, coarse_runs{}, donate_runs{}, default_runs{};
  u64 supports{}, max_shell{}, donations{}, full_refusals{}, empty_runs{}, selected64{}, filtered64{}, passthrough{};
  u64 joint_runs{}, pairwise_runs{}, callback_failures{}, reentrant_runs{}, index_resets{}, invalid_inputs{}, mutants{};
  void require(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
  }
  template<class Exception = std::invalid_argument, class Function>
  void rejects(Function&& function, const char* message) {
    bool caught = false;
    try { function(); } catch (const Exception&) { caught = true; }
    require(caught, message); ++invalid_inputs;
  }
};

Support copy(const mhgp8::Q2Support& item) {
  Support result{std::min(item.a_id, item.b_id), std::max(item.a_id, item.b_id), item.key,
                 {item.interior.begin(), item.interior.end()}, {item.shell.begin(), item.shell.end()}};
  std::sort(result.interior.begin(), result.interior.end());
  std::sort(result.shell.begin(), result.shell.end());
  return result;
}

void sort(Output& output) {
  std::sort(output.begin(), output.end(), [](const auto& a, const auto& b) {
    return a.a < b.a || (a.a == b.a && a.b < b.b);
  });
}

// Independent test-only scalar all-pairs/all-sites oracle. Its dot-product
// formula and ball key never call product predicates, census or front code.
// Every subtraction is promoted; |H| <= 3*65535^2 fits signed int64_t.
Output oracle(Gate& gate, const Points& points) {
  gate.require(!points.empty() && points.size() <= 140, "dynamic oracle exceeded n<=140");
  Output result;
  for (std::size_t a = 0; a < points.size(); ++a) for (std::size_t b = a + 1; b < points.size(); ++b) {
    Support item; item.a = a; item.b = b;
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
    result.push_back(std::move(item)); ++gate.oracle_pairs;
  }
  return result;
}

Output accepted(const Output& all, unsigned k) {
  Output result;
  for (const auto& item : all) if (item.interior.size() < k) result.push_back(item);
  return result;
}

void check_output(Gate& gate, Output& output, const Output& expected) {
  sort(output);
  gate.require(output == expected, "dynamic key/interior/full-shell/support multiset differs from the independent oracle");
  for (const auto& item : output) gate.max_shell = std::max(gate.max_shell, static_cast<u64>(item.shell.size()));
  gate.supports += output.size();
}

struct Options {
  unsigned k{5}, s{8};
  WspdFrontMode front{WspdFrontMode::MidpointSamples};
  Q2CensusMode census{Q2CensusMode::SharedBlocks};
  Q2SiblingMode sibling{Q2SiblingMode::Saturating};
  Q2WitnessOrder order{Q2WitnessOrder::ComplementFirst};
  Q2AnchorMode anchor{Q2AnchorMode::Individual};
  std::size_t pool{};
};

mhgp8::Q2PoolWork integers(mhgp8::Q2PoolWork work) {
  work.preparation_ms = 0; work.selected_total_ms = 0;
  return work;
}

bool same_work(const mhgp8::WspdQ2ParallelResult& a, const mhgp8::WspdQ2CensusResult& b) {
  return a.front.work == b.front.work && a.front.total_unordered_pairs == b.front.total_unordered_pairs &&
         a.front.active_lane_mask == b.front.active_lane_mask && a.census_work == b.census.work &&
         a.sibling_work == b.sibling_work && a.order_work == b.order_work && a.joint_work == b.joint_work &&
         integers(a.pool_work) == integers(b.pool_work) && a.input_rectangles == b.input_rectangles &&
         a.anchor_queries == b.anchor_queries && a.candidate_pairs == b.census.candidate_pairs &&
         a.accepted_pairs == b.census.accepted_pairs && a.rejected_pairs == b.census.rejected_pairs;
}

struct Mono { mhgp8::WspdQ2CensusResult result; Output output; };

Mono mono(Gate& gate, const mhgp8::Q2CensusIndex& index, const Output& expected, const Options& options) {
  Mono result;
  result.result = mhgp8::run_wspd_q2_census(index, options.k, options.s, options.front, options.census,
      [&](const mhgp8::Q2Support& item) { result.output.push_back(copy(item)); },
      options.sibling, options.order, options.anchor, options.pool);
  check_output(gate, result.output, expected); ++gate.mono_runs;
  return result;
}

void add_dispatch(mhgp8::WspdFrontDispatchWork& total, const mhgp8::WspdFrontDispatchWork& part) {
  using Work = mhgp8::WspdFrontDispatchWork;
  constexpr std::array<u64 Work::*, 12> fields{
      &Work::seeds_started, &Work::seeds_completed, &Work::donations, &Work::donor_checks,
      &Work::offer_attempts, &Work::offer_full, &Work::offer_busy, &Work::offer_no_demand,
      &Work::stolen_started, &Work::stolen_completed, &Work::waits, &Work::wakes};
  for (const auto field : fields) total.*field += part.*field;
  total.max_queue_size = std::max(total.max_queue_size, part.max_queue_size);
  total.max_local_stack_size = std::max(total.max_local_stack_size, part.max_local_stack_size);
}

mhgp8::WspdQ2ParallelResult run(Gate& gate, const mhgp8::Q2CensusIndexPtr& index,
                               const Mono& baseline, const Options& options, WspdQ2Schedule schedule,
                               std::size_t workers, std::size_t granularity, bool implicit = false) {
  // Slot vectors exist before launches; workers never access Gate or the
  // storage of another consumer. Deep copies pay for borrowed payloads.
  std::vector<Output> slots(workers);
  std::vector<mhgp8::Q2CensusConsumer> consumers;
  for (std::size_t slot = 0; slot < workers; ++slot)
    consumers.emplace_back([&, slot](const mhgp8::Q2Support& item) { slots[slot].push_back(copy(item)); });
  const auto result = implicit
      ? mhgp8::run_wspd_q2_census_parallel(index, options.k, options.s, options.front, options.census,
          consumers, granularity, options.sibling, options.order, options.anchor, options.pool)
      : mhgp8::run_wspd_q2_census_parallel(index, options.k, options.s, options.front, options.census,
          consumers, granularity, options.sibling, options.order, options.anchor, options.pool, schedule);
  Output output;
  for (const auto& slot : slots) output.insert(output.end(), slot.begin(), slot.end());
  check_output(gate, output, baseline.output);
  gate.require(same_work(result, baseline.result), "mono/coarse/donate changed any geometric work field, bin, mass or maximum");
  gate.require(result.requested_workers == workers && result.target_jobs == workers * granularity &&
                   result.started_workers == std::min<u64>(workers, result.jobs) && result.completed_jobs == result.jobs &&
                   result.workers.size() == result.started_workers,
               "dynamic scheduler changed initial-seed metadata or failed completion");
  mhgp8::WspdFrontDispatchWork dispatch;
  u64 jobs = 0, products = result.prefix_product_visits, rectangles = 0, visits = 0, supports = 0, peak_sum = 0, peak_max = 0;
  for (std::size_t slot = 0; slot < result.workers.size(); ++slot) {
    const auto& item = result.workers[slot];
    add_dispatch(dispatch, item.dispatch_work);
    jobs += item.jobs; products += item.front_products; rectangles += item.input_rectangles;
    visits += item.count_node_visits; supports += item.supports;
    peak_sum += item.pool_peak_bytes; peak_max = std::max(peak_max, item.pool_peak_bytes);
    gate.require(item.supports == slots[slot].size(), "dynamic worker used another consumer slot");
    if (schedule.mode == WspdQ2ScheduleMode::Donate && !implicit)
      gate.require(item.jobs == item.dispatch_work.seeds_completed, "worker jobs silently became donated fragments");
  }
  gate.require(jobs == result.jobs && products == result.front.work.product_visits && rectangles == result.input_rectangles &&
                   visits == result.census_work.count_node_visits && supports == result.accepted_pairs &&
                   peak_sum == result.pool_peak_bytes_sum && peak_max == result.pool_work.plan_peak_bytes &&
                   dispatch == result.dispatch_work, "dynamic per-worker reductions lost a work unit or maximum");
  for (const auto value : {result.total_ms, result.partition_ms, result.worker_ms_sum, result.payload_ms_sum,
                            result.pool_work.preparation_ms, result.pool_work.selected_total_ms})
    gate.require(std::isfinite(value) && value >= 0, "dynamic timing is negative or nonfinite");
  if (schedule.mode == WspdQ2ScheduleMode::Donate && !implicit) {
    gate.require(dispatch.seeds_started == result.jobs && dispatch.seeds_completed == result.jobs &&
                     dispatch.donations == dispatch.stolen_started && dispatch.donations == dispatch.stolen_completed &&
                     dispatch.donor_checks == dispatch.offer_no_demand + dispatch.offer_attempts &&
                     dispatch.offer_attempts == dispatch.donations + dispatch.offer_full + dispatch.offer_busy &&
                     dispatch.waits == dispatch.wakes && dispatch.max_queue_size <= schedule.queue_capacity &&
                     dispatch.max_local_stack_size <= 97 && (result.started_workers == 0 || result.queue_storage_bytes > 0),
                 "dynamic dispatcher accounting or physical queue/stack bound failed");
    if (result.started_workers <= 1)
      gate.require(dispatch.donor_checks == 0 && dispatch.donations == 0,
                   "one-worker fast path polled or donated work");
    gate.donations += dispatch.donations; gate.full_refusals += dispatch.offer_full; ++gate.donate_runs;
  } else {
    gate.require(dispatch == mhgp8::WspdFrontDispatchWork{} && result.queue_storage_bytes == 0,
                 "default/coarse mode allocated or used the dynamic dispatcher");
    ++gate.coarse_runs;
  }
  gate.default_runs += static_cast<u64>(implicit);
  gate.empty_runs += static_cast<u64>(result.jobs == 0);
  gate.passthrough += result.pool_work.passthrough_rectangles;
  if (options.pool == 64) { gate.selected64 += result.pool_work.selected_rectangles; gate.filtered64 += result.pool_work.filtered_pairs; }
  gate.joint_runs += static_cast<u64>(options.anchor != Q2AnchorMode::Individual);
  gate.pairwise_runs += static_cast<u64>(options.census == Q2CensusMode::Pairwise);
  return result;
}

Points axis() { return {{0, 0, 0}, {1, 0, 0}, {99, 0, 0}, {100, 0, 0}}; }

std::vector<Points> fixtures() {
  std::vector<Points> result{{{7, 8, 9}}, {{0, 0, 0}, {65535, 65535, 65535}}, axis()};
  Points shell, cube, rows, random, clusters;
  for (int x = -5; x <= 5; ++x) for (int y = -5; y <= 5; ++y) for (int z = -5; z <= 5; ++z)
    if (x * x + y * y + z * z == 25)
      shell.push_back({static_cast<std::uint16_t>(x + 8), static_cast<std::uint16_t>(y + 8), static_cast<std::uint16_t>(z + 8)});
  for (unsigned bits = 0; bits < 8; ++bits)
    cube.push_back({static_cast<std::uint16_t>((bits & 1U) * 65535), static_cast<std::uint16_t>(((bits >> 1U) & 1U) * 65535),
                    static_cast<std::uint16_t>(((bits >> 2U) & 1U) * 65535)});
  cube.push_back({32767, 32768, 32767});
  for (unsigned side = 0; side < 2; ++side) for (unsigned i = 0; i < 8; ++i)
    rows.push_back({static_cast<std::uint16_t>(1000 + side * 59000), static_cast<std::uint16_t>(i), 0});
  std::uint32_t state = 971;
  for (unsigned i = 0; i < 13; ++i) {
    state = state * 1664525U + 1013904223U; const auto y = static_cast<std::uint16_t>(state >> 16U);
    state = state * 1664525U + 1013904223U;
    random.push_back({static_cast<std::uint16_t>(i * 4093), y, static_cast<std::uint16_t>(state >> 16U)});
  }
  for (unsigned side = 0; side < 2; ++side) for (unsigned i = 0; i < 65; ++i)
    clusters.push_back({static_cast<std::uint16_t>(side * 60000 + i), 17, 31});
  result.push_back(shell); result.push_back(cube); result.push_back(rows); result.push_back(random); result.push_back(clusters);
  return result;
}

void corpus(Gate& gate) {
  const auto clouds = fixtures();
  for (std::size_t cloud = 0; cloud < clouds.size(); ++cloud) {
    const auto& points = clouds[cloud];
    const auto all = oracle(gate, points);
    const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
    unsigned variant = 0;
    for (const unsigned k : {1U, 5U, 10U}) for (const unsigned s : {8U, 10U, 12U}) {
      const unsigned choice = variant++;
      if (points.size() > 100 && choice != 0 && choice != 3 && choice != 8) continue;
      Options options;
      options.k = k; options.s = s;
      options.front = choice % 2 == 0 ? WspdFrontMode::Pure : WspdFrontMode::MidpointSamples;
      options.pool = points.size() > 100 ? 64 : std::array<std::size_t, 3>{0, 2, 64}[(choice + cloud) % 3];
      options.order = choice % 2 == 0 ? Q2WitnessOrder::GlobalDfs : Q2WitnessOrder::ComplementFirst;
      if (choice == 2) { options.census = Q2CensusMode::Pairwise; options.sibling = Q2SiblingMode::Disabled; options.order = Q2WitnessOrder::GlobalDfs; }
      if (choice == 4) options.anchor = Q2AnchorMode::SharedProduct;
      if (choice == 8) options.anchor = Q2AnchorMode::SharedAnchors;
      const auto baseline = mono(gate, *index, accepted(all, k), options);
      static_cast<void>(run(gate, index, baseline, options, {WspdQ2ScheduleMode::Coarse, 1, 1}, 2, 1));
      static_cast<void>(run(gate, index, baseline, options, {WspdQ2ScheduleMode::Donate, 1, 1}, 2, 1));
      static_cast<void>(run(gate, index, baseline, options, {WspdQ2ScheduleMode::Donate, 7, 4}, 4, 4));
      if (choice == 0) {
        static_cast<void>(run(gate, index, baseline, options, {}, 1, 1, true));
        static_cast<void>(run(gate, index, baseline, options, {WspdQ2ScheduleMode::Donate, 1, 1}, 1, 1));
      }
    }
    ++gate.clouds;
  }
}

struct CallbackFailure {};

void callbacks(Gate& gate) {
  const auto points = axis();
  const auto all = oracle(gate, points);
  auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
  Options options; options.k = 1; options.front = WspdFrontMode::Pure; options.pool = 2;
  const auto baseline = mono(gate, *index, accepted(all, 1), options);
  gate.require(baseline.result.pool_work.filtered_pairs == 3 && baseline.result.pool_work.pair_roots == 1,
               "dynamic exception fixture lost its effective Pool survivor {1,2}");
  const WspdQ2Schedule donate{WspdQ2ScheduleMode::Donate, 1, 1};
  std::atomic<unsigned> failed{0};
  std::vector<mhgp8::Q2CensusConsumer> throwing(4, [&](const mhgp8::Q2Support& item) {
    if (std::min(item.a_id, item.b_id) == 1 && std::max(item.a_id, item.b_id) == 2) {
      failed.fetch_add(1); throw CallbackFailure{};
    }
  });
  bool caught = false;
  try {
    static_cast<void>(mhgp8::run_wspd_q2_census_parallel(index, 1, 8, options.front, options.census, throwing,
        1, options.sibling, options.order, options.anchor, 2, donate));
  } catch (const CallbackFailure&) { caught = true; }
  gate.require(caught && failed.load() == 1, "Donate swallowed or never reached its effective-Pool callback exception");
  ++gate.callback_failures;
  static_cast<void>(run(gate, index, baseline, options, donate, 4, 1));

  const std::weak_ptr<const mhgp8::Q2CensusIndex> weak = index;
  std::vector<Output> outer_slots(4), inner_slots(2);
  std::atomic<bool> entered{false};
  std::atomic<unsigned> nested{0};
  std::vector<mhgp8::Q2CensusConsumer> consumers;
  for (std::size_t slot = 0; slot < outer_slots.size(); ++slot)
    consumers.emplace_back([&, slot](const mhgp8::Q2Support& item) {
      const auto before = copy(item);
      if (before.a == 1 && before.b == 2) {
        if (entered.exchange(true)) throw std::runtime_error("duplicate reentrant selected support");
        index.reset();
        const auto nested_owner = weak.lock();
        if (!nested_owner) throw std::runtime_error("Donate lost its shared index during callback");
        std::vector<mhgp8::Q2CensusConsumer> inner;
        for (std::size_t i = 0; i < inner_slots.size(); ++i)
          inner.emplace_back([&, i](const mhgp8::Q2Support& source) { inner_slots[i].push_back(copy(source)); });
        static_cast<void>(mhgp8::run_wspd_q2_census_parallel(nested_owner, 5, 10, WspdFrontMode::MidpointSamples,
            Q2CensusMode::SharedBlocks, inner, 1, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst,
            Q2AnchorMode::SharedAnchors, 2, donate));
        if (!(copy(item) == before)) throw std::runtime_error("nested Donate invalidated the outer borrowed payload");
        nested.fetch_add(1);
      }
      outer_slots[slot].push_back(copy(item));
    });
  const auto result = mhgp8::run_wspd_q2_census_parallel(index, 1, 8, options.front, options.census, consumers,
      1, options.sibling, options.order, options.anchor, 2, donate);
  Output outer, inner;
  for (const auto& slot : outer_slots) outer.insert(outer.end(), slot.begin(), slot.end());
  for (const auto& slot : inner_slots) inner.insert(inner.end(), slot.begin(), slot.end());
  check_output(gate, outer, baseline.output); check_output(gate, inner, accepted(all, 5));
  gate.require(nested.load() == 1 && !index && weak.expired() && same_work(result, baseline.result),
               "nested Donate/reset changed work, skipped nesting, or retained the external index");
  ++gate.reentrant_runs; ++gate.index_resets;
}

void invalids(Gate& gate) {
  const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(axis()));
  std::atomic<unsigned> emitted{0};
  const std::vector<mhgp8::Q2CensusConsumer> consumers(2, [&](const mhgp8::Q2Support&) { emitted.fetch_add(1); });
  for (const auto mode : {WspdQ2ScheduleMode::Coarse, WspdQ2ScheduleMode::Donate}) {
    for (unsigned invalid = 0; invalid < 3; ++invalid) {
      WspdQ2Schedule schedule{mode, 1, 1};
      if (invalid == 0) schedule.mode = static_cast<WspdQ2ScheduleMode>(99);
      if (invalid == 1) schedule.queue_capacity = 0;
      if (invalid == 2) schedule.donation_interval = 0;
      gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census_parallel(index, 1, 8, WspdFrontMode::Pure,
          Q2CensusMode::SharedBlocks, consumers, 1, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs,
          Q2AnchorMode::Individual, 1, schedule)); }, "parallel entry accepted invalid scheduling options");
    }
  }
  gate.rejects<std::overflow_error>([&] { static_cast<void>(mhgp8::run_wspd_q2_census_parallel(index, 1, 8,
      WspdFrontMode::Pure, Q2CensusMode::SharedBlocks, consumers, std::numeric_limits<std::size_t>::max(),
      Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, Q2AnchorMode::Individual, 1,
      {WspdQ2ScheduleMode::Donate, 1, 1})); }, "Donate target multiplication silently overflowed");
  auto empty = consumers; empty.back() = {};
  gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census_parallel(index, 1, 8, WspdFrontMode::Pure,
      Q2CensusMode::SharedBlocks, empty, 1, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs,
      Q2AnchorMode::Individual, 1, {WspdQ2ScheduleMode::Donate, 1, 1})); }, "Donate accepted an empty later callback slot");
  gate.require(emitted.load() == 0, "invalid schedule/options emitted before complete validation");

  const auto all = oracle(gate, axis());
  Options options; options.k = 1; options.front = WspdFrontMode::Pure; options.pool = 2;
  const auto baseline = mono(gate, *index, accepted(all, 1), options);
  const auto actual = run(gate, index, baseline, options, {WspdQ2ScheduleMode::Donate, 1, 1}, 2, 1);
  auto changed = actual; ++changed.front.work.max_stack_size;
  gate.require(!same_work(changed, baseline.result), "logical-stack mutant escaped exact work comparator");
  changed = actual; ++changed.census_work.count_node_visits;
  gate.require(!same_work(changed, baseline.result), "census visit mutant escaped exact work comparator");
  changed = actual; ++changed.pool_work.factor_sites;
  gate.require(!same_work(changed, baseline.result), "duplicated Pool preparation escaped exact work comparator");
  auto truncated = baseline.output; truncated.front().shell.pop_back();
  gate.require(truncated != baseline.output, "truncated full shell escaped payload comparator");
  gate.mutants += 4;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_wspd_q2_dynamic_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    corpus(gate); callbacks(gate); invalids(gate);
    gate.require(gate.clouds == 8 && gate.oracle_sites > 1000000 && gate.mono_runs >= 60 &&
                     gate.coarse_runs >= 60 && gate.donate_runs >= 130 && gate.default_runs == 8 &&
                     gate.supports > 1000 && gate.max_shell == 30 && gate.empty_runs > 0 && gate.selected64 > 0 &&
                     gate.filtered64 > 0 && gate.passthrough > 0 && gate.joint_runs > 0 && gate.pairwise_runs > 0 &&
                     gate.callback_failures == 1 && gate.reentrant_runs == 1 && gate.index_resets == 1 &&
                     gate.invalid_inputs == 8 && gate.mutants == 4,
                 "dynamic q2 gate lost a declared non-vacuity floor");
    // Donation/full-queue positives are deterministic in the companion
    // dispatch gate. Here scheduling counters are observations, not stable
    // signatures to demand from every run of a parallel scheduler.
    std::cout << "{\"schema\":\"mhgp8_wspd_q2_dynamic_gate_v1\",\"status\":\"passed\","
              << "\"public_status\":\"not_claimed\",\"checks\":" << gate.checks << ",\"clouds\":" << gate.clouds
              << ",\"oracle_pairs\":" << gate.oracle_pairs << ",\"oracle_sites\":" << gate.oracle_sites
              << ",\"mono_runs\":" << gate.mono_runs << ",\"coarse_runs\":" << gate.coarse_runs
              << ",\"donate_runs\":" << gate.donate_runs << ",\"default_runs\":" << gate.default_runs
              << ",\"supports\":" << gate.supports << ",\"max_shell\":" << gate.max_shell
              << ",\"donations\":" << gate.donations << ",\"full_refusals\":" << gate.full_refusals
              << ",\"empty_runs\":" << gate.empty_runs << ",\"selected64\":" << gate.selected64
              << ",\"filtered64\":" << gate.filtered64 << ",\"passthrough\":" << gate.passthrough
              << ",\"joint_runs\":" << gate.joint_runs << ",\"pairwise_runs\":" << gate.pairwise_runs
              << ",\"callback_failures\":" << gate.callback_failures << ",\"reentrant_runs\":" << gate.reentrant_runs
              << ",\"index_resets\":" << gate.index_resets << ",\"invalid_inputs\":" << gate.invalid_inputs
              << ",\"mutants\":" << gate.mutants << "}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_wspd_q2_dynamic_gate failed: " << error.what() << '\n';
    return 1;
  }
}
