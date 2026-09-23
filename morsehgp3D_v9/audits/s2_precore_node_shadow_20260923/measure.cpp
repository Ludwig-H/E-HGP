#include "gen/lanes/q34_witness_search.hpp"
#include "gen/pipeline/prepared_cloud.hpp"
#include "gen/wspd/front.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

using namespace mhgp9::gen;

namespace {

using i64 = std::int64_t;
using u64 = std::uint64_t;
constexpr unsigned kmax = 5, min_segment = 16, cells = 8, vertices = 27;

struct Trace {
  std::uint32_t core_sites{};
  std::uint8_t mask{}, post{};
  bool seen{}, has_post{};
};
struct Edge {
  std::size_t a{}, b{};
  Trace* trace{};
};
struct CellResult {
  bool proved{}, exhausted_budget{};
  unsigned credit{};
};
struct Stats {
  u64 segments{}, edges{}, forms{}, closed_segments{}, closed_edges{}, closed_forms{};
  u64 proved_cells{}, budget_cells{}, exhausted_cells{}, q_terms{}, node_visits{};
  u64 node_tests{}, vertex_tests{}, excluded_node_visits{}, excluded_subtree_skips{};
  u64 credited_nodes{}, credited_sites{};
  u64 obstructed_cells{}, obstructed_segments{}, obstructed_forms{};
  u64 post_zero_edges{}, post_nonzero_edges{}, no_post_edges{}, shadow_ns{};
  std::array<u64, 9> segments_by_proved_cells{}, forms_by_proved_cells{};
};

std::vector<unsigned char> bytes(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) throw std::runtime_error("could not open " + path.string());
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
std::uint32_t u32(const unsigned char* p) {
  return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
         (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}
u64 edge_key(std::uint32_t a, std::uint32_t b) {
  if (a == b) throw std::runtime_error("self edge");
  if (a > b) std::swap(a, b);
  return (u64(a) << 32) | b;
}
i64 coordinate(Point3 p, unsigned axis) { return static_cast<i64>(p[axis]); }
i64 squared_edge(Point3 a, Point3 b) {
  i64 d = 0;
  for (unsigned i = 0; i != 3; ++i) {
    const auto x = coordinate(a, i) - coordinate(b, i);
    d += x * x;
  }
  return d;
}
// Smallest integer r with 8r² >= D, hence a closed exterior AABB for the
// nominal q4 centre disk of a longest-edge positive presentation.
i64 exterior_radius(i64 d) {
  if (d <= 0 || d > 3LL * 262143 * 262143) throw std::runtime_error("invalid edge length");
  i64 lo = 0, hi = 262144;
  while (hi - lo > 1) {
    const auto m = (lo + hi) / 2;
    if (8 * m * m >= d) hi = m;
    else lo = m;
  }
  return hi;
}
std::unordered_map<u64, Trace> read_trace(const std::filesystem::path& folder, bool after) {
  u64 expected = 0;
  for (unsigned part = 0; part != 8; ++part) {
    const auto file = folder / ("part_" + std::to_string(part) + ".bin");
    const auto size = std::filesystem::file_size(file);
    if (size % 16 != 0) throw std::runtime_error("unaligned trace file");
    expected += size / 16;
  }
  if (expected > std::numeric_limits<std::size_t>::max() / 2) throw std::runtime_error("huge trace");
  std::unordered_map<u64, Trace> result;
  result.reserve(static_cast<std::size_t>(expected * 2));
  for (unsigned part = 0; part != 8; ++part) {
    const auto data = bytes(folder / ("part_" + std::to_string(part) + ".bin"));
    for (std::size_t off = 0; off < data.size(); off += 16) {
      const auto a = u32(data.data() + off), b = u32(data.data() + off + 4);
      const auto count = u32(data.data() + off + 8), word = u32(data.data() + off + 12);
      const auto mask = static_cast<std::uint8_t>(after ? word & 0xffU : word);
      const auto post = static_cast<std::uint8_t>(after ? (word >> 8) & 0xffU : 0);
      if (a >= b || count < 2 || (mask != 2 && mask != 4 && mask != 6) ||
          (after && ((word >> 16) != 0 || (post != 0 && post != 2 && post != 4 && post != 6) ||
                     (post & ~mask) != 0)))
        throw std::runtime_error("invalid trace record");
      const auto [it, inserted] = result.emplace(edge_key(a, b), Trace{count, mask, post, false, after});
      if (!inserted) throw std::runtime_error("duplicate trace record");
      (void)it;
    }
  }
  if (result.size() != expected) throw std::runtime_error("trace count mismatch");
  return result;
}

// Fourfold integer centre coordinates: original u18 sites are multiplied by
// four. The original bounding box and midpoint-rounded q4 envelope are
// dyadic, and one bisection on each axis has exact integer v4 coordinates.
using V4 = std::array<i64, 3>;

std::array<V4, vertices> grid_vertices(const std::vector<Edge>& edges,
                                       std::span<const Point3> points, const Box3& cloud_box) {
  std::array<i64, 3> lo2{}, hi2{};
  lo2.fill(std::numeric_limits<i64>::max());
  hi2.fill(std::numeric_limits<i64>::min());
  for (const auto& e : edges) {
    const auto a = points[e.a], b = points[e.b];
    const auto radius = exterior_radius(squared_edge(a, b));
    for (unsigned axis = 0; axis != 3; ++axis) {
      const auto mid2 = coordinate(a, axis) + coordinate(b, axis);
      lo2[axis] = std::min(lo2[axis], mid2 - 2 * radius);
      hi2[axis] = std::max(hi2[axis], mid2 + 2 * radius);
    }
  }
  std::array<std::array<i64, 3>, 3> axis_grid{};
  for (unsigned axis = 0; axis != 3; ++axis) {
    lo2[axis] = std::max(lo2[axis], 2 * coordinate(cloud_box.low, axis));
    hi2[axis] = std::min(hi2[axis], 2 * coordinate(cloud_box.high, axis));
    if (lo2[axis] > hi2[axis]) throw std::runtime_error("empty clipped centre box");
    axis_grid[axis] = {2 * lo2[axis], lo2[axis] + hi2[axis], 2 * hi2[axis]};
  }
  std::array<V4, vertices> result{};
  for (unsigned x = 0; x != 3; ++x)
    for (unsigned y = 0; y != 3; ++y)
      for (unsigned z = 0; z != 3; ++z)
        result[(x * 3 + y) * 3 + z] = {axis_grid[0][x], axis_grid[1][y], axis_grid[2][z]};
  return result;
}

i64 scaled_squared(Point3 p, const V4& v) {
  i64 sum = 0;
  for (unsigned axis = 0; axis != 3; ++axis) {
    const auto delta = 4 * coordinate(p, axis) - v[axis];
    sum += delta * delta;
  }
  return sum;
}
i64 node_farthest_squared(const Box3& box, const V4& v) {
  i64 sum = 0;
  for (unsigned axis = 0; axis != 3; ++axis) {
    const auto low = 4 * coordinate(box.low, axis) - v[axis];
    const auto high = 4 * coordinate(box.high, axis) - v[axis];
    sum += std::max(low * low, high * high);
  }
  return sum;
}
// Sufficient *impossibility* check for this common-guard certificate: the
// full nominal q4 centre disk of an S2-surviving edge lies in this cell.
// Its exact axial projections have squared halfwidth (D-d_i²)/8. If common
// guards certified the cell, S2 would already have rejected that edge.
bool disk_inside_cell(Point3 a, Point3 b, const V4& low, const V4& high) {
  const auto d2 = squared_edge(a, b);
  for (unsigned axis = 0; axis != 3; ++axis) {
    const auto d = coordinate(a, axis) - coordinate(b, axis);
    const auto mid4 = 2 * (coordinate(a, axis) + coordinate(b, axis));
    const auto l = mid4 - low[axis], h = high[axis] - mid4;
    const auto need = 2 * (d2 - d * d);
    if (l < 0 || h < 0 || l * l < need || h * h < need) return false;
  }
  return true;
}
bool overlaps(Range x, Range y) { return x.first < y.last && y.first < x.last; }
bool within(Range x, Range y) { return y.first <= x.first && x.last <= y.last; }
i64 nearest_to_center8(const Box3& box, const V4& low, const V4& high) {
  i64 sum = 0;
  for (unsigned axis = 0; axis != 3; ++axis) {
    const auto center8 = low[axis] + high[axis];
    const auto lo8 = 8 * coordinate(box.low, axis), hi8 = 8 * coordinate(box.high, axis);
    const auto d = center8 < lo8 ? lo8 - center8 : center8 > hi8 ? center8 - hi8 : 0;
    sum += d * d;
  }
  return sum;
}

CellResult certify_cell(const Q2CensusIndex& index,
                        const std::array<V4, vertices>& grid,
                        const std::array<i64, vertices>& q,
                        Range a, Range b, unsigned cell, unsigned budget,
                        bool near_first, std::array<std::size_t, kmax - 1>& guards,
                        Stats& stats) {
  const auto nodes = index.spatial_nodes();
  const auto order = index.spatial_order();
  const unsigned bx = (cell >> 2) & 1U, by = (cell >> 1) & 1U, bz = cell & 1U;
  unsigned count = 0, visits = 0;
  std::size_t cursor = 0;
  std::array<std::size_t, 64> stack{};
  unsigned size = 0;
  if (near_first) stack[size++] = 0;
  const auto cell_low = grid[(bx * 3 + by) * 3 + bz];
  const auto cell_high = grid[((bx + 1) * 3 + by + 1) * 3 + bz + 1];
  while ((near_first ? size != 0 : cursor < nodes.size()) && count < kmax - 1 && visits < budget) {
    const auto id = near_first ? stack[--size] : cursor;
    const auto& node = nodes[id];
    ++visits;
    ++stats.node_visits;
    if (within(node.range, a) || within(node.range, b)) {
      ++stats.excluded_subtree_skips;
      if (!near_first) cursor = node.escape;
      continue;
    }
    const bool excluded = overlaps(node.range, a) || overlaps(node.range, b);
    if (excluded) ++stats.excluded_node_visits;
    bool admitted = !excluded;
    if (admitted) {
      ++stats.node_tests;
      for (unsigned dx = 0; dx != 2 && admitted; ++dx)
        for (unsigned dy = 0; dy != 2 && admitted; ++dy)
          for (unsigned dz = 0; dz != 2 && admitted; ++dz) {
            const unsigned vi = ((bx + dx) * 3 + by + dy) * 3 + bz + dz;
            ++stats.vertex_tests;
            if (2 * node_farthest_squared(node.box, grid[vi]) >= q[vi]) admitted = false;
          }
    }
    if (admitted) {
      ++stats.credited_nodes;
      stats.credited_sites += node.range.size();
      for (auto rank = node.range.first; rank < node.range.last && count < kmax - 1; ++rank)
        guards[count++] = order[rank];
      if (!near_first) cursor = node.escape;
    } else if (node.left != Q2SpatialNode::absent) {
      if (!near_first) cursor = node.left;
      else {
        if (size + 2 > stack.size()) throw std::runtime_error("near DFS stack overflow");
        const auto dl = nearest_to_center8(nodes[node.left].box, cell_low, cell_high);
        const auto dr = nearest_to_center8(nodes[node.right].box, cell_low, cell_high);
        const auto near = dl <= dr ? node.left : node.right;
        const auto far = dl <= dr ? node.right : node.left;
        stack[size++] = far;
        stack[size++] = near;
      }
    } else {
      if (!near_first) cursor = node.escape;
    }
  }
  const bool proved = count >= kmax - 1;
  const bool budget_hit = !proved && visits == budget &&
      (near_first ? size != 0 : cursor < nodes.size());
  stats.proved_cells += proved;
  stats.budget_cells += budget_hit;
  stats.exhausted_cells += !proved && !budget_hit;
  return {proved, budget_hit, count};
}

void shadow_segment(const Q2CensusIndex& index, const WspdRectangle& rectangle,
                    const std::vector<Edge>& edges, unsigned budget,
                    bool near_first, const std::vector<std::uint32_t>& raw_ids,
                    Stats& stats) {
  const auto start = std::chrono::steady_clock::now();
  const auto points = index.cloud().points();
  const auto grid = grid_vertices(edges, points, index.spatial_nodes()[0].box);
  std::array<i64, vertices> q;
  q.fill(std::numeric_limits<i64>::max());
  u64 forms = 0;
  for (const auto& e : edges) {
    forms += e.trace->core_sites;
    for (unsigned vi = 0; vi != vertices; ++vi) {
      q[vi] = std::min(q[vi], scaled_squared(points[e.a], grid[vi]) +
                                scaled_squared(points[e.b], grid[vi]));
      ++stats.q_terms;
    }
  }
  const auto nodes = index.spatial_nodes();
  const auto a = nodes[rectangle.a_node].range, b = nodes[rectangle.b_node].range;
  unsigned proved = 0;
  bool obstruction = false;
  std::array<std::array<std::size_t, kmax - 1>, cells> guards{};
  for (unsigned cell = 0; cell != cells; ++cell) {
    const unsigned bx = (cell >> 2) & 1U, by = (cell >> 1) & 1U, bz = cell & 1U;
    const auto low = grid[(bx * 3 + by) * 3 + bz];
    const auto high = grid[((bx + 1) * 3 + by + 1) * 3 + bz + 1];
    bool blocked = false;
    for (const auto& edge : edges)
      if (disk_inside_cell(points[edge.a], points[edge.b], low, high)) {
        blocked = true;
        break;
      }
    obstruction |= blocked;
    stats.obstructed_cells += blocked;
    const auto result = certify_cell(index, grid, q, a, b, cell, budget, near_first,
                                     guards[cell], stats);
    if (blocked && result.proved)
      throw std::runtime_error("certified cell contradicts S2 nominal-disk obstruction");
    proved += result.proved;
  }
  ++stats.segments;
  stats.edges += edges.size();
  stats.forms += forms;
  if (obstruction) {
    ++stats.obstructed_segments;
    stats.obstructed_forms += forms;
  }
  ++stats.segments_by_proved_cells[proved];
  stats.forms_by_proved_cells[proved] += forms;
  if (proved == cells) {
    ++stats.closed_segments;
    stats.closed_edges += edges.size();
    stats.closed_forms += forms;
    for (const auto& e : edges) {
      if (!e.trace->has_post) ++stats.no_post_edges;
      else if (e.trace->post == 0) ++stats.post_zero_edges;
      else {
        ++stats.post_nonzero_edges;
        const auto raw_a = raw_ids[e.a], raw_b = raw_ids[e.b];
        std::cout << "anomaly " << raw_a << ' ' << raw_b << ' '
                  << unsigned(e.trace->mask) << ' ' << unsigned(e.trace->post)
                  << ' ' << e.trace->core_sites << '\n';
        for (unsigned cell = 0; cell != cells; ++cell) {
          const unsigned bx = (cell >> 2) & 1U, by = (cell >> 1) & 1U, bz = cell & 1U;
          const auto low = grid[(bx * 3 + by) * 3 + bz];
          const auto high = grid[((bx + 1) * 3 + by + 1) * 3 + bz + 1];
          std::cout << "proof " << raw_a << ' ' << raw_b << ' ' << cell;
          for (unsigned axis = 0; axis != 3; ++axis) std::cout << ' ' << low[axis];
          for (unsigned axis = 0; axis != 3; ++axis) std::cout << ' ' << high[axis];
          for (const auto id : guards[cell]) std::cout << ' ' << raw_ids[id];
          std::cout << '\n';
        }
      }
    }
  }
  stats.shadow_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now() - start).count();
}

}  // namespace

int main(int argc, char** argv) {
  // points.u32le, raw IDs.u32le, before trace dir, optional after trace dir,
  // --budget=N and --near. The baseline is preorder DFS and 64 visits/cell.
  if (argc < 4 || argc > 7) return 2;
  std::filesystem::path after_folder;
  unsigned budget = 64;
  bool near_first = false;
  for (int i = 4; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--near") near_first = true;
    else if (arg.starts_with("--budget=")) {
      const auto value = std::stoul(arg.substr(9));
      if (value == 0 || value > 1000000) throw std::invalid_argument("invalid node visit budget");
      budget = static_cast<unsigned>(value);
    } else if (after_folder.empty()) after_folder = arg;
    else throw std::invalid_argument("duplicate after trace or unknown option");
  }
  const auto started = std::chrono::steady_clock::now();
  const auto point_data = bytes(argv[1]), raw_data = bytes(argv[2]);
  if (point_data.empty() || point_data.size() % 12 || raw_data.size() != point_data.size() / 3)
    throw std::runtime_error("unaligned points/raw IDs");
  const auto n = point_data.size() / 12;
  std::vector<Point3> points(n);
  std::vector<std::uint32_t> raw_ids(n);
  for (std::size_t i = 0; i != n; ++i) {
    points[i] = {Coordinate(u32(point_data.data() + 12 * i)),
                 Coordinate(u32(point_data.data() + 12 * i + 4)),
                 Coordinate(u32(point_data.data() + 12 * i + 8))};
    raw_ids[i] = u32(raw_data.data() + 4 * i);
  }
  auto ids_copy = raw_ids;
  std::sort(ids_copy.begin(), ids_copy.end());
  if (std::adjacent_find(ids_copy.begin(), ids_copy.end()) != ids_copy.end())
    throw std::runtime_error("duplicate raw ID");
  auto traced = read_trace(argv[3], false);
  if (!after_folder.empty()) {
    const auto after = read_trace(after_folder, true);
    if (after.size() != traced.size()) throw std::runtime_error("post trace edge count mismatch");
    for (auto& [key, before] : traced) {
      const auto it = after.find(key);
      if (it == after.end() || it->second.core_sites != before.core_sites ||
          it->second.mask != before.mask)
        throw std::runtime_error("post trace edge/pre-mask/core mismatch");
      before.post = it->second.post;
      before.has_post = true;
    }
  }
  const auto index = make_q2_cloud_index(prepare_cloud(points));
  const auto nodes = index->spatial_nodes();
  const auto order = index->spatial_order();
  u64 front_rect = 0, front_mass = 0, open_rect = 0, open_mass = 0;
  u64 rectangle_visits = 0, edges_seen = 0, forms_seen = 0, q3 = 0, q4 = 0;
  u64 empty_open = 0, positive = 0, segment16_q3 = 0, segment16_q4 = 0;
  Stats stats;
  const auto result = run_wspd_front(*index, kmax, 8, WspdFrontMode::MidpointSamples,
      [&](const WspdRectangle& rectangle) {
        const auto a = nodes[rectangle.a_node].range, b = nodes[rectangle.b_node].range;
        const auto product = u64(a.size()) * b.size();
        ++front_rect;
        front_mass += product;
        Q34WitnessSearchWork work{};
        Q34WitnessBoundsWork bounds{};
        const auto mask = filter_q34_witnesses(*index, nodes[rectangle.a_node].box,
            nodes[rectangle.b_node].box, kmax, rectangle.lane_mask, work,
            Q34WitnessBoundsMode::Affine, bounds);
        rectangle_visits += work.node_visits;
        if (mask == 0) return;
        ++open_rect;
        open_mass += product;
        std::vector<Edge> segment;
        for (auto ai = a.first; ai < a.last; ++ai)
          for (auto bi = b.first; bi < b.last; ++bi) {
            const auto id_a = order[ai], id_b = order[bi];
            const auto it = traced.find(edge_key(raw_ids[id_a], raw_ids[id_b]));
            if (it == traced.end()) continue;
            auto& trace = it->second;
            if (trace.seen || (trace.mask & ~mask) != 0)
              throw std::runtime_error("duplicate edge or widened S2 pair mask");
            trace.seen = true;
            segment.push_back({id_a, id_b, &trace});
            ++edges_seen;
            forms_seen += trace.core_sites;
            q3 += (trace.mask & 2U) != 0;
            q4 += (trace.mask & 4U) != 0;
          }
        if (segment.empty()) ++empty_open;
        else ++positive;
        if (segment.size() >= min_segment) {
          for (const auto& edge : segment) {
            segment16_q3 += (edge.trace->mask & 2U) != 0;
            segment16_q4 += (edge.trace->mask & 4U) != 0;
          }
          shadow_segment(*index, rectangle, segment, budget, near_first, raw_ids, stats);
        }
      }, 6);
  if (result.work.emitted_rectangles != front_rect || edges_seen != traced.size())
    throw std::runtime_error("front/trace totals do not match");
  for (const auto& [_, trace] : traced) if (!trace.seen) throw std::runtime_error("unmatched trace edge");
  const auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now() - started).count();
  std::cout << "config " << budget << ' ' << near_first << ' ' << !after_folder.empty() << '\n';
  std::cout << "head " << n << ' ' << front_rect << ' ' << front_mass << ' '
            << open_rect << ' ' << open_mass << ' ' << rectangle_visits << ' '
            << edges_seen << ' ' << forms_seen << ' ' << q3 << ' ' << q4 << ' '
            << empty_open << ' ' << positive << ' ' << segment16_q3 << ' ' << segment16_q4 << '\n';
  std::cout << "shadow " << stats.segments << ' ' << stats.edges << ' ' << stats.forms << ' '
            << stats.closed_segments << ' ' << stats.closed_edges << ' ' << stats.closed_forms << ' '
            << stats.proved_cells << ' ' << stats.budget_cells << ' ' << stats.exhausted_cells << ' '
            << stats.q_terms << ' ' << stats.node_visits << ' ' << stats.node_tests << ' '
            << stats.vertex_tests << ' ' << stats.excluded_node_visits << ' '
            << stats.excluded_subtree_skips << ' '
            << stats.credited_nodes << ' ' << stats.credited_sites << ' '
            << stats.obstructed_cells << ' ' << stats.obstructed_segments << ' '
            << stats.obstructed_forms << ' '
            << stats.post_zero_edges << ' ' << stats.post_nonzero_edges << ' '
            << stats.no_post_edges << ' ' << stats.shadow_ns << ' ' << total_ns << '\n';
  for (unsigned c = 0; c <= cells; ++c)
    std::cout << "cells " << c << ' ' << stats.segments_by_proved_cells[c] << ' '
              << stats.forms_by_proved_cells[c] << '\n';
}
