#include "front_tasks.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <tuple>
#include <vector>

// Independent, bounded front-continuation gate. No census or timing claim.
// Point witnesses below use the scalar Gram formula, not product predicates.
namespace {
using namespace mhgp8;
using audit_tasks::PartitionResult;
using Record = std::tuple<std::size_t, std::size_t, std::uint8_t>;
using Records = std::vector<Record>;
using PairCounts = std::array<std::vector<unsigned>, 3>;

struct Gate {
  u64 checks{}, clouds{}, public_runs{}, partition_runs{}, rectangles{};
  u64 oracle_point_tests{}, covered_lane_pairs{}, rejected_lane_pairs{};
  u64 inherited_mask_jobs{}, positive_depth_jobs{}, diagonal_jobs{};
  u64 prefix_emission_runs{}, prefix_and_jobs_runs{}, exhausted_prefix_runs{};
  u64 nonidentity_orders{}, invalid_inputs{}, callback_exceptions{}, model_mutants{};

  void require(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
  }
  template <class Function>
  void rejects(Function&& function) {
    bool caught = false;
    try { function(); } catch (const std::invalid_argument&) { caught = true; }
    require(caught, "invalid option accepted");
    ++invalid_inputs;
  }
};

u64 choose2(std::size_t n) {
  return static_cast<u64>(n) * (n - 1) / 2;
}

bool same_work(const WspdFrontWork& a, const WspdFrontWork& b) {
#define SAME(field) if (a.field != b.field) return false
  SAME(product_visits); SAME(diagonal_splits); SAME(diagonal_leaves);
  SAME(disjoint_splits); SAME(separation_tests); SAME(witness_searches);
  SAME(witness_descent_steps); SAME(witness_box_distance_tests);
  SAME(proposed_sites); SAME(proposals_in_factors); SAME(h_bound_tests);
  SAME(xi_bound_tests); SAME(witness_lane_credits); SAME(fully_rejected_products);
  SAME(emitted_rectangles); SAME(emitted_factor_sites); SAME(max_factor_size);
  SAME(leaf_pair_rectangles); SAME(max_product_depth);
  SAME(size_class_rectangles); SAME(size_class_pair_mass);
  SAME(rejected_pair_mass); SAME(residual_pair_mass); SAME(lane_rectangles);
#undef SAME
  return true;  // max_stack_size is a different, local scheduling maximum.
}

PairCounts oracle(Gate& gate, const std::vector<Point3>& points) {
  const auto n = points.size();
  PairCounts result;
  for (auto& lane : result) lane.resize(n * n);
  for (std::size_t a = 0; a < n; ++a) {
    for (std::size_t b = a + 1; b < n; ++b) {
      for (const auto& z : points) {
        i128 uu = 0, vv = 0, uv = 0;
        for (std::size_t axis = 0; axis < 3; ++axis) {
          const i128 u = static_cast<i128>(z[axis]) - points[a][axis];
          const i128 v = static_cast<i128>(points[b][axis]) - points[a][axis];
          uu += u * u; vv += v * v; uv += u * v;
        }
        const i128 h = uv - uu;
        const i128 gram = uu * vv - uv * uv;
        result[0][a * n + b] += static_cast<unsigned>(h > 0);
        result[1][a * n + b] += static_cast<unsigned>(h > 0 && 3 * h * h > gram);
        result[2][a * n + b] += static_cast<unsigned>(h > 0 && 2 * h * h > gram);
        gate.oracle_point_tests += 3;
      }
    }
  }
  return result;
}

PairCounts cover(Gate& gate, const Q2CensusIndex& index, const Records& records,
                 std::uint8_t active) {
  const auto nodes = index.spatial_nodes();
  const auto order = index.spatial_order();
  const auto n = order.size();
  PairCounts result;
  for (auto& lane : result) lane.resize(n * n);
  for (const auto& [a_id, b_id, mask] : records) {
    gate.require(a_id < nodes.size() && b_id < nodes.size(), "rectangle node out of range");
    const auto a = nodes[a_id].range, b = nodes[b_id].range;
    gate.require(a.last <= b.first || b.last <= a.first, "rectangle factors overlap");
    gate.require(mask != 0 && (mask & active) == mask, "rectangle has an inactive lane");
    for (auto ai = a.first; ai < a.last; ++ai) {
      for (auto bi = b.first; bi < b.last; ++bi) {
        const auto lo = std::min(order[ai], order[bi]);
        const auto hi = std::max(order[ai], order[bi]);
        for (unsigned lane = 0; lane < 3; ++lane) {
          if ((mask & (1U << lane)) != 0) ++result[lane][lo * n + hi];
        }
      }
    }
  }
  return result;
}

bool valid_cover(const PairCounts& actual, const PairCounts& truth, std::size_t n,
                 unsigned k, std::uint8_t active, WspdFrontMode mode) {
  for (unsigned lane = 0; lane < 3; ++lane) {
    for (std::size_t a = 0; a < n; ++a) {
      for (std::size_t b = a + 1; b < n; ++b) {
        const auto offset = a * n + b;
        const auto count = actual[lane][offset];
        if ((active & (1U << lane)) == 0) {
          if (count != 0) return false;
        } else {
          if (count > 1 || (mode == WspdFrontMode::Pure && count != 1)) return false;
          if (count == 0 && truth[lane][offset] < k - lane) return false;
        }
      }
    }
  }
  return true;
}

void check_jobs(Gate& gate, const Q2CensusIndex& index, const PartitionResult& result,
                std::size_t width) {
  const auto nodes = index.spatial_nodes();
  const auto n = index.spatial_order().size();
  const auto& front = result.front;
  const auto& prefix = result.prefix;
  u64 visits = prefix.product_visits, descents = prefix.witness_descent_steps;
  u64 rectangles = prefix.emitted_rectangles;
  u64 mass = std::accumulate(prefix.size_class_pair_mass.begin(),
                             prefix.size_class_pair_mass.end(), u64{0});
  std::array<u64, 3> initial{};
  std::vector<unsigned> seed_cover(n * n);
  double intervals = result.prefix_ms;
  gate.require(result.task_bytes == sizeof(audit_tasks::Task), "incorrect descriptor sizeof");
  gate.require(result.maximum_ready >= result.jobs.size() &&
                   result.maximum_ready <= width + 1, "ready queue bound failed");
  gate.require(std::isfinite(result.total_ms) && result.total_ms >= 0 &&
                   std::isfinite(result.prefix_ms) && result.prefix_ms >= 0,
               "nonfinite or negative duration");
  for (const auto& job : result.jobs) {
    const auto& seed = job.seed;
    gate.require(seed.a < nodes.size() && seed.b < nodes.size(), "seed node out of range");
    gate.require(seed.mask != 0 && (seed.mask & front.active_lane_mask) == seed.mask,
                 "seed lane mask is invalid");
    const auto a = nodes[seed.a].range, b = nodes[seed.b].range;
    const bool diagonal = seed.a == seed.b;
    gate.require(diagonal || a.last <= b.first || b.last <= a.first, "seed factors overlap");
    const auto expected = diagonal ? choose2(a.size()) : static_cast<u64>(a.size()) * b.size();
    gate.require(job.seed_mass == expected, "incorrect job input mass");
    gate.require(job.product_visits > 0 && seed.depth <= front.work.max_product_depth,
                 "job seed already consumed or depth reset");
    gate.require(std::isfinite(job.elapsed_ms) && job.elapsed_ms >= 0, "invalid job duration");
    for (auto ai = a.first; ai < a.last; ++ai) {
      for (auto bi = b.first; bi < b.last; ++bi) {
        if (diagonal && ai >= bi) continue;
        const auto lo = std::min(ai, bi), hi = std::max(ai, bi);
        gate.require(++seed_cover[lo * n + hi] == 1, "job seeds duplicate a pair");
      }
    }
    for (unsigned lane = 0; lane < 3; ++lane) {
      if ((seed.mask & (1U << lane)) != 0) initial[lane] += expected;
    }
    visits += job.product_visits; descents += job.witness_descent_steps;
    rectangles += job.rectangles; mass += job.pair_mass; intervals += job.elapsed_ms;
    gate.inherited_mask_jobs += static_cast<u64>(seed.mask != front.active_lane_mask);
    gate.positive_depth_jobs += static_cast<u64>(seed.depth > 0);
    gate.diagonal_jobs += static_cast<u64>(diagonal);
  }
  for (unsigned lane = 0; lane < 3; ++lane) {
    const auto expected = (front.active_lane_mask & (1U << lane)) ? choose2(n) : 0;
    gate.require(prefix.rejected_pair_mass[lane] + prefix.residual_pair_mass[lane] +
                     initial[lane] == expected, "prefix and pending seed masses do not partition lane");
  }
  gate.require(visits == front.work.product_visits && descents == front.work.witness_descent_steps &&
                   rectangles == front.work.emitted_rectangles &&
                   mass == std::accumulate(front.work.size_class_pair_mass.begin(),
                                           front.work.size_class_pair_mass.end(), u64{0}),
               "prefix plus job work differs from final work");
  gate.require(intervals <= result.total_ms + 1e-6, "job intervals exceed enclosing duration");
  gate.prefix_emission_runs += static_cast<u64>(prefix.emitted_rectangles != 0);
  gate.prefix_and_jobs_runs += static_cast<u64>(prefix.emitted_rectangles != 0 && !result.jobs.empty());
  gate.exhausted_prefix_runs += static_cast<u64>(result.jobs.empty());
}

std::vector<std::vector<Point3>> fixtures() {
  std::vector<std::vector<Point3>> result{
    {{7, 8, 9}}, {{0, 0, 0}, {65535, 65535, 65535}},
    {{0, 0, 0}, {2, 0, 0}, {5, 0, 0}, {8, 0, 0}, {10, 0, 0}},
    {{0, 0, 0}, {2, 0, 0}, {5, 0, 0}, {8, 0, 0}, {10, 0, 0}, {1000, 0, 0}, {1001, 0, 0}},
    {{0, 0, 0}, {10, 0, 0}, {5, 5, 0}, {5, 0, 0}, {5, 0, 5}},
    {{0, 0, 0}, {65535, 0, 0}, {0, 65535, 0}, {0, 0, 65535},
     {65535, 65535, 65535}, {65535, 65535, 0}, {65535, 0, 65535}, {0, 65535, 65535}}
  };
  std::vector<Point3> grid, clusters, irregular;
  for (unsigned x = 0; x < 3; ++x)
    for (unsigned y = 0; y < 3; ++y)
      for (unsigned z = 0; z < 3; ++z)
        grid.push_back({static_cast<std::uint16_t>(x * 100),
                        static_cast<std::uint16_t>(y * 100), static_cast<std::uint16_t>(z * 100)});
  for (unsigned group = 0; group < 3; ++group)
    for (unsigned i = 0; i < 8; ++i)
      clusters.push_back({static_cast<std::uint16_t>(group * 20000 + (i & 1U)),
                          static_cast<std::uint16_t>((i >> 1U) & 1U),
                          static_cast<std::uint16_t>((i >> 2U) & 1U)});
  for (unsigned i = 0; i < 19; ++i)
    irregular.push_back({static_cast<std::uint16_t>(i * 337),
                         static_cast<std::uint16_t>((i * i * 941 + 17) % 65536),
                         static_cast<std::uint16_t>((i * 3119 + 71) % 65536)});
  result.push_back(grid); result.push_back(clusters); result.push_back(irregular);
  return result;
}

void corpus(Gate& gate) {
  bool mask_mutant = false, duplicate_mutant = false, work_mutant = false, omission_mutant = false;
  for (const auto& original : fixtures()) {
    for (unsigned permutation = 0; permutation < 2; ++permutation) {
      auto points = original;
      if (permutation != 0) std::reverse(points.begin(), points.end());
      const auto index = make_q2_cloud_index(prepare_cloud(points));
      const auto truth = oracle(gate, points);
      const auto order = index->spatial_order();
      for (std::size_t i = 0; i < order.size(); ++i) {
        if (order[i] != i) { ++gate.nonidentity_orders; break; }
      }
      ++gate.clouds;
      for (const unsigned k : {1U, 2U, 5U, 10U}) {
        const auto available = (1U << std::min(k, 3U)) - 1;
        for (unsigned requested = 1; requested <= 7; ++requested) {
          if ((requested & available) == 0) continue;
          const auto mask = static_cast<std::uint8_t>(requested);
          for (const auto mode : {WspdFrontMode::Pure, WspdFrontMode::MidpointSamples}) {
            Records reference;
            const auto expected = run_wspd_front(*index, k, 8, mode,
                [&](const WspdRectangle& r) { reference.emplace_back(r.a_node, r.b_node, r.lane_mask); }, mask);
            const Records reference_order = reference;
            std::sort(reference.begin(), reference.end());
            ++gate.public_runs;
            const auto expected_cover = cover(gate, *index, reference, expected.active_lane_mask);
            gate.require(valid_cover(expected_cover, truth, points.size(), k, expected.active_lane_mask, mode),
                         "public front fails scalar pair coverage oracle");
            for (unsigned lane = 0; lane < 3; ++lane) {
              gate.covered_lane_pairs += expected.work.residual_pair_mass[lane];
              gate.rejected_lane_pairs += expected.work.rejected_pair_mass[lane];
            }
            for (const std::size_t width : {1U, 2U, 8U, 12U, 64U}) {
              Records actual;
              const auto result = audit_tasks::run_partitioned(*index, k, 8, mode,
                  [&](const WspdRectangle& r) { actual.emplace_back(r.a_node, r.b_node, r.lane_mask); }, mask, width);
              const Records ordered = actual;
              if (width == 1) {
                gate.require(ordered == reference_order, "width-one changed the monolithic DFS order");
              }
              std::sort(actual.begin(), actual.end());
              gate.require(actual == reference, "partition changed oriented rectangle multiset");
              gate.require(result.front.total_unordered_pairs == expected.total_unordered_pairs &&
                               result.front.active_lane_mask == expected.active_lane_mask &&
                               same_work(result.front.work, expected.work), "partition changed front work or masks");
              gate.require(cover(gate, *index, actual, expected.active_lane_mask) == expected_cover,
                           "partition changed pair and lane cover");
              check_jobs(gate, *index, result, width);
              if (points.size() == 7 && k == 2 && mask == 3 &&
                  mode == WspdFrontMode::MidpointSamples && width == 12) {
                u64 reduced = 0;
                for (const auto& job : result.jobs) reduced += static_cast<u64>(job.seed.mask == 1);
                gate.require(result.prefix.emitted_rectangles > 0 && reduced >= 2,
                             "seven-site inherited-mask and emitted-prefix fixture is vacuous");
              }
              gate.rectangles += actual.size(); ++gate.partition_runs;
              if (!work_mutant) {
                auto changed = result.front.work; ++changed.witness_descent_steps;
                gate.require(!same_work(changed, expected.work), "work mutant escaped");
                work_mutant = true; ++gate.model_mutants;
              }
              if (!duplicate_mutant && !actual.empty()) {
                auto changed = actual; changed.push_back(actual.front());
                gate.require(!valid_cover(cover(gate, *index, changed, expected.active_lane_mask), truth,
                                          points.size(), k, expected.active_lane_mask, mode), "duplicate mutant escaped");
                duplicate_mutant = true; ++gate.model_mutants;
              }
              if (!omission_mutant && result.prefix.emitted_rectangles != 0 && !actual.empty()) {
                auto changed = actual;
                const auto prefix_record = std::find(changed.begin(), changed.end(), ordered.front());
                gate.require(prefix_record != changed.end(), "recorded prefix event disappeared");
                changed.erase(prefix_record);
                gate.require(changed != reference, "prefix-emission omission mutant escaped");
                omission_mutant = true; ++gate.model_mutants;
              }
              for (std::size_t i = 0; !mask_mutant && i < actual.size(); ++i) {
                if (std::get<2>(actual[i]) == expected.active_lane_mask) continue;
                auto changed = actual; std::get<2>(changed[i]) = expected.active_lane_mask;
                std::sort(changed.begin(), changed.end());
                gate.require(changed != reference, "restored inherited-mask mutant escaped");
                mask_mutant = true; ++gate.model_mutants;
              }
            }
          }
        }
      }
    }
  }
}

void rejections(Gate& gate) {
  const auto index = make_q2_cloud_index(prepare_cloud(std::vector<Point3>{{0, 0, 0}, {10, 0, 0}, {5, 0, 0}}));
  u64 invalid_callbacks = 0;
  const WspdRectangleConsumer consumer = [&](const WspdRectangle&) { ++invalid_callbacks; };
  const auto call = [&](unsigned k, unsigned s, WspdFrontMode mode,
                        const WspdRectangleConsumer& callback, std::uint8_t mask, std::size_t width) {
    static_cast<void>(audit_tasks::run_partitioned(*index, k, s, mode, callback, mask, width));
  };
  for (const unsigned k : {0U, 11U}) gate.rejects([&] { call(k, 8, WspdFrontMode::Pure, consumer, 1, 8); });
  gate.rejects([&] { call(2, 0, WspdFrontMode::Pure, consumer, 1, 8); });
  gate.rejects([&] { call(2, 8, static_cast<WspdFrontMode>(99), consumer, 1, 8); });
  gate.rejects([&] { call(2, 8, WspdFrontMode::Pure, {}, 1, 8); });
  gate.rejects([&] { call(2, 8, WspdFrontMode::Pure, consumer, 1, 0); });
  for (const std::uint8_t mask : {std::uint8_t{0}, std::uint8_t{8}, std::uint8_t{4}})
    gate.rejects([&] { call(2, 8, WspdFrontMode::Pure, consumer, mask, 8); });
  gate.rejects([&] { call(1, 8, WspdFrontMode::Pure, consumer, 2, 8); });
  gate.require(invalid_callbacks == 0, "invalid options emitted before rejection");
  struct CallbackStopped {};
  for (const std::size_t width : {1U, 64U}) {
    u64 callbacks = 0;
    bool caught = false;
    try {
      call(2, 8, WspdFrontMode::Pure, [&](const WspdRectangle&) {
        ++callbacks; throw CallbackStopped{};
      }, 3, width);
    } catch (const CallbackStopped&) { caught = true; }
    gate.require(caught && callbacks == 1, "callback failure not propagated exactly once");
    ++gate.callback_exceptions;
  }
}
}  // namespace

int main() {
  try {
    Gate gate;
    corpus(gate); rejections(gate);
    gate.require(gate.inherited_mask_jobs > 0 && gate.positive_depth_jobs > 0 && gate.diagonal_jobs > 0,
                 "job provenance coverage is vacuous");
    gate.require(gate.prefix_emission_runs > 0 && gate.prefix_and_jobs_runs > 0 && gate.exhausted_prefix_runs > 0,
                 "prefix emission or exhaustion coverage is vacuous");
    gate.require(gate.nonidentity_orders > 0 && gate.covered_lane_pairs > 0 && gate.rejected_lane_pairs > 0 &&
                     gate.model_mutants == 4 && gate.invalid_inputs == 10 && gate.callback_exceptions == 2,
                 "oracle or rejection coverage is vacuous");
    std::cout << "{\"status\":\"passed\",\"scope\":\"front_only_bounded_continuations\"";
#define OUTPUT(field) std::cout << ",\"" #field "\":" << gate.field
    OUTPUT(checks); OUTPUT(clouds); OUTPUT(public_runs); OUTPUT(partition_runs); OUTPUT(rectangles);
    OUTPUT(oracle_point_tests); OUTPUT(covered_lane_pairs); OUTPUT(rejected_lane_pairs);
    OUTPUT(inherited_mask_jobs); OUTPUT(positive_depth_jobs); OUTPUT(diagonal_jobs);
    OUTPUT(prefix_emission_runs); OUTPUT(prefix_and_jobs_runs); OUTPUT(exhausted_prefix_runs);
    OUTPUT(nonidentity_orders); OUTPUT(invalid_inputs); OUTPUT(callback_exceptions); OUTPUT(model_mutants);
#undef OUTPUT
    std::cout << "}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "front task gate: " << error.what() << '\n';
    return 1;
  }
}
