#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "pipeline/wspd_q2_batched.hpp"
#include "pipeline/wspd_q2_cooperative.hpp"
#include "pipeline/wspd_q2_ranges.hpp"

// Gate of the inherited witness identifiers of the WSPD front
// (WspdFrontProposals::inherit_witnesses). Independent judges, none of which
// calls the front's own predicates or shares its data structures:
//  1. a q2-only REPLAY of the whole front traversal with inheritance, written
//     from the contract (root descent, historical window, two extension
//     intervals, received ranks held in a growing vector, strict H>0 by brute
//     force over the 64 box-corner pairs), compared counter for counter and
//     rectangle for rectangle with the engine; it also computes what the engine
//     cannot: the rejections that the same product would not have obtained
//     alone, by replaying the reference filter on every rejected product;
//  2. a brute-force SAFETY oracle: every pair that the front does not emit has
//     at least Kmax strict interior sites. Four causal mutants are unsafe and
//     must each lose a support that the engine keeps;
//  3. ten causal mutants of the inheritance, each of which must disagree with
//     the engine on the engraved corpus; named minimal fixtures with their own
//     exact expectations; counters of an independently written Python model;
//     the engraved pre-tranche constants of the default front;
//  4. the brute-force q2 support oracle against the five q2 entries.
namespace {
using mhgp9::gen::Point3;
using mhgp9::gen::Q2CensusIndex;
using mhgp9::gen::Q2SiblingMode;
using mhgp9::gen::Q2SpatialNode;
using mhgp9::gen::Q2WitnessOrder;
using mhgp9::gen::WspdFrontMode;
using mhgp9::gen::WspdFrontProposals;
using mhgp9::gen::u64;
using i64 = std::int64_t;
using Points = std::vector<Point3>;
using Rectangles = std::vector<std::pair<std::size_t, std::size_t>>;
constexpr auto unlimited = std::numeric_limits<std::size_t>::max();

struct Gate {
  u64 checks{}, clouds{}, replay_runs{}, replay_products{}, replay_rectangles{};
  u64 nonempty_lists{}, full_lists{}, duplicates{}, extension_duplicates{}, inherited_rejections{};
  u64 true_inherited_rejections{}, overcounted_rejections{}, gained_rejected_mass{}, fewer_rejected_products{};
  u64 safety_runs{}, safety_pairs{}, rejected_pairs_judged{};
  u64 mutants{}, mutant_disagreements{}, unsafe_mutants{}, mutant_lost_supports{};
  std::array<u64, 10> disagreements_by_mutant{};
  std::array<u64, 10> lost_supports_by_mutant{};
  u64 named_fixture_checks{}, model_constant_checks{}, historical_constant_checks{};
  u64 oracle_pairs{}, oracle_sites{}, entry_runs{}, supports{}, max_shell{}, entry_inherited_credits{};
  u64 invalid_inputs{}, fit_checks{}, wide_rank_runs{}, max_duplicate_rank{};
  // 18-bit twins (coordinate_limit = 262143): clouds pushed to the far corner of the grid.
  u64 wide_clouds{}, wide_named_fixture_checks{}, wide_model_constant_checks{}, wide_historical_constant_checks{};
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

// ----------------------------------------------------------------- 1. replay
// Causal faults of the INHERITANCE only. Unsafe ones are listed first.
enum class Mutant {
  None,
  CountOnly,                  // Unsafe: the child starts from the count and credits a received rank again.
  DedupHistoricalOnly,        // Unsafe: received ranks are skipped in the historical window only.
  SiblingOutputLeak,          // Unsafe: the second child receives the list left by the last surviving search.
  SkippedSearchLeak,          // Unsafe: a product that skips its search passes that remanent list on.
  DuplicateNotProposed,       // A skipped received rank is not counted as proposed.
  DropNewCredits,             // Children receive what the parent received, not its new credits.
  CapacityTruncated,          // The passed list loses its last rank once it holds Kmax-1 ranks.
  ExtensionDuplicateUncounted,  // Duplicates of the extension are not reported apart.
  EmittedNewCreditsOnly,      // The emitted-credit ledger forgets the received ranks.
  StopOnNewCreditsOnly        // Received ranks are passed on but never counted towards Kmax.
};
constexpr std::array all_mutants{Mutant::CountOnly, Mutant::DedupHistoricalOnly, Mutant::SiblingOutputLeak,
                                 Mutant::SkippedSearchLeak, Mutant::DuplicateNotProposed, Mutant::DropNewCredits,
                                 Mutant::CapacityTruncated, Mutant::ExtensionDuplicateUncounted,
                                 Mutant::EmittedNewCreditsOnly, Mutant::StopOnNewCreditsOnly};
constexpr std::size_t unsafe_mutants = 4;  // The first four of all_mutants.

struct Replay {
  u64 product_visits{}, fully_rejected{}, emitted{}, separation_tests{}, searches{}, descent_steps{};
  u64 box_distance_tests{}, proposed{}, in_factors{}, h_tests{}, credits{};
  u64 extended_products{}, extended_proposals{}, extended_in_factors{}, extended_credits{}, extended_rejections{};
  u64 inherited_credits{}, inherited_duplicates{}, extended_inherited_duplicates{}, inherited_rejections{};
  u64 emitted_witness_credits{}, rejected_mass{}, residual_mass{};
  // Judge-only observations.
  u64 nonempty_lists{}, full_lists{}, true_inherited_rejections{};
  u64 max_duplicate_rank{};  // Largest received rank that a child window proposed again.
  Rectangles rectangles;
  std::set<std::pair<std::size_t, std::size_t>> visited;  // Filled on request only.
};

i64 diagonal2(const mhgp9::gen::Box3& box) {
  i64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 delta = i64{box.high[axis]} - box.low[axis];
    result += delta * delta;
  }
  return result;
}
i64 gap2(const mhgp9::gen::Box3& a, const mhgp9::gen::Box3& b) {
  i64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 delta = std::max<i64>({0, i64{a.low[axis]} - b.high[axis], i64{b.low[axis]} - a.high[axis]});
    result += delta * delta;
  }
  return result;
}
// Minimum of H=(z-a).(b-z) over both boxes by brute force over the 8x8
// corner pairs: H is affine in a for fixed b and in b for fixed a.
i64 corner_h_minimum(const mhgp9::gen::Box3& a, const mhgp9::gen::Box3& b, const Point3& z) {
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

Replay replay_front(const Q2CensusIndex& index, unsigned k, unsigned s, WspdFrontProposals proposals, Mutant mutant,
                    bool track_visited = false) {
  const auto nodes = index.spatial_nodes();
  const auto order = index.spatial_order();
  const auto points = index.cloud().points();
  const auto n = points.size();
  Replay out;
  using List = std::vector<std::size_t>;
  const auto inside = [](mhgp9::gen::Range range, std::size_t rank) { return range.first <= rank && rank < range.last; };
  const auto holds = [](const List& list, std::size_t rank) { return std::find(list.begin(), list.end(), rank) != list.end(); };
  struct Outcome { bool searched{}, rejected{}; List list; };
  // `count` is false for the uncounted reference filter that judges the counterfactual.
  const auto search = [&](const Q2SpatialNode& a, const Q2SpatialNode& b, const List& received, bool inherit,
                          bool count, Mutant fault) {
    Outcome outcome;
    if (n - a.range.size() - b.range.size() < k) return outcome;
    outcome.searched = true;
    Replay scratch;
    Replay& w = count ? out : scratch;
    ++w.searches;
    std::array<i64, 3> center4{};
    for (std::size_t axis = 0; axis < 3; ++axis)
      center4[axis] = i64{a.box.low[axis]} + a.box.high[axis] + b.box.low[axis] + b.box.high[axis];
    const auto distance = [&](const mhgp9::gen::Box3& box) {
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
      ++w.descent_steps;
      w.box_distance_tests += 2;
      const auto left = distance(nodes[nodes[node].left].box), right = distance(nodes[nodes[node].right].box);
      node = left <= right ? nodes[node].left : nodes[node].right;
    }
    const i64 pivot = static_cast<i64>(nodes[node].range.first);
    const auto centered = [&](i64 size) {
      const i64 first = std::min<i64>(std::max<i64>(pivot - size / 2, 0), static_cast<i64>(n) - size);
      return std::pair<i64, i64>{first, first + size};
    };
    const auto window = centered(std::min<i64>(k, static_cast<i64>(n)));
    const List given = inherit ? received : List{};
    w.inherited_credits += given.size();
    w.nonempty_lists += static_cast<u64>(!given.empty());
    List fresh;
    u64 duplicates = 0;
    // Distinct certified ranks known for THIS product; the faulty stop rules differ.
    const auto total = [&] {
      return fault == Mutant::StopOnNewCreditsOnly ? fresh.size() : given.size() + fresh.size();
    };
    const auto propose = [&](i64 signed_rank, bool extension) {
      const auto rank = static_cast<std::size_t>(signed_rank);
      const bool received_again = holds(given, rank) && fault != Mutant::CountOnly &&
                                  !(fault == Mutant::DedupHistoricalOnly && extension);
      if (received_again && fault == Mutant::DuplicateNotProposed) {
        ++duplicates;
        ++w.inherited_duplicates;
        w.extended_inherited_duplicates += static_cast<u64>(extension);
        return false;
      }
      ++w.proposed;
      if (inside(a.range, rank) || inside(b.range, rank)) {
        ++w.in_factors;
        w.extended_in_factors += static_cast<u64>(extension);
        return true;
      }
      if (received_again) {
        ++duplicates;
        ++w.inherited_duplicates;
        w.extended_inherited_duplicates += static_cast<u64>(extension && fault != Mutant::ExtensionDuplicateUncounted);
        w.max_duplicate_rank = std::max<u64>(w.max_duplicate_rank, rank);
        return true;
      }
      ++w.h_tests;
      if (corner_h_minimum(a.box, b.box, points[order[rank]]) > 0) {
        ++w.credits;
        w.extended_credits += static_cast<u64>(extension);
        fresh.push_back(rank);
      }
      return true;
    };
    for (i64 rank = window.first; rank < window.second && total() < k; ++rank) propose(rank, false);
    bool extension = false;
    if (total() < k && proposals.window_factor != 1 &&
        std::max(a.range.size(), b.range.size()) <= proposals.small_factor_limit) {
      const auto wider = centered(std::min<i64>(i64{k} * proposals.window_factor, static_cast<i64>(n)));
      extension = true;
      ++w.extended_products;
      for (const auto& interval : {std::pair<i64, i64>{wider.first, window.first},
                                   std::pair<i64, i64>{window.second, wider.second}})
        for (i64 rank = interval.first; rank < interval.second && total() < k; ++rank)
          if (propose(rank, true)) ++w.extended_proposals;
    }
    outcome.rejected = total() >= k;
    if (outcome.rejected) {
      w.extended_rejections += static_cast<u64>(extension);
      w.inherited_rejections += static_cast<u64>(inherit && fresh.size() + duplicates < k);
    }
    outcome.list = given;
    if (fault != Mutant::DropNewCredits) outcome.list.insert(outcome.list.end(), fresh.begin(), fresh.end());
    if (fault == Mutant::CapacityTruncated && outcome.list.size() + 1 == k && !outcome.list.empty())
      outcome.list.pop_back();
    if (!inherit) outcome.list.clear();
    return outcome;
  };
  struct Item { std::size_t a, b; List list; bool takes_remanent; };
  std::vector<Item> stack{{0, 0, {}, false}};
  List remanent;  // What a front holding its list in a member would still contain.
  while (!stack.empty()) {
    auto item = std::move(stack.back());
    stack.pop_back();
    if (item.takes_remanent) item.list = remanent;
    ++out.product_visits;
    if (track_visited) out.visited.emplace(item.a, item.b);
    const auto& a = nodes[item.a];
    const auto& b = nodes[item.b];
    if (item.a == item.b) {
      if (a.left == Q2SpatialNode::absent) continue;
      stack.push_back({a.right, a.right, {}, false});
      stack.push_back({a.left, a.right, {}, false});
      stack.push_back({a.left, a.left, {}, false});
      continue;
    }
    const u64 mass = static_cast<u64>(a.range.size()) * b.range.size();
    auto outcome = search(a, b, item.list, proposals.inherit_witnesses, true, mutant);
    if (outcome.searched && !outcome.rejected) {
      remanent = outcome.list;
      out.full_lists += static_cast<u64>(k > 1 && outcome.list.size() == k - 1);
    }
    if (!outcome.searched && mutant == Mutant::SkippedSearchLeak && proposals.inherit_witnesses)
      outcome.list = remanent;
    if (outcome.rejected) {
      ++out.fully_rejected;
      out.rejected_mass += mass;
      // Would the same product have been rejected by its window alone?
      if (mutant == Mutant::None && proposals.inherit_witnesses && !item.list.empty() &&
          !search(a, b, {}, false, false, Mutant::None).rejected)
        ++out.true_inherited_rejections;
      continue;
    }
    ++out.separation_tests;
    const auto da = diagonal2(a.box), db = diagonal2(b.box);
    if (static_cast<mhgp9::gen::i128>(gap2(a.box, b.box)) >= static_cast<mhgp9::gen::i128>(s) * s * std::max(da, db)) {
      ++out.emitted;
      out.residual_mass += mass;
      out.emitted_witness_credits += mutant == Mutant::EmittedNewCreditsOnly
                                         ? outcome.list.size() - std::min(outcome.list.size(), item.list.size())
                                         : outcome.list.size();
      out.rectangles.emplace_back(item.a, item.b);
      continue;
    }
    const bool split_a = a.left != Q2SpatialNode::absent && (b.left == Q2SpatialNode::absent || da >= db);
    const auto& split = split_a ? a : b;
    if (split.left == Q2SpatialNode::absent) throw std::logic_error("replay met two unseparated distinct leaves");
    // The right child is pushed first and visited after the whole left subtree.
    stack.push_back({split_a ? split.right : item.a, split_a ? item.b : split.right, outcome.list,
                     mutant == Mutant::SiblingOutputLeak && proposals.inherit_witnesses});
    stack.push_back({split_a ? split.left : item.a, split_a ? item.b : split.left, outcome.list, false});
  }
  std::sort(out.rectangles.begin(), out.rectangles.end());
  return out;
}

struct Observed {
  mhgp9::gen::WspdFrontResult result;
  Rectangles rectangles;
};
Observed observe(const Q2CensusIndex& index, unsigned k, unsigned s, WspdFrontProposals proposals) {
  Observed out;
  out.result = mhgp9::gen::run_wspd_front(index, k, s, WspdFrontMode::MidpointSamples,
      [&](const mhgp9::gen::WspdRectangle& rectangle) {
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
         w.extended_credits == replay.extended_credits && w.extended_rejections == replay.extended_rejections &&
         w.inherited_credits == replay.inherited_credits && w.inherited_duplicates == replay.inherited_duplicates &&
         w.extended_inherited_duplicates == replay.extended_inherited_duplicates &&
         w.inherited_rejections == replay.inherited_rejections &&
         w.emitted_witness_credits == replay.emitted_witness_credits &&
         w.rejected_pair_mass == std::array<u64, 3>{replay.rejected_mass, 0, 0} &&
         w.residual_pair_mass == std::array<u64, 3>{replay.residual_mass, 0, 0};
}

// ----------------------------------------------------------------- 2. safety
// Strict interior population of every diametral ball, by brute force on IDs.
struct Depths {
  std::size_t n{};
  std::vector<unsigned> depth;
  [[nodiscard]] unsigned at(std::size_t a, std::size_t b) const { return depth[a * n + b]; }
};
Depths depths(const Points& points) {
  Depths out{points.size(), std::vector<unsigned>(points.size() * points.size())};
  for (std::size_t a = 0; a < out.n; ++a) for (std::size_t b = a + 1; b < out.n; ++b) {
    unsigned count = 0;
    for (std::size_t z = 0; z < out.n; ++z) {
      i64 h = 0;
      for (std::size_t axis = 0; axis < 3; ++axis)
        h += (i64{points[z][axis]} - points[a][axis]) * (i64{points[b][axis]} - points[z][axis]);
      count += static_cast<unsigned>(h > 0);
    }
    out.depth[a * out.n + b] = out.depth[b * out.n + a] = count;
  }
  return out;
}
// Pairs that the residual cover does not contain although fewer than Kmax sites
// are strictly inside their ball: supports that a front would have lost.
u64 lost_supports(const Q2CensusIndex& index, const Rectangles& rectangles, const Depths& oracle, unsigned k,
                  u64* judged = nullptr) {
  const auto nodes = index.spatial_nodes();
  const auto order = index.spatial_order();
  std::vector<unsigned char> covered(oracle.n * oracle.n);
  for (const auto& [a, b] : rectangles)
    for (auto ra = nodes[a].range.first; ra < nodes[a].range.last; ++ra)
      for (auto rb = nodes[b].range.first; rb < nodes[b].range.last; ++rb)
        covered[order[ra] * oracle.n + order[rb]] = covered[order[rb] * oracle.n + order[ra]] = 1;
  u64 lost = 0;
  for (std::size_t a = 0; a < oracle.n; ++a) for (std::size_t b = a + 1; b < oracle.n; ++b) {
    if (covered[a * oracle.n + b] != 0) continue;
    if (judged != nullptr) ++*judged;
    lost += static_cast<u64>(oracle.at(a, b) < k);
  }
  return lost;
}

// --------------------------------------------------------------- 3. fixtures
// K2_union_rejects, judged pair (id 3, id 4): two strict witnesses of the pair that no single
// window of two ranks holds together on its product path. Only the union of one received
// rank and one new, distinct rank rejects it: depth 2 >= K, so the rejection is safe.
Points k2_union_rejects();
// K2_reproposed_witness, judged pair (id 3, id 4): ONE strict witness, credited by the parent
// and proposed again by the child. The pair has depth 1 < K = 2 and must stay residual; a
// front that inherits a bare count rejects it and loses a support.
Points k2_reproposed_witness();
// Found by the design refutation of 17 September 2026 (K = 2, window factor 1): the rejection
// of product {rank 0} x {rank 4} uses the received rank 2 although its own window {1, 2}
// would have sufficed. The engine counter is an upper bound of the true counterfactual.
Points k2_counterfactual_overcount() {
  return {{5, 2, 0}, {16, 1, 0}, {31, 0, 0}, {36, 5, 0}, {52, 7, 0}};
}
Points bridged_cubes() {
  using mhgp9::gen::Coordinate;
  Points cloud;
  for (unsigned corner = 0; corner < 8; ++corner) {
    const auto x = static_cast<Coordinate>((corner & 1U) * 6000U);
    const auto y = static_cast<Coordinate>(((corner >> 1U) & 1U) * 6000U);
    const auto z = static_cast<Coordinate>(((corner >> 2U) & 1U) * 6000U);
    cloud.push_back({x, y, z});
    cloud.push_back({static_cast<Coordinate>(x + 54000), y, z});
  }
  cloud.push_back({30100, 3000, 3000});
  cloud.push_back({30110, 3010, 2990});
  cloud.push_back({30120, 2990, 3010});
  for (unsigned i = 0; i < 8; ++i)
    cloud.push_back({static_cast<Coordinate>(7000U * i + 500U), 65000, static_cast<Coordinate>(100U * i)});
  return cloud;
}
Points grid4() {
  using mhgp9::gen::Coordinate;
  Points grid;
  for (unsigned x = 0; x < 4; ++x) for (unsigned y = 0; y < 4; ++y) for (unsigned z = 0; z < 4; ++z)
    grid.push_back({static_cast<Coordinate>(100 * x), static_cast<Coordinate>(100 * y), static_cast<Coordinate>(100 * z)});
  return grid;
}
Points sphere22() {
  using mhgp9::gen::Coordinate;
  Points sphere{{100, 100, 100}, {150, 100, 100}};
  for (const auto& d : std::vector<std::array<int, 3>>{{0, 25, 0}, {0, -25, 0}, {0, 0, 25}, {0, 0, -25}, {-20, 15, 0},
       {-20, -15, 0}, {20, 15, 0}, {20, -15, 0}, {-15, 20, 0}, {15, -20, 0}, {-7, 24, 0}, {7, -24, 0},
       {0, 15, 20}, {0, -15, -20}, {-15, 0, 20}, {15, 0, -20}, {-7, 0, 24}, {7, 0, -24}, {0, 7, 24}, {0, -24, 7}})
    sphere.push_back({static_cast<Coordinate>(125 + d[0]), static_cast<Coordinate>(100 + d[1]),
                      static_cast<Coordinate>(100 + d[2])});
  return sphere;
}
// 18-bit twins (coordinate_limit = 262143). Every integer predicate of the index
// (longest-axis split at the integer midpoint, partition by <=) and of the front
// (H, box gaps, descent distances, separation) is invariant under an integer
// translation: the cloud pushed to the far corner of the grid (its maximum on
// each axis becomes 262143) has the same tree, counters, rectangles and
// supports, so every engraved expectation of the original holds for the twin
// by invariance, while the replay and the brute-force oracle judge it anew.
// Any difference is an engine fault of the 18-bit port.
Points far_corner(Points points) {
  static_assert(mhgp9::gen::coordinate_limit == 262143);
  std::array<mhgp9::gen::Coordinate, 3> maximum{};
  for (const auto& point : points)
    for (std::size_t axis = 0; axis < 3; ++axis) maximum[axis] = std::max(maximum[axis], point[axis]);
  for (auto& point : points) {
    point.x = static_cast<mhgp9::gen::Coordinate>(point.x + (262143 - maximum[0]));
    point.y = static_cast<mhgp9::gen::Coordinate>(point.y + (262143 - maximum[1]));
    point.z = static_cast<mhgp9::gen::Coordinate>(point.z + (262143 - maximum[2]));
  }
  return points;
}
bool wide(const Points& points) {
  return std::any_of(points.begin(), points.end(), [](const Point3& point) {
    return point.x > 65535 || point.y > 65535 || point.z > 65535; });
}

// Engraved clouds. Coordinates are exact u16 values; none is random at run time.
std::vector<Points> fixtures() {
  std::vector<Points> clouds;
  clouds.push_back({{7, 8, 9}});
  clouds.push_back({{0, 0, 0}, {10, 0, 0}, {5, 4, 0}});
  clouds.push_back(sphere22());                              // Tangent sites: H = 0 must never enter a list.
  Points line;                                               // Collinear: long lists, factor ranks adjacent.
  for (unsigned i = 0; i < 40; ++i) line.push_back({static_cast<std::uint16_t>(3 * i + (i % 3)), 50, 50});
  clouds.push_back(line);
  Points rows;                                               // Two parallel rows: almost no universal witness.
  for (unsigned i = 0; i < 24; ++i) { rows.push_back({static_cast<std::uint16_t>(10 * i), 0, 0});
                                      rows.push_back({static_cast<std::uint16_t>(10 * i + 3), 4000, 0}); }
  clouds.push_back(rows);
  clouds.push_back(grid4());                                 // Equal distances and ties everywhere.
  for (const auto& [size, seed] : std::vector<std::pair<unsigned, std::uint32_t>>{{24, 5}, {48, 11}, {96, 23}}) {
    Points cloud;
    std::uint32_t state = seed;
    const auto next = [&] { state = state * 1664525U + 1013904223U; return static_cast<std::uint16_t>((state >> 16U) % 2000U); };
    for (unsigned i = 0; i < size; ++i) cloud.push_back({static_cast<std::uint16_t>(next() + 7 * i), next(), next()});
    clouds.push_back(cloud);
  }
  Points clusters;                                           // Two dense clusters and a sparse bridge.
  {
    std::uint32_t state = 77;
    const auto next = [&](unsigned span) { state = state * 1664525U + 1013904223U; return static_cast<std::uint16_t>((state >> 16U) % span); };
    for (unsigned i = 0; i < 30; ++i) clusters.push_back({static_cast<std::uint16_t>(1000 + next(60) + i), static_cast<std::uint16_t>(1000 + next(60)), static_cast<std::uint16_t>(1000 + next(60))});
    for (unsigned i = 0; i < 30; ++i) clusters.push_back({static_cast<std::uint16_t>(9000 + next(60) + i), static_cast<std::uint16_t>(1000 + next(60)), static_cast<std::uint16_t>(1000 + next(60))});
    for (unsigned i = 0; i < 12; ++i) clusters.push_back({static_cast<std::uint16_t>(2000 + 550 * i), static_cast<std::uint16_t>(1030 + (i % 2)), 1030});
  }
  clouds.push_back(clusters);
  Points fourteen;                                           // K < n < 2K for K = 10: truncated windows.
  for (unsigned i = 0; i < 14; ++i)
    fourteen.push_back({static_cast<std::uint16_t>(40 * i + (i * i) % 7), static_cast<std::uint16_t>(30 + (3 * i) % 5),
                        static_cast<std::uint16_t>(20 + (5 * i) % 3)});
  clouds.push_back(fourteen);
  clouds.push_back(bridged_cubes());                         // High products surviving with three credits.
  clouds.push_back(k2_counterfactual_overcount());
  clouds.push_back(k2_union_rejects());
  clouds.push_back(k2_reproposed_witness());
  return clouds;
}
constexpr std::size_t fixture_count = 15;
// Far-corner twins of the pinned clouds: the far sites of the bridged cubes (y = 65000,
// the u16 frontier) reach y = 262143; the named fixtures keep their exact expectations.
std::vector<Points> wide_fixtures() {
  return {far_corner(grid4()), far_corner(sphere22()), far_corner(bridged_cubes()),
          far_corner(k2_counterfactual_overcount()), far_corner(k2_union_rejects()), far_corner(k2_reproposed_witness())};
}
constexpr std::size_t wide_fixture_count = 6;

const std::vector<WspdFrontProposals>& variants() {
  static const std::vector<WspdFrontProposals> values{{1, unlimited, true}, {2, 16, true}, {2, unlimited, true},
                                                      {4, 2, true}, {4, unlimited, true}};
  return values;
}

bool pair_is_residual(const Q2CensusIndex& index, const Rectangles& rectangles, std::size_t id_a, std::size_t id_b) {
  const auto nodes = index.spatial_nodes();
  const auto order = index.spatial_order();
  const auto holds = [&](mhgp9::gen::Range range, std::size_t id) {
    for (auto rank = range.first; rank < range.last; ++rank) if (order[rank] == id) return true;
    return false;
  };
  for (const auto& [a, b] : rectangles)
    if ((holds(nodes[a].range, id_a) && holds(nodes[b].range, id_b)) ||
        (holds(nodes[a].range, id_b) && holds(nodes[b].range, id_a))) return true;
  return false;
}

Points k2_union_rejects() { return {{30, 41, 3}, {23, 51, 2}, {46, 59, 1}, {15, 57, 3}, {35, 34, 1}}; }
Points k2_reproposed_witness() { return {{57, 60, 4}, {9, 55, 2}, {50, 32, 2}, {16, 51, 1}, {63, 27, 1}}; }

// Exact expectations of the named fixtures, K = 2, s = 8, judged pair (id 3, id 4).
void named_fixtures(Gate& gate) {
  struct Expected {
    WspdFrontProposals proposals;
    bool residual;
    u64 rejected_mass, inherited_credits, duplicates, inherited_rejections, true_rejections, extended_rejections;
  };
  // Each fixture is checked twice: as engraved, then its far-corner 18-bit twin
  // against the SAME expectations (translation invariance), on a separate counter.
  const auto check = [&](const Points& engraved, unsigned depth, const std::vector<Expected>& expectations,
                         const char* message) {
    for (const bool twin : {false, true}) {
      const auto points = twin ? far_corner(engraved) : engraved;
      const auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
      const auto oracle = depths(points);
      gate.require(oracle.at(3, 4) == depth, "named fixture lost the engraved depth of its judged pair");
      for (const auto& e : expectations) {
        const auto engine = observe(*index, 2, 8, e.proposals);
        const auto replay = replay_front(*index, 2, 8, e.proposals, Mutant::None);
        const auto& w = engine.result.work;
        gate.require(agrees(engine, replay) && pair_is_residual(*index, engine.rectangles, 3, 4) == e.residual &&
                         w.rejected_pair_mass[0] == e.rejected_mass && w.inherited_credits == e.inherited_credits &&
                         w.inherited_duplicates == e.duplicates && w.inherited_rejections == e.inherited_rejections &&
                         replay.true_inherited_rejections == e.true_rejections &&
                         w.extended_rejections == e.extended_rejections &&
                         lost_supports(*index, engine.rectangles, oracle, 2) == 0,
                     message);
        ++(twin ? gate.wide_named_fixture_checks : gate.named_fixture_checks);
      }
    }
  };
  check(k2_union_rejects(), 2,
        {{{1, unlimited, false}, true, 0, 0, 0, 0, 0, 0}, {{1, unlimited, true}, false, 1, 2, 1, 1, 1, 0},
         {{2, unlimited, false}, false, 1, 0, 0, 0, 0, 1}, {{2, unlimited, true}, false, 1, 2, 1, 1, 0, 0}},
        "K2_union_rejects: only one received rank plus one new distinct rank must reject the pair");
  check(k2_reproposed_witness(), 1,
        {{{1, unlimited, false}, true, 1, 0, 0, 0, 0, 0}, {{1, unlimited, true}, true, 1, 2, 1, 1, 0, 0},
         {{2, unlimited, false}, true, 1, 0, 0, 0, 0, 0}, {{2, unlimited, true}, true, 1, 2, 1, 1, 0, 0}},
        "K2_reproposed_witness: the depth-1 pair must stay residual, its witness counted once");
  check(k2_counterfactual_overcount(), 0,
        {{{1, unlimited, false}, true, 2, 0, 0, 0, 0, 0}, {{1, unlimited, true}, true, 2, 4, 3, 1, 0, 0},
         {{2, unlimited, true}, true, 3, 2, 2, 0, 0, 2}},
        "K2_counterfactual_overcount: the engine counter must stay an upper bound of the true count");
  // The bare-count mutant loses the depth-1 support that the engine keeps.
  for (const bool twin : {false, true}) {
    const auto points = twin ? far_corner(k2_reproposed_witness()) : k2_reproposed_witness();
    const auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
    for (const auto& proposals : {WspdFrontProposals{1, unlimited, true}, WspdFrontProposals{2, unlimited, true}}) {
      const auto mutant = replay_front(*index, 2, 8, proposals, Mutant::CountOnly);
      gate.require(!pair_is_residual(*index, mutant.rectangles, 3, 4) &&
                       lost_supports(*index, mutant.rectangles, depths(points), 2) >= 1,
                   "K2_reproposed_witness: inheriting a bare count must lose the depth-1 support");
      ++(twin ? gate.wide_named_fixture_checks : gate.named_fixture_checks);
    }
  }
}

// Counters of an independently written Python model of the front (design refutation of
// 17 September 2026; receipts/q2_front_inheritance_20260917/cadrage/independent_front_model.py
// and its constants file). Third implementation: neither the engine nor the replay above.
void model_constants(Gate& gate) {
  struct Model {
    unsigned k, factor;
    u64 visits, searches, steps, proposed, in_factors, h_tests, credits, rejected, emitted, rejected_mass,
        residual_mass, extended_products, extended_proposals, extended_credits, extended_rejections,
        inherited_credits, duplicates, extended_duplicates, inherited_rejections, true_rejections, emitted_credits;
  };
  // Each cloud is checked as engraved, then as its far-corner 18-bit twin against the
  // SAME model constants (translation invariance), on a separate counter.
  const auto check = [&](const Points& engraved, const Model& m) {
    for (const bool twin : {false, true}) {
      const auto points = twin ? far_corner(engraved) : engraved;
      const auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
      const WspdFrontProposals proposals{m.factor, unlimited, true};
      const auto w = observe(*index, m.k, 8, proposals).result.work;
      const auto replay = replay_front(*index, m.k, 8, proposals, Mutant::None);
      gate.require(w.product_visits == m.visits && w.witness_searches == m.searches &&
                       w.witness_descent_steps == m.steps && w.proposed_sites == m.proposed &&
                       w.proposals_in_factors == m.in_factors && w.h_bound_tests == m.h_tests &&
                       w.witness_lane_credits == m.credits && w.fully_rejected_products == m.rejected &&
                       w.emitted_rectangles == m.emitted && w.rejected_pair_mass[0] == m.rejected_mass &&
                       w.residual_pair_mass[0] == m.residual_mass && w.extended_products == m.extended_products &&
                       w.extended_proposals == m.extended_proposals && w.extended_credits == m.extended_credits &&
                       w.extended_rejections == m.extended_rejections && w.inherited_credits == m.inherited_credits &&
                       w.inherited_duplicates == m.duplicates &&
                       w.extended_inherited_duplicates == m.extended_duplicates &&
                       w.inherited_rejections == m.inherited_rejections &&
                       replay.true_inherited_rejections == m.true_rejections &&
                       w.emitted_witness_credits == m.emitted_credits,
                   twin ? "18-bit far-corner twin differs from the independent Python model (translation invariance broken)"
                        : "front with inherited witnesses differs from the independent Python model");
      ++(twin ? gate.wide_model_constant_checks : gate.model_constant_checks);
    }
  };
  const auto grid = grid4();
  check(grid, {2, 1, 3204, 3076, 18456, 6010, 1191, 3949, 912, 436, 1134, 882, 1134, 0, 0, 0, 0, 1252, 870, 0, 382, 240, 666});
  check(grid, {2, 2, 2880, 2752, 16512, 10242, 1955, 7675, 967, 491, 917, 1099, 917, 2436, 4834, 175, 175, 928, 612, 0, 316, 86, 449});
  check(grid, {5, 1, 3880, 3752, 22512, 18407, 2735, 12712, 1934, 168, 1740, 276, 1740, 0, 0, 0, 0, 3902, 2960, 0, 168, 136, 3045});
  check(grid, {5, 2, 3642, 3514, 21084, 33528, 4327, 26390, 2332, 339, 1450, 566, 1450, 3314, 16351, 580, 139, 3724, 2811, 483, 286, 76, 2499});
  check(grid, {10, 1, 4054, 3926, 23556, 38960, 4629, 29410, 3124, 71, 1924, 92, 1924, 0, 0, 0, 0, 6314, 4921, 0, 71, 71, 5571});
  check(grid, {10, 2, 3974, 3846, 23076, 75162, 6758, 62179, 3638, 188, 1767, 249, 1767, 3757, 37127, 898, 99, 7290, 6225, 1534, 167, 45, 5403});
  check(k2_reproposed_witness(), {2, 1, 25, 14, 36, 27, 15, 11, 4, 1, 9, 1, 9, 0, 0, 0, 0, 2, 1, 0, 1, 0, 3});
  check(k2_reproposed_witness(), {2, 2, 25, 14, 36, 53, 25, 27, 5, 1, 9, 1, 9, 13, 26, 1, 0, 2, 1, 0, 1, 0, 4});
  check(k2_union_rejects(), {2, 1, 25, 13, 33, 26, 14, 11, 4, 1, 9, 1, 9, 0, 0, 0, 0, 2, 1, 0, 1, 1, 3});
  check(k2_union_rejects(), {2, 2, 25, 13, 33, 50, 24, 25, 4, 1, 9, 1, 9, 12, 24, 0, 0, 2, 1, 0, 1, 0, 3});
  check(k2_counterfactual_overcount(), {2, 1, 25, 14, 38, 27, 16, 8, 6, 2, 8, 2, 8, 0, 0, 0, 0, 4, 3, 0, 1, 0, 4});
  check(k2_counterfactual_overcount(), {2, 2, 23, 12, 32, 47, 26, 19, 6, 2, 7, 3, 7, 12, 23, 2, 2, 2, 2, 0, 0, 0, 3});
}

// Counters of the front BEFORE the widened window (commit 8d615cfd, pinned tranche-19 library),
// default proposals, q2 lane alone, s = 8: the default front must not drift.
void historical_constants(Gate& gate) {
  struct Constants { u64 visits, searches, descents, proposed, in_factors, h_tests, credits, rejected, emitted, separations,
                         rejected_mass, residual_mass; };
  // Each cloud is checked as engraved, then as its far-corner 18-bit twin against the
  // SAME pre-tranche constants (translation invariance), on a separate counter.
  const auto check = [&](const Points& engraved, unsigned k, const Constants& c) {
    for (const bool twin : {false, true}) {
      const auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(twin ? far_corner(engraved) : engraved));
      for (const auto& proposals : {WspdFrontProposals{}, WspdFrontProposals{1, unlimited, false}}) {
        const auto w = observe(*index, k, 8, proposals).result.work;
        gate.require(w.product_visits == c.visits && w.witness_searches == c.searches &&
                         w.witness_descent_steps == c.descents && w.proposed_sites == c.proposed &&
                         w.proposals_in_factors == c.in_factors && w.h_bound_tests == c.h_tests &&
                         w.witness_lane_credits == c.credits && w.fully_rejected_products == c.rejected &&
                         w.emitted_rectangles == c.emitted && w.separation_tests == c.separations &&
                         w.rejected_pair_mass[0] == c.rejected_mass && w.residual_pair_mass[0] == c.residual_mass &&
                         w.inherited_credits == 0 && w.inherited_duplicates == 0 &&
                         w.extended_inherited_duplicates == 0 && w.inherited_rejections == 0 &&
                         w.emitted_witness_credits == 0,
                     twin ? "18-bit far-corner twin drifted from the engraved pre-tranche counters (translation invariance broken)"
                          : "default front drifted from its engraved pre-tranche counters");
        ++(twin ? gate.wide_historical_constant_checks : gate.historical_constant_checks);
      }
    }
  };
  check(grid4(), 5, {4064, 3936, 23616, 19680, 2794, 16886, 5824, 48, 1952, 3889, 64, 1952});
  check(grid4(), 10, {4096, 3968, 23808, 39680, 4653, 35027, 8572, 0, 2016, 3969, 0, 2016});
  check(sphere22(), 2, {444, 400, 1995, 800, 272, 528, 229, 40, 171, 361, 60, 171});
}

// ----------------------------------------------------------------- 4. corpus
void replay_corpus(Gate& gate) {
  static_assert(all_mutants.size() == 10);
  auto clouds = fixtures();
  gate.require(clouds.size() == fixture_count, "engraved corpus changed its size");
  const auto twins = wide_fixtures();
  gate.require(twins.size() == wide_fixture_count &&
                   std::all_of(twins.begin(), twins.end(), [](const Points& points) { return wide(points); }),
               "18-bit twin corpus changed its size or lost its 18-bit coordinates");
  clouds.insert(clouds.end(), twins.begin(), twins.end());
  for (const auto& points : clouds) {
    const auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
    const auto oracle = depths(points);
    ++gate.clouds;
    gate.wide_clouds += static_cast<u64>(wide(points));
    for (const unsigned k : {1U, 2U, 3U, 5U, 10U}) for (const unsigned s : {8U, 10U, 12U}) {
      u64 previous_mass = std::numeric_limits<u64>::max();
      Rectangles previous_rectangles;
      for (const auto& proposals : variants()) {
        const auto plain = observe(*index, k, s, {proposals.window_factor, proposals.small_factor_limit, false});
        const auto engine = observe(*index, k, s, proposals);
        const auto replay = replay_front(*index, k, s, proposals, Mutant::None);
        const auto& w = engine.result.work;
        const auto& p = plain.result.work;
        gate.require(agrees(engine, replay),
                     "front with inherited witnesses differs from the independent q2 replay");
        // Ledgers that a reader of receipts can check without any judge.
        gate.require(w.proposed_sites == w.proposals_in_factors + w.inherited_duplicates + w.h_bound_tests &&
                         w.extended_proposals >= w.extended_proposals_in_factors + w.extended_inherited_duplicates +
                                                     w.extended_credits &&
                         w.inherited_credits % 2 == 0 && w.inherited_duplicates <= w.inherited_credits &&
                         w.witness_lane_credits + w.inherited_credits / 2 ==
                             u64{k} * w.fully_rejected_products + w.emitted_witness_credits,
                     "inheritance ledger identities failed on the engine counters");
        // Domination: the search with a received list never stops later than the window
        // alone, so visited products, rectangles and residual mass can only shrink. The
        // NUMBER of rejected products is not monotone: a rejected ancestor hides its
        // rejected descendants.
        gate.require(std::includes(plain.rectangles.begin(), plain.rectangles.end(),
                                   engine.rectangles.begin(), engine.rectangles.end()) &&
                         w.product_visits <= p.product_visits && w.witness_searches <= p.witness_searches &&
                         w.rejected_pair_mass[0] >= p.rejected_pair_mass[0] &&
                         w.residual_pair_mass[0] <= p.residual_pair_mass[0],
                     "inherited witnesses kept a product or a rectangle that the same window alone rejects");
        gate.require(replay.true_inherited_rejections <= w.inherited_rejections &&
                         w.inherited_rejections <= w.fully_rejected_products &&
                         w.fully_rejected_products - replay.true_inherited_rejections <= p.fully_rejected_products,
                     "inherited rejection counter is not an upper bound of the true counterfactual");
        if (k == 1)
          gate.require(w == p && engine.rectangles == plain.rectangles,
                       "Kmax 1 leaves no room for a received rank: the front must equal the plain one");
        // Safety, by brute force: no pair outside the residual cover is a support.
        gate.require(lost_supports(*index, engine.rectangles, oracle, k, &gate.rejected_pairs_judged) == 0,
                     "front with inherited witnesses rejected a pair of depth below Kmax");
        gate.safety_pairs += oracle.n * (oracle.n - 1) / 2;
        ++gate.safety_runs;
        // Unlimited windows of growing factor: received lists and windows both grow.
        if (proposals.small_factor_limit == unlimited) {
          gate.require(w.residual_pair_mass[0] <= previous_mass &&
                           (previous_mass == std::numeric_limits<u64>::max() ||
                            std::includes(previous_rectangles.begin(), previous_rectangles.end(),
                                          engine.rectangles.begin(), engine.rectangles.end())),
                       "a wider window with inherited witnesses kept more than a narrower one");
          previous_mass = w.residual_pair_mass[0];
          previous_rectangles = engine.rectangles;
        }
        if (s == 8) {
          const auto reference = replay_front(*index, k, s, {proposals.window_factor, proposals.small_factor_limit, false},
                                              Mutant::None, true);
          const auto visited = replay_front(*index, k, s, proposals, Mutant::None, true).visited;
          gate.require(std::includes(reference.visited.begin(), reference.visited.end(), visited.begin(), visited.end()),
                       "inherited witnesses visited a product that the same window alone never reaches");
          if (k != 1 && k != 3)
            for (std::size_t m = 0; m < all_mutants.size(); ++m) {
              const auto mutant = replay_front(*index, k, s, proposals, all_mutants[m]);
              gate.disagreements_by_mutant[m] += static_cast<u64>(!agrees(engine, mutant));
              if (m < unsafe_mutants)
                gate.lost_supports_by_mutant[m] += lost_supports(*index, mutant.rectangles, oracle, k);
            }
        }
        gate.nonempty_lists += replay.nonempty_lists;
        gate.full_lists += replay.full_lists;
        gate.duplicates += w.inherited_duplicates;
        gate.extension_duplicates += w.extended_inherited_duplicates;
        gate.inherited_rejections += w.inherited_rejections;
        gate.true_inherited_rejections += replay.true_inherited_rejections;
        gate.overcounted_rejections += w.inherited_rejections - replay.true_inherited_rejections;
        gate.gained_rejected_mass += w.rejected_pair_mass[0] - p.rejected_pair_mass[0];
        gate.fewer_rejected_products += static_cast<u64>(w.fully_rejected_products < p.fully_rejected_products);
        gate.replay_products += replay.product_visits;
        gate.replay_rectangles += replay.emitted;
        ++gate.replay_runs;
      }
    }
  }
  for (std::size_t m = 0; m < all_mutants.size(); ++m) {
    gate.require(gate.disagreements_by_mutant[m] > 0,
                 "a causal inheritance mutant agreed with the front on the whole engraved corpus");
    gate.mutant_disagreements += gate.disagreements_by_mutant[m];
    ++gate.mutants;
    if (m < unsafe_mutants) {
      gate.require(gate.lost_supports_by_mutant[m] > 0,
                   "an unsafe inheritance mutant lost no support on the whole engraved corpus");
      gate.mutant_lost_supports += gate.lost_supports_by_mutant[m];
      ++gate.unsafe_mutants;
    }
  }
}

// Ranks beyond one byte. Every other cloud of the front gates has at most 128 sites: a front
// that stored its received ranks on 8 bits would pass them all and lose supports from n = 300.
// 320 engraved sites: the replay compares every counter and the brute-force oracle judges
// every rejected pair, with a floor on the largest received rank that is proposed again.
// The 16-bit frontier needs more than 65 536 sites: it is judged by the `frontier` campaign
// of the receipts runner (support digest of an inheriting run against its plain twin).
void wide_ranks(Gate& gate) {
  Points cloud;
  std::uint32_t state = 2026;
  const auto next = [&] { state = state * 1664525U + 1013904223U; return static_cast<std::uint16_t>((state >> 16U) % 4000U); };
  for (unsigned i = 0; i < 320; ++i) cloud.push_back({static_cast<std::uint16_t>(next() + 13 * i), next(), next()});
  const auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(cloud));
  const auto oracle = depths(cloud);
  for (const unsigned k : {5U, 10U})
    for (const auto& proposals : {WspdFrontProposals{1, unlimited, true}, WspdFrontProposals{2, 16, true}}) {
      const auto engine = observe(*index, k, 8, proposals);
      const auto replay = replay_front(*index, k, 8, proposals, Mutant::None);
      gate.require(agrees(engine, replay), "front with inherited witnesses differs from the replay beyond 256 sites");
      gate.require(lost_supports(*index, engine.rectangles, oracle, k, &gate.rejected_pairs_judged) == 0,
                   "front with inherited witnesses rejected a pair of depth below Kmax beyond 256 sites");
      gate.max_duplicate_rank = std::max(gate.max_duplicate_rank, replay.max_duplicate_rank);
      ++gate.wide_rank_runs;
    }
}

// ---------------------------------------------------------------- 5. entries
struct Support {
  std::size_t a{}, b{};
  mhgp9::gen::Q2BallKey key;
  std::vector<std::size_t> interior, shell;
  bool operator==(const Support&) const = default;
};
using Output = std::vector<Support>;
Support copy(const mhgp9::gen::Q2Support& value) {
  Support result{std::min(value.a_id, value.b_id), std::max(value.a_id, value.b_id), value.key,
                 {value.interior.begin(), value.interior.end()}, {value.shell.begin(), value.shell.end()}};
  std::sort(result.interior.begin(), result.interior.end());
  std::sort(result.shell.begin(), result.shell.end());
  return result;
}
void normalize(Output& output) {
  std::sort(output.begin(), output.end(), [](const auto& x, const auto& y) { return x.a < y.a || (x.a == y.a && x.b < y.b); });
}
Output support_oracle(Gate& gate, const Points& points, unsigned k) {
  gate.require(points.size() <= 100, "inheritance oracle exceeded its bounded n<=100 domain");
  Output result;
  for (std::size_t a = 0; a < points.size(); ++a) for (std::size_t b = a + 1; b < points.size(); ++b) {
    Support item;
    item.a = a;
    item.b = b;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      item.key.center_twice[axis] = static_cast<std::uint32_t>(points[a][axis]) + points[b][axis];
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
  auto clouds = fixtures();
  const auto twins = wide_fixtures();
  clouds.insert(clouds.end(), twins.begin(), twins.end());
  for (const auto& points : clouds) {
    const auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
    for (const unsigned k : {2U, 5U, 10U}) {
      const auto expected = support_oracle(gate, points, k);
      for (const auto& proposals : {WspdFrontProposals{1, unlimited, true}, WspdFrontProposals{2, 16, true}}) {
        const auto mono_front = observe(*index, k, 8, proposals);
        const auto check = [&](Output output, const mhgp9::gen::WspdFrontResult& front, const char* message) {
          normalize(output);
          gate.require(output == expected, message);
          gate.require(front.work == mono_front.result.work && front.active_lane_mask == 1,
                       "a q2 entry lost the inherited witnesses between its front and the mono front");
          for (const auto& item : output) gate.max_shell = std::max<u64>(gate.max_shell, item.shell.size());
          gate.supports += output.size();
          gate.entry_inherited_credits += front.work.inherited_credits;
          ++gate.entry_runs;
        };
        {
          Output output;
          const auto result = mhgp9::gen::run_wspd_q2_census(*index, k, 8, WspdFrontMode::MidpointSamples,
              mhgp9::gen::Q2CensusMode::SharedBlocks, [&](const mhgp9::gen::Q2Support& s) { output.push_back(copy(s)); },
              Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, mhgp9::gen::Q2AnchorMode::Individual, 0, proposals);
          check(std::move(output), result.front, "serial q2 census with inherited witnesses differs from the brute-force oracle");
        }
        for (const std::size_t workers : {std::size_t{1}, std::size_t{3}}) {
          const auto run = [&](auto&& call, const char* message) {
            std::vector<Output> slots(workers);
            std::vector<mhgp9::gen::Q2CensusConsumer> consumers;
            for (std::size_t w = 0; w < workers; ++w)
              consumers.emplace_back([&slots, w](const mhgp9::gen::Q2Support& s) { slots[w].push_back(copy(s)); });
            const auto front = call(std::span<const mhgp9::gen::Q2CensusConsumer>(consumers));
            Output joined;
            for (auto& slot : slots) joined.insert(joined.end(), slot.begin(), slot.end());
            check(std::move(joined), front, message);
          };
          run([&](auto consumers) { return mhgp9::gen::run_wspd_q2_census_parallel(index, k, 8, WspdFrontMode::MidpointSamples,
                  mhgp9::gen::Q2CensusMode::SharedBlocks, consumers, 4, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst,
                  mhgp9::gen::Q2AnchorMode::Individual, 0, {}, proposals).front; },
              "parallel Coarse q2 census with inherited witnesses differs from the brute-force oracle");
          run([&](auto consumers) { return mhgp9::gen::run_wspd_q2_census_parallel(index, k, 8, WspdFrontMode::MidpointSamples,
                  mhgp9::gen::Q2CensusMode::SharedBlocks, consumers, 4, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst,
                  mhgp9::gen::Q2AnchorMode::Individual, 0, {mhgp9::gen::WspdQ2ScheduleMode::Donate, 4, 1}, proposals).front; },
              "parallel Donate q2 census with inherited witnesses differs from the brute-force oracle");
          run([&](auto consumers) { return mhgp9::gen::run_wspd_q2_census_cooperative(index, k, 8, WspdFrontMode::MidpointSamples,
                  consumers, {4, 4, 8, 1}, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, 0, proposals).pipeline.front; },
              "cooperative q2 census with inherited witnesses differs from the brute-force oracle");
          run([&](auto consumers) { return mhgp9::gen::run_wspd_q2_census_ranges(index, k, 8, WspdFrontMode::MidpointSamples,
                  consumers, {4, 4, 2}, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, 0, proposals).pipeline.front; },
              "anchor-range q2 census with inherited witnesses differs from the brute-force oracle");
          run([&](auto consumers) { return mhgp9::gen::run_wspd_q2_census_batched(index, k, 8, WspdFrontMode::MidpointSamples,
                  consumers, {4, 8, 64}, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, 0, proposals).pipeline.front; },
              "batched q2 census with inherited witnesses differs from the brute-force oracle");
        }
      }
    }
  }
}

// ------------------------------------------------------------- 6. rejections
void rejections(Gate& gate) {
  const Points points{{0, 0, 0}, {10, 0, 0}, {5, 4, 0}, {40, 40, 40}, {3, 30, 7}};
  const auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
  const mhgp9::gen::WspdRectangleConsumer rectangles = [](const mhgp9::gen::WspdRectangle&) {};
  const mhgp9::gen::Q2CensusConsumer sink = [](const mhgp9::gen::Q2Support&) {};
  const std::vector<mhgp9::gen::Q2CensusConsumer> sinks{sink};
  const std::span<const mhgp9::gen::Q2CensusConsumer> consumers(sinks);
  const WspdFrontProposals inheriting{1, unlimited, true};
  // The q2 lane alone: every mask with an active q3 or q4 lane is refused, by the mono
  // front and by the job factory. At Kmax 1 only q2 exists: mask 7 is then q2 alone.
  for (const std::uint8_t mask : {std::uint8_t{2}, std::uint8_t{3}, std::uint8_t{4}, std::uint8_t{5},
                                  std::uint8_t{6}, std::uint8_t{7}}) {
    gate.rejects([&] { static_cast<void>(mhgp9::gen::run_wspd_front(*index, 3, 8, WspdFrontMode::MidpointSamples,
        rectangles, mask, inheriting)); }, "mono front accepted inherited witnesses with a q3 or q4 lane");
    gate.rejects([&] { static_cast<void>(mhgp9::gen::make_wspd_front_jobs(index, 3, 8, WspdFrontMode::MidpointSamples,
        4, mask, inheriting)); }, "job factory accepted inherited witnesses with a q3 or q4 lane");
  }
  static_cast<void>(mhgp9::gen::run_wspd_front(*index, 1, 8, WspdFrontMode::MidpointSamples, rectangles, 7, inheriting));
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_wspd_front(*index, 2, 8, WspdFrontMode::Pure,
      rectangles, 1, inheriting)); }, "Pure front accepted inherited witnesses");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::make_wspd_front_jobs(index, 2, 8, WspdFrontMode::Pure,
      4, 1, inheriting)); }, "Pure job factory accepted inherited witnesses");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_wspd_q2_census(*index, 2, 8, WspdFrontMode::Pure,
      mhgp9::gen::Q2CensusMode::SharedBlocks, sink, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs,
      mhgp9::gen::Q2AnchorMode::Individual, 0, inheriting)); }, "Pure serial q2 census accepted inherited witnesses");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_wspd_q2_census_parallel(index, 2, 8, WspdFrontMode::Pure,
      mhgp9::gen::Q2CensusMode::SharedBlocks, consumers, 4, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs,
      mhgp9::gen::Q2AnchorMode::Individual, 0, {}, inheriting)); }, "Pure parallel q2 census accepted inherited witnesses");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_wspd_q2_census_cooperative(index, 2, 8, WspdFrontMode::Pure,
      consumers, {}, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, 0, inheriting)); },
      "Pure cooperative q2 census accepted inherited witnesses");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_wspd_q2_census_ranges(index, 2, 8, WspdFrontMode::Pure,
      consumers, {}, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, 0, inheriting)); },
      "Pure anchor-range q2 census accepted inherited witnesses");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_wspd_q2_census_batched(index, 2, 8, WspdFrontMode::Pure,
      consumers, {}, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, 0, inheriting)); },
      "Pure batched q2 census accepted inherited witnesses");
  // Received ranks are stored on 32 bits: the bound is judged on sizes no cloud of this gate has.
  constexpr std::uint64_t limit = std::uint64_t{1} << 32;
  static_assert(mhgp9::gen::wspd_proposals_fit({1, unlimited, true}, limit) &&
                !mhgp9::gen::wspd_proposals_fit({1, unlimited, true}, limit + 1) &&
                mhgp9::gen::wspd_proposals_fit({4, 16, false}, std::numeric_limits<std::uint64_t>::max()) &&
                mhgp9::gen::wspd_proposals_fit({}, limit + 1));
  for (const auto sites : {std::uint64_t{0}, std::uint64_t{1}, limit - 1, limit, limit + 1, limit * 4}) {
    gate.require(mhgp9::gen::wspd_proposals_fit(inheriting, sites) == (sites <= limit) &&
                     mhgp9::gen::wspd_proposals_fit({2, 16, false}, sites),
                 "32-bit rank bound of the inherited witnesses is wrong");
    ++gate.fit_checks;
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp9_gen_wspd_front_inheritance_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    named_fixtures(gate);
    model_constants(gate);
    historical_constants(gate);
    replay_corpus(gate);
    wide_ranks(gate);
    entries(gate);
    rejections(gate);
    constexpr std::size_t corpus_count = fixture_count + wide_fixture_count;
    gate.require(gate.clouds == corpus_count && gate.wide_clouds == wide_fixture_count &&
                     gate.replay_runs == corpus_count * 15 * 5 &&
                     gate.safety_runs == gate.replay_runs && gate.rejected_pairs_judged > 0 &&
                     gate.named_fixture_checks == 13 && gate.model_constant_checks == 12 &&
                     gate.historical_constant_checks == 6 && gate.wide_named_fixture_checks == 13 &&
                     gate.wide_model_constant_checks == 12 && gate.wide_historical_constant_checks == 6 &&
                     gate.nonempty_lists > 0 && gate.full_lists > 0 &&
                     gate.duplicates > 0 && gate.extension_duplicates > 0 && gate.inherited_rejections > 0 &&
                     gate.true_inherited_rejections > 0 && gate.overcounted_rejections > 0 &&
                     gate.gained_rejected_mass > 0 && gate.fewer_rejected_products > 0 && gate.mutants == 10 &&
                     gate.unsafe_mutants == 4 && gate.mutant_lost_supports > 0 &&
                     gate.entry_runs == corpus_count * 3 * 2 * 11 && gate.entry_inherited_credits > 0 &&
                     gate.supports > 0 && gate.max_shell >= 20 && gate.invalid_inputs == 19 && gate.fit_checks == 6 &&
                     gate.wide_rank_runs == 4 && gate.max_duplicate_rank >= 256,
                 "inheritance gate lost a declared non-vacuity floor");
    std::cout << "{\"schema\":\"mhgp9_gen_wspd_front_inheritance_gate_v1\",\"status\":\"passed\",\"public_status\":\"not_claimed\""
              << ",\"checks\":" << gate.checks << ",\"clouds\":" << gate.clouds << ",\"replay_runs\":" << gate.replay_runs
              << ",\"replay_products\":" << gate.replay_products << ",\"replay_rectangles\":" << gate.replay_rectangles
              << ",\"nonempty_lists\":" << gate.nonempty_lists << ",\"full_lists\":" << gate.full_lists
              << ",\"duplicates\":" << gate.duplicates << ",\"extension_duplicates\":" << gate.extension_duplicates
              << ",\"inherited_rejections\":" << gate.inherited_rejections
              << ",\"true_inherited_rejections\":" << gate.true_inherited_rejections
              << ",\"overcounted_rejections\":" << gate.overcounted_rejections
              << ",\"gained_rejected_mass\":" << gate.gained_rejected_mass
              << ",\"fewer_rejected_products\":" << gate.fewer_rejected_products
              << ",\"safety_runs\":" << gate.safety_runs << ",\"safety_pairs\":" << gate.safety_pairs
              << ",\"rejected_pairs_judged\":" << gate.rejected_pairs_judged
              << ",\"named_fixture_checks\":" << gate.named_fixture_checks
              << ",\"model_constant_checks\":" << gate.model_constant_checks
              << ",\"historical_constant_checks\":" << gate.historical_constant_checks
              << ",\"mutants\":" << gate.mutants << ",\"unsafe_mutants\":" << gate.unsafe_mutants
              << ",\"mutant_disagreements\":" << gate.mutant_disagreements
              << ",\"mutant_lost_supports\":" << gate.mutant_lost_supports << ",\"disagreements_by_mutant\":[";
    for (std::size_t m = 0; m < gate.disagreements_by_mutant.size(); ++m)
      std::cout << (m == 0 ? "" : ",") << gate.disagreements_by_mutant[m];
    std::cout << "],\"lost_supports_by_unsafe_mutant\":[";
    for (std::size_t m = 0; m < unsafe_mutants; ++m) std::cout << (m == 0 ? "" : ",") << gate.lost_supports_by_mutant[m];
    std::cout << "],\"oracle_pairs\":" << gate.oracle_pairs << ",\"oracle_sites\":" << gate.oracle_sites
              << ",\"entry_runs\":" << gate.entry_runs << ",\"entry_inherited_credits\":" << gate.entry_inherited_credits
              << ",\"supports\":" << gate.supports << ",\"max_shell\":" << gate.max_shell
              << ",\"invalid_inputs\":" << gate.invalid_inputs << ",\"fit_checks\":" << gate.fit_checks
              << ",\"wide_rank_runs\":" << gate.wide_rank_runs << ",\"max_duplicate_rank\":" << gate.max_duplicate_rank
              << ",\"wide_clouds\":" << gate.wide_clouds << ",\"wide_named_fixture_checks\":" << gate.wide_named_fixture_checks
              << ",\"wide_model_constant_checks\":" << gate.wide_model_constant_checks
              << ",\"wide_historical_constant_checks\":" << gate.wide_historical_constant_checks << "}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp9_gen_wspd_front_inheritance_gate failed: " << error.what() << '\n';
    return 1;
  }
}
