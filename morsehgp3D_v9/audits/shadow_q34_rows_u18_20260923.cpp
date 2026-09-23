// Audit-only K5/s8 shadow of row certificates on a complete u18/u32le cloud.
// Build in a v9 checkout with a libmhgp9_gen.a rebuilt from the SAME commit:
//   g++ -std=c++20 -O2 -I morsehgp3D_v9/src -I morsehgp3D_v9/src/gen \
//     morsehgp3D_v9/audits/shadow_q34_rows_u18_20260923.cpp \
//     build/v9-dev/libmhgp9_gen.a -pthread -o /tmp/mhgp9_q34_rows_shadow
// Run with one full.u32le scene path, e.g. scene_00_grid/full.u32le under
// v8/receipts/lidar_ground_20260921/release/. No catalogue, q3/q4 census
// or FULL tower is built.

#include "gen/lanes/q34_witness_search.hpp"
#include "gen/pipeline/prepared_cloud.hpp"
#include "gen/wspd/front.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

using namespace mhgp9::gen;

struct Bucket {
  std::uint64_t queries = 0, pair_mass = 0, full_mass = 0;
  std::uint64_t q3_mass = 0, q4_mass = 0, node_visits = 0;
};

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: mhgp9_q34_rows_shadow full.u32le\n";
    return 2;
  }
  std::ifstream input(argv[1], std::ios::binary);
  if (!input) throw std::runtime_error("cannot open input");
  const std::vector<unsigned char> bytes(
      (std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  if (bytes.empty() || bytes.size() % 12 != 0) throw std::runtime_error("invalid u32le XYZ length");
  std::vector<Point3> points(bytes.size() / 12);
  for (std::size_t i = 0; i < points.size(); ++i) {
    std::uint32_t xyz[3]{};
    for (unsigned axis = 0; axis < 3; ++axis)
      for (unsigned byte = 0; byte < 4; ++byte)
        xyz[axis] |= std::uint32_t(bytes[12 * i + 4 * axis + byte]) << (8 * byte);
    points[i] = {static_cast<Coordinate>(xyz[0]), static_cast<Coordinate>(xyz[1]),
                 static_cast<Coordinate>(xyz[2])};
  }

  const auto index = make_q2_cloud_index(prepare_cloud(points));
  const auto nodes = index->spatial_nodes();
  const auto order = index->spatial_order();
  std::array<Bucket, 4> buckets{};  // |B|: 8..15, 16..31, 32..63, >=64.
  std::uint64_t all_front_mass = 0, eligible_rectangles = 0, pre_mass = 0, post_mass = 0;
  std::uint64_t post_rows = 0, rectangle_visits = 0, selected = 0;
  const auto start = std::chrono::steady_clock::now();

  const auto front = run_wspd_front(*index, 5, 8, WspdFrontMode::MidpointSamples,
      [&](const WspdRectangle& rectangle) {
        const auto ar = nodes[rectangle.a_node].range;
        const auto br = nodes[rectangle.b_node].range;
        all_front_mass += std::uint64_t(ar.size()) * br.size();
        if (ar.size() < 2 || br.size() < 8) return;
        ++eligible_rectangles;
        const auto mass = std::uint64_t(ar.size()) * br.size();
        pre_mass += mass;

        Q34WitnessSearchWork rectangle_work{};
        Q34WitnessBoundsWork rectangle_bounds{};
        const auto mask = filter_q34_witnesses(*index, nodes[rectangle.a_node].box,
            nodes[rectangle.b_node].box, 5, rectangle.lane_mask, rectangle_work,
            Q34WitnessBoundsMode::Affine, rectangle_bounds);
        rectangle_visits += rectangle_work.node_visits;
        if (!mask) return;
        post_mass += mass;
        post_rows += ar.size();

        // Deterministic ~1/128 sample of surviving eligible rectangles,
        // one A row selected by node IDs. Never extrapolate it as a bound.
        const std::uint64_t hash = std::uint64_t(rectangle.a_node) * 0x9e3779b97f4a7c15ULL ^
                                   std::uint64_t(rectangle.b_node) * 0xbf58476d1ce4e5b9ULL;
        if ((hash & 127U) != 0) return;
        ++selected;
        const auto rank = ar.first + (hash >> 7) % ar.size();
        const auto a = points[order[rank]];
        Q34WitnessSearchWork row_work{};
        Q34WitnessBoundsWork row_bounds{};
        const auto row_mask = filter_q34_witnesses(*index, singleton_box(a),
            nodes[rectangle.b_node].box, 5, mask, row_work,
            Q34WitnessBoundsMode::Affine, row_bounds);
        const unsigned bucket = br.size() < 16 ? 0 : br.size() < 32 ? 1 : br.size() < 64 ? 2 : 3;
        auto& out = buckets[bucket];
        ++out.queries;
        out.pair_mass += br.size();
        out.node_visits += row_work.node_visits;
        if (!row_mask) out.full_mass += br.size();
        if ((mask & 2U) && !(row_mask & 2U)) out.q3_mass += br.size();
        if ((mask & 4U) && !(row_mask & 4U)) out.q4_mass += br.size();
      }, 6);
  (void)front;

  const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
  std::cout << "sites " << points.size() << " all_front_mass " << all_front_mass
            << " eligible_rect " << eligible_rectangles
            << " pre_mass " << pre_mass << " post_mass " << post_mass
            << " post_rows " << post_rows << " rectangle_visits " << rectangle_visits
            << " selected " << selected << " elapsed_s " << elapsed << '\n';
  for (unsigned i = 0; i < buckets.size(); ++i) {
    const auto& b = buckets[i];
    std::cout << "Bbucket " << i << " queries " << b.queries << " pair_mass " << b.pair_mass
              << " full_mass " << b.full_mass << " q3_mass " << b.q3_mass
              << " q4_mass " << b.q4_mass << " visits " << b.node_visits << '\n';
  }
}
