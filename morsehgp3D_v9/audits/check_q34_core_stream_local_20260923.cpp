// Independent, local ON/OFF comparison of the complete q3/q4 candidate stream.
// Usage: check_q34_core_stream_local_20260923 input.u32le K workers
// This is an audit helper, not a LiDAR qualification or a timing probe.
#include "pipeline/prepared_cloud.hpp"
#include "pipeline/q2_census.hpp"
#include "pipeline/wspd_q34.hpp"

#include <algorithm>
#include <array>
#include <compare>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace mhgp9::gen;

namespace {
struct Record {
  std::array<i128, 5> key{};
  unsigned arity{};
  std::array<std::size_t, 4> support{};
  std::size_t depth{};
  std::vector<std::size_t> shell;
  auto operator<=>(const Record&) const = default;
};

std::vector<Point3> read_points(const char* path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("input open failed");
  std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), {});
  if (bytes.size() % 12 != 0) throw std::runtime_error("not XYZ u32le");
  std::vector<Point3> points(bytes.size() / 12);
  for (std::size_t i = 0; i < points.size(); ++i) {
    std::uint32_t v[3]{};
    for (std::size_t j = 0; j < 3; ++j) {
      for (std::size_t b = 0; b < 4; ++b)
        v[j] |= static_cast<std::uint32_t>(bytes[12 * i + 4 * j + b]) << (8 * b);
      if (v[j] > 262143u) throw std::runtime_error("coordinate exceeds u18");
    }
    points[i] = Point3{static_cast<Coordinate>(v[0]), static_cast<Coordinate>(v[1]),
                       static_cast<Coordinate>(v[2])};
  }
  return points;
}

std::vector<Record> run(Q2CensusIndexPtr index, unsigned k, std::size_t workers,
                        bool core, WspdQ34Work& work) {
  WspdQ34Options o;
  o.front_mode = WspdFrontMode::MidpointSamples;
  o.requested_lane_mask = 6;
  o.q4_backend = WspdQ4Backend::Local28;
  o.local.saturate_deep = true;
  o.local.retain_q3_fragments = true;
  o.witness_mode = WspdQ34WitnessMode::RectanglePair;
  o.q3_census_mode = WspdQ3CensusMode::GlobalBoxes;
  o.witness_bounds_mode = Q34WitnessBoundsMode::Affine;
  o.q4_seed_cells = Q4SeedCellOptions{Q4SeedCellMode::LiveOnly, 64};
  o.q3_atlas_consultation = true;
  o.q3_leaf_census = true;
  o.dead_lanes = true;
  o.pair_witness_cache = true;
  o.dead_core = core;
  std::vector<std::vector<Record>> slots(workers);
  const auto result = run_wspd_q34_parallel(index, k, 8, o, workers,
      [&](std::size_t slot, const Q34SeedCandidate& c) {
        Record r;
        r.key = c.ball.coefficients();
        r.arity = c.arity;
        r.support = c.support_ids;
        r.depth = c.depth;
        r.shell.insert(r.shell.end(), c.shell_first.begin(), c.shell_first.end());
        r.shell.insert(r.shell.end(), c.shell_second.begin(), c.shell_second.end());
        std::sort(r.shell.begin(), r.shell.end());
        slots.at(slot).push_back(std::move(r));
      }, 16);
  work = result.pipeline.work;
  std::size_t total = 0;
  for (const auto& slot : slots) total += slot.size();
  std::vector<Record> records;
  records.reserve(total);
  for (auto& slot : slots)
    records.insert(records.end(), std::make_move_iterator(slot.begin()),
                   std::make_move_iterator(slot.end()));
  std::sort(records.begin(), records.end());
  return records;
}
}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc != 4) throw std::runtime_error("usage: input.u32le K workers");
    const auto k = static_cast<unsigned>(std::stoul(argv[2]));
    const auto workers = static_cast<std::size_t>(std::stoul(argv[3]));
    if (k != 5 && k != 10) throw std::runtime_error("K must be 5 or 10");
    if (workers == 0) throw std::runtime_error("workers must be positive");
    const auto points = read_points(argv[1]);
    const auto index = make_q2_cloud_index(prepare_cloud(points));
    WspdQ34Work off_work{}, on_work{};
    const auto off = run(index, k, workers, false, off_work);
    const auto on = run(index, k, workers, true, on_work);
    const auto equal = off == on;
    std::cout << "sites=" << points.size() << " K=" << k << " workers=" << workers
              << " off_records=" << off.size() << " on_records=" << on.size()
              << " equal=" << (equal ? "true" : "false")
              << " off_q3=" << off_work.q3_emitted << " on_q3=" << on_work.q3_emitted
              << " off_q4=" << off_work.q4_emitted << " on_q4=" << on_work.q4_emitted
              << " core_closed=" << on_work.core_closed_edges << '\n';
    return equal ? 0 : 1;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 2;
  }
}
