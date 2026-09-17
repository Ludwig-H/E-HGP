#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
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
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

// Explicit bounded reuse of the independent H/Gram witness oracle.
#include "oracle/p0_oracle.hpp"
#include "parallel/joined_workers.hpp"
#include "parallel/work_reduction.hpp"
#include "wspd/front.hpp"

namespace {

using mhgp8::Point3;
using mhgp8::WspdFrontMode;
using mhgp8::WspdRectangle;
using mhgp8::u64;
using Points = std::vector<Point3>;
using Rectangle = std::tuple<std::size_t, std::size_t, std::uint8_t>;
using Rectangles = std::vector<Rectangle>;
using DispatchWork = mhgp8::WspdFrontDispatchWork;

struct Gate {
  u64 checks{}, oracle_sites{}, reference_runs{}, dispatch_runs{}, donations{}, full_refusals{};
  u64 reduced_rectangles{}, masked_cases{}, zero_seed_cases{}, ownership_cases{}, waits{};
  u64 cross_worker_transfers{}, callback_failures{}, launch_failures{}, invalid_inputs{};
  u64 widened_cases{}, extended_products{};
  void require(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
  }
  template<class Function> void rejects(Function&& function, const char* message) {
    bool caught = false;
    try { function(); } catch (const std::invalid_argument&) { caught = true; }
    require(caught, message); ++invalid_inputs;
  }
};

constexpr std::array<u64 DispatchWork::*, 12> sums{
    &DispatchWork::seeds_started, &DispatchWork::seeds_completed,
    &DispatchWork::donations, &DispatchWork::donor_checks, &DispatchWork::offer_attempts,
    &DispatchWork::offer_full, &DispatchWork::offer_busy, &DispatchWork::offer_no_demand,
    &DispatchWork::stolen_started, &DispatchWork::stolen_completed,
    &DispatchWork::waits, &DispatchWork::wakes};

void merge(DispatchWork& total, const DispatchWork& part) {
  for (const auto field : sums) total.*field += part.*field;
  total.max_queue_size = std::max(total.max_queue_size, part.max_queue_size);
  total.max_local_stack_size = std::max(total.max_local_stack_size, part.max_local_stack_size);
}

struct Oracle {
  std::size_t n{};
  std::array<std::vector<unsigned>, 3> counts;
};

Oracle oracle(Gate& gate, const Points& points) {
  Oracle result;
  result.n = points.size();
  gate.require(result.n > 0 && result.n <= 140, "front oracle escaped its bounded domain");
  for (auto& lane : result.counts) lane.resize(result.n * result.n);
  for (std::size_t a = 0; a < result.n; ++a) for (std::size_t b = a + 1; b < result.n; ++b)
    for (const auto& z : points) for (unsigned lane = 0; lane < 3; ++lane) {
      result.counts[lane][a * result.n + b] += static_cast<unsigned>(mhgp8::oracle::point_witness(
          static_cast<mhgp8::Lane>(lane + 2), points[a], points[b], z));
      ++gate.oracle_sites;
    }
  return result;
}

void check_cover(Gate& gate, const Rectangles& rectangles, const mhgp8::Q2CensusIndex& index,
                 const Oracle& expected, unsigned k, std::uint8_t mask, bool pure) {
  const auto active = static_cast<std::uint8_t>(mask & ((1U << std::min(k, 3U)) - 1U));
  std::array<std::vector<unsigned>, 3> covers;
  for (auto& lane : covers) lane.resize(expected.n * expected.n);
  const auto nodes = index.spatial_nodes();
  const auto order = index.spatial_order();
  for (const auto& [a, b, lanes] : rectangles) {
    gate.require(a < nodes.size() && b < nodes.size() && lanes != 0 && (lanes & active) == lanes,
                 "dispatcher emitted an invalid node or inactive lane");
    const auto ar = nodes[a].range, br = nodes[b].range;
    gate.require(ar.last <= br.first || br.last <= ar.first, "dispatcher emitted overlapping factors");
    gate.reduced_rectangles += static_cast<u64>(lanes != active);
    for (auto ai = ar.first; ai < ar.last; ++ai) for (auto bi = br.first; bi < br.last; ++bi) {
      const auto lo = std::min(order[ai], order[bi]), hi = std::max(order[ai], order[bi]);
      for (unsigned lane = 0; lane < 3; ++lane)
        if ((lanes & (1U << lane)) != 0) ++covers[lane][lo * expected.n + hi];
    }
  }
  for (unsigned lane = 0; lane < 3; ++lane) {
    const bool enabled = (active & (1U << lane)) != 0;
    for (std::size_t a = 0; a < expected.n; ++a) for (std::size_t b = a + 1; b < expected.n; ++b) {
      const auto id = a * expected.n + b;
      const auto count = covers[lane][id];
      gate.require(enabled ? count <= 1 : count == 0, "dispatcher duplicated a pair or reactivated an excluded lane");
      if (enabled) gate.require(count == 1 || (!pure && expected.counts[lane][id] >= k - lane),
                                "dispatcher lost or unsafely rejected an oracle lane pair");
    }
  }
}

struct Reference { mhgp8::WspdFrontResult result; Rectangles rectangles; };

Reference reference(Gate& gate, const mhgp8::Q2CensusIndex& index, unsigned k, unsigned s,
                    WspdFrontMode mode, std::uint8_t mask, mhgp8::WspdFrontProposals proposals = {}) {
  Reference result;
  result.result = mhgp8::run_wspd_front(index, k, s, mode, [&](const WspdRectangle& rectangle) {
    result.rectangles.emplace_back(rectangle.a_node, rectangle.b_node, rectangle.lane_mask);
  }, mask, proposals);
  std::sort(result.rectangles.begin(), result.rectangles.end());
  ++gate.reference_runs;
  return result;
}

DispatchWork run_case(Gate& gate, const mhgp8::Q2CensusIndexPtr& index, const Oracle& expected,
                      unsigned k, unsigned s, WspdFrontMode mode, std::uint8_t mask,
                      std::size_t target, std::size_t capacity, std::size_t interval,
                      std::size_t declared_workers, std::size_t actual_workers,
                      mhgp8::WspdFrontProposals proposals = {}) {
  const auto baseline = reference(gate, *index, k, s, mode, mask, proposals);
  gate.extended_products += baseline.result.work.extended_products;
  const auto plan = mhgp8::make_wspd_front_jobs(index, k, s, mode, target, mask, proposals);
  auto total = plan->prefix_result();
  const auto dispatch = plan->make_dispatch(capacity, interval, declared_workers);
  const auto retained = dispatch->retained_bytes();
  std::vector<Rectangles> slots(actual_workers);
  std::vector<mhgp8::WspdFrontDispatchResult> results(actual_workers);
  mhgp8::parallel_detail::run_joined_workers(actual_workers,
      [&](std::size_t slot, const std::atomic<bool>& cancel) {
        results[slot] = dispatch->run_worker([&, slot](const WspdRectangle& rectangle) {
          slots[slot].emplace_back(rectangle.a_node, rectangle.b_node, rectangle.lane_mask);
        }, cancel);
      }, mhgp8::parallel_detail::ThreadLauncher{}, [&]() noexcept { dispatch->cancel(); });
  Rectangles rectangles;
  DispatchWork work;
  for (std::size_t slot = 0; slot < actual_workers; ++slot) {
    gate.require(results[slot].front.total_unordered_pairs == total.total_unordered_pairs &&
                     results[slot].front.active_lane_mask == total.active_lane_mask,
                 "dispatcher changed global pair/lane metadata into local metadata");
    mhgp8::parallel_detail::merge_work(total.work, results[slot].front.work);
    merge(work, results[slot].work);
    rectangles.insert(rectangles.end(), slots[slot].begin(), slots[slot].end());
  }
  std::sort(rectangles.begin(), rectangles.end());
  gate.require(rectangles == baseline.rectangles && total.work == baseline.result.work &&
                   total.total_unordered_pairs == baseline.result.total_unordered_pairs &&
                   total.active_lane_mask == baseline.result.active_lane_mask,
               "dynamic front differs from mono rectangles, masks, all work fields or logical maxima");
  check_cover(gate, rectangles, *index, expected, k, mask, mode == WspdFrontMode::Pure);
  gate.require(work.seeds_started == plan->job_count() && work.seeds_completed == plan->job_count() &&
                   work.donations == work.stolen_started && work.donations == work.stolen_completed &&
                   work.donor_checks == work.offer_no_demand + work.offer_attempts &&
                   work.offer_attempts == work.donations + work.offer_full + work.offer_busy &&
                   work.waits == work.wakes && work.max_queue_size <= capacity && work.max_local_stack_size <= 97,
               "dispatcher lost work or violated its queue/local-stack/event ledger");
  gate.require(dispatch->waiting_workers() == 0 && dispatch->retained_bytes() == retained && retained > 0,
               "completed dispatcher retains waiters or changes its queue storage");
  gate.donations += work.donations; gate.full_refusals += work.offer_full; gate.waits += work.waits;
  gate.zero_seed_cases += static_cast<u64>(plan->job_count() == 0);
  ++gate.dispatch_runs;
  return work;
}

void corpus(Gate& gate) {
  std::vector<Points> fixtures{
      {{7, 8, 9}}, {{0, 0, 0}, {65535, 65535, 65535}},
      {{0, 0, 0}, {2, 0, 0}, {5, 0, 0}, {8, 0, 0}, {10, 0, 0}, {1000, 0, 0}, {1001, 0, 0}},
      {{0, 0, 0}, {6, 0, 0}, {2, 1, 1}}};
  Points cube, random;
  for (unsigned bits = 0; bits < 8; ++bits)
    cube.push_back({static_cast<std::uint16_t>((bits & 1U) * 65535),
                    static_cast<std::uint16_t>(((bits >> 1U) & 1U) * 65535),
                    static_cast<std::uint16_t>(((bits >> 2U) & 1U) * 65535)});
  std::uint32_t state = 13;
  for (unsigned i = 0; i < 17; ++i) {
    state = state * 1664525U + 1013904223U; const auto y = static_cast<std::uint16_t>(state >> 16U);
    state = state * 1664525U + 1013904223U;
    random.push_back({static_cast<std::uint16_t>(i * 4093), y, static_cast<std::uint16_t>(state >> 16U)});
  }
  fixtures.push_back(cube); fixtures.push_back(random);
  for (const auto& points : fixtures) {
    const auto expected = oracle(gate, points);
    const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
    unsigned sample = 0;
    for (const unsigned k : {1U, 2U, 5U, 10U}) for (const unsigned s : {8U, 10U, 12U})
      for (const auto mode : {WspdFrontMode::Pure, WspdFrontMode::MidpointSamples}) {
        const auto mask = static_cast<std::uint8_t>(sample % 2 == 0 ? 1 : 7);
        static_cast<void>(run_case(gate, index, expected, k, s, mode, mask, sample % 3 == 0 ? 1 : 13,
                                   sample % 2 == 0 ? 1 : 7, sample % 2 == 0 ? 1 : 4, 2, 2));
        if (mode == WspdFrontMode::MidpointSamples) {
          // The dispatcher's private fronts and merge_work must carry the widened
          // window (q2 lane alone, mask 1): total.work == mono baseline with nonzero
          // extension counters and a small-factor limit that bites on these clouds.
          const mhgp8::WspdFrontProposals widened = sample % 4 == 1
              ? mhgp8::WspdFrontProposals{2, std::numeric_limits<std::size_t>::max()}
              : mhgp8::WspdFrontProposals{4, 2};
          static_cast<void>(run_case(gate, index, expected, k, s, mode, 1, sample % 3 == 0 ? 1 : 13,
                                     sample % 2 == 0 ? 1 : 7, sample % 2 == 0 ? 1 : 4, 2, 2, widened));
          ++gate.widened_cases;
        }
        ++sample;
      }
  }
  const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(random));
  const auto expected = oracle(gate, random);
  // Declared demand precedes the receiver's launch. One active worker can
  // fill queue1 and must keep processing when further offers are refused.
  const auto full = run_case(gate, index, expected, 5, 8, WspdFrontMode::Pure, 7, 1, 1, 1, 2, 1);
  gate.require(full.donations > 0 && full.offer_full > 0 && full.max_queue_size == 1,
               "queue1 fixture did not actually donate, refuse a full queue and finish locally");
  const auto masked_index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(fixtures[2]));
  const auto masked_oracle = oracle(gate, fixtures[2]);
  for (const std::uint8_t mask : {2, 3, 4, 5, 6}) {
    const unsigned k = mask <= 3 ? 2 : 5;
    static_cast<void>(run_case(gate, masked_index, masked_oracle, k, 8,
                               WspdFrontMode::MidpointSamples, mask, 1, 1, 1, 2, 1));
    ++gate.masked_cases;
  }
}

// Test-only deadline bounds synchronization bugs. No timing-based claim of
// waiting: the observed predicate is the dispatcher's registered waiters.
void await_waiter(const mhgp8::WspdFrontDispatch& dispatch) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (dispatch.waiting_workers() == 0) {
    if (std::chrono::steady_clock::now() >= deadline)
      throw std::runtime_error("receiver did not register its wait before the test deadline");
    std::this_thread::yield();
  }
}

struct CallbackFailure {};
struct LaunchFailure {};

void forced_transfer(Gate& gate) {
  const Points points{{0, 0, 0}, {1, 0, 0}, {99, 0, 0}, {100, 0, 0}};
  const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
  const auto baseline = reference(gate, *index, 2, 8, WspdFrontMode::Pure, 3);
  const auto plan = mhgp8::make_wspd_front_jobs(index, 2, 8, WspdFrontMode::Pure, 1, 3);
  const auto dispatch = plan->make_dispatch(1, 1, 2);
  std::latch donor_entered(1), receiver_entered(1), release_donor(1);
  std::array<mhgp8::WspdFrontDispatchResult, 2> results;
  std::array<Rectangles, 2> slots;
  std::array<std::exception_ptr, 2> failures;
  std::atomic<bool> donor_signal{false}, receiver_signal{false}, cancel{false};
  std::thread donor([&] {
    try {
      results[0] = dispatch->run_worker([&](const WspdRectangle& rectangle) {
        slots[0].emplace_back(rectangle.a_node, rectangle.b_node, rectangle.lane_mask);
        if (!donor_signal.exchange(true)) { donor_entered.count_down(); release_donor.wait(); }
      }, cancel);
    } catch (...) { failures[0] = std::current_exception(); dispatch->cancel(); }
    if (!donor_signal.exchange(true)) donor_entered.count_down();
  });
  donor_entered.wait();
  if (failures[0]) { release_donor.count_down(); donor.join(); std::rethrow_exception(failures[0]); }
  std::thread receiver;
  try {
    receiver = std::thread([&] {
      try {
        results[1] = dispatch->run_worker([&](const WspdRectangle& rectangle) {
          slots[1].emplace_back(rectangle.a_node, rectangle.b_node, rectangle.lane_mask);
          if (!receiver_signal.exchange(true)) receiver_entered.count_down();
        }, cancel);
      } catch (...) { failures[1] = std::current_exception(); dispatch->cancel(); }
      if (!receiver_signal.exchange(true)) receiver_entered.count_down();
    });
  } catch (...) {
    cancel.store(true); dispatch->cancel(); release_donor.count_down(); donor.join(); throw;
  }
  receiver_entered.wait();
  release_donor.count_down();
  donor.join(); receiver.join();
  for (const auto& failure : failures) if (failure) std::rethrow_exception(failure);
  auto total = plan->prefix_result();
  Rectangles output;
  for (std::size_t slot = 0; slot < 2; ++slot) {
    mhgp8::parallel_detail::merge_work(total.work, results[slot].front.work);
    output.insert(output.end(), slots[slot].begin(), slots[slot].end());
  }
  std::sort(output.begin(), output.end());
  gate.require(results[0].work.donations > 0 && results[1].work.stolen_started > 0 && !slots[1].empty() &&
                   output == baseline.rectangles && total.work == baseline.result.work && dispatch->waiting_workers() == 0,
               "latch-ordered receiver did not consume the other worker's donation without changing the front");
  ++gate.cross_worker_transfers;
}

void cancellation(Gate& gate) {
  const Points points{{0, 0, 0}, {1, 0, 0}, {99, 0, 0}, {100, 0, 0}};
  auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
  auto plan = mhgp8::make_wspd_front_jobs(index, 2, 8, WspdFrontMode::Pure, 1, 3);
  const auto prefix = plan->prefix_result();
  const auto expected = reference(gate, *index, 2, 8, WspdFrontMode::Pure, 3);
  auto owner = plan->make_dispatch(1, std::numeric_limits<std::size_t>::max(), 1);
  plan.reset(); index.reset();
  Rectangles rectangles;
  const std::atomic<bool> no_cancel{false};
  const auto owned = owner->run_worker([&](const WspdRectangle& rectangle) {
    rectangles.emplace_back(rectangle.a_node, rectangle.b_node, rectangle.lane_mask);
  }, no_cancel);
  auto total = prefix;
  mhgp8::parallel_detail::merge_work(total.work, owned.front.work);
  std::sort(rectangles.begin(), rectangles.end());
  gate.require(rectangles == expected.rectangles && total.work == expected.result.work,
               "dispatch lost its shared index/context when both caller handles were reset");
  ++gate.ownership_cases;

  index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
  plan = mhgp8::make_wspd_front_jobs(index, 2, 8, WspdFrontMode::Pure, 1, 3);
  const auto dispatch = plan->make_dispatch(1, std::numeric_limits<std::size_t>::max(), 2);
  std::atomic<unsigned> callbacks{0}, wake_hooks{0};
  bool caught = false;
  try {
    mhgp8::parallel_detail::run_joined_workers(2,
        [&](std::size_t, const std::atomic<bool>& cancel) {
          static_cast<void>(dispatch->run_worker([&](const WspdRectangle&) {
            callbacks.fetch_add(1);
            await_waiter(*dispatch);
            throw CallbackFailure{};
          }, cancel));
        }, mhgp8::parallel_detail::ThreadLauncher{}, [&]() noexcept { wake_hooks.fetch_add(1); dispatch->cancel(); });
  } catch (const CallbackFailure&) { caught = true; }
  gate.require(caught && callbacks.load() == 1 && wake_hooks.load() == 1 && dispatch->waiting_workers() == 0,
               "callback failure did not wake and join its registered waiting receiver");
  ++gate.callback_failures;

  const auto launch_dispatch = plan->make_dispatch(1, std::numeric_limits<std::size_t>::max(), 3);
  std::latch donor_entered(1), release_donor(1);
  std::exception_ptr donor_failure;
  std::atomic<bool> donor_cancel{false};
  std::atomic<bool> donor_once{false};
  std::thread donor([&] {
    try {
      static_cast<void>(launch_dispatch->run_worker([&](const WspdRectangle&) {
        if (!donor_once.exchange(true)) { donor_entered.count_down(); release_donor.wait(); }
      }, donor_cancel));
    } catch (...) { donor_failure = std::current_exception(); }
  });
  donor_entered.wait();
  unsigned attempts = 0;
  const auto launcher = [&](auto&& function) -> std::thread {
    if (++attempts == 2) { await_waiter(*launch_dispatch); throw LaunchFailure{}; }
    return std::thread(std::forward<decltype(function)>(function));
  };
  caught = false;
  std::exception_ptr unexpected;
  try {
    mhgp8::parallel_detail::run_joined_workers(2,
        [&](std::size_t, const std::atomic<bool>& cancel) {
          static_cast<void>(launch_dispatch->run_worker([](const WspdRectangle&) {}, cancel));
        }, launcher, [&]() noexcept { launch_dispatch->cancel(); });
  } catch (const LaunchFailure&) { caught = true; }
  catch (...) { unexpected = std::current_exception(); }
  donor_cancel.store(true); launch_dispatch->cancel(); release_donor.count_down(); donor.join();
  if (unexpected) std::rethrow_exception(unexpected);
  if (donor_failure) std::rethrow_exception(donor_failure);
  gate.require(caught && attempts == 2 && launch_dispatch->waiting_workers() == 0,
               "partial launch failure did not wake/join a receiver while a donor remained active");
  ++gate.launch_failures;

  gate.rejects([&] { static_cast<void>(plan->make_dispatch(0, 1, 2)); }, "dispatch accepted zero queue capacity");
  gate.rejects([&] { static_cast<void>(plan->make_dispatch(1, 0, 2)); }, "dispatch accepted zero donation interval");
  gate.rejects([&] { static_cast<void>(plan->make_dispatch(1, 1, 0)); }, "dispatch accepted zero declared workers");
  const auto invalid = plan->make_dispatch(1, 1, 1);
  gate.rejects([&] { static_cast<void>(invalid->run_worker({}, no_cancel)); }, "dispatch accepted empty consumer");
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_wspd_front_dispatch_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    corpus(gate); forced_transfer(gate); cancellation(gate);
    gate.require(gate.dispatch_runs >= 150 && gate.oracle_sites > 10000 && gate.donations > 0 &&
                     gate.full_refusals > 0 && gate.reduced_rectangles > 0 && gate.masked_cases == 5 &&
                     gate.zero_seed_cases > 0 && gate.ownership_cases == 1 && gate.callback_failures == 1 &&
                     gate.cross_worker_transfers == 1 && gate.launch_failures == 1 && gate.invalid_inputs == 4 &&
                     gate.widened_cases >= 60 && gate.extended_products > 0,
                 "front dispatch gate lost a declared non-vacuity floor");
    std::cout << "{\"schema\":\"mhgp8_wspd_front_dispatch_gate_v1\",\"status\":\"passed\","
              << "\"public_status\":\"not_claimed\",\"checks\":" << gate.checks
              << ",\"oracle_sites\":" << gate.oracle_sites << ",\"reference_runs\":" << gate.reference_runs
              << ",\"dispatch_runs\":" << gate.dispatch_runs << ",\"donations\":" << gate.donations
              << ",\"full_refusals\":" << gate.full_refusals << ",\"reduced_rectangles\":" << gate.reduced_rectangles
              << ",\"masked_cases\":" << gate.masked_cases << ",\"zero_seed_cases\":" << gate.zero_seed_cases
              << ",\"ownership_cases\":" << gate.ownership_cases << ",\"waits\":" << gate.waits
              << ",\"cross_worker_transfers\":" << gate.cross_worker_transfers
              << ",\"callback_failures\":" << gate.callback_failures << ",\"launch_failures\":" << gate.launch_failures
              << ",\"invalid_inputs\":" << gate.invalid_inputs
              << ",\"widened_cases\":" << gate.widened_cases << ",\"extended_products\":" << gate.extended_products << "}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_wspd_front_dispatch_gate failed: " << error.what() << '\n';
    return 1;
  }
}
