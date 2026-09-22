#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

// Explicit bounded reuse of the independent multiprecision witness oracle.
// No production predicate or front certificate decides missing-pair safety.
#include "oracle/p0_oracle.hpp"
#include "wspd/front.hpp"

namespace {

using mhgp8::Lane;
using mhgp8::Point3;
using mhgp8::Q2CensusIndex;
using mhgp8::WspdFrontJobs;
using mhgp8::WspdFrontMode;
using mhgp8::WspdFrontResult;
using mhgp8::WspdFrontWork;
using mhgp8::WspdRectangle;
using mhgp8::u64;
using Rectangle = std::tuple<std::size_t, std::size_t, std::uint8_t>;

static_assert(!std::is_copy_constructible_v<WspdFrontJobs>);
static_assert(!std::is_copy_assignable_v<WspdFrontJobs>);
static_assert(!std::is_move_constructible_v<WspdFrontJobs>);
static_assert(!std::is_move_assignable_v<WspdFrontJobs>);
static_assert(std::is_same_v<decltype(std::declval<const WspdFrontJobs&>().index()),
                             const Q2CensusIndex&>);
static_assert(std::is_same_v<decltype(std::declval<const WspdFrontJobs&>().prefix_result()),
                             const WspdFrontResult&>);

constexpr std::array<u64 WspdFrontWork::*, 27> additive_fields{
    &WspdFrontWork::product_visits, &WspdFrontWork::diagonal_splits,
    &WspdFrontWork::diagonal_leaves, &WspdFrontWork::disjoint_splits,
    &WspdFrontWork::separation_tests, &WspdFrontWork::witness_searches,
    &WspdFrontWork::witness_descent_steps, &WspdFrontWork::witness_box_distance_tests,
    &WspdFrontWork::proposed_sites, &WspdFrontWork::proposals_in_factors,
    &WspdFrontWork::h_bound_tests, &WspdFrontWork::xi_bound_tests,
    &WspdFrontWork::witness_lane_credits, &WspdFrontWork::fully_rejected_products,
    &WspdFrontWork::emitted_rectangles, &WspdFrontWork::emitted_factor_sites,
    &WspdFrontWork::leaf_pair_rectangles, &WspdFrontWork::extended_products,
    &WspdFrontWork::extended_proposals, &WspdFrontWork::extended_proposals_in_factors,
    &WspdFrontWork::extended_credits, &WspdFrontWork::extended_rejections,
    &WspdFrontWork::inherited_credits, &WspdFrontWork::inherited_duplicates,
    &WspdFrontWork::extended_inherited_duplicates, &WspdFrontWork::inherited_rejections,
    &WspdFrontWork::emitted_witness_credits};
// A std::array with fewer initializers than its size compiles and holds null
// member pointers: every entry must be a real field.
static_assert(std::none_of(additive_fields.begin(), additive_fields.end(),
                           [](u64 WspdFrontWork::* field) { return field == nullptr; }));
constexpr std::array<u64 WspdFrontWork::*, 3> maximum_fields{
    &WspdFrontWork::max_factor_size, &WspdFrontWork::max_stack_size,
    &WspdFrontWork::max_product_depth};
constexpr std::array<std::array<u64, 5> WspdFrontWork::*, 2> class_fields{
    &WspdFrontWork::size_class_rectangles, &WspdFrontWork::size_class_pair_mass};
constexpr std::array<std::array<u64, 3> WspdFrontWork::*, 3> lane_fields{
    &WspdFrontWork::rejected_pair_mass, &WspdFrontWork::residual_pair_mass,
    &WspdFrontWork::lane_rectangles};
// The manual tables above drive both the independent sum and the mutants;
// a field added to the structure but not to a table must fail to compile.
static_assert(sizeof(WspdFrontWork) == (27 + 3 + 2 * 5 + 3 * 3) * sizeof(u64));

bool same_work(const WspdFrontWork& a, const WspdFrontWork& b) {
  for (const auto field : additive_fields) if (a.*field != b.*field) return false;
  for (const auto field : maximum_fields) if (a.*field != b.*field) return false;
  for (const auto field : class_fields) if (a.*field != b.*field) return false;
  for (const auto field : lane_fields) if (a.*field != b.*field) return false;
  return true;
}

bool same_result(const WspdFrontResult& a, const WspdFrontResult& b) {
  return a.total_unordered_pairs == b.total_unordered_pairs &&
         a.active_lane_mask == b.active_lane_mask && same_work(a.work, b.work);
}

void add_work(WspdFrontWork& total, const WspdFrontWork& part) {
  for (const auto field : additive_fields) total.*field += part.*field;
  for (const auto field : maximum_fields) total.*field = std::max(total.*field, part.*field);
  for (const auto field : class_fields) {
    for (std::size_t i = 0; i < 5; ++i) (total.*field)[i] += (part.*field)[i];
  }
  for (const auto field : lane_fields) {
    for (std::size_t i = 0; i < 3; ++i) (total.*field)[i] += (part.*field)[i];
  }
}

struct Gate {
  u64 checks{};
  u64 clouds{};
  u64 oracle_point_tests{};
  u64 mono_runs{};
  u64 plans{};
  u64 ordered_replays{};
  u64 job_runs{};
  u64 zero_job_plans{};
  u64 all_terminal_plans{};
  u64 terminal_jobs{};
  u64 pending_jobs{};
  u64 filtered_cuts{};
  u64 mixed_lane_cuts{};
  u64 sparse_masks{};
  u64 rejected_lane_pairs{};
  u64 residual_lane_pairs{};
  u64 partial_lane_rectangles{};
  u64 nonsingleton_rectangles{};
  u64 reentrant_runs{};
  u64 callback_failures{};
  u64 invalid_inputs{};
  u64 model_mutants{};
  u64 widened_plans{};
  u64 extended_products{};
  u64 limit_bites{};
  // Inherited witnesses through preparation, run_job and the sum.
  u64 inheriting_plans{};
  WspdFrontWork inheriting_work{};  // Sum over the mono runs with inherit_witnesses.
  u64 witness_carrying_jobs{};      // Jobs whose ROOT task received at least one rank.
  u64 wide_clouds{};                // 18-bit twins: clouds with a coordinate above 65535.

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

std::uint8_t active_mask(unsigned kmax, std::uint8_t requested) {
  return static_cast<std::uint8_t>(requested & ((1U << std::min(kmax, 3U)) - 1U));
}

std::size_t pair_offset(std::size_t a, std::size_t b, std::size_t n) {
  return std::min(a, b) * n + std::max(a, b);
}

struct Oracle {
  std::size_t n{};
  std::array<std::vector<unsigned>, 3> counts;
};

Oracle make_oracle(Gate& gate, std::span<const Point3> points) {
  Oracle result;
  result.n = points.size();
  for (auto& lane : result.counts) lane.resize(result.n * result.n);
  for (std::size_t a = 0; a < result.n; ++a) {
    for (std::size_t b = a + 1; b < result.n; ++b) {
      for (const auto& z : points) {
        for (unsigned lane = 0; lane < 3; ++lane) {
          result.counts[lane][pair_offset(a, b, result.n)] += static_cast<unsigned>(
              mhgp8::oracle::point_witness(static_cast<Lane>(lane + 2), points[a], points[b], z));
          ++gate.oracle_point_tests;
        }
      }
    }
  }
  return result;
}

bool valid_cover(const std::vector<Rectangle>& rectangles, const Q2CensusIndex& index,
                 const Oracle& oracle, unsigned kmax, std::uint8_t active, bool pure) {
  const auto nodes = index.spatial_nodes();
  const auto order = index.spatial_order();
  std::array<std::vector<unsigned>, 3> cover;
  for (auto& lane : cover) lane.resize(oracle.n * oracle.n);
  for (const auto& [a_id, b_id, mask] : rectangles) {
    if (a_id >= nodes.size() || b_id >= nodes.size() || mask == 0 || (mask & active) != mask) return false;
    const auto a = nodes[a_id].range;
    const auto b = nodes[b_id].range;
    if (!(a.last <= b.first || b.last <= a.first)) return false;
    for (auto ai = a.first; ai < a.last; ++ai) {
      for (auto bi = b.first; bi < b.last; ++bi) {
        if (order[ai] == order[bi]) return false;
        const auto offset = pair_offset(order[ai], order[bi], oracle.n);
        for (unsigned lane = 0; lane < 3; ++lane) {
          if ((mask & (1U << lane)) != 0) ++cover[lane][offset];
        }
      }
    }
  }
  for (unsigned lane = 0; lane < 3; ++lane) {
    const bool enabled = (active & (1U << lane)) != 0;
    for (std::size_t a = 0; a < oracle.n; ++a) {
      for (std::size_t b = a + 1; b < oracle.n; ++b) {
        const auto offset = pair_offset(a, b, oracle.n);
        const auto count = cover[lane][offset];
        if (!enabled && count != 0) return false;
        if (enabled && (count > 1 || (pure && count != 1))) return false;
        if (enabled && count == 0 && oracle.counts[lane][offset] < kmax - lane) return false;
      }
    }
  }
  return true;
}

struct Capture {
  WspdFrontResult result;
  std::vector<Rectangle> rectangles;
};

auto collector(Capture& capture) {
  return [&](const WspdRectangle& rectangle) {
    capture.rectangles.emplace_back(rectangle.a_node, rectangle.b_node, rectangle.lane_mask);
  };
}

Capture mono(Gate& gate, const Q2CensusIndex& index, const Oracle& oracle,
             unsigned kmax, unsigned s, WspdFrontMode mode, std::uint8_t requested,
             mhgp8::WspdFrontProposals proposals = {}) {
  Capture capture;
  capture.result = mhgp8::run_wspd_front(index, kmax, s, mode, collector(capture), requested, proposals);
  gate.extended_products += capture.result.work.extended_products;
  if (proposals.inherit_witnesses) add_work(gate.inheriting_work, capture.result.work);
  const auto active = active_mask(kmax, requested);
  gate.require(capture.result.total_unordered_pairs == oracle.n * (oracle.n - 1) / 2 &&
                   capture.result.active_lane_mask == active,
               "monolithic front metadata differs from the independent global domain");
  gate.require(valid_cover(capture.rectangles, index, oracle, kmax, active, mode == WspdFrontMode::Pure),
               "monolithic front duplicates, loses, or unsafely rejects an oracle lane pair");
  const auto nodes = index.spatial_nodes();
  for (const auto& [a, b, mask] : capture.rectangles) {
    gate.partial_lane_rectangles += static_cast<u64>(mask != active);
    gate.nonsingleton_rectangles += static_cast<u64>(nodes[a].range.size() > 1 || nodes[b].range.size() > 1);
  }
  for (unsigned lane = 0; lane < 3; ++lane) {
    gate.rejected_lane_pairs += capture.result.work.rejected_pair_mass[lane];
    gate.residual_lane_pairs += capture.result.work.residual_pair_mass[lane];
  }
  std::sort(capture.rectangles.begin(), capture.rectangles.end());
  ++gate.mono_runs;
  return capture;
}

// `inheriting_kmax` is Kmax when the plan runs inherit_witnesses, else zero.
void compare_jobs(Gate& gate, const WspdFrontJobs& plan, const Capture& reference,
                  std::size_t target, unsigned inheriting_kmax = 0) {
  const auto prefix = plan.prefix_result();
  const auto bytes = plan.retained_bytes();
  const auto job_count = plan.job_count();
  const auto terminal_count = plan.terminal_job_count();
  gate.require((job_count <= target || job_count - target <= 2) && terminal_count <= job_count,
               "preparation retained more than target+2 jobs or invalid terminal count");
  gate.require(job_count == 0 || bytes > 0, "nonempty job plan reports no retained storage");
  gate.require(prefix.total_unordered_pairs == reference.result.total_unordered_pairs &&
                   prefix.active_lane_mask == reference.result.active_lane_mask,
               "prefix metadata is not the full global pair/lane domain");
  if (target == 1) {
    gate.require(job_count == 1 && terminal_count == 0 && same_work(prefix.work, WspdFrontWork{}),
                 "target one did not leave precisely the unvisited root and zero prefix work");
  } else if (plan.index().cloud().points().size() == 1) {
    gate.require(job_count == 0 && terminal_count == 0 && prefix.work.diagonal_leaves == 1 &&
                     prefix.work.product_visits == 1,
                 "completed singleton preparation retained a spurious job or lost its diagonal leaf");
  }
  gate.zero_job_plans += static_cast<u64>(job_count == 0);
  gate.all_terminal_plans += static_cast<u64>(job_count != 0 && job_count == terminal_count);
  if (terminal_count < job_count) {
    const auto& rejected = prefix.work.rejected_pair_mass;
    if (std::any_of(rejected.begin(), rejected.end(), [](u64 count) { return count != 0; })) {
      ++gate.filtered_cuts;
      for (unsigned lane = 1; lane < 3; ++lane) {
        if ((prefix.active_lane_mask & 1U) != 0 && (prefix.active_lane_mask & (1U << lane)) != 0 &&
            rejected[0] != rejected[lane]) {
          ++gate.mixed_lane_cuts;
          break;
        }
      }
    }
  }
  std::vector<std::size_t> ids(job_count);
  std::iota(ids.begin(), ids.end(), std::size_t{0});
  for (unsigned ordering = 0; ordering < 3; ++ordering) {
    if (ordering == 1) std::reverse(ids.begin(), ids.end());
    if (ordering == 2) {
      // Deterministic interleaving, not a scheduler/RNG implementation.
      ids.clear();
      for (std::size_t id = 0; id < job_count; id += 2) ids.push_back(id);
      for (std::size_t id = 1; id < job_count; id += 2) ids.push_back(id);
      if (ids.size() > 2) std::rotate(ids.begin(), ids.begin() + 1, ids.end());
    }
    Capture capture;
    capture.result = prefix;  // Global metadata is copied once, never summed.
    std::size_t observed_terminals = 0;
    for (const auto id : ids) {
      const auto before = capture.rectangles.size();
      const auto part = plan.run_job(id, collector(capture));
      gate.require(part.total_unordered_pairs == prefix.total_unordered_pairs &&
                       part.active_lane_mask == prefix.active_lane_mask,
                   "job metadata changed from the global domain to a local pair count");
      if (part.work.product_visits == 0) {
        gate.require(same_work(part.work, WspdFrontWork{}) && capture.rectangles.size() == before + 1,
                     "terminal job repeated prefix work or failed its sole callback");
        ++observed_terminals;
        ++gate.terminal_jobs;
      } else {
        ++gate.pending_jobs;
        if (inheriting_kmax != 0) {
          // Credit ledger of ONE job. Every searched product ends rejected with
          // Kmax credits, emitted with its final credits, or split, its final
          // credits being received once by each child INSIDE the job. With W the
          // new credits, I the received ones, R the rejections and E the emitted
          // credits: W + I = Kmax*R + E + (I - I_root)/2, where I_root is what
          // the ROOT task of the job received from the preparation prefix.
          const auto& w = part.work;
          const u64 closed = 2 * (u64{inheriting_kmax} * w.fully_rejected_products + w.emitted_witness_credits);
          const u64 opened = 2 * w.witness_lane_credits + w.inherited_credits;
          gate.require(closed >= opened && closed - opened < inheriting_kmax,
                       "job credit ledger does not leave a received list of at most Kmax-1 ranks");
          gate.witness_carrying_jobs += static_cast<u64>(closed != opened);
        }
      }
      add_work(capture.result.work, part.work);
      ++gate.job_runs;
    }
    gate.require(observed_terminals == terminal_count, "terminal-job metadata disagrees with replay behavior");
    std::sort(capture.rectangles.begin(), capture.rectangles.end());
    gate.require(capture.rectangles == reference.rectangles,
                 "job order changed exact oriented node rectangles or their lane masks");
    gate.require(same_result(capture.result, reference.result),
                 "prefix+jobs differs from monolithic work, arrays, or virtual DFS maxima");
    gate.require(same_result(plan.prefix_result(), prefix) && plan.retained_bytes() == bytes &&
                     plan.job_count() == job_count && plan.terminal_job_count() == terminal_count,
                 "job execution mutated its immutable plan or retained storage");
    ++gate.ordered_replays;
  }
  ++gate.plans;
}

// Every integer predicate of the index and the front (midpoint splits, box gaps,
// H, descent distances) is invariant under an integer translation: the twin of a
// cloud pushed to the far corner of the 18-bit grid (its maximum on each axis
// becomes coordinate_limit) has the same tree, counters and rectangles, and the
// oracle judges it independently. Any difference is an engine fault.
std::vector<Point3> far_corner(std::vector<Point3> points) {
  static_assert(mhgp8::coordinate_limit == 262143);
  std::array<mhgp8::Coordinate, 3> maximum{};
  for (const auto& point : points)
    for (std::size_t axis = 0; axis < 3; ++axis) maximum[axis] = std::max(maximum[axis], point[axis]);
  for (auto& point : points) {
    point.x = static_cast<mhgp8::Coordinate>(point.x + (262143 - maximum[0]));
    point.y = static_cast<mhgp8::Coordinate>(point.y + (262143 - maximum[1]));
    point.z = static_cast<mhgp8::Coordinate>(point.z + (262143 - maximum[2]));
  }
  return points;
}

bool wide(std::span<const Point3> points) {
  return std::any_of(points.begin(), points.end(), [](const Point3& point) {
    return point.x > 65535 || point.y > 65535 || point.z > 65535; });
}

// Historical corpus (u16 profile) followed by its 18-bit twins.
constexpr std::size_t historical_fixture_count = 12;
constexpr std::size_t wide_fixture_count = 4;

std::vector<std::vector<Point3>> fixtures() {
  using mhgp8::Coordinate;
  std::vector<std::vector<Point3>> result{
      {{123, 456, 789}},
      {{0, 0, 0}, {65535, 65535, 65535}},
      {{0, 0, 0}, {20, 0, 0}, {10, 0, 0}},
      {{0, 0, 0}, {20, 0, 0}, {10, 10, 0}},
      {{0, 3, 0}, {3, 0, 0}, {1, 1, 1}},
      {{0, 0, 0}, {6, 0, 0}, {2, 1, 1}}};
  std::vector<Point3> line;
  for (unsigned i = 0; i < 17; ++i) {
    line.push_back({static_cast<Coordinate>(i * 37), 101, 203});
  }
  result.push_back(line);
  std::vector<Point3> cube;
  for (const auto x : {0U, 65535U}) {
    for (const auto y : {0U, 65535U}) {
      for (const auto z : {0U, 65535U}) {
        cube.push_back({static_cast<Coordinate>(x), static_cast<Coordinate>(y),
                        static_cast<Coordinate>(z)});
      }
    }
  }
  result.push_back(cube);
  std::vector<Point3> separated_groups;
  for (unsigned group = 0; group < 3; ++group) {
    for (unsigned i = 0; i < 4; ++i) {
      separated_groups.push_back({static_cast<Coordinate>(group * 27000 + i),
                                  static_cast<Coordinate>(i & 1U),
                                  static_cast<Coordinate>(i >> 1U)});
    }
  }
  result.push_back(separated_groups);
  for (std::uint32_t seed : {13U, 71U}) {
    std::vector<Point3> random;
    auto state = seed;
    const auto coordinate = [&] {
      state = state * 1664525U + 1013904223U;
      return static_cast<std::uint16_t>(state >> 16U);
    };
    for (unsigned i = 0; i < 13; ++i) {
      // Distinct x is explicit; no duplicate-rejection loop hides a quota.
      random.push_back({static_cast<std::uint16_t>(i * 4093 + seed), coordinate(), coordinate()});
    }
    result.push_back(random);
  }
  // Bridged cubes: two cubes of side 6000 whose product is NOT separated at
  // s<=12 (diagonal 10392, gap 48000), three strict universal witnesses of that
  // product halfway, and eight far sites that make the search possible at
  // Kmax=10 without ever being creditable. At Kmax 5 and 10 the product
  // survives with three credits and is split: the root tasks of the jobs cut
  // inside its subtree carry a non-empty received list.
  std::vector<Point3> bridged;
  for (unsigned corner = 0; corner < 8; ++corner) {
    const auto x = static_cast<Coordinate>((corner & 1U) * 6000U);
    const auto y = static_cast<Coordinate>(((corner >> 1U) & 1U) * 6000U);
    const auto z = static_cast<Coordinate>(((corner >> 2U) & 1U) * 6000U);
    bridged.push_back({x, y, z});
    bridged.push_back({static_cast<Coordinate>(x + 54000), y, z});
  }
  // x > 30000: the index puts the three witnesses with the second cube, then
  // splits them from it; they are never inside a factor of the cube product.
  bridged.push_back({30100, 3000, 3000});
  bridged.push_back({30110, 3010, 2990});
  bridged.push_back({30120, 2990, 3010});
  for (unsigned i = 0; i < 8; ++i) {
    bridged.push_back({static_cast<Coordinate>(7000U * i + 500U), 65000, static_cast<Coordinate>(100U * i)});
  }
  result.push_back(bridged);
  if (result.size() != historical_fixture_count) throw std::logic_error("historical front-job corpus changed its size");
  // ---- 18-bit twins (coordinate_limit = 262143), judged by the same Boost oracle.
  result.push_back({{0, 0, 0}, {262143, 262143, 262143}});
  std::vector<Point3> wide_cube;
  for (const auto x : {0U, 262143U}) {
    for (const auto y : {0U, 262143U}) {
      for (const auto z : {0U, 262143U}) {
        wide_cube.push_back({static_cast<Coordinate>(x), static_cast<Coordinate>(y),
                             static_cast<Coordinate>(z)});
      }
    }
  }
  result.push_back(wide_cube);
  // The bridged cubes at the far corner: their far sites (y = 65000, the u16
  // frontier of the historical fixture) now sit at y = 262143.
  result.push_back(far_corner(bridged));
  // A separate 18-bit pseudo-random cloud (state >> 14 gives 18 bits); the u16
  // generators above are pinned recipes and do not change.
  for (std::uint32_t seed : {1013U}) {
    std::vector<Point3> random;
    auto state = seed;
    const auto coordinate = [&] {
      state = state * 1664525U + 1013904223U;
      return static_cast<Coordinate>(state >> 14U);
    };
    for (unsigned i = 0; i < 13; ++i) {
      random.push_back({static_cast<Coordinate>(i * 20011U + seed), coordinate(), coordinate()});  // < 262143 for i < 13.
    }
    result.push_back(random);
  }
  if (result.size() != historical_fixture_count + wide_fixture_count) throw std::logic_error("wide front-job corpus changed its size");
  return result;
}

void corpus(Gate& gate) {
  for (const auto& points : fixtures()) {
    const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
    const auto oracle = make_oracle(gate, points);
    gate.wide_clouds += static_cast<u64>(wide(points));
    const auto* const node_storage = index->spatial_nodes().data();
    const auto* const order_storage = index->spatial_order().data();
    const auto index_work = index->work();
    const auto index_bytes = index->retained_bytes();
    for (const unsigned kmax : {1U, 2U, 5U, 10U}) {
      std::vector<std::uint8_t> masks{1, 7};
      if (kmax >= 2) { masks.push_back(2); masks.push_back(3); }
      if (kmax >= 3) { masks.push_back(4); masks.push_back(5); masks.push_back(6); }
      for (const unsigned s : {8U, 10U, 12U}) {
        for (const auto mode : {WspdFrontMode::Pure, WspdFrontMode::MidpointSamples}) {
          for (const auto mask : masks) {
            const auto reference = mono(gate, *index, oracle, kmax, s, mode, mask);
            gate.sparse_masks += static_cast<u64>(mask != 1 && mask != 7);
            for (const auto target : {std::size_t{1}, std::size_t{2}, std::size_t{3},
                                      std::size_t{7}, std::size_t{13}, std::size_t{31}, std::size_t{4096}}) {
              const auto plan = mhgp8::make_wspd_front_jobs(index, kmax, s, mode, target, mask);
              gate.require(&plan->index() == index.get(), "job factory replaced the shared census index");
              compare_jobs(gate, *plan, reference, target);
            }
            // Widened windows are qualified for the q2 lane alone (active mask 1).
            if (mode != WspdFrontMode::MidpointSamples || active_mask(kmax, mask) != 1) continue;
            // Through preparation, run_job and the sum: BOTH option fields must
            // reach all three, with nonzero extension counters. The limit 2 bites
            // on these small clouds, which a limit of 16 never would.
            const auto unlimited = mono(gate, *index, oracle, kmax, s, mode, mask,
                                        mhgp8::WspdFrontProposals{4, std::numeric_limits<std::size_t>::max()});
            for (const auto proposals : {mhgp8::WspdFrontProposals{2, std::numeric_limits<std::size_t>::max()},
                                         mhgp8::WspdFrontProposals{4, 2}}) {
              const auto widened = mono(gate, *index, oracle, kmax, s, mode, mask, proposals);
              gate.limit_bites += static_cast<u64>(proposals.small_factor_limit == 2 &&
                                                   !same_work(widened.result.work, unlimited.result.work));
              for (const auto target : {std::size_t{1}, std::size_t{3}, std::size_t{13}, std::size_t{4096}}) {
                const auto plan = mhgp8::make_wspd_front_jobs(index, kmax, s, mode, target, mask, proposals);
                compare_jobs(gate, *plan, widened, target);
                ++gate.widened_plans;
              }
            }
            // Inherited witnesses: the list travels in the task, through the
            // FIFO preparation, the stored jobs, run_job and the sum, for every
            // cut and every replay order. Rectangles are judged by the oracle in
            // mono(): a rank counted twice would reject a pair below its depth.
            const auto max_limit = std::numeric_limits<std::size_t>::max();
            for (const auto proposals : {mhgp8::WspdFrontProposals{1, max_limit, true},
                                         mhgp8::WspdFrontProposals{2, max_limit, true},
                                         mhgp8::WspdFrontProposals{4, 2, true}}) {
              const auto stateful = mono(gate, *index, oracle, kmax, s, mode, mask, proposals);
              // The residual cover with inheritance is included in the reference one.
              gate.require(std::includes(reference.rectangles.begin(), reference.rectangles.end(),
                                         stateful.rectangles.begin(), stateful.rectangles.end()),
                           "inherited witnesses emitted a rectangle that the reference front rejects");
              // Cuts at 31 and 63 jobs fall inside the searched subtrees of the
              // bridged cubes: the root tasks of those jobs carry received ranks.
              for (const auto target : {std::size_t{1}, std::size_t{3}, std::size_t{13}, std::size_t{31},
                                        std::size_t{63}, std::size_t{4096}}) {
                const auto plan = mhgp8::make_wspd_front_jobs(index, kmax, s, mode, target, mask, proposals);
                compare_jobs(gate, *plan, stateful, target, kmax);
                ++gate.inheriting_plans;
              }
            }
          }
        }
      }
    }
    gate.require(index->spatial_nodes().data() == node_storage && index->spatial_order().data() == order_storage &&
                     index->retained_bytes() == index_bytes && index->work().point_visits == index_work.point_visits &&
                     index->work().nodes == index_work.nodes && index->work().max_depth == index_work.max_depth &&
                     index->work().escape_links == index_work.escape_links,
                 "front jobs rebuilt or mutated the shared index");
    ++gate.clouds;
  }
}

void ownership_and_callbacks(Gate& gate) {
  for (const auto target : {std::size_t{1}, std::size_t{4096}}) {
    std::vector<Point3> points{{0, 0, 0}, {10, 0, 0}, {5, 3, 0}, {4, 9, 2}};
    auto cloud = mhgp8::prepare_cloud(points);
    auto index = mhgp8::make_q2_cloud_index(cloud);
    const std::weak_ptr<const Q2CensusIndex> weak = index;
    const auto* const identity = index.get();
    const auto plan = mhgp8::make_wspd_front_jobs(index, 2, 8, WspdFrontMode::Pure, target, 7);
    index.reset();
    cloud.reset();
    std::fill(points.begin(), points.end(), Point3{65535, 65535, 65535});
    gate.require(!weak.expired() && &plan->index() == identity && plan->job_count() > 0,
                 "plan lost ownership after caller reset or failed to retain an emitting job");
    Capture expected;
    expected.result = plan->run_job(0, collector(expected));
    gate.require(!expected.rectangles.empty(), "callback fixture job emits nothing");

    Capture outer;
    Capture nested;
    bool entered = false;
    outer.result = plan->run_job(0, [&](const WspdRectangle& rectangle) {
      if (!entered) {
        entered = true;
        nested.result = plan->run_job(0, collector(nested));
      }
      outer.rectangles.emplace_back(rectangle.a_node, rectangle.b_node, rectangle.lane_mask);
    });
    gate.require(entered && outer.rectangles == expected.rectangles && nested.rectangles == expected.rectangles &&
                     same_result(outer.result, expected.result) && same_result(nested.result, expected.result),
                 "same-job callback reentrancy changed work or output");
    ++gate.reentrant_runs;

    struct CallbackFailure {};
    unsigned emitted = 0;
    bool caught = false;
    try {
      static_cast<void>(plan->run_job(0, [&](const WspdRectangle&) { ++emitted; throw CallbackFailure{}; }));
    } catch (const CallbackFailure&) { caught = true; }
    gate.require(caught && emitted == 1, "job swallowed callback failure or kept emitting after it");
    Capture retry;
    retry.result = plan->run_job(0, collector(retry));
    gate.require(retry.rectangles == expected.rectangles && same_result(retry.result, expected.result),
                 "callback exception changed the immutable job or its fresh retry");
    ++gate.callback_failures;
  }
}

void rejects_and_models(Gate& gate) {
  const std::vector<Point3> points{{0, 0, 0}, {20, 0, 0}, {10, 0, 0}};
  const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
  gate.rejects([&] { static_cast<void>(mhgp8::make_wspd_front_jobs({}, 2, 8, WspdFrontMode::Pure, 1)); },
               "jobs accepted a null index owner");
  gate.rejects([&] { static_cast<void>(mhgp8::make_wspd_front_jobs(index, 2, 8, WspdFrontMode::Pure, 0)); },
               "jobs accepted a zero target");
  for (const unsigned k : {0U, 11U, std::numeric_limits<unsigned>::max()}) {
    gate.rejects([&] { static_cast<void>(mhgp8::make_wspd_front_jobs(index, k, 8, WspdFrontMode::Pure, 1)); },
                 "jobs accepted an invalid Kmax");
  }
  gate.rejects([&] { static_cast<void>(mhgp8::make_wspd_front_jobs(index, 2, 0, WspdFrontMode::Pure, 1)); },
               "jobs accepted zero separation");
  gate.rejects([&] { static_cast<void>(mhgp8::make_wspd_front_jobs(index, 2, 8,
                                      static_cast<WspdFrontMode>(99), 1)); }, "jobs accepted invalid mode");
  for (const std::uint8_t mask : {0, 8, 255}) {
    gate.rejects([&] { static_cast<void>(mhgp8::make_wspd_front_jobs(index, 2, 8, WspdFrontMode::Pure, 1, mask)); },
                 "jobs accepted an invalid requested lane mask");
  }
  for (const std::uint8_t mask : {2, 4, 6}) {
    gate.rejects([&] { static_cast<void>(mhgp8::make_wspd_front_jobs(index, 1, 8, WspdFrontMode::Pure, 1, mask)); },
                 "jobs accepted a mask with no available lane");
  }
  const auto plan = mhgp8::make_wspd_front_jobs(index, 2, 8, WspdFrontMode::Pure, 1);
  unsigned emissions = 0;
  const auto consumer = [&](const WspdRectangle&) { ++emissions; };
  for (const auto id : {plan->job_count(), std::numeric_limits<std::size_t>::max()}) {
    gate.rejects([&] { static_cast<void>(plan->run_job(id, consumer)); }, "jobs accepted out-of-domain job ID");
  }
  gate.rejects([&] { static_cast<void>(plan->run_job(0, {})); }, "job accepted an empty callback");
  gate.require(emissions == 0, "invalid job input emitted a rectangle before rejection");

  const auto oracle = make_oracle(gate, points);
  const auto reference = mono(gate, *index, oracle, 2, 8, WspdFrontMode::Pure, 7);
  const auto unbounded_target = std::numeric_limits<std::size_t>::max();
  const auto large_plan = mhgp8::make_wspd_front_jobs(index, 2, 8, WspdFrontMode::Pure, unbounded_target, 7);
  compare_jobs(gate, *large_plan, reference, unbounded_target);
  auto missing = reference.rectangles;
  missing.pop_back();
  gate.require(!valid_cover(missing, *index, oracle, 2, 3, true), "lost-rectangle model escaped coverage judge");
  auto duplicate = reference.rectangles;
  duplicate.push_back(duplicate.front());
  gate.require(!valid_cover(duplicate, *index, oracle, 2, 3, true), "duplicate-rectangle model escaped coverage judge");
  auto inactive = reference.rectangles;
  std::get<2>(inactive.front()) = 7;
  gate.require(!valid_cover(inactive, *index, oracle, 2, 3, true), "inactive-lane model escaped coverage judge");
  gate.require(!valid_cover({}, *index, oracle, 2, 3, false), "unsafe sampled rejection escaped coverage judge");
  gate.model_mutants += 4;
  for (const auto field : additive_fields) {
    auto changed = reference.result.work;
    ++(changed.*field);
    gate.require(!same_work(changed, reference.result.work), "additive-work mutant escaped full comparator");
    ++gate.model_mutants;
  }
  for (const auto field : maximum_fields) {
    auto changed = reference.result.work;
    ++(changed.*field);
    gate.require(!same_work(changed, reference.result.work), "maximum-work mutant escaped full comparator");
    ++gate.model_mutants;
  }
  for (const auto field : class_fields) {
    for (std::size_t i = 0; i < 5; ++i) {
      auto changed = reference.result.work;
      ++(changed.*field)[i];
      gate.require(!same_work(changed, reference.result.work), "class-bin mutant escaped full comparator");
      ++gate.model_mutants;
    }
  }
  for (const auto field : lane_fields) {
    for (std::size_t i = 0; i < 3; ++i) {
      auto changed = reference.result.work;
      ++(changed.*field)[i];
      gate.require(!same_work(changed, reference.result.work), "lane-bin mutant escaped full comparator");
      ++gate.model_mutants;
    }
  }
  auto local_metadata = reference.result;
  ++local_metadata.total_unordered_pairs;
  gate.require(!same_result(local_metadata, reference.result), "summed global-domain metadata escaped comparator");
  local_metadata = reference.result;
  local_metadata.active_lane_mask = 1;
  gate.require(!same_result(local_metadata, reference.result), "local lane metadata escaped comparator");
  gate.model_mutants += 2;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_wspd_front_jobs_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    corpus(gate);
    ownership_and_callbacks(gate);
    rejects_and_models(gate);
    const auto& inheriting = gate.inheriting_work;
    const bool floors = gate.clouds == historical_fixture_count + wide_fixture_count &&
                     gate.wide_clouds == wide_fixture_count && gate.mono_runs > 1000 && gate.plans > 5000 &&
                     gate.ordered_replays == 3 * gate.plans && gate.job_runs > 10000 &&
                     gate.oracle_point_tests > 10000 && gate.zero_job_plans > 0 &&
                     gate.all_terminal_plans > 0 && gate.terminal_jobs > 0 && gate.pending_jobs > 0 &&
                     gate.filtered_cuts > 0 && gate.mixed_lane_cuts > 0 &&
                     gate.sparse_masks > 0 && gate.rejected_lane_pairs > 0 && gate.residual_lane_pairs > 0 &&
                     gate.partial_lane_rectangles > 0 && gate.nonsingleton_rectangles > 0 &&
                     gate.reentrant_runs == 2 && gate.callback_failures == 2 && gate.invalid_inputs >= 16 &&
                     gate.model_mutants == 55 && gate.widened_plans > 500 && gate.extended_products > 0 &&
                     gate.limit_bites > 0 && gate.inheriting_plans > 1000 && inheriting.inherited_credits > 0 &&
                     inheriting.inherited_duplicates > 0 && inheriting.extended_inherited_duplicates > 0 &&
                     inheriting.inherited_rejections > 0 && inheriting.emitted_witness_credits > 0 &&
                     gate.witness_carrying_jobs > 0;
    // A lost floor names its counters: the message alone would not say which.
    if (!floors) {
      std::cerr << "mhgp8_wspd_front_jobs_gate floors: inheriting_plans=" << gate.inheriting_plans
                << " inherited=" << inheriting.inherited_credits
                << " duplicates=" << inheriting.inherited_duplicates
                << " extended_duplicates=" << inheriting.extended_inherited_duplicates
                << " inherited_rejections=" << inheriting.inherited_rejections
                << " emitted_credits=" << inheriting.emitted_witness_credits
                << " witness_jobs=" << gate.witness_carrying_jobs
                << " model_mutants=" << gate.model_mutants << " widened_plans=" << gate.widened_plans
                << " clouds=" << gate.clouds << " wide_clouds=" << gate.wide_clouds << '\n';
    }
    gate.require(floors, "front-job gate lost a declared non-vacuity floor");
    std::cout << "mhgp8_wspd_front_jobs_gate passed checks=" << gate.checks << " clouds=" << gate.clouds
              << " oracle_point_tests=" << gate.oracle_point_tests << " mono_runs=" << gate.mono_runs
              << " plans=" << gate.plans << " ordered_replays=" << gate.ordered_replays
              << " job_runs=" << gate.job_runs << " zero_job_plans=" << gate.zero_job_plans
              << " all_terminal_plans=" << gate.all_terminal_plans << " terminal_jobs=" << gate.terminal_jobs
              << " pending_jobs=" << gate.pending_jobs << " sparse_masks=" << gate.sparse_masks
              << " filtered_cuts=" << gate.filtered_cuts << " mixed_lane_cuts=" << gate.mixed_lane_cuts
              << " rejected_lane_pairs=" << gate.rejected_lane_pairs << " residual_lane_pairs=" << gate.residual_lane_pairs
              << " partial_lane_rectangles=" << gate.partial_lane_rectangles
              << " nonsingleton_rectangles=" << gate.nonsingleton_rectangles
              << " reentrant_runs=" << gate.reentrant_runs << " callback_failures=" << gate.callback_failures
              << " invalid_inputs=" << gate.invalid_inputs << " model_mutants=" << gate.model_mutants
              << " widened_plans=" << gate.widened_plans << " extended_products=" << gate.extended_products
              << " limit_bites=" << gate.limit_bites << " inheriting_plans=" << gate.inheriting_plans
              << " inherited_credits=" << inheriting.inherited_credits
              << " inherited_rejections=" << inheriting.inherited_rejections
              << " witness_carrying_jobs=" << gate.witness_carrying_jobs
              << " wide_clouds=" << gate.wide_clouds << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_wspd_front_jobs_gate failed: " << error.what() << '\n';
    return 1;
  }
}
