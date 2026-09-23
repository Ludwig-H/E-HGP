#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "front_fixtures.hpp"
#include "oracle/p0_oracle.hpp"
#include "wspd/front.hpp"

namespace {

using mhgp9::gen::Box3;
using mhgp9::gen::Lane;
using mhgp9::gen::Point3;
using mhgp9::gen::Q2CensusIndex;
using mhgp9::gen::Q2SpatialNode;
using mhgp9::gen::Range;
using mhgp9::gen::WspdFrontMode;
using mhgp9::gen::WspdRectangle;
using mhgp9::gen::u64;
using mhgp9::gen::oracle::Integer;
using Pair = std::pair<std::size_t, std::size_t>;
using Covers = std::array<std::vector<unsigned>, 3>;

static_assert(std::is_same_v<decltype(std::declval<const Q2CensusIndex&>().spatial_order()),
                             std::span<const std::size_t>>);
static_assert(std::is_same_v<decltype(std::declval<const Q2CensusIndex&>().spatial_nodes()),
                             std::span<const Q2SpatialNode>>);

struct Gate {
  u64 checks{};
  u64 clouds{};
  u64 spatial_nodes{};
  u64 nonidentity_orders{};
  u64 oracle_point_tests{};
  u64 front_runs{};
  u64 rectangles{};
  u64 nonsingleton_rectangles{};
  u64 partial_lane_rectangles{};
  u64 absent_lane_pairs{};
  u64 residual_lane_pairs{};
  u64 q2_support_checks{};
  u64 permutations{};
  u64 invalid_inputs{};
  u64 model_mutants{};
  u64 callback_exceptions{};
  u64 witness_searches{};
  u64 witness_descent_steps{};
  u64 proposed_sites{};
  // Widened proposal windows (WspdFrontProposals).
  u64 widened_runs{};
  u64 extended_products{};
  u64 extended_proposals{};
  u64 extended_rejections{};
  u64 extension_only_pairs{};  // q2 pairs covered by the historical window, rejected only by the extension.
  u64 limit_bites{};           // Runs whose finite small-factor limit changed the work.
  // Inherited witness identifiers (WspdFrontProposals::inherit_witnesses).
  u64 inheriting_runs{};
  u64 inherited_credits{};
  u64 inherited_duplicates{};
  u64 inheritance_only_pairs{};  // q2 pairs kept by window 4K alone, rejected with inherited witnesses.
  u64 historical_constant_checks{};
  // 18-bit twins (coordinate_limit = 262143): clouds with a coordinate above 65535 and
  // the deepest index path met, which the saturating fixture drives to max_index_depth.
  u64 wide_clouds{};
  u64 max_depth_seen{};

  void require(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
  }

  template <class Function>
  void rejects(Function&& function, const char* message) {
    bool caught = false;
    try { function(); } catch (const std::invalid_argument&) { caught = true; }
    require(caught, message);
    ++invalid_inputs;
  }
};

std::uint8_t active_mask(unsigned kmax) {
  std::uint8_t result = 0;
  for (unsigned lane = 0; lane < 3; ++lane) {
    if (lane + 2 <= kmax + 1) result |= static_cast<std::uint8_t>(1U << lane);
  }
  return result;
}

std::size_t pair_offset(std::size_t a, std::size_t b, std::size_t n) {
  if (a > b) std::swap(a, b);
  return a * n + b;
}

bool disjoint(Range a, Range b) {
  return a.last <= b.first || b.last <= a.first;
}

Box3 scanned_box(const Q2CensusIndex& index, Range range) {
  const auto order = index.spatial_order();
  const auto points = index.cloud().points();
  Box3 result{points[order[range.first]], points[order[range.first]]};
  for (std::size_t rank = range.first; rank < range.last; ++rank) {
    const auto& point = points[order[rank]];
    result.low.x = std::min(result.low.x, point.x);
    result.low.y = std::min(result.low.y, point.y);
    result.low.z = std::min(result.low.z, point.z);
    result.high.x = std::max(result.high.x, point.x);
    result.high.y = std::max(result.high.y, point.y);
    result.high.z = std::max(result.high.z, point.z);
  }
  return result;
}

// Multiprecision, scalar box definition: no product separation helper.
bool separated(const Box3& a, const Box3& b, unsigned s) {
  Integer gap_squared = 0;
  Integer a_squared = 0;
  Integer b_squared = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    Integer gap = 0;
    if (a.high[axis] < b.low[axis]) gap = Integer(b.low[axis]) - a.high[axis];
    if (b.high[axis] < a.low[axis]) gap = Integer(a.low[axis]) - b.high[axis];
    gap_squared += gap * gap;
    const Integer a_delta = Integer(a.high[axis]) - a.low[axis];
    const Integer b_delta = Integer(b.high[axis]) - b.low[axis];
    a_squared += a_delta * a_delta;
    b_squared += b_delta * b_delta;
  }
  return gap_squared >= Integer(s) * s * std::max(a_squared, b_squared);
}

void check_spatial_index(Gate& gate, const Q2CensusIndex& index) {
  const auto order = index.spatial_order();
  const auto nodes = index.spatial_nodes();
  const auto n = index.cloud().points().size();
  gate.require(order.size() == n && nodes.size() == 2 * n - 1,
               "spatial index does not contain the full immutable cloud");
  auto sorted = std::vector<std::size_t>(order.begin(), order.end());
  std::sort(sorted.begin(), sorted.end());
  bool nonidentity = false;
  for (std::size_t i = 0; i < n; ++i) {
    gate.require(sorted[i] == i, "spatial order is not a permutation of original IDs");
    nonidentity = nonidentity || order[i] != i;
  }
  gate.nonidentity_orders += static_cast<u64>(nonidentity);
  gate.require(nodes[0].range.first == 0 && nodes[0].range.last == n &&
                 nodes[0].escape == nodes.size(), "spatial root or terminal escape changed");
  gate.require(index.work().nodes == nodes.size() &&
                 index.work().escape_links == nodes.size() && index.work().max_depth <= mhgp9::gen::max_index_depth,
               "spatial index ledger or u16 depth envelope changed");
  for (std::size_t id = 0; id < nodes.size(); ++id) {
    const auto& node = nodes[id];
    gate.require(node.range.first < node.range.last && node.range.last <= n,
                 "spatial node has invalid ranks");
    const auto box = scanned_box(index, node.range);
    gate.require(node.box.low == box.low && node.box.high == box.high,
                 "spatial node box differs from its original-ID population");
    gate.require(id < node.escape && node.escape <= nodes.size(),
                 "spatial escape does not advance in this exact index");
    if (node.range.size() == 1) {
      gate.require(node.left == Q2SpatialNode::absent && node.right == Q2SpatialNode::absent &&
                       node.escape == id + 1, "spatial leaf has invalid children or escape");
    } else {
      gate.require(node.left == id + 1 && node.left < nodes.size() && node.right < nodes.size(),
                   "spatial children are not valid preorder node IDs");
      const auto& left = nodes[node.left];
      const auto& right = nodes[node.right];
      gate.require(left.range.first == node.range.first && left.range.last == right.range.first &&
                       right.range.last == node.range.last && left.escape == node.right &&
                       right.escape == node.escape,
                   "spatial children or continuation do not partition the parent");
    }
    ++gate.spatial_nodes;
  }
}

struct PairOracle {
  std::size_t n{};
  std::array<std::vector<unsigned>, 3> counts;
};

PairOracle make_oracle(Gate& gate, std::span<const Point3> points) {
  PairOracle result;
  result.n = points.size();
  for (auto& lane : result.counts) lane.resize(result.n * result.n);
  for (std::size_t a = 0; a < result.n; ++a) {
    for (std::size_t b = a + 1; b < result.n; ++b) {
      for (std::size_t z = 0; z < result.n; ++z) {
        for (unsigned lane = 0; lane < 3; ++lane) {
          const auto q = static_cast<Lane>(lane + 2);
          result.counts[lane][pair_offset(a, b, result.n)] += static_cast<unsigned>(
              mhgp9::gen::oracle::point_witness(q, points[a], points[b], points[z]));
          ++gate.oracle_point_tests;
        }
      }
    }
  }
  return result;
}

bool valid_cover(const Covers& covers, const PairOracle& oracle, unsigned kmax, bool pure,
                 std::uint8_t requested = 7) {
  for (unsigned lane = 0; lane < 3; ++lane) {
    const bool active = lane + 2 <= kmax + 1 && (requested & (1U << lane)) != 0;
    const unsigned h = active ? kmax - lane : 0;
    for (std::size_t a = 0; a < oracle.n; ++a) {
      for (std::size_t b = a + 1; b < oracle.n; ++b) {
        const auto offset = pair_offset(a, b, oracle.n);
        const unsigned count = covers[lane][offset];
        if (!active && count != 0) return false;
        if (active && (count > 1 || (pure && count != 1))) return false;
        if (active && count == 0 && oracle.counts[lane][offset] < h) return false;
      }
    }
  }
  return true;
}

std::size_t size_class(std::size_t count) {
  if (count == 1) return 0;
  if (count < 8) return 1;
  if (count < 64) return 2;
  if (count < 1024) return 3;
  return 4;
}

struct Capture {
  Covers cover;
  std::vector<WspdRectangle> rectangles;
  mhgp9::gen::WspdFrontResult result;
};

Capture checked_front(Gate& gate, const Q2CensusIndex& index, const PairOracle& oracle,
                      unsigned kmax, unsigned separation_s, WspdFrontMode mode,
                      mhgp9::gen::WspdFrontProposals proposals = {}, std::uint8_t requested = 7) {
  Capture capture;
  for (auto& lane : capture.cover) lane.resize(oracle.n * oracle.n);
  const auto nodes = index.spatial_nodes();
  const auto order = index.spatial_order();
  const auto active = static_cast<std::uint8_t>(active_mask(kmax) & requested);
  std::array<u64, 3> pair_mass{};
  std::array<u64, 3> lane_rectangles{};
  std::array<u64, 5> class_rectangles{};
  std::array<u64, 5> class_pairs{};
  u64 factor_sites = 0;
  u64 maximum_factor = 0;
  u64 leaf_pairs = 0;
  capture.result = mhgp9::gen::run_wspd_front(index, kmax, separation_s, mode,
      [&](const WspdRectangle& rectangle) {
    gate.require(rectangle.a_node < nodes.size() && rectangle.b_node < nodes.size(),
                 "front emitted node IDs outside this exact spatial index");
    const auto& a = nodes[rectangle.a_node];
    const auto& b = nodes[rectangle.b_node];
    gate.require(disjoint(a.range, b.range), "front emitted overlapping factors");
    gate.require(rectangle.lane_mask != 0 && (rectangle.lane_mask & active) == rectangle.lane_mask,
                 "front emitted empty or inactive lanes");
    gate.require(separated(a.box, b.box, separation_s),
                 "front emitted factors outside the v8 box-gap separation convention");
    const auto maximum = std::max(a.range.size(), b.range.size());
    const auto bin = size_class(maximum);
    const auto mass = static_cast<u64>(a.range.size()) * b.range.size();
    factor_sites += a.range.size() + b.range.size();
    maximum_factor = std::max(maximum_factor, static_cast<u64>(maximum));
    ++class_rectangles[bin];
    class_pairs[bin] += mass;
    leaf_pairs += static_cast<u64>(maximum == 1);
    gate.nonsingleton_rectangles += static_cast<u64>(maximum > 1);
    gate.partial_lane_rectangles += static_cast<u64>(rectangle.lane_mask != active);
    for (unsigned lane = 0; lane < 3; ++lane) {
      if ((rectangle.lane_mask & (1U << lane)) == 0) continue;
      pair_mass[lane] += mass;
      ++lane_rectangles[lane];
      for (std::size_t ai = a.range.first; ai < a.range.last; ++ai) {
        for (std::size_t bi = b.range.first; bi < b.range.last; ++bi) {
          const auto a_id = order[ai];
          const auto b_id = order[bi];
          gate.require(a_id != b_id, "front emitted a diagonal original-ID pair");
          ++capture.cover[lane][pair_offset(a_id, b_id, oracle.n)];
        }
      }
    }
    capture.rectangles.push_back(rectangle);
    ++gate.rectangles;
  }, requested, proposals);
  const auto& result = capture.result;
  const auto& work = result.work;
  const u64 total = static_cast<u64>(oracle.n) * (oracle.n - 1) / 2;
  const u64 historical_window = std::min<u64>(kmax, oracle.n);
  const u64 wider_window = std::min<u64>(u64{kmax} * proposals.window_factor, oracle.n);
  gate.require(result.total_unordered_pairs == total && result.active_lane_mask == active,
               "front changed the unordered-pair or active-lane domain");
  gate.require(work.emitted_rectangles == capture.rectangles.size() &&
                   work.emitted_factor_sites == factor_sites && work.max_factor_size == maximum_factor &&
                   work.leaf_pair_rectangles == leaf_pairs && work.size_class_rectangles == class_rectangles &&
                   work.size_class_pair_mass == class_pairs && work.residual_pair_mass == pair_mass &&
                   work.lane_rectangles == lane_rectangles,
               "front factor, size-class or residual ledger differs from independently expanded output");
  gate.require(valid_cover(capture.cover, oracle, kmax, mode == WspdFrontMode::Pure, requested),
               "front lost, duplicated, or unsafely rejected an independently judged lane pair");
  for (unsigned lane = 0; lane < 3; ++lane) {
    const bool lane_active = (active & (1U << lane)) != 0;
    gate.require(work.rejected_pair_mass[lane] + work.residual_pair_mass[lane] ==
                     (lane_active ? total : 0), "front pair ledger does not close per lane");
    if (!lane_active) continue;
    for (std::size_t a = 0; a < oracle.n; ++a) {
      for (std::size_t b = a + 1; b < oracle.n; ++b) {
        const auto count = capture.cover[lane][pair_offset(a, b, oracle.n)];
        gate.absent_lane_pairs += static_cast<u64>(count == 0);
        gate.residual_lane_pairs += static_cast<u64>(count == 1);
      }
    }
  }
  gate.require(work.diagonal_splits == oracle.n - 1 && work.diagonal_leaves == oracle.n &&
                   work.product_visits == 1 + 3 * work.diagonal_splits + 2 * work.disjoint_splits &&
                   work.product_visits == work.diagonal_splits + work.diagonal_leaves +
                       work.disjoint_splits + work.fully_rejected_products + work.emitted_rectangles,
               "front product/diagonal/split event ledger does not close");
  gate.require(work.product_visits > 0 &&
                   work.max_product_depth <= 2 * mhgp9::gen::max_index_depth && work.max_stack_size <= 2 * work.max_product_depth + 1,
               "front traversal exceeds the finite u16 depth/DFS stack envelope");
  gate.require(work.witness_searches <= work.product_visits &&
                   work.witness_descent_steps <= mhgp9::gen::max_index_depth * work.witness_searches &&
                   work.witness_box_distance_tests <= 2 * work.witness_descent_steps &&
                   work.extended_proposals <= work.proposed_sites &&
                   work.proposed_sites - work.extended_proposals <= historical_window * work.witness_searches &&
                   work.extended_proposals <= (wider_window - historical_window) * work.extended_products &&
                   work.extended_products <= work.witness_searches &&
                   work.extended_products <= work.extended_proposals &&
                   work.extended_proposals_in_factors <= work.extended_proposals &&
                   work.extended_proposals_in_factors <= work.proposals_in_factors &&
                   work.extended_credits <= work.witness_lane_credits &&
                   work.extended_inherited_duplicates <= work.inherited_duplicates &&
                   work.extended_credits + work.extended_inherited_duplicates <=
                       work.extended_proposals - work.extended_proposals_in_factors &&
                   work.extended_rejections <= work.extended_credits &&
                   work.extended_rejections <= work.extended_products &&
                   work.extended_rejections <= work.fully_rejected_products &&
                   work.h_bound_tests + work.proposals_in_factors + work.inherited_duplicates == work.proposed_sites &&
                   work.witness_lane_credits <= 3 * work.proposed_sites,
               "midpoint sampling exceeded its bounded one-path proposal envelope");
  if (proposals.window_factor == 1 || mode == WspdFrontMode::Pure) {
    gate.require(work.extended_products == 0 && work.extended_proposals == 0 &&
                     work.extended_proposals_in_factors == 0 && work.extended_credits == 0 &&
                     work.extended_rejections == 0 && work.extended_inherited_duplicates == 0,
                 "historical window reported extension work");
  }
  if (proposals.inherit_witnesses) {
    // q2 lane alone. Every searched product is rejected with Kmax credits, emitted with its
    // final credits, or split, its credits being received once by each of its two children.
    gate.require(work.inherited_duplicates <= work.inherited_credits && work.inherited_credits % 2 == 0 &&
                     work.inherited_credits <= (kmax - 1) * work.witness_searches &&
                     work.inherited_rejections <= work.fully_rejected_products &&
                     work.witness_lane_credits + work.inherited_credits / 2 ==
                         kmax * work.fully_rejected_products + work.emitted_witness_credits,
                 "inherited witness ledger does not close");
  } else {
    gate.require(work.inherited_credits == 0 && work.inherited_duplicates == 0 &&
                     work.extended_inherited_duplicates == 0 && work.inherited_rejections == 0 &&
                     work.emitted_witness_credits == 0,
                 "front without inheritance reported inherited witnesses");
  }
  if (mode == WspdFrontMode::Pure) {
    gate.require(work.witness_searches == 0 && work.witness_descent_steps == 0 &&
                     work.witness_box_distance_tests == 0 && work.proposed_sites == 0 &&
                     work.proposals_in_factors == 0 && work.h_bound_tests == 0 && work.xi_bound_tests == 0 &&
                     work.witness_lane_credits == 0 && work.fully_rejected_products == 0,
                 "Pure front performed witness work or discarded a product");
  }
  gate.witness_searches += work.witness_searches;
  gate.witness_descent_steps += work.witness_descent_steps;
  gate.proposed_sites += work.proposed_sites;
  gate.extended_products += work.extended_products;
  gate.extended_proposals += work.extended_proposals;
  gate.extended_rejections += work.extended_rejections;
  ++gate.front_runs;
  return capture;
}

std::vector<Pair> q2_supports(Gate& gate, const Covers& cover, const PairOracle& oracle,
                             unsigned kmax, std::span<const std::size_t> original_ids) {
  std::vector<Pair> result;
  for (std::size_t a = 0; a < oracle.n; ++a) {
    for (std::size_t b = a + 1; b < oracle.n; ++b) {
      const auto offset = pair_offset(a, b, oracle.n);
      if (oracle.counts[0][offset] >= kmax) continue;
      gate.require(cover[0][offset] == 1,
                   "q2 direct strict-interior census lost or duplicated an accepted support");
      result.emplace_back(std::min(original_ids[a], original_ids[b]),
                          std::max(original_ids[a], original_ids[b]));
      ++gate.q2_support_checks;
    }
  }
  std::sort(result.begin(), result.end());
  return result;
}

// Historical corpus (u16 profile) followed by its 18-bit twins. The historical
// clouds keep their oracle role unchanged; since the 18-bit widening their
// 65535 corners are interior points and exercise no bound any more.
constexpr std::size_t historical_fixture_count = 15;
constexpr std::size_t wide_fixture_count = 5;

std::vector<std::vector<Point3>> fixtures() {
  using mhgp9::gen::Coordinate;
  std::vector<std::vector<Point3>> result{
      {{7, 8, 9}},
      {{0, 0, 0}, {65535, 65535, 65535}},
      {{0, 0, 0}, {10, 0, 0}, {5, 0, 0}},
      {{0, 0, 0}, {10, 0, 0}, {5, 5, 0}},
      {{0, 3, 0}, {3, 0, 0}, {1, 1, 1}},
      {{0, 0, 0}, {6, 0, 0}, {2, 1, 1}},
      {{0, 0, 0}, {65535, 0, 0}, {0, 65535, 0}, {0, 0, 65535},
       {65535, 65535, 65535}, {65535, 65535, 0}, {65535, 0, 65535}, {0, 65535, 65535}}};
  std::vector<Point3> line;
  for (unsigned i = 0; i < 17; ++i) {
    line.push_back({static_cast<Coordinate>(i * 31), 9, 11});
  }
  result.push_back(line);
  std::vector<Point3> grid;
  for (unsigned x = 0; x < 3; ++x) {
    for (unsigned y = 0; y < 3; ++y) {
      for (unsigned z = 0; z < 3; ++z) {
        grid.push_back({static_cast<Coordinate>(x * 100),
                        static_cast<Coordinate>(y * 100),
                        static_cast<Coordinate>(z * 100)});
      }
    }
  }
  result.push_back(grid);
  std::vector<Point3> sheet;
  for (unsigned x = 0; x < 5; ++x) {
    for (unsigned y = 0; y < 5; ++y) {
      sheet.push_back({static_cast<Coordinate>(x * 37), static_cast<Coordinate>(y * 41), 73});
    }
  }
  result.push_back(sheet);
  std::vector<Point3> clusters;
  for (unsigned group = 0; group < 3; ++group) {
    for (unsigned i = 0; i < 8; ++i) {
      clusters.push_back({static_cast<Coordinate>(group * 20000 + (i & 1U)),
                          static_cast<Coordinate>((i >> 1U) & 1U),
                          static_cast<Coordinate>((i >> 2U) & 1U)});
    }
  }
  result.push_back(clusters);
  for (std::uint32_t seed : {1U, 7U, 29U, 311U}) {
    std::vector<Point3> random;
    auto state = seed;
    const auto coordinate = [&]() {
      state = state * 1664525U + 1013904223U;
      return static_cast<std::uint16_t>(state >> 16U);
    };
    for (std::size_t i = 0; i < 19; ++i) {
      // The first coordinate explicitly distinguishes sites independently
      // of the pseudo-random remaining coordinates.
      const auto x = static_cast<std::uint16_t>(i * 257 + seed % 101);
      random.push_back({x, coordinate(), coordinate()});
    }
    result.push_back(random);
  }
  if (result.size() != historical_fixture_count) throw std::logic_error("historical front corpus changed its size");
  // ---- 18-bit twins (coordinate_limit = 262143), judged by the same Boost oracle.
  static_assert(mhgp9::gen::coordinate_limit == 262143);
  // Diagonal of the whole 18-bit grid and the eight corners of its cube.
  result.push_back({{0, 0, 0}, {262143, 262143, 262143}});
  result.push_back({{0, 0, 0}, {262143, 0, 0}, {0, 262143, 0}, {0, 0, 262143},
                    {262143, 262143, 262143}, {262143, 262143, 0}, {262143, 0, 262143}, {0, 262143, 262143}});
  // The eight u16 corners are interior points of the 18-bit cube (origin shared).
  result.push_back({{0, 0, 0}, {65535, 0, 0}, {0, 65535, 0}, {0, 0, 65535},
                    {65535, 65535, 65535}, {65535, 65535, 0}, {65535, 0, 65535}, {0, 65535, 65535},
                    {262143, 0, 0}, {0, 262143, 0}, {0, 0, 262143},
                    {262143, 262143, 262143}, {262143, 262143, 0}, {262143, 0, 262143}, {0, 262143, 262143}});
  // Depth saturation: three chains of powers of two 2^0..2^17 plus 262143 on each
  // axis, sharing the origin. The index splits the longest axis at its integer
  // midpoint (ties to x): the root cube is cut on x, then y, then z, and each cut
  // halves one extent 262143 -> 65536 -> ... -> 1 -> 0, so the origin leaf sits at
  // depth 3 * 18 = max_index_depth exactly (3 * 16 = 48 on the u16 grid).
  std::vector<Point3> chains{{0, 0, 0}};
  for (std::size_t axis = 0; axis < 3; ++axis) {
    for (unsigned bit = 0; bit <= 17; ++bit) {
      Point3 point{};
      if (axis == 0) point.x = static_cast<Coordinate>(1U << bit);
      if (axis == 1) point.y = static_cast<Coordinate>(1U << bit);
      if (axis == 2) point.z = static_cast<Coordinate>(1U << bit);
      chains.push_back(point);
    }
    Point3 corner{};
    if (axis == 0) corner.x = 262143;
    if (axis == 1) corner.y = 262143;
    if (axis == 2) corner.z = 262143;
    chains.push_back(corner);
  }
  result.push_back(chains);
  // A separate 18-bit pseudo-random cloud (state >> 14 gives 18 bits); the u16
  // generators above are pinned recipes and do not change.
  for (std::uint32_t seed : {1013U}) {
    std::vector<Point3> random;
    auto state = seed;
    const auto coordinate = [&]() {
      state = state * 1664525U + 1013904223U;
      return static_cast<Coordinate>(state >> 14U);
    };
    for (std::size_t i = 0; i < 19; ++i) {
      const auto x = static_cast<Coordinate>(i * 13797 + seed % 101);  // < 262143 for i < 19.
      random.push_back({x, coordinate(), coordinate()});
    }
    result.push_back(random);
  }
  if (result.size() != historical_fixture_count + wide_fixture_count) throw std::logic_error("wide front corpus changed its size");
  return result;
}

bool wide(std::span<const Point3> points) {
  return std::any_of(points.begin(), points.end(), [](const Point3& point) {
    return point.x > 65535 || point.y > 65535 || point.z > 65535; });
}

void corpus(Gate& gate) {
  for (const auto& original : fixtures()) {
    std::array<std::vector<Pair>, 4> reference_supports;
    for (unsigned permutation = 0; permutation < 2; ++permutation) {
      auto points = original;
      std::vector<std::size_t> original_ids(points.size());
      std::iota(original_ids.begin(), original_ids.end(), std::size_t{0});
      if (permutation != 0) {
        std::reverse(points.begin(), points.end());
        std::reverse(original_ids.begin(), original_ids.end());
        if (points.size() > 2) {
          std::rotate(points.begin(), points.begin() + 1, points.end());
          std::rotate(original_ids.begin(), original_ids.begin() + 1, original_ids.end());
        }
        ++gate.permutations;
      }
      const auto expected_points = points;
      const auto cloud = mhgp9::gen::prepare_cloud(points);
      const auto index = mhgp9::gen::make_q2_cloud_index(cloud);
      ++gate.clouds;
      gate.wide_clouds += static_cast<u64>(wide(cloud->points()));
      gate.max_depth_seen = std::max(gate.max_depth_seen, index->work().max_depth);
      check_spatial_index(gate, *index);
      const auto oracle = make_oracle(gate, expected_points);
      // Mutating the caller's storage after certification must not change
      // the original-ID geometry or any subsequent front traversal.
      std::fill(points.begin(), points.end(), Point3{65535, 65535, 65535});
      gate.require(std::equal(cloud->points().begin(), cloud->points().end(), expected_points.begin()),
                   "caller mutation reached immutable cloud geometry");
      const auto nodes_before = index->spatial_nodes().data();
      const auto order_before = index->spatial_order().data();
      const auto work_before = index->work();
      std::size_t k_slot = 0;
      for (const unsigned kmax : {1U, 2U, 5U, 10U}) {
        for (const unsigned separation_s : {8U, 10U, 12U}) {
          const auto pure = checked_front(gate, *index, oracle, kmax, separation_s, WspdFrontMode::Pure);
          const auto sampled = checked_front(gate, *index, oracle, kmax, separation_s,
                                             WspdFrontMode::MidpointSamples);
          const auto pure_supports = q2_supports(gate, pure.cover, oracle, kmax, original_ids);
          gate.require(pure_supports == q2_supports(gate, sampled.cover, oracle, kmax, original_ids),
                       "sampling changed direct-census q2 supports");
          if (permutation == 0 && separation_s == 8) reference_supports[k_slot] = pure_supports;
          gate.require(pure_supports == reference_supports[k_slot],
                       "input permutation or separation changed independently accepted q2 supports");
          // Widened windows, qualified for the q2 lane alone (requested mask 1). The
          // explicit factor-1 options, with or without a finite limit, are the
          // historical front; every wider window stays sound against the same
          // independent pair oracle (checked_front), keeps the q2 supports, and can
          // only reject MORE pairs: the q2 lane falls iff the creditable ranks of
          // the whole window reach Kmax, which is monotone in the window and in
          // the small-factor limit.
          static_assert(mhgp9::gen::WspdFrontProposals{}.window_factor == 1 &&
                        mhgp9::gen::WspdFrontProposals{}.small_factor_limit == std::numeric_limits<std::size_t>::max() &&
                        !mhgp9::gen::WspdFrontProposals{}.inherit_witnesses);
          const auto unlimited = std::numeric_limits<std::size_t>::max();
          const auto q2_only = checked_front(gate, *index, oracle, kmax, separation_s,
                                             WspdFrontMode::MidpointSamples, {}, 1);
          for (const auto inert : {mhgp9::gen::WspdFrontProposals{1, unlimited}, mhgp9::gen::WspdFrontProposals{1, 1}}) {
            const auto same = checked_front(gate, *index, oracle, kmax, separation_s,
                                            WspdFrontMode::MidpointSamples, inert, 1);
            gate.require(same.result.work == q2_only.result.work && same.rectangles.size() == q2_only.rectangles.size() &&
                             std::equal(q2_only.rectangles.begin(), q2_only.rectangles.end(), same.rectangles.begin(),
                                        [](const auto& x, const auto& y) {
                                          return x.a_node == y.a_node && x.b_node == y.b_node && x.lane_mask == y.lane_mask; }),
                         "factor-1 proposals differ from the historical q2 front");
          }
          const auto subset = [&](const Capture& smaller, const Capture& larger, const char* message) {
            gate.require(smaller.result.work.rejected_pair_mass[0] >= larger.result.work.rejected_pair_mass[0], message);
            for (std::size_t offset = 0; offset < larger.cover[0].size(); ++offset)
              gate.require(smaller.cover[0][offset] <= larger.cover[0][offset], message);
          };
          const auto wide = [&](mhgp9::gen::WspdFrontProposals proposals) {
            ++gate.widened_runs;
            return checked_front(gate, *index, oracle, kmax, separation_s, WspdFrontMode::MidpointSamples, proposals, 1);
          };
          const auto wide2 = wide({2, unlimited}), wide2_small = wide({2, 2});
          const auto wide4 = wide({4, unlimited}), wide4_leaf = wide({4, 1});
          for (const auto* capture : {&wide2, &wide2_small, &wide4, &wide4_leaf})
            gate.require(pure_supports == q2_supports(gate, capture->cover, oracle, kmax, original_ids),
                         "widened proposals changed direct-census q2 supports");
          subset(wide2, q2_only, "window 2K rejected fewer pairs than the historical window");
          subset(wide2_small, q2_only, "limited window 2K rejected fewer pairs than the historical window");
          subset(wide2, wide2_small, "unlimited window 2K rejected fewer pairs than its limited variant");
          subset(wide4, wide2, "window 4K rejected fewer pairs than window 2K");
          subset(wide4, wide4_leaf, "unlimited window 4K rejected fewer pairs than its leaf-only variant");
          subset(wide4_leaf, q2_only, "leaf-only window 4K rejected fewer pairs than the historical window");
          gate.limit_bites += static_cast<u64>(!(wide2_small.result.work == wide2.result.work)) +
                              static_cast<u64>(!(wide4_leaf.result.work == wide4.result.work));
          for (std::size_t offset = 0; offset < q2_only.cover[0].size(); ++offset)
            gate.extension_only_pairs += static_cast<u64>(q2_only.cover[0][offset] == 1 && wide4.cover[0][offset] == 0);
          // Inherited witness identifiers (q2 lane alone). The same independent pair
          // oracle judges every rejection (checked_front): a received rank counted twice
          // would reject a pair below Kmax. A search with a received list never stops
          // later than the same window alone: supports are unchanged, and the pairs
          // rejected can only grow, in the window as in the limit.
          const auto inherit = [&](mhgp9::gen::WspdFrontProposals proposals) {
            ++gate.inheriting_runs;
            auto capture = checked_front(gate, *index, oracle, kmax, separation_s, WspdFrontMode::MidpointSamples, proposals, 1);
            gate.inherited_credits += capture.result.work.inherited_credits;
            gate.inherited_duplicates += capture.result.work.inherited_duplicates;
            return capture;
          };
          const auto inherit1 = inherit({1, unlimited, true}), inherit2 = inherit({2, unlimited, true});
          const auto inherit4 = inherit({4, unlimited, true}), inherit4_leaf = inherit({4, 1, true});
          for (const auto* capture : {&inherit1, &inherit2, &inherit4, &inherit4_leaf})
            gate.require(pure_supports == q2_supports(gate, capture->cover, oracle, kmax, original_ids),
                         "inherited witnesses changed direct-census q2 supports");
          subset(inherit1, q2_only, "inherited witnesses rejected fewer pairs than the historical window");
          subset(inherit2, wide2, "inherited witnesses rejected fewer pairs than window 2K alone");
          subset(inherit4, wide4, "inherited witnesses rejected fewer pairs than window 4K alone");
          subset(inherit4_leaf, wide4_leaf, "inherited witnesses rejected fewer pairs than leaf-only window 4K alone");
          subset(inherit2, inherit1, "window 2K with inherited witnesses rejected fewer pairs than the historical one");
          subset(inherit4, inherit2, "window 4K with inherited witnesses rejected fewer pairs than window 2K");
          subset(inherit4, inherit4_leaf, "unlimited window 4K with inherited witnesses rejected fewer pairs than leaf-only");
          for (std::size_t offset = 0; offset < wide4.cover[0].size(); ++offset)
            gate.inheritance_only_pairs += static_cast<u64>(wide4.cover[0][offset] == 1 && inherit4.cover[0][offset] == 0);
        }
        ++k_slot;
      }
      const auto work_after = index->work();
      gate.require(nodes_before == index->spatial_nodes().data() && order_before == index->spatial_order().data() &&
                       work_before.nodes == work_after.nodes && work_before.point_visits == work_after.point_visits &&
                       work_before.max_depth == work_after.max_depth && work_before.escape_links == work_after.escape_links,
                   "front mutated or rebuilt the shared spatial index");
      check_spatial_index(gate, *index);
    }
  }
}

void rejection_and_models(Gate& gate) {
  const std::vector<Point3> line{{0, 0, 0}, {10, 0, 0}, {5, 0, 0}};
  const auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(line));
  const auto oracle = make_oracle(gate, line);
  const auto consumer = [](const WspdRectangle&) {};
  for (const unsigned kmax : {0U, 11U, 999U}) {
    gate.rejects([&] { static_cast<void>(mhgp9::gen::run_wspd_front(*index, kmax, 8, WspdFrontMode::Pure, consumer)); },
                 "front accepted an invalid Kmax");
  }
  for (const unsigned factor : {0U, 3U, 5U, 8U}) {
    gate.rejects([&] { static_cast<void>(mhgp9::gen::run_wspd_front(*index, 2, 8, WspdFrontMode::MidpointSamples, consumer, 1,
                                                        mhgp9::gen::WspdFrontProposals{factor, 16})); },
                 "front accepted an invalid proposal window factor");
  }
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_wspd_front(*index, 2, 8, WspdFrontMode::MidpointSamples, consumer, 1,
                                                      mhgp9::gen::WspdFrontProposals{2, 0})); },
               "front accepted a zero small-factor limit");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_wspd_front(*index, 2, 8, WspdFrontMode::Pure, consumer, 1,
                                                      mhgp9::gen::WspdFrontProposals{2, 16})); },
               "Pure front accepted a widened proposal window");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::make_wspd_front_jobs(index, 2, 8, WspdFrontMode::MidpointSamples, 4, 1,
                                                            mhgp9::gen::WspdFrontProposals{3, 16})); },
               "front jobs accepted an invalid proposal window factor");
  for (const std::uint8_t mask : {std::uint8_t{7}, std::uint8_t{3}, std::uint8_t{2}}) {
    gate.rejects([&] { static_cast<void>(mhgp9::gen::run_wspd_front(*index, 2, 8, WspdFrontMode::MidpointSamples, consumer, mask,
                                                        mhgp9::gen::WspdFrontProposals{2, 16})); },
                 "front widened the proposal window of a q3/q4 lane");
  }
  gate.rejects([&] { static_cast<void>(mhgp9::gen::make_wspd_front_jobs(index, 5, 8, WspdFrontMode::MidpointSamples, 4, 7,
                                                            mhgp9::gen::WspdFrontProposals{4, 16})); },
               "front jobs widened the proposal window of a q3/q4 lane");
  const mhgp9::gen::WspdFrontProposals inheriting{1, std::numeric_limits<std::size_t>::max(), true};
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_wspd_front(*index, 2, 8, WspdFrontMode::Pure, consumer, 1, inheriting)); },
               "Pure front accepted inherited witnesses");
  for (const std::uint8_t mask : {std::uint8_t{7}, std::uint8_t{3}, std::uint8_t{2}}) {
    gate.rejects([&] { static_cast<void>(mhgp9::gen::run_wspd_front(*index, 2, 8, WspdFrontMode::MidpointSamples, consumer, mask,
                                                        inheriting)); },
                 "front inherited witnesses with a q3/q4 lane");
  }
  gate.rejects([&] { static_cast<void>(mhgp9::gen::make_wspd_front_jobs(index, 5, 8, WspdFrontMode::MidpointSamples, 4, 7,
                                                            inheriting)); },
               "front jobs inherited witnesses with a q3/q4 lane");
  for (const unsigned s : {0U}) {
    gate.rejects([&] { static_cast<void>(mhgp9::gen::run_wspd_front(*index, 2, s, WspdFrontMode::Pure, consumer)); },
                 "front accepted a zero separation");
  }
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_wspd_front(*index, 2, 8,
                     static_cast<WspdFrontMode>(99), consumer)); }, "front accepted an invalid mode");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::run_wspd_front(*index, 2, 8, WspdFrontMode::Pure, {})); },
               "front accepted an empty callback");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::prepare_cloud(std::vector<Point3>{{1, 2, 3}, {1, 2, 3}})); },
               "front foundation accepted duplicate coordinates");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::prepare_cloud({})); },
               "front foundation accepted an empty cloud");
  gate.rejects([&] { static_cast<void>(mhgp9::gen::make_q2_cloud_index({})); },
               "front foundation accepted a null cloud");

  const auto pure = checked_front(gate, *index, oracle, 2, 8, WspdFrontMode::Pure);
  const auto ab = pair_offset(0, 1, line.size());
  auto loss = pure.cover;
  loss[0][ab] = 0;
  gate.require(!valid_cover(loss, oracle, 2, true) && !valid_cover(loss, oracle, 2, false),
               "lost accepted pair model escaped independent coverage checks");
  ++gate.model_mutants;
  auto duplicate = pure.cover;
  duplicate[0][ab] = 2;
  gate.require(!valid_cover(duplicate, oracle, 2, true),
               "duplicate emitted pair model escaped independent coverage checks");
  ++gate.model_mutants;

  gate.require(oracle.counts[0][ab] == 1 && oracle.counts[2][ab] == 1 &&
                   oracle.counts[2][ab] >= mhgp9::gen::oracle::threshold(3, Lane::Q4) &&
                   oracle.counts[0][ab] < mhgp9::gen::oracle::threshold(3, Lane::Q2),
               "q4 rejection incorrectly implies q2 rejection fixture became vacuous");
  const auto pure_k3 = checked_front(gate, *index, oracle, 3, 8, WspdFrontMode::Pure);
  auto wrong_lane = pure_k3.cover;
  wrong_lane[0][ab] = 0;
  gate.require(!valid_cover(wrong_lane, oracle, 3, false), "q4-to-q2 lane erasure model survived");
  ++gate.model_mutants;

  gate.require(mhgp9::gen::oracle::point_witness(Lane::Q2, line[0], line[1], line[2]) &&
                   oracle.counts[0][ab] < 2 && 2 * oracle.counts[0][ab] >= 2,
               "rechecking an inherited witness did not expose the double-credit model");
  const auto sampled = checked_front(gate, *index, oracle, 2, 8, WspdFrontMode::MidpointSamples);
  gate.require(sampled.cover[0][ab] == 1 && !valid_cover(loss, oracle, 2, false),
               "repeated-ancestor credit removed an accepted q2 support");
  ++gate.model_mutants;

  const std::vector<Point3> shell{{0, 0, 0}, {10, 0, 0}, {5, 5, 0}};
  const auto shell_oracle = make_oracle(gate, shell);
  const auto shell_index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(shell));
  const auto shell_front = checked_front(gate, *shell_index, shell_oracle, 1, 8,
                                         WspdFrontMode::MidpointSamples);
  const auto shell_h = mhgp9::gen::oracle::metrics(shell[0], shell[1], shell[2]).h;
  gate.require(shell_h == 0 && shell_h >= 0 && shell_oracle.counts[0][ab] == 0 && shell_front.cover[0][ab] == 1,
               "nonstrict shell credit model no longer differs from the strict oracle");
  auto shell_loss = shell_front.cover;
  shell_loss[0][ab] = 0;
  gate.require(!valid_cover(shell_loss, shell_oracle, 1, false), "nonstrict shell-credit model survived");
  ++gate.model_mutants;

  for (const unsigned q : {3U, 4U}) {
    const std::vector<Point3> boundary = q == 3
        ? std::vector<Point3>{{0, 3, 0}, {3, 0, 0}, {1, 1, 1}}
        : std::vector<Point3>{{0, 0, 0}, {6, 0, 0}, {2, 1, 1}};
    const auto value = mhgp9::gen::oracle::metrics(boundary[0], boundary[1], boundary[2]);
    const unsigned coefficient = q == 3 ? 3 : 2;
    const auto boundary_oracle = make_oracle(gate, boundary);
    const auto boundary_index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(boundary));
    const auto boundary_front = checked_front(gate, *boundary_index, boundary_oracle, q - 1, 8,
                                              WspdFrontMode::MidpointSamples);
    gate.require(value.h > 0 && coefficient * value.h * value.h == value.xi &&
                     boundary_oracle.counts[q - 2][ab] == 0 && boundary_front.cover[q - 2][ab] == 1,
                 "strict spindle-boundary fixture no longer distinguishes equality from interior");
    auto nonstrict = boundary_front.cover;
    nonstrict[q - 2][ab] = 0;
    gate.require(!valid_cover(nonstrict, boundary_oracle, q - 1, false),
                 "nonstrict q3/q4 spindle-credit model survived");
    ++gate.model_mutants;
  }

  struct CallbackFailure {};
  unsigned emissions = 0;
  bool caught = false;
  try {
    static_cast<void>(mhgp9::gen::run_wspd_front(*index, 2, 8, WspdFrontMode::Pure,
        [&](const WspdRectangle&) { ++emissions; throw CallbackFailure{}; }));
  } catch (const CallbackFailure&) { caught = true; }
  gate.require(caught && emissions == 1, "front swallowed a callback exception or continued emitting");
  const auto retry = checked_front(gate, *index, oracle, 2, 8, WspdFrontMode::Pure);
  gate.require(retry.cover == pure.cover, "callback failure changed a subsequent fresh front traversal");
  ++gate.callback_exceptions;
}

// Counters of the DEFAULT three-lane front before the widened-window tranche (commit
// 8d615cfd, pinned tranche-19 library), n = 2000, Kmax 10, s 8, seed 3: the historical
// path, Xi tests and q3/q4 credits included, must not drift when the option exists.
void historical_constants(Gate& gate) {
  struct Constants { const char* family; u64 visits, searches, proposed, in_factors, h_tests, xi_tests, credits,
                         rejected_products, emitted; std::array<u64, 3> rejected, residual; };
  for (const auto& c : {Constants{"uniform", 1601268, 1597268, 15879672, 343169, 15536503, 11535369, 18576701,
                                  142626, 657008, {1389928, 799768, 861465}, {609072, 1199232, 1137535}},
                        Constants{"clusters", 381576, 377576, 3774689, 236437, 3538252, 1845108, 3263268,
                                  2733, 187055, {35432, 6271, 8176}, {1963568, 1992729, 1990824}}}) {
    const auto fixture = mhgp9::gen::bench::make_front_fixture(2000, c.family, 3);
    const auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(fixture.points));
    const auto w = mhgp9::gen::run_wspd_front(*index, 10, 8, WspdFrontMode::MidpointSamples,
                                         [](const WspdRectangle&) {}).work;
    gate.require(w.product_visits == c.visits && w.witness_searches == c.searches && w.proposed_sites == c.proposed &&
                     w.proposals_in_factors == c.in_factors && w.h_bound_tests == c.h_tests &&
                     w.xi_bound_tests == c.xi_tests && w.witness_lane_credits == c.credits &&
                     w.fully_rejected_products == c.rejected_products && w.emitted_rectangles == c.emitted &&
                     w.rejected_pair_mass == c.rejected && w.residual_pair_mass == c.residual &&
                     w.extended_products == 0 && w.extended_proposals == 0 && w.extended_rejections == 0,
                 "default three-lane front drifted from its engraved pre-tranche counters");
    ++gate.historical_constant_checks;
  }
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp9_gen_wspd_front_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    corpus(gate);
    rejection_and_models(gate);
    historical_constants(gate);
    gate.require(gate.clouds >= 30 && gate.front_runs >= 720 && gate.spatial_nodes > 1000 &&
                     gate.nonidentity_orders >= 10 && gate.oracle_point_tests > 200000 &&
                     gate.nonsingleton_rectangles > 0 && gate.partial_lane_rectangles > 0 &&
                     gate.absent_lane_pairs > 0 && gate.residual_lane_pairs > 0 &&
                     gate.q2_support_checks > 1000 && gate.permutations == historical_fixture_count + wide_fixture_count &&
                     gate.wide_clouds == 2 * wide_fixture_count && gate.max_depth_seen >= mhgp9::gen::max_index_depth &&
                     gate.invalid_inputs >= 25 && gate.model_mutants >= 7 && gate.callback_exceptions == 1 &&
                     gate.witness_searches > 0 && gate.witness_descent_steps > 0 && gate.proposed_sites > 0 &&
                     gate.widened_runs >= 1440 && gate.extended_products > 0 && gate.extended_proposals > 0 &&
                     gate.extended_rejections > 0 && gate.extension_only_pairs > 0 && gate.limit_bites > 0 &&
                     gate.inheriting_runs >= 1440 && gate.inherited_credits > 0 && gate.inherited_duplicates > 0 &&
                     gate.inheritance_only_pairs > 0 &&
                     gate.historical_constant_checks == 2,
                 "WSPD front qualification lost a non-vacuity floor");
    std::cout << "mhgp9_gen_wspd_front_gate passed checks=" << gate.checks << " clouds=" << gate.clouds
              << " spatial_nodes=" << gate.spatial_nodes << " nonidentity_orders=" << gate.nonidentity_orders
              << " oracle_point_tests=" << gate.oracle_point_tests << " front_runs=" << gate.front_runs
              << " rectangles=" << gate.rectangles << " nonsingleton_rectangles=" << gate.nonsingleton_rectangles
              << " partial_lane_rectangles=" << gate.partial_lane_rectangles
              << " absent_lane_pairs=" << gate.absent_lane_pairs << " residual_lane_pairs=" << gate.residual_lane_pairs
              << " q2_support_checks=" << gate.q2_support_checks << " permutations=" << gate.permutations
              << " invalid_inputs=" << gate.invalid_inputs << " model_mutants=" << gate.model_mutants
              << " callback_exceptions=" << gate.callback_exceptions << " witness_searches=" << gate.witness_searches
              << " witness_descent_steps=" << gate.witness_descent_steps << " proposed_sites=" << gate.proposed_sites
              << " widened_runs=" << gate.widened_runs << " extended_products=" << gate.extended_products
              << " extended_proposals=" << gate.extended_proposals
              << " extended_rejections=" << gate.extended_rejections
              << " extension_only_pairs=" << gate.extension_only_pairs << " limit_bites=" << gate.limit_bites
              << " inheriting_runs=" << gate.inheriting_runs << " inherited_credits=" << gate.inherited_credits
              << " inherited_duplicates=" << gate.inherited_duplicates
              << " inheritance_only_pairs=" << gate.inheritance_only_pairs
              << " wide_clouds=" << gate.wide_clouds << " max_depth_seen=" << gate.max_depth_seen << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "mhgp9_gen_wspd_front_gate failed: " << error.what() << '\n';
    return 1;
  }
}
