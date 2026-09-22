#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "pipeline/local_credits.hpp"

namespace {

using mhgp8::Lane;
using mhgp8::RectangleInput;
using mhgp8::Strategy;
constexpr std::array<Lane, 3> lanes{Lane::Q2, Lane::Q3, Lane::Q4};
constexpr std::array<Strategy, 3> strategies{Strategy::Pool, Strategy::DualBlocks, Strategy::Tubes};

struct Gate {
  std::uint64_t checks{};
  std::uint64_t batches{};
  std::uint64_t lane_comparisons{};
  std::uint64_t rejections{};
  std::uint64_t shared_preparations{};
  std::uint64_t saturated_batches{};
  std::uint64_t fallback_batches{};
  std::array<std::uint64_t, 4> active_populations{};

  void require(bool condition, const std::string& cause) {
    ++checks;
    if (!condition) throw std::runtime_error(cause);
  }

  template <class Function>
  void rejects(Function&& function, const std::string& cause) {
    bool refused = false;
    try {
      function();
    } catch (const std::invalid_argument&) {
      refused = true;
    }
    require(refused, cause);
    ++rejections;
  }
};

[[nodiscard]] auto all_work(const mhgp8::Work& w) {
  return std::tie(w.validation_points, w.uniqueness_comparisons,
                  w.pool_selection_tests, w.pool_selected, w.tree_nodes,
                  w.tree_point_visits, w.dual_tasks, w.credited_blocks,
                  w.noncredit_blocks, w.leaf_pairs, w.saturated_tasks,
                  w.max_tree_depth, w.max_task_depth, w.tube_records,
                  w.tube_cells, w.tube_sort_comparisons, w.tube_sweep_tests,
                  w.tube_credited_sites, w.tube_separation_fallbacks,
                  w.predicates.point_tests, w.predicates.universal_queries,
                  w.predicates.q2_axis_terms, w.predicates.corner_tests,
                  w.predicates.block_bound_tests, w.predicates.negative_probes);
}

template <class Left, class Right>
bool same_sequence(const Left& left, const Right& right) {
  return left.size() == right.size() && std::equal(left.begin(), left.end(), right.begin());
}

void same_plan(Gate& gate, const mhgp8::CreditPlan& batch, const mhgp8::CreditPlan& single) {
  gate.require(batch.lane() == single.lane() && batch.strategy() == single.strategy() &&
                   batch.threshold() == single.threshold() &&
                   batch.core_credit() == single.core_credit() &&
                   batch.total_pairs() == single.total_pairs() &&
                   batch.candidate_pairs() == single.candidate_pairs() &&
                   &batch.rectangle() == &single.rectangle(),
                "batch changed plan metadata or immutable owner");
  gate.require(same_sequence(batch.a_credits(), single.a_credits()) &&
                   same_sequence(batch.b_credits(), single.b_credits()) &&
                   same_sequence(batch.a_order(), single.a_order()) &&
                   same_sequence(batch.b_order(), single.b_order()),
                "batch changed physical credits or grouped original IDs");
  gate.require(batch.blocks().size() == single.blocks().size(),
                "batch changed the number of residual blocks");
  for (std::size_t i = 0; i < batch.blocks().size(); ++i) {
    const auto& a = batch.blocks()[i];
    const auto& b = single.blocks()[i];
    gate.require(a.a.first == b.a.first && a.a.last == b.a.last &&
                     a.b.first == b.b.first && a.b.last == b.b.last,
                  "batch changed a physical residual block");
  }
  std::set<std::pair<std::size_t, std::size_t>> expanded;
  batch.for_each_candidate([&](std::size_t a, std::size_t b) {
    gate.require(expanded.emplace(a, b).second && single.keeps(a, b),
                  "batch expansion duplicated or introduced a pair");
  });
  gate.require(expanded.size() == single.candidate_pairs(), "batch lost candidate pairs");
  ++gate.lane_comparisons;
}

void compare_batch(Gate& gate, const RectangleInput& input, unsigned kmax,
                     unsigned separation, Strategy strategy) {
  const auto owner = mhgp8::prepare_rectangle(input, kmax, separation);
  const auto batch = mhgp8::make_credit_batch(owner, strategy);
  std::array<std::uint64_t, 4> old_preparation{};
  unsigned active = 0;
  for (const auto lane : lanes) {
    const auto single = mhgp8::make_credit_plan(owner, lane, strategy);
    const auto& grouped = batch.plan(lane);
    same_plan(gate, grouped, single);
    active += static_cast<unsigned>(single.threshold() > single.core_credit());
    const auto& old_work = single.work();
    const auto& query_work = grouped.work();
    if (strategy != Strategy::Tubes) {
      gate.require(all_work(query_work) == all_work(old_work),
                    "batch changed non-tube work");
    } else {
      gate.require(query_work.tube_records == 0 && query_work.tube_cells == 0 &&
                       query_work.tube_sort_comparisons == 0 &&
                       query_work.tube_separation_fallbacks == 0,
                    "shared tube preparation was charged again to a lane");
      gate.require(query_work.tube_sweep_tests == old_work.tube_sweep_tests &&
                       query_work.tube_credited_sites == old_work.tube_credited_sites,
                    "shared tube preparation changed lane-dependent sweeps");
      auto stripped = old_work;
      stripped.tube_records = 0;
      stripped.tube_cells = 0;
      stripped.tube_sort_comparisons = 0;
      stripped.tube_separation_fallbacks = 0;
      gate.require(all_work(query_work) == all_work(stripped),
                    "batch lost non-preparation work accounting");
      old_preparation[0] += old_work.tube_records;
      old_preparation[1] += old_work.tube_cells;
      old_preparation[2] += old_work.tube_sort_comparisons;
      old_preparation[3] += old_work.tube_separation_fallbacks;
    }
    if (single.threshold() == 0) {
      gate.require(grouped.candidate_pairs() == 0 && all_work(query_work) == all_work(mhgp8::Work{}),
                    "inactive lane performed work or emitted candidates");
    }
  }
  ++gate.batches;
  ++gate.active_populations[active];
  const auto& shared = batch.shared_work();
  if (strategy != Strategy::Tubes || active == 0) {
    gate.require(all_work(shared) == all_work(mhgp8::Work{}),
                  "batch prepared unused shared tube data");
  } else {
    const std::array<std::uint64_t, 4> preparation{
        shared.tube_records, shared.tube_cells, shared.tube_sort_comparisons,
        shared.tube_separation_fallbacks};
    for (std::size_t i = 0; i < preparation.size(); ++i) {
      gate.require(old_preparation[i] == active * preparation[i],
                    "tube preparation is not paid exactly once across active lanes");
    }
    auto stripped = shared;
    stripped.tube_records = 0;
    stripped.tube_cells = 0;
    stripped.tube_sort_comparisons = 0;
    stripped.tube_separation_fallbacks = 0;
    gate.require(all_work(stripped) == all_work(mhgp8::Work{}),
                  "lane-dependent work was hidden in shared preparation");
    if (shared.tube_records > 0 && active > 1) ++gate.shared_preparations;
    if (shared.tube_separation_fallbacks > 0) {
      gate.require(shared.tube_separation_fallbacks == 2,
                    "failed separation was not charged once per factor");
      ++gate.fallback_batches;
    }
  }
  if (active == 0) ++gate.saturated_batches;
}

// side_shift is the x offset of factor B, core_x the first core proposal.
// The historical (30000, 15000) values keep the pinned 16-bit fixture; the
// 18-bit twin ends factor B exactly at coordinate_limit.
RectangleInput fixture(unsigned core_count, unsigned side_shift = 30000,
                       unsigned core_x = 15000) {
  RectangleInput input;
  for (unsigned side = 0; side < 2; ++side) {
    for (unsigned i = 0; i < 12; ++i) {
      input.points.push_back({static_cast<mhgp8::Coordinate>(1000 + side * side_shift + 2 * i),
                               static_cast<mhgp8::Coordinate>(1000 + i % 2),
                               static_cast<mhgp8::Coordinate>(1000 + (i / 2) % 2)});
    }
  }
  input.a = {0, 12};
  input.b = {12, 24};
  for (unsigned i = 0; i < core_count; ++i) {
    input.core_candidates.push_back(input.points.size());
    input.points.push_back({static_cast<mhgp8::Coordinate>(core_x + i), 1000, 1000});
  }
  return input;
}

void run(Gate& gate) {
  for (const auto kmax : {1U, 2U, 3U, 5U, 10U}) {
    for (const auto core : {0U, 1U, 3U, 12U}) {
      for (const auto separation : {8U, 10U, 12U}) {
        for (const auto strategy : strategies) {
          compare_batch(gate, fixture(core), kmax, separation, strategy);
        }
      }
    }
  }
  const RectangleInput fallback{
      {{0, 2, 0}, {4, 5, 0}, {15, 1, 0}, {15, 6, 0}}, {0, 2}, {2, 4}, {}};
  for (const auto kmax : {1U, 2U, 3U, 5U, 10U}) {
    for (const auto strategy : strategies) compare_batch(gate, fallback, kmax, 1, strategy);
  }
  gate.rejects([] { static_cast<void>(mhgp8::make_credit_batch({}, Strategy::Tubes)); },
                 "batch accepted a null owner");
  const auto owner = mhgp8::prepare_rectangle(fixture(0), 10, 8);
  gate.rejects([&] { static_cast<void>(mhgp8::make_credit_batch(owner, static_cast<Strategy>(255))); },
                 "batch accepted an invalid strategy");
  const auto batch = mhgp8::make_credit_batch(owner, Strategy::Tubes);
  gate.rejects([&] { static_cast<void>(batch.plan(static_cast<Lane>(255))); },
                 "batch accepted an invalid lane lookup");
  gate.require(!std::is_default_constructible_v<mhgp8::CreditBatch>,
                "uncertified batch is default constructible");
  auto source = fixture(0);
  auto retained = mhgp8::prepare_rectangle(source, 10, 8);
  std::weak_ptr<const mhgp8::PreparedRectangle> weak = retained;
  const auto persistent = mhgp8::make_credit_batch(retained, Strategy::Tubes);
  const auto first = retained->points()[0];
  retained.reset();
  source.points[0] = {65535, 65535, 65535};
  for (const auto lane : lanes) {
    gate.require(!weak.expired() && persistent.plan(lane).rectangle().points()[0] == first,
                  "batch lost its immutable copied owner");
  }
  gate.require(gate.batches == 195 && gate.lane_comparisons == 585 &&
                   gate.rejections == 3 && gate.shared_preparations > 0 &&
                   gate.saturated_batches > 0 && gate.fallback_batches == 5 &&
                   std::all_of(gate.active_populations.begin(), gate.active_populations.end(),
                                [](std::uint64_t n) { return n > 0; }),
                "batch coverage or accounting non-vacuity failed");
}

// 18-bit twin of run(): the same loops on a fixture whose B factor ends at
// coordinate_limit (1000 + 261121 + 22 = 262143) with core proposals at 2^17,
// the same failed-separation fixture translated to the (limit, limit, limit)
// corner, and the range refusal one grid step beyond the limit. Its counters
// are pinned separately from the 16-bit gate.
void run_wide(Gate& gate) {
  constexpr auto limit = mhgp8::coordinate_limit;
  constexpr unsigned wide_shift = 261121;
  constexpr unsigned wide_core = 131072;
  static_assert(1000 + wide_shift + 2 * 11 == static_cast<unsigned>(limit));
  for (const auto kmax : {1U, 2U, 3U, 5U, 10U}) {
    for (const auto core : {0U, 1U, 3U, 12U}) {
      for (const auto separation : {8U, 10U, 12U}) {
        for (const auto strategy : strategies) {
          compare_batch(gate, fixture(core, wide_shift, wide_core), kmax, separation, strategy);
        }
      }
    }
  }
  const RectangleInput fallback{
      {{limit - 15, limit - 4, limit}, {limit - 11, limit - 1, limit},
       {limit, limit - 5, limit}, {limit, limit, limit}}, {0, 2}, {2, 4}, {}};
  for (const auto kmax : {1U, 2U, 3U, 5U, 10U}) {
    for (const auto strategy : strategies) compare_batch(gate, fallback, kmax, 1, strategy);
  }
  gate.rejects([&] {
    static_cast<void>(mhgp8::prepare_rectangle(fixture(0, wide_shift + 1, wide_core), 10, 8));
  }, "batch owner accepted a coordinate one step beyond coordinate_limit");
  auto source = fixture(0, wide_shift, wide_core);
  auto retained = mhgp8::prepare_rectangle(source, 10, 8);
  std::weak_ptr<const mhgp8::PreparedRectangle> weak = retained;
  const auto persistent = mhgp8::make_credit_batch(retained, Strategy::Tubes);
  const auto first = retained->points()[0];
  retained.reset();
  source.points[0] = {limit, limit, limit};
  for (const auto lane : lanes) {
    gate.require(!weak.expired() && persistent.plan(lane).rectangle().points()[0] == first,
                  "wide batch lost its immutable copied owner");
  }
  gate.require(gate.batches == 195 && gate.lane_comparisons == 585 &&
                   gate.rejections == 1 && gate.shared_preparations > 0 &&
                   gate.saturated_batches > 0 && gate.fallback_batches == 5 &&
                   std::all_of(gate.active_populations.begin(), gate.active_populations.end(),
                                [](std::uint64_t n) { return n > 0; }),
                "wide batch coverage or accounting non-vacuity failed");
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_batch_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    run(gate);
    Gate wide;
    run_wide(wide);
    std::cout << "mhgp8_batch_gate passed checks=" << gate.checks
              << " batches=" << gate.batches << " lane_comparisons=" << gate.lane_comparisons
              << " shared_preparations=" << gate.shared_preparations
              << " saturated_batches=" << gate.saturated_batches
              << " fallback_batches=" << gate.fallback_batches
              << " rejections=" << gate.rejections
              << " wide_checks=" << wide.checks << " wide_batches=" << wide.batches
              << " wide_lane_comparisons=" << wide.lane_comparisons
              << " wide_shared_preparations=" << wide.shared_preparations
              << " wide_saturated_batches=" << wide.saturated_batches
              << " wide_fallback_batches=" << wide.fallback_batches
              << " wide_rejections=" << wide.rejections << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_batch_gate failed: " << error.what() << '\n';
    return 1;
  }
}
