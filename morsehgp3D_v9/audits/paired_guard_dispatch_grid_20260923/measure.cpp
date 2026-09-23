#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

struct Point {
  std::uint32_t x, y, z;
};
struct Edge {
  std::uint32_t a, b, core_sites, mask;
};
static_assert(sizeof(Point) == 12 && sizeof(Edge) == 16);
static_assert(std::endian::native == std::endian::little);

template <class T>
std::vector<T> read_binary(const std::string& path) {
  std::ifstream in(path, std::ios::binary | std::ios::ate);
  if (!in) throw std::runtime_error("open failed: " + path);
  const auto size = in.tellg();
  if (size < 0 || size % sizeof(T) != 0)
    throw std::runtime_error("invalid file size: " + path);
  in.seekg(0);
  std::vector<T> data(static_cast<std::size_t>(size) / sizeof(T));
  in.read(reinterpret_cast<char*>(data.data()), size);
  if (!in) throw std::runtime_error("read failed: " + path);
  return data;
}

// XYZ is u18. Leave 18 bits per coordinate so all grid levels share a key.
std::uint64_t cell_key(Point p, unsigned bits) {
  return (std::uint64_t(p.x >> bits) << 36) |
         (std::uint64_t(p.y >> bits) << 18) | (p.z >> bits);
}
std::uint64_t edge_key(std::uint32_t a, std::uint32_t b) {
  return (std::uint64_t(a) << 32) | b;
}

struct Totals {
  std::uint64_t edges = 0;
  std::uint64_t heavy_edges = 0;
  std::uint64_t heavy_core_sites = 0;
  std::uint64_t panel_edges = 0;
  std::uint64_t panel_closable = 0;
  std::uint64_t panel_closable_core_sites = 0;
};

struct Group {
  Totals totals;
  std::uint64_t d_min = std::numeric_limits<std::uint64_t>::max();
  std::uint64_t d_max = 0;
  std::array<std::uint32_t, 3> a_low{UINT32_MAX, UINT32_MAX, UINT32_MAX};
  std::array<std::uint32_t, 3> a_high{};
  std::array<std::uint32_t, 3> b_low{UINT32_MAX, UINT32_MAX, UINT32_MAX};
  std::array<std::uint32_t, 3> b_high{};
};

constexpr std::array<unsigned, 5> d_powers{20, 21, 22, 23, 24};
constexpr std::array<unsigned, 11> populations{
    1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};

}  // namespace

int main(int argc, char** argv) {
  if (argc != 5) {
    std::cerr << "usage: measure points.u32le raw_ids.u32le trace_dir panel.tsv\n";
    return 2;
  }
  const auto points = read_binary<Point>(argv[1]);
  const auto ids = read_binary<std::uint32_t>(argv[2]);
  if (points.size() != 123389 || points.size() != ids.size())
    throw std::runtime_error("unexpected cloud size");

  const auto max_id = *std::max_element(ids.begin(), ids.end());
  std::vector<Point> by_id(max_id + 1);
  std::vector<std::uint8_t> present(max_id + 1);
  std::array<std::unordered_map<std::uint64_t, std::uint32_t>, 5> grids;
  for (std::size_t i = 0; i < points.size(); ++i) {
    const Point p = points[i];
    if (p.x >= (1U << 18) || p.y >= (1U << 18) || p.z >= (1U << 18) ||
        present[ids[i]])
      throw std::runtime_error("invalid point or duplicate raw ID");
    present[ids[i]] = 1;
    by_id[ids[i]] = p;
    for (unsigned level = 0; level < grids.size(); ++level)
      ++grids[level][cell_key(p, 10 + level)];
  }

  // The stratified panel is an outcome diagnostic only. It never routes an edge.
  std::unordered_map<std::uint64_t, std::pair<std::uint32_t, bool>> panel;
  std::ifstream panel_file(argv[4]);
  std::uint32_t a, b, core_sites, closable;
  while (panel_file >> a >> b >> core_sites >> closable) {
    if (a >= b || closable > 1 ||
        !panel.emplace(edge_key(a, b),
                       std::make_pair(core_sites, closable != 0)).second)
      throw std::runtime_error("invalid or duplicate panel edge");
  }
  if (panel.size() != 120) throw std::runtime_error("unexpected panel size");

  std::array<std::array<std::array<Totals, populations.size()>, 5>,
             d_powers.size()> totals{};
  // Diagnostic groups for the 4.096 m grid and threshold 1024, at D23/D24.
  std::array<std::unordered_map<std::uint64_t, Group>, 2> groups;
  std::uint64_t edge_count = 0, panel_count = 0;
  std::uint64_t heavy_count = 0, heavy_core_sites = 0, all_core_sites = 0;
  for (unsigned part = 0; part < 8; ++part) {
    const auto trace = read_binary<Edge>(
        std::string(argv[3]) + "/part_" + std::to_string(part) + ".bin");
    for (const Edge e : trace) {
      if (e.a >= e.b || e.a >= present.size() || e.b >= present.size() ||
          !present[e.a] || !present[e.b] || e.core_sites < 2 ||
          (e.mask != 2 && e.mask != 4 && e.mask != 6))
        throw std::runtime_error("invalid trace edge");
      const Point pa = by_id[e.a], pb = by_id[e.b];
      const std::int64_t dx = std::int64_t(pa.x) - pb.x;
      const std::int64_t dy = std::int64_t(pa.y) - pb.y;
      const std::int64_t dz = std::int64_t(pa.z) - pb.z;
      const std::uint64_t D = dx * dx + dy * dy + dz * dz;
      const Point midpoint{(pa.x + pb.x) / 2, (pa.y + pb.y) / 2,
                           (pa.z + pb.z) / 2};
      std::array<std::uint32_t, 5> occupancy{};
      for (unsigned level = 0; level < grids.size(); ++level) {
        const auto it = grids[level].find(cell_key(midpoint, 10 + level));
        if (it != grids[level].end()) occupancy[level] = it->second;
      }
      const auto sample = panel.find(edge_key(e.a, e.b));
      if (sample != panel.end()) {
        if (sample->second.first != e.core_sites)
          throw std::runtime_error("panel/trace core size mismatch");
        ++panel_count;
        std::cout << "S " << e.a << ' ' << e.b << ' ' << e.core_sites << ' '
                  << sample->second.second << ' ' << D;
        for (const auto count : occupancy) std::cout << ' ' << count;
        std::cout << '\n';
      }

      // These are O(1) routing descriptors. core_sites is used only below to
      // assess the selected population after the fact, never for selection.
      for (unsigned di = 0; di < d_powers.size(); ++di) {
        if (D < (std::uint64_t{1} << d_powers[di])) continue;
        for (unsigned level = 0; level < grids.size(); ++level) {
          for (unsigned threshold = 0; threshold < populations.size();
               ++threshold) {
            if (occupancy[level] < populations[threshold]) continue;
            auto& row = totals[di][level][threshold];
            ++row.edges;
            if (e.core_sites >= 1000) {
              ++row.heavy_edges;
              row.heavy_core_sites += e.core_sites;
            }
            if (sample != panel.end()) {
              ++row.panel_edges;
              if (sample->second.second) {
                ++row.panel_closable;
                row.panel_closable_core_sites += e.core_sites;
              }
            }
          }
        }
      }

      if (occupancy[2] >= 1024) {
        unsigned dominant = 0;
        if (std::abs(dy) > std::abs(dx)) dominant = 1;
        if (std::abs(dz) > std::abs(dominant == 0 ? dx : dy))
          dominant = 2;
        const std::uint64_t group =
            (cell_key(midpoint, 12) << 2) | dominant;
        for (unsigned threshold = 0; threshold < groups.size(); ++threshold) {
          if (D < (std::uint64_t{1} << (23 + threshold))) continue;
          auto& box = groups[threshold][group];
          auto& row = box.totals;
          ++row.edges;
          if (e.core_sites >= 1000) {
            ++row.heavy_edges;
            row.heavy_core_sites += e.core_sites;
          }
          box.d_min = std::min(box.d_min, D);
          box.d_max = std::max(box.d_max, D);
          Point left = pa, right = pb;
          if (left.x > right.x ||
              (left.x == right.x && left.y > right.y) ||
              (left.x == right.x && left.y == right.y && left.z > right.z))
            std::swap(left, right);
          const std::array<std::uint32_t, 3> left_xyz{left.x, left.y, left.z};
          const std::array<std::uint32_t, 3> right_xyz{right.x, right.y,
                                                        right.z};
          for (unsigned axis = 0; axis < 3; ++axis) {
            box.a_low[axis] = std::min(box.a_low[axis], left_xyz[axis]);
            box.a_high[axis] = std::max(box.a_high[axis], left_xyz[axis]);
            box.b_low[axis] = std::min(box.b_low[axis], right_xyz[axis]);
            box.b_high[axis] = std::max(box.b_high[axis], right_xyz[axis]);
          }
        }
      }
      ++edge_count;
      all_core_sites += e.core_sites;
      if (e.core_sites >= 1000) {
        ++heavy_count;
        heavy_core_sites += e.core_sites;
      }
    }
  }
  if (edge_count != 3986433 || panel_count != 120)
    throw std::runtime_error("trace/panel count mismatch");

  std::cout << "T " << edge_count << ' ' << heavy_count << ' '
            << heavy_core_sites << ' ' << all_core_sites << '\n';
  for (unsigned level = 0; level < grids.size(); ++level) {
    std::uint32_t largest = 0;
    for (const auto& entry : grids[level])
      largest = std::max(largest, entry.second);
    std::cout << "M " << 10 + level << ' ' << grids[level].size() << ' '
              << largest << '\n';
  }
  std::cout << "H Dpower grid_bits occupancy_min edges heavy_edges "
               "heavy_core_sites panel_edges panel_closable "
               "panel_closable_core_sites\n";
  for (unsigned di = 0; di < d_powers.size(); ++di) {
    for (unsigned level = 0; level < grids.size(); ++level) {
      for (unsigned threshold = 0; threshold < populations.size();
           ++threshold) {
        const auto row = totals[di][level][threshold];
        std::cout << "R " << d_powers[di] << ' ' << 10 + level << ' '
                  << populations[threshold] << ' ' << row.edges << ' '
                  << row.heavy_edges << ' ' << row.heavy_core_sites << ' '
                  << row.panel_edges << ' ' << row.panel_closable << ' '
                  << row.panel_closable_core_sites << '\n';
      }
    }
  }
  for (unsigned threshold = 0; threshold < groups.size(); ++threshold) {
    std::vector<std::uint64_t> keys;
    for (const auto& entry : groups[threshold]) keys.push_back(entry.first);
    std::sort(keys.begin(), keys.end());
    for (const auto key : keys) {
      const auto box = groups[threshold][key];
      const auto row = box.totals;
      std::cout << "G " << 23 + threshold << ' ' << (key >> 2) << ' '
                << (key & 3) << ' ' << row.edges << ' ' << row.heavy_edges
                << ' ' << row.heavy_core_sites << ' ' << box.d_min << ' '
                << box.d_max;
      for (const auto axis : box.a_low) std::cout << ' ' << axis;
      for (const auto axis : box.a_high) std::cout << ' ' << axis;
      for (const auto axis : box.b_low) std::cout << ' ' << axis;
      for (const auto axis : box.b_high) std::cout << ' ' << axis;
      std::cout << '\n';
    }
  }
}
