#include "gen/lanes/q34_witness_search.hpp"
#include "gen/pipeline/prepared_cloud.hpp"
#include "gen/wspd/front.hpp"

#include <algorithm>
#include <array>
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

struct Trace {
  std::uint32_t core_sites{};
  std::uint8_t mask{};
  bool seen{};
};

struct Bucket {
  std::uint64_t rectangles{}, product_pairs{}, surviving_edges{}, core_forms{};
  void add(std::uint64_t product, std::uint64_t surviving, std::uint64_t forms) {
    ++rectangles;
    product_pairs += product;
    surviving_edges += surviving;
    core_forms += forms;
  }
};

struct Segment {
  std::uint64_t product{}, surviving{}, forms{};
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

std::uint64_t edge_key(std::uint32_t a, std::uint32_t b) {
  if (a == b) throw std::runtime_error("self edge");
  if (a > b) std::swap(a, b);
  return (std::uint64_t(a) << 32) | b;
}

unsigned bucket(std::uint64_t n) {
  unsigned b = 0;
  while (n > 1) { n >>= 1; ++b; }
  if (b >= 32) throw std::runtime_error("bucket overflow");
  return b;
}

void dump(const char* label, const std::array<Bucket, 32>& hist) {
  for (unsigned i = 0; i < hist.size(); ++i) {
    const auto& b = hist[i];
    if (b.rectangles)
      std::cout << label << ' ' << i << ' ' << b.rectangles << ' ' << b.product_pairs
                << ' ' << b.surviving_edges << ' ' << b.core_forms << '\n';
  }
}

}  // namespace

int main(int argc, char** argv) {
  // One raw 08/000000 sector/density at a time, common 1 mm, K5/s8, q3+q4.
  if (argc != 4) return 2;
  const auto point_bytes = bytes(argv[1]);
  const auto raw_bytes = bytes(argv[2]);
  if (point_bytes.empty() || point_bytes.size() % 12 ||
      raw_bytes.size() != point_bytes.size() / 3)
    throw std::runtime_error("unaligned input and raw IDs");
  const auto n = point_bytes.size() / 12;
  if (n < 2) throw std::runtime_error("fewer than two sites");
  std::vector<Point3> points(n);
  std::vector<std::uint32_t> raw_ids(n);
  for (std::size_t i = 0; i < n; ++i) {
    points[i] = {Coordinate(u32(point_bytes.data() + 12 * i)),
                 Coordinate(u32(point_bytes.data() + 12 * i + 4)),
                 Coordinate(u32(point_bytes.data() + 12 * i + 8))};
    raw_ids[i] = u32(raw_bytes.data() + 4 * i);
  }
  auto ids_copy = raw_ids;
  std::sort(ids_copy.begin(), ids_copy.end());
  if (std::adjacent_find(ids_copy.begin(), ids_copy.end()) != ids_copy.end())
    throw std::runtime_error("duplicate raw ID");

  std::unordered_map<std::uint64_t, Trace> traced;
  std::uint64_t expected_records = 0;
  for (unsigned part = 0; part < 8; ++part) {
    const auto path = std::filesystem::path(argv[3]) / ("part_" + std::to_string(part) + ".bin");
    const auto size = std::filesystem::file_size(path);
    if (size % 16) throw std::runtime_error("unaligned trace part");
    expected_records += size / 16;
  }
  if (expected_records > std::numeric_limits<std::size_t>::max() / 2)
    throw std::runtime_error("trace too large");
  traced.reserve(static_cast<std::size_t>(expected_records * 2));
  std::uint64_t trace_forms = 0;
  for (unsigned part = 0; part < 8; ++part) {
    const auto path = std::filesystem::path(argv[3]) / ("part_" + std::to_string(part) + ".bin");
    const auto data = bytes(path);
    if (data.size() % 16) throw std::runtime_error("unaligned trace part");
    for (std::size_t off = 0; off < data.size(); off += 16) {
      const auto a = u32(data.data() + off), b = u32(data.data() + off + 4);
      const auto core_sites = u32(data.data() + off + 8), mask = u32(data.data() + off + 12);
      if (a >= b || core_sites < 2 || (mask != 2 && mask != 4 && mask != 6))
        throw std::runtime_error("invalid trace record");
      const auto [_, inserted] = traced.emplace(edge_key(a, b), Trace{core_sites, std::uint8_t(mask), false});
      if (!inserted) throw std::runtime_error("duplicate trace edge");
      trace_forms += core_sites;
    }
  }
  if (traced.size() != expected_records) throw std::runtime_error("trace record count differs");

  const auto index = make_q2_cloud_index(prepare_cloud(points));
  const auto nodes = index->spatial_nodes();
  const auto order = index->spatial_order();
  std::array<Bucket, 32> by_product{}, by_segment{};
  std::vector<Segment> positive;
  positive.reserve(static_cast<std::size_t>(std::min<std::uint64_t>(expected_records, 2000000)));
  std::uint64_t front_rect = 0, front_mass = 0, open_rect = 0, open_mass = 0;
  std::uint64_t survivor_edges = 0, survivor_forms = 0, rectangle_visits = 0;
  std::uint64_t q3_survivors = 0, q4_survivors = 0, empty_open = 0;
  std::uint64_t singleton_open = 0, singleton_survivors = 0, singleton_forms = 0;
  std::uint64_t large_forms = 0, large_edges = 0, large_q3 = 0, large_q4 = 0;
  std::uint64_t segment16_q3 = 0, segment16_q4 = 0;
  const auto result = run_wspd_front(*index, 5, 8, WspdFrontMode::MidpointSamples,
      [&](const WspdRectangle& r) {
        const auto a = nodes[r.a_node].range, b = nodes[r.b_node].range;
        const auto product = std::uint64_t(a.size()) * b.size();
        ++front_rect;
        front_mass += product;
        Q34WitnessSearchWork work{};
        Q34WitnessBoundsWork bounds{};
        const auto mask = filter_q34_witnesses(*index, nodes[r.a_node].box, nodes[r.b_node].box,
            5, r.lane_mask, work, Q34WitnessBoundsMode::Affine, bounds);
        rectangle_visits += work.node_visits;
        if (mask == 0) return;
        ++open_rect;
        open_mass += product;
        std::uint64_t count = 0, forms = 0, rectangle_q3 = 0, rectangle_q4 = 0;
        for (auto ai = a.first; ai < a.last; ++ai) {
          for (auto bi = b.first; bi < b.last; ++bi) {
            const auto key = edge_key(raw_ids[order[ai]], raw_ids[order[bi]]);
            auto it = traced.find(key);
            if (it == traced.end()) continue;
            auto& trace = it->second;
            if (trace.seen || (trace.mask & ~mask) != 0)
              throw std::runtime_error("duplicate edge or widened pair mask");
            trace.seen = true;
            ++count;
            forms += trace.core_sites;
            q3_survivors += (trace.mask & 2U) != 0;
            q4_survivors += (trace.mask & 4U) != 0;
            rectangle_q3 += (trace.mask & 2U) != 0;
            rectangle_q4 += (trace.mask & 4U) != 0;
            if (product >= 1024) {
              ++large_edges;
              large_q3 += (trace.mask & 2U) != 0;
              large_q4 += (trace.mask & 4U) != 0;
            }
          }
        }
        by_product[bucket(product)].add(product, count, forms);
        if (count == 0) ++empty_open;
        else {
          by_segment[bucket(count)].add(product, count, forms);
          positive.push_back({product, count, forms});
        }
        if (count >= 16) {
          segment16_q3 += rectangle_q3;
          segment16_q4 += rectangle_q4;
        }
        if (product == 1) {
          ++singleton_open;
          singleton_survivors += count;
          singleton_forms += forms;
        }
        if (product >= 1024) large_forms += forms;
        survivor_edges += count;
        survivor_forms += forms;
      }, 6);
  if (result.work.emitted_rectangles != front_rect || survivor_edges != traced.size() ||
      survivor_forms != trace_forms)
    throw std::runtime_error("front/filter/core totals do not reproduce pinned receipt");
  for (const auto& [_, row] : traced)
    if (!row.seen) throw std::runtime_error("unmatched traced edge");

  std::cout << "head " << n << ' ' << front_rect << ' ' << front_mass << ' '
            << open_rect << ' ' << open_mass << ' ' << rectangle_visits << ' '
            << survivor_edges << ' ' << survivor_forms << ' ' << q3_survivors << ' '
            << q4_survivors << ' ' << empty_open << ' ' << positive.size() << ' '
            << singleton_open << ' ' << singleton_survivors << ' ' << singleton_forms << ' '
            << large_forms << ' ' << large_edges << ' ' << large_q3 << ' ' << large_q4 << ' '
            << segment16_q3 << ' ' << segment16_q4 << '\n';
  dump("product", by_product);
  dump("segment", by_segment);
  std::sort(positive.begin(), positive.end(), [](const Segment& a, const Segment& b) {
    if (a.forms != b.forms) return a.forms > b.forms;
    if (a.surviving != b.surviving) return a.surviving > b.surviving;
    return a.product > b.product;
  });
  for (const auto& [num, den] : {std::pair{1U, 1000U}, {1U, 100U}, {5U, 100U}, {10U, 100U}}) {
    const auto take = (positive.size() * num + den - 1) / den;
    std::uint64_t forms = 0, edges = 0, product = 0;
    for (std::size_t i = 0; i < take; ++i) {
      forms += positive[i].forms;
      edges += positive[i].surviving;
      product += positive[i].product;
    }
    std::cout << "top " << num << ' ' << den << ' ' << take << ' ' << forms << ' '
              << edges << ' ' << product << '\n';
  }
}
