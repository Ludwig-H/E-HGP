#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "pipeline/wspd_q2_batched.hpp"
#include "pipeline/wspd_q2_cooperative.hpp"
#include "pipeline/wspd_q2_ranges.hpp"

// Gate of the widened proposal window of the WSPD front (WspdFrontProposals).
// Three independent judges, none of which calls the front's own predicates:
//  1. an exhaustive bounded judge of the window arithmetic;
//  2. a q2-only REPLAY of the whole front traversal, written from the
//     contract (one-path descent, historical window, two disjoint extra
//     intervals left first, persistent credits, factor-rank skips, strict
//     H>0 by brute force over the 64 box-corner pairs), compared counter for
//     counter and rectangle for rectangle with the engine; ten causal mutants
//     of the EXTENSION must each disagree with the engine on the engraved
//     corpus, named minimal fixtures carry their own exact expectations, and
//     pre-tranche constants pin the historical window;
//  3. the brute-force q2 support oracle (strict interiors, full shells)
//     against the five q2 entries with widened windows.
namespace {
using mhgp8::Point3;
using mhgp8::Q2CensusIndex;
using mhgp8::Q2SiblingMode;
using mhgp8::Q2SpatialNode;
using mhgp8::Q2WitnessOrder;
using mhgp8::WspdFrontMode;
using mhgp8::WspdFrontProposals;
using mhgp8::u64;
using i64 = std::int64_t;
using Points = std::vector<Point3>;
constexpr auto unlimited = std::numeric_limits<std::size_t>::max();

struct Gate {
  u64 checks{}, windows{}, clamped_left{}, clamped_right{}, whole_permutation_windows{};
  u64 clouds{}, replay_runs{}, replay_products{}, replay_rectangles{};
  u64 tangent_extension_sites{}, factor_ranks_in_extension{}, truncated_windows{}, exhausted_extensions{};
  u64 kth_witness_in_extension{}, equal_distance_ties{}, limit_skips{}, extended_products{};
  u64 extension_clamped_left{}, extension_clamped_right{}, truncated_double_windows{};
  u64 rejections_with_historical_credit{}, singleton_tangent_extension_sites{}, singleton_kth_in_extension{};
  u64 named_fixture_checks{}, historical_constant_checks{};
  u64 mutants{}, mutant_disagreements{};
  std::array<u64, 10> disagreements_by_mutant{};
  u64 oracle_pairs{}, oracle_sites{}, entry_runs{}, supports{}, max_shell{}, entry_extended_products{};
  u64 invalid_inputs{};
  void require(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
  }
  template<class Function> void rejects(Function&& function, const char* message) {
    bool caught = false;
    try { function(); } catch (const std::invalid_argument&) { caught = true; }
    require(caught, message);
    ++invalid_inputs;
  }
};

// ---------------------------------------------------------------- 1. windows
void window_judge(Gate& gate) {
  for (std::size_t n = 1; n <= 48; ++n) for (unsigned k = 1; k <= 10; ++k)
    for (const unsigned factor : {1U, 2U, 4U}) for (std::size_t pivot = 0; pivot < n; ++pivot) {
      const auto count = std::min<std::size_t>(k, n);
      const auto wider_count = std::min<std::size_t>(std::size_t{k} * factor, n);
      const auto reference = [&](std::size_t size) {
        i64 first = static_cast<i64>(pivot) - static_cast<i64>(size / 2);
        first = std::max<i64>(first, 0);
        first = std::min<i64>(first, static_cast<i64>(n) - static_cast<i64>(size));
        return std::pair<std::size_t, std::size_t>{static_cast<std::size_t>(first), static_cast<std::size_t>(first) + size};
      };
      const auto window = mhgp8::wspd_proposal_window(pivot, count, n);
      const auto wider = mhgp8::wspd_proposal_window(pivot, wider_count, n);
      const auto expected = reference(count), expected_wider = reference(wider_count);
      gate.require(window.first == expected.first && window.last == expected.second &&
                       wider.first == expected_wider.first && wider.last == expected_wider.second,
                   "proposal window differs from the signed clamped reference");
      gate.require(window.last - window.first == count && wider.last - wider.first == wider_count &&
                       wider.last <= n && window.first <= pivot && pivot < window.last,
                   "proposal window has a wrong size, leaves the permutation or loses its pivot");
      gate.require(wider.first <= window.first && window.last <= wider.last &&
                       (window.first - wider.first) + (wider.last - window.last) == wider_count - count,
                   "widened window does not contain the historical window");
      std::vector<unsigned> seen(n);
      for (auto rank = window.first; rank < window.last; ++rank) ++seen[rank];
      for (auto rank = wider.first; rank < window.first; ++rank) ++seen[rank];
      for (auto rank = window.last; rank < wider.last; ++rank) ++seen[rank];
      for (std::size_t rank = 0; rank < n; ++rank)
        gate.require(seen[rank] == static_cast<unsigned>(wider.first <= rank && rank < wider.last),
                     "historical window and its two extra intervals duplicate or lose a rank");
      gate.clamped_left += static_cast<u64>(pivot < wider_count / 2);
      gate.clamped_right += static_cast<u64>(pivot + (wider_count - wider_count / 2) > n);
      gate.whole_permutation_windows += static_cast<u64>(wider_count == n);
      ++gate.windows;
    }
}

// ----------------------------------------------------------------- 2. replay
// Causal faults of the EXTENSION only: the historical window is judged apart,
// by the factor-1 equalities and by the engraved pre-tranche constants.
enum class Mutant { None, CreditTangentInExtension, TestFactorRanksInExtension, RightIntervalFirst, ResetCredits,
                    IgnoreLimit, ShiftedWindow, LimitOnMinimum, NoEarlyStop, InheritCredits, WiderScanInOrder };

struct Replay {
  u64 product_visits{}, fully_rejected{}, emitted{}, separation_tests{}, searches{}, descent_steps{};
  u64 box_distance_tests{}, proposed{}, in_factors{}, h_tests{}, credits{};
  u64 extended_products{}, extended_proposals{}, extended_in_factors{}, extended_credits{};
  u64 extended_rejections{}, rejected_mass{}, residual_mass{};
  u64 tangent_extension_sites{}, factor_ranks_in_extension{}, truncated_windows{}, exhausted_extensions{};
  u64 equal_distance_ties{}, limit_skips{};
  u64 extension_clamped_left{}, extension_clamped_right{}, truncated_double_windows{};
  u64 rejections_with_historical_credit{}, singleton_tangent_extension_sites{}, singleton_kth_in_extension{};
  std::vector<std::pair<std::size_t, std::size_t>> rectangles;
};

i64 diagonal2(const mhgp8::Box3& box) {
  i64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 delta = i64{box.high[axis]} - box.low[axis];
    result += delta * delta;
  }
  return result;
}
i64 gap2(const mhgp8::Box3& a, const mhgp8::Box3& b) {
  i64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 delta = std::max<i64>({0, i64{a.low[axis]} - b.high[axis], i64{b.low[axis]} - a.high[axis]});
    result += delta * delta;
  }
  return result;
}
// Minimum of H=(z-a).(b-z) over both boxes by brute force over the 8x8
// corner pairs: H is affine in a for fixed b and in b for fixed a.
i64 corner_h_minimum(const mhgp8::Box3& a, const mhgp8::Box3& b, const Point3& z) {
  i64 best = std::numeric_limits<i64>::max();
  for (unsigned ca = 0; ca < 8; ++ca) for (unsigned cb = 0; cb < 8; ++cb) {
    i64 h = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const i64 av = ((ca >> axis) & 1U) != 0 ? a.high[axis] : a.low[axis];
      const i64 bv = ((cb >> axis) & 1U) != 0 ? b.high[axis] : b.low[axis];
      h += (i64{z[axis]} - av) * (bv - i64{z[axis]});
    }
    best = std::min(best, h);
  }
  return best;
}

Replay replay_front(const Q2CensusIndex& index, unsigned k, unsigned s, WspdFrontProposals proposals, Mutant mutant) {
  const auto nodes = index.spatial_nodes();
  const auto order = index.spatial_order();
  const auto points = index.cloud().points();
  const auto n = points.size();
  Replay out;
  const auto inside = [](mhgp8::Range range, std::size_t rank) { return range.first <= rank && rank < range.last; };
  // `inherited` is read by one mutant only; `final_credits` reports the credits of this call.
  const auto sampled_rejection = [&](const Q2SpatialNode& a, const Q2SpatialNode& b, unsigned inherited,
                                     unsigned& final_credits) {
    final_credits = 0;
    if (n - a.range.size() - b.range.size() < k) return false;
    ++out.searches;
    std::array<i64, 3> center4{};
    for (std::size_t axis = 0; axis < 3; ++axis)
      center4[axis] = i64{a.box.low[axis]} + a.box.high[axis] + b.box.low[axis] + b.box.high[axis];
    const auto distance = [&](const mhgp8::Box3& box) {
      i64 result = 0;
      for (std::size_t axis = 0; axis < 3; ++axis) {
        const i64 below = 4 * i64{box.low[axis]} - center4[axis], above = center4[axis] - 4 * i64{box.high[axis]};
        const i64 delta = std::max<i64>({0, below, above});
        result += delta * delta;
      }
      return result;
    };
    std::size_t node = 0;
    while (nodes[node].left != Q2SpatialNode::absent) {
      ++out.descent_steps;
      out.box_distance_tests += 2;
      const auto left = distance(nodes[nodes[node].left].box), right = distance(nodes[nodes[node].right].box);
      out.equal_distance_ties += static_cast<u64>(left == right);
      node = left <= right ? nodes[node].left : nodes[node].right;
    }
    const i64 pivot = static_cast<i64>(nodes[node].range.first);
    const auto centered = [&](i64 size, i64 shift) {
      i64 first = std::min<i64>(std::max<i64>(pivot - size / 2 + shift, 0), static_cast<i64>(n) - size);
      return std::pair<i64, i64>{first, first + size};
    };
    const i64 count = std::min<i64>(k, static_cast<i64>(n));
    const auto window = centered(count, 0);
    const bool singletons = a.range.size() == 1 && b.range.size() == 1;
    unsigned credits = mutant == Mutant::InheritCredits ? std::min(inherited, k - 1) : 0;
    const bool stop_early = mutant != Mutant::NoEarlyStop;
    const auto propose = [&](i64 rank, bool extension) {
      ++out.proposed;
      const auto r = static_cast<std::size_t>(rank);
      if (inside(a.range, r) || inside(b.range, r)) {
        out.factor_ranks_in_extension += static_cast<u64>(extension);
        if (!(extension && mutant == Mutant::TestFactorRanksInExtension)) {
          ++out.in_factors;
          out.extended_in_factors += static_cast<u64>(extension);
          return;
        }
      }
      ++out.h_tests;
      const auto h = corner_h_minimum(a.box, b.box, points[order[r]]);
      out.tangent_extension_sites += static_cast<u64>(extension && h == 0);
      out.singleton_tangent_extension_sites += static_cast<u64>(extension && h == 0 && singletons);
      if ((h > 0 || (extension && mutant == Mutant::CreditTangentInExtension && h == 0)) && credits < k) {
        ++credits;
        ++out.credits;
        out.extended_credits += static_cast<u64>(extension);
      }
    };
    const bool eligible = proposals.window_factor != 1 &&
        (mutant == Mutant::IgnoreLimit ||
         (mutant == Mutant::LimitOnMinimum ? std::min(a.range.size(), b.range.size())
                                           : std::max(a.range.size(), b.range.size())) <= proposals.small_factor_limit);
    const i64 wider_count = std::min<i64>(i64{k} * proposals.window_factor, static_cast<i64>(n));
    const auto wider = centered(wider_count, mutant == Mutant::ShiftedWindow ? 1 : 0);
    if (mutant == Mutant::WiderScanInOrder && eligible) {
      // Fault: the wider window scanned in rank order, the historical window no longer first.
      bool extended = false, last_credit_outside = false;
      for (i64 rank = std::min(wider.first, window.first);
           rank < std::max(wider.second, window.second) && credits < k; ++rank) {
        const bool outside = rank < window.first || rank >= window.second;
        const auto before = credits;
        if (outside) { ++out.extended_proposals; extended = true; }
        propose(rank, outside);
        if (credits != before) last_credit_outside = outside;
      }
      out.extended_products += static_cast<u64>(extended);
      final_credits = credits;
      if (credits >= k) { out.extended_rejections += static_cast<u64>(last_credit_outside); return true; }
      return false;
    }
    for (i64 rank = window.first; rank < window.second && (!stop_early || credits < k); ++rank) propose(rank, false);
    const unsigned historical_credits = credits;
    final_credits = credits;
    if (credits >= k) return true;
    if (proposals.window_factor == 1) return false;
    if (!eligible) { ++out.limit_skips; return false; }
    out.truncated_windows += static_cast<u64>(wider_count < i64{k} * proposals.window_factor);
    out.truncated_double_windows += static_cast<u64>(proposals.window_factor == 2 && wider_count < 2 * i64{k});
    const std::pair<i64, i64> left{std::min(wider.first, window.first), window.first};
    const std::pair<i64, i64> right{window.second, std::max(wider.second, window.second)};
    if (left.first == left.second && right.first == right.second) return false;
    // A clamped window that is not the whole permutation: one side gives its ranks to the other.
    const bool whole = wider_count == static_cast<i64>(n);
    out.extension_clamped_left += static_cast<u64>(!whole && pivot - wider_count / 2 < 0);
    out.extension_clamped_right += static_cast<u64>(!whole && pivot - wider_count / 2 + wider_count > static_cast<i64>(n));
    ++out.extended_products;
    if (mutant == Mutant::ResetCredits) credits = 0;
    const auto run = [&](std::pair<i64, i64> interval) {
      for (i64 rank = interval.first; rank < interval.second && (!stop_early || credits < k); ++rank) {
        ++out.extended_proposals;
        propose(rank, true);
      }
    };
    if (mutant == Mutant::RightIntervalFirst) { run(right); run(left); } else { run(left); run(right); }
    final_credits = credits;
    if (credits >= k) {
      ++out.extended_rejections;
      out.rejections_with_historical_credit += static_cast<u64>(historical_credits > 0);
      out.singleton_kth_in_extension += static_cast<u64>(singletons);
      return true;
    }
    ++out.exhausted_extensions;
    return false;
  };
  struct Item { std::size_t a, b; unsigned inherited; };
  std::vector<Item> stack{{0, 0, 0}};
  while (!stack.empty()) {
    const auto [ia, ib, inherited] = stack.back();
    stack.pop_back();
    ++out.product_visits;
    const auto& a = nodes[ia];
    const auto& b = nodes[ib];
    if (ia == ib) {
      if (a.left == Q2SpatialNode::absent) continue;
      stack.push_back({a.right, a.right, 0});
      stack.push_back({a.left, a.right, 0});
      stack.push_back({a.left, a.left, 0});
      continue;
    }
    const u64 mass = static_cast<u64>(a.range.size()) * b.range.size();
    unsigned credits = 0;
    if (sampled_rejection(a, b, inherited, credits)) { ++out.fully_rejected; out.rejected_mass += mass; continue; }
    ++out.separation_tests;
    const auto da = diagonal2(a.box), db = diagonal2(b.box);
    if (static_cast<mhgp8::i128>(gap2(a.box, b.box)) >= static_cast<mhgp8::i128>(s) * s * std::max(da, db)) {
      ++out.emitted;
      out.residual_mass += mass;
      out.rectangles.emplace_back(ia, ib);
      continue;
    }
    const bool split_a = a.left != Q2SpatialNode::absent && (b.left == Q2SpatialNode::absent || da >= db);
    const auto& split = split_a ? a : b;
    if (split.left == Q2SpatialNode::absent) throw std::logic_error("replay met two unseparated distinct leaves");
    stack.push_back({split_a ? split.right : ia, split_a ? ib : split.right, credits});
    stack.push_back({split_a ? split.left : ia, split_a ? ib : split.left, credits});
  }
  std::sort(out.rectangles.begin(), out.rectangles.end());
  return out;
}

struct Observed {
  mhgp8::WspdFrontResult result;
  std::vector<std::pair<std::size_t, std::size_t>> rectangles;
};
Observed observe(const Q2CensusIndex& index, unsigned k, unsigned s, WspdFrontProposals proposals) {
  Observed out;
  out.result = mhgp8::run_wspd_front(index, k, s, WspdFrontMode::MidpointSamples,
      [&](const mhgp8::WspdRectangle& rectangle) {
        if (rectangle.lane_mask != 1) throw std::logic_error("q2-only front emitted another lane");
        out.rectangles.emplace_back(rectangle.a_node, rectangle.b_node);
      }, 1, proposals);
  std::sort(out.rectangles.begin(), out.rectangles.end());
  return out;
}
bool agrees(const Observed& engine, const Replay& replay) {
  const auto& w = engine.result.work;
  return engine.rectangles == replay.rectangles && w.product_visits == replay.product_visits &&
         w.fully_rejected_products == replay.fully_rejected && w.emitted_rectangles == replay.emitted &&
         w.separation_tests == replay.separation_tests && w.witness_searches == replay.searches &&
         w.witness_descent_steps == replay.descent_steps && w.witness_box_distance_tests == replay.box_distance_tests &&
         w.proposed_sites == replay.proposed && w.proposals_in_factors == replay.in_factors &&
         w.h_bound_tests == replay.h_tests && w.xi_bound_tests == 0 && w.witness_lane_credits == replay.credits &&
         w.extended_products == replay.extended_products && w.extended_proposals == replay.extended_proposals &&
         w.extended_proposals_in_factors == replay.extended_in_factors &&
         w.extended_credits == replay.extended_credits &&
         w.extended_rejections == replay.extended_rejections &&
         w.rejected_pair_mass == std::array<u64, 3>{replay.rejected_mass, 0, 0} &&
         w.residual_pair_mass == std::array<u64, 3>{replay.residual_mass, 0, 0};
}

// Named minimal fixtures, K = 2, judged pair (id 0, id 1) = ((0,0,0), (100,0,0)).
// K2_tangent_second: ONE strict witness (50,1,0), H = 2499, and two tangent sites, H = 0:
// the pair has depth 1 < K and must stay residual for every window.
Points k2_tangent_second() {
  return {{0, 0, 0}, {100, 0, 0}, {50, 1, 0}, {50, 50, 0}, {50, 0, 50},
          {60000, 60000, 60000}, {0, 60000, 0}, {60000, 0, 60000}};
}
// K2_second_in_extension: strict witnesses H = 2499, 900 and 99 plus one tangent site. The
// historical window of two ranks misses the second witness; only the extension finds it.
Points k2_second_in_extension() {
  return {{0, 0, 0}, {100, 0, 0}, {50, 1, 0}, {50, 40, 0}, {50, 50, 0}, {50, 0, 49},
          {60000, 60000, 60000}, {0, 60000, 0}};
}

// Engraved clouds. Coordinates are exact u16 values; none is random at run time.
std::vector<Points> fixtures() {
  std::vector<Points> clouds;
  clouds.push_back({{7, 8, 9}});
  clouds.push_back({{0, 0, 0}, {10, 0, 0}});
  clouds.push_back({{0, 0, 0}, {10, 0, 0}, {5, 4, 0}});   // One strict witness, n < every widened window.
  clouds.push_back({{0, 0, 0}, {10, 0, 0}, {5, 5, 0}});   // One tangent site: H = 0 must never credit.
  // Endpoints a, b of a diameter of the sphere of center (125,100,100) and radius 25,
  // plus integer sites exactly ON that sphere: every one is tangent for {a} x {b}.
  Points sphere{{100, 100, 100}, {150, 100, 100}};
  for (const auto& d : std::vector<std::array<int, 3>>{{0, 25, 0}, {0, -25, 0}, {0, 0, 25}, {0, 0, -25}, {-20, 15, 0},
       {-20, -15, 0}, {20, 15, 0}, {20, -15, 0}, {-15, 20, 0}, {15, -20, 0}, {-7, 24, 0}, {7, -24, 0},
       {0, 15, 20}, {0, -15, -20}, {-15, 0, 20}, {15, 0, -20}, {-7, 0, 24}, {7, 0, -24}, {0, 7, 24}, {0, -24, 7}})
    sphere.push_back({static_cast<std::uint16_t>(125 + d[0]), static_cast<std::uint16_t>(100 + d[1]),
                      static_cast<std::uint16_t>(100 + d[2])});
  clouds.push_back(sphere);
  Points line;                                              // Collinear: many strict witnesses, factor ranks adjacent.
  for (unsigned i = 0; i < 40; ++i) line.push_back({static_cast<std::uint16_t>(3 * i + (i % 3)), 50, 50});
  clouds.push_back(line);
  Points rows;                                              // Two parallel rows: almost no universal witness.
  for (unsigned i = 0; i < 24; ++i) { rows.push_back({static_cast<std::uint16_t>(10 * i), 0, 0});
                                      rows.push_back({static_cast<std::uint16_t>(10 * i + 3), 4000, 0}); }
  clouds.push_back(rows);
  Points grid;                                              // Equal distances and ties everywhere.
  for (unsigned x = 0; x < 4; ++x) for (unsigned y = 0; y < 4; ++y) for (unsigned z = 0; z < 4; ++z)
    grid.push_back({static_cast<std::uint16_t>(100 * x), static_cast<std::uint16_t>(100 * y), static_cast<std::uint16_t>(100 * z)});
  clouds.push_back(grid);
  for (const auto& [size, seed] : std::vector<std::pair<unsigned, std::uint32_t>>{{24, 5}, {48, 11}, {96, 23}}) {
    Points cloud;
    std::uint32_t state = seed;
    const auto next = [&] { state = state * 1664525U + 1013904223U; return static_cast<std::uint16_t>((state >> 16U) % 2000U); };
    for (unsigned i = 0; i < size; ++i) cloud.push_back({static_cast<std::uint16_t>(next() + 7 * i), next(), next()});
    clouds.push_back(cloud);
  }
  Points clusters;                                          // Two dense clusters and a sparse bridge.
  {
    std::uint32_t state = 77;
    const auto next = [&](unsigned span) { state = state * 1664525U + 1013904223U; return static_cast<std::uint16_t>((state >> 16U) % span); };
    for (unsigned i = 0; i < 30; ++i) clusters.push_back({static_cast<std::uint16_t>(1000 + next(60) + i), static_cast<std::uint16_t>(1000 + next(60)), static_cast<std::uint16_t>(1000 + next(60))});
    for (unsigned i = 0; i < 30; ++i) clusters.push_back({static_cast<std::uint16_t>(9000 + next(60) + i), static_cast<std::uint16_t>(1000 + next(60)), static_cast<std::uint16_t>(1000 + next(60))});
    for (unsigned i = 0; i < 12; ++i) clusters.push_back({static_cast<std::uint16_t>(2000 + 550 * i), static_cast<std::uint16_t>(1030 + (i % 2)), 1030});
  }
  clouds.push_back(clusters);
  Points fourteen;                                           // K < n < 2K for K = 10: the 2K window is truncated.
  for (unsigned i = 0; i < 14; ++i)
    fourteen.push_back({static_cast<std::uint16_t>(40 * i + (i * i) % 7), static_cast<std::uint16_t>(30 + (3 * i) % 5),
                        static_cast<std::uint16_t>(20 + (5 * i) % 3)});
  clouds.push_back(fourteen);
  clouds.push_back(k2_tangent_second());
  clouds.push_back(k2_second_in_extension());
  return clouds;
}

const std::vector<WspdFrontProposals>& variants() {
  static const std::vector<WspdFrontProposals> values{{1, unlimited}, {1, 1}, {2, unlimited}, {2, 16},
                                                      {4, unlimited}, {4, 2}, {4, 1}};
  return values;
}

bool pair_is_residual(const Q2CensusIndex& index, const Observed& engine, std::size_t id_a, std::size_t id_b) {
  const auto nodes = index.spatial_nodes();
  const auto order = index.spatial_order();
  const auto holds = [&](mhgp8::Range range, std::size_t id) {
    for (auto rank = range.first; rank < range.last; ++rank) if (order[rank] == id) return true;
    return false;
  };
  for (const auto& [a, b] : engine.rectangles)
    if ((holds(nodes[a].range, id_a) && holds(nodes[b].range, id_b)) ||
        (holds(nodes[a].range, id_b) && holds(nodes[b].range, id_a))) return true;
  return false;
}

// Exact expectations of the two named fixtures, K = 2, s = 8, q2 lane alone.
void named_fixtures(Gate& gate) {
  struct Expected { WspdFrontProposals proposals; bool residual; u64 rejected_mass, extended_rejections, extended_credits; };
  {
    const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(k2_tangent_second()));
    for (const auto& e : {Expected{{1, unlimited}, true, 1, 0, 0}, Expected{{2, unlimited}, true, 1, 0, 0},
                          Expected{{4, unlimited}, true, 1, 0, 0}, Expected{{4, 1}, true, 1, 0, 0}}) {
      const auto engine = observe(*index, 2, 8, e.proposals);
      gate.require(pair_is_residual(*index, engine, 0, 1) == e.residual &&
                       engine.result.work.rejected_pair_mass[0] == e.rejected_mass &&
                       engine.result.work.extended_rejections == e.extended_rejections &&
                       engine.result.work.extended_credits == e.extended_credits,
                   "K2_tangent_second: a tangent site was credited or the depth-1 pair was rejected");
      ++gate.named_fixture_checks;
    }
  }
  {
    const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(k2_second_in_extension()));
    for (const auto& e : {Expected{{1, unlimited}, true, 0, 0, 0}, Expected{{2, unlimited}, false, 4, 4, 6},
                          Expected{{4, unlimited}, false, 4, 4, 6}, Expected{{4, 1}, false, 4, 4, 4}}) {
      const auto engine = observe(*index, 2, 8, e.proposals);
      gate.require(pair_is_residual(*index, engine, 0, 1) == e.residual &&
                       engine.result.work.rejected_pair_mass[0] == e.rejected_mass &&
                       engine.result.work.extended_rejections == e.extended_rejections &&
                       engine.result.work.extended_credits == e.extended_credits,
                   "K2_second_in_extension: only the extension must find the second witness");
      ++gate.named_fixture_checks;
    }
  }
}

// Counters of the front BEFORE this tranche (commit 8d615cfd, pinned tranche-19 library),
// default proposals, q2 lane alone, s = 8: the historical window must not drift.
void historical_constants(Gate& gate) {
  struct Constants { u64 visits, searches, descents, proposed, in_factors, h_tests, credits, rejected, emitted, separations,
                         rejected_mass, residual_mass; };
  const auto check = [&](const Points& points, unsigned k, const Constants& c) {
    const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
    for (const auto& proposals : {WspdFrontProposals{}, WspdFrontProposals{1, 1}}) {
      const auto w = observe(*index, k, 8, proposals).result.work;
      gate.require(w.product_visits == c.visits && w.witness_searches == c.searches &&
                       w.witness_descent_steps == c.descents && w.proposed_sites == c.proposed &&
                       w.proposals_in_factors == c.in_factors && w.h_bound_tests == c.h_tests &&
                       w.witness_lane_credits == c.credits && w.fully_rejected_products == c.rejected &&
                       w.emitted_rectangles == c.emitted && w.separation_tests == c.separations &&
                       w.rejected_pair_mass[0] == c.rejected_mass && w.residual_pair_mass[0] == c.residual_mass &&
                       w.extended_products == 0 && w.extended_proposals == 0,
                   "default front drifted from its engraved pre-tranche counters");
      ++gate.historical_constant_checks;
    }
  };
  const auto clouds = fixtures();
  check(clouds[7], 5, {4064, 3936, 23616, 19680, 2794, 16886, 5824, 48, 1952, 3889, 64, 1952});   // Grid 4x4x4.
  check(clouds[7], 10, {4096, 3968, 23808, 39680, 4653, 35027, 8572, 0, 2016, 3969, 0, 2016});
  check(clouds[4], 2, {444, 400, 1995, 800, 272, 528, 229, 40, 171, 361, 60, 171});               // Sphere, 22 sites.
}

void replay_corpus(Gate& gate) {
  constexpr std::array mutants{Mutant::CreditTangentInExtension, Mutant::TestFactorRanksInExtension,
                               Mutant::RightIntervalFirst, Mutant::ResetCredits, Mutant::IgnoreLimit,
                               Mutant::ShiftedWindow, Mutant::LimitOnMinimum, Mutant::NoEarlyStop,
                               Mutant::InheritCredits, Mutant::WiderScanInOrder};
  static_assert(mutants.size() == 10);
  for (const auto& points : fixtures()) {
    const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
    ++gate.clouds;
    for (const unsigned k : {1U, 2U, 3U, 5U, 10U}) for (const unsigned s : {8U, 10U, 12U}) {
      const auto historical = observe(*index, k, s, {});
      for (const auto& proposals : variants()) {
        const auto engine = observe(*index, k, s, proposals);
        const auto replay = replay_front(*index, k, s, proposals, Mutant::None);
        gate.require(agrees(engine, replay),
                     "front differs from the independent q2 replay in rectangles or counters");
        if (proposals.window_factor == 1)
          gate.require(engine.result.work == historical.result.work && engine.rectangles == historical.rectangles,
                       "factor-1 proposals differ from the default front");
        // A wider window visits a subset of the historical products and separation is
        // purely geometric: its rectangles are a subset of the historical rectangles.
        gate.require(std::includes(historical.rectangles.begin(), historical.rectangles.end(),
                                   engine.rectangles.begin(), engine.rectangles.end()),
                     "widened window emitted a rectangle that the historical window did not emit");
        gate.require(engine.result.work.residual_pair_mass[0] <= historical.result.work.residual_pair_mass[0],
                     "widened window kept more q2 pair mass than the historical window");
        if (proposals.window_factor != 1)
          for (std::size_t m = 0; m < mutants.size(); ++m)
            gate.disagreements_by_mutant[m] +=
                static_cast<u64>(!agrees(engine, replay_front(*index, k, s, proposals, mutants[m])));
        gate.tangent_extension_sites += replay.tangent_extension_sites;
        gate.factor_ranks_in_extension += replay.factor_ranks_in_extension;
        gate.truncated_windows += replay.truncated_windows;
        gate.truncated_double_windows += replay.truncated_double_windows;
        gate.exhausted_extensions += replay.exhausted_extensions;
        gate.kth_witness_in_extension += replay.extended_rejections;
        gate.equal_distance_ties += replay.equal_distance_ties;
        gate.limit_skips += replay.limit_skips;
        gate.extended_products += replay.extended_products;
        gate.extension_clamped_left += replay.extension_clamped_left;
        gate.extension_clamped_right += replay.extension_clamped_right;
        gate.rejections_with_historical_credit += replay.rejections_with_historical_credit;
        gate.singleton_tangent_extension_sites += replay.singleton_tangent_extension_sites;
        gate.singleton_kth_in_extension += replay.singleton_kth_in_extension;
        gate.replay_products += replay.product_visits;
        gate.replay_rectangles += replay.emitted;
        ++gate.replay_runs;
      }
    }
  }
  for (const auto count : gate.disagreements_by_mutant) {
    gate.require(count > 0, "a causal extension mutant agreed with the front on the whole engraved corpus");
    gate.mutant_disagreements += count;
    ++gate.mutants;
  }
}

// ---------------------------------------------------------------- 3. entries
struct Support {
  std::size_t a{}, b{};
  mhgp8::Q2BallKey key;
  std::vector<std::size_t> interior, shell;
  bool operator==(const Support&) const = default;
};
using Output = std::vector<Support>;
Support copy(const mhgp8::Q2Support& value) {
  Support result{std::min(value.a_id, value.b_id), std::max(value.a_id, value.b_id), value.key,
                 {value.interior.begin(), value.interior.end()}, {value.shell.begin(), value.shell.end()}};
  std::sort(result.interior.begin(), result.interior.end());
  std::sort(result.shell.begin(), result.shell.end());
  return result;
}
void normalize(Output& output) {
  std::sort(output.begin(), output.end(), [](const auto& x, const auto& y) { return x.a < y.a || (x.a == y.a && x.b < y.b); });
}
Output oracle(Gate& gate, const Points& points, unsigned k) {
  gate.require(points.size() <= 100, "proposal oracle exceeded its bounded n<=100 domain");
  Output result;
  for (std::size_t a = 0; a < points.size(); ++a) for (std::size_t b = a + 1; b < points.size(); ++b) {
    Support item;
    item.a = a;
    item.b = b;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      item.key.center_twice[axis] = std::uint32_t{points[a][axis]} + points[b][axis];
      const auto delta = i64{points[a][axis]} - points[b][axis];
      item.key.diameter_squared += static_cast<u64>(delta * delta);
    }
    for (std::size_t z = 0; z < points.size(); ++z) {
      i64 h = 0;
      for (std::size_t axis = 0; axis < 3; ++axis)
        h += (i64{points[z][axis]} - points[a][axis]) * (i64{points[b][axis]} - points[z][axis]);
      if (h > 0) item.interior.push_back(z);
      if (h == 0) item.shell.push_back(z);
      ++gate.oracle_sites;
    }
    ++gate.oracle_pairs;
    if (item.interior.size() < k) result.push_back(std::move(item));
  }
  return result;
}

void entries(Gate& gate) {
  for (const auto& points : fixtures()) {
    const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
    for (const unsigned k : {2U, 5U, 10U}) {
      const auto expected = oracle(gate, points, k);
      for (const auto& proposals : {WspdFrontProposals{2, 16}, WspdFrontProposals{4, unlimited}}) {
        const auto mono_front = observe(*index, k, 8, proposals);
        const auto check = [&](Output output, const mhgp8::WspdFrontResult& front, const char* message) {
          normalize(output);
          gate.require(output == expected, message);
          gate.require(front.work == mono_front.result.work && front.active_lane_mask == 1,
                       "a q2 entry lost the widened proposals between its front and the mono front");
          for (const auto& item : output) gate.max_shell = std::max<u64>(gate.max_shell, item.shell.size());
          gate.supports += output.size();
          gate.entry_extended_products += front.work.extended_products;
          ++gate.entry_runs;
        };
        {
          Output output;
          const auto result = mhgp8::run_wspd_q2_census(*index, k, 8, WspdFrontMode::MidpointSamples,
              mhgp8::Q2CensusMode::SharedBlocks, [&](const mhgp8::Q2Support& s) { output.push_back(copy(s)); },
              Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, mhgp8::Q2AnchorMode::Individual, 0, proposals);
          check(std::move(output), result.front, "serial q2 census with widened proposals differs from the brute-force oracle");
        }
        for (const std::size_t workers : {std::size_t{1}, std::size_t{3}}) {
          const auto run = [&](auto&& call, const char* message) {
            std::vector<Output> slots(workers);
            std::vector<mhgp8::Q2CensusConsumer> consumers;
            for (std::size_t w = 0; w < workers; ++w)
              consumers.emplace_back([&slots, w](const mhgp8::Q2Support& s) { slots[w].push_back(copy(s)); });
            const auto front = call(std::span<const mhgp8::Q2CensusConsumer>(consumers));
            Output joined;
            for (auto& slot : slots) joined.insert(joined.end(), slot.begin(), slot.end());
            check(std::move(joined), front, message);
          };
          run([&](auto consumers) { return mhgp8::run_wspd_q2_census_parallel(index, k, 8, WspdFrontMode::MidpointSamples,
                  mhgp8::Q2CensusMode::SharedBlocks, consumers, 4, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst,
                  mhgp8::Q2AnchorMode::Individual, 0, {}, proposals).front; },
              "parallel Coarse q2 census with widened proposals differs from the brute-force oracle");
          run([&](auto consumers) { return mhgp8::run_wspd_q2_census_parallel(index, k, 8, WspdFrontMode::MidpointSamples,
                  mhgp8::Q2CensusMode::SharedBlocks, consumers, 4, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst,
                  mhgp8::Q2AnchorMode::Individual, 0, {mhgp8::WspdQ2ScheduleMode::Donate, 4, 1}, proposals).front; },
              "parallel Donate q2 census with widened proposals differs from the brute-force oracle");
          run([&](auto consumers) { return mhgp8::run_wspd_q2_census_cooperative(index, k, 8, WspdFrontMode::MidpointSamples,
                  consumers, {4, 4, 8, 1}, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, 0, proposals).pipeline.front; },
              "cooperative q2 census with widened proposals differs from the brute-force oracle");
          run([&](auto consumers) { return mhgp8::run_wspd_q2_census_ranges(index, k, 8, WspdFrontMode::MidpointSamples,
                  consumers, {4, 4, 2}, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, 0, proposals).pipeline.front; },
              "anchor-range q2 census with widened proposals differs from the brute-force oracle");
          run([&](auto consumers) { return mhgp8::run_wspd_q2_census_batched(index, k, 8, WspdFrontMode::MidpointSamples,
                  consumers, {4, 8, 64}, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, 0, proposals).pipeline.front; },
              "batched q2 census with widened proposals differs from the brute-force oracle");
        }
      }
    }
  }
}

void rejections(Gate& gate) {
  const Points points{{0, 0, 0}, {10, 0, 0}, {5, 4, 0}, {40, 40, 40}};
  const auto index = mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
  const mhgp8::Q2CensusConsumer sink = [](const mhgp8::Q2Support&) {};
  const std::vector<mhgp8::Q2CensusConsumer> sinks{sink};
  const std::span<const mhgp8::Q2CensusConsumer> consumers(sinks);
  for (const auto& bad : {WspdFrontProposals{0, 16}, WspdFrontProposals{3, 16}, WspdFrontProposals{8, 16}, WspdFrontProposals{2, 0}}) {
    gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census(*index, 2, 8, WspdFrontMode::MidpointSamples,
        mhgp8::Q2CensusMode::SharedBlocks, sink, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs,
        mhgp8::Q2AnchorMode::Individual, 0, bad)); }, "serial q2 census accepted invalid front proposals");
    gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census_parallel(index, 2, 8, WspdFrontMode::MidpointSamples,
        mhgp8::Q2CensusMode::SharedBlocks, consumers, 4, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs,
        mhgp8::Q2AnchorMode::Individual, 0, {}, bad)); }, "parallel q2 census accepted invalid front proposals");
    gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census_cooperative(index, 2, 8, WspdFrontMode::MidpointSamples,
        consumers, {}, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, 0, bad)); },
        "cooperative q2 census accepted invalid front proposals");
    gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census_ranges(index, 2, 8, WspdFrontMode::MidpointSamples,
        consumers, {}, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, 0, bad)); },
        "anchor-range q2 census accepted invalid front proposals");
    gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census_batched(index, 2, 8, WspdFrontMode::MidpointSamples,
        consumers, {}, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, 0, bad)); },
        "batched q2 census accepted invalid front proposals");
  }
  gate.rejects([&] { static_cast<void>(mhgp8::run_wspd_q2_census(*index, 2, 8, WspdFrontMode::Pure,
      mhgp8::Q2CensusMode::SharedBlocks, sink, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs,
      mhgp8::Q2AnchorMode::Individual, 0, WspdFrontProposals{2, unlimited})); },
      "Pure q2 census accepted a widened proposal window");
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_wspd_q2_proposals_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    window_judge(gate);
    named_fixtures(gate);
    historical_constants(gate);
    replay_corpus(gate);
    entries(gate);
    rejections(gate);
    gate.require(gate.windows == 3 * 10 * (48 * 49 / 2) && gate.clamped_left > 0 && gate.clamped_right > 0 &&
                     gate.whole_permutation_windows > 0 && gate.clouds == 15 && gate.replay_runs == 15 * 15 * 7 &&
                     gate.named_fixture_checks == 8 && gate.historical_constant_checks == 6 &&
                     gate.extension_clamped_left > 0 && gate.extension_clamped_right > 0 &&
                     gate.truncated_double_windows > 0 && gate.rejections_with_historical_credit > 0 &&
                     gate.singleton_tangent_extension_sites > 0 && gate.singleton_kth_in_extension > 0 &&
                     gate.tangent_extension_sites > 0 && gate.factor_ranks_in_extension > 0 &&
                     gate.truncated_windows > 0 && gate.exhausted_extensions > 0 && gate.kth_witness_in_extension > 0 &&
                     gate.equal_distance_ties > 0 && gate.limit_skips > 0 && gate.extended_products > 0 &&
                     gate.mutants == 10 && gate.entry_runs == 15 * 3 * 2 * 11 && gate.entry_extended_products > 0 &&
                     gate.supports > 0 && gate.max_shell >= 20 && gate.invalid_inputs == 21,
                 "proposal gate lost a declared non-vacuity floor");
    std::cout << "{\"schema\":\"mhgp8_wspd_q2_proposals_gate_v1\",\"status\":\"passed\",\"public_status\":\"not_claimed\""
              << ",\"checks\":" << gate.checks << ",\"windows\":" << gate.windows
              << ",\"clamped_left\":" << gate.clamped_left << ",\"clamped_right\":" << gate.clamped_right
              << ",\"whole_permutation_windows\":" << gate.whole_permutation_windows << ",\"clouds\":" << gate.clouds
              << ",\"replay_runs\":" << gate.replay_runs << ",\"replay_products\":" << gate.replay_products
              << ",\"replay_rectangles\":" << gate.replay_rectangles << ",\"extended_products\":" << gate.extended_products
              << ",\"tangent_extension_sites\":" << gate.tangent_extension_sites
              << ",\"factor_ranks_in_extension\":" << gate.factor_ranks_in_extension
              << ",\"truncated_windows\":" << gate.truncated_windows << ",\"exhausted_extensions\":" << gate.exhausted_extensions
              << ",\"kth_witness_in_extension\":" << gate.kth_witness_in_extension
              << ",\"equal_distance_ties\":" << gate.equal_distance_ties << ",\"limit_skips\":" << gate.limit_skips
              << ",\"extension_clamped_left\":" << gate.extension_clamped_left
              << ",\"extension_clamped_right\":" << gate.extension_clamped_right
              << ",\"truncated_double_windows\":" << gate.truncated_double_windows
              << ",\"rejections_with_historical_credit\":" << gate.rejections_with_historical_credit
              << ",\"singleton_tangent_extension_sites\":" << gate.singleton_tangent_extension_sites
              << ",\"singleton_kth_in_extension\":" << gate.singleton_kth_in_extension
              << ",\"named_fixture_checks\":" << gate.named_fixture_checks
              << ",\"historical_constant_checks\":" << gate.historical_constant_checks
              << ",\"mutants\":" << gate.mutants << ",\"mutant_disagreements\":" << gate.mutant_disagreements
              << ",\"disagreements_by_mutant\":[";
    for (std::size_t m = 0; m < gate.disagreements_by_mutant.size(); ++m)
      std::cout << (m == 0 ? "" : ",") << gate.disagreements_by_mutant[m];
    std::cout << "]"
              << ",\"oracle_pairs\":" << gate.oracle_pairs << ",\"oracle_sites\":" << gate.oracle_sites
              << ",\"entry_runs\":" << gate.entry_runs << ",\"entry_extended_products\":" << gate.entry_extended_products
              << ",\"supports\":" << gate.supports << ",\"max_shell\":" << gate.max_shell
              << ",\"invalid_inputs\":" << gate.invalid_inputs << "}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_wspd_q2_proposals_gate failed: " << error.what() << '\n';
    return 1;
  }
}
