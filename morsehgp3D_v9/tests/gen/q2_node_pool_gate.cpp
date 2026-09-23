#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <set>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "pipeline/q2_node_pool.hpp"

namespace {
using mhgp9::gen::Point3;
using mhgp9::gen::Range;
using mhgp9::gen::u64;
using Plan = mhgp9::gen::detail::Q2NodePoolPlan;
using Points = std::vector<Point3>;
using Pair = std::pair<std::size_t, std::size_t>;
using Pairs = std::set<Pair>;

struct Gate {
  u64 checks{}, plans{}, pairs{}, oracle_corners{}, strict_rejections{}, candidates{};
  u64 empty_plans{}, saturated{}, jobs{}, jobs_pairs{}, factor_sites{}, fixed_rescans{}, growing_rescans{};
  u64 model_mutants{}, invalid_inputs{};
  void require(bool ok, const char* message) {
    ++checks;
    if (!ok) throw std::runtime_error(message);
  }
  template<class Error = std::invalid_argument, class Function>
  void rejects(Function function) {
    bool caught = false;
    try { function(); } catch (const Error&) { caught = true; }
    require(caught, "invalid node Pool operation did not reject with its contracted exception");
    ++invalid_inputs;
  }
};

// Independent scalar geometry; no product predicates or bound helpers.
std::int64_t h(const Point3& a, const Point3& b, const Point3& z) {
  std::int64_t value = 0;
  for (std::size_t d = 0; d < 3; ++d)
    value += (std::int64_t{z[d]} - a[d]) * (std::int64_t{b[d]} - z[d]);
  return value;
}

std::vector<std::uint8_t> credits(Gate& gate, const mhgp9::gen::Q2CensusIndex& index,
                                 std::size_t own, std::size_t opposite, unsigned k) {
  const auto nodes = index.spatial_nodes();
  const auto order = index.spatial_order();
  const auto points = index.cloud().points();
  const auto range = nodes[own].range;
  std::vector<std::pair<std::int64_t, std::size_t>> proposals;
  for (auto rank = range.first; rank < range.last; ++rank) {
    const auto id = order[rank];
    std::int64_t score = 0;
    for (std::size_t d = 0; d < 3; ++d)
      score += (std::int64_t{nodes[opposite].box.low[d]} + nodes[opposite].box.high[d] -
                nodes[own].box.low[d] - nodes[own].box.high[d]) * points[id][d];
    proposals.emplace_back(score, id);
  }
  std::sort(proposals.begin(), proposals.end(), [](const auto& a, const auto& b) {
    return a.first != b.first ? a.first > b.first : a.second < b.second;
  });
  proposals.resize(std::min(proposals.size(), static_cast<std::size_t>(k) + 1));
  std::vector<std::uint8_t> result(range.size(), 0);
  for (auto rank = range.first; rank < range.last; ++rank) {
    for (const auto& proposal : proposals) {
      if (proposal.second == order[rank]) continue;
      bool strict = true;
      for (unsigned mask = 0; mask < 8; ++mask) {
        std::array<mhgp9::gen::Coordinate, 3> coordinates{};
        for (std::size_t d = 0; d < 3; ++d)
          coordinates[d] = (mask & (1U << d)) != 0 ? nodes[opposite].box.high[d] : nodes[opposite].box.low[d];
        const Point3 corner{coordinates[0], coordinates[1], coordinates[2]};
        strict = (h(points[order[rank]], corner, points[proposal.second]) > 0) && strict;
        ++gate.oracle_corners;
      }
      if (strict && ++result[rank - range.first] == k) break;
    }
  }
  return result;
}

auto work(const mhgp9::gen::detail::Q2NodePoolWork& w) {
  return std::array{w.factor_visits, w.selection_point_visits, w.selection_tests, w.pool_selected,
      w.pool_insertions, w.pool_shifted_entries, w.certification_anchor_visits, w.witness_attempts,
      w.group_credit_visits, w.group_scatter_visits, w.prefix_class_visits, w.prefix_anchor_visits,
      w.predicates.universal_queries, w.predicates.q2_axis_terms};
}

Pairs check(Gate& gate, const mhgp9::gen::Q2CensusIndex& index, std::size_t an, std::size_t bn, unsigned k) {
  const auto nodes = index.spatial_nodes();
  const auto order = index.spatial_order();
  const auto points = index.cloud().points();
  const auto a = nodes[an].range, b = nodes[bn].range;
  const auto ac = credits(gate, index, an, bn, k), bc = credits(gate, index, bn, an, k);
  const auto* original_points = points.data();
  const auto* original_order = order.data();
  const auto original_visits = index.work().point_visits;
  const Plan plan(index, an, bn, k);
  gate.require(&plan.index() == &index && plan.a_node() == an && plan.b_node() == bn && plan.kmax() == k,
               "node plan lost its index, factor or threshold identity");
  gate.require(plan.a_range().first == a.first && plan.a_range().last == a.last &&
                   plan.b_range().first == b.first && plan.b_range().last == b.last,
               "node plan confused original IDs and spatial ranges");
  gate.require(std::vector<std::uint8_t>(plan.a_credits().begin(), plan.a_credits().end()) == ac &&
                   std::vector<std::uint8_t>(plan.b_credits().begin(), plan.b_credits().end()) == bc,
               "node Pool credits differ from independent projection/corner oracle");
  std::vector<std::size_t> ar(a.size()), br(b.size());
  std::iota(ar.begin(), ar.end(), a.first);
  std::iota(br.begin(), br.end(), b.first);
  std::stable_sort(ar.begin(), ar.end(), [&](auto x, auto y) { return ac[x - a.first] < ac[y - a.first]; });
  std::stable_sort(br.begin(), br.end(), [&](auto x, auto y) { return bc[x - b.first] < bc[y - b.first]; });
  std::vector<std::size_t> bids;
  for (const auto rank : br) bids.push_back(order[rank]);
  gate.require(std::vector<std::size_t>(plan.a_ranks().begin(), plan.a_ranks().end()) == ar &&
                   std::vector<std::size_t>(plan.b_order().begin(), plan.b_order().end()) == bids,
               "stable grouping confused A ranks or B original IDs");
  Pairs emitted, expected, jobs;
  std::size_t offset = 0, max_prefix = 0;
  unsigned bands = 0;
  for (unsigned c = 0; c <= k; ++c) {
    const auto group = plan.a_groups()[c];
    const auto size = static_cast<std::size_t>(std::count(ac.begin(), ac.end(), c));
    const auto prefix = static_cast<std::size_t>(std::count_if(bc.begin(), bc.end(),
                                            [&](unsigned cb) { return c + cb < k; }));
    gate.require(group.first == offset && group.last == offset + size && plan.prefix_for_credit(c) == prefix,
                 "credit class or candidate prefix differs from its exact population");
    offset += size;
    if (size != 0) max_prefix = std::max(max_prefix, prefix);
    if (size != 0 && prefix != 0) ++bands;
    for (auto position = group.first; position < group.last; ++position) {
      const auto rank = plan.a_ranks()[position];
      gate.require(plan.prefix_for_a_rank(rank) == prefix, "anchor does not use its parent class prefix");
      for (std::size_t j = 0; j < prefix; ++j)
        gate.require(emitted.emplace(order[rank], plan.b_order()[j]).second, "overlapping Pool bands repeated a pair");
    }
  }
  gate.require(offset == a.size() && bands <= k && plan.max_prefix() == max_prefix,
               "group partition, band bound or maximum useful prefix failed");
  for (auto ai = a.first; ai < a.last; ++ai) for (auto bi = b.first; bi < b.last; ++bi) {
    unsigned depth = 0;
    for (const auto& z : points) depth += h(points[order[ai]], points[order[bi]], z) > 0;
    const unsigned credit = ac[ai - a.first] + bc[bi - b.first];
    gate.require(credit <= depth, "sum of disjoint local credits exceeds the global strict population");
    if (credit < k) expected.emplace(order[ai], order[bi]);
    else { gate.require(depth >= k, "Pool rejected an oracle-admissible pair"); ++gate.strict_rejections; }
    ++gate.pairs;
  }
  gate.require(expected == emitted && plan.candidate_pairs() == emitted.size() &&
                   plan.total_pairs() == a.size() * b.size(), "Pool residual coverage or mass failed");
  // A shortest cross pair has no strict witness in A union B: such a
  // witness would produce a shorter cross pair. Local-only Pool cannot
  // remove every pair, although the subsequent GLOBAL census can do so.
  gate.require(plan.candidate_pairs() >= 1, "local-only Pool removed the shortest cross-pair witness");
  const auto before = work(plan.work());
  // Simulated jobs partition parent output positions. They never reconstruct
  // a plan, copy B, or reinterpret a prefix as a global spatial subtree.
  for (std::size_t job = 0; job < 3; ++job) {
    ++gate.jobs;
    for (auto pos = ar.size() * job / 3; pos < ar.size() * (job + 1) / 3; ++pos) {
      const auto rank = plan.a_ranks()[pos];
      for (std::size_t j = 0; j < plan.prefix_for_a_rank(rank); ++j)
        gate.require(jobs.emplace(order[rank], plan.b_order()[j]).second, "parent jobs repeated a pair");
    }
  }
  gate.require(jobs == emitted && work(plan.work()) == before, "parent jobs changed residual or repeated preparation work");
  const auto f = static_cast<u64>(a.size() + b.size());
  const auto& w = plan.work();
  gate.require(w.factor_visits == 2 * f && w.selection_point_visits == f && w.certification_anchor_visits == f &&
                   w.group_credit_visits == f && w.group_scatter_visits == f && w.prefix_class_visits == k + 1 &&
                   w.prefix_anchor_visits == 0 && w.selection_tests <= (k + 1) * f &&
                   w.witness_attempts <= (k + 1) * f && w.pool_insertions <= f &&
                   w.pool_shifted_entries <= (k + 1) * w.pool_insertions &&
                   w.pool_selected == std::min(a.size(), static_cast<std::size_t>(k) + 1) +
                                      std::min(b.size(), static_cast<std::size_t>(k) + 1) &&
                   w.predicates.universal_queries <= w.witness_attempts &&
                   w.predicates.q2_axis_terms <= 3 * w.predicates.universal_queries,
               "node Pool escaped its O(KF) selection, certification or grouping ledger");
  gate.require(index.cloud().points().data() == original_points && index.spatial_order().data() == original_order &&
                   index.work().point_visits == original_visits && plan.retained_bytes() >= f * (sizeof(std::size_t) + 1),
               "node Pool changed the immutable index or lost its owned grouping storage");
  gate.jobs_pairs += jobs.size();
  gate.factor_sites += f;
  gate.candidates += emitted.size();
  gate.empty_plans += emitted.empty();
  gate.saturated += std::count(ac.begin(), ac.end(), k) + std::count(bc.begin(), bc.end(), k);
  ++gate.plans;
  return emitted;
}

std::size_t node(const mhgp9::gen::Q2CensusIndex& index, std::vector<std::size_t> ids) {
  std::sort(ids.begin(), ids.end());
  for (std::size_t id = 0; id < index.spatial_nodes().size(); ++id) {
    const auto r = index.spatial_nodes()[id].range;
    std::vector<std::size_t> found(index.spatial_order().begin() + r.first, index.spatial_order().begin() + r.last);
    std::sort(found.begin(), found.end());
    if (found == ids) return id;
  }
  throw std::runtime_error("declared fixture factor is not a spatial node");
}

void targeted(Gate& gate) {
  const auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(Points{{0, 0, 0}, {1, 0, 0}, {100, 0, 0}}));
  const auto an = node(*index, {0, 1}), bn = node(*index, {2});
  const auto parent = check(gate, *index, an, bn, 1);
  gate.require(parent == Pairs{{1, 2}}, "parent Pool lost the useful sibling anchor witness");
  Pairs rebuilt;
  for (const std::size_t aid : {0U, 1U}) {
    const auto child = check(gate, *index, node(*index, {aid}), bn, 1);
    rebuilt.insert(child.begin(), child.end());
  }
  gate.require(rebuilt == Pairs{{0, 2}, {1, 2}} && rebuilt != parent,
               "reconstructing jobs model no longer demonstrates repeated work and changed candidates");
  ++gate.model_mutants;
  const auto permutation = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(Points{{100, 0, 0}, {0, 1, 0}, {1, 0, 1}}));
  const auto pa = node(*permutation, {0}), pb = node(*permutation, {1, 2});
  const Plan plan(*permutation, pa, pb, 1);
  gate.require(plan.b_order()[0] == 2 && permutation->spatial_order()[plan.b_range().first] == 1 &&
                   check(gate, *permutation, pa, pb, 1) == Pairs{{0, 2}}, "rank/ID counterfixture became vacuous");
  ++gate.model_mutants;
  const auto copied_owner = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(Points{{100, 0, 0}, {0, 1, 0}, {1, 0, 1}}));
  const Plan same_coordinates(*copied_owner, pa, pb, 1);
  const Plan other_k(*permutation, pa, pb, 2);
  gate.require(&same_coordinates.index() != &plan.index() && other_k.kmax() != plan.kmax(),
               "identity model equated a copied owner or different threshold");
  // No public consumer adopts arbitrary plans: these are identity models,
  // not fictional API rejection tests for bare numerically equal handles.
  ++gate.model_mutants;
  for (const unsigned k : {0U, 11U}) gate.rejects([&] { static_cast<void>(Plan(*index, an, bn, k)); });
  gate.rejects([&] { static_cast<void>(Plan(*index, index->spatial_nodes().size(), bn, 1)); });
  gate.rejects([&] { static_cast<void>(Plan(*index, an, an, 1)); });
  gate.rejects([&] { static_cast<void>(Plan(*index, 0, bn, 1)); });
  gate.rejects([&] { static_cast<void>(plan.prefix_for_credit(2)); });
  gate.rejects([&] { static_cast<void>(plan.prefix_for_a_rank(plan.b_range().first)); });
}

void exercise(Gate& gate, const std::vector<Points>& clouds) {
  for (auto points : clouds) for (unsigned permutation = 0; permutation < 2; ++permutation) {
    if (permutation != 0) std::reverse(points.begin(), points.end());
    const auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
    const auto nodes = index->spatial_nodes();
    for (std::size_t a = 0; a < nodes.size(); ++a) for (std::size_t b = 0; b < nodes.size(); ++b)
      if (nodes[a].range.last <= nodes[b].range.first)
        for (const unsigned k : {1U, 2U, 5U, 10U}) static_cast<void>(check(gate, *index, a, b, k));
  }
}

void corpus(Gate& gate) {
  std::vector<Points> clouds{
      {{0, 0, 0}, {1, 0, 0}, {99, 0, 0}, {100, 0, 0}},
      {{0, 0, 0}, {1, 0, 0}, {100, 0, 0}},
      {{100, 0, 0}, {0, 1, 0}, {1, 0, 1}},
      {{1000, 0, 0}, {0, 1, 0}, {0, 0, 0}},
      {{0, 0, 0}, {100, 0, 0}, {50, 0, 0}}};
  Points cube;
  for (unsigned bits = 0; bits < 8; ++bits)
    cube.push_back({static_cast<mhgp9::gen::Coordinate>((bits & 1U) * 65535),
                   static_cast<mhgp9::gen::Coordinate>(((bits >> 1U) & 1U) * 65535),
                   static_cast<mhgp9::gen::Coordinate>(((bits >> 2U) & 1U) * 65535)});
  clouds.push_back(cube);
  Points random;
  std::uint32_t state = 571U;
  for (unsigned i = 0; i < 11; ++i) {
    state = state * 1664525U + 1013904223U;
    const auto y = static_cast<mhgp9::gen::Coordinate>(state >> 16U);
    state = state * 1664525U + 1013904223U;
    random.push_back({static_cast<mhgp9::gen::Coordinate>(i * 251), y, static_cast<mhgp9::gen::Coordinate>(state >> 16U)});
  }
  clouds.push_back(random);
  exercise(gate, clouds);
}

// 18-bit twins (coordinate_limit = 262143): the full-extent corner cube and
// a random cloud drawn on 18 bits (state >> 14), judged by the same
// projection/corner oracle over the same node pairs, permutations and
// thresholds as the u16 corpus, whose fixtures and floors are untouched.
void corpus_18bits(Gate& gate) {
  constexpr mhgp9::gen::Coordinate limit = mhgp9::gen::coordinate_limit;
  std::vector<Points> clouds;
  Points cube;
  for (unsigned bits = 0; bits < 8; ++bits)
    cube.push_back({(bits & 1U) != 0 ? limit : 0, (bits & 2U) != 0 ? limit : 0, (bits & 4U) != 0 ? limit : 0});
  clouds.push_back(cube);
  Points random;
  std::uint32_t state = 571U;
  for (unsigned i = 0; i < 11; ++i) {
    state = state * 1664525U + 1013904223U;
    const auto y = static_cast<mhgp9::gen::Coordinate>(state >> 14U);
    state = state * 1664525U + 1013904223U;
    random.push_back({static_cast<mhgp9::gen::Coordinate>(i * 23831), y, static_cast<mhgp9::gen::Coordinate>(state >> 14U)});
  }
  clouds.push_back(random);
  for (const auto& points : clouds) {
    mhgp9::gen::Coordinate widest = 0;
    for (const auto& point : points) widest = std::max({widest, point.x, point.y, point.z});
    gate.require(widest > 65535 && widest <= limit,
                 "18-bit fixture does not leave the historical u16 range or exceeds coordinate_limit");
  }
  exercise(gate, clouds);
}

void repeated_cost(Gate& gate) {
  for (const std::size_t n : {8U, 16U, 32U}) {
    Points points;
    std::vector<std::size_t> aids, bids;
    for (std::size_t i = 0; i < n; ++i) {
      aids.push_back(points.size());
      points.push_back({0, static_cast<mhgp9::gen::Coordinate>(3 * i), 0});
    }
    for (std::size_t i = 0; i < n; ++i) {
      bids.push_back(points.size());
      points.push_back({static_cast<mhgp9::gen::Coordinate>(10000 + i), 0, 0});
    }
    const auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
    const auto an = node(*index, aids), bn = node(*index, bids);
    static_cast<void>(check(gate, *index, an, bn, 2));
    for (const auto r : {std::size_t{4}, n}) {
      const auto before = gate.factor_sites;
      for (std::size_t i = 0; i < r; ++i)
        static_cast<void>(check(gate, *index, node(*index, {i}), bn, 2));
      const auto f = gate.factor_sites - before;
      gate.require(f == r * (n + 1), "independent plans hid their repeated scans of shared B");
      if (r == 4) gate.fixed_rescans += f;
      else gate.growing_rescans += f;
    }
  }
  gate.require(gate.fixed_rescans == 236 && gate.growing_rescans == 1400,
               "fixed versus growing rectangle count lost its explicit F cost");
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp9_gen_q2_node_pool_gate --selftest\n";
    return 2;
  }
  try {
    static_assert(!std::is_copy_constructible_v<Plan> && !std::is_copy_assignable_v<Plan> &&
                  !std::is_move_assignable_v<Plan> && !std::is_move_constructible_v<Plan>);
    Gate gate;
    corpus(gate);
    targeted(gate);
    repeated_cost(gate);
    gate.require(gate.plans > 500 && gate.pairs > 2000 && gate.oracle_corners > 10000 &&
                     gate.strict_rejections > 0 && gate.candidates > 0 && gate.empty_plans == 0 && gate.saturated > 0 &&
                     gate.jobs == 3 * gate.plans && gate.jobs_pairs == gate.candidates &&
                     gate.model_mutants == 3 && gate.invalid_inputs == 7,
                 "node Pool qualification lost a non-vacuity floor");
    // 18-bit corpus: separate floors on its increments; the global identities
    // (jobs, jobs_pairs, empty_plans) must keep holding on the totals.
    const Gate u16 = gate;
    corpus_18bits(gate);
    gate.require(gate.plans > u16.plans && gate.pairs > u16.pairs && gate.oracle_corners > u16.oracle_corners &&
                     gate.strict_rejections > u16.strict_rejections && gate.candidates > u16.candidates &&
                     gate.saturated > u16.saturated && gate.empty_plans == 0 &&
                     gate.jobs == 3 * gate.plans && gate.jobs_pairs == gate.candidates &&
                     gate.fixed_rescans == u16.fixed_rescans && gate.growing_rescans == u16.growing_rescans &&
                     gate.model_mutants == u16.model_mutants && gate.invalid_inputs == u16.invalid_inputs,
                 "18-bit node Pool corpus lost a non-vacuity floor");
    std::cout << "mhgp9_gen_q2_node_pool_gate passed checks=" << gate.checks << " plans=" << gate.plans
              << " pairs=" << gate.pairs << " oracle_corners=" << gate.oracle_corners
              << " rejected=" << gate.strict_rejections << " candidates=" << gate.candidates
              << " empty_plans=" << gate.empty_plans << " saturated=" << gate.saturated
              << " jobs=" << gate.jobs << " jobs_pairs=" << gate.jobs_pairs << " factor_sites=" << gate.factor_sites
              << " fixed_rescans=" << gate.fixed_rescans << " growing_rescans=" << gate.growing_rescans
              << " model_mutants=" << gate.model_mutants << " invalid_inputs=" << gate.invalid_inputs << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp9_gen_q2_node_pool_gate failed: " << error.what() << '\n';
    return 1;
  }
}
